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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef	PCB_ROTATE_H
#define	PCB_ROTATE_H

#include "config.h"
#include "global_typedefs.h"
#include "compat_misc.h"

/*** Transformation macros ***/
#define	PCB_COORD_ROTATE90(x,y,x0,y0,n)							\
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

#define PCB_ROTATE_TYPES (PCB_OBJ_PSTK | PCB_OBJ_SUBC | PCB_OBJ_TEXT | PCB_OBJ_ARC | PCB_OBJ_LINE_POINT | PCB_OBJ_LINE | PCB_OBJ_POLY | PCB_OBJ_FLOATER)

/* rotates an object passed;
 * the center of rotation is determined by the current cursor location
 */
void *pcb_obj_rotate90(int, void *, void *, void *, pcb_coord_t, pcb_coord_t, unsigned);

/* rotates an objects passed;
 * the center of rotation is determined by the current cursor location */
void *pcb_obj_rotate(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t X, pcb_coord_t Y, pcb_angle_t angle);

void pcb_screen_obj_rotate90(pcb_coord_t, pcb_coord_t, unsigned);
void pcb_screen_obj_rotate(pcb_coord_t X, pcb_coord_t Y, pcb_angle_t angle);
void pcb_point_rotate90(pcb_point_t *Point, pcb_coord_t X, pcb_coord_t Y, unsigned Number);

PCB_INLINE void pcb_rotate(pcb_coord_t * x, pcb_coord_t * y, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina)
{
	double nx, ny;
	pcb_coord_t px = *x - cx;
	pcb_coord_t py = *y - cy;

	nx = pcb_round(px * cosa + py * sina + cx);
	ny = pcb_round(py * cosa - px * sina + cy);

	*x = nx;
	*y = ny;
}

#endif
