/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2013..2015,2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "event.h"
#include "plug_io.h"
#include "find.h"
#include "polygon.h"
#include "search.h"
#include "undo.h"
#include "plugins.h"
#include "netlist2.h"
#include "compat_misc.h"
#include "obj_common.h"
#include "obj_subc_parent.h"

#include "rats.h"
#include "pcb-mincut/graph.h"
#include "pcb-mincut/solve.h"

#include "conf.h"
#include "rats_mincut_conf.h"
conf_mincut_t conf_mincut;

static const char *pcb_mincut_cookie = "mincut";

/* define to 1 to enable debug prints */
#if 0
#	define debprintf pcb_trace
#else
	static void debprintf(const char *fmt, ...) {}
#endif

typedef struct short_conn_s short_conn_t;
struct short_conn_s {
	int gid;											/* id in the graph */
	int from_type;
	int from_id;
	int to_type;
	int edges;										/* number of edges */
	pcb_any_obj_t *to;
	pcb_found_conn_type_t type;
	short_conn_t *next;
};

TODO("netlist: remove this, use the find context")
static short_conn_t *short_conns = NULL;
static int num_short_conns = 0;
static int short_conns_maxid = 0;

static int proc_short_cb(pcb_find_t *fctx, pcb_any_obj_t *curr, pcb_any_obj_t *from, pcb_found_conn_type_t type)
{
	short_conn_t *s;

	s = malloc(sizeof(short_conn_t));
	if (from != NULL) {
		s->from_type = from->type;
		s->from_id = from->ID;
	}
	else {
		s->from_type = 0;
		s->from_id = -1;
	}
	s->to_type = curr->type;
	s->to = curr;
	s->type = type;
	s->edges = 0;
	s->next = short_conns;
	short_conns = s;
	if (curr->ID > short_conns_maxid)
		short_conns_maxid = curr->ID;
	num_short_conns++;

	debprintf(" found %d %d/%p type=%d from %d\n", curr->type, curr->ID, (void *)curr, s->type, from == NULL ? -1 : from->ID);
	return 0;
}

/* returns 0 on succes */
static int proc_short(pcb_any_obj_t *term, int ignore, pcb_net_t *Snet, pcb_net_t *Tnet)
{
	pcb_coord_t x, y;
	short_conn_t *n, **lut_by_oid, **lut_by_gid, *next;
	int gids;
	gr_t *g;
	void *S, *T;
	int *solution;
	int i, maxedges, nonterms = 0;
	int bad_gr = 0;
	pcb_find_t fctx;

TODO("remove this check from here, handled at the caller");
	if (!conf_mincut.plugins.mincut.enable)
		return bad_gr;

	pcb_obj_center(term, &x, &y);
	debprintf("short on terminal\n");

	/* run only if net is not ignored */
	if (ignore)
		return 0;

	short_conns = NULL;
	num_short_conns = 0;
	short_conns_maxid = 0;

	/* perform a search calling back proc_short_cb() with the connections */
	memset(&fctx, 0, sizeof(fctx));
	fctx.found_cb = proc_short_cb;
	pcb_find_from_obj(&fctx, PCB->Data, term);
	pcb_find_free(&fctx);


	debprintf("- alloced for %d\n", (short_conns_maxid + 1));
	lut_by_oid = calloc(sizeof(short_conn_t *), (short_conns_maxid + 1));
	lut_by_gid = calloc(sizeof(short_conn_t *), (num_short_conns + 3));

	g = gr_alloc(num_short_conns + 2);

	g->node2name = calloc(sizeof(char *), (num_short_conns + 2));

	/* conn 0 is S and conn 1 is T and set up lookup arrays */
	for (n = short_conns, gids = 2; n != NULL; n = n->next, gids++) {
		char *s;
		const char *typ;
		pcb_subc_t *parent;
		pcb_any_obj_t *o = (pcb_any_obj_t *)n->to;

		n->gid = gids;
		debprintf(" {%d} found %d %d/%p type=%d from %d\n", n->gid, n->to_type, n->to->ID, (void *)n->to, n->type, n->from_id);
		lut_by_oid[n->to->ID] = n;
		lut_by_gid[n->gid] = n;

		parent = NULL;
		switch (n->to_type) {
			case PCB_OBJ_LINE: typ = "line";       parent = pcb_lobj_parent_subc(o->parent_type, &o->parent); break;
			case PCB_OBJ_ARC:  typ = "arc";        parent = pcb_lobj_parent_subc(o->parent_type, &o->parent); break;
			case PCB_OBJ_POLY: typ = "polygon";    parent = pcb_lobj_parent_subc(o->parent_type, &o->parent); break;
			case PCB_OBJ_TEXT: typ = "text";       parent = pcb_lobj_parent_subc(o->parent_type, &o->parent); break;
			case PCB_OBJ_PSTK: typ = "padstack";   parent = pcb_gobj_parent_subc(o->parent_type, &o->parent); break;
			case PCB_OBJ_SUBC: typ = "subcircuit"; parent = pcb_gobj_parent_subc(o->parent_type, &o->parent); break;
			default: typ = "<unknown>";             parent = NULL; break;
		}
		if (parent != NULL) {
			if (parent->refdes == NULL)
				s = pcb_strdup_printf("%s #%ld \\nof #%ld", typ, n->to->ID, parent->ID);
			else
				s = pcb_strdup_printf("%s #%ld \\nof %s", typ, n->to->ID, parent->refdes);
		}
		else
			s = pcb_strdup_printf("%s #%ld", typ, n->to->ID);

		g->node2name[n->gid] = s;
		if ((o->term == NULL) && (o->type != PCB_OBJ_RAT))
			nonterms++;
	}

	/* Shortcut: do not even attempt to do the mincut if there are no objects
	   between S and T that could be cut. This is a typical case when a lot of
	   parts are thrown on the board with overlapping terminals after an initial
	   import sch. */
	if (nonterms < 1)
		bad_gr = 1;

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

	if (Snet != NULL) {
		S = pcb_netlist_lookup(1, Snet->name, 0);
		assert(S != NULL);
	}
	if (Tnet != NULL) {
		T = pcb_netlist_lookup(1, Tnet->name, 0);
		assert(T != NULL);
	}

	for (n = short_conns; n != NULL; n = n->next) {
		if (Snet != NULL) {
			pcb_any_obj_t *o = (pcb_any_obj_t *)n->to;
			if (o->term != NULL) {
				pcb_subc_t *sc = pcb_obj_parent_subc(o);
				if (sc != NULL) {
					pcb_net_term_t *t = pcb_net_find_by_refdes_term(&PCB->netlist[PCB_NETLIST_EDITED], sc->refdes, o->term);
					if (t != NULL) {
						if (t->parent.net == Snet)
							gr_add_(g, n->gid, 0, 100000);
						else if (t->parent.net == Tnet)
							gr_add_(g, n->gid, 1, 100000);
					}
				}
			}
		}
		else {
		void *spare = ((pcb_any_obj_t *) n->to)->ratconn;
		if (spare != NULL) { /* REMOVE THIS when the old netlist is removed */
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
		}

		/* if we have a from object, look it up and make a connection between the two gids */
		if (n->from_id >= 0) {
			int weight;
			short_conn_t *from = lut_by_oid[n->from_id];

			from = lut_by_oid[n->from_id];
			/* weight: 1 for connections we can break, large value for connections we shall not break */
			if ((n->type == PCB_FCT_COPPER) || (n->type == PCB_FCT_START)) {
				/* connection to a pin/pad is slightly stronger than the
				   strongest obj-obj conn; obj-obj conns are weaker at junctions where many
				   objects connect */
				if ((n->from_type == PCB_OBJ_PSTK) || (n->to_type == PCB_OBJ_PSTK))
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
		solution = solve(g, pcb_hid_progress);

		if (solution != NULL) {
			debprintf("Would cut:\n");
			for (i = 0; solution[i] != -1; i++) {
				short_conn_t *s;
				debprintf("%d:", i);
				s = lut_by_gid[solution[i]];
				debprintf("%d %p", solution[i], (void *)s);
				if (s != NULL) {
					PCB_FLAG_SET(PCB_FLAG_WARN, s->to);
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

	for(i = 0; i < g->n; i++)
		free(g->node2name[i]);
	free(g->node2name);
	gr_free(g);

	return bad_gr;
}

typedef struct pinpad_s pinpad_t;
struct pinpad_s {
	int ignore;										/* if 1, changed our mind, do not check */
	pcb_any_obj_t *term;
	const char *with_net;					/* the name of the net this pin/pad is in short with */
	pinpad_t *next;
};

static pinpad_t *shorts = NULL;

void rat_found_short(pcb_any_obj_t *term, const char *with_net)
{
	pinpad_t *pp;
	pp = calloc(sizeof(pinpad_t), 1);
	pp->term = term;
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

		PCB_FLAG_SET(PCB_FLAG_WARN, n->term);

		/* run only if net is not ignored */
		if ((!bad_gr) && (proc_short(n->term, n->ignore, NULL, NULL) != 0)) {
			fprintf(stderr, "Can't run mincut :(\n");
			bad_gr = 1;
		}

		if (!bad_gr) {
			/* check if the rest of the shorts affect the same nets - ignore them if so */
			for (i = n->next; i != NULL; i = i->next) {
				pcb_lib_menu_t *spn, *spi;
				spn = n->term->ratconn;
				spi = i->term->ratconn;

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

static void pcb_mincut_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	int *handled;
	pcb_any_obj_t *term;
	pcb_net_t *Snet, *Tnet;

	if (!conf_mincut.plugins.mincut.enable)
		return;

	if (argc < 5)
		return;

	if (argv[4].type != PCB_EVARG_PTR)
		return;
	handled = (int *)argv[4].d.p;
	if (*handled)
		return;

	if (argv[2].type != PCB_EVARG_PTR)
		return;
	term = (pcb_any_obj_t *)argv[2].d.p;

	if (argv[1].type != PCB_EVARG_PTR)
		return;
	Snet = (pcb_net_t *)argv[1].d.p;

	if (argv[3].type != PCB_EVARG_PTR)
		return;
	Tnet = (pcb_net_t *)argv[3].d.p;

	if (proc_short(term, 0, Snet, Tnet) == 0)
		*handled = 1;
}


int pplg_check_ver_mincut(int ver_needed) { return 0; }

void pplg_uninit_mincut(void)
{
	conf_unreg_fields("plugins/mincut/");
	pcb_event_unbind_allcookie(pcb_mincut_cookie);
}

#include "stub_mincut.h"
int pplg_init_mincut(void)
{
	PCB_API_CHK_VER;
	pcb_stub_rat_found_short = rat_found_short;
	pcb_stub_rat_proc_shorts = rat_proc_shorts;

	pcb_event_bind(PCB_EVENT_NET_INDICATE_SHORT, pcb_mincut_ev, NULL, pcb_mincut_cookie);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_mincut, field,isarray,type_name,cpath,cname,desc,flags);
#include "rats_mincut_conf_fields.h"
	return 0;
}
