/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: pins and vias */

#ifndef PCB_OBJ_PINVIA_H
#define PCB_OBJ_PINVIA_H

#include <genlist/gendlist.h>
#include "obj_common.h"

struct pcb_pin_s {
	PCB_ANYOBJECTFIELDS;
	pcb_coord_t Thickness, Clearance, Mask, DrillingHole;
	pcb_coord_t X, Y;                   /* center and diameter */
	char *Name, *Number;
	void *Element;
	gdl_elem_t link;              /* a pin is in a list (element) */
};

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

/* Rotate pin shape style by n_in * 90 degrees */
#define PCB_PIN_ROTATE_SHAPE(p,n_in) \
do { \
	int _n_; \
	for(_n_ = n_in;_n_>0;_n_--) { \
		int _old_, _nw_ = 0; \
		_old_ = PCB_FLAG_SQUARE_GET(p); \
		if ((_old_ > 1) && (_old_ < 17)) { \
			_old_--; \
			if (_old_ & 1) \
				_nw_ |= 8; \
			if (_old_ & 8) \
				_nw_ |= 2; \
			if (_old_ & 2) \
				_nw_ |= 4; \
			if (_old_ & 4) \
				_nw_ |= 1; \
			PCB_FLAG_SQUARE_GET(p) = _nw_+1; \
		} \
	} \
} while(0)

#define	PCB_VIA_ROTATE90(v,x0,y0,n)	\
do { \
	PCB_COORD_ROTATE90((v)->X,(v)->Y,(x0),(y0),(n)); \
	PCB_PIN_ROTATE_SHAPE(v, (n)); \
} while(0)

#define	PCB_PIN_ROTATE90(p,x0,y0,n)	\
do { \
	PCB_COORD_ROTATE90((p)->X,(p)->Y,(x0),(y0),(n)); \
	PCB_PIN_ROTATE_SHAPE((p), (n)); \
} while(0)


#endif
