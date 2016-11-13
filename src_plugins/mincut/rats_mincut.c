/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2013..2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "const.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "plug_io.h"
#include "find.h"
#include "polygon.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "plugins.h"
#include "compat_misc.h"

#include "rats.h"
#include "pcb-mincut/graph.h"
#include "pcb-mincut/solve.h"

#include "conf.h"
#include "rats_mincut_conf.h"
conf_mincut_t conf_mincut;

static void debprintf(const char *fmt, ...) {}

typedef struct short_conn_s short_conn_t;
struct short_conn_s {
	int gid;											/* id in the graph */
	int from_type;
/*	pcb_any_obj_t *from;*/
	int from_id;
	int to_type;
	int edges;										/* number of edges */
	pcb_any_obj_t *to;
	pcb_found_conn_type_t type;
	short_conn_t *next;
};

static short_conn_t *short_conns = NULL;
static int num_short_conns = 0;
static int short_conns_maxid = 0;

static void proc_short_cb(int current_type, void *current_obj, int from_type, void *from_obj, pcb_found_conn_type_t type)
{
	pcb_any_obj_t *curr = current_obj, *from = from_obj;
	short_conn_t *s;

	s = malloc(sizeof(short_conn_t));
	if (from != NULL) {
		s->from_type = from_type;
		s->from_id = from->ID;
	}
	else {
		s->from_type = 0;
		s->from_id = -1;
	}
	s->to_type = current_type;
	s->to = curr;
	s->type = type;
	s->edges = 0;
	s->next = short_conns;
	short_conns = s;
	if (curr->ID > short_conns_maxid)
		short_conns_maxid = curr->ID;
	num_short_conns++;

	debprintf(" found %d %d/%p type %d from %d\n", current_type, curr->ID, (void *)current_obj, type, from == NULL ? -1 : from->ID);
}

/* returns 0 on succes */
static int proc_short(pcb_pin_t * pin, pcb_pad_t * pad, int ignore)
{
	find_callback_t old_cb;
	pcb_coord_t x, y;
	short_conn_t *n, **lut_by_oid, **lut_by_gid, *next;
	int gids;
	gr_t *g;
	void *S, *T;
	int *solution;
	int i, maxedges;
	int bad_gr = 0;

	if (!conf_mincut.plugins.mincut.enable)
		return bad_gr;

	/* only one should be set, but one must be set */
	assert((pin != NULL) || (pad != NULL));
	assert((pin == NULL) || (pad == NULL));

	if (pin != NULL) {
		debprintf("short on pin!\n");
		SET_FLAG(PCB_FLAG_WARN, pin);
		x = pin->X;
		y = pin->Y;
	}
	else if (pad != NULL) {
		debprintf("short on pad!\n");
		SET_FLAG(PCB_FLAG_WARN, pad);
		if (TEST_FLAG(PCB_FLAG_EDGE2, pad)) {
			x = pad->Point2.X;
			y = pad->Point2.Y;
		}
		else {
			x = pad->Point1.X;
			y = pad->Point1.Y;
		}
	}

	/* run only if net is not ignored */
	if (ignore)
		return 0;

	short_conns = NULL;
	num_short_conns = 0;
	short_conns_maxid = 0;

	/* perform a search using PCB_FLAG_MINCUT, calling back proc_short_cb() with the connections */
	old_cb = find_callback;
	find_callback = proc_short_cb;
	SaveFindFlag(PCB_FLAG_MINCUT);
	pcb_lookup_conn(x, y, pcb_false, 1, PCB_FLAG_MINCUT);

	debprintf("- alloced for %d\n", (short_conns_maxid + 1));
	lut_by_oid = calloc(sizeof(short_conn_t *), (short_conns_maxid + 1));
	lut_by_gid = calloc(sizeof(short_conn_t *), (num_short_conns + 3));

	g = gr_alloc(num_short_conns + 2);

	g->node2name = calloc(sizeof(char *), (num_short_conns + 2));

	/* conn 0 is S and conn 1 is T and set up lookup arrays */
	for (n = short_conns, gids = 2; n != NULL; n = n->next, gids++) {
		char *s;
		const char *typ;
		pcb_element_t *parent;
		n->gid = gids;
		debprintf(" {%d} found %d %d/%p type %d from %d\n", n->gid, n->to_type, n->to->ID, (void *)n->to, n->type, n->from_id);
		lut_by_oid[n->to->ID] = n;
		lut_by_gid[n->gid] = n;

		s = malloc(256);
		parent = NULL;
		switch (n->to_type) {
		case PCB_TYPE_PIN:
			typ = "pin";
			parent = ((pcb_pin_t *) (n->to))->Element;
			break;
		case PCB_TYPE_VIA:
			typ = "via";
			parent = ((pcb_pin_t *) (n->to))->Element;
			break;
		case PCB_TYPE_PAD:
			typ = "pad";
			parent = ((pcb_pad_t *) (n->to))->Element;
			break;
		case PCB_TYPE_LINE:
			typ = "line";
			break;
		default:
			typ = "other";
			break;
		}
		if (parent != NULL) {
			pcb_text_t *name;
			name = &parent->Name[1];
			if ((name->TextString == NULL) || (*name->TextString == '\0'))
				sprintf(s, "%s #%ld \\nof #%ld", typ, n->to->ID, parent->ID);
			else
				sprintf(s, "%s #%ld \\nof %s", typ, n->to->ID, name->TextString);
		}
		else
			sprintf(s, "%s #%ld", typ, n->to->ID);
		g->node2name[n->gid] = s;
	}
	g->node2name[0] = pcb_strdup("S");
	g->node2name[1] = pcb_strdup("T");

	/* calculate how many edges each node has and the max edge count */
	maxedges = 0;
	for (n = short_conns; n != NULL; n = n->next) {
		short_conn_t *from;

		n->edges++;
		if (n->edges > maxedges)
			maxedges = n->edges;

		if (n->from_id >= 0) {
			from = lut_by_oid[n->from_id];
			if (from == NULL) {
				/* no from means broken graph (multiple components) */
				if (n->from_id >= 2) {	/* ID 0 and 1 are start/stop, there won't be from for them */
					fprintf(stderr, "rats_mincut.c error: graph has multiple components, bug in find.c (n->from_id=%d)!\n", n->from_id);
					bad_gr = 1;
				}
				continue;
			}
			from->edges++;
			if (from->edges > maxedges)
				maxedges = from->edges;
		}
	}


	S = NULL;
	T = NULL;
	for (n = short_conns; n != NULL; n = n->next) {
		void *spare;

		spare = NULL;
		if (n->to_type == PCB_TYPE_PIN)
			spare = ((pcb_pin_t *) n->to)->Spare;
		if (n->to_type == PCB_TYPE_PAD)
			spare = ((pcb_pad_t *) n->to)->Spare;
		if (spare != NULL) {
			void *net = &(((pcb_lib_menu_t *) spare)->Name[2]);
			debprintf(" net=%s\n", net);
			if (S == NULL) {
				debprintf(" -> became S\n");
				S = net;
			}
			else if ((T == NULL) && (net != S)) {
				debprintf(" -> became T\n");
				T = net;
			}

			if (net == S)
				gr_add_(g, n->gid, 0, 100000);
			else if (net == T)
				gr_add_(g, n->gid, 1, 100000);
		}
		/* if we have a from object, look it up and make a connection between the two gids */
		if (n->from_id >= 0) {
			int weight;
			short_conn_t *from = lut_by_oid[n->from_id];

			from = lut_by_oid[n->from_id];
			/* weight: 1 for connections we can break, large value for connections we shall not break */
			if ((n->type == FCT_COPPER) || (n->type == FCT_START)) {
				/* connection to a pin/pad is slightly stronger than the
				   strongest obj-obj conn; obj-obj conns are weaker at junctions where many
				   objects connect */
				if ((n->from_type == PCB_TYPE_PIN) || (n->from_type == PCB_TYPE_PAD) || (n->to_type == PCB_TYPE_PIN) || (n->to_type == PCB_TYPE_PAD))
					weight = maxedges * 2 + 2;
				else
					weight = maxedges * 2 - n->edges - from->edges + 1;
			}
			else
				weight = 10000;
			if (from != NULL) {
				gr_add_(g, n->gid, from->gid, weight);
				debprintf(" CONN %d %d\n", n->gid, from->gid);
			}
		}
	}

/*#define MINCUT_DRAW*/
#ifdef MINCUT_DRAW
	{
		static int drw = 0;
		char gfn[256];
		drw++;
		sprintf(gfn, "A_%d_a", drw);
		debprintf("gfn=%s\n", gfn);
		gr_draw(g, gfn, "png");
	}
#endif

	if (!bad_gr) {
		solution = solve(g);

		if (solution != NULL) {
			debprintf("Would cut:\n");
			for (i = 0; solution[i] != -1; i++) {
				short_conn_t *s;
				debprintf("%d:", i);
				s = lut_by_gid[solution[i]];
				debprintf("%d %p", solution[i], (void *)s);
				if (s != NULL) {
					SET_FLAG(PCB_FLAG_WARN, s->to);
					debprintf("  -> %d", s->to->ID);
				}
				debprintf("\n");
			}

			free(solution);
		}
		else {
			fprintf(stderr, "mincut didn't find a solution, falling back to the old warn\n");
			bad_gr = 1;
		}
	}
	free(lut_by_oid);
	free(lut_by_gid);

	for (n = short_conns; n != NULL; n = next) {
		next = n->next;
		free(n);
	}


	ResetFoundLinesAndPolygons(pcb_false);
	ResetFoundPinsViasAndPads(pcb_false);
	RestoreFindFlag();

	find_callback = old_cb;
	return bad_gr;
}

typedef struct pinpad_s pinpad_t;
struct pinpad_s {
	int ignore;										/* if 1, changed our mind, do not check */
	pcb_pin_t *pin;
	pcb_pad_t *pad;
	const char *with_net;					/* the name of the net this pin/pad is in short with */
	pinpad_t *next;
};

static pinpad_t *shorts = NULL;

void rat_found_short(pcb_pin_t * pin, pcb_pad_t * pad, const char *with_net)
{
	pinpad_t *pp;
	pp = malloc(sizeof(pinpad_t));
	pp->ignore = 0;
	pp->pin = pin;
	pp->pad = pad;
	pp->with_net = with_net;
	pp->next = shorts;
	shorts = pp;
}

void rat_proc_shorts(void)
{
	pinpad_t *n, *i, *next;
	int bad_gr = 0;
	for (n = shorts; n != NULL; n = next) {
		next = n->next;

		if (n->pin != NULL)
			SET_FLAG(PCB_FLAG_WARN, n->pin);
		if (n->pad != NULL)
			SET_FLAG(PCB_FLAG_WARN, n->pad);


		/* run only if net is not ignored */
		if ((!bad_gr) && (proc_short(n->pin, n->pad, n->ignore) != 0)) {
			fprintf(stderr, "Can't run mincut :(\n");
			bad_gr = 1;
		}

		if (!bad_gr) {
			/* check if the rest of the shorts affect the same nets - ignore them if so */
			for (i = n->next; i != NULL; i = i->next) {
				pcb_lib_menu_t *spn, *spi;
				spn = (n->pin != NULL) ? n->pin->Spare : n->pad->Spare;
				spi = (i->pin != NULL) ? i->pin->Spare : i->pad->Spare;

				if ((spn == NULL) || (spi == NULL))
					continue;

				/* can compare by pointer - names are never pcb_strdup()'d */
				if ((&spn->Name[2] == i->with_net) || (&spi->Name[2] == n->with_net))
					i->ignore = 1;
			}
		}
		free(n);
	}
	shorts = NULL;
}

void hid_mincut_uninit(void)
{
	conf_unreg_fields("plugins/mincut/");
}

#include "stub_mincut.h"
pcb_uninit_t hid_mincut_init(void)
{
	stub_rat_found_short = rat_found_short;
	stub_rat_proc_shorts = rat_proc_shorts;
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_mincut, field,isarray,type_name,cpath,cname,desc,flags);
#include "rats_mincut_conf_fields.h"
	return hid_mincut_uninit;
}
