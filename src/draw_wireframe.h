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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */
#ifndef PCB_DRAW_WIREFRAME_H
#define PCB_DRAW_WIREFRAME_H

#include "config.h"
#include "hid.h"

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
static inline PCB_FUNC_UNUSED void pcb_draw_wireframe_arc(pcb_hid_gc_t gc, pcb_arc_t *arc)
{
	pcb_coord_t wid = arc->Thickness / 2;
	pcb_coord_t x1, y1, x2, y2;

	pcb_arc_get_end(arc, 0, &x1, &y1);
	pcb_arc_get_end(arc, 1, &x2, &y2);

	pcb_gui->draw_arc(gc, arc->X, arc->Y, arc->Width + wid, arc->Height + wid, arc->StartAngle, arc->Delta);
	if (wid > pcb_pixel_slop) {
		pcb_gui->draw_arc(gc, arc->X, arc->Y, arc->Width - wid, arc->Height - wid, arc->StartAngle, arc->Delta);
		pcb_gui->draw_arc(gc, x1, y1, wid, wid, arc->StartAngle, -180 * SGN(arc->Delta));
		pcb_gui->draw_arc(gc, x2, y2, wid, wid, arc->StartAngle + arc->Delta, 180 * SGN(arc->Delta));
	}
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static inline PCB_FUNC_UNUSED void pcb_draw_wireframe_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick, int square)
{
	if((x1 != x2) || (y1 != y2)) {
		double dx = x2 - x1;
		double dy = y2 - y1;
		double h = 0.5 * thick / sqrt(PCB_SQUARE(dx) + PCB_SQUARE(dy));
		pcb_coord_t ox = dy * h + 0.5 * SGN(dy);
		pcb_coord_t oy = -(dx * h + 0.5 * SGN(dx));

		if (coord_abs(ox) >= pcb_pixel_slop || coord_abs(oy) >= pcb_pixel_slop) {
			pcb_angle_t angle = atan2(dx, dy) * 57.295779;
			pcb_gui->draw_line(gc, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
			pcb_gui->draw_line(gc, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
			pcb_gui->draw_arc(gc, x1, y1, thick / 2, thick / 2, angle - 180, 180);
			pcb_gui->draw_arc(gc, x2, y2, thick / 2, thick / 2, angle, 180);
		}
		else
			pcb_gui->draw_line(gc, x1, y1, x2, y2);
	}
	else
			pcb_gui->draw_arc(gc, x1, y1, thick/2, thick/2, 0, 360); 
}

#endif /* ! defined PCB_DRAW_WIREFRAME_H */

