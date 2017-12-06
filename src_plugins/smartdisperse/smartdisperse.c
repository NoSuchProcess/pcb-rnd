/*!
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
 *
 * Improve the initial dispersion of elements by choosing an order based
 * on the netlist, rather than the arbitrary element order.  This isn't
 * the same as a global autoplace, it's more of a linear autoplace.  It
 * might make some useful local groupings.  For example, you should not
 * have to chase all over the board to find the resistor that goes with
 * a given LED.
 */

#include <stdio.h>
#include <math.h>

#include <genht/htpi.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "rats.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "plugins.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "compat_nls.h"
#include "obj_subc.h"

#define GAP 10000
static pcb_coord_t minx;
static pcb_coord_t miny;
static pcb_coord_t maxx;
static pcb_coord_t maxy;

/* Place one element or subcircuit.
   Must initialize statics above before calling for the first time.
   This was taken almost entirely from pcb_act_DisperseElements, with cleanup */
static void place_elem(pcb_element_t * element)
{
	pcb_coord_t dx, dy;

	/* figure out how much to move the element */
	dx = minx - element->BoundingBox.X1;
	dy = miny - element->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (element->MarkX + dx) % (long) (PCB->Grid);
	dx += (long) (PCB->Grid);
	dy -= (element->MarkY + dy) % (long) (PCB->Grid);
	dy += (long) (PCB->Grid);

	/* and add one grid size so we make sure we always space by GAP or more */
	dx += (long) (PCB->Grid);

	/* Figure out if this row has room.  If not, start a new row */
	if (minx != GAP && GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
		miny = maxy + GAP;
		minx = GAP;
		place_elem(element); /* recurse can't loop, now minx==GAP */
		return;
	}

	pcb_element_move(PCB->Data, element, dx, dy);
	pcb_undo_add_obj_to_move(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);

	/* keep track of how tall this row is */
	minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
	if (maxy < element->BoundingBox.Y2) {
		maxy = element->BoundingBox.Y2;
	}
}

/* the same for subcircuits */
static void place_subc(pcb_subc_t *sc)
{
	pcb_coord_t dx, dy, ox = 0, oy = 0;

	pcb_subc_get_origin(sc, &ox, &oy);

	/* figure out how much to move the element */
	dx = minx - sc->BoundingBox.X1;
	dy = miny - sc->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (ox + dx) % PCB->Grid;
	dx += PCB->Grid;
	dy -= (oy + dy) % PCB->Grid;
	dy += PCB->Grid;

	/* and add one grid size so we make sure we always space by GAP or more */
	dx += PCB->Grid;

	/* Figure out if this row has room.  If not, start a new row */
	if (minx != GAP && GAP + sc->BoundingBox.X2 + dx > PCB->MaxWidth) {
		miny = maxy + GAP;
		minx = GAP;
		place_subc(sc); /* recurse can't loop, now minx==GAP */
		return;
	}

	pcb_subc_move(sc, dx, dy, 1);
	pcb_undo_add_obj_to_move(PCB_TYPE_SUBC, NULL, NULL, sc, dx, dy);

	/* keep track of how tall this row is */
	minx += sc->BoundingBox.X2 - sc->BoundingBox.X1 + GAP;
	if (maxy < sc->BoundingBox.Y2) {
		maxy = sc->BoundingBox.Y2;
	}
}

static void place(pcb_any_obj_t *obj)
{
	switch(obj->type) {
		case PCB_OBJ_ELEMENT: place_elem((pcb_element_t *)obj); break;
		case PCB_OBJ_SUBC:    place_subc((pcb_subc_t *)obj); break;
		default: break;
	}
}


/* Return the X location of a connection's pad or pin within its element. */
static pcb_coord_t padDX(pcb_connection_t * conn)
{
	pcb_element_t *element = (pcb_element_t *) conn->ptr1;
	pcb_any_line_t *line = (pcb_any_line_t *) conn->obj;

	return line->BoundingBox.X1 - (element->BoundingBox.X1 + element->BoundingBox.X2) / 2;
}

/* Return true if ea,eb would be the best order, else eb,ea, based on pad loc. */
static int padorder(pcb_connection_t * conna, pcb_connection_t * connb)
{
	pcb_coord_t dxa, dxb;

	dxa = padDX(conna);
	dxb = padDX(connb);
	/* there are other cases that merit rotation, ignore them for now */
	if (dxa > 0 && dxb < 0)
		return 1;
	return 0;
}

/* ewww, these are actually arrays */
#define ELEMENT_N(DATA,ELT)	((ELT) - (DATA)->Element)
#define VISITED(ELT)		(visited[ELEMENT_N(PCB->Data, (ELT))])
#define IS_ELEMENT(CONN)	((CONN)->obj->type == PCB_OBJ_PAD || (CONN)->obj->type == PCB_OBJ_PIN)

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char smartdisperse_syntax[] = "SmartDisperse([All|Selected])";

#define set_visited(obj) htpi_set(&visited, ((void *)(obj)), 1)
#define is_visited(obj)  htpi_has(&visited, ((void *)(obj)))


static int smartdisperse(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = ARG(0);
	pcb_netlist_t *Nets;
	htpi_t visited;
	int all;

	if (!function) {
		all = 1;
	}
	else if (strcmp(function, "All") == 0) {
		all = 1;
	}
	else if (strcmp(function, "Selected") == 0) {
		all = 0;
	}
	else {
		PCB_AFAIL(smartdisperse);
	}

	Nets = pcb_rat_proc_netlist(&PCB->NetlistLib[0]);
	if (!Nets) {
		pcb_message(PCB_MSG_ERROR, _("Can't use SmartDisperse because no netlist is loaded.\n"));
		return 0;
	}

	/* remember which elements we finish with */
	htpi_init(&visited, ptrhash, ptrkeyeq);

	/* if we're not doing all, mark the unselected elements as "visited" */
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (!(all || PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))) {
			set_visited(element);
		}
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
	PCB_NET_LOOP(Nets);
	{
		pcb_connection_t *conna, *connb;
		pcb_any_obj_t *ea, *eb;

		if (net->ConnectionN != 2)
			continue;

		conna = &net->Connection[0];
		connb = &net->Connection[1];
		if (!IS_ELEMENT(conna) || !IS_ELEMENT(conna))
			continue;

		ea = (pcb_any_obj_t *) conna->ptr1;
		eb = (pcb_any_obj_t *) connb->ptr1;

		/* place this pair if possible */
		if (is_visited(ea) || is_visited(eb))
			continue;
		set_visited(ea);
		set_visited(eb);

		/* a weak attempt to get the linked pads side-by-side */
		if (padorder(conna, connb)) {
			place(ea);
			place(eb);
		}
		else {
			place(eb);
			place(ea);
		}
	}
	PCB_END_LOOP;

	/* Place larger nets, still grouping by net */
	PCB_NET_LOOP(Nets);
	{
		PCB_CONNECTION_LOOP(net);
		{
			pcb_any_obj_t *parent;

			if (!IS_ELEMENT(connection))
				continue;

			parent = connection->ptr1;

			/* place this one if needed */
			if (is_visited(parent))
				continue;
			set_visited(parent);
			place(parent);
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	/* Place up anything else */
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (!is_visited(element))
			place_elem(element);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!is_visited(subc))
			place_subc(subc);
	}
	PCB_END_LOOP;

	htpi_uninit(&visited);

	pcb_undo_inc_serial();
	pcb_redraw();
	pcb_board_set_changed_flag(1);

	return 0;
}

static pcb_hid_action_t smartdisperse_action_list[] = {
	{"smartdisperse", NULL, smartdisperse, NULL, NULL}
};

char *smartdisperse_cookie = "smartdisperse plugin";

PCB_REGISTER_ACTIONS(smartdisperse_action_list, smartdisperse_cookie)

int pplg_check_ver_smartdisperse(int ver_needed) { return 0; }

void pplg_uninit_smartdisperse(void)
{
	pcb_hid_remove_actions_by_cookie(smartdisperse_cookie);
}

#include "dolists.h"
int pplg_init_smartdisperse(void)
{
	PCB_REGISTER_ACTIONS(smartdisperse_action_list, smartdisperse_cookie);
	return 0;
}
