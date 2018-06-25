/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include "data.h"
#include "action_helper.h"
#include "tool.h"
#include "grid.h"
#include "error.h"
#include "undo.h"
#include "funchash_core.h"
#include "change.h"

#include "draw.h"
#include "search.h"
#include "find.h"
#include "stub_stroke.h"
#include "actions.h"
#include "hid_init.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "event.h"
#include "layer.h"
#include "layer_ui.h"
#include "layer_grp.h"
#include "layer_vis.h"
#include "attrib.h"
#include "hid_attrib.h"
#include "operation.h"
#include "obj_subc_op.h"
#include "tool.h"

#define CLONE_TYPES PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY

/* --------------------------------------------------------------------------- */
/* Toggle actions are kept for compatibility; new code should use the conf system instead */
static const char pcb_acts_Display[] =
	"Display(SubcID, template)\n"
	"Display(Grid|Redraw|Pinout|PinOrPadName)\n"
	"Display(CycleClip|CycleCrosshair|ToggleAllDirections|ToggleStartDirection)\n"
	"Display(ToggleGrid|ToggleRubberBandMode|ToggleUniqueNames)\n"
	"Display(ToggleName|ToggleClearLine|ToggleFullPoly|ToggleSnapPin)\n"
	"Display(ToggleSnapOffGridLine|ToggleHighlightOnPoint|ToggleCheckPlanes)\n"
	"Display(ToggleThindraw|ToggleThindrawPoly|ToggleOrthoMove|ToggleLocalRef)\n"
	"Display(ToggleLiveRoute|ToggleShowDRC|ToggleAutoDRC|LockNames|OnlyNames)";

static const char pcb_acth_Display[] = "Several display-related actions.";

/* %start-doc actions Display

@table @code

@item SubcID
@item Description
@item Value
Specify the subcircuit ID template to be printed on the subcircuit layer

@item Redraw
Redraw the whole board.

@item ToggleAllDirections
When clear, lines can be drawn at any angle.  When set, lines are
restricted to multiples of 45 degrees and requested lines may be
broken up according to the clip setting.

@item CycleClip
Changes the way lines are restricted to 45 degree increments.  The
various settings are: straight only, orthogonal then angled, and angled
then orthogonal.  If AllDirections is set, this action disables it.

@item CycleCrosshair
Changes crosshair drawing.  pcb_crosshair may accept form of 4-ray,
8-ray and 12-ray cross.

@item ToggleRubberBandMode
If set, moving an object moves all the lines attached to it too.

@item ToggleStartDirection
If set, each time you set a point in a line, the Clip toggles between
orth-angle and angle-ortho.

@item ToggleUniqueNames
If set, you will not be permitted to change the name of an element to
match that of another element.

@item ToggleSnapPin
If set, pin centers and pad end points are treated as additional grid
points that the cursor can snap to.

@item ToggleSnapOffGridLine
If set, snap at some sensible point along a line.

@item ToggleHighlightOnPoint
If set, highlights lines and arcs when the crosshair is on one of their
two (end) points.

@item ToggleLocalRef
If set, the mark is automatically set to the beginning of any move, so
you can see the relative distance you've moved.

@item ToggleThindraw
If set, objects on the screen are drawn as outlines (lines are drawn
as center-lines).  This lets you see line endpoints hidden under pins,
for example.

@item ToggleThindrawPoly
If set, polygons on the screen are drawn as outlines.

@item ToggleShowDRC
If set, pending objects (i.e. lines you're in the process of drawing)
will be drawn with an outline showing how far away from other copper
you need to be.

@item ToggleLiveRoute
If set, the progress of the autorouter will be visible on the screen.

@item ToggleAutoDRC
If set, you will not be permitted to make connections which violate
the current DRC and netlist settings.

@item ToggleCheckPlanes
If set, lines and arcs aren't drawn, which usually leaves just the
polygons.  If you also disable all but the layer you're interested in,
this allows you to check for isolated regions.

@item ToggleOrthoMove
If set, the crosshair is only allowed to move orthogonally from its
previous position.  I.e. you can move a subcircuit or line up, down,
left, or right, but not up+left or down+right.

@item ToggleName
Selects whether the pinouts show the pin names or the pin numbers.

@item ToggleLockNames
If set, text will ignore left mouse clicks and actions that work on
objects under the mouse. You can still select text with a lasso (left
mouse drag) and perform actions on the selection.

@item ToggleOnlyNames
If set, only text will be sensitive for mouse clicks and actions that
work on objects under the mouse. You can still select other objects
with a lasso (left mouse drag) and perform actions on the selection.

@item ToggleClearLine
When set, the clear-line flag causes new lines and arcs to have their
``clear polygons'' flag set, so they won't be electrically connected
to any polygons they overlap.

@item ToggleFullPoly
When set, the full-poly flag causes new polygons to have their
``full polygon'' flag set, so all parts of them will be displayed
instead of only the biggest one.

@item ToggleGrid
Resets the origin of the current grid to be wherever the crosshair is.
If you provide two numbers after this, the origin is set to that
coordinate.

@item Grid
Toggles whether the grid is displayed or not.

@item Pinout
Causes the pinout of the subcircuit indicated by the cursor to be
displayed, usually in a separate window.

@item PinOrPadName
Toggles whether the names of terminals will be
displayed.  If the cursor is over an subcircuit, all of its terminals
are affected.

@end table

%end-doc */

static enum pcb_crosshair_shape_e CrosshairShapeIncrement(enum pcb_crosshair_shape_e shape)
{
	switch (shape) {
	case pcb_ch_shape_basic:
		shape = pcb_ch_shape_union_jack;
		break;
	case pcb_ch_shape_union_jack:
		shape = pcb_ch_shape_dozen;
		break;
	case pcb_ch_shape_dozen:
		shape = pcb_ch_shape_NUM;
		break;
	case pcb_ch_shape_NUM:
		shape = pcb_ch_shape_basic;
		break;
	}
	return shape;
}

extern pcb_opfunc_t ChgFlagFunctions;
static fgw_error_t pcb_act_Display(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *str_dir = NULL;
	int id;
	int err = 0;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_KEYWORD, Display, id = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Display, str_dir = argv[2].val.str);

	if (id == F_SubcID) { /* change the displayed name of subcircuits */
		if (argc > 1)
			conf_set(CFR_DESIGN, "editor/subc_id", -1, str_dir, POL_OVERWRITE);
		else
			conf_set(CFR_DESIGN, "editor/subc_id", -1, "", POL_OVERWRITE);

		pcb_gui->invalidate_all(); /* doesn't change too often, isn't worth anything more complicated */
		pcb_draw();
		return 0;
	}

	if (!str_dir || !*str_dir) {
		switch (id) {

			/* redraw layout */
		case F_ClearAndRedraw:
		case F_Redraw:
			pcb_redraw();
			break;

			/* toggle line-adjust flag */
		case F_ToggleAllDirections:
			conf_toggle_editor(all_direction_lines);
			pcb_tool_adjust_attached_objects();
			break;

		case F_CycleClip:
			pcb_notify_crosshair_change(pcb_false);
			if (conf_core.editor.all_direction_lines) {
				conf_toggle_editor(all_direction_lines);
				conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",0);
			}
			else {
				conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",(conf_core.editor.line_refraction +1) % 3);
			}
			pcb_tool_adjust_attached_objects();
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_CycleCrosshair:
			pcb_notify_crosshair_change(pcb_false);
			pcb_crosshair.shape = CrosshairShapeIncrement(pcb_crosshair.shape);
			if (pcb_ch_shape_NUM == pcb_crosshair.shape)
				pcb_crosshair.shape = pcb_ch_shape_basic;
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleRubberBandMode:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(rubber_band_mode);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleStartDirection:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(swap_start_direction);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleUniqueNames:
			pcb_message(PCB_MSG_ERROR, "Unique names/refdes is not supported any more - please use the renumber plugin\n");
			break;

		case F_ToggleSnapPin:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(snap_pin);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleSnapOffGridLine:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(snap_offgrid_line);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleHighlightOnPoint:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(highlight_on_point);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleLocalRef:
			conf_toggle_editor(local_ref);
			break;

		case F_ToggleThindraw:
			conf_toggle_editor(thin_draw);
			pcb_redraw();
			break;

		case F_ToggleThindrawPoly:
			conf_toggle_editor(thin_draw_poly);
			pcb_redraw();
			break;

		case F_ToggleLockNames:
			conf_toggle_editor(lock_names);
			conf_set_editor(only_names, 0);
			break;

		case F_ToggleOnlyNames:
			conf_toggle_editor(only_names);
			conf_set_editor(lock_names, 0);
			break;

		case F_ToggleHideNames:
			conf_toggle_editor(hide_names);
			pcb_redraw();
			break;

		case F_ToggleStroke:
			conf_toggle_editor(enable_stroke);
			break;

		case F_ToggleShowDRC:
			conf_toggle_editor(show_drc);
			break;

		case F_ToggleLiveRoute:
			conf_toggle_editor(live_routing);
			break;

		case F_ToggleAutoDRC:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(auto_drc);
			if (conf_core.editor.auto_drc && conf_core.editor.mode == PCB_MODE_LINE) {
				if (pcb_reset_conns(pcb_true)) {
					pcb_undo_inc_serial();
					pcb_draw();
				}
				if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST)
					pcb_lookup_conn(pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, pcb_true, 1, PCB_FLAG_FOUND);
			}
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleCheckPlanes:
			conf_toggle_editor(check_planes);
			pcb_redraw();
			break;

		case F_ToggleOrthoMove:
			conf_toggle_editor(orthogonal_moves);
			break;

		case F_ToggleName:
			conf_toggle_editor(show_number);
			pcb_redraw();
			break;

		case F_ToggleClearLine:
			conf_toggle_editor(clear_line);
			break;

		case F_ToggleFullPoly:
			conf_toggle_editor(full_poly);
			break;

			/* shift grid alignment */
		case F_ToggleGrid:
			{
				pcb_coord_t oldGrid = PCB->Grid;

				PCB->Grid = 1;
				if (pcb_crosshair_move_absolute(pcb_crosshair.X, pcb_crosshair.Y))
					pcb_notify_crosshair_change(pcb_true);	/* first notify was in MoveCrosshairAbs */
				pcb_board_set_grid(oldGrid, pcb_true, pcb_crosshair.X, pcb_crosshair.Y);
				pcb_grid_inval();
			}
			break;

			/* toggle displaying of the grid */
		case F_Grid:
			conf_toggle_editor(draw_grid);
			pcb_redraw();
			break;

			/* display the pinout of a subcircuit */
		case F_Pinout:
			{
				pcb_subc_t *subc;
				void *ptrtmp;
				pcb_coord_t x, y;

				pcb_hid_get_coords(_("Click on a subcircuit"), &x, &y);
				if ((pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID) {
					subc = (pcb_subc_t *) ptrtmp;
					pcb_gui->show_item(subc);
				}
				break;
			}

			/* toggle displaying of terminal names */
		case F_PinOrPadName:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;
				pcb_coord_t x, y;
				pcb_hid_get_coords(_("Click on a subcircuit"), &x, &y);

				/* toggle terminal ID print for subcircuit parts */
				type = pcb_search_screen(x, y, PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART | PCB_OBJ_PSTK | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT, (void **)&ptr1, (void **)&ptr2, (void **)&ptr3);
				if (type) {
					pcb_any_obj_t *obj = ptr2;
					pcb_opctx_t opctx;
					
					switch(type) {
						case PCB_OBJ_SUBC:
							opctx.chgflag.pcb = PCB;
							opctx.chgflag.how = PCB_CHGFLG_TOGGLE;
							opctx.chgflag.flag = PCB_FLAG_TERMNAME;
							pcb_subc_op(PCB->Data, (pcb_subc_t *)obj, &ChgFlagFunctions, &opctx, 0);
							pcb_undo_inc_serial();
							return 0;
							break;
						case PCB_OBJ_LINE:
						case PCB_OBJ_ARC:
						case PCB_OBJ_POLY:
						case PCB_OBJ_TEXT:
						case PCB_OBJ_PSTK:
							pcb_obj_invalidate_label(type, ptr1, ptr2, ptr3);
							break;
						default:
							return 0; /* nothing else can have a displayed name */
					}

					pcb_undo_add_obj_to_flag(ptr2);
					PCB_FLAG_TOGGLE(PCB_FLAG_TERMNAME, obj);
					pcb_board_set_changed_flag(pcb_true);
					pcb_undo_inc_serial();
					pcb_draw();
				}
				break;
			}
		default:
			err = 1;
		}
	}
	else if (str_dir) {
		switch(id) {
		case F_ToggleGrid:
			if (argc > 3) {
				if (fgw_argv_conv(&pcb_fgw, &argv[3], FGW_KEYWORD) != 0) {
					PCB_ACT_FAIL(Display);
					return FGW_ERR_ARG_CONV;
				}
				PCB->GridOffsetX = pcb_get_value(argv[2].val.str, NULL, NULL, NULL);
				PCB->GridOffsetY = pcb_get_value(argv[3].val.str, NULL, NULL, NULL);
				if (conf_core.editor.draw_grid)
					pcb_redraw();
			}
			break;

		default:
			err = 1;
			break;
		}
	}

	if (!err)
		return 0;

	PCB_ACT_FAIL(Display);
}
/* --------------------------------------------------------------------------- */

static const char pcb_acts_Mode[] =
	"Mode(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
	"Mode(Polygon|Rectangle|Remove|Rotate|Text|Thermal|Via)\n" "Mode(Notify|Release|Cancel|Stroke)\n" "Mode(Save|Restore)";

static const char pcb_acth_Mode[] = "Change or use the tool mode.";

/* %start-doc actions Mode

@table @code

@item Arc
@itemx Arrow
@itemx Copy
@itemx InsertPoint
@itemx Line
@itemx Lock
@itemx Move
@itemx None
@itemx PasteBuffer
@itemx Polygon
@itemx Rectangle
@itemx Remove
@itemx Rotate
@itemx Text
@itemx Thermal
@itemx Via
Select the indicated tool.

@item Notify
Called when you press the mouse button, or move the mouse.

@item Release
Called when you release the mouse button.

@item Cancel
Cancels any pending tool activity, allowing you to restart elsewhere.
For example, this allows you to start a new line rather than attach a
line to the previous line.

@item Escape
Similar to Cancel but calling this action a second time will return
to the Arrow tool.

@item Stroke
If your @code{pcb} was built with libstroke, this invokes the stroke
input method.  If not, this will restart a drawing mode if you were
drawing, else it will select objects.

@item Save
Remembers the current tool.

@item Restore
Restores the tool to the last saved tool.

@end table

%end-doc */

static fgw_error_t pcb_act_Mode(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_KEYWORD, Display, ;);

	/* it is okay to use crosshair directly here, the mode command is called from a click when it needs coords */
	pcb_tool_note.X = pcb_crosshair.X;
	pcb_tool_note.Y = pcb_crosshair.Y;
	pcb_notify_crosshair_change(pcb_false);
	switch(fgw_keyword(&argv[1])) {
	case F_Arc:
		pcb_tool_select_by_id(PCB_MODE_ARC);
		break;
	case F_Arrow:
		pcb_tool_select_by_id(PCB_MODE_ARROW);
		break;
	case F_Copy:
		pcb_tool_select_by_id(PCB_MODE_COPY);
		break;
	case F_InsertPoint:
		pcb_tool_select_by_id(PCB_MODE_INSERT_POINT);
		break;
	case F_Line:
		pcb_tool_select_by_id(PCB_MODE_LINE);
		break;
	case F_Lock:
		pcb_tool_select_by_id(PCB_MODE_LOCK);
		break;
	case F_Move:
		pcb_tool_select_by_id(PCB_MODE_MOVE);
		break;
	case F_None:
		pcb_tool_select_by_id(PCB_MODE_NO);
		break;
	case F_Cancel:
		{
			int saved_mode = conf_core.editor.mode;
			pcb_tool_select_by_id(PCB_MODE_NO);
			pcb_tool_select_by_id(saved_mode);
		}
		break;
	case F_Escape:
		{
			switch (conf_core.editor.mode) {
			case PCB_MODE_VIA:
			case PCB_MODE_PASTE_BUFFER:
			case PCB_MODE_TEXT:
			case PCB_MODE_ROTATE:
			case PCB_MODE_REMOVE:
			case PCB_MODE_MOVE:
			case PCB_MODE_COPY:
			case PCB_MODE_INSERT_POINT:
			case PCB_MODE_RUBBERBAND_MOVE:
			case PCB_MODE_THERMAL:
			case PCB_MODE_LOCK:
				pcb_tool_select_by_id(PCB_MODE_NO);
				pcb_tool_select_by_id(PCB_MODE_ARROW);
				pcb_tool_note.Hit = pcb_tool_note.Click = 0; /* if the mouse button is still pressed, don't start selecting a box */
				break;

			case PCB_MODE_LINE:
				if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST) {
					pcb_tool_select_by_id(PCB_MODE_ARROW);
				}
				else {
					pcb_tool_select_by_id(PCB_MODE_NO);
					pcb_tool_select_by_id(PCB_MODE_LINE);
				}
				break;

			case PCB_MODE_RECTANGLE:
				if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_FIRST)
					pcb_tool_select_by_id(PCB_MODE_ARROW);
				else {
					pcb_tool_select_by_id(PCB_MODE_NO);
					pcb_tool_select_by_id(PCB_MODE_RECTANGLE);
				}
				break;

			case PCB_MODE_POLYGON:
				if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST)
					pcb_tool_select_by_id(PCB_MODE_ARROW);
				else {
					pcb_tool_select_by_id(PCB_MODE_NO);
					pcb_tool_select_by_id(PCB_MODE_POLYGON);
				}
				break;

			case PCB_MODE_POLYGON_HOLE:
				if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST)
					pcb_tool_select_by_id(PCB_MODE_ARROW);
				else {
					pcb_tool_select_by_id(PCB_MODE_NO);
					pcb_tool_select_by_id(PCB_MODE_POLYGON_HOLE);
				}
				break;

			case PCB_MODE_ARC:
				if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_FIRST)
					pcb_tool_select_by_id(PCB_MODE_ARROW);
				else {
					pcb_tool_select_by_id(PCB_MODE_NO);
					pcb_tool_select_by_id(PCB_MODE_ARC);
				}
				break;

			case PCB_MODE_ARROW:
				break;

			default:
				break;
			}
		}
		break;

	case F_Notify:
		pcb_notify_mode();
		break;
	case F_PasteBuffer:
		pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
		break;
	case F_Polygon:
		pcb_tool_select_by_id(PCB_MODE_POLYGON);
		break;
	case F_PolygonHole:
		pcb_tool_select_by_id(PCB_MODE_POLYGON_HOLE);
		break;
	case F_Release:
		if ((pcb_mid_stroke) && (conf_core.editor.enable_stroke) && (pcb_stub_stroke_finish() == 0)) {
			/* Ugly hack: returning 1 here will break execution of the
			   action script, so actions after this one could do things
			   that would be executed only after non-recognized gestures */
			pcb_release_mode();
			return 1;
		}
		pcb_release_mode();
		break;
	case F_Remove:
		pcb_tool_select_by_id(PCB_MODE_REMOVE);
		break;
	case F_Rectangle:
		pcb_tool_select_by_id(PCB_MODE_RECTANGLE);
		break;
	case F_Rotate:
		pcb_tool_select_by_id(PCB_MODE_ROTATE);
		break;
	case F_Stroke:
		if (conf_core.editor.enable_stroke) {
			pcb_stub_stroke_start();
			break;
		}
		/* Handle middle mouse button restarts of drawing mode.
		   If not in a drawing mode, middle mouse button will select objects. */
		if (conf_core.editor.mode == PCB_MODE_LINE && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST)
			pcb_tool_select_by_id(PCB_MODE_LINE);
		else if (conf_core.editor.mode == PCB_MODE_ARC && pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST)
			pcb_tool_select_by_id(PCB_MODE_ARC);
		else if (conf_core.editor.mode == PCB_MODE_RECTANGLE && pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST)
			pcb_tool_select_by_id(PCB_MODE_RECTANGLE);
		else if (conf_core.editor.mode == PCB_MODE_POLYGON && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST)
			pcb_tool_select_by_id(PCB_MODE_POLYGON);
		else {
			pcb_tool_save();
			pcb_tool_is_saved = pcb_true;
			pcb_tool_select_by_id(PCB_MODE_ARROW);
			pcb_notify_mode();
		}
		break;
	case F_Text:
		pcb_tool_select_by_id(PCB_MODE_TEXT);
		break;
	case F_Thermal:
		pcb_tool_select_by_id(PCB_MODE_THERMAL);
		break;
	case F_Via:
		pcb_tool_select_by_id(PCB_MODE_VIA);
		break;

	case F_Restore: /* restore the last saved tool */
		pcb_tool_restore();
		break;

	case F_Save: /* save currently selected tool */
		pcb_tool_save();
		break;
	}
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}

/* ---------------------------------------------------------------- */
static const char pcb_acts_CycleDrag[] = "CycleDrag()\n";
static const char pcb_acth_CycleDrag[] = "Cycle through which object is being dragged";

#define close_enough(a, b) ((((a)-(b)) > 0) ? ((a)-(b) < (PCB_SLOP * pcb_pixel_slop)) : ((a)-(b) > -(PCB_SLOP * pcb_pixel_slop)))
static fgw_error_t pcb_act_CycleDrag(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	void *ptr1, *ptr2, *ptr3;
	int over = 0;

	PCB_ACT_IRES(0);
	if (pcb_crosshair.drags == NULL)
		return 0;

	do {
		pcb_crosshair.drags_current++;
		if (pcb_crosshair.drags_current >= pcb_crosshair.drags_len) {
			pcb_crosshair.drags_current = 0;
			over++;
		}

		if (pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, pcb_crosshair.drags[pcb_crosshair.drags_current], PCB_OBJ_LINE) != PCB_OBJ_VOID) {
			/* line has two endpoints, check which one is close to the original x;y */
			pcb_line_t *l = ptr2;
			if (close_enough(pcb_tool_note.X, l->Point1.X) && close_enough(pcb_tool_note.Y, l->Point1.Y)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_LINE_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = &l->Point1;
				goto switched;
			}
			if (close_enough(pcb_tool_note.X, l->Point2.X) && close_enough(pcb_tool_note.Y, l->Point2.Y)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_LINE_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = &l->Point2;
				goto switched;
			}
		}
		else if (pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, pcb_crosshair.drags[pcb_crosshair.drags_current], PCB_OBJ_ARC_POINT) != PCB_OBJ_VOID) {
			pcb_coord_t ex, ey;
			pcb_arc_get_end((pcb_arc_t *)ptr2, 0, &ex, &ey);
			if (close_enough(pcb_tool_note.X, ex) && close_enough(pcb_tool_note.Y, ey)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_ARC_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = pcb_arc_start_ptr;
				goto switched;
			}
			pcb_arc_get_end((pcb_arc_t *)ptr2, 1, &ex, &ey);
			if (close_enough(pcb_tool_note.X, ex) && close_enough(pcb_tool_note.Y, ey)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_ARC_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = pcb_arc_end_ptr;
				goto switched;
			}
		}
		else if (pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, pcb_crosshair.drags[pcb_crosshair.drags_current], PCB_OBJ_ARC) != PCB_OBJ_VOID) {
			pcb_crosshair.AttachedObject.Type = PCB_OBJ_ARC;
			pcb_crosshair.AttachedObject.Ptr1 = ptr1;
			pcb_crosshair.AttachedObject.Ptr2 = ptr2;
			pcb_crosshair.AttachedObject.Ptr3 = ptr3;
			goto switched;
		}

	} while (over <= 1);

	PCB_ACT_IRES(-1);
	return 0;
	switched:;
	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	return 0;
}

#undef close_enough

/* -------------------------------------------------------------------------- */

static const char pcb_acts_Message[] = "message(message)";

static const char pcb_acth_Message[] = "Writes a message to the log window.";

/* %start-doc actions Message

This action displays a message to the log window.  This action is primarily
provided for use by other programs which may interface with PCB.  If
multiple arguments are given, each one is sent to the log window
followed by a newline.

%end-doc */

static fgw_error_t pcb_act_Message(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int i;

	if (argc < 2)
		PCB_ACT_FAIL(Message);

	PCB_ACT_IRES(0);
	for (i = 1; i < argc; i++) {
		PCB_ACT_MAY_CONVARG(i, FGW_STR, Message, ;);
		pcb_message(PCB_MSG_INFO, argv[i].val.str);
		pcb_message(PCB_MSG_INFO, "\n");
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_MarkCrosshair[] = "MarkCrosshair()\n" "MarkCrosshair(Center)";

static const char pcb_acth_MarkCrosshair[] = "Set/Reset the pcb_crosshair mark.";

/* %start-doc actions MarkCrosshair

The ``mark'' is a small X-shaped target on the display which is
treated like a second origin (the normal origin is the upper let
corner of the board).  The GUI will display a second set of
coordinates for this mark, which tells you how far you are from it.

If no argument is given, the mark is toggled - disabled if it was
enabled, or enabled at the current cursor position of disabled.  If
the @code{Center} argument is given, the mark is moved to the current
cursor location.

%end-doc */

static fgw_error_t pcb_act_MarkCrosshair(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id = -2;

	PCB_ACT_IRES(0);
	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, Display, id = fgw_keyword(&argv[1]));

	if (id == -1) { /* invalid */
		PCB_ACT_FAIL(MarkCrosshair);
		return FGW_ERR_ARGV_TYPE;
	}
	else if (id == -2) { /* empty */
		if (pcb_marked.status) {
			pcb_notify_mark_change(pcb_false);
			pcb_marked.status = pcb_false;
			pcb_notify_mark_change(pcb_true);
		}
		else {
			pcb_notify_mark_change(pcb_false);
			pcb_marked.status = pcb_true;
			if (conf_core.editor.marker_snaps) {
				pcb_marked.X = pcb_crosshair.X;
				pcb_marked.Y = pcb_crosshair.Y;
			}
			else
				pcb_hid_get_coords(_("Click on new mark"), &pcb_marked.X, &pcb_marked.Y);
			pcb_notify_mark_change(pcb_true);
		}
	}
	else if (id == F_Center) {
		pcb_notify_mark_change(pcb_false);
		pcb_marked.status = pcb_true;
		if (conf_core.editor.marker_snaps) {
			pcb_marked.X = pcb_crosshair.X;
			pcb_marked.Y = pcb_crosshair.Y;
		}
		else
			pcb_hid_get_coords(_("Click on new mark"), &pcb_marked.X, &pcb_marked.Y);
		pcb_notify_mark_change(pcb_true);
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RouteStyle[] = "RouteStyle(style_id|style_name)";
static const char pcb_acth_RouteStyle[] = "Copies the indicated routing style into the current pen.";
/* %start-doc actions RouteStyle

%end-doc */
static fgw_error_t pcb_act_RouteStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *end;
	const char *str = NULL;
	pcb_route_style_t *rts;
	int number;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_STR, RouteStyle, str = argv[1].val.str);

	number = strtol(str, &end, 10);

	if (*end != '\0') { /* if not an integer, find by name */
		int n;
		number = -1;
		for(n = 0; n < vtroutestyle_len(&PCB->RouteStyle); n++) {
			rts = &PCB->RouteStyle.array[n];
			if (pcb_strcasecmp(rts->name, str) == 0) {
				number = n + 1;
				break;
			}
		}
	}

	if (number > 0 && number <= vtroutestyle_len(&PCB->RouteStyle)) {
		rts = &PCB->RouteStyle.array[number - 1];
		pcb_board_set_line_width(rts->Thick);
		pcb_board_set_via_size(rts->Diameter, pcb_true);
		pcb_board_set_via_drilling_hole(rts->Hole, pcb_true);
		pcb_board_set_clearance(rts->Clearance);
	}
	else {
		PCB_ACT_IRES(-1);
		pcb_message(PCB_MSG_ERROR, "Error: invalid route style name or index\n");
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_CreateMenu[] = "CreateMenu(path)\nCreateMenu(path, action, mnemonic, accel, tooltip, cookie)";
static const char pcb_acth_CreateMenu[] = "Creates a new menu, popup (only path specified) or submenu (at least path and action are specified)";

/* %start-doc actions RouteStyle

%end-doc */

static fgw_error_t pcb_act_CreateMenu(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (pcb_gui == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error: can't create menu, there's no GUI hid loaded\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_CONVARG(1, FGW_STR, CreateMenu, ;);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, CreateMenu, ;);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, CreateMenu, ;);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, CreateMenu, ;);
	PCB_ACT_MAY_CONVARG(5, FGW_STR, CreateMenu, ;);
	PCB_ACT_MAY_CONVARG(6, FGW_STR, CreateMenu, ;);

	if (argc > 1) {
		pcb_menu_prop_t props;
		memset(&props, 0, sizeof(props));
		props.action = (argc > 2) ? argv[2].val.str : NULL;
		props.mnemonic = (argc > 3) ? argv[3].val.str : NULL;
		props.accel = (argc > 4) ? argv[4].val.str : NULL;
		props.tip = (argc > 5) ? argv[5].val.str : NULL;
		props.cookie = (argc > 6) ? argv[6].val.str : NULL;
		pcb_gui->create_menu(argv[1].val.str, &props);
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_FAIL(CreateMenu);
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RemoveMenu[] = "RemoveMenu(path|cookie)";
static const char pcb_acth_RemoveMenu[] = "Recursively removes a new menu, popup (only path specified) or submenu. ";

/* %start-doc actions RemoveMenu

%end-doc */

static fgw_error_t pcb_act_RemoveMenu(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (pcb_gui == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't remove menu, there's no GUI hid loaded\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	if (pcb_gui->remove_menu == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't remove menu, the GUI doesn't support it\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_CONVARG(1, FGW_STR, RemoveMenu, ;);
	if (pcb_gui->remove_menu(argv[1].val.str) != 0) {
		pcb_message(PCB_MSG_ERROR, "failed to remove some of the menu items\n");
		PCB_ACT_IRES(-1);
	}
	else
		PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_SetSame[] = "SetSame()";

static const char pcb_acth_SetSame[] = "Sets current layer and sizes to match indicated item.";

/* %start-doc actions SetSame

When invoked over any line, arc, polygon, or via, this changes the
current layer to be the layer that item is on, and changes the current
sizes (thickness, clearance, drill, etc) according to that item.

%end-doc */
static void set_same_(pcb_coord_t Thick, pcb_coord_t Diameter, pcb_coord_t Hole, pcb_coord_t Clearance, char *Name)
{
	int known;
	known = pcb_route_style_lookup(&PCB->RouteStyle, Thick, Diameter, Hole, Clearance, Name);
	if (known < 0) {
		/* unknown style, set properties */
		if (Thick != -1)     { pcb_custom_route_style.Thick     = Thick;     conf_set_design("design/line_thickness", "%$mS", Thick); }
		if (Clearance != -1) { pcb_custom_route_style.Clearance = Clearance; conf_set_design("design/clearance", "%$mS", Clearance); }
		if (Diameter != -1)  { pcb_custom_route_style.Diameter  = Diameter;  conf_set_design("design/via_thickness", "%$mS", Diameter); }
		if (Hole != -1)      { pcb_custom_route_style.Hole      = Hole;      conf_set_design("design/via_drilling_hole", "%$mS", Hole); }
		PCB->pen_attr = NULL;
	}
	else
		pcb_use_route_style_idx(&PCB->RouteStyle, known);
}

static fgw_error_t pcb_act_SetSame(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y;
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_layer_t *layer = CURRENT;

	pcb_hid_get_coords("Select item to use properties from", &x, &y);

	type = pcb_search_screen(x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);
/* set layer current and size from line or arc */
	switch (type) {
	case PCB_OBJ_LINE:
		pcb_notify_crosshair_change(pcb_false);
		set_same_(((pcb_line_t *) ptr2)->Thickness, -1, -1, ((pcb_line_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (conf_core.editor.mode != PCB_MODE_LINE)
			pcb_tool_select_by_id(PCB_MODE_LINE);
		pcb_notify_crosshair_change(pcb_true);
		pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
		break;

	case PCB_OBJ_ARC:
		pcb_notify_crosshair_change(pcb_false);
		set_same_(((pcb_arc_t *) ptr2)->Thickness, -1, -1, ((pcb_arc_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (conf_core.editor.mode != PCB_MODE_ARC)
			pcb_tool_select_by_id(PCB_MODE_ARC);
		pcb_notify_crosshair_change(pcb_true);
		pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
		break;

	case PCB_OBJ_POLY:
		layer = (pcb_layer_t *) ptr1;
		break;

	default:
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (layer != CURRENT) {
		pcb_layervis_change_group_vis(pcb_layer_id(PCB->Data, layer), pcb_true, pcb_true);
		pcb_redraw();
	}
	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_SwitchHID[] = "SwitchHID(lesstif|gtk|batch)";

static const char pcb_acth_SwitchHID[] = "Switch to another HID.";

/* %start-doc actions SwitchHID

Switch to another HID.

%end-doc */

static fgw_error_t pcb_act_SwitchHID(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hid_t *ng;
	int chg;

	PCB_ACT_CONVARG(1, FGW_STR, SwitchHID, ;);
	ng = pcb_hid_find_gui(argv[1].val.str);
	if (ng == NULL) {
		pcb_message(PCB_MSG_ERROR, "No such HID.");
		PCB_ACT_IRES(-1);
		return 0;
	}

	pcb_next_gui = ng;
	chg = PCB->Changed;
	pcb_quit_app();
	PCB->Changed = chg;

	PCB_ACT_IRES(0);
	return 0;
}


/* --------------------------------------------------------------------------- */

/* This action is provided for CLI convenience */
static const char pcb_acts_FullScreen[] = "pcb_act_FullScreen(on|off|toggle)\n";
static const char pcb_acth_FullScreen[] = "Hide widgets to get edit area full screen";

static fgw_error_t pcb_act_FullScreen(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id = -2;
	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, FullScreen, id = fgw_keyword(&argv[1]));

	PCB_ACT_IRES(0);
	switch(id) {
		case -2:
		case F_Toggle:
			conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !conf_core.editor.fullscreen, POL_OVERWRITE);
			break;
		case F_On:
			conf_set(CFR_DESIGN, "editor/fullscreen", -1, "1", POL_OVERWRITE);
			break;
		case F_Off:
			conf_set(CFR_DESIGN, "editor/fullscreen", -1, "0", POL_OVERWRITE);
			break;
		default:
			PCB_ACT_FAIL(FullScreen);
	}
	return 0;
}

static const char pcb_acts_cursor[] = "Cursor(Type,DeltaUp,DeltaRight,Units)";
static const char pcb_acth_cursor[] = "Move the cursor.";
/* %start-doc actions Cursor

This action moves the mouse cursor.  Unlike other actions which take
coordinates, this action's coordinates are always relative to the
user's view of the board.  Thus, a positive @var{DeltaUp} may move the
cursor towards the board origin if the board is inverted.

Type is one of @samp{Pan} or @samp{Warp}.  @samp{Pan} causes the
viewport to move such that the crosshair is under the mouse cursor.
@samp{Warp} causes the mouse cursor to move to be above the crosshair.

@var{Units} can be one of the following:

@table @samp

@item mil
@itemx mm
The cursor is moved by that amount, in board units.

@item grid
The cursor is moved by that many grid points.

@item view
The values are percentages of the viewport's view.  Thus, a pan of
@samp{100} would scroll the viewport by exactly the width of the
current view.

@item board
The values are percentages of the board size.  Thus, a move of
@samp{50,50} moves you halfway across the board.

@end table

%end-doc */

static fgw_error_t pcb_act_Cursor(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_unit_list_t extra_units_x = {
		{"grid", 0, 0},
		{"view", 0, PCB_UNIT_PERCENT},
		{"board", 0, PCB_UNIT_PERCENT},
		{"", 0, 0}
	};
	pcb_unit_list_t extra_units_y = {
		{"grid", 0, 0},
		{"view", 0, PCB_UNIT_PERCENT},
		{"board", 0, PCB_UNIT_PERCENT},
		{"", 0, 0}
	};
	int pan_warp = HID_SC_DO_NOTHING;
	double dx, dy;
	pcb_coord_t view_width, view_height;

	extra_units_x[0].scale = PCB->Grid;
	extra_units_x[2].scale = PCB->MaxWidth;

	extra_units_y[0].scale = PCB->Grid;
	extra_units_y[2].scale = PCB->MaxHeight;

	pcb_gui->get_view_size(&view_width, &view_height);

	extra_units_x[1].scale = view_width;
	extra_units_y[1].scale = view_height;

	if (argc != 4)
		PCB_ACT_FAIL(cursor);

	if (pcb_strcasecmp(argv[0], "pan") == 0)
		pan_warp = HID_SC_PAN_VIEWPORT;
	else if (pcb_strcasecmp(argv[0], "warp") == 0)
		pan_warp = HID_SC_WARP_POINTER;
	else
		PCB_ACT_FAIL(cursor);

	dx = pcb_get_value_ex(argv[1], argv[3], NULL, extra_units_x, "", NULL);
	if (conf_core.editor.view.flip_x)
		dx = -dx;
	dy = pcb_get_value_ex(argv[2], argv[3], NULL, extra_units_y, "", NULL);
	if (!conf_core.editor.view.flip_y)
		dy = -dy;
	
	/* Allow leaving snapped pin/pad/padstack */
	if (pcb_crosshair.snapped_pstk) {
		pcb_pstk_t *ps = pcb_crosshair.snapped_pstk;
		pcb_coord_t radius = ((ps->BoundingBox.X2 - ps->BoundingBox.X1) + (ps->BoundingBox.Y2 - ps->BoundingBox.Y1))/2;
		if (dx < 0)
			dx -= radius;
		else if (dx > 0)
			dx += radius;
		if (dy < 0)
			dy -= radius;
		else if (dy > 0)
			dy += radius;
	}

	pcb_event_move_crosshair(pcb_crosshair.X + dx, pcb_crosshair.Y + dy);
	pcb_gui->set_crosshair(pcb_crosshair.X, pcb_crosshair.Y, pan_warp);

	return 0;
	PCB_OLD_ACT_END;
}


#define istrue(s) ((*(s) == '1') || (*(s) == 'y') || (*(s) == 'Y') || (*(s) == 't') || (*(s) == 'T'))

static const char pcb_acts_EditLayer[] = "Editlayer([@layer], [name=text|auto=[0|1]|sub=[0|1])]\nEditlayer([@layer], attrib, key=value)";
static const char pcb_acth_EditLayer[] = "Change a property or attribute of a layer. If the first argument starts with @, it is taken as the layer name to manipulate, else the action uses the current layer. Without arguments or if only a layer name is specified, interactive runs editing.";
static fgw_error_t pcb_act_EditLayer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	int ret = 0, n, interactive = 1, explicit = 0;
	pcb_layer_t *ly = CURRENT;

	for(n = 0; n < argc; n++) {
		if (!explicit && (*argv[n] == '@')) {
			pcb_layer_id_t lid = pcb_layer_by_name(PCB->Data, argv[n]+1);
			if (lid < 0) {
				pcb_message(PCB_MSG_ERROR, "Can't find layer named %s\n", argv[n]+1);
				return 1;
			}
			ly = pcb_get_layer(PCB->Data, lid);
			explicit = 1;
		}
		else if (strncmp(argv[n], "name=", 5) == 0) {
			interactive = 0;
			ret |= pcb_layer_rename_(ly, pcb_strdup(argv[n]+5));
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(argv[n], "auto=", 5) == 0) {
			interactive = 0;
			if (istrue(argv[n]+5))
				ly->comb |= PCB_LYC_AUTO;
			else
				ly->comb &= ~PCB_LYC_AUTO;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(argv[n], "sub=", 4) == 0) {
			interactive = 0;
			if (istrue(argv[n]+4))
				ly->comb |= PCB_LYC_SUB;
			else
				ly->comb &= ~PCB_LYC_SUB;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(argv[n], "attrib", 6) == 0) {
			char *key, *val;
			interactive = 0;
			n++;
			if (n >= argc) {
				pcb_message(PCB_MSG_ERROR, "Need an attribute name=value\n", argv[n]+1);
				return 1;
			}
			key = pcb_strdup(argv[n]);
			val = strchr(key, '=');
			if (val != NULL) {
				*val = '\0';
				val++;
				if (*val == '\0')
					val = NULL;
			}
			if (val == NULL)
				ret |= pcb_attribute_remove(&ly->Attributes, key);
			else
				ret |= pcb_attribute_put(&ly->Attributes, key, val);
			free(key);
			pcb_board_set_changed_flag(pcb_true);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Invalid EditLayer() command: %s\n", argv[n]);
			PCB_ACT_FAIL(EditLayer);
		}
	}

	if (interactive) {
		int ar;
		pcb_hid_attr_val_t rv[16];
		pcb_hid_attribute_t attr[] = {
			{"name", "logical layer name",          PCB_HATT_STRING, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
			{"sub: drawn using subtraction", NULL,  PCB_HATT_BOOL, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
			{"auto: auto-generated layer", NULL,    PCB_HATT_BOOL, 0, 0, {0}, NULL, NULL, 0, NULL, NULL}
		};

		attr[0].default_val.str_value = pcb_strdup(ly->name);
		attr[1].default_val.int_value = ly->comb & PCB_LYC_SUB;
		attr[2].default_val.int_value = ly->comb & PCB_LYC_AUTO;

		ar = pcb_attribute_dialog(attr,sizeof(attr)/sizeof(attr[0]), rv, "Edit layer properties", "Edit the properties of a logical layer", NULL);

		if (ar == 0) {
			pcb_layer_combining_t comb = 0;
			if (strcmp(ly->name, attr[0].default_val.str_value) != 0) {
				ret |= pcb_layer_rename_(ly, (char *)attr[0].default_val.str_value);
				attr[0].default_val.str_value = NULL;
				pcb_board_set_changed_flag(pcb_true);
			}
			if (attr[1].default_val.int_value) comb |= PCB_LYC_SUB;
			if (attr[2].default_val.int_value) comb |= PCB_LYC_AUTO;
			if (ly->comb != comb) {
				ly->comb = comb;
				pcb_board_set_changed_flag(pcb_true);
			}
		}
		free((char *)attr[0].default_val.str_value);
		ret |= ar;
	}

	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return ret;
	PCB_OLD_ACT_END;
}

pcb_layergrp_id_t pcb_actd_EditGroup_gid = -1;
static const char pcb_acts_EditGroup[] = "Editgroup([@group], [name=text|type=+bit|type=-bit])]\nEditlayer([@layer], attrib, key=value)";
static const char pcb_acth_EditGroup[] = "Change a property or attribute of a layer group. If the first argument starts with @, it is taken as the group name to manipulate, else the action uses the current layer's group. Without arguments or if only a layer name is specified, interactive runs editing.";
static fgw_error_t pcb_act_EditGroup(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	int ret = 0, n, interactive = 1, explicit = 0;
	pcb_layergrp_t *g = NULL;

#warning layer TODO: rewrite this with DAD in two versions (bound and unbound)
	if (CURRENT->is_bound) {
		pcb_message(PCB_MSG_ERROR, "Can't edit bound layers yet\n");
		return 1;
	}

	if (CURRENT != NULL)
		g = pcb_get_layergrp(PCB, CURRENT->meta.real.grp);

	for(n = 0; n < argc; n++) {
		if (!explicit && (*argv[n] == '@')) {
			pcb_layergrp_id_t gid;
			if (argv[n][1] == '\0')
				gid = pcb_actd_EditGroup_gid;
			else
				gid = pcb_layergrp_by_name(PCB, argv[n]+1);
			if (gid < 0) {
				pcb_message(PCB_MSG_ERROR, "Can't find layer group named %s\n", argv[n]+1);
				return 1;
			}
			g = pcb_get_layergrp(PCB, gid);
			explicit = 1;
		}
		else if (strncmp(argv[n], "name=", 5) == 0) {
			interactive = 0;
			ret |= pcb_layergrp_rename_(g, pcb_strdup(argv[n]+5));
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(argv[n], "type=", 5) == 0) {
			const char *sbit = argv[n]+5;
			pcb_layer_type_t bit = pcb_layer_type_str2bit(sbit+1);
			if (bit == 0) {
				if (strcmp(sbit+1, "anything") == 0) bit = PCB_LYT_ANYTHING;
				else if (strcmp(sbit+1, "anywhere") == 0) bit = PCB_LYT_ANYWHERE;
			}
			if (bit == 0) {
				pcb_message(PCB_MSG_ERROR, "Unknown type bit %s\n", sbit+1);
				return 1;
			}
			switch(*sbit) {
				case '+': g->ltype |= bit; break;
				case '-': g->ltype &= ~bit; break;
			}
			interactive = 0;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(argv[n], "attrib", 6) == 0) {
			char *key, *val;
			interactive = 0;
			n++;
			if (n >= argc) {
				pcb_message(PCB_MSG_ERROR, "Need an attribute name=value\n", argv[n]+1);
				return 1;
			}
			key = pcb_strdup(argv[n]);
			val = strchr(key, '=');
			if (val != NULL) {
				*val = '\0';
				val++;
				if (*val == '\0')
					val = NULL;
			}
			if (val == NULL)
				ret |= pcb_attribute_remove(&g->Attributes, key);
			else
				ret |= pcb_attribute_put(&g->Attributes, key, val);
			free(key);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Invalid EditGroup() command: %s\n", argv[n]);
			PCB_ACT_FAIL(EditLayer);
		}
	}

	if (interactive) {
		int ar;
		pcb_hid_attr_val_t rv[16];
		pcb_hid_attribute_t attr[] = {
			{"name", "logical layer name",          PCB_HATT_STRING, 0, 0, {0}, NULL, NULL, 0, NULL, NULL},
		};

		attr[0].default_val.str_value = pcb_strdup(g->name);

		ar = pcb_attribute_dialog(attr,sizeof(attr)/sizeof(attr[0]), rv, "Edit layer properties", "Edit the properties of a logical layer", NULL);

		if (ar == 0) {
			if (strcmp(g->name, attr[0].default_val.str_value) != 0) {
				ret |= pcb_layergrp_rename_(g, (char *)attr[0].default_val.str_value);
				attr[0].default_val.str_value = NULL;
				pcb_board_set_changed_flag(pcb_true);
			}
		}

		free((char *)attr[0].default_val.str_value);

		ret |= ar;
	}

	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return ret;
	PCB_OLD_ACT_END;
}


const char pcb_acts_selectlayer[] = "SelectLayer(1..MAXLAYER|Silk|Rats)";
const char pcb_acth_selectlayer[] = "Select which layer is the current layer.";

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

static fgw_error_t pcb_act_SelectLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_id_t lid;
	const pcb_menu_layers_t *ml;
	char *name;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_STR, selectlayer, name = argv[1].val.str);

	if (pcb_strcasecmp(name, "silk") == 0) {
		PCB->RatDraw = 0;
		if (pcb_layer_list(PCB, PCB_LYT_VISIBLE_SIDE() | PCB_LYT_SILK, &lid, 1) > 0) {
			pcb_layervis_change_group_vis(lid, 1, 1);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Can't find this-side silk layer\n");
			PCB_ACT_IRES(-1);
		}
		return 0;
	}

	ml = pcb_menu_layer_find(name);
	if (ml != NULL) {
		pcb_bool *v = (pcb_bool *)((char *)PCB + ml->vis_offs);
		pcb_bool *s = (pcb_bool *)((char *)PCB + ml->sel_offs);
		if (ml->sel_offs == 0) {
			pcb_message(PCB_MSG_ERROR, "Virtual layer '%s' (%s) can not be selected\n", ml->name, ml->abbrev);
			PCB_ACT_IRES(-1);
			return 0;
		}
		*s = *v = 1;
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
		return 0;
	}

	PCB->RatDraw = 0;
	pcb_layervis_change_group_vis(atoi(name)-1, 1, 1);
	pcb_gui->invalidate_all();
	pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	return 0;
}

const char pcb_acts_chklayer[] = "ChkLayer(layerid)";
const char pcb_acth_chklayer[] = "Returns 1 if the specified layer is the active layer";
/* %start-doc actions ChkLayer

Returns 1 if the specified layer is the active layer.

%end-doc */

static fgw_error_t pcb_act_ChkLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_id_t lid;
	pcb_layer_t *ly;
	char *end;
	const char *name;
	const pcb_menu_layers_t *ml;

	PCB_ACT_CONVARG(1, FGW_STR, chklayer, name = argv[1].val.str);

	lid = strtol(name, &end, 10);
	if (*end != '\0') {
		ml = pcb_menu_layer_find(name);
		if (ml != NULL) {
			if (ml->sel_offs != 0) {
				pcb_bool *s = (pcb_bool *)((char *)PCB + ml->sel_offs);
				PCB_ACT_IRES(*s);
			}
			else
				PCB_ACT_IRES(-1);
			return 0;
		}
		pcb_message(PCB_MSG_ERROR, "pcb_act_ChkLayer: '%s' is not a valid layer ID - check your menu file!\n", argv[0]);
		PCB_ACT_IRES(-1);
		return 0;
	}

	/* if any virtual is selected, do not accept CURRENT as selected */
	for(ml = pcb_menu_layers; ml->name != NULL; ml++) {
		pcb_bool *s = (pcb_bool *)((char *)PCB + ml->sel_offs);
		if ((ml->sel_offs != 0) && (*s)) {
			PCB_ACT_IRES(0);
			return 0;
		}
	}

	lid--;
	ly = pcb_get_layer(PCB->Data, lid);
	if (ly == NULL)
		PCB_ACT_IRES(-1);
	else
		PCB_ACT_IRES(ly == CURRENT);
	return 0;
}


const char pcb_acts_toggleview[] = "ToggleView(1..MAXLAYER)\n" "ToggleView(layername)\n" "ToggleView(Silk|Rats|Pins|Vias|BackSide)\n" "ToggleView(All, Open|Vis, Set|Clear|Toggle)";
const char pcb_acth_toggleview[] = "Toggle the visibility of the specified layer or layer group.";

/* %start-doc actions ToggleView

If you pass an integer, that layer is specified by index (the first
layer is @code{1}, etc).  If you pass a layer name, that layer is
specified by name.  When a layer is specified, the visibility of the
layer group containing that layer is toggled.

If you pass a special layer name, the visibility of those components
(silk, rats, etc) is toggled.  Note that if you have a layer named
the same as a special layer, the layer is chosen over the special layer.

%end-doc */

static fgw_error_t pcb_act_ToggleView(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_id_t lid;
	const char *name;

	PCB_ACT_CONVARG(1, FGW_STR, toggleview, name = argv[1].val.str);
	PCB_ACT_IRES(0);

	if (pcb_strcasecmp(name, "all") == 0) {
		pcb_bool_op_t open = PCB_BOOL_PRESERVE, vis = PCB_BOOL_PRESERVE, user = PCB_BOOL_PRESERVE;
		const char *cmd, *suser;
		PCB_ACT_CONVARG(2, FGW_STR, toggleview, cmd = argv[2].val.str);
		PCB_ACT_CONVARG(3, FGW_STR, toggleview, suser = argv[3].val.str);

		user = pcb_str2boolop(suser);
		if (user == PCB_BOOL_INVALID)
			PCB_ACT_FAIL(toggleview);
		if (pcb_strcasecmp(cmd, "open") == 0)
			open = user;
		else if (pcb_strcasecmp(cmd, "vis") == 0)
			vis = user;
		else
			PCB_ACT_FAIL(toggleview);
		pcb_layer_vis_change_all(PCB, open, vis);
		pcb_gui->invalidate_all();
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
		return 0;
	}
	else if (pcb_strcasecmp(name, "silk") == 0) {
		if (pcb_layer_list(PCB, PCB_LYT_VISIBLE_SIDE() | PCB_LYT_SILK, &lid, 1) > 0)
			pcb_layervis_change_group_vis(lid, -1, 0);
		else
			pcb_message(PCB_MSG_ERROR, "Can't find this-side silk layer\n");
	}
	else if ((pcb_strcasecmp(name, "padstacks") == 0) || (pcb_strcasecmp(name, "vias") == 0) || (pcb_strcasecmp(name, "pins") == 0) || (pcb_strcasecmp(name, "pads") == 0)) {
		PCB->pstk_on = !PCB->pstk_on;
		pcb_gui->invalidate_all();
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else if (pcb_strcasecmp(name, "BackSide") == 0) {
		PCB->InvisibleObjectsOn = !PCB->InvisibleObjectsOn;
		pcb_gui->invalidate_all();
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else if (strncmp(name, "ui:", 3) == 0) {
		pcb_layer_t *ly = vtlayer_get(&pcb_uilayer, atoi(name+3), 0);
		if (ly == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid ui layer id: '%s'\n", name);
			PCB_ACT_IRES(-1);
			return 0;
		}
		ly->meta.real.vis = !ly->meta.real.vis;
		pcb_gui->invalidate_all();
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else {
		char *end;
		int id = strtol(name, &end, 10) - 1;
		if (*end == '\0') { /* integer layer */
			pcb_layervis_change_group_vis(id, -1, 0);
			pcb_gui->invalidate_all();
			pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
			return 0;
		}
		else { /* virtual menu layer */
			const pcb_menu_layers_t *ml = pcb_menu_layer_find(name);
			if (ml != NULL) {
				pcb_bool *v = (pcb_bool *)((char *)PCB + ml->vis_offs);
				*v = !(*v);
				pcb_gui->invalidate_all();
				pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
				return 0;
			}
			pcb_message(PCB_MSG_ERROR, "Invalid layer id: '%s'\n", name);
		}
	}

	PCB_ACT_IRES(1);
	return 0;
}

const char pcb_acts_chkview[] = "ChkView(layerid)\n";
const char pcb_acth_chkview[] = "Return 1 if layerid is visible.";

/* %start-doc actions ChkView
Return 1 if layerid is visible. Intended for meu item 'checked' fields.
%end-doc */

static fgw_error_t pcb_act_ChkView(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_id_t lid;
	pcb_layer_t *ly;
	char *end;
	const char *name;

	PCB_ACT_CONVARG(1, FGW_STR, chkview, name = argv[1].val.str);

	if (strncmp(name, "ui:", 3) == 0) {
		pcb_layer_t *ly = vtlayer_get(&pcb_uilayer, atoi(name+3), 0);
		if (ly == NULL) {
			PCB_ACT_IRES(-1);
			return 0;
		}
		PCB_ACT_IRES(ly->meta.real.vis);
		return 0;
	}

	lid = strtol(name, &end, 10);

	if (*end != '\0') {
		const pcb_menu_layers_t *ml = pcb_menu_layer_find(name);

		if (ml != NULL) {
			pcb_bool *v = (pcb_bool *)((char *)PCB + ml->vis_offs);
			return *v;
		}

		pcb_message(PCB_MSG_ERROR, "pcb_act_ChkView: '%s' is not a valid layer ID - check your menu file!\n", name);
		return FGW_ERR_ARGV_TYPE;
	}

	lid--;
	ly = pcb_get_layer(PCB->Data, lid);
	if (ly == NULL) {
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_IRES(ly->meta.real.vis);
	return 0;
}


static const char pcb_acts_setunits[] = "SetUnits(mm|mil)";
static const char pcb_acth_setunits[] = "Set the default measurement units.";

/* %start-doc actions SetUnits

@table @code

@item mil
Sets the display units to mils (1/1000 inch).

@item mm
Sets the display units to millimeters.

@end table

%end-doc */

static fgw_error_t pcb_act_SetUnits(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const pcb_unit_t *new_unit;
	const char *name;

	PCB_ACT_CONVARG(1, FGW_STR, setunits, name = argv[1].val.str);
	PCB_ACT_IRES(0);

	new_unit = get_unit_struct(name);
	pcb_board_set_unit(PCB, new_unit);

	return 0;
}

static const char pcb_acts_grid[] =
	"grid(set, [name:]size[@offs][!unit])\n"
	"grid(+|up)\n" "grid(-|down)\n" "grid(#N)\n" "grid(idx, N)\n";
static const char pcb_acth_grid[] = "Set the grid.";
static fgw_error_t pcb_act_grid(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op, *a;

	PCB_ACT_CONVARG(1, FGW_STR, grid, op = argv[1].val.str);
	PCB_ACT_IRES(0);

	if (strcmp(op, "set") == 0) {
		pcb_grid_t dst;
		PCB_ACT_CONVARG(2, FGW_STR, grid, a = argv[2].val.str);
		if (!pcb_grid_parse(&dst, a))
			PCB_ACT_FAIL(grid);
		pcb_grid_set(PCB, &dst);
		pcb_grid_free(&dst);
	}
	else if ((strcmp(op, "up") == 0) || (strcmp(op, "+") == 0))
		pcb_grid_list_step(+1);
	else if ((strcmp(op, "down") == 0) || (strcmp(op, "-") == 0))
		pcb_grid_list_step(-1);
	else if (strcmp(op, "idx") == 0) {
		PCB_ACT_CONVARG(2, FGW_STR, grid, a = argv[2].val.str);
		pcb_grid_list_jump(atoi(a));
	}
	else if (op[0] == '#') {
		pcb_grid_list_jump(atoi(op+1));
	}
	else
		PCB_ACT_FAIL(grid);

	return 0;
}

const char pcb_acts_chkrst[] = "ChkRst(route_style_id)\n";
const char pcb_acth_chkrst[] = "Return 1 if route_style_id matches pen.";

/* %start-doc actions ChkRst
Return 1 if route_style_id matches pen.
%end-doc */

static fgw_error_t pcb_act_ChkRst(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int rid;
	pcb_route_style_t *rst;

	PCB_ACT_CONVARG(1, FGW_INT, chkrst, rid = argv[1].val.nat_int);

	rst = vtroutestyle_get(&PCB->RouteStyle, rid, 0);
	if (rst == NULL)
		PCB_ACT_IRES(-1);
	else
		PCB_ACT_IRES(pcb_route_style_match(rst, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL));
	return 0;
}

static const char pcb_acts_GetXY[] = "GetXY()";
static const char pcb_acth_GetXY[] = "Get a coordinate.";
/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */
static fgw_error_t pcb_act_GetXY(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y;
	const char *msg = "Click to enter a coordinate.";
	PCB_ACT_MAY_CONVARG(1, FGW_STR, GetXY, msg = argv[1].val.str);
	PCB_ACT_IRES(0);
	pcb_hid_get_coords(msg, &x, &y);
	return 0;
}

static pcb_hid_attribute_t printer_calibrate_attrs[] = {
	{N_("Enter Values here:"), "",
	 PCB_HATT_LABEL, 0, 0, {0, 0, 0}, 0, 0},
	{N_("x-calibration"), N_("X scale for calibrating your printer"),
	 PCB_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0},
	{N_("y-calibration"), N_("Y scale for calibrating your printer"),
	 PCB_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0}
};

static pcb_hid_attr_val_t printer_calibrate_values[3];

const char pcb_acts_PrintCalibrate[] = "PrintCalibrate()";
const char pcb_acth_PrintCalibrate[] = N_("Calibrate the printer.");

/* %start-doc actions PrintCalibrate

This will print a calibration page, which you would measure and type
the measurements in, so that future printouts will be more precise.

%end-doc */

fgw_error_t pcb_act_PrintCalibrate(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_hid_t *printer = pcb_hid_find_printer();
	printer->calibrate(0.0, 0.0);

	if (pcb_attribute_dialog(printer_calibrate_attrs, 3, printer_calibrate_values, "Printer Calibration Values", "Enter calibration values for your printer", NULL))
		return 1;
	printer->calibrate(printer_calibrate_values[1].real_value, printer_calibrate_values[2].real_value);
	return 0;
	PCB_OLD_ACT_END;
}

pcb_action_t gui_action_list[] = {
	{"Display", pcb_act_Display, pcb_acth_Display, pcb_acts_Display},
	{"CycleDrag", pcb_act_CycleDrag, pcb_acth_CycleDrag, pcb_acts_CycleDrag},
	{"FullScreen", pcb_act_FullScreen, pcb_acth_FullScreen, pcb_acts_FullScreen},
	{"MarkCrosshair", pcb_act_MarkCrosshair, pcb_acth_MarkCrosshair, pcb_acts_MarkCrosshair},
	{"Message", pcb_act_Message, pcb_acth_Message, pcb_acts_Message},
	{"Mode", pcb_act_Mode, pcb_acth_Mode, pcb_acts_Mode},
	{"SetSame", pcb_act_SetSame, pcb_acth_SetSame, pcb_acts_SetSame},
	{"RouteStyle", pcb_act_RouteStyle, pcb_acth_RouteStyle, pcb_acts_RouteStyle},
	{"CreateMenu", pcb_act_CreateMenu, pcb_acth_CreateMenu, pcb_acts_CreateMenu},
	{"RemoveMenu", pcb_act_RemoveMenu, pcb_acth_RemoveMenu, pcb_acts_RemoveMenu},
	{"SelectLayer", pcb_act_SelectLayer, pcb_acth_selectlayer, pcb_acts_selectlayer},
	{"ChkLayer", pcb_act_ChkLayer, pcb_acth_chklayer, pcb_acts_chklayer},
	{"SwitchHID", pcb_act_SwitchHID, pcb_acth_SwitchHID, pcb_acts_SwitchHID},
	{"ToggleView", pcb_act_ToggleView, pcb_acth_toggleview, pcb_acts_toggleview},
	{"ChkView", pcb_act_ChkView, pcb_acth_chkview, pcb_acts_chkview},
	{"Cursor", pcb_act_Cursor, pcb_acth_cursor, pcb_acts_cursor},
	{"EditLayer", pcb_act_EditLayer, pcb_acth_EditLayer, pcb_acts_EditLayer},
	{"EditGroup", pcb_act_EditGroup, pcb_acth_EditGroup, pcb_acts_EditGroup},
	{"Grid", pcb_act_grid, pcb_acth_grid, pcb_acts_grid},
	{"SetUnits", pcb_act_SetUnits, pcb_acth_setunits, pcb_acts_setunits},
	{"ChkRst", pcb_act_ChkRst, pcb_acth_chkrst, pcb_acts_chkrst},
	{"GetXY", pcb_act_GetXY, pcb_acth_GetXY, pcb_acts_GetXY},
	{"PrintCalibrate", pcb_act_PrintCalibrate, pcb_acth_PrintCalibrate, pcb_acts_PrintCalibrate}
};

PCB_REGISTER_ACTIONS(gui_action_list, NULL)
