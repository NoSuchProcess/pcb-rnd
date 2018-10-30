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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Inserting points into objects */

#ifndef	PCB_INSERT_H
#define	PCB_INSERT_H

#include "config.h"

#define	PCB_INSERT_TYPES	(PCB_OBJ_POLY | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_RAT)

void *pcb_insert_point_in_object(int Type, void *Ptr1, void *Ptr2, pcb_cardinal_t * Ptr3, pcb_coord_t DX, pcb_coord_t DY, pcb_bool Force, pcb_bool insert_last);

/* adjusts the insert point to make 45 degree lines as necessary */
pcb_point_t *pcb_adjust_insert_point(void);

#endif
