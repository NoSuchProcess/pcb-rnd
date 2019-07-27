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
#include "netlist.h"
#include "compat_misc.h"
#include "obj_common.h"
#include "obj_subc_parent.h"

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

typedef struct {
	short_conn_t *short_conns;
	int num_short_conns;
	int short_conns_maxid;
} shctx_t;

static int proc_short_cb(pcb_find_t *fctx, pcb_any_obj_t *curr, pcb_any_obj_t *from, pcb_found_conn_type_t type)
{
	short_conn_t *s;
	shctx_t *shctx = fctx->user_data;

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
	s->next = shctx->short_conns;
	shctx->short_conns = s;
	if (curr->ID > shctx->short_conns_maxid)
		shctx->short_conns_maxid = curr->ID;
	shctx->num_short_conns++;

	debprintf(" found %d %d/%p type=%d from %d\n", curr->type, curr->ID, (void *)curr, s->type, from == NULL ? -1 : from->ID);
	return 0;
}

/* returns 0 on succes */
static int proc_short(pcb_any_obj_t *term, pcb_net_t *Snet, pcb_net_t *Tnet)
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
	shctx_t shctx;

	pcb_obj_center(term, &x, &y);
	debprintf("short on terminal\n");

	/* perform a search calling back proc_short_cb() with the connections */
	memset(&shctx, 0, sizeof(shctx));
	memset(&fctx, 0, sizeof(fctx));
	fctx.found_cb = proc_short_cb;
	fctx.user_data = &shctx;
	pcb_find_from_obj(&fctx, PCB->Data, term);
	pcb_find_free(&fctx);


	debprintf("- alloced for %d\n", (shctx.short_conns_maxid + 1));
	lut_by_oid = calloc(sizeof(short_conn_t *), (shctx.short_conns_maxid + 1));
	lut_by_gid = calloc(sizeof(short_conn_t *), (shctx.num_short_conns + 3));

	g = gr_alloc(shctx.num_short_conns + 2);

	g->node2name = calloc(sizeof(char *), (shctx.num_short_conns + 2));

	/* conn 0 is S and conn 1 is T and set up lookup arrays */
	for (n = shctx.short_conns, gids = 2; n != NULL; n = n->next, gids++) {
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
	for (n = shctx.short_conns; n != NULL; n = n->next) {
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
		S = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], Snet->name, 0);
		assert(S != NULL);
	}
	if (Tnet != NULL) {
		T = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], Tnet->name, 0);
		assert(T != NULL);
	}

	for (n = shctx.short_conns; n != NULL; n = n->next) {
		pcb_any_obj_t *o = (pcb_any_obj_t *)n->to;
		if (o->term != NULL) {
			pcb_subc_t *sc = pcb_obj_parent_subc(o);
			if ((sc != NULL) && !PCB_FLAG_TEST(PCB_FLAG_NONETLIST, sc)) {
				if ((sc->refdes != NULL) && (o->term != NULL)) {
					pcb_net_term_t *t = pcb_net_find_by_refdes_term(&PCB->netlist[PCB_NETLIST_EDITED], sc->refdes, o->term);
					if (t != NULL) {
						if (t->parent.net == Snet)
							gr_add_(g, n->gid, 0, 100000);
						else if (t->parent.net == Tnet)
							gr_add_(g, n->gid, 1, 100000);
					}
				}
				else
					pcb_message(PCB_MSG_WARNING, "Ignoring subcircuit %ld connecting to a net because it has no refdes;\nplease use the nonetlist flag on the subcircuit to suppress this warning\n", sc->ID);
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

	for (n = shctx.short_conns; n != NULL; n = next) {
		next = n->next;
		free(n);
	}

	for(i = 0; i < g->n; i++)
		free(g->node2name[i]);
	free(g->node2name);
	gr_free(g);

	return bad_gr;
}

static void pcb_mincut_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
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

	if (proc_short(term, Snet, Tnet) == 0)
		*handled = 1;
}


int pplg_check_ver_mincut(int ver_needed) { return 0; }

void pplg_uninit_mincut(void)
{
	pcb_conf_unreg_fields("plugins/mincut/");
	pcb_event_unbind_allcookie(pcb_mincut_cookie);
}

int pplg_init_mincut(void)
{
	PCB_API_CHK_VER;

	pcb_event_bind(PCB_EVENT_NET_INDICATE_SHORT, pcb_mincut_ev, NULL, pcb_mincut_cookie);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_mincut, field,isarray,type_name,cpath,cname,desc,flags);
#include "rats_mincut_conf_fields.h"
	return 0;
}
