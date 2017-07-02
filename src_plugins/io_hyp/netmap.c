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

#define APPEND(a,b,c,d,e)

static pcb_cardinal_t found(void *ctx, pcb_any_obj_t *obj)
{
	pcb_netmap_t *map = ctx;
	pcb_trace(" %x %p\n", obj->type, obj);
	return 1;
}

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
}

static void list_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	APPEND(ctx, PCB_OBJ_ARC, arc, PCB_PARENT_LAYER, layer);
}

static void list_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	APPEND(ctx, PCB_OBJ_POLYGON, poly, PCB_PARENT_LAYER, layer);
}

static void list_epin_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pin_t *pin)
{
	pcb_netmap_t *map = ctx;

	if (htpp_get(&map->o2n, pin) != NULL)
		return;

	pcb_trace("pin!\n");
	pcb_lookup_conn_by_obj(map, pin, 0, found);
}

static void list_epad_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pad_t *pad)
{
	APPEND(ctx, PCB_OBJ_PAD, pad, PCB_PARENT_ELEMENT, element);
}

static void list_via_cb(void *ctx, pcb_board_t *pcb, pcb_pin_t *via)
{
	APPEND(ctx, PCB_OBJ_VIA, via, PCB_PARENT_DATA, pcb->Data);
}

int pcb_netmap_init(pcb_netmap_t *map, pcb_board_t *pcb)
{
	htpp_init(&map->o2n, ptrhash, ptrkeyeq);
	htpp_init(&map->n2o, ptrhash, ptrkeyeq);
	map->anon_cnt = 0;
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

#if 0
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
#endif

}

int pcb_netmap_uninit(pcb_netmap_t *map)
{
	htpp_uninit(&map->o2n);
	htpp_uninit(&map->n2o);
}

