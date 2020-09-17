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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#ifndef PCB_DRAW_WIREFRAME_H
#define PCB_DRAW_WIREFRAME_H

#include "config.h"
#include <librnd/core/hid.h>

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
RND_INLINE void pcb_draw_wireframe_arc(rnd_hid_gc_t gc, pcb_arc_t *arc, rnd_coord_t thick)
{
	rnd_coord_t wid = thick / 2;
	rnd_coord_t x1, y1, x2, y2;

	if ((arc->Width == 0) && (arc->Height == 0)) {
		rnd_render->draw_arc(gc, arc->X, arc->Y, wid, wid, 0, 360);
		return;
	}

	pcb_arc_get_end(arc, 0, &x1, &y1);
	pcb_arc_get_end(arc, 1, &x2, &y2);

	rnd_render->draw_arc(gc, arc->X, arc->Y, arc->Width + wid, arc->Height + wid, arc->StartAngle, arc->Delta);
	if (wid > rnd_pixel_slop) {
		rnd_coord_t w = (wid < arc->Width) ? (arc->Width - wid) : 0;
		rnd_coord_t h = (wid < arc->Height) ? (arc->Height - wid) : 0;
		rnd_render->draw_arc(gc, arc->X, arc->Y, w, h, arc->StartAngle, arc->Delta);
		rnd_render->draw_arc(gc, x1, y1, wid, wid, arc->StartAngle, -180 * SGN(arc->Delta));
		rnd_render->draw_arc(gc, x2, y2, wid, wid, arc->StartAngle + arc->Delta, 180 * SGN(arc->Delta));
	}
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
RND_INLINE void pcb_draw_wireframe_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick, int square)
{
	if((x1 != x2) || (y1 != y2)) {
		double dx = x2 - x1;
		double dy = y2 - y1;
		double h = 0.5 * thick / sqrt(RND_SQUARE(dx) + RND_SQUARE(dy));
		rnd_coord_t ox = dy * h + 0.5 * SGN(dy);
		rnd_coord_t oy = -(dx * h + 0.5 * SGN(dx));
		if (square) {
			/* make the line longer by cap */
			x1 -= dx * h;
			x2 += dx * h;
			y1 -= dy * h;
			y2 += dy * h;
		}
		if ((coord_abs(ox) >= rnd_pixel_slop) || (coord_abs(oy) >= rnd_pixel_slop)) {
			rnd_render->draw_line(gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
			rnd_render->draw_line(gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);

			/* draw caps */
			if (!square) {
				rnd_angle_t angle = atan2(dx, dy) * 57.295779;
				rnd_render->draw_arc(gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
				rnd_render->draw_arc(gc, x2, y2, thick / 2, thick / 2, angle, 180);
			}
			else {
				rnd_render->draw_line(gc, x1 + ox, y1 + oy, x1 - ox, y1 - oy);
				rnd_render->draw_line(gc, x2 + ox, y2 + oy, x2 - ox, y2 - oy);
			}
		}
		else
			rnd_render->draw_line(gc, x1, y1, x2, y2);
	}
	else {
		if (square) {
			/* square cap 0 long line does not have an angle -> always draw it axis aligned */
			rnd_coord_t cx1 = x1 - thick/2, cx2 = x1 + thick/2, cy1 = y1 - thick/2, cy2 = y1 + thick/2;
			rnd_render->draw_line(gc, cx1, cy1, cx2, cy1);
			rnd_render->draw_line(gc, cx2, cy1, cx2, cy2);
			rnd_render->draw_line(gc, cx2, cy2, cx1, cy2);
			rnd_render->draw_line(gc, cx1, cy2, cx1, cy1);
		}
		else
			rnd_render->draw_arc(gc, x1, y1, thick/2, thick/2, 0, 360); 
	}
}

#endif /* ! defined PCB_DRAW_WIREFRAME_H */

