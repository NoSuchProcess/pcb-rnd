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


typedef struct pcb_obj_s pcb_obj_t;

struct pcb_obj_s {
	pcb_objtype_t type;
	union {
		void         *any;
		pcb_any_obj_t *anyobj;
		pcb_point_t    *point;
		pcb_line_t     *line;
		pcb_text_t     *text;
		pcb_polygon_t  *polygon;
		pcb_arc_t      *arc;
		pcb_rat_t      *rat;
		pcb_pad_t      *pad;
		pcb_pin_t      *pin;
		pcb_pin_t      *via;
		pcb_element_t  *element;
		pcb_net_t      *net;
		pcb_layer_t    *layer;
	} data;

	pcb_parenttype_t parent_type;
	pcb_parent_t parent;
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
