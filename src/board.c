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
#include "config.h"
#include "board.h"
#include "data.h"

PCBTypePtr PCB;									/* pointer to layout struct */

/* ---------------------------------------------------------------------------
 * free memory used by PCB
 */
void FreePCBMemory(PCBType * pcb)
{
	int i;

	if (pcb == NULL)
		return;

	free(pcb->Name);
	free(pcb->Filename);
	free(pcb->PrintFilename);
	rats_patch_destroy(pcb);
	FreeDataMemory(pcb->Data);
	free(pcb->Data);
	/* release font symbols */
	for (i = 0; i <= MAX_FONTPOSITION; i++)
		free(pcb->Font.Symbol[i].Line);
	for (i = 0; i < NUM_NETLISTS; i++)
		FreeLibraryMemory(&(pcb->NetlistLib[i]));
	vtroutestyle_uninit(&pcb->RouteStyle);
	FreeAttributeListMemory(&pcb->Attributes);
	/* clear struct */
	memset(pcb, 0, sizeof(PCBType));
}
