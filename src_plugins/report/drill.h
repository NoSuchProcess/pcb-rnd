/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This module, drill.h, was written and is Copyright (C) 1997 harry eaton
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
#ifndef PCB_DRILL_H
#define PCB_DRILL_H

typedef struct {								/* holds drill information */
	Coord DrillSize;							/* this drill's diameter */
	pcb_cardinal_t ElementN,						/* the number of elements using this drill size */
	  ElementMax,									/* max number of elements from malloc() */
	  PinCount,										/* number of pins drilled this size */
	  ViaCount,										/* number of vias drilled this size */
	  UnplatedCount,							/* number of these holes that are unplated */
	  PinN,												/* number of drill coordinates in the list */
	  PinMax;											/* max number of coordinates from malloc() */
	PinTypePtr *Pin;							/* coordinates to drill */
	ElementTypePtr *Element;			/* a pointer to an array of element pointers */
} DrillType, *DrillTypePtr;

typedef struct {								/* holds a range of Drill Infos */
	pcb_cardinal_t DrillN,							/* number of drill sizes */
	  DrillMax;										/* max number from malloc() */
	DrillTypePtr Drill;						/* plated holes */
} DrillInfoType, *DrillInfoTypePtr;

DrillInfoTypePtr GetDrillInfo(DataTypePtr);
void FreeDrillInfo(DrillInfoTypePtr);
void RoundDrillInfo(DrillInfoTypePtr, int);
DrillTypePtr GetDrillInfoDrillMemory(DrillInfoTypePtr);
PinTypeHandle GetDrillPinMemory(DrillTypePtr);
ElementTypeHandle GetDrillElementMemory(DrillTypePtr);

#define DRILL_LOOP(top) do             {                           \
        pcb_cardinal_t        n;                                   \
        DrillTypePtr    drill;                                     \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)   \
        {                                                          \
                drill = &(top)->Drill[n]

#endif
