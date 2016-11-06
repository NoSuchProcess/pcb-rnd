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

/* Drawing primitive: pins and vias */

#ifndef PCB_OBJ_PINVIA_H
#define PCB_OBJ_PINVIA_H

#include "obj_common.h"

struct pin_st {
	ANYOBJECTFIELDS;
	Coord Thickness, Clearance, Mask, DrillingHole;
	Coord X, Y;                   /* center and diameter */
	char *Name, *Number;
	void *Element;
	void *Spare;
	gdl_elem_t link;              /* a pin is in a list (element) */
};


PinType *GetViaMemory(DataType * data);
void RemoveFreeVia(PinType * data);
PinType *GetPinMemory(ElementType * element);
void RemoveFreePin(PinType * data);

PinTypePtr CreateNewVia(DataTypePtr Data, Coord X, Coord Y, Coord Thickness, Coord Clearance, Coord Mask, Coord DrillingHole, const char *Name, FlagType Flags);
PinTypePtr CreateNewPin(ElementTypePtr Element, Coord X, Coord Y, Coord Thickness, Coord Clearance, Coord Mask, Coord DrillingHole, char *Name, char *Number, FlagType Flags);
void pcb_add_via(DataType *Data, PinType *Via);
void SetPinBoundingBox(PinTypePtr Pin);
pcb_bool ChangeHole(PinTypePtr Via);

/* This is the extents of a Pin or Via, depending on whether it's a
   hole or not.  */
#define PIN_SIZE(pinptr) \
	(TEST_FLAG(PCB_FLAG_HOLE, (pinptr)) \
	? (pinptr)->DrillingHole \
	: (pinptr)->Thickness)


#define	MOVE_VIA_LOWLEVEL(v,dx,dy) \
        { \
	        MOVE((v)->X,(v)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((v)->BoundingBox),(dx),(dy));		\
	}

#define	MOVE_PIN_LOWLEVEL(p,dx,dy) \
	{ \
		MOVE((p)->X,(p)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((p)->BoundingBox),(dx),(dy));		\
	}


#define VIA_LOOP(top) do {                                          \
  PinType *via;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Via, &__it__, via) {

#define PIN_LOOP(element) do {                                      \
  PinType *pin;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(element)->Pin, &__it__, pin) {

#define ALLPIN_LOOP(top)   \
        ELEMENT_LOOP(top); \
        PIN_LOOP(element)  \


#endif
