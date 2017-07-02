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

#define APPEND(a,b,c,d,e)

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	APPEND(ctx, PCB_OBJ_LINE, line, PCB_PARENT_LAYER, layer);
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
	APPEND(ctx, PCB_OBJ_PIN, pin, PCB_PARENT_ELEMENT, element);
}

static void list_epad_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pad_t *pad)
{
	APPEND(ctx, PCB_OBJ_PAD, pad, PCB_PARENT_ELEMENT, element);
}

static void list_via_cb(void *ctx, pcb_board_t *pcb, pcb_pin_t *via)
{
	APPEND(ctx, PCB_OBJ_VIA, via, PCB_PARENT_DATA, pcb->Data);
}

int pcb_netmap(pcb_board_t *pcb)
{
	htpp_t ht;
	htpp_init(&ht, ptrhash, ptrkeyeq);

	pcb_loop_all(&ht,
		NULL, /* layer */
		list_line_cb,
		list_arc_cb,
		NULL, /* text */
		list_poly_cb,
		NULL, /* element */
		NULL, /* eline */
		NULL, /* earc */
		NULL, /* etext */
		list_epin_cb,
		list_epad_cb,
		list_via_cb
	);

	htpp_uninit(&ht);
}