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

#ifndef	PCB_MOVE_H
#define	PCB_MOVE_H

#include "config.h"

/*** Transformation macros and constants ***/
#define	PCB_MOVE(xs,ys,deltax,deltay)							\
	{														\
		((xs) += (deltax));									\
		((ys) += (deltay));									\
	}

#define	PCB_MOVE_TYPES	\
	(PCB_TYPE_PSTK | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_SUBC | \
	PCB_TYPE_POLY | PCB_TYPE_POLY_POINT | PCB_TYPE_LINE_POINT | PCB_TYPE_ARC | PCB_TYPE_ARC_POINT)
#define	PCB_MOVETOLAYER_TYPES	\
	(PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLY | PCB_TYPE_RATLINE | PCB_TYPE_ARC)


/* undoably moves an object by relative DX and DY. Doesn't bump
   the undo serial. Operation wrapper. The rubberband version also moves
   or modifies connected objects. */
void *pcb_move_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY);
void *pcb_move_obj_and_rubberband(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY);

/* undoably moves objects to a different layer. Doesn't bump
   the undo serial. Operation wrapper. */
void *pcb_move_obj_to_layer(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_layer_t *Target, pcb_bool enmasse);
pcb_bool pcb_move_selected_objs_to_layer(pcb_layer_t *Target);

#endif
