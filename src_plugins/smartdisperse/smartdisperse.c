/*
 * Smartdisperse plug-in for PCB.
 *
 * Copyright (C) 2007 Ben Jackson <ben@ben.com> based on
 * teardrops.c by Copyright (C) 2006 DJ Delorie <dj@delorie.com> as well
 * as the original action.c, and autoplace.c.
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 * Upgraded to subc by Tibor 'Igor2' Palinkas in 2017.
 * Upgraded to the new netlist API by Tibor 'Igor2' Palinkas in 2019.
 *
 */

#include <stdio.h>
#include <math.h>

#include <genht/htpi.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include <librnd/core/hid.h>
#include <librnd/poly/rtree.h>
#include "undo.h"
#include "netlist.h"
#include <librnd/core/error.h>
#include "move.h"
#include "draw.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "obj_subc.h"
#include "obj_subc_parent.h"
#include "obj_term.h"
#include "funchash_core.h"

#define GAP 10000
static rnd_coord_t minx;
static rnd_coord_t miny;
static rnd_coord_t maxx;
static rnd_coord_t maxy;

/* the same for subcircuits
   Must initialize statics above before calling for the first time.
   This was taken almost entirely from pcb_act_DisperseElements, with cleanup */
static void place_subc(pcb_subc_t *sc)
{
	rnd_coord_t dx, dy, ox = 0, oy = 0;

	pcb_subc_get_origin(sc, &ox, &oy);

	/* figure out how much to move the subcircuit */
	dx = minx - sc->BoundingBox.X1;
	dy = miny - sc->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (ox + dx) % PCB->hidlib.grid;
	dx += PCB->hidlib.grid;
	dy -= (oy + dy) % PCB->hidlib.grid;
	dy += PCB->hidlib.grid;

	/* and add one grid size so we make sure we always space by GAP or more */
	dx += PCB->hidlib.grid;

	/* Figure out if this row has room.  If not, start a new row */
	if (minx != GAP && GAP + sc->BoundingBox.X2 + dx > PCB->hidlib.size_x) {
		miny = maxy + GAP;
		minx = GAP;
		place_subc(sc); /* recurse can't loop, now minx==GAP */
		return;
	}

	pcb_subc_move(sc, dx, dy, 1);
	pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, NULL, NULL, sc, dx, dy);

	/* keep track of how tall this row is */
	minx += sc->BoundingBox.X2 - sc->BoundingBox.X1 + GAP;
	if (maxy < sc->BoundingBox.Y2)
		maxy = sc->BoundingBox.Y2;
}


/* Return true if term1,term2 would be the best order, else term2,term1, based on pad loc. */
static int padorder(pcb_subc_t *parent1, pcb_any_obj_t *term1, pcb_subc_t *parent2, pcb_any_obj_t *term2)
{
	rnd_coord_t x1, x2;

	x1 = term1->BoundingBox.X1 - (parent1->BoundingBox.X1 + parent1->BoundingBox.X2) / 2;
	x2 = term2->BoundingBox.X1 - (parent2->BoundingBox.X1 + parent2->BoundingBox.X2) / 2;

	return (x1 > 0) && (x2 < 0);
}

#define IS_IN_SUBC(CONN)  (pcb_obj_parent_subc((CONN)->obj) != NULL)

#define set_visited(obj) htpi_set(&visited, ((void *)(obj)), 1)
#define is_visited(obj)  htpi_has(&visited, ((void *)(obj)))

static const char pcb_acts_smartdisperse[] = "SmartDisperse([All|Selected])";
static const char pcb_acth_smartdisperse[] = "Disperse subcircuits into clusters, by netlist connections";
/* DOC: smartdisperse.html */
static fgw_error_t pcb_act_smartdisperse(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = -2;
	pcb_netlist_t *nl = &PCB->netlist[PCB_NETLIST_EDITED];
	htpi_t visited;
	int all;
	htsp_entry_t *e;

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, smartdisperse, op = fgw_keyword(&argv[1]));

	switch(op) {
		case -2:
		case F_All: all = 1; break;
		case F_Selected: all = 0; break;
		default:
			RND_ACT_FAIL(smartdisperse);
	}

	if (nl->used == 0) {
		rnd_message(RND_MSG_ERROR, "Can't use SmartDisperse because no netlist is loaded.\n");
		RND_ACT_IRES(1);
		return 0;
	}

	/* remember which subcircuits we finish with */
	htpi_init(&visited, ptrhash, ptrkeyeq);

	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!(all || PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)))
			set_visited(subc);
	}
	PCB_END_LOOP;

	/* initialize variables for place() */
	minx = GAP;
	miny = GAP;
	maxx = GAP;
	maxy = GAP;

	/* Pick nets with two connections.  This is the start of a more
	 * elaborate algorithm to walk serial nets, but the datastructures
	 * are too gross so I'm going with the 80% solution. */
	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *t1, *t2;
		pcb_any_obj_t *to1, *to2;
		pcb_subc_t *ea, *eb;

		if (pcb_termlist_length(&net->conns) != 2)
			continue;

		t1 = pcb_termlist_first(&net->conns);
		t2 = pcb_termlist_next(t1);
		to1 = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t1->refdes, t1->term, NULL, NULL);
		to2 = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t2->refdes, t2->term, NULL, NULL);

		if ((to1 == NULL) || (to2 == NULL))
			continue;

		ea = pcb_obj_parent_subc(to1);
		eb = pcb_obj_parent_subc(to2);

		/* place this pair if possible */
		if (is_visited(ea) || is_visited(eb))
			continue;
		set_visited(ea);
		set_visited(eb);

		/* a weak attempt to get the linked pads side-by-side */
		if (padorder(ea, to1, eb, to2)) {
			place_subc(ea);
			place_subc(eb);
		}
		else {
			place_subc(eb);
			place_subc(ea);
		}
	}

	/* Place larger nets, still grouping by net */
	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_term_t *t;
		pcb_net_t *net = e->value;

		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
			pcb_subc_t *parent;
			pcb_any_obj_t *to;

			to = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
			if (to == NULL)
				continue;

			parent = pcb_obj_parent_subc(to);
			if (parent == NULL)
				continue;

			/* place this one if needed */
			if (is_visited(parent))
				continue;
			set_visited(parent);
			place_subc(parent);
		}
	}

	/* Place up anything else */
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!is_visited(subc))
			place_subc(subc);
	}
	PCB_END_LOOP;

	htpi_uninit(&visited);

	pcb_undo_inc_serial();
	rnd_hid_redraw(PCB);
	pcb_board_set_changed_flag(PCB, 1);

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t smartdisperse_action_list[] = {
	{"smartdisperse", pcb_act_smartdisperse, pcb_acth_smartdisperse, pcb_acts_smartdisperse}
};

char *smartdisperse_cookie = "smartdisperse plugin";

int pplg_check_ver_smartdisperse(int ver_needed) { return 0; }

void pplg_uninit_smartdisperse(void)
{
	rnd_remove_actions_by_cookie(smartdisperse_cookie);
}

int pplg_init_smartdisperse(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(smartdisperse_action_list, smartdisperse_cookie);
	return 0;
}
