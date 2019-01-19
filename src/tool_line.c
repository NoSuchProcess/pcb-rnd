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
#include "data.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "find.h"
#include "obj_line.h"
#include "obj_subc.h"
#include "rats.h"
#include "search.h"
#include "tool.h"
#include "undo.h"

#include "obj_line_draw.h"
#include "obj_pstk_draw.h"
#include "obj_rat_draw.h"

TODO("pstk: remove this when via is removed and the padstack is created from style directly")
#include "src_plugins/lib_compat_help/pstk_compat.h"

static pcb_layer_t *last_layer;


void pcb_tool_line_init(void)
{
	pcb_notify_crosshair_change(pcb_false);
	if (pcb_tool_prev_id == PCB_MODE_ARC && pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST) {
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
		pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.AttachedBox.Point1.X;
		pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.AttachedBox.Point1.Y;
		pcb_tool_adjust_attached_objects();
	}
	else {
		if (conf_core.editor.auto_drc) {
			if (pcb_data_clear_flag(PCB->Data, PCB_FLAG_FOUND, 1, 1) > 0) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
		}
	}
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_tool_line_uninit(void)
{
	pcb_notify_crosshair_change(pcb_false);
	pcb_added_lines = 0;
	pcb_route_reset(&pcb_crosshair.Route);
	if (pcb_tool_next_id != PCB_MODE_ARC) {
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
		pcb_crosshair_set_local_ref(0, 0, pcb_false);
	}
	pcb_notify_crosshair_change(pcb_true);
}

/* creates points of a line (when clicked) */
static void notify_line(void)
{
	int type = PCB_OBJ_VOID;
	void *ptr1, *ptr2, *ptr3;

	if (!pcb_marked.status || conf_core.editor.local_ref)
		pcb_crosshair_set_local_ref(pcb_crosshair.X, pcb_crosshair.Y, pcb_true);
	switch (pcb_crosshair.AttachedLine.State) {
	case PCB_CH_STATE_FIRST:						/* first point */
TODO("subc: this should work on heavy terminals too!")
		if (PCB->RatDraw && pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr1, &ptr1) == PCB_OBJ_VOID) {
			pcb_gui->beep();
			break;
		}
		if (conf_core.editor.auto_drc) {
			pcb_find_t fctx;
			type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);
			memset(&fctx, 0, sizeof(fctx));
			fctx.flag_set = PCB_FLAG_FOUND;
			fctx.flag_chg_undoable = 1;
			pcb_find_from_xy(&fctx, PCB->Data, pcb_crosshair.X, pcb_crosshair.Y);
			pcb_find_free(&fctx);
		}
		if (type == PCB_OBJ_PSTK) {
			pcb_pstk_t *pstk = (pcb_pstk_t *)ptr2;
			pcb_crosshair.AttachedLine.Point1.X = pstk->x;
			pcb_crosshair.AttachedLine.Point1.Y = pstk->y;
		}
		else {
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.Y;
		}
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:
		/* fall through to third state too */
		last_layer = CURRENT;
	default:											/* all following points */
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_THIRD;
		break;
	}
}

void pcb_tool_line_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	
	/* do update of position */
	notify_line();
	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_THIRD)
		return;
	/* Remove anchor if clicking on start point;
	 * this means we can't paint 0 length lines
	 * which could be used for square SMD pads.
	 * Instead use a very small delta, or change
	 * the file after saving.
	 */
	if (pcb_crosshair.X == pcb_crosshair.AttachedLine.Point1.X && pcb_crosshair.Y == pcb_crosshair.AttachedLine.Point1.Y) {
		pcb_tool_select_by_id(PCB_MODE_LINE);
		return;
	}

	if (PCB->RatDraw) {
		pcb_rat_t *line;
		if ((line = pcb_rat_add_net())) {
			pcb_added_lines++;
			pcb_undo_add_obj_to_create(PCB_OBJ_RAT, line, line, line);
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
		pcb_pstk_t *ps = NULL;

		/* place a via if vias are visible, the layer is
			 in a new group since the last line and there
			 isn't a pin already here */
TODO("pstk #21: do not work in comp mode, use a pstk proto - scconfig also has TODO #21, fix it there too")
		if (conf_core.editor.auto_via && PCB->pstk_on
				&& pcb_layer_get_group_(CURRENT) != pcb_layer_get_group_(last_layer)
				&& pcb_search_obj_by_location(PCB_OBJ_CLASS_PIN, &ptr1, &ptr2, &ptr3,
																			pcb_crosshair.AttachedLine.Point1.X,
																			pcb_crosshair.AttachedLine.Point1.Y,
																			conf_core.design.via_thickness / 2) ==
																				PCB_OBJ_VOID
				&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER)
				&& (pcb_layer_flags_(last_layer) & PCB_LYT_COPPER)
				&& (!PCB->is_footprint)
				&& ((ps = pcb_pstk_new_compat_via(PCB->Data, -1,
															pcb_crosshair.AttachedLine.Point1.X,
															pcb_crosshair.AttachedLine.Point1.Y,
				conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance,
			0, PCB_PSTK_COMPAT_ROUND, pcb_true)) != NULL)) {
					pcb_obj_add_attribs(ps, PCB->pen_attr);
					pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, ps, ps, ps);
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

		if (ps)
			pcb_pstk_invalidate_draw(ps);

		pcb_draw();
		pcb_undo_inc_serial();
		last_layer = CURRENT;
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
				&& (pcb_crosshair.AttachedLine.Point2.X != pcb_tool_note.X || pcb_crosshair.AttachedLine.Point2.Y != pcb_tool_note.Y)) {
			/* We will only need to paint the second line segment.
				 Since we only check for vias on the first segment,
				 swap them so the non-empty segment is the first segment. */
			pcb_crosshair.AttachedLine.Point2.X = pcb_tool_note.X;
			pcb_crosshair.AttachedLine.Point2.Y = pcb_tool_note.Y;
		}

		if (conf_core.editor.auto_drc
				&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER))
			maybe_found_flag = PCB_FLAG_FOUND;
		else
			maybe_found_flag = 0;

		if ((pcb_crosshair.AttachedLine.Point1.X !=
				 pcb_crosshair.AttachedLine.Point2.X || pcb_crosshair.AttachedLine.Point1.Y != pcb_crosshair.AttachedLine.Point2.Y)
				&& (line =
						pcb_line_new_merge(pcb_loose_subc_layer(PCB, CURRENT, pcb_true),
																	 pcb_crosshair.AttachedLine.Point1.X,
																	 pcb_crosshair.AttachedLine.Point1.Y,
																	 pcb_crosshair.AttachedLine.Point2.X,
																	 pcb_crosshair.AttachedLine.Point2.Y,
																	 conf_core.design.line_thickness,
																	 2 * conf_core.design.clearance,
																	 pcb_flag_make(maybe_found_flag |
																						 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
			pcb_pstk_t *ps = NULL;

			pcb_added_lines++;
			pcb_obj_add_attribs(line, PCB->pen_attr);
			pcb_undo_add_obj_to_create(PCB_OBJ_LINE, CURRENT, line, line);
			pcb_line_invalidate_draw(CURRENT, line);
			/* place a via if vias are visible, the layer is
				 in a new group since the last line and there
				 isn't a pin already here */
TODO("pstk #21: do not work in comp mode, use a pstk proto - scconfig also has TODO #21, fix it there too")
			if (PCB->pstk_on
					&& pcb_layer_get_group_(CURRENT) != pcb_layer_get_group_(last_layer) 
					&& (pcb_layer_flags_(CURRENT) & PCB_LYT_COPPER)
					&& (pcb_layer_flags_(last_layer) & PCB_LYT_COPPER)
					&& (!PCB->is_footprint)
					&& pcb_search_obj_by_location(PCB_OBJ_CLASS_PIN, &ptr1, &ptr2, &ptr3,
																 pcb_crosshair.AttachedLine.Point1.X,
																 pcb_crosshair.AttachedLine.Point1.Y,
																 conf_core.design.via_thickness / 2) == PCB_OBJ_VOID
				&& ((ps = pcb_pstk_new_compat_via(PCB->Data, -1,
															pcb_crosshair.AttachedLine.Point1.X,
															pcb_crosshair.AttachedLine.Point1.Y,
															conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance,
															0, PCB_PSTK_COMPAT_ROUND, pcb_true)) != NULL)) {
				pcb_obj_add_attribs(ps, PCB->pen_attr);
				pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, ps, ps, ps);
				pcb_pstk_invalidate_draw(ps);
			}
			/* copy the coordinates */
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
			pcb_undo_inc_serial();
			last_layer = CURRENT;
		}
		if (conf_core.editor.line_refraction && (pcb_tool_note.X != pcb_crosshair.AttachedLine.Point2.X || pcb_tool_note.Y != pcb_crosshair.AttachedLine.Point2.Y)
				&& (line =
						pcb_line_new_merge(pcb_loose_subc_layer(PCB, CURRENT, pcb_true),
																	 pcb_crosshair.AttachedLine.Point2.X,
																	 pcb_crosshair.AttachedLine.Point2.Y,
																	 pcb_tool_note.X, pcb_tool_note.Y,
																	 conf_core.design.line_thickness,
																	 2 * conf_core.design.clearance,
																	 pcb_flag_make((conf_core.editor.auto_drc ? PCB_FLAG_FOUND : 0) |
																						 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
			pcb_added_lines++;
			pcb_obj_add_attribs(line, PCB->pen_attr);
			pcb_undo_add_obj_to_create(PCB_OBJ_LINE, CURRENT, line, line);
			pcb_undo_inc_serial();
			pcb_line_invalidate_draw(CURRENT, line);
			/* move to new start point */
			pcb_crosshair.AttachedLine.Point1.X = pcb_tool_note.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_tool_note.Y;
			pcb_crosshair.AttachedLine.Point2.X = pcb_tool_note.X;
			pcb_crosshair.AttachedLine.Point2.Y = pcb_tool_note.Y;


			if (conf_core.editor.swap_start_direction) {
				conf_setf(CFR_DESIGN,"editor/line_refraction", -1, "%d",conf_core.editor.line_refraction ^ 3);
			}
		}
		if (conf_core.editor.orthogonal_moves) {
			/* set the mark to the new starting point so ortho works as expected and we can draw a perpendicular line from here */
			pcb_marked.X = pcb_tool_note.X;
			pcb_marked.Y = pcb_tool_note.Y;
		}
		pcb_draw();
	}
}

void pcb_tool_line_adjust_attached_objects(void)
{
	/* don't draw outline when ctrl key is pressed */
	if (pcb_gui->control_is_pressed()) {
		pcb_crosshair.AttachedLine.draw = pcb_false;
	}
	else {
		pcb_crosshair.AttachedLine.draw = pcb_true;
		pcb_line_adjust_attached();
	}
}

void pcb_tool_line_draw_attached(void)
{
	if (PCB->RatDraw) {
		/* draw only if starting point exists and the line has length */
		if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST && pcb_crosshair.AttachedLine.draw) 
			pcb_draw_wireframe_line(pcb_crosshair.GC,
				pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y,
				pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y,
				10, 0);
	}
	else if (pcb_crosshair.Route.size > 0) {
		pcb_route_draw(&pcb_crosshair.Route,pcb_crosshair.GC);
		if (conf_core.editor.show_drc)
			pcb_route_draw_drc(&pcb_crosshair.Route,pcb_crosshair.GC);
		pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
	}
	else {
		/* Draw a circle (0 length line) to show where the line will be placed */
		if (CURRENT)
			pcb_gui->set_color(pcb_crosshair.GC, &CURRENT->meta.real.color);

		pcb_draw_wireframe_line(pcb_crosshair.GC,
			pcb_crosshair.X, pcb_crosshair.Y,
			pcb_crosshair.X, pcb_crosshair.Y, 
			conf_core.design.line_thickness,0 );

		if (conf_core.editor.show_drc) {
			pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.cross);
			pcb_draw_wireframe_line(pcb_crosshair.GC,
				pcb_crosshair.X, pcb_crosshair.Y,
				pcb_crosshair.X, pcb_crosshair.Y, 
				conf_core.design.line_thickness + (2 * conf_core.design.bloat), 0);
			pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
		}
	}
}

pcb_bool pcb_tool_line_undo_act(void)
{
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND) {
		if (conf_core.editor.auto_drc)
			pcb_undo(pcb_true);	  /* undo the connection find */
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
		pcb_route_reset(&pcb_crosshair.Route);
		pcb_crosshair_set_local_ref(0, 0, pcb_false);
		return pcb_false;
	}
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_THIRD) {
		int type;
		void *ptr1, *ptr3, *ptrtmp;
		pcb_line_t *ptr2;
		ptrtmp = &pcb_crosshair.AttachedLine; /* a workaround for the line undo bug */
		/* this search is guaranteed to succeed */
		pcb_search_obj_by_location(PCB_OBJ_LINE | PCB_OBJ_RAT, &ptr1,
			&ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, 0);
		ptr2 = (pcb_line_t *) ptrtmp;

		/* save both ends of line */
		pcb_crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
		pcb_crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;
		if ((type = pcb_undo(pcb_true)) == 0)
			pcb_board_set_changed_flag(pcb_true);
		/* check that the undo was of the right type */
		if ((type & PCB_UNDO_CREATE) == 0) {
			/* wrong undo type, restore anchor points */
			pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.AttachedLine.Point1.X;
			pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.AttachedLine.Point1.Y;
			return pcb_false;
		}
		/* move to new anchor */
		pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
		pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
		/* check if an intermediate point was removed */
		if (type & PCB_UNDO_REMOVE) {
			/* this search should find the restored line */
			pcb_search_obj_by_location(PCB_OBJ_LINE | PCB_OBJ_RAT, &ptr1, &ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y, 0);
			ptr2 = (pcb_line_t *) ptrtmp;
			if (conf_core.editor.auto_drc) {
				/* undo loses PCB_FLAG_FOUND */
				PCB_FLAG_SET(PCB_FLAG_FOUND, ptr2);
				pcb_line_invalidate_draw(CURRENT, ptr2);
			}
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
		}
		pcb_crosshair_grid_fit(pcb_crosshair.X, pcb_crosshair.Y);
		pcb_tool_adjust_attached_objects();
		if (--pcb_added_lines == 0) {
			pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
			last_layer = CURRENT;
		}
		else {
			/* this search is guaranteed to succeed too */
			pcb_search_obj_by_location(PCB_OBJ_LINE | PCB_OBJ_RAT, &ptr1, &ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, 0);
			ptr2 = (pcb_line_t *) ptrtmp;
			last_layer = (pcb_layer_t *) ptr1;
		}
		return pcb_false;
	}
	return pcb_true;
}

pcb_bool pcb_tool_line_redo_act(void)
{
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND)
		return pcb_false;
	if (pcb_redo(pcb_true)) {
		pcb_board_set_changed_flag(pcb_true);
		if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST) {
			pcb_line_t *line = linelist_last(&CURRENT->Line);
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = line->Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = line->Point2.Y;
			pcb_added_lines++;
		}
	}
	return pcb_false;
}

pcb_tool_t pcb_tool_line = {
	"line", NULL, 100,
	pcb_tool_line_init,
	pcb_tool_line_uninit,
	pcb_tool_line_notify_mode,
	NULL,
	pcb_tool_line_adjust_attached_objects,
	pcb_tool_line_draw_attached,
	pcb_tool_line_undo_act,
	pcb_tool_line_redo_act,
	
	pcb_true
};
