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

	if (seg->hub)
		printf("seg=%p HUB\n", (void *)seg);
	else
		printf("seg=%p junc: %ld %ld\n", (void *)seg, OID(seg->junction[0]), OID(seg->junction[1]));

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

static pcb_2netmap_oseg_t *oseg_new(pcb_2netmap_t *map, pcb_net_t *net)
{
	pcb_2netmap_oseg_t *os;
	os = calloc(sizeof(pcb_2netmap_iseg_t), 1);
	os->next = map->isegs;
	map->isegs = os;
	os->net = net;
	return os;
}

static void map_segs(pcb_2netmap_t *map)
{
	
}

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

