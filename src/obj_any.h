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

/* object model - type+union that can represent any object */

#ifndef PCB_OBJ_H
#define PCB_OBJ_H

#include "global_objs.h"
#include "global_typedefs.h"

/* Can be used as a bitfield */
typedef enum pcb_objtype_e {
	PCB_OBJ_VOID      = 0x0000,

	PCB_OBJ_POINT     = 0x0001,
	PCB_OBJ_LINE      = 0x0002,
	PCB_OBJ_TEXT      = 0x0004,
	PCB_OBJ_POLYGON   = 0x0008,
	PCB_OBJ_ARC       = 0x0010,
	PCB_OBJ_RAT       = 0x0020,
	PCB_OBJ_PAD       = 0x0040,
	PCB_OBJ_PIN       = 0x0080,
	PCB_OBJ_VIA       = 0x0100,
	PCB_OBJ_ELEMENT   = 0x0200,

	PCB_OBJ_NET       = 0x0400,
	PCB_OBJ_LAYER     = 0x0800
} pcb_objtype_t;

typedef struct pcb_obj_s pcb_obj_t;

struct pcb_obj_s {
	pcb_objtype_t type;
	union {
		PointType    *point;
		LineType     *line;
		TextType     *text;
		PolygonType  *polygon;
		ArcType      *arc;
		RatType      *rat;
		PadType      *pad;
		PinType      *pin;
		PinType      *via;
		ElementType  *element;
		NetType      *net;
		LayerType    *layer;
	} data;
	gdl_elem_t link;
};

/* List of Arcs */
#define TDL(x)      pcb_objlist_ ## x
#define TDL_LIST_T  pcb_objlist_t
#define TDL_ITEM_T  pcb_obj_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_objlist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

#endif
