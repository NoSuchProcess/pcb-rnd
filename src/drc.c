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

#include "drc.h"
#include "error.h"
#include "idpath.h"

/* Build a list of the of offending items by ID */
void pcb_drc_append_obj(pcb_view_t *view, int grp, pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;

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
