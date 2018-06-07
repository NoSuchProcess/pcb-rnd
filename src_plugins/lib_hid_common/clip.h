/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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

#ifndef PCB_CLIP_H
#define PCB_CLIP_H


#include "global_typedefs.h"

/* Clip X,Y to the given bounding box, plus a margin.  Returns pcb_true if
   there is something left to be drawn.  */
PCB_INLINE pcb_bool pcb_line_clip(double minx, double miny, double maxx, double maxy, double *x1, double *y1, double *x2, double *y2, double margin)
{
	double d, r;

	minx -= margin;
	miny -= margin;
	maxx += margin;
	maxy += margin;

	/* clip first point on left side */
	if (*x1 < minx) {
		if (*x2 < minx)
			return pcb_false;
		d = *x2 - *x1;
		r = (minx - *x1) / d;
		*x1 = minx;
		*y1 += r * (*y2 - *y1);
	}
	/* clip second point on left side */
	if (*x2 < minx) {
		d = *x1 - *x2;
		r = (minx - *x2) / d;
		*x2 = minx;
		*y2 += r * (*y1 - *y2);
	}
	/* clip first point on right side */
	if (*x1 > maxx) {
		if (*x2 > maxx)
			return pcb_false;
		d = *x2 - *x1;
		r = (maxx - *x1) / d;
		*x1 = maxx;
		*y1 += r * (*y2 - *y1);
	}
	/* clip second point on right side */
	if (*x2 > maxx) {
		d = *x1 - *x2;
		r = (maxx - *x2) / d;
		*x2 = maxx;
		*y2 += r * (*y1 - *y2);
	}

	/* clip first point on top */
	if (*y1 < miny) {
		if (*y2 < miny)
			return pcb_false;
		d = *y2 - *y1;
		r = (miny - *y1) / d;
		*y1 = miny;
		*x1 += r * (*x2 - *x1);
	}
	/* clip second point on top */
	if (*y2 < miny) {
		d = *y1 - *y2;
		r = (miny - *y2) / d;
		*y2 = miny;
		*x2 += r * (*x1 - *x2);
	}
	/* clip first point on bottom */
	if (*y1 > maxy) {
		if (*y2 > maxy)
			return pcb_false;
		d = *y2 - *y1;
		r = (maxy - *y1) / d;
		*y1 = maxy;
		*x1 += r * (*x2 - *x1);
	}
	/* clip second point on top */
	if (*y2 > maxy) {
		d = *y1 - *y2;
		r = (maxy - *y2) / d;
		*y2 = maxy;
		*x2 += r * (*x1 - *x2);
	}
	return pcb_true;
}

#endif
