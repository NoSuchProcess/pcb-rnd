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

#include "conf_core.h"
#include "global_typedefs.h"
#include "const.h"
#include "error.h"
#include "obj_common.h"


/* returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
pcb_box_t *GetObjectBoundingBox(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
	case PCB_TYPE_POLYGON:
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_ELEMENT_NAME:
		return (pcb_box_t *) Ptr2;
	case PCB_TYPE_VIA:
	case PCB_TYPE_ELEMENT:
		return (pcb_box_t *) Ptr1;
	case PCB_TYPE_POLYGON_POINT:
	case PCB_TYPE_LINE_POINT:
		return (pcb_box_t *) Ptr3;
	default:
		pcb_message(PCB_MSG_DEFAULT, "Request for bounding box of unsupported type %d\n", Type);
		return (pcb_box_t *) Ptr2;
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

void pcb_obj_add_attribs(void *obj, pcb_attribute_list_t *src)
{
	pcb_any_obj_t *o = obj;
	pcb_attribute_copy_all(&o->Attributes, src, 0);
}
