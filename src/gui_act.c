/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2019, 2020 Tibor 'Igor2' Palinkas
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
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "data.h"
#include <librnd/core/tool.h>
#include <librnd/core/grid.h>
#include <librnd/core/error.h>
#include "undo.h"
#include "funchash_core.h"
#include "change.h"

#include "draw.h"
#include "search.h"
#include "find.h"
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/compat_misc.h>
#include "event.h"
#include "layer.h"
#include "layer_ui.h"
#include "layer_grp.h"
#include "layer_vis.h"
#include <librnd/core/attrib.h>
#include <librnd/core/hid_attrib.h>
#include "operation.h"
#include "obj_subc_op.h"
#include <librnd/core/tool.h>
#include "route_style.h"
#include "tool_logic.h"

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
/* DOC: display.html */
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
			pcb_conf_set(CFR_DESIGN, "editor/subc_id", -1, str_dir, POL_OVERWRITE);
		else
			pcb_conf_set(CFR_DESIGN, "editor/subc_id", -1, "", POL_OVERWRITE);

		pcb_gui->invalidate_all(pcb_gui); /* doesn't change too often, isn't worth anything more complicated */
		pcb_draw();
		return 0;
	}

	if (id == F_TermID) { /* change the displayed name of terminals */
		if (argc > 1)
			pcb_conf_set(CFR_DESIGN, "editor/term_id", -1, str_dir, POL_OVERWRITE);
		else
			pcb_conf_set(CFR_DESIGN, "editor/term_id", -1, "", POL_OVERWRITE);

		pcb_gui->invalidate_all(pcb_gui); /* doesn't change too often, isn't worth anything more complicated */
		pcb_draw();
		return 0;
	}

	if (!str_dir || !*str_dir) {
		switch (id) {

			/* redraw layout */
		case F_ClearAndRedraw:
		case F_Redraw:
			pcb_hid_redraw(PCB);
			break;

			/* toggle line-adjust flag */
		case F_ToggleAllDirections:
			conf_toggle_editor(all_direction_lines);
			pcb_tool_adjust_attached(PCB_ACT_HIDLIB);
			break;

		case F_CycleClip:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			if (conf_core.editor.all_direction_lines) {
				conf_toggle_editor(all_direction_lines);
				pcb_conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",0);
			}
			else {
				pcb_conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",(conf_core.editor.line_refraction +1) % 3);
			}
			pcb_tool_adjust_attached(PCB_ACT_HIDLIB);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_CycleCrosshair:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			pcb_conf_setf(CFR_CLI, "editor/crosshair_shape_idx", 0, "%d", CrosshairShapeIncrement(pcbhl_conf.editor.crosshair_shape_idx));
			if (pcb_ch_shape_NUM == pcbhl_conf.editor.crosshair_shape_idx)
				pcb_conf_set(CFR_CLI, "editor/crosshair_shape_idx", 0, "0", POL_OVERWRITE);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleRubberBandMode:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(rubber_band_mode);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleStartDirection:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(swap_start_direction);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleUniqueNames:
			pcb_message(PCB_MSG_ERROR, "Unique names/refdes is not supported any more - please use the renumber plugin\n");
			break;

		case F_ToggleSnapPin:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(snap_pin);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleSnapOffGridLine:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(snap_offgrid_line);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleHighlightOnPoint:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(highlight_on_point);
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleLocalRef:
			conf_toggle_editor(local_ref);
			break;

		case F_ToggleThindraw:
			conf_toggle_editor(thin_draw);
			pcb_hid_redraw(PCB);
			break;

		case F_ToggleThindrawPoly:
			conf_toggle_editor(thin_draw_poly);
			pcb_hid_redraw(PCB);
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
			pcb_hid_redraw(PCB);
			break;

		case F_ToggleStroke:
			conf_toggle_heditor(enable_stroke);
			break;

		case F_ToggleShowDRC:
			conf_toggle_editor(show_drc);
			break;

		case F_ToggleLiveRoute:
			conf_toggle_editor(live_routing);
			break;

		case F_ToggleAutoDRC:
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
			conf_toggle_editor(auto_drc);
			if (conf_core.editor.auto_drc && pcbhl_conf.editor.mode == pcb_crosshair.tool_line) {
				if (pcb_data_clear_flag(PCB->Data, PCB_FLAG_FOUND, 1, 1) > 0) {
					pcb_undo_inc_serial();
					pcb_draw();
				}
				if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST) {
					pcb_find_t fctx;
					memset(&fctx, 0, sizeof(fctx));
					fctx.flag_set = PCB_FLAG_FOUND;
					fctx.flag_chg_undoable = 1;
					pcb_find_from_xy(&fctx, PCB->Data, pcb_crosshair.X, pcb_crosshair.Y);
					pcb_find_free(&fctx);
				}
			}
			pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
			break;

		case F_ToggleCheckPlanes:
			conf_toggle_editor(check_planes);
			pcb_hid_redraw(PCB);
			break;

		case F_ToggleOrthoMove:
			conf_toggle_editor(orthogonal_moves);
			break;

		case F_ToggleName:
			conf_toggle_editor(show_number);
			pcb_hid_redraw(PCB);
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
				pcb_coord_t oldGrid = PCB_ACT_HIDLIB->grid;

				PCB_ACT_HIDLIB->grid = 1;
				if (pcb_crosshair_move_absolute(pcb_crosshair.X, pcb_crosshair.Y))
					pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true); /* first notify was in MoveCrosshairAbs */
				pcb_hidlib_set_grid(PCB_ACT_HIDLIB, oldGrid, pcb_true, pcb_crosshair.X, pcb_crosshair.Y);
				pcb_grid_inval();
			}
			break;

			/* toggle displaying of the grid */
		case F_Grid:
			conf_toggle_heditor(draw_grid);
			pcb_hid_redraw(PCB);
			break;

			/* display the pinout of a subcircuit */
		case F_Pinout:
			return pcb_actionva(PCB_ACT_HIDLIB, "pinout", NULL);

			/* toggle displaying of terminal names */
		case F_PinOrPadName:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;
				pcb_coord_t x, y;
				pcb_hid_get_coords("Click on a subcircuit", &x, &y, 0);

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
							((pcb_subc_t *)obj)->auto_termname_display = !((pcb_subc_t *)obj)->auto_termname_display;
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
				if (fgw_arg_conv(&pcb_fgw, &argv[3], FGW_KEYWORD) != 0) {
					PCB_ACT_FAIL(Display);
					return FGW_ERR_ARG_CONV;
				}
				PCB_ACT_HIDLIB->grid_ox = pcb_get_value(argv[2].val.str, NULL, NULL, NULL);
				PCB_ACT_HIDLIB->grid_oy = pcb_get_value(argv[3].val.str, NULL, NULL, NULL);
				if (pcbhl_conf.editor.draw_grid)
					pcb_hid_redraw(PCB);
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

/* ---------------------------------------------------------------- */
static const char pcb_acts_CycleDrag[] = "CycleDrag()\n";
static const char pcb_acth_CycleDrag[] = "Cycle through which object is being dragged";
/* DOC: cycledrag.html */
#define close_enough(a, b) ((((a)-(b)) > 0) ? ((a)-(b) < (PCB_SLOP * pcb_pixel_slop)) : ((a)-(b) > -(PCB_SLOP * pcb_pixel_slop)))
static fgw_error_t pcb_act_CycleDrag(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hidlib_t *hidlib = PCB_ACT_HIDLIB;
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
			if (close_enough(hidlib->tool_x, l->Point1.X) && close_enough(hidlib->tool_y, l->Point1.Y)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_LINE_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = &l->Point1;
				goto switched;
			}
			if (close_enough(hidlib->tool_x, l->Point2.X) && close_enough(hidlib->tool_y, l->Point2.Y)) {
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
			if (close_enough(hidlib->tool_x, ex) && close_enough(hidlib->tool_y, ey)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_ARC_POINT;
				pcb_crosshair.AttachedObject.Ptr1 = ptr1;
				pcb_crosshair.AttachedObject.Ptr2 = ptr2;
				pcb_crosshair.AttachedObject.Ptr3 = pcb_arc_start_ptr;
				goto switched;
			}
			pcb_arc_get_end((pcb_arc_t *)ptr2, 1, &ex, &ey);
			if (close_enough(hidlib->tool_x, ex) && close_enough(hidlib->tool_y, ey)) {
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
	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_RUBBER_RESET, NULL);
	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	return 0;
}

#undef close_enough

/* --------------------------------------------------------------------------- */

static const char pcb_acts_MarkCrosshair[] = "MarkCrosshair()\n" "MarkCrosshair(Center)";
static const char pcb_acth_MarkCrosshair[] = "Set/Reset the pcb_crosshair mark.";
/* DOC: markcrosshair.html */
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
			pcb_marked.user_placed = 0;
			pcb_notify_mark_change(pcb_true);
		}
		else {
			pcb_notify_mark_change(pcb_false);
			pcb_marked.status = pcb_true;
			pcb_marked.user_placed = 1;
			if (conf_core.editor.marker_snaps) {
				pcb_marked.X = pcb_crosshair.X;
				pcb_marked.Y = pcb_crosshair.Y;
			}
			else
				pcb_hid_get_coords("Click on new mark", &pcb_marked.X, &pcb_marked.Y, 0);
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
			pcb_hid_get_coords("Click on new mark", &pcb_marked.X, &pcb_marked.Y, 0);
		pcb_notify_mark_change(pcb_true);
		pcb_marked.user_placed = 1;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RouteStyle[] = "RouteStyle(style_id|style_name)";
static const char pcb_acth_RouteStyle[] = "Copies the indicated routing style into the current pen.";
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

static const char pcb_acts_SetSame[] = "SetSame()";
static const char pcb_acth_SetSame[] = "Sets current layer and sizes to match indicated item.";
/* DOC: setsame.html */
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
	pcb_layer_t *layer = PCB_CURRLAYER(PCB_ACT_BOARD);

	pcb_hid_get_coords("Select item to use properties from", &x, &y, 0);

	type = pcb_search_screen(x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);
/* set layer current and size from line or arc */
	switch (type) {
	case PCB_OBJ_LINE:
		pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
		set_same_(((pcb_line_t *) ptr2)->Thickness, -1, -1, ((pcb_line_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (pcbhl_conf.editor.mode != pcb_crosshair.tool_line)
			pcb_tool_select_by_name(PCB_ACT_HIDLIB, "line");
		pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
		break;

	case PCB_OBJ_ARC:
		pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_false);
		set_same_(((pcb_arc_t *) ptr2)->Thickness, -1, -1, ((pcb_arc_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (pcbhl_conf.editor.mode != pcb_crosshair.tool_arc)
			pcb_tool_select_by_name(PCB_ACT_HIDLIB, "arc");
		pcb_hid_notify_crosshair_change(PCB_ACT_HIDLIB, pcb_true);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
		break;

	case PCB_OBJ_POLY:
		layer = (pcb_layer_t *) ptr1;
		break;

	default:
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (layer != PCB_CURRLAYER(PCB_ACT_BOARD)) {
		pcb_layervis_change_group_vis(PCB_ACT_HIDLIB, pcb_layer_id(PCB_ACT_BOARD->Data, layer), pcb_true, pcb_true);
		pcb_hid_redraw(PCB);
	}
	PCB_ACT_IRES(0);
	return 0;
}

#define istrue(s) ((*(s) == '1') || (*(s) == 'y') || (*(s) == 'Y') || (*(s) == 't') || (*(s) == 'T'))

static const char pcb_acts_EditLayer[] = "Editlayer([@layer], [name=text|auto=[0|1]|sub=[0|1])]\nEditlayer([@layer], attrib, key=value)";
static const char pcb_acth_EditLayer[] = "Change a property or attribute of a layer. If the first argument starts with @, it is taken as the layer name to manipulate, else the action uses the current layer. Without arguments or if only a layer name is specified, interactive runs editing.";
static fgw_error_t pcb_act_EditLayer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ret = 0, n, interactive = 1, explicit = 0;
	pcb_layer_t *ly = PCB_CURRLAYER(PCB_ACT_BOARD);

	for(n = 1; n < argc; n++) {
		const char *arg;
		PCB_ACT_CONVARG(n, FGW_STR, EditLayer, arg = argv[n].val.str);
		if (!explicit && (*arg == '@')) {
			pcb_layer_id_t lid = pcb_layer_by_name(PCB->Data, arg+1);
			if (lid < 0) {
				pcb_message(PCB_MSG_ERROR, "Can't find layer named %s\n", arg+1);
				return 1;
			}
			ly = pcb_get_layer(PCB->Data, lid);
			explicit = 1;
		}
		else if (strncmp(arg, "name=", 5) == 0) {
			interactive = 0;
			ret |= pcb_layer_rename_(ly, pcb_strdup(arg+5), 1);
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(arg, "auto=", 5) == 0) {
			interactive = 0;
			if (istrue(arg+5))
				ly->comb |= PCB_LYC_AUTO;
			else
				ly->comb &= ~PCB_LYC_AUTO;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(arg, "sub=", 4) == 0) {
			interactive = 0;
			if (istrue(arg+4))
				ly->comb |= PCB_LYC_SUB;
			else
				ly->comb &= ~PCB_LYC_SUB;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(arg, "attrib", 6) == 0) {
			char *key, *val;
			interactive = 0;
			n++;
			if (n >= argc) {
				pcb_message(PCB_MSG_ERROR, "Need an attribute name=value\n", arg+1);
				return 1;
			}
			key = pcb_strdup(arg);
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
			pcb_message(PCB_MSG_ERROR, "Invalid EditLayer() command: %s\n", arg);
			PCB_ACT_FAIL(EditLayer);
		}
	}

	if (interactive) {
		char fn[PCB_ACTION_NAME_MAX];
		fgw_arg_t args[2];
		args[0].type = FGW_FUNC;
		args[0].val.argv0.func = fgw_func_lookup(&pcb_fgw, pcb_aname(fn, "LayerPropGui"));
		args[0].val.argv0.user_call_ctx = PCB_ACT_HIDLIB;
		if (args[0].val.func != NULL) {
			args[1].type = FGW_LONG;
			args[1].val.nat_long = pcb_layer_id(PCB->Data, ly);
			ret = pcb_actionv_(args[0].val.func, res, 2, args);
			pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
		}
		else
			return FGW_ERR_NOT_FOUND;
		return ret;
	}

	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
	PCB_ACT_IRES(0);
	return ret;
}

pcb_layergrp_id_t pcb_actd_EditGroup_gid = -1;
static const char pcb_acts_EditGroup[] = "Editgroup([@group], [name=text|type=+bit|type=-bit])]\nEditlayer([@layer], attrib, key=value)";
static const char pcb_acth_EditGroup[] = "Change a property or attribute of a layer group. If the first argument starts with @, it is taken as the group name to manipulate, else the action uses the current layer's group. Without arguments or if only a layer name is specified, interactive runs editing.";
static fgw_error_t pcb_act_EditGroup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ret = 0, n, interactive = 1, explicit = 0;
	pcb_layergrp_t *g = NULL;

	if (PCB_CURRLAYER(PCB_ACT_BOARD)->is_bound) {
		pcb_message(PCB_MSG_ERROR, "Can't edit bound layers yet\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	if (PCB_CURRLAYER(PCB_ACT_BOARD) != NULL)
		g = pcb_get_layergrp(PCB, PCB_CURRLAYER(PCB_ACT_BOARD)->meta.real.grp);

	for(n = 1; n < argc; n++) {
		const char *arg;
		PCB_ACT_CONVARG(n, FGW_STR, EditLayer, arg = argv[n].val.str);
		if (!explicit && (*arg == '@')) {
			pcb_layergrp_id_t gid;
			if (arg[1] == '\0')
				gid = pcb_actd_EditGroup_gid;
			else
				gid = pcb_layergrp_by_name(PCB, arg+1);
			if (gid < 0) {
				pcb_message(PCB_MSG_ERROR, "Can't find layer group named %s\n", arg+1);
				PCB_ACT_IRES(1);
				return 0;
			}
			g = pcb_get_layergrp(PCB, gid);
			explicit = 1;
		}
		else if (strncmp(arg, "name=", 5) == 0) {
			interactive = 0;
			ret |= pcb_layergrp_rename_(g, pcb_strdup(arg+5), 1);
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(arg, "type=", 5) == 0) {
			const char *sbit = arg+5;
			pcb_layer_type_t bit = pcb_layer_type_str2bit(sbit+1);
			if (bit == 0) {
				if (strcmp(sbit+1, "anything") == 0) bit = PCB_LYT_ANYTHING;
				else if (strcmp(sbit+1, "anywhere") == 0) bit = PCB_LYT_ANYWHERE;
			}
			if (bit == 0) {
				pcb_message(PCB_MSG_ERROR, "Unknown type bit %s\n", sbit+1);
				PCB_ACT_IRES(1);
				return 0;
			}
			switch(*sbit) {
				case '+': g->ltype |= bit; break;
				case '-': g->ltype &= ~bit; break;
			}
			interactive = 0;
			pcb_board_set_changed_flag(pcb_true);
		}
		else if (strncmp(arg, "attrib", 6) == 0) {
			char *key, *val;
			interactive = 0;
			n++;
			if (n >= argc) {
				pcb_message(PCB_MSG_ERROR, "Need an attribute name=value\n", arg+1);
				PCB_ACT_IRES(1);
				return 0;
			}
			key = pcb_strdup(arg);
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
			pcb_message(PCB_MSG_ERROR, "Invalid EditGroup() command: %s\n", arg);
			PCB_ACT_FAIL(EditLayer);
		}
	}

	if (interactive) {
		char fn[PCB_ACTION_NAME_MAX];
		fgw_arg_t args[2];
		args[0].type = FGW_FUNC;
		args[0].val.argv0.func = fgw_func_lookup(&pcb_fgw, pcb_aname(fn, "GroupPropGui"));
		args[0].val.argv0.user_call_ctx = PCB_ACT_HIDLIB;
		if (args[0].val.func != NULL) {
			args[1].type = FGW_LONG;
			args[1].val.nat_long = pcb_layergrp_id(PCB, g);
			ret = pcb_actionv_(args[0].val.func, res, 2, args);
			pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
		}
		else
			return FGW_ERR_NOT_FOUND;
		return ret;

	}

	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
	PCB_ACT_IRES(0);
	return ret;
}

static const char pcb_acts_DelGroup[] = "DelGroup([@group])";
static const char pcb_acth_DelGroup[] = "Remove a layer group; if the first argument is not specified, the current group is removed";
static fgw_error_t pcb_act_DelGroup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name = NULL;
	pcb_layergrp_t *g = NULL;
	pcb_layergrp_id_t gid;

	if (PCB_CURRLAYER(PCB_ACT_BOARD) != NULL)
		g = pcb_get_layergrp(PCB, PCB_CURRLAYER(PCB_ACT_BOARD)->meta.real.grp);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DelGroup, name = argv[1].val.str);
	if (*name == '@') {
		gid = pcb_actd_EditGroup_gid;
		if (name[1] != '\0')
			gid = pcb_layergrp_by_name(PCB, name+1);
		if (gid < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't find layer group named %s\n", name+1);
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else {
		if (g == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can't find layer group\n");
			PCB_ACT_IRES(1);
			return 0;
		}
		gid = pcb_layergrp_id(PCB, g);
	}

	PCB_ACT_IRES(pcb_layergrp_del(PCB, gid, 1, 1));
	return 0;
}

static const char pcb_acts_NewGroup[] = "NewGroup(type [,location [, purpose[, auto|sub [,name[,grp_attribs]]]])";
static const char pcb_acth_NewGroup[] = "Create a new layer group with a single, positive drawn layer in it";
static fgw_error_t pcb_act_NewGroup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *stype = NULL, *sloc = NULL, *spurp = NULL, *scomb = NULL, *name = NULL, *attr = NULL;
	pcb_layergrp_t *g = NULL;
	pcb_layer_type_t ltype = 0, lloc = 0;

	PCB_ACT_CONVARG(1, FGW_STR, NewGroup, stype = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, NewGroup, sloc = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, NewGroup, spurp = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, NewGroup, scomb = argv[4].val.str);
	PCB_ACT_MAY_CONVARG(5, FGW_STR, NewGroup, name = argv[5].val.str);
	PCB_ACT_MAY_CONVARG(6, FGW_STR, NewGroup, attr = argv[6].val.str);

	ltype = pcb_layer_type_str2bit(stype) & PCB_LYT_ANYTHING;
	if (ltype == 0) {
		pcb_message(PCB_MSG_ERROR, "Invalid type: '%s'\n", sloc);
		PCB_ACT_IRES(-1);
		return 0;
	}

	if ((ltype == PCB_LYT_COPPER) || (ltype == PCB_LYT_SUBSTRATE)) {
		pcb_message(PCB_MSG_ERROR, "Can not create this type of layer group: '%s'\n", sloc);
		PCB_ACT_IRES(-1);
		return 0;
	}

	if (sloc != NULL) {
		if (strcmp(sloc, "global") != 0) {
			lloc = pcb_layer_type_str2bit(sloc) & PCB_LYT_ANYWHERE;
			if (lloc == 0) {
				pcb_message(PCB_MSG_ERROR, "Invalid location: '%s'\n", sloc);
				PCB_ACT_IRES(-1);
				return 0;
			}
		}
	}

	pcb_layergrp_inhibit_inc();
	g = pcb_get_grp_new_misc(PCB);
	if (g != NULL) {
		pcb_layer_id_t lid;

		if (spurp != NULL)
			pcb_layergrp_set_purpose__(g, pcb_strdup(spurp), 1);

		free(g->name);
		if (name != NULL)
			g->name = pcb_strdup(name);
		else
			g->name = pcb_strdup_printf("%s%s%s", sloc == NULL ? "" : sloc, sloc == NULL ? "" : "-", stype);
		g->ltype = ltype | lloc;
		g->vis = 1;
		g->open = 1;

		pcb_undo_freeze_serial();
		pcb_layergrp_undoable_created(g);

		if (attr != NULL) {
			char *attrs = pcb_strdup(attr), *curr, *next, *val;
			for(curr = attrs; curr != NULL; curr = next) {
				next = strchr(curr, ';');
				if (next != NULL) {
					*next = '\0';
					next++;
				}
				while(isspace(*curr)) curr++;
				val = strchr(curr, '=');
				if (val != NULL) {
					*val = '\0';
					val++;
				}
				else
					val = "";
				pcb_attribute_put(&g->Attributes, curr, val);
			}
			free(attrs);
		}

		lid = pcb_layer_create(PCB, g - PCB->LayerGroups.grp, stype, 1);
		pcb_undo_unfreeze_serial();
		pcb_undo_inc_serial();
		if (lid >= 0) {
			pcb_layer_t *ly;
			PCB_ACT_IRES(0);
			ly = &PCB->Data->Layer[lid];
			ly->meta.real.vis = 1;
			if (scomb != NULL) {
				if (strstr(scomb, "auto") != NULL) ly->comb |= PCB_LYC_AUTO;
				if (strstr(scomb, "sub") != NULL)  ly->comb |= PCB_LYC_SUB;
			}
			if (name != NULL) {
				free((char *)ly->name);
				ly->name = pcb_strdup(name);
			}
		}
		else
			PCB_ACT_IRES(-1);

	}
	else
		PCB_ACT_IRES(-1);
	pcb_layergrp_inhibit_dec();

	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
	if ((pcb_gui != NULL) && (pcb_exporter == NULL))
		pcb_gui->invalidate_all(pcb_gui);

	return 0;
}

static const char pcb_acts_DupGroup[] = "DupGroup([@group])";
static const char pcb_acth_DupGroup[] = "Duplicate a layer group; if the first argument is not specified, the current group is duplicated";
static fgw_error_t pcb_act_DupGroup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name = NULL;
	pcb_layergrp_t *g = NULL;
	pcb_layergrp_id_t gid, ng;

	if (PCB_CURRLAYER(PCB_ACT_BOARD) != NULL)
		g = pcb_get_layergrp(PCB, PCB_CURRLAYER(PCB_ACT_BOARD)->meta.real.grp);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DupGroup, name = argv[1].val.str);
	if (*name == '@') {
		gid = pcb_actd_EditGroup_gid;
		if (name[1] != '\0')
			gid = pcb_layergrp_by_name(PCB, name+1);
		if (gid < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't find layer group named %s\n", name+1);
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else {
		if (g == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can't find layer group\n");
			PCB_ACT_IRES(1);
			return 0;
		}
		gid = pcb_layergrp_id(PCB, g);
	}

	pcb_layergrp_inhibit_inc();
	pcb_undo_freeze_serial();
	ng = pcb_layergrp_dup(PCB, gid, 1, 1);
	if (ng >= 0) {
		pcb_layer_id_t lid = pcb_layer_create(PCB, ng, g->name, 1);
		if (lid >= 0) {
			PCB_ACT_IRES(0);
			PCB->Data->Layer[lid].meta.real.vis = 1;
		}
		else
			PCB_ACT_IRES(-1);
	}
	else
		PCB_ACT_IRES(-1);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();
	pcb_layergrp_inhibit_dec();

	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERS_CHANGED, NULL);
	if ((pcb_gui != NULL) && (pcb_exporter == NULL))
		pcb_gui->invalidate_all(pcb_gui);
	return 0;
}

const char pcb_acts_selectlayer[] = "SelectLayer(1..MAXLAYER|Silk|Rats)";
const char pcb_acth_selectlayer[] = "Select which layer is the current layer.";
/* DOC: selectlayer.html */
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
			pcb_layervis_change_group_vis(PCB_ACT_HIDLIB, lid, 1, 1);
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
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
		return 0;
	}

	PCB->RatDraw = 0;
	pcb_layervis_change_group_vis(PCB_ACT_HIDLIB, atoi(name)-1, 1, 1);
	pcb_gui->invalidate_all(pcb_gui);
	pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	return 0;
}

const char pcb_acts_chklayer[] = "ChkLayer(layerid)";
const char pcb_acth_chklayer[] = "Returns 1 if the specified layer is the active layer";
/* DOC: chklayer.html */
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

	/* if any virtual is selected, do not accept PCB_CURRLAYER(PCB_ACT_BOARD) as selected */
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
		PCB_ACT_IRES(ly == PCB_CURRLAYER(PCB_ACT_BOARD));
	return 0;
}


const char pcb_acts_toggleview[] = "ToggleView(1..MAXLAYER)\n" "ToggleView(layername)\n" "ToggleView(Silk|Rats|Pins|Vias|BackSide)\n" "ToggleView(All, Open|Vis, Set|Clear|Toggle)";
const char pcb_acth_toggleview[] = "Toggle the visibility of the specified layer or layer group.";
/* DOC: toggleview.html */
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
		pcb_gui->invalidate_all(pcb_gui);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
		return 0;
	}
	else if (pcb_strcasecmp(name, "silk") == 0) {
		if (pcb_layer_list(PCB, PCB_LYT_VISIBLE_SIDE() | PCB_LYT_SILK, &lid, 1) > 0)
			pcb_layervis_change_group_vis(PCB_ACT_HIDLIB, lid, -1, 0);
		else
			pcb_message(PCB_MSG_ERROR, "Can't find this-side silk layer\n");
	}
	else if ((pcb_strcasecmp(name, "padstacks") == 0) || (pcb_strcasecmp(name, "vias") == 0) || (pcb_strcasecmp(name, "pins") == 0) || (pcb_strcasecmp(name, "pads") == 0)) {
		PCB->pstk_on = !PCB->pstk_on;
		pcb_gui->invalidate_all(pcb_gui);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else if (pcb_strcasecmp(name, "BackSide") == 0) {
		PCB->InvisibleObjectsOn = !PCB->InvisibleObjectsOn;
		pcb_gui->invalidate_all(pcb_gui);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else if (strncmp(name, "ui:", 3) == 0) {
		pcb_layer_t *ly = pcb_uilayer_get(atoi(name+3));
		if (ly == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid ui layer id: '%s'\n", name);
			PCB_ACT_IRES(-1);
			return 0;
		}
		ly->meta.real.vis = !ly->meta.real.vis;
		pcb_gui->invalidate_all(pcb_gui);
		pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else {
		char *end;
		int id = strtol(name, &end, 10) - 1;
		if (*end == '\0') { /* integer layer */
			pcb_layervis_change_group_vis(PCB_ACT_HIDLIB, id, -1, 0);
			pcb_gui->invalidate_all(pcb_gui);
			pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
			return 0;
		}
		else { /* virtual menu layer */
			const pcb_menu_layers_t *ml = pcb_menu_layer_find(name);
			if (ml != NULL) {
				pcb_bool *v = (pcb_bool *)((char *)PCB + ml->vis_offs);
				*v = !(*v);
				pcb_gui->invalidate_all(pcb_gui);
				pcb_event(PCB_ACT_HIDLIB, PCB_EVENT_LAYERVIS_CHANGED, NULL);
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
/* DOC: chkview.html */
static fgw_error_t pcb_act_ChkView(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layer_id_t lid;
	pcb_layer_t *ly;
	char *end;
	const char *name;

	PCB_ACT_CONVARG(1, FGW_STR, chkview, name = argv[1].val.str);

	if (strncmp(name, "ui:", 3) == 0) {
		pcb_layer_t *ly = pcb_uilayer_get(atoi(name+3));
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
			PCB_ACT_IRES(*v);
			return 0;
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


static int layer_get_curr_pos(pcb_board_t *pcb, pcb_layer_id_t *lidout, pcb_layergrp_id_t *gid, pcb_layergrp_t **grp, int *glidx)
{
	pcb_layergrp_t *g;
	pcb_layer_id_t lid;
	int n;

	lid = PCB_CURRLID(pcb);
	if (lid == -1)
		return -1;

	*gid = pcb_layer_get_group(pcb, lid);
	g = pcb_get_layergrp(pcb, *gid);
	if (g == NULL)
		return -1;

	for(n = 0; n < g->len; n++) {
		if (g->lid[n] == lid) {
			*lidout = lid;
			*glidx = n;
			*grp = g;
			return 0;
		}
	}

	return -1;
}

static int layer_select_delta(pcb_board_t *pcb, int d)
{
	pcb_layergrp_t *g;
	pcb_layer_id_t lid;
	pcb_layergrp_id_t gid;
	int glidx;

	if (layer_get_curr_pos(pcb, &lid, &gid, &g, &glidx) < 0)
		return -1;

	if (d > 0) {
		while(d > 0) {
			glidx++;
			while(glidx >= g->len) {
				gid++;
				g = pcb_get_layergrp(pcb, gid);
				if (g == NULL)
					return -1;
				glidx = 0;
			}
			lid = g->lid[glidx];
			d--;
		}
	}
	else if (d < 0) {
		while(d < 0) {
			glidx--;
			while(glidx < 0) {
				gid--;
				g = pcb_get_layergrp(pcb, gid);
				if (g == NULL)
					return -1;
				glidx = g->len-1;
			}
			lid = g->lid[glidx];
			d++;
		}
	}
	else
		return 0; /* 0 means: stay where we are */

	pcb->RatDraw = 0;
	pcb_layervis_change_group_vis(&pcb->hidlib, lid, 1, 1);
	pcb_gui->invalidate_all(pcb_gui);
	pcb_event(&pcb->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);

	return 0;
}

static const char pcb_acts_LayerByStack[] = "LayerByStack(select, prev|next)";
static const char pcb_acth_LayerByStack[] = "Layer operations based on physical layer stacking";
/* DOC: layerbystack.html */
static fgw_error_t pcb_act_LayerByStack(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op1, op2;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, LayerByStack, op1 = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_KEYWORD, LayerByStack, op2 = fgw_keyword(&argv[2]));
	PCB_ACT_IRES(0);

	switch(op1) {
		case F_Select:
			switch(op2) {
				case F_Prev: PCB_ACT_IRES(layer_select_delta(PCB_ACT_BOARD, -1)); return 0;
				case F_Next: PCB_ACT_IRES(layer_select_delta(PCB_ACT_BOARD, +1)); return 0;
			}
	}

	PCB_ACT_FAIL(LayerByStack);
	PCB_ACT_IRES(1);
	return 0;
}



const char pcb_acts_chkrst[] = "ChkRst(route_style_id)\n";
const char pcb_acth_chkrst[] = "Return 1 if route_style_id matches pen.";
/* DOC: chkrst.html */
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

static const char pcb_acts_boardflip[] = "BoardFlip([sides])";
static const char pcb_acth_boardflip[] = "Mirror the board over the x axis, optionally mirroring sides as well.";
/* DOC: boardflip.html */
static fgw_error_t pcb_act_boardflip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = -2;
	pcb_bool smirror;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, boardflip, op = fgw_keyword(&argv[1]));

	smirror = (op == F_Sides);
	pcb_data_mirror(PCB->Data, 0, smirror ? PCB_TXM_SIDE : PCB_TXM_COORD, smirror, 1);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ClipInhibit[] = "ClipInhibit([on|off|check])";
static const char pcb_acth_ClipInhibit[] = "ClipInhibit Feature Template.";
static fgw_error_t pcb_act_ClipInhibit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static int is_on = 0;
	int target;
	const char *op;
	char tmp[2];

	PCB_ACT_CONVARG(1, FGW_STR, ClipInhibit, op = argv[1].val.str);

	if (strcmp(op, "check") == 0) {
		PCB_ACT_IRES(is_on);
		return 0;
	}


	if (strcmp(op, "toggle") == 0) {
		target = !is_on;
	}
	else {
		if (op[0] != 'o')
			PCB_ACT_FAIL(ClipInhibit);

		switch(op[1]) {
			case 'n': target = 1; break;
			case 'f': target = 0; break;
			default: PCB_ACT_FAIL(ClipInhibit);
		}
	}

	if (is_on == target) {
		PCB_ACT_IRES(0);
		return 0;
	}

	is_on = target;
	if (is_on)
		pcb_data_clip_inhibit_inc(PCB->Data);
	else
		pcb_data_clip_inhibit_dec(PCB->Data, 1);

	tmp[0] = '0' + !conf_core.temp.clip_inhibit_chg;
	tmp[1] = '\0';
	pcb_conf_set(CFR_CLI, "temp/clip_inhibit_chg", 0, tmp, POL_OVERWRITE);


	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LayerVisReset[] = "LayerVisReset()";
static const char pcb_acth_LayerVisReset[] = "Reset layer visibility to safe defaults.";
static fgw_error_t pcb_act_LayerVisReset(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layervis_reset_stack(&PCB->hidlib);
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t gui_action_list[] = {
	{"Display", pcb_act_Display, pcb_acth_Display, pcb_acts_Display},
	{"CycleDrag", pcb_act_CycleDrag, pcb_acth_CycleDrag, pcb_acts_CycleDrag},
	{"MarkCrosshair", pcb_act_MarkCrosshair, pcb_acth_MarkCrosshair, pcb_acts_MarkCrosshair},
	{"SetSame", pcb_act_SetSame, pcb_acth_SetSame, pcb_acts_SetSame},
	{"RouteStyle", pcb_act_RouteStyle, pcb_acth_RouteStyle, pcb_acts_RouteStyle},
	{"SelectLayer", pcb_act_SelectLayer, pcb_acth_selectlayer, pcb_acts_selectlayer},
	{"ChkLayer", pcb_act_ChkLayer, pcb_acth_chklayer, pcb_acts_chklayer},
	{"ToggleView", pcb_act_ToggleView, pcb_acth_toggleview, pcb_acts_toggleview},
	{"ChkView", pcb_act_ChkView, pcb_acth_chkview, pcb_acts_chkview},
	{"LayerByStack", pcb_act_LayerByStack, pcb_acth_LayerByStack, pcb_acts_LayerByStack},
	{"EditLayer", pcb_act_EditLayer, pcb_acth_EditLayer, pcb_acts_EditLayer},
	{"EditGroup", pcb_act_EditGroup, pcb_acth_EditGroup, pcb_acts_EditGroup},
	{"DelGroup",  pcb_act_DelGroup, pcb_acth_DelGroup, pcb_acts_DelGroup},
	{"DupGroup",  pcb_act_DupGroup, pcb_acth_DupGroup, pcb_acts_DupGroup},
	{"NewGroup",  pcb_act_NewGroup, pcb_acth_NewGroup, pcb_acts_NewGroup},
	{"ChkRst", pcb_act_ChkRst, pcb_acth_chkrst, pcb_acts_chkrst},
	{"BoardFlip", pcb_act_boardflip, pcb_acth_boardflip, pcb_acts_boardflip},
	{"ClipInhibit", pcb_act_ClipInhibit, pcb_acth_ClipInhibit, pcb_acts_ClipInhibit},
	{"LayerVisReset", pcb_act_LayerVisReset, pcb_acth_LayerVisReset, pcb_acts_LayerVisReset}
};

void pcb_gui_act_init2(void)
{
	PCB_REGISTER_ACTIONS(gui_action_list, NULL);
}
