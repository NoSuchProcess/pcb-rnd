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

/* prototypes connection search routines */

#ifndef	PCB_FIND_H
#define	PCB_FIND_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "config.h"

typedef enum {
	FCT_COPPER = 1,								/* copper connection */
	FCT_INTERNAL = 2,							/* element-internal connection */
	FCT_RAT = 4,									/* connected by a rat line */
	FCT_ELEMENT = 8,							/* pin/pad is part of an element whose pins/pads are being listed */
	FCT_START = 16								/* starting object of a query */
} pcb_found_conn_type_t;

typedef void (*find_callback_t) (int current_type, void *current_ptr, int from_type, void *from_ptr,
																 pcb_found_conn_type_t conn_type);


/* if not NULL, this function is called whenever something is found
   (in LookupConnections for example). The caller should save the original
   value and set that back around the call, if the callback needs to be changed.
   */
extern find_callback_t find_callback;

/* ---------------------------------------------------------------------------
 * some local defines
 */
#define LOOKUP_FIRST	\
	(PCB_TYPE_PIN | PCB_TYPE_PAD)
#define LOOKUP_MORE	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_RATLINE | PCB_TYPE_POLYGON | PCB_TYPE_ARC)
#define SILK_TYPE	\
	(PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLYGON)

pcb_bool pcb_intersect_line_line(pcb_line_t *, pcb_line_t *);
pcb_bool pcb_intersect_line_arc(pcb_line_t *, pcb_arc_t *);
pcb_bool pcb_intersect_line_pin(pcb_pin_t *, pcb_line_t *);
pcb_bool pcb_intersect_line_pad(pcb_line_t *, pcb_pad_t *);
pcb_bool pcb_intersect_arc_pad(pcb_arc_t *, pcb_pad_t *);
pcb_bool pcb_is_poly_in_poly(pcb_polygon_t *, pcb_polygon_t *);
void LookupElementConnections(pcb_element_t *, FILE *);
void LookupConnectionsToAllElements(FILE *);
void LookupConnection(pcb_coord_t, pcb_coord_t, pcb_bool, pcb_coord_t, int);
void LookupConnectionByPin(int type, void *ptr1);
void LookupUnusedPins(FILE *);
pcb_bool ResetFoundLinesAndPolygons(pcb_bool);
pcb_bool ResetFoundPinsViasAndPads(pcb_bool);
pcb_bool ResetConnections(pcb_bool);
void InitConnectionLookup(void);
void InitComponentLookup(void);
void InitLayoutLookup(void);
void FreeConnectionLookupMemory(void);
void FreeComponentLookupMemory(void);
void FreeLayoutLookupMemory(void);
void RatFindHook(int, void *, void *, void *, pcb_bool, pcb_bool);
void SaveFindFlag(int);
void RestoreFindFlag(void);
int DRCAll(void);
pcb_bool IsLineInPolygon(pcb_line_t *, pcb_polygon_t *);
pcb_bool IsArcInPolygon(pcb_arc_t *, pcb_polygon_t *);
pcb_bool IsPadInPolygon(pcb_pad_t *, pcb_polygon_t *);

/* find_clear.c */
pcb_bool ClearFlagOnPinsViasAndPads(pcb_bool AndDraw, int flag);
pcb_bool ClearFlagOnLinesAndPolygons(pcb_bool AndDraw, int flag);
pcb_bool ClearFlagOnAllObjects(pcb_bool AndDraw, int flag);

#endif
