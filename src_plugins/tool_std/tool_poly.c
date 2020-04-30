/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "crosshair.h"
#include <librnd/core/actions.h>
#include "polygon.h"
#include <librnd/core/tool.h>


void pcb_tool_poly_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	pcb_crosshair.AttachedPolygon.PointN = 0;
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

TODO("remove this when pcb_tool_poly_notify_mode() loses PCB")
#include "board.h"

void pcb_tool_poly_notify_mode(rnd_hidlib_t *hl)
{
	rnd_point_t *points = pcb_crosshair.AttachedPolygon.Points;
	rnd_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

	/* do update of position */
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST) {
		pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.X;
		pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.Y;
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_THIRD;
		pcb_crosshair.AttachedPolygon_pts = 0;
	}

	/* check if this is the last point of a polygon */
	if (n >= 3 && points[0].X == pcb_crosshair.AttachedLine.Point2.X && points[0].Y == pcb_crosshair.AttachedLine.Point2.Y) {
		rnd_actionva(hl, "Polygon", "Close", NULL);
		return;
	}

	/* Someone clicking twice on the same point ('doubleclick'): close polygon */
	if (n >= 3 && points[n - 1].X == pcb_crosshair.AttachedLine.Point2.X && points[n - 1].Y == pcb_crosshair.AttachedLine.Point2.Y) {
		rnd_actionva(hl, "Polygon", "Close", NULL);
		return;
	}

	/* create new point if it's the first one or if it's
	 * different to the last one
	 */
	if (!n || points[n - 1].X != pcb_crosshair.AttachedLine.Point2.X || points[n - 1].Y != pcb_crosshair.AttachedLine.Point2.Y) {
		pcb_poly_point_new(&pcb_crosshair.AttachedPolygon, pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y);
		pcb_crosshair.AttachedPolygon_pts = pcb_crosshair.AttachedPolygon.PointN;

		/* copy the coordinates */
		pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
		pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
	}

	if (conf_core.editor.orthogonal_moves) {
		/* set the mark to the new starting point so ortho works */
		hl->tool_grabbed.X = hl->tool_x;
		hl->tool_grabbed.Y = hl->tool_y;
		hl->tool_grabbed.status = rnd_true;
	}
}

void pcb_tool_poly_adjust_attached_objects(rnd_hidlib_t *hl)
{
	pcb_attached_line_t *line = &pcb_crosshair.AttachedLine;

	if (rnd_gui->control_is_pressed(rnd_gui)) {
		line->draw = rnd_false;
		return;
	}
	else
		line->draw = rnd_true;
	if (conf_core.editor.all_direction_lines) {
		line->Point2.X = pcb_crosshair.X;
		line->Point2.Y = pcb_crosshair.Y;
		return;
	}

	pcb_line_45(line);
}

void pcb_tool_poly_draw_attached(rnd_hidlib_t *hl)
{
	/* draw only if starting point is set */
	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST)
		rnd_render->draw_line(pcb_crosshair.GC,
									 pcb_crosshair.AttachedLine.Point1.X,
									 pcb_crosshair.AttachedLine.Point1.Y, pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y);

	/* draw attached polygon only if in polygon or polygon_hole tool */
	if (pcb_crosshair.AttachedPolygon.PointN > 1) {
		pcb_xordraw_poly(&pcb_crosshair.AttachedPolygon, 0, 0, 1);
	}
}

rnd_bool pcb_tool_poly_undo_act(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedPolygon.PointN) {
		pcb_polygon_go_to_prev_point();
		return rnd_false;
	}
	return rnd_true;
}

rnd_bool pcb_tool_poly_redo_act(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedPolygon.PointN) {
		pcb_polygon_go_to_next_point();
		return rnd_false;
	}
	else
		return rnd_true;
}

void pcb_tool_poly_escape(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST)
		pcb_tool_select_by_name(hl, "arrow");
	else
		pcb_tool_select_by_name(hl, "poly");
}

/* XPM */
static const char *poly_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c black",
". c #6EA5D7",
"o c None",
/* pixels */
"ooooooooooo.ooooooooo",
"oooooooooo..ooooooooo",
"ooooooooo...ooooooooo",
"oooooooo....ooooooooo",
"ooooooo.....ooooooooo",
"oooooo.......oooooooo",
"ooooo.........ooooooo",
"oooo...........oooooo",
"oooo............ooooo",
"oooo.............oooo",
"oooo..............ooo",
"ooooooooooooooooooooo",
"o    ooo   oo oo ooo ",
"o ooo o ooo o oo ooo ",
"o ooo o ooo o ooo o o",
"o    oo ooo o oooo oo",
"o ooooo ooo o oooo oo",
"o ooooo ooo o oooo oo",
"o ooooo ooo o oooo oo",
"o oooooo   oo    o oo",
"ooooooooooooooooooooo"
};

pcb_tool_t pcb_tool_poly = {
	"poly", NULL, NULL, 100, poly_icon, PCB_TOOL_CURSOR_NAMED("up_arrow"), 0,
	NULL,
	pcb_tool_poly_uninit,
	pcb_tool_poly_notify_mode,
	NULL,
	pcb_tool_poly_adjust_attached_objects,
	pcb_tool_poly_draw_attached,
	pcb_tool_poly_undo_act,
	pcb_tool_poly_redo_act,
	pcb_tool_poly_escape,
	
	0
};
