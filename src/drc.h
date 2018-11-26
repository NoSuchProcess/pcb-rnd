/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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

#include <genlist/gendlist.h>
#include "unit.h"
#include "vtid.h"

typedef struct drc_violation_s pcb_drc_violation_t;
struct drc_violation_s {
	char *title;
	char *explanation;
	pcb_coord_t x, y;
	pcb_angle_t angle;
	int have_measured;
	pcb_coord_t measured_value;
	pcb_coord_t required_value;

	vtid_t objs[2];               /* two groups of offending objects */

	gdl_elem_t link;              /* always part of a list */
};

/* List of drc violations */
#define TDL(x)      pcb_drc_list_ ## x
#define TDL_LIST_T  pcb_drc_list_t
#define TDL_ITEM_T  pcb_drc_violation_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_drc_list_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

void pcb_drc_free(pcb_drc_violation_t *item);

void pcb_drc_list_free_fields(pcb_drc_list_t *lst);
void pcb_drc_list_free(pcb_drc_list_t *lst);

int pcb_drc_all(pcb_drc_list_t *lst);

#endif
