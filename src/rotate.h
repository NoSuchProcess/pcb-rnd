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

/* prototypes for transform routines */

#ifndef	PCB_ROTATE_H
#define	PCB_ROTATE_H

#include "config.h"
#include "global_typedefs.h"

/* ---------------------------------------------------------------------------
 * some useful transformation macros and constants
 */
#define	PCB_ROTATE90(x,y,x0,y0,n)							\
	do {												\
		pcb_coord_t	dx = (x)-(x0),					\
					dy = (y)-(y0);					\
													\
		switch(n & 0x03)									\
		{											\
			case 1:		(x)=(x0)+dy; (y)=(y0)-dx;	\
						break;						\
			case 2:		(x)=(x0)-dx; (y)=(y0)-dy;	\
						break;						\
			case 3:		(x)=(x0)-dy; (y)=(y0)+dx;	\
						break;						\
			default:	break;						\
		}											\
	} while(0)

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
	PCB_ROTATE90((v)->X,(v)->Y,(x0),(y0),(n)); \
	PCB_PIN_ROTATE_SHAPE(v, (n)); \
} while(0)

#define	PCB_PIN_ROTATE90(p,x0,y0,n)	\
do { \
	PCB_ROTATE90((p)->X,(p)->Y,(x0),(y0),(n)); \
	PCB_PIN_ROTATE_SHAPE((p), (n)); \
} while(0)

#define	PCB_PAD_ROTATE90(p,x0,y0,n)	\
	pcb_line_rotate90(((pcb_line_t *) (p)),(x0),(y0),(n))

#define	ROTATE_TYPES	(PCB_TYPE_ELEMENT | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_ARC)


void pcb_box_rotate90(pcb_box_t *, pcb_coord_t, pcb_coord_t, unsigned);
void pcb_poly_rotate90(pcb_polygon_t *, pcb_coord_t, pcb_coord_t, unsigned);
void *pcb_obj_rotate90(int, void *, void *, void *, pcb_coord_t, pcb_coord_t, unsigned);
void pcb_screen_obj_rotate90(pcb_coord_t, pcb_coord_t, unsigned);

void pcb_point_rotate90(pcb_point_t *Point, pcb_coord_t X, pcb_coord_t Y, unsigned Number);

static inline PCB_FUNC_UNUSED void pcb_rotate(pcb_coord_t * x, pcb_coord_t * y, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina)
{
	double nx, ny;
	pcb_coord_t px = *x - cx;
	pcb_coord_t py = *y - cy;

	nx = px * cosa + py * sina;
	ny = py * cosa - px * sina;

	*x = nx + cx;
	*y = ny + cy;
}

#endif
