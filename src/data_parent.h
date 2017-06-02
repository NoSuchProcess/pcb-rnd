/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2017 Tibor 'Igor2' Palinkas
 * 
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_DATA_PARENT_H
#define PCB_DATA_PARENT_H

#include "global_typedefs.h"

/* which elem of the parent union is active */
typedef enum pcb_parenttype_e {
	PCB_PARENT_INVALID = 0,  /* invalid or unknown */
	PCB_PARENT_LAYER,        /* object is on a layer */
	PCB_PARENT_ELEMENT,      /* object is part of an element */
	PCB_PARENT_DATA          /* global objects like via */
} pcb_parenttype_t;

/* class is e.g. PCB_OBJ_CLASS_OBJ */
#define PCB_OBJ_IS_CLASS(type, class)  (((type) & PCB_OBJ_CLASS_MASK) == (class))

union pcb_parent_s {
	void           *any;
	pcb_layer_t    *layer;
	pcb_data_t     *data;
	pcb_element_t  *element;
};

#define PCB_PARENT_TYPENAME_layer    PCB_PARENT_LAYER
#define PCB_PARENT_TYPENAME_data     PCB_PARENT_DATA
#define PCB_PARENT_TYPENAME_element  PCB_PARENT_ELEMENT

#define PCB_SET_PARENT(obj, ptype, parent_ptr) \
	do { \
		obj->parent_type = PCB_PARENT_TYPENAME_ ## ptype; \
		obj->parent.ptype = parent_ptr; \
	} while(0)

#define PCB_CLEAR_PARENT(obj) \
	do { \
		obj->parent_type = PCB_PARENT_INVALID; \
		obj->parent.any = NULL; \
	} while(0)

#endif
