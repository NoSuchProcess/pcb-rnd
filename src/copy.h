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

#ifndef	PCB_COPY_H
#define	PCB_COPY_H

#include "config.h"

#define	PCB_COPY_TYPES              \
	(PCB_OBJ_PSTK | PCB_OBJ_LINE | PCB_OBJ_TEXT | \
	PCB_OBJ_SUBC | PCB_OBJ_POLY | PCB_OBJ_ARC)

/* Undoably copies (duplicates) an object; the new objects is moved by DX,DY
   (operation wrapper) */
void *pcb_copy_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY);

#endif
