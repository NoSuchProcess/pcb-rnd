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

#ifndef PCB_DRC_H
#define PCB_DRC_H

/* Generic DRC infra: helper functions for keeping track of a list of views
   for drc violations (without any actual checking logics) */

#include "view.h"

/* Append obj to one of the object groups in view (resolving to idpath) */
void pcb_drc_append_obj(pcb_view_t *view, int grp, pcb_any_obj_t *obj);

/* Calculate and set v->bbox from v->objs[] bboxes */
void pcb_drc_set_bbox_by_objs(pcb_data_t *data, pcb_view_t *v);

#endif

