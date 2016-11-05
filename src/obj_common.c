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

#include <setjmp.h>

#include "conf_core.h"

#include "board.h"
#include "math_helper.h"
#include "data.h"
#include "rtree.h"
#include "search.h"
#include "undo.h"
#include "plug_io.h"
#include "stub_vendor.h"
#include "hid_actions.h"
#include "paths.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_all.h"

/* returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
BoxTypePtr GetObjectBoundingBox(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
	case PCB_TYPE_POLYGON:
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_ELEMENT_NAME:
		return (BoxType *) Ptr2;
	case PCB_TYPE_VIA:
	case PCB_TYPE_ELEMENT:
		return (BoxType *) Ptr1;
	case PCB_TYPE_POLYGON_POINT:
	case PCB_TYPE_LINE_POINT:
		return (BoxType *) Ptr3;
	default:
		Message(PCB_MSG_DEFAULT, "Request for bounding box of unsupported type %d\n", Type);
		return (BoxType *) Ptr2;
	}
}



/* current object ID; incremented after each creation of an object */
long int ID = 1;

pcb_bool pcb_create_be_lenient = pcb_false;

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */
void CreateBeLenient(pcb_bool v)
{
	pcb_create_be_lenient = v;
}

void CreateIDBump(int min_id)
{
	if (ID < min_id)
		ID = min_id;
}

void CreateIDReset(void)
{
	ID = 1;
}

long int CreateIDGet(void)
{
	return ID++;
}
