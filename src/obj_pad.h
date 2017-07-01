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

/* Drawing primitive: smd pads */

#ifndef PCB_OBJ_PAD_H
#define PCB_OBJ_PAD_H

#include "obj_common.h"

struct pcb_pad_s {                  /* a SMD pad */
	PCB_ANYLINEFIELDS;
	pcb_coord_t Mask;
	char *Name, *Number;           /* 'Line' */
	void *Element;
	void *Spare;
	gdl_elem_t link;               /* a pad is in a list (element) */
};


pcb_pad_t *pcb_pad_alloc(pcb_element_t * element);
void pcb_pad_free(pcb_pad_t * data);

pcb_pad_t *pcb_element_pad_new(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags);
pcb_pad_t *pcb_element_pad_new_rect(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Clearance, pcb_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags);

void pcb_pad_bbox(pcb_pad_t *Pad);

/* Calculate the net copper bbox and return it in out */
void pcb_pad_copper_bbox(pcb_box_t *out, pcb_pad_t *Pad);


pcb_bool pcb_pad_change_paste(pcb_pad_t *Pad);

/* hash */
int pcb_pad_eq(const pcb_element_t *e1, const pcb_pad_t *p1, const pcb_element_t *e2, const pcb_pad_t *p2);
int pcb_pad_eq_padstack(const pcb_pad_t *p1, const pcb_pad_t *p2);
unsigned int pcb_pad_hash(const pcb_element_t *e, const pcb_pad_t *p);
unsigned int pcb_pad_hash_padstack(const pcb_pad_t *p);


/* Rather than move the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define pcb_pad_move(p,dx,dy) \
	do { \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy); \
		pcb_pad_t *__p__ = (p); \
		PCB_MOVE((__p__)->Point1.X,(__p__)->Point1.Y,(__dx__),(__dy__)) \
		PCB_MOVE((__p__)->Point2.X,(__p__)->Point2.Y,(__dx__),(__dy__)) \
		pcb_pad_bbox((__p__)); \
	} while(0)

#define	PCB_PAD_ROTATE90(p,x0,y0,n)	\
	pcb_line_rotate90(((pcb_line_t *) (p)),(x0),(y0),(n))

#define PCB_PAD_LOOP(element) do {                                      \
	pcb_pad_t *pad;                                                     \
	gdl_iterator_t __it__;                                            \
	padlist_foreach(&(element)->Pad, &__it__, pad) {

#define PCB_PAD_ALL_LOOP(top)    \
	PCB_ELEMENT_LOOP(top);        \
		PCB_PAD_LOOP(element)


#endif

