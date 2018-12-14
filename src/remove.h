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

/* Remove objects from the board */

#ifndef	PCB_REMOVE_H
#define	PCB_REMOVE_H

#include "config.h"

#define PCB_REMOVE_TYPES \
	(PCB_OBJ_PSTK | PCB_OBJ_LINE_POINT | PCB_OBJ_LINE | PCB_OBJ_TEXT | PCB_OBJ_SUBC | \
	PCB_OBJ_POLY_POINT | PCB_OBJ_POLY | PCB_OBJ_RAT | PCB_OBJ_ARC | PCB_OBJ_ARC_POINT)

pcb_bool pcb_remove_selected(pcb_bool locked_too);

/* Undoable delete (operation wrapper) */
void *pcb_remove_object(int Type, void *Ptr1, void *Ptr2, void *Ptr3);

/* Non-undoable delete (operation wrapper) */
void *pcb_destroy_object(pcb_data_t *Target, pcb_objtype_t Type, void *Ptr1, void *Ptr2, void *Ptr3);

#endif
