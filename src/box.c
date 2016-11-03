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

#warning TODO: remove this and use genvect

#define STEP_POINT 100
#include <string.h>
#include "box.h"

/* ---------------------------------------------------------------------------
 * get next slot for a box, allocates memory if necessary
 */
BoxTypePtr GetBoxMemory(BoxListTypePtr Boxes)
{
	BoxTypePtr box = Boxes->Box;

	/* realloc new memory if necessary and clear it */
	if (Boxes->BoxN >= Boxes->BoxMax) {
		Boxes->BoxMax = STEP_POINT + (2 * Boxes->BoxMax);
		box = (BoxTypePtr) realloc(box, Boxes->BoxMax * sizeof(BoxType));
		Boxes->Box = box;
		memset(box + Boxes->BoxN, 0, (Boxes->BoxMax - Boxes->BoxN) * sizeof(BoxType));
	}
	return (box + Boxes->BoxN++);
}

/* ---------------------------------------------------------------------------
 * frees memory used by a box list
 */
void FreeBoxListMemory(BoxListTypePtr Boxlist)
{
	if (Boxlist) {
		free(Boxlist->Box);
		memset(Boxlist, 0, sizeof(BoxListType));
	}
}
