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

/* prototypes to change object properties */

#ifndef	PCB_CHANGE_H
#define	PCB_CHANGE_H

#include "global.h"

/* ---------------------------------------------------------------------------
 * some defines
 */
#define	CHANGENAME_TYPES        \
	(PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_LINE)

#define	CHANGESIZE_TYPES        \
	(PCB_TYPE_POLYGON | PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_LINE | \
	 PCB_TYPE_ARC | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_ELEMENT)

#define	CHANGE2NDSIZE_TYPES     \
	(PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_ELEMENT)

/* We include polygons here only to inform the user not to do it that way.  */
#define CHANGECLEARSIZE_TYPES	\
	(PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLYGON)

#define	CHANGENONETLIST_TYPES     \
	(PCB_TYPE_ELEMENT)

#define	CHANGESQUARE_TYPES     \
	(PCB_TYPE_ELEMENT | PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA)

#define	CHANGEOCTAGON_TYPES     \
	(PCB_TYPE_ELEMENT | PCB_TYPE_PIN | PCB_TYPE_VIA)

#define CHANGEJOIN_TYPES	\
	(PCB_TYPE_ARC | PCB_TYPE_LINE | PCB_TYPE_TEXT)

#define CHANGETHERMAL_TYPES	\
	(PCB_TYPE_PIN | PCB_TYPE_VIA)

#define CHANGEMASKSIZE_TYPES    \
        (PCB_TYPE_PIN | PCB_TYPE_VIA | PCB_TYPE_PAD)

pcb_bool ChangeLayoutName(char *);
pcb_bool ChangeLayerName(LayerTypePtr, char *);
pcb_bool ChangeSelectedSize(int, Coord, pcb_bool);
pcb_bool ChangeSelectedClearSize(int, Coord, pcb_bool);
pcb_bool ChangeSelected2ndSize(int, Coord, pcb_bool);
pcb_bool ChangeSelectedMaskSize(int, Coord, pcb_bool);
pcb_bool ChangeSelectedJoin(int);
pcb_bool SetSelectedJoin(int);
pcb_bool ClrSelectedJoin(int);
pcb_bool ChangeSelectedNonetlist(int);
pcb_bool ChangeSelectedSquare(int);
pcb_bool SetSelectedSquare(int);
pcb_bool ClrSelectedSquare(int);
pcb_bool ChangeSelectedThermals(int, int);
pcb_bool ChangeSelectedHole(void);
pcb_bool ChangeSelectedPaste(void);
pcb_bool ChangeSelectedOctagon(int);
pcb_bool SetSelectedOctagon(int);
pcb_bool ClrSelectedOctagon(int);
pcb_bool ChangeSelectedElementSide(void);
pcb_bool ChangeElementSide(ElementTypePtr, Coord);
pcb_bool ChangeHole(PinTypePtr);
pcb_bool ChangePaste(PadTypePtr);
pcb_bool ChangeObjectSize(int, void *, void *, void *, Coord, pcb_bool);
pcb_bool ChangeObject1stSize(int, void *, void *, void *, Coord, pcb_bool);
pcb_bool ChangeObjectThermal(int, void *, void *, void *, int);
pcb_bool ChangeObjectClearSize(int, void *, void *, void *, Coord, pcb_bool);
pcb_bool ChangeObject2ndSize(int, void *, void *, void *, Coord, pcb_bool, pcb_bool);
pcb_bool ChangeObjectMaskSize(int, void *, void *, void *, Coord, pcb_bool);
pcb_bool ChangeObjectJoin(int, void *, void *, void *);
pcb_bool SetObjectJoin(int, void *, void *, void *);
pcb_bool ClrObjectJoin(int, void *, void *, void *);
pcb_bool ChangeObjectNonetlist(int Type, void *Ptr1, void *Ptr2, void *Ptr3);
pcb_bool ChangeObjectSquare(int, void *, void *, void *, int);
pcb_bool SetObjectSquare(int, void *, void *, void *);
pcb_bool ClrObjectSquare(int, void *, void *, void *);
pcb_bool ChangeObjectOctagon(int, void *, void *, void *);
pcb_bool SetObjectOctagon(int, void *, void *, void *);
pcb_bool ClrObjectOctagon(int, void *, void *, void *);
void *ChangeObjectName(int, void *, void *, void *, char *);
void *QueryInputAndChangeObjectName(int, void *, void *, void *, int);
void ChangePCBSize(Coord, Coord);
void *ChangeObjectPinnum(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name);
pcb_bool ChangeObjectRadius(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_x, Coord r, pcb_bool absolute);
pcb_bool ChangeObjectAngle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, Angle a, pcb_bool absolute);


/* Change the specified text on an element, either on the board (give
   PCB, PCB->Data) or in a buffer (give NULL, Buffer->Data).  The old
   string is returned, and must be properly freed by the caller.  */
char *ChangeElementText(PCBType * pcb, DataType * data, ElementTypePtr Element, int which, char *new_name);

#endif
