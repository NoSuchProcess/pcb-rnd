/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for move routines */

#ifndef	PCB_MOVE_H
#define	PCB_MOVE_H

#include "config.h"

/* ---------------------------------------------------------------------------
 * some useful transformation macros and constants
 */
#define	MOVE(xs,ys,deltax,deltay)							\
	{														\
		((xs) += (deltax));									\
		((ys) += (deltay));									\
	}

#define	MOVE_TYPES	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT | PCB_TYPE_ELEMENT_NAME |	\
	PCB_TYPE_POLYGON | PCB_TYPE_POLYGON_POINT | PCB_TYPE_LINE_POINT | PCB_TYPE_ARC)
#define	MOVETOLAYER_TYPES	\
	(PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLYGON | PCB_TYPE_RATLINE | PCB_TYPE_ARC)


/* ---------------------------------------------------------------------------
 * prototypes
 */
void *MoveObject(int, void *, void *, void *, Coord, Coord);
void *MoveObjectToLayer(int, void *, void *, void *, LayerTypePtr, pcb_bool);
void *MoveObjectAndRubberband(int, void *, void *, void *, Coord, Coord);
pcb_bool MoveSelectedObjectsToLayer(LayerTypePtr);

/* index is 0..MAX_LAYER-1.  If old_index is -1, a new layer is
   inserted at that index.  If new_index is -1, the specified layer is
   deleted.  Returns non-zero on error, zero if OK.  */
int MoveLayer(int old_index, int new_index);

#endif
