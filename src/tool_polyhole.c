/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "conf_core.h"

#include "action_helper.h"
#include "compat_nls.h"
#include "crosshair.h"
#include "hid_actions.h"
#include "search.h"
#include "tool.h"


void pcb_tool_polyhole_notify_mode(void)
{
	switch (pcb_crosshair.AttachedObject.State) {
		/* first notify, lookup object */
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedObject.Type =
			pcb_search_screen(Note.X, Note.Y, PCB_TYPE_POLY,
									 &pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);

		if (pcb_crosshair.AttachedObject.Type == PCB_TYPE_NONE) {
			pcb_message(PCB_MSG_WARNING, "The first point of a polygon hole must be on a polygon.\n");
			break; /* don't start doing anything if clicket out of polys */
		}

		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_poly_t *)
									pcb_crosshair.AttachedObject.Ptr2)) {
			pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
			pcb_crosshair.AttachedObject.Type = PCB_TYPE_NONE;
			break;
		}
		else
			pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;
	/* fall thru: first click is also the first point of the poly hole */

		/* second notify, insert new point into object */
	case PCB_CH_STATE_SECOND:
		{
			pcb_point_t *points = pcb_crosshair.AttachedPolygon.Points;
			pcb_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

			/* do update of position; use the 'PCB_MODE_LINE' mechanism */
			pcb_notify_line();

			if (conf_core.editor.orthogonal_moves) {
				/* set the mark to the new starting point so ortho works */
				pcb_marked.X = Note.X;
				pcb_marked.Y = Note.Y;
			}

			/* check if this is the last point of a polygon */
			if (n >= 3 && points[0].X == pcb_crosshair.AttachedLine.Point2.X && points[0].Y == pcb_crosshair.AttachedLine.Point2.Y) {
				pcb_hid_actionl("Polygon", "CloseHole", NULL);
				break;
			}

			/* Someone clicking twice on the same point ('doubleclick'): close polygon hole */
			if (n >= 3 && points[n - 1].X == pcb_crosshair.AttachedLine.Point2.X && points[n - 1].Y == pcb_crosshair.AttachedLine.Point2.Y) {
				pcb_hid_actionl("Polygon", "CloseHole", NULL);
				break;
			}

			/* create new point if it's the first one or if it's
			 * different to the last one
			 */
			if (!n || points[n - 1].X != pcb_crosshair.AttachedLine.Point2.X || points[n - 1].Y != pcb_crosshair.AttachedLine.Point2.Y) {
				pcb_poly_point_new(&pcb_crosshair.AttachedPolygon,
																pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y);

				/* copy the coordinates */
				pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
				pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
			}
			break;
		}
	}
}

void pcb_tool_polyhole_adjust_attached_objects(void)
{
	pcb_line_adjust_attached();
}

void pcb_tool_polyhole_draw_attached(void)
{
	/* draw only if starting point is set */
	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST)
		pcb_gui->draw_line(pcb_crosshair.GC,
									 pcb_crosshair.AttachedLine.Point1.X,
									 pcb_crosshair.AttachedLine.Point1.Y, pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y);

	/* draw attached polygon only if in PCB_MODE_POLYGON or PCB_MODE_POLYGON_HOLE */
	if (pcb_crosshair.AttachedPolygon.PointN > 1) {
		XORPolygon(&pcb_crosshair.AttachedPolygon, 0, 0, 1);
	}
}

pcb_bool pcb_tool_polyhole_undo_act(void)
{
	if (pcb_crosshair.AttachedPolygon.PointN) {
		pcb_polygon_go_to_prev_point();
		return pcb_false;
	}
	return pcb_true;
}

pcb_bool pcb_tool_polyhole_redo_act(void)
{
	if (pcb_crosshair.AttachedPolygon.PointN)
		return pcb_false;
	else
		return pcb_true;
}

pcb_tool_t pcb_tool_polyhole = {
	"polyhole", NULL, 100,
	pcb_tool_polyhole_notify_mode,
	pcb_tool_polyhole_adjust_attached_objects,
	pcb_tool_polyhole_draw_attached,
	pcb_tool_polyhole_undo_act,
	pcb_tool_polyhole_redo_act,
	NULL
};
