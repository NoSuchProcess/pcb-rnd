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
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "obj_line.h"
#include "rats.h"
#include "search.h"
#include "tool.h"
#include "undo.h"

#include "obj_line_draw.h"
#include "obj_pinvia_draw.h"
#include "obj_rat_draw.h"


void pcb_tool_line_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	
	/* do update of position */
	pcb_notify_line();
	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_THIRD)
		return;
	/* Remove anchor if clicking on start point;
	 * this means we can't paint 0 length lines
	 * which could be used for square SMD pads.
	 * Instead use a very small delta, or change
	 * the file after saving.
	 */
	if (pcb_crosshair.X == pcb_crosshair.AttachedLine.Point1.X && pcb_crosshair.Y == pcb_crosshair.AttachedLine.Point1.Y) {
		pcb_crosshair_set_mode(PCB_MODE_LINE);
		return;
	}

	if (PCB->RatDraw) {
		pcb_rat_t *line;
		if ((line = pcb_rat_add_net())) {
			pcb_added_lines++;
			pcb_undo_add_obj_to_create(PCB_TYPE_RATLINE, line, line, line);
			pcb_undo_inc_serial();
			pcb_rat_invalidate_draw(line);
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
			pcb_draw();
		}
		return;
	}
	else if(pcb_crosshair.Route.size > 0)
	{
		pcb_pin_t *via = NULL;

		/* place a via if vias are visible, the layer is
			 in a new group since the last line and there
			 isn't a pin already here */
		if (PCB->ViaOn 
				&& pcb_layer_get_group_(CURRENT) != pcb_layer_get_group_(lastLayer)
				&& pcb_search_obj_by_location(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
																			pcb_crosshair.AttachedLine.Point1.X,
																			pcb_crosshair.AttachedLine.Point1.Y,
																			conf_core.design.via_thickness / 2) ==
																				PCB_TYPE_NONE
				&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER)
				&& (pcb_layer_flags_(lastLayer) & PCB_LYT_COPPER)
				&& (via =	pcb_via_new(PCB->Data,
															pcb_crosshair.AttachedLine.Point1.X,
															pcb_crosshair.AttachedLine.Point1.Y,
															conf_core.design.via_thickness,
															2 * conf_core.design.clearance, 0, 
															conf_core.design.via_drilling_hole, NULL, 
															pcb_no_flags())) != NULL) {
					pcb_obj_add_attribs(via, PCB->pen_attr);
					pcb_undo_add_obj_to_create(PCB_TYPE_VIA, via, via, via);
		}

		/* Add the route to the design */
		pcb_route_apply(&pcb_crosshair.Route);

		/* move to new start point */
		pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.Route.end_point.X;
		pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.Route.end_point.Y;
		pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.Route.end_point.X;
		pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.Route.end_point.Y;

		if (conf_core.editor.swap_start_direction) 
			conf_setf(CFR_DESIGN,"editor/line_refraction", -1, "%d",conf_core.editor.line_refraction ^ 3);

		if (conf_core.editor.orthogonal_moves) {
			/* set the mark to the new starting point so ortho works as expected and we can draw a perpendicular line from here */
			pcb_marked.X = pcb_crosshair.Route.end_point.X;
			pcb_marked.Y = pcb_crosshair.Route.end_point.Y;
		}

		if(via)
			pcb_via_invalidate_draw(via);

		pcb_draw();
		pcb_undo_inc_serial();
		lastLayer = CURRENT;
	}
	else
		/* create line if both ends are determined && length != 0 */
	{
		pcb_line_t *line;
		int maybe_found_flag;

		if (conf_core.editor.line_refraction
				&& pcb_crosshair.AttachedLine.Point1.X ==
				pcb_crosshair.AttachedLine.Point2.X
				&& pcb_crosshair.AttachedLine.Point1.Y ==
				pcb_crosshair.AttachedLine.Point2.Y
				&& (pcb_crosshair.AttachedLine.Point2.X != Note.X || pcb_crosshair.AttachedLine.Point2.Y != Note.Y)) {
			/* We will only need to paint the second line segment.
				 Since we only check for vias on the first segment,
				 swap them so the non-empty segment is the first segment. */
			pcb_crosshair.AttachedLine.Point2.X = Note.X;
			pcb_crosshair.AttachedLine.Point2.Y = Note.Y;
		}

		if (conf_core.editor.auto_drc
				&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER))
			maybe_found_flag = PCB_FLAG_FOUND;
		else
			maybe_found_flag = 0;

		if ((pcb_crosshair.AttachedLine.Point1.X !=
				 pcb_crosshair.AttachedLine.Point2.X || pcb_crosshair.AttachedLine.Point1.Y != pcb_crosshair.AttachedLine.Point2.Y)
				&& (line =
						pcb_line_new_merge(CURRENT,
																	 pcb_crosshair.AttachedLine.Point1.X,
																	 pcb_crosshair.AttachedLine.Point1.Y,
																	 pcb_crosshair.AttachedLine.Point2.X,
																	 pcb_crosshair.AttachedLine.Point2.Y,
																	 conf_core.design.line_thickness,
																	 2 * conf_core.design.clearance,
																	 pcb_flag_make(maybe_found_flag |
																						 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
			pcb_pin_t *via;

			pcb_added_lines++;
			pcb_obj_add_attribs(line, PCB->pen_attr);
			pcb_undo_add_obj_to_create(PCB_TYPE_LINE, CURRENT, line, line);
			pcb_line_invalidate_draw(CURRENT, line);
			/* place a via if vias are visible, the layer is
				 in a new group since the last line and there
				 isn't a pin already here */
			if (PCB->ViaOn 
					&& pcb_layer_get_group_(CURRENT) != pcb_layer_get_group_(lastLayer) 
					&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER)
					&& (pcb_layer_flags_(lastLayer) & PCB_LYT_COPPER)
					&& pcb_search_obj_by_location(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
																 pcb_crosshair.AttachedLine.Point1.X,
																 pcb_crosshair.AttachedLine.Point1.Y,
																 conf_core.design.via_thickness / 2) == PCB_TYPE_NONE
					&& (via =
							pcb_via_new(PCB->Data,
													 pcb_crosshair.AttachedLine.Point1.X,
													 pcb_crosshair.AttachedLine.Point1.Y,
													 conf_core.design.via_thickness,
													 2 * conf_core.design.clearance, 0, conf_core.design.via_drilling_hole, NULL, pcb_no_flags())) != NULL) {
				pcb_obj_add_attribs(via, PCB->pen_attr);
				pcb_undo_add_obj_to_create(PCB_TYPE_VIA, via, via, via);
				pcb_via_invalidate_draw(via);
			}
			/* copy the coordinates */
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
			pcb_undo_inc_serial();
			lastLayer = CURRENT;
		}
		if (conf_core.editor.line_refraction && (Note.X != pcb_crosshair.AttachedLine.Point2.X || Note.Y != pcb_crosshair.AttachedLine.Point2.Y)
				&& (line =
						pcb_line_new_merge(CURRENT,
																	 pcb_crosshair.AttachedLine.Point2.X,
																	 pcb_crosshair.AttachedLine.Point2.Y,
																	 Note.X, Note.Y,
																	 conf_core.design.line_thickness,
																	 2 * conf_core.design.clearance,
																	 pcb_flag_make((conf_core.editor.auto_drc ? PCB_FLAG_FOUND : 0) |
																						 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
			pcb_added_lines++;
			pcb_obj_add_attribs(line, PCB->pen_attr);
			pcb_undo_add_obj_to_create(PCB_TYPE_LINE, CURRENT, line, line);
			pcb_undo_inc_serial();
			pcb_line_invalidate_draw(CURRENT, line);
			/* move to new start point */
			pcb_crosshair.AttachedLine.Point1.X = Note.X;
			pcb_crosshair.AttachedLine.Point1.Y = Note.Y;
			pcb_crosshair.AttachedLine.Point2.X = Note.X;
			pcb_crosshair.AttachedLine.Point2.Y = Note.Y;


			if (conf_core.editor.swap_start_direction) {
				conf_setf(CFR_DESIGN,"editor/line_refraction", -1, "%d",conf_core.editor.line_refraction ^ 3);
			}
		}
		if (conf_core.editor.orthogonal_moves) {
			/* set the mark to the new starting point so ortho works as expected and we can draw a perpendicular line from here */
			pcb_marked.X = Note.X;
			pcb_marked.Y = Note.Y;
		}
		pcb_draw();
	}
}

void pcb_tool_line_adjust_attached_objects(void)
{
	pcb_line_adjust_attached();
}

pcb_tool_t pcb_tool_line = {
	"line", NULL, 100,
	pcb_tool_line_notify_mode,
	pcb_tool_line_adjust_attached_objects
};
