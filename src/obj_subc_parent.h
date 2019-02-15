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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_OBJ_SUBC_PARENT_H
#define PCB_OBJ_SUBC_PARENT_H

#include "data.h"
#include "data_parent.h"
#include "layer.h"

/* Returns the subc a global (on-data) object is part of (or NULL if not part of any subc) */
PCB_INLINE pcb_subc_t *pcb_gobj_parent_subc(pcb_parenttype_t pt, pcb_parent_t *p)
{
	if (pt != PCB_PARENT_DATA)
		return NULL;

	if (p->data == NULL)
		return NULL;
	
	if (p->data->parent_type == PCB_PARENT_SUBC)
		return p->data->parent.subc;
	return NULL;
}

/* Returns the subc a layer object is part of (or NULL if not part of any subc) */
PCB_INLINE pcb_subc_t *pcb_lobj_parent_subc(pcb_parenttype_t pt, pcb_parent_t *p)
{
	if (pt != PCB_PARENT_LAYER)
		return NULL;

	if (p->layer == NULL)
		return NULL;
	
	if (p->layer->parent.data == NULL)
		return NULL;

	if (p->layer->parent.data->parent_type == PCB_PARENT_SUBC)
		return p->layer->parent.data->parent.subc;
	return NULL;
}

/* Returns the subc an object is part of (or NULL if not part of any subc) */
PCB_INLINE pcb_subc_t *pcb_obj_parent_subc(pcb_any_obj_t *obj)
{
	switch(obj->type) {
		case PCB_OBJ_PSTK:
		case PCB_OBJ_SUBC:
			return pcb_gobj_parent_subc(obj->parent_type, &obj->parent);

		case PCB_OBJ_LINE:
		case PCB_OBJ_POLY:
		case PCB_OBJ_TEXT:
		case PCB_OBJ_ARC:
			return pcb_lobj_parent_subc(obj->parent_type, &obj->parent);

		default:
			/* anything else: virtual */
			return 0;
	}
	return 0;
}

PCB_INLINE pcb_bool_t pcb_obj_is_under(pcb_any_obj_t *obj, pcb_data_t *data)
{
	for(;;) {
		switch(obj->parent_type) {
			case PCB_PARENT_INVALID: return pcb_false;
			case PCB_PARENT_BOARD:   return pcb_false;
			case PCB_PARENT_UI:      return pcb_false; /* shouldn't happen */
			case PCB_PARENT_LAYER:   if (obj->parent.layer->parent.data == data) return pcb_true; break;
			case PCB_PARENT_SUBC:    if (obj->parent.subc->data == data) return pcb_true; break;
			case PCB_PARENT_DATA:    if (obj->parent.data == data) return pcb_true; break;
			case PCB_PARENT_NET:     if (obj->parent.data == data) return pcb_true; break;
		}
		obj = (pcb_any_obj_t *)pcb_obj_parent_subc(obj);
		if (obj == NULL)
			return pcb_false;
	}
}


#endif
