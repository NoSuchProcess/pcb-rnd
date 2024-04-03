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
#include "search.h"
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

	if (!map->nonterminals && (obj->term == NULL))
		return;

	if ((layer != NULL) && (pcb_layer_flags_(layer) & PCB_LYT_COPPER) == 0)
		return;

	if ((obj->term == NULL) && (htpp_get(&map->o2n, obj) != NULL)) /* object already found */
		return;

	seg = pcb_qry_parent_net_len_mapseg(map->ec, obj, map->find_rats);
	if (seg == NULL)
		return;

	ns = calloc(sizeof(pcb_2netmap_iseg_t), 1);
	if (!seg->has_invalid_hub) {
		ns->next = map->isegs;
		map->isegs = ns;
	}
	ns->seg = seg;
	ns->net = NULL;

	if (seg->objs.used > 0) {
		ns->term[0] = (((pcb_any_obj_t *)seg->objs.array[0])->term != NULL);
		ns->term[1] = (((pcb_any_obj_t *)seg->objs.array[seg->objs.used-1])->term != NULL);
	}

	printf("seg=%p %s junc: %ld %ld term: %d %d\n", (void *)seg, (seg->hub ? "HUB" : ""), OID(seg->junction[0]), OID(seg->junction[1]), ns->term[0], ns->term[1]);

	/* insert end terminals if they are junctions */
	if ((seg->junction[1] != NULL) && (seg->junction[1]->term != NULL) && (seg->objs.used > 1)) {
		pcb_any_obj_t *o = seg->objs.array[seg->objs.used-1];
		if (o->term == NULL) /* insert only if the first object is not a terminal already - we want terminal-terminal 2nets */
			vtp0_append(&seg->objs, &seg->junction[1]);
	}
	if ((seg->junction[0] != NULL) && (seg->junction[0]->term != NULL)) {
		pcb_any_obj_t *o = seg->objs.array[0];
		if (o->term == NULL) /* insert only if the first object is not a terminal already - we want terminal-terminal 2nets */
			vtp0_insert_len(&seg->objs, 0, (void **)(&seg->junction[0]), 1);
	}

	/* figure the net by looking at terminals along this segment */
	for(n = 0, o = (pcb_any_obj_t **)seg->objs.array; n < seg->objs.used; n++,o++) {
		if (*o == NULL) {
			printf("  NULL\n");
			continue;
		}
		if (!seg->has_invalid_hub)
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

	if (seg->has_invalid_hub) {
		rnd_message(RND_MSG_ERROR, "Network %s can not be included in the net map due to invalid junction\n", ns->net->name);
		pcb_qry_lenseg_free_fields(seg);
		free(ns);
		return;
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

static int list_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter)
{
	pcb_board_t tmp;

	tmp = *pcb;
	tmp.Data = subc->data;
	pcb_loop_all(&tmp, ctx,
		NULL, /* layer */
		list_line_cb,
		list_arc_cb,
		NULL, /* text */
		list_poly_cb,
		NULL, /* gfx */
		list_subc_cb, /* subc */
		list_pstk_cb
	);
	return 0;
}


#include "map_2nets_geo.c"
#include "map_2nets_map.c"


/*** API functions ***/

int pcb_map_2nets_init(pcb_2netmap_t *map, pcb_board_t *pcb)
{
	pcb_qry_exec_t ec;

	pcb_qry_init(&ec, pcb, NULL, 0);

	ec.cfg_prefer_term = 1;

	map->ec = &ec;

	htpp_init(&map->o2n, ptrhash, ptrkeyeq);

	/* map segments using query's netlen mapper; first from terminals then
	   from non-terminals; this way some corner cases can be avoided by
	   picking out objects forming real nets, starting from terminals */
	for(map->nonterminals = 0; map->nonterminals < 2; map->nonterminals++) {
		pcb_loop_all(PCB, map,
			NULL, /* layer */
			list_line_cb,
			list_arc_cb,
			NULL, /* text */
			list_poly_cb,
			NULL, /* gfx */
			list_subc_cb, /* subc */
			list_pstk_cb
		);
	}

	/* the result is really a graph because of junctions; search random paths
	   from terminal to terminal (junctions resolved into overlaps) */
	map_segs(map);

	pcb_qry_uninit(&ec);

	return 0;
}

int pcb_map_2nets_uninit(pcb_2netmap_t *map)
{
	pcb_2netmap_oseg_t *oseg, *next;
	for(oseg = map->osegs; oseg != NULL; oseg = next) {
		next = oseg->next;
		vtp0_uninit(&oseg->objs);
		free(oseg);
	}
	htpp_uninit(&map->o2n);
	return -1;
}

