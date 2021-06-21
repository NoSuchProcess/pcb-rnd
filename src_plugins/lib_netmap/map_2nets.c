/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include <stdio.h>

#include "map_2nets.h"
#include "data.h"
#include "find.h"
#include "netlist.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include "../src_3rd/genprique/fibheap.h"
#include "../src_3rd/genprique/fibheap.c"
#include "../src_3rd/libusearch/a_star_api.h"
#include "../src_3rd/libusearch/a_star_impl.h"

#define OID(o) ((o == NULL) ? 0 : o->ID)

static void list_obj(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_any_obj_t *obj)
{
	pcb_2netmap_t *map = ctx;
	pcb_qry_netseg_len_t *seg;
	pcb_any_obj_t **o;
	long n;
	pcb_2netmap_iseg_t *ns;


	if ((layer != NULL) && (pcb_layer_flags_(layer) & PCB_LYT_COPPER) == 0)
		return;

	if (htpp_get(&map->o2n, obj) != NULL) /* object already found */
		return;

	seg = pcb_qry_parent_net_len_mapseg(map->ec, obj);
	if (seg == NULL)
		return;

	ns = calloc(sizeof(pcb_2netmap_iseg_t), 1);
	ns->next = map->isegs;
	map->isegs = ns;
	ns->seg = seg;
	ns->net = NULL;

	if (seg->objs.used > 0) {
		ns->term[0] = (((pcb_any_obj_t *)seg->objs.array[0])->term != NULL);
		ns->term[1] = (((pcb_any_obj_t *)seg->objs.array[seg->objs.used-1])->term != NULL);
	}

	printf("seg=%p %s junc: %ld %ld term: %d %d\n", (void *)seg, (seg->hub ? "HUB" : ""), OID(seg->junction[0]), OID(seg->junction[1]), ns->term[0], ns->term[1]);

	for(n = 0, o = (pcb_any_obj_t **)seg->objs.array; n < seg->objs.used; n++,o++) {
		if (*o == NULL) {
			printf("  NULL\n");
			continue;
		}
		htpp_set(&map->o2n, *o, ns);
		printf("  #%ld\n", (*o)->ID);

		if ((*o)->term != NULL) {
			pcb_net_term_t *t = pcb_net_find_by_obj(&pcb->netlist[PCB_NETLIST_EDITED], *o);
			if ((t != NULL) && (t->parent.net != NULL)) {
				if ((ns->net != NULL) && (ns->net != t->parent.net))
					ns->shorted = 1;
				ns->net = t->parent.net;
			}
		}
	}
}

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)line);
}

static void list_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)arc);
}

static void list_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)poly);
}

static void list_pstk_cb(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps)
{
	list_obj(ctx, pcb, NULL, (pcb_any_obj_t *)ps);
}

static pcb_2netmap_oseg_t *oseg_new(pcb_2netmap_t *map, pcb_net_t *net, int shorted)
{
	pcb_2netmap_oseg_t *os;
	os = calloc(sizeof(pcb_2netmap_oseg_t), 1);
	os->next = map->osegs;
	map->osegs = os;
	os->net = net;
	os->shorted = shorted;
	return os;
}

static pcb_2netmap_obj_t *map_seg_out_obj(pcb_2netmap_t *map, pcb_any_obj_t *obj)
{
	pcb_2netmap_obj_t *res = malloc(sizeof(pcb_2netmap_obj_t));

	/* copy the object but reset some fields as this object is not part of any layer list */
	switch(obj->type) {
		case PCB_OBJ_LINE: memcpy(res, obj, sizeof(res->line)); memset(&res->line.link, 0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_ARC:  memcpy(res, obj, sizeof(res->arc));  memset(&res->arc.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_PSTK: memcpy(res, obj, sizeof(res->pstk)); memset(&res->pstk.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_POLY: memcpy(res, obj, sizeof(res->poly)); memset(&res->poly.link,  0, sizeof(gdl_elem_t)); break;
		case PCB_OBJ_TEXT: memcpy(res, obj, sizeof(res->text)); memset(&res->text.link,  0, sizeof(gdl_elem_t)); break;
		default:;
	}

	return res;
}

/* copy all objects of i->seg to o, starting from start_side; returns last
   object copied */
static pcb_any_obj_t *map_seg_out_copy(pcb_2netmap_t *map, pcb_2netmap_oseg_t *o, pcb_2netmap_iseg_t *i, int start_side)
{
	long n;
	if (start_side == 0) {
		for(n = 0; n < i->seg->objs.used; n++)
			vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
		return i->seg->objs.array[i->seg->objs.used-1];
	}

	for(n = i->seg->objs.used-1; n >= 0; n--)
		vtp0_append(&o->objs, map_seg_out_obj(map, i->seg->objs.array[n]));
	return i->seg->objs.array[0];
}

/* replace a hub object with a dummy object that acts as a bridge between 'from'
   object and the starting object of to_iseg */
static void map_seg_add_bridge(pcb_2netmap_t *map, pcb_2netmap_oseg_t *oseg, pcb_any_obj_t *from,  pcb_any_obj_t *hub_obj, pcb_2netmap_iseg_t *to_iseg, int to_start_side)
{
	pcb_any_obj_t *to;
	pcb_2netmap_obj_t *tmp = calloc(sizeof(pcb_2netmap_obj_t), 1);

	assert(to_iseg->seg->objs.used > 0);
	if (to_start_side == 0)
		to = to_iseg->seg->objs.array[0];
	else
		to = to_iseg->seg->objs.array[to_iseg->seg->objs.used - 1];

	tmp->line.type = hub_obj->type;
	vtp0_append(&oseg->objs, tmp);
	TODO("calculate coords of the bridge object")
}

static void map_seg_out(pcb_2netmap_t *map, pcb_2netmap_iseg_t *iseg)
{
	pcb_2netmap_iseg_t *prev, *es;
	pcb_2netmap_oseg_t *oseg = oseg_new(map, iseg->net, iseg->shorted);
	int n, start_side = iseg->term[0] ? 0 : 1; /* assumes we are starting from a terminal */
	pcb_any_obj_t *end_obj, *hub_last_obj = NULL, *last_obj = NULL, *hub_obj;

	for(;;) {
		/* copy current iseg */
printf("* iseg: %p %d\n", iseg, start_side);
		if (iseg->seg->hub) {
			hub_last_obj = last_obj;
			hub_obj = iseg->seg->objs.array[0];
		}
		else {
			if (hub_last_obj != NULL) {
				map_seg_add_bridge(map, oseg, hub_last_obj, hub_obj, iseg, start_side);
				hub_last_obj = NULL;
			}
			last_obj = map_seg_out_copy(map, oseg, iseg, start_side);
		}

		prev = iseg;
		iseg = iseg->path_next;
		if (iseg == NULL)
			break;

		end_obj = NULL;

printf("* prev term: %d %d\n", prev->term[0], prev->term[1]);

		/* figure the common object and determine start_side for the new iseg */
		end_obj = NULL;
		for(n = 0; n < 2; n++) {
			printf("* junc: %d %p\n", n, prev->seg->junction[n]);
			if (prev->seg->junction[n] != NULL) {
				es = htpp_get(&map->o2n, prev->seg->junction[n]);
				if (es == iseg) {
					end_obj = prev->seg->junction[n];
					break;
				}
			}
		}

		assert(end_obj != NULL);
		if (iseg->seg->objs.array[0] == end_obj)
			start_side = 0;
		else 
			start_side = 1;
	}
}

/*** Search a path from a starting segment to any terminal using the A* algorithm ***/

typedef struct ast_ctx_s {
	pcb_2netmap_t *map;
	pcb_2netmap_iseg_t *start;
} ast_ctx_t;

static long heur(usrch_a_star_t *ctx, void *node_)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t *node = node_;
	long score = node->seg->objs.used;

	/* avoid shorts: overlaps should happen on legit segs */
	if (node->net != actx->start->net) score += 1000;

	/* try to use new segment: least overlap */
	if (node->used) score += 300;

	/* avoid bridge segments: go for paths with less junctions */
	if (!node->term[0] && !node->term[1]) score += 200;

	return score;
}


static long cost(usrch_a_star_t *ctx, void *from, void *to_)
{
	return heur(ctx, to_);
}

static void *neighbor_pre(usrch_a_star_t *ctx, void *curr)
{
	static int count;
	count = 0;
	return &count;
}

static void *neighbor(usrch_a_star_t *ctx, void *curr_, void *nctx)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t  *curr = curr_;
	pcb_any_obj_t *obj = NULL;
	void *res;
	int *count = nctx;

	for(;;) {
		if (*count == 2)
			return NULL;

		obj = curr->seg->junction[*count];
		(*count)++;
		if (obj != NULL) {
			res = htpp_get(&actx->map->o2n, obj);
			if (res != NULL)
				break;
		}
	}

	return res;
}

static int is_target(usrch_a_star_t *ctx, void *curr_)
{
	ast_ctx_t *actx = ctx->user_data;
	pcb_2netmap_iseg_t *curr = curr_;
	return ((curr != actx->start) && (curr->term[0] || curr->term[1]));
}

static void set_mark(usrch_a_star_t *ctx, void *node, usrch_a_star_node_t *mark)
{
	pcb_2netmap_iseg_t *seg = node;
	seg->mark = mark;
}

static usrch_a_star_node_t *get_mark(usrch_a_star_t *ctx, void *node)
{
	pcb_2netmap_iseg_t *seg = node;
	return seg->mark;
}

static void map_seg_search(pcb_2netmap_t *map, pcb_2netmap_iseg_t *iseg)
{
	usrch_a_star_node_t *it;
	usrch_a_star_t a = {0};
	ast_ctx_t actx;
	pcb_2netmap_iseg_t *n, *prev, *first, *last;

	actx.map = map;
	actx.start = iseg;

	a.heuristic = heur;
	a.cost = cost;
	a.is_target = is_target;
	a.neighbor_pre = neighbor_pre;
	a.neighbor = neighbor;
	a.set_mark = set_mark;
	a.get_mark = get_mark;
	a.user_data = &actx;

	usrch_a_star_search(&a, iseg, NULL);

printf("-------------------\n");

	/* pick up the the results and build a path using ->path_next and render the output net */
	last = NULL;
	for(n = usrch_a_star_path_first(&a, &it); n != NULL; n = usrch_a_star_path_next(&a, &it)) {
printf(" + %p\n", n);
		n->path_next = last;
		last = n;
		n->used = 1;
	}
	map_seg_out(map, last);

	usrch_a_star_uninit(&a);
}

static void map_segs(pcb_2netmap_t *map)
{
	pcb_2netmap_iseg_t *i;

	/* search nets that start or end in a terminal */
	for(i = map->isegs; i != NULL; i = i->next) {
		if (i->used) continue;
		if (i->term[0] && i->term[1]) /* spinlest case: terminal-to-terminal */
			map_seg_out(map, i);
		else if (i->term[0] || i->term[1])
			map_seg_search(map, i);
	}
}

/*** API functions ***/

int pcb_map_2nets_init(pcb_2netmap_t *map, pcb_board_t *pcb, pcb_2netmap_control_t how)
{
	pcb_qry_exec_t ec;

	pcb_qry_init(&ec, pcb, NULL, 0);

	map->ec = &ec;

	htpp_init(&map->o2n, ptrhash, ptrkeyeq);

	/* map segments using query's netlen mapper */
	pcb_loop_all(PCB, map,
		NULL, /* layer */
		list_line_cb,
		list_arc_cb,
		NULL, /* text */
		list_poly_cb,
		NULL, /* gfx */
		NULL, /* subc */
		list_pstk_cb
	);

	/* the result is really a graph because of junctions; search random paths
	   from terminal to terminal (junctions resolved into overlaps) */
	map_segs(map);

	pcb_qry_uninit(&ec);

	return -1;
}

int pcb_map_2nets_uninit(pcb_2netmap_t *map)
{

	htpp_uninit(&map->o2n);
	return -1;
}

