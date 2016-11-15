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

struct pcb_pin_s {
	PCB_ANYOBJECTFIELDS;
	pcb_coord_t Thickness, Clearance, Mask, DrillingHole;
	pcb_coord_t X, Y;                   /* center and diameter */
	char *Name, *Number;
	void *Element;
	void *Spare;
	gdl_elem_t link;              /* a pin is in a list (element) */
};


pcb_pin_t *pcb_via_alloc(pcb_data_t * data);
void pcb_via_free(pcb_pin_t * data);
pcb_pin_t *pcb_pin_alloc(pcb_element_t * element);
void pcb_pin_free(pcb_pin_t * data);

pcb_pin_t *pcb_via_new(pcb_data_t *Data, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags);
pcb_pin_t *pcb_element_pin_new(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, char *Name, char *Number, pcb_flag_t Flags);
void pcb_add_via(pcb_data_t *Data, pcb_pin_t *Via);
void pcb_pin_bbox(pcb_pin_t *Pin);

pcb_bool pcb_pin_change_hole(pcb_pin_t *Via);

/* This is the extents of a Pin or Via, depending on whether it's a
   hole or not.  */
#define PIN_SIZE(pinptr) \
	(PCB_FLAG_TEST(PCB_FLAG_HOLE, (pinptr)) \
	? (pinptr)->DrillingHole \
	: (pinptr)->Thickness)


#define pcb_via_move(v,dx,dy) \
	do { \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy); \
		pcb_pin_t *__v__ = (v); \
		PCB_MOVE((__v__)->X,(__v__)->Y,(__dx__),(__dy__)) \
		PCB_BOX_MOVE_LOWLEVEL(&((__v__)->BoundingBox),(__dx__),(__dy__)); \
	} while(0)

#define pcb_pin_move(p,dx,dy) pcb_via_move(p, dx, dy)

#define VIA_LOOP(top) do {                                          \
  pcb_pin_t *via;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(top)->Via, &__it__, via) {

#define PIN_LOOP(element) do {                                      \
  pcb_pin_t *pin;                                                     \
  gdl_iterator_t __it__;                                            \
  pinlist_foreach(&(element)->Pin, &__it__, pin) {

#define ALLPIN_LOOP(top)   \
        ELEMENT_LOOP(top); \
        PIN_LOOP(element)  \

#endif
