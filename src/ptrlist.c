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

#warning TODO: remove this in favor of vtptr

#include "global_typedefs.h"
#include "config.h"
#include "ptrlist.h"
#include <stdlib.h>
#include <string.h>

#define STEP_POINT 100

void **GetPointerMemory(PointerListTypePtr list)
{
	void **ptr = list->Ptr;

	/* realloc new memory if necessary and clear it */
	if (list->PtrN >= list->PtrMax) {
		list->PtrMax = STEP_POINT + (2 * list->PtrMax);
		ptr = (void **) realloc(ptr, list->PtrMax * sizeof(void *));
		list->Ptr = ptr;
		memset(ptr + list->PtrN, 0, (list->PtrMax - list->PtrN) * sizeof(void *));
	}
	return (ptr + list->PtrN++);
}

void FreePointerListMemory(PointerListTypePtr list)
{
	free(list->Ptr);
	memset(list, 0, sizeof(PointerListType));
}
