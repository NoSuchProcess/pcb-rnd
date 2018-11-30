/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include <assert.h>

#include "drc.h"
#include "error.h"
#include "idpath.h"

/* Build a list of the of offending items by ID */
void pcb_drc_append_obj(pcb_view_t *view, int grp, pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;

	assert((grp == 0) || (grp == 1));

	switch(obj->type) {
		case PCB_OBJ_LINE:
		case PCB_OBJ_ARC:
		case PCB_OBJ_POLY:
		case PCB_OBJ_PSTK:
		case PCB_OBJ_RAT:
			idp = pcb_obj2idpath(obj);
			if (idp == NULL)
				pcb_message(PCB_MSG_ERROR, "Internal error in pcb_drc_append_obj: can not resolve object id path\n");
			else
				pcb_idpath_list_append(&view->objs[grp], idp);
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Internal error in pcb_drc_append_obj: unknown object type %i\n", obj->type);
	}
}

void pcb_drc_set_bbox_by_objs(pcb_data_t *data, pcb_view_t *v)
{
	int g;
	pcb_box_t b;
	pcb_any_obj_t *obj;
	pcb_idpath_t *idp;

	/* special case: no object - leave coords unloaded/invalid */
	if ((pcb_idpath_list_length(&v->objs[0]) < 1) && (pcb_idpath_list_length(&v->objs[1]) < 1))
		return;

	/* special case: single objet in group A, use the center */
	if (pcb_idpath_list_length(&v->objs[0]) == 1) {
		idp = pcb_idpath_list_first(&v->objs[0]);
		obj = pcb_idpath2obj(data, idp);
		if (obj != NULL) {
			v->have_bbox = 1;
			pcb_obj_center(obj, &v->x, &v->y);
			memcpy(&v->bbox, &obj->BoundingBox, sizeof(obj->BoundingBox));
			pcb_box_enlarge(&v->bbox, 0.25, 0.25);
			return;
		}
	}

	b.X1 = b.Y1 = PCB_MAX_COORD;
	b.X2 = b.Y2 = -PCB_MAX_COORD;
	for(g = 0; g < 2; g++) {
		for(idp = pcb_idpath_list_first(&v->objs[g]); idp != NULL; idp = pcb_idpath_list_next(idp)) {
			obj = pcb_idpath2obj(data, idp);
			if (obj != NULL) {
				v->have_bbox = 1;
				pcb_box_bump_box(&b, &obj->BoundingBox);
			}
		}
	}

	if (v->have_bbox) {
		v->x = (b.X1 + b.X2)/2;
		v->y = (b.Y1 + b.Y2)/2;
		memcpy(&v->bbox, &b, sizeof(b));
		pcb_box_enlarge(&v->bbox, 0.25, 0.25);
	}
}

void pcb_drc_set_data(pcb_view_t *violation, const pcb_coord_t *measured_value, pcb_coord_t required_value)
{
	violation->data_type = PCB_VIEW_DRC;
	violation->data.drc.required_value = required_value;
	if (measured_value != NULL) {
		violation->data.drc.have_measured = 1;
		violation->data.drc.measured_value = *measured_value;
	}
	else
		violation->data.drc.have_measured = 0;
}

