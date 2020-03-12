/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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
#include <librnd/config.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/tool.h>
#include <librnd/core/grid.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>
#include <librnd/core/attrib.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/tool.h>

static const char pcb_acts_CreateMenu[] = "CreateMenu(path)\nCreateMenu(path, action, tooltip, cookie)";
static const char pcb_acth_CreateMenu[] = "Creates a new menu, popup (only path specified) or submenu (at least path and action are specified)";
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

	if (argc > 1) {
		pcb_menu_prop_t props;

		memset(&props, 0, sizeof(props));
		props.action = (argc > 2) ? argv[2].val.str : NULL;
		props.tip = (argc > 3) ? argv[3].val.str : NULL;
		props.cookie = (argc > 4) ? argv[4].val.str : NULL;

		pcb_gui->create_menu(pcb_gui, argv[1].val.str, &props);

		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_FAIL(CreateMenu);
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RemoveMenu[] = "RemoveMenu(path|cookie)";
static const char pcb_acth_RemoveMenu[] = "Recursively removes a new menu, popup (only path specified) or submenu. ";
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
	if (pcb_gui->remove_menu(pcb_gui, argv[1].val.str) != 0) {
		pcb_message(PCB_MSG_ERROR, "failed to remove some of the menu items\n");
		PCB_ACT_IRES(-1);
	}
	else
		PCB_ACT_IRES(0);
	return 0;
}

/* This action is provided for CLI convenience */
static const char pcb_acts_FullScreen[] = "FullScreen(on|off|toggle)\n";
static const char pcb_acth_FullScreen[] = "Hide widgets to get edit area full screen";

static fgw_error_t pcb_act_FullScreen(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, FullScreen, cmd = argv[1].val.str);

	if ((cmd == NULL) || (pcb_strcasecmp(cmd, "Toggle") == 0))
		pcb_conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !pcbhl_conf.editor.fullscreen, POL_OVERWRITE);
	else if (pcb_strcasecmp(cmd, "On") == 0)
		pcb_conf_set(CFR_DESIGN, "editor/fullscreen", -1, "1", POL_OVERWRITE);
	else if (pcb_strcasecmp(cmd, "Off") == 0)
		pcb_conf_set(CFR_DESIGN, "editor/fullscreen", -1, "0", POL_OVERWRITE);
	else
		PCB_ACT_FAIL(FullScreen);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Cursor[] = "Cursor(Type,DeltaUp,DeltaRight,Units)";
static const char pcb_acth_Cursor[] = "Move the cursor.";
/* DOC: cursor.html */
static fgw_error_t pcb_act_Cursor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hidlib_t *hidlib = PCB_ACT_HIDLIB;
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
	const char *a1, *a2, *a3, *op;
	pcb_box_t vbx;

	extra_units_x[0].scale = PCB_ACT_HIDLIB->grid;
	extra_units_x[2].scale = PCB_ACT_HIDLIB->size_x;

	extra_units_y[0].scale = PCB_ACT_HIDLIB->grid;
	extra_units_y[2].scale = PCB_ACT_HIDLIB->size_y;

	pcb_gui->view_get(pcb_gui, &vbx);
	view_width = vbx.X2 - vbx.X1;
	view_height = vbx.Y2 - vbx.Y1;

	extra_units_x[1].scale = view_width;
	extra_units_y[1].scale = view_height;

	PCB_ACT_CONVARG(1, FGW_STR, Cursor, op = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, Cursor, a1 = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, Cursor, a2 = argv[3].val.str);
	PCB_ACT_CONVARG(4, FGW_STR, Cursor, a3 = argv[4].val.str);

	switch(*op) {
		case 'p': case 'P': /* Pan */
			pan_warp = HID_SC_PAN_VIEWPORT;
			break;
		case 'w': case 'W': /* Warp */
			pan_warp = HID_SC_WARP_POINTER;
			break;
		default:
			PCB_ACT_FAIL(Cursor);
	}

	if (pcb_strcasecmp(a3, "grid") == 0) {
		char *end;
		dx = strtod(a1, &end) * pcbhl_conf.editor.grid;
		dy = strtod(a2, &end) * pcbhl_conf.editor.grid;
	}
	else {
		dx = pcb_get_value_ex(a1, a3, NULL, extra_units_x, "", NULL);
		dy = pcb_get_value_ex(a2, a3, NULL, extra_units_y, "", NULL);
	}

	if (pcbhl_conf.editor.view.flip_x)
		dx = -dx;
	if (!pcbhl_conf.editor.view.flip_y)
		dy = -dy;

	/* Allow leaving snapped pin/pad/padstack */
	if (hidlib->tool_snapped_obj_bbox) {
		pcb_box_t *bbx = hidlib->tool_snapped_obj_bbox;
		pcb_coord_t radius = ((bbx->X2 - bbx->X1) + (bbx->Y2 - bbx->Y1))/6;
		if (dx < 0)
			dx -= radius;
		else if (dx > 0)
			dx += radius;
		if (dy < 0)
			dy -= radius;
		else if (dy > 0)
			dy += radius;
	}

	pcb_hidcore_crosshair_move_to(hidlib, hidlib->ch_x + dx, hidlib->ch_y + dy, 1);
	pcb_gui->set_crosshair(pcb_gui, hidlib->ch_x, hidlib->ch_y, pan_warp);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MoveCursorTo[] = "MoveCursorTo(x,y)";
static const char pcb_acth_MoveCursorTo[] = "Move the cursor to absolute coords, pan the view as needed.";
static fgw_error_t pcb_act_MoveCursorTo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hidlib_t *hidlib = PCB_ACT_HIDLIB;
	pcb_coord_t x, y;

	PCB_ACT_CONVARG(1, FGW_COORD, Cursor, x = fgw_coord(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_COORD, Cursor, y = fgw_coord(&argv[2]));

	pcb_hidcore_crosshair_move_to(PCB_ACT_HIDLIB, x, y, 0);
	pcb_gui->set_crosshair(pcb_gui, hidlib->ch_x, hidlib->ch_y, HID_SC_PAN_VIEWPORT);

	PCB_ACT_IRES(0);
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
		pcb_grid_set(PCB_ACT_HIDLIB, &dst);
		pcb_grid_free(&dst);
	}
	else if ((strcmp(op, "up") == 0) || (strcmp(op, "+") == 0))
		pcb_grid_list_step(PCB_ACT_HIDLIB, +1);
	else if ((strcmp(op, "down") == 0) || (strcmp(op, "-") == 0))
		pcb_grid_list_step(PCB_ACT_HIDLIB, -1);
	else if (strcmp(op, "idx") == 0) {
		PCB_ACT_CONVARG(2, FGW_STR, grid, a = argv[2].val.str);
		pcb_grid_list_jump(PCB_ACT_HIDLIB, atoi(a));
	}
	else if (op[0] == '#') {
		pcb_grid_list_jump(PCB_ACT_HIDLIB, atoi(op+1));
	}
	else
		PCB_ACT_FAIL(grid);

	return 0;
}

static const char pcb_acts_GetXY[] = "GetXY([message, [x|y]])";
static const char pcb_acth_GetXY[] = "Get a coordinate. If x or y specified, the return value of the action is the x or y coordinate.";
/* DOC: getxy.html */
static fgw_error_t pcb_act_GetXY(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y;
	const char *op = NULL, *msg = "Click to enter a coordinate.";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, GetXY, msg = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, GetXY, op = argv[2].val.str);

	pcb_hid_get_coords(msg, &x, &y, 0);

	PCB_ACT_IRES(0);
	if (op != NULL) {
		if (((op[0] == 'x') || (op[0] == 'X')) && op[1] == '\0') {
			res->type = FGW_COORD;
			fgw_coord(res) = x;
		}
		else if (((op[0] == 'y') || (op[0] == 'Y')) && op[1] == '\0') {
			res->type = FGW_COORD;
			fgw_coord(res) = y;
		}
		else
			PCB_ACT_FAIL(GetXY);
	}

	return 0;
}

static const char pcb_acts_Benchmark[] = "Benchmark()";
static const char pcb_acth_Benchmark[] = "Benchmark the GUI speed.";
/* DOC: benchmark.html */
static fgw_error_t pcb_act_Benchmark(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	double fps = 0;

	if ((pcb_gui != NULL) && (pcb_gui->benchmark != NULL)) {
		fps = pcb_gui->benchmark(pcb_gui);
		pcb_message(PCB_MSG_INFO, "%f redraws per second\n", fps);
	}
	else
		pcb_message(PCB_MSG_ERROR, "benchmark is not available in the current HID\n");

	PCB_ACT_DRES(fps);
	return 0;
}

static const char pcb_acts_Help[] = "Help()";
static const char pcb_acth_Help[] = "On-line action help";
static fgw_error_t pcb_act_Help(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_print_actions();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Redraw[] = "Redraw()";
static const char pcb_acth_Redraw[] = "Redraw the entire screen";
static fgw_error_t pcb_act_Redraw(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_gui->invalidate_all(pcb_gui);
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t rnd_gui_action_list[] = {
	{"FullScreen", pcb_act_FullScreen, pcb_acth_FullScreen, pcb_acts_FullScreen},
	{"CreateMenu", pcb_act_CreateMenu, pcb_acth_CreateMenu, pcb_acts_CreateMenu},
	{"RemoveMenu", pcb_act_RemoveMenu, pcb_acth_RemoveMenu, pcb_acts_RemoveMenu},
	{"Cursor", pcb_act_Cursor, pcb_acth_Cursor, pcb_acts_Cursor},
	{"MoveCursorTo", pcb_act_MoveCursorTo, pcb_acth_MoveCursorTo, pcb_acts_MoveCursorTo},
	{"Grid", pcb_act_grid, pcb_acth_grid, pcb_acts_grid},
	{"GetXY", pcb_act_GetXY, pcb_acth_GetXY, pcb_acts_GetXY},
	{"Benchmark", pcb_act_Benchmark, pcb_acth_Benchmark, pcb_acts_Benchmark},
	{"Help", pcb_act_Help, pcb_acth_Help, pcb_acts_Help},
	{"Redraw", pcb_act_Redraw, pcb_acth_Redraw, pcb_acts_Redraw}
};

void rnd_gui_act_init2(void)
{
	PCB_REGISTER_ACTIONS(rnd_gui_action_list, NULL);
}
