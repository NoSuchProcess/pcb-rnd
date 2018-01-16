/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: rats */

#ifndef PCB_OBJ_RAT_H
#define PCB_OBJ_RAT_H

#include "obj_common.h"

struct pcb_rat_line_s {          /* a rat-line */
	PCB_ANYLINEFIELDS;
	pcb_cardinal_t group1, group2; /* the layer group each point is on */
	gdl_elem_t link;               /* an arc is in a list on a design */
};


pcb_rat_t *pcb_rat_alloc(pcb_data_t *data);
void pcb_rat_free(pcb_rat_t *data);

pcb_rat_t *pcb_rat_new(pcb_data_t *Data, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_cardinal_t group1, pcb_cardinal_t group2, pcb_coord_t Thickness, pcb_flag_t Flags);
pcb_bool pcb_rats_destroy(pcb_bool selected);

#endif
