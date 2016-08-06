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
 *  RCS: $Id$
 */

/* prototypes connection search routines
 */

#ifndef	PCB_FIND_H
#define	PCB_FIND_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "global.h"

typedef enum {
	FCT_COPPER = 1,								/* copper connection */
	FCT_INTERNAL = 2,							/* element-internal connection */
	FCT_RAT = 4,									/* connected by a rat line */
	FCT_ELEMENT = 8,							/* pin/pad is part of an element whose pins/pads are being listed */
	FCT_START = 16								/* starting object of a query */
} found_conn_type_t;

typedef void (*find_callback_t) (int current_type, void *current_ptr, int from_type, void *from_ptr,
																 found_conn_type_t conn_type);


/* if not NULL, this function is called whenever something is found
   (in LookupConnections for example). The caller should save the original
   value and set that back around the call, if the callback needs to be changed.
   */
find_callback_t find_callback;

/* ---------------------------------------------------------------------------
 * some local defines
 */
#define LOOKUP_FIRST	\
	(PIN_TYPE | PAD_TYPE)
#define LOOKUP_MORE	\
	(VIA_TYPE | LINE_TYPE | RATLINE_TYPE | POLYGON_TYPE | ARC_TYPE)
#define SILK_TYPE	\
	(LINE_TYPE | ARC_TYPE | POLYGON_TYPE)

bool LineLineIntersect(LineTypePtr, LineTypePtr);
bool LineArcIntersect(LineTypePtr, ArcTypePtr);
bool PinLineIntersect(PinTypePtr, LineTypePtr);
bool LinePadIntersect(LineTypePtr, PadTypePtr);
bool ArcPadIntersect(ArcTypePtr, PadTypePtr);
bool IsPolygonInPolygon(PolygonTypePtr, PolygonTypePtr);
void LookupElementConnections(ElementTypePtr, FILE *);
void LookupConnectionsToAllElements(FILE *);
void LookupConnection(Coord, Coord, bool, Coord, int);
void LookupUnusedPins(FILE *);
bool ResetFoundLinesAndPolygons(bool);
bool ResetFoundPinsViasAndPads(bool);
bool ResetConnections(bool);
void InitConnectionLookup(void);
void InitComponentLookup(void);
void InitLayoutLookup(void);
void FreeConnectionLookupMemory(void);
void FreeComponentLookupMemory(void);
void FreeLayoutLookupMemory(void);
void RatFindHook(int, void *, void *, void *, bool, bool);
void SaveFindFlag(int);
void RestoreFindFlag(void);
int DRCAll(void);
bool IsLineInPolygon(LineTypePtr, PolygonTypePtr);
bool IsArcInPolygon(ArcTypePtr, PolygonTypePtr);
bool IsPadInPolygon(PadTypePtr, PolygonTypePtr);

#endif
