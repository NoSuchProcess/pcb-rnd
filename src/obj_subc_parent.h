/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#ifndef PCB_OBJ_SUBC_PARENT_H
#define PCB_OBJ_SUBC_PARENT_H

#include "data.h"
#include "data_parent.h"
#include "layer.h"

/* Returns whether global (on-data) object is part of a subc */
static inline PCB_FUNC_UNUSED int pcb_is_gobj_in_subc(pcb_parenttype_t pt, pcb_parent_t *p)
{
	if (pt != PCB_PARENT_DATA)
		return 0;

	if (p->data == NULL)
		return 0;
	
	return (p->data->parent_type == PCB_PARENT_SUBC);
}

/* Returns whether layer object is part of a subc */
static inline PCB_FUNC_UNUSED int pcb_is_lobj_in_subc(pcb_parenttype_t pt, pcb_parent_t *p)
{
	if (pt != PCB_PARENT_LAYER)
		return 0;

	if (p->layer == NULL)
		return 0;
	
	if (p->layer->parent == NULL)
		return 0;

	return (p->layer->parent->parent_type == PCB_PARENT_SUBC);
}

#endif
