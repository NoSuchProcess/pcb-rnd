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
#include "hid.h"

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
PCB_INLINE void pcb_draw_wireframe_arc(pcb_hid_gc_t gc, pcb_arc_t *arc, pcb_coord_t thick)
{
	pcb_coord_t wid = thick / 2;
	pcb_coord_t x1, y1, x2, y2;

	pcb_arc_get_end(arc, 0, &x1, &y1);
	pcb_arc_get_end(arc, 1, &x2, &y2);

	pcb_render->draw_arc(gc, arc->X, arc->Y, arc->Width + wid, arc->Height + wid, arc->StartAngle, arc->Delta);
	if (wid > pcb_pixel_slop) {
		pcb_render->draw_arc(gc, arc->X, arc->Y, arc->Width - wid, arc->Height - wid, arc->StartAngle, arc->Delta);
		pcb_render->draw_arc(gc, x1, y1, wid, wid, arc->StartAngle, -180 * SGN(arc->Delta));
		pcb_render->draw_arc(gc, x2, y2, wid, wid, arc->StartAngle + arc->Delta, 180 * SGN(arc->Delta));
	}
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
PCB_INLINE void pcb_draw_wireframe_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick, int square)
{
	if((x1 != x2) || (y1 != y2)) {
		double dx = x2 - x1;
		double dy = y2 - y1;
		double h = 0.5 * thick / sqrt(PCB_SQUARE(dx) + PCB_SQUARE(dy));
		pcb_coord_t ox = dy * h + 0.5 * SGN(dy);
		pcb_coord_t oy = -(dx * h + 0.5 * SGN(dx));
		if (square) {
			/* make the line longer by cap */
			x1 -= dx * h;
			x2 += dx * h;
			y1 -= dy * h;
			y2 += dy * h;
		}
		if ((coord_abs(ox) >= pcb_pixel_slop) || (coord_abs(oy) >= pcb_pixel_slop)) {
			pcb_render->draw_line(gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
			pcb_render->draw_line(gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);

			/* draw caps */
			if (!square) {
				pcb_angle_t angle = atan2(dx, dy) * 57.295779;
				pcb_render->draw_arc(gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
				pcb_render->draw_arc(gc, x2, y2, thick / 2, thick / 2, angle, 180);
			}
			else {
				pcb_render->draw_line(gc, x1 + ox, y1 + oy, x1 - ox, y1 - oy);
				pcb_render->draw_line(gc, x2 + ox, y2 + oy, x2 - ox, y2 - oy);
			}
		}
		else
			pcb_render->draw_line(gc, x1, y1, x2, y2);
	}
	else {
		if (square) {
			/* square cap 0 long line does not have an angle -> always draw it axis aligned */
			pcb_coord_t cx1 = x1 - thick/2, cx2 = x1 + thick/2, cy1 = y1 - thick/2, cy2 = y1 + thick/2;
			pcb_render->draw_line(gc, cx1, cy1, cx2, cy1);
			pcb_render->draw_line(gc, cx2, cy1, cx2, cy2);
			pcb_render->draw_line(gc, cx2, cy2, cx1, cy2);
			pcb_render->draw_line(gc, cx1, cy2, cx1, cy1);
		}
		else
			pcb_render->draw_arc(gc, x1, y1, thick/2, thick/2, 0, 360); 
	}
}

#endif /* ! defined PCB_DRAW_WIREFRAME_H */

