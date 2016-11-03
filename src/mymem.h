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

/* prototypes for memory routines
 */

#ifndef	PCB_MYMEM_H
#define	PCB_MYMEM_H

#include "config.h"

#include <stdlib.h>
#include "config.h"
#include "rubberband.h"

/* ---------------------------------------------------------------------------
 * number of additional objects that are allocated with one system call
 */
#define	STEP_ELEMENT		50
#define STEP_DRILL		30
#define STEP_POINT		100
#define	STEP_SYMBOLLINE		10
#define	STEP_SELECTORENTRY	128
#define	STEP_REMOVELIST		500
#define	STEP_UNDOLIST		500
#define	STEP_POLYGONPOINT	10
#define	STEP_POLYGONHOLEINDEX	10
#define	STEP_RUBBERBAND		100

/* ---------------------------------------------------------------------------
 * some memory types
 */

PinTypePtr GetPinMemory(ElementTypePtr);
PadTypePtr GetPadMemory(ElementTypePtr);
PinTypePtr GetViaMemory(DataTypePtr);
LineTypePtr GetLineMemory(LayerTypePtr);
ArcTypePtr GetArcMemory(LayerTypePtr);
RatTypePtr GetRatMemory(DataTypePtr);
TextTypePtr GetTextMemory(LayerTypePtr);
PolygonTypePtr GetPolygonMemory(LayerTypePtr);
PointTypePtr GetPointMemoryInPolygon(PolygonTypePtr);
pcb_cardinal_t *GetHoleIndexMemoryInPolygon(PolygonTypePtr);
ElementTypePtr GetElementMemory(DataTypePtr);
void FreePolygonMemory(PolygonTypePtr);
void FreeElementMemory(ElementTypePtr);

char *StripWhiteSpaceAndDup(const char *);

void RemoveFreeArc(ArcType * data);
void RemoveFreeLine(LineType * data);
void RemoveFreeText(TextType * data);
void RemoveFreePolygon(PolygonType * data);
void RemoveFreePin(PinType * data);
void RemoveFreePad(PadType * data);
void RemoveFreeVia(PinType * data);
void RemoveFreeElement(ElementType * data);
void RemoveFreeRat(RatType * data);

/* Allocate element-objects */
LineType *GetElementLineMemory(ElementType *Element);
ArcType *GetElementArcMemory(ElementType *Element);

#ifndef HAVE_LIBDMALLOC
#define malloc(x) calloc(1,(x))
#endif

#endif
