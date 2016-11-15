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

pcb_pad_t *pcb_element_pad_new(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, char *Name, char *Number, pcb_flag_t Flags);
void pcb_pad_bbox(pcb_pad_t *Pad);

pcb_bool pcb_pad_change_paste(pcb_pad_t *Pad);


/* Rather than move the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define	MOVE_PAD_LOWLEVEL(p,dx,dy)              \
	{                                             \
		PCB_MOVE((p)->Point1.X,(p)->Point1.Y,(dx),(dy)) \
		PCB_MOVE((p)->Point2.X,(p)->Point2.Y,(dx),(dy)) \
		pcb_pad_bbox((p));                    \
	}

#define PAD_LOOP(element) do {                                      \
	pcb_pad_t *pad;                                                     \
	gdl_iterator_t __it__;                                            \
	padlist_foreach(&(element)->Pad, &__it__, pad) {

#define	ALLPAD_LOOP(top)    \
	ELEMENT_LOOP(top);        \
		PAD_LOOP(element)


#endif

