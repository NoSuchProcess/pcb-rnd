/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* functions used to create vias, pins ... */

#include "config.h"

#include <stddef.h>
#include "conf_core.h"
#include "global_typedefs.h"
#include "const.h"
#include "error.h"
#include "obj_common.h"
#include "obj_arc_ui.h"

const char *pcb_obj_type_name(pcb_objtype_t type)
{
	switch(type) {
		case PCB_OBJ_VOID:    return "void";
		case PCB_OBJ_POINT:   return "point";
		case PCB_OBJ_LINE:    return "line";
		case PCB_OBJ_TEXT:    return "text";
		case PCB_OBJ_POLYGON: return "polygon";
		case PCB_OBJ_ARC:     return "arc";
		case PCB_OBJ_RAT:     return "ratline";
		case PCB_OBJ_PAD:     return "pad";
		case PCB_OBJ_PIN:     return "pin";
		case PCB_OBJ_VIA:     return "via";
		case PCB_OBJ_ELEMENT: return "element";
		case PCB_OBJ_SUBC:    return "subcircuit";
		case PCB_OBJ_NET:     return "net";
		case PCB_OBJ_LAYER:   return "layer";
		case PCB_OBJ_ELINE:
		case PCB_OBJ_EARC:
		case PCB_OBJ_ETEXT:
		case PCB_OBJ_CLASS_MASK:
		case PCB_OBJ_ANY:
			break;
	}
	return "<unknown/composite>";
}

/* returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
int GetObjectBoundingBox(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_box_t *res)
{
	switch (Type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
	case PCB_TYPE_POLYGON:
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_ELEMENT_NAME:
		*res = *(pcb_box_t *)Ptr2;
		return 0;
	case PCB_TYPE_VIA:
	case PCB_TYPE_ELEMENT:
	case PCB_TYPE_SUBC:
		*res = *(pcb_box_t *)Ptr1;
		return 0;
	case PCB_TYPE_POLYGON_POINT:
	case PCB_TYPE_LINE_POINT:
		*res = *(pcb_box_t *)Ptr3;
		return 0;
	case PCB_TYPE_ARC_POINT:
		return pcb_obj_ui_arc_point_bbox(Type, Ptr1, Ptr2, Ptr3, res);
	default:
		pcb_message(PCB_MSG_ERROR, "Request for bounding box of unsupported type %d\n", Type);
		*res = *(pcb_box_t *)Ptr2;
		return -1;
	}
}



/* current object ID; incremented after each creation of an object */
long int ID = 1;

pcb_bool pcb_create_being_lenient = pcb_false;

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */
void pcb_create_be_lenient(pcb_bool v)
{
	pcb_create_being_lenient = v;
}

void pcb_create_ID_bump(int min_id)
{
	if (ID < min_id)
		ID = min_id;
}

void pcb_create_ID_reset(void)
{
	ID = 1;
}

long int pcb_create_ID_get(void)
{
	return ID++;
}

void pcb_obj_add_attribs(void *obj, const pcb_attribute_list_t *src)
{
	pcb_any_obj_t *o = obj;
	if (src == NULL)
		return;
	pcb_attribute_copy_all(&o->Attributes, src);
}

void pcb_obj_center(pcb_any_obj_t *obj, pcb_coord_t *x, pcb_coord_t *y)
{
	switch (obj->type) {
		case PCB_OBJ_PIN:
			*x = ((pcb_pin_t *)(obj))->X;
			*y = ((pcb_pin_t *)(obj))->Y;
			break;
		default:
			*x = (obj->BoundingBox.X1 + obj->BoundingBox.X2) / 2;
			*y = (obj->BoundingBox.Y1 + obj->BoundingBox.Y2) / 2;
	}
}

void pcb_obj_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)(((char *)list) - offsetof(pcb_any_obj_t, Attributes));
	if (strcmp(name, "term") == 0)
		obj->term = value;
}
