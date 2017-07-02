/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <genht/htpp.h>

#include "netmap.h"
#include "data.h"
#include "find.h"
#include "netlist.h"
#include "pcb-printf.h"

static pcb_lib_menu_t *alloc_net(pcb_netmap_t *map)
{
	dyn_net_t *dn = calloc(sizeof(dyn_net_t), 1);
	dn->next = map->dyn_nets;
	map->dyn_nets = dn;
	dn->net.Name = pcb_strdup_printf("  netmap_anon_%d", map->anon_cnt++);
	return &dn->net;
}

static pcb_cardinal_t found(void *ctx, pcb_any_obj_t *obj)
{
	pcb_netmap_t *map = ctx;
	dyn_obj_t *d, *dl;

	if (map->curr_net == NULL)
		map->curr_net = alloc_net(map);

	htpp_set(&map->o2n, obj, map->curr_net);

	dl = htpp_get(&map->n2o, map->curr_net);
	d = malloc(sizeof(dyn_obj_t));
	d->obj = obj;
	d->next = dl;
	htpp_set(&map->n2o, map->curr_net, d);

	return 1;
}

static void list_obj(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_any_obj_t *obj)
{
	pcb_netmap_t *map = ctx;
	map->curr_net = NULL;

	if (htpp_get(&map->o2n, obj) != NULL)
		return;

	if ((layer != NULL) && (pcb_layer_flags_(map->pcb, layer) & PCB_LYT_COPPER) == 0)
		return;

	pcb_lookup_conn_by_obj(map, obj, 0, found);
}

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)line);
}

static void list_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)arc);
}

static void list_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	list_obj(ctx, pcb, layer, (pcb_any_obj_t *)poly);
}

static void list_via_cb(void *ctx, pcb_board_t *pcb, pcb_pin_t *via)
{
	list_obj(ctx, pcb, NULL, (pcb_any_obj_t *)via);
}

static void list_epin_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pin_t *pin)
{
	pcb_netmap_t *map = ctx;

	if (htpp_get(&map->o2n, pin) != NULL)
		return;

	map->curr_net = pcb_netlist_find_net4pin(map->pcb, pin);
	if (map->curr_net == NULL)
		map->curr_net = alloc_net(map);

	pcb_lookup_conn_by_obj(map, (pcb_any_obj_t *)pin, 0, found);
}

static void list_epad_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pad_t *pad)
{
	pcb_netmap_t *map = ctx;

	if (htpp_get(&map->o2n, pad) != NULL)
		return;

	map->curr_net = pcb_netlist_find_net4pad(map->pcb, pad);
	if (map->curr_net == NULL)
		map->curr_net = alloc_net(map);

	pcb_lookup_conn_by_obj(map, (pcb_any_obj_t *)pad, 0, found);
}


int pcb_netmap_init(pcb_netmap_t *map, pcb_board_t *pcb)
{
	htpp_init(&map->o2n, ptrhash, ptrkeyeq);
	htpp_init(&map->n2o, ptrhash, ptrkeyeq);
	map->anon_cnt = 0;
	map->curr_net = NULL;
	map->dyn_nets = 0;
	map->pcb = pcb;

/* step 1: find known nets (from pins and pads) */
	pcb_loop_all(map,
		NULL, /* layer */
		NULL, /* line */
		NULL, /* arc */
		NULL, /* text */
		NULL, /* poly */
		NULL, /* element */
		NULL, /* eline */
		NULL, /* earc */
		NULL, /* etext */
		list_epin_cb,
		list_epad_cb,
		NULL /* via */
	);

	/* step 2: find unknown nets and uniquely name them */
	pcb_loop_all(map,
		NULL, /* layer */
		list_line_cb,
		list_arc_cb,
		NULL, /* text */
		list_poly_cb,
		NULL, /* element */
		NULL, /* eline */
		NULL, /* earc */
		NULL, /* etext */
		NULL, /* pin */
		NULL, /* pad */
		list_via_cb
	);
}

int pcb_netmap_uninit(pcb_netmap_t *map)
{

	{
		htpp_entry_t *e;
		for(e = htpp_first(&map->n2o); e != NULL; e = htpp_next(&map->n2o, e)) {
			dyn_obj_t *o, *next;
			for(o = e->value; o != NULL; o = next) {
				next = o->next;
				free(o);
			}
		}
	}

	{
		dyn_net_t *dn, *next;
		for(dn = map->dyn_nets; dn != NULL; dn = next) {
			next = dn->next;
			free(dn->net.Name);
			free(dn);
		}
	}

	htpp_uninit(&map->o2n);
	htpp_uninit(&map->n2o);
}

