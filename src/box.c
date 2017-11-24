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

#include <string.h>
#include "config.h"
#include "rotate.h"
#include "box.h"

/* ---------------------------------------------------------------------------
 * sets the bounding box of a point (which is silly)
 */
void pcb_set_point_bounding_box(pcb_point_t *Pnt)
{
	Pnt->X2 = Pnt->X + 1;
	Pnt->Y2 = Pnt->Y + 1;
}

/* ---------------------------------------------------------------------------
 * rotates a box in 90 degree steps
 */
void pcb_box_rotate90(pcb_box_t *Box, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_coord_t x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

	PCB_COORD_ROTATE90(x1, y1, X, Y, Number);
	PCB_COORD_ROTATE90(x2, y2, X, Y, Number);
	Box->X1 = MIN(x1, x2);
	Box->Y1 = MIN(y1, y2);
	Box->X2 = MAX(x1, x2);
	Box->Y2 = MAX(y1, y2);
}
