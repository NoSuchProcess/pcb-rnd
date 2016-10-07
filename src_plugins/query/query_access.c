/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* Query language - access to / extract core data */

#include "global.h"
#include "data.h"
#include "query_access.h"

#define APPEND(_ctx_, _type_, _obj_) \
do { \
	pcb_objlist_t *lst = (pcb_objlist_t *)_ctx_; \
	pcb_obj_t *o = malloc(sizeof(pcb_obj_t)); \
	o->type = _type_; \
	o->data.any = _obj_; \
	pcb_objlist_append(lst, o); \
} while(0)

static int list_layer_cb(void *ctx, PCBType *pcb, LayerType *layer, int enter)
{
	if (enter)
		APPEND(lst, PCB_OBJ_LAYER, layer);
	return 0;
}

static void list_line_cb(void *ctx, PCBType *pcb, LayerType *layer, LineType *line)
{
	APPEND(lst, PCB_OBJ_LINE, line);
}

static void list_arc_cb(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc)
{
	APPEND(lst, PCB_OBJ_ARC, arc);
}

static void list_text_cb(void *ctx, PCBType *pcb, LayerType *layer, TextType *text)
{
	APPEND(lst, PCB_OBJ_TEXT, text);
}

static void list_poly_cb(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly)
{
	APPEND(lst, PCB_OBJ_POLYGON, poly);
}

static int list_element_cb(void *ctx, PCBType *pcb, ElementType *element, int enter)
{
	if (enter)
		APPEND(lst, PCB_OBJ_ELEMENT, element);
	return 0;
}

static void list_eline_cb(void *ctx, PCBType *pcb, ElementType *element, LineType *line)
{
	APPEND(lst, PCB_OBJ_ELINE, line);
}

static void list_earc_cb(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc)
{
	APPEND(lst, PCB_OBJ_EARC, arc);
}

static void list_etext_cb(void *ctx, PCBType *pcb, ElementType *element, TextType *text)
{
	APPEND(lst, PCB_OBJ_ETEXT, text);
}

static void list_epin_cb(void *ctx, PCBType *pcb, ElementType *element, PinType *pin)
{
	APPEND(lst, PCB_OBJ_PIN, pin);
}

static void list_epad_cb(void *ctx, PCBType *pcb, ElementType *element, PadType *pad)
{
	APPEND(lst, PCB_OBJ_PAD, pad);
}

static void list_via_cb(void *ctx, PCBType *pcb, PinType *via)
{
	APPEND(lst, PCB_OBJ_VIA, via);
}

void pcb_qry_list_all(pcb_qry_val_t *lst, pcb_objtype_t mask)
{
	assert(lst->type == PCBQ_VT_LST);
	pcb_loop_all(&lst->data.lst,
		(mask & PCB_OBJ_LAYER) ? list_layer_cb : NULL,
		(mask & PCB_OBJ_LINE) ? list_line_cb : NULL,
		(mask & PCB_OBJ_ARC) ? list_arc_cb : NULL,
		(mask & PCB_OBJ_TEXT) ? list_text_cb : NULL,
		(mask & PCB_OBJ_POLYGON) ? list_poly_cb : NULL,
		(mask & PCB_OBJ_ELEMENT) ? list_element_cb : NULL,
		(mask & PCB_OBJ_ELINE) ? list_eline_cb : NULL,
		(mask & PCB_OBJ_EARC) ? list_earc_cb : NULL,
		(mask & PCB_OBJ_ETEXT) ? list_etext_cb : NULL,
		(mask & PCB_OBJ_PIN) ? list_epin_cb : NULL,
		(mask & PCB_OBJ_PAD) ? list_epad_cb : NULL,
		(mask & PCB_OBJ_VIA) ? list_via_cb : NULL
	);
}

void pcb_qry_list_free(pcb_qry_val_t *lst_)
{
	pcb_objlist_t *lst = &lst_->data.lst;

	assert(lst_->type == PCBQ_VT_LST);

	for(;;) {
		pcb_obj_t *n = pcb_objlist_first(lst);
		if (n == NULL)
			break;
		free(n);
	}
}

int pcb_qry_list_cmp(pcb_qry_val_t *lst1, pcb_qry_val_t *lst2)
{
	abort();
}

