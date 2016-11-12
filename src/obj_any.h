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

#include "obj_common.h"
#include "global_typedefs.h"

/* Can be used as a bitfield */
typedef enum pcb_objtype_e {
	PCB_OBJ_VOID      = 0x000000,

	PCB_OBJ_POINT     = 0x000001,
	PCB_OBJ_LINE      = 0x000002,
	PCB_OBJ_TEXT      = 0x000004,
	PCB_OBJ_POLYGON   = 0x000008,
	PCB_OBJ_ARC       = 0x000010,
	PCB_OBJ_RAT       = 0x000020,
	PCB_OBJ_PAD       = 0x000040,
	PCB_OBJ_PIN       = 0x000080,
	PCB_OBJ_VIA       = 0x000100,
	PCB_OBJ_ELEMENT   = 0x000200,

	/* more abstract objects */
	PCB_OBJ_NET       = 0x100001,
	PCB_OBJ_LAYER     = 0x100002,

	/* temporary, for backward compatibility */
	PCB_OBJ_ELINE     = 0x200001,
	PCB_OBJ_EARC      = 0x200002,
	PCB_OBJ_ETEXT     = 0x200004,

	/* combinations, groups, masks */
	PCB_OBJ_CLASS_MASK= 0xF00000,
	PCB_OBJ_CLASS_OBJ = 0x000000, /* anything with common object fields (AnyObjectType) */
	PCB_OBJ_ANY       = 0xFFFFFF
} pcb_objtype_t;

/* which elem of the parent union is active */
typedef enum pcb_parenttype_e {
	PCB_PARENT_INVALID = 0,  /* invalid or unknown */
	PCB_PARENT_LAYER,        /* object is on a layer */
	PCB_PARENT_ELEMENT,      /* object is part of an element */
	PCB_PARENT_DATA          /* global objects like via */
} pcb_parenttype_t;


/* class is e.g. PCB_OBJ_CLASS_OBJ */
#define PCB_OBJ_IS_CLASS(type, class)  (((type) & PCB_OBJ_CLASS_MASK) == (class))


typedef struct pcb_obj_s pcb_obj_t;

struct pcb_obj_s {
	pcb_objtype_t type;
	union {
		void         *any;
		AnyObjectType *anyobj;
		pcb_point_t    *point;
		pcb_line_t     *line;
		TextType     *text;
		PolygonType  *polygon;
		pcb_arc_t      *arc;
		pcb_rat_t      *rat;
		PadType      *pad;
		PinType      *pin;
		PinType      *via;
		ElementType  *element;
		pcb_net_t      *net;
		pcb_layer_t    *layer;
	} data;

	pcb_parenttype_t parent_type;
	union {
		void         *any;
		pcb_layer_t    *layer;
		pcb_data_t     *data;
		ElementType  *element;
	} parent;
	gdl_elem_t link;
};


/* List of objects */
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
