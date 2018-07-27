/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <genvector/gds_char.h>
#include <genvector/vtp0.h>

#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"
#include "event.h"
#include "actions.h"
#include "funchash_core.h"
#include "search.h"
#include "centgeo.h"
#include "misc_util.h"
#include "board.h"
#include "data_it.h"
#include "obj_line.h"
#include "undo.h"
#include "fields_sphash.h"

static const char *ddraft_cookie = "ddraft plugin";

#define EDGE_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)
#define CUT_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)

static void list_by_flag(pcb_data_t *data, vtp0_t *dst, unsigned long types, unsigned long flag)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	
	for(o = pcb_data_first(&it, data, types); o != NULL; o = pcb_data_next(&it))
		if (PCB_FLAG_TEST(flag, o))
			vtp0_append(dst, o);
}

#include "trim.c"
#include "constraint.c"

static long do_trim_split(vtp0_t *edges, int kwobj, int trim)
{
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	long res = 0;

	switch(kwobj) {
		case F_Object:
			for(;;) {
				x = PCB_MAX_COORD;
				pcb_hid_get_coords("Select an object to cut or press esc", &x, &y, 1);
				if (x == PCB_MAX_COORD)
					break;

				type = pcb_search_screen(x, y, CUT_TYPES, &ptr1, &ptr2, &ptr3);
				if (type == 0) {
					pcb_message(PCB_MSG_ERROR, "Can't cut that object\n");
					continue;
				}
				res += pcb_trim_split(edges, (pcb_any_obj_t *)ptr2, x, y, trim);
				pcb_gui->invalidate_all();
			}
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Invalid second argument\n");
			return -1;
	}

	return res;
}

static const char pcb_acts_trim_split[] = "trim([selected|found|object], [selected|found|object])\nsplit([selected|found|object], [selected|found|object])";
static const char pcb_acth_trim_split[] = "Use one or more objects as cutting edge and trim or split other objects. First argument is the cutting edge";
static fgw_error_t pcb_act_trim_split(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	int kwcut = F_Object, kwobj = F_Object;
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	vtp0_t edges;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, trim_split, kwcut = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_KEYWORD, trim_split, kwobj = fgw_keyword(&argv[2]));

	vtp0_init(&edges);

	if ((kwobj == kwcut) && (kwobj != F_Object)) {
		pcb_message(PCB_MSG_ERROR, "Both cutting edge and objects to cut can't be '%s'\n", kwcut == F_Selected ? "selected" : "found");
		goto err;
	}

	switch(kwcut) {
		case F_Object:
			pcb_hid_get_coords("Select cutting edge object", &x, &y, 0);
			type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
			if (type == 0) {
				pcb_message(PCB_MSG_ERROR, "Invalid cutting edge object\n");
				goto err;
			}
			vtp0_append(&edges, ptr2);
			break;
		case F_Selected:
			list_by_flag(PCB->Data, &edges, EDGE_TYPES, PCB_FLAG_SELECTED);
			break;
		case F_Found:
			list_by_flag(PCB->Data, &edges, EDGE_TYPES, PCB_FLAG_FOUND);
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Invalid first argument\n");
			goto err;
	}

	if (vtp0_len(&edges) < 1) {
		pcb_message(PCB_MSG_ERROR, "No cutting edge found\n");
		goto err;
	}


	if (do_trim_split(&edges, kwobj, (*actname == 't')) < 0)
		goto err;

	PCB_ACT_IRES(0);
	vtp0_uninit(&edges);
	return 0;

	err:;
	PCB_ACT_IRES(-1);
	vtp0_uninit(&edges);
	return 0;
}

#define load_arr(arr, ctr, msg, fgw_type_, fgw_val_) \
do { \
	int n; \
	if (argc-2 >= sizeof(arr) / sizeof(arr[0])) { \
		pcb_message(PCB_MSG_ERROR, "constraint: Too many " msg "\n"); \
		PCB_ACT_IRES(-1); \
		return 0; \
	} \
	ctr = 0; \
	for(n = 2; n < argc; n++) { \
		PCB_ACT_CONVARG(n, fgw_type_, constraint, arr[ctr] = fgw_val_); \
		ctr++; \
	} \
} while(0)

static const char pcb_acts_constraint[] = "constraint(type, off)\nconstraint(type, value, [value...])";
static const char pcb_acth_constraint[] = "Configure or remove a drawing constraint";
static fgw_error_t pcb_act_constraint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *stype;
	int type;

	PCB_ACT_CONVARG(1, FGW_STR, constraint, stype = argv[1].val.str);
	type = ddraft_fields_sphash(stype);
	switch(type) {
		case ddraft_fields_line_angle:
			load_arr(cons.line_angle, cons.line_angle_len, "line angles", FGW_DOUBLE, argv[n].val.nat_double);
			break;
		case ddraft_fields_move_angle:
			load_arr(cons.move_angle, cons.move_angle_len, "move angles", FGW_DOUBLE, argv[n].val.nat_double);
			break;
		case ddraft_fields_line_angle_mod:
			cons.line_angle_mod = 0;
			PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, constraint, cons.line_angle_mod = argv[2].val.nat_double);
			break;
		case ddraft_fields_move_angle_mod:
			cons.move_angle_mod = 0;
			PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, constraint, cons.move_angle_mod = argv[2].val.nat_double);
			break;
		case ddraft_fields_line_length:
			load_arr(cons.line_length, cons.line_length_len, "line lengths", FGW_COORD, fgw_coord(&argv[n]));
			break;
		case ddraft_fields_move_length:
			load_arr(cons.move_length, cons.move_length_len, "move lengths", FGW_COORD, fgw_coord(&argv[n]));
			break;
		case ddraft_fields_line_length_mod:
			cons.line_length_mod = 0;
			PCB_ACT_MAY_CONVARG(2, FGW_COORD, constraint, cons.line_length_mod = fgw_coord(&argv[2]));
			break;
		case ddraft_fields_move_length_mod:
			cons.move_length_mod = 0;
			PCB_ACT_MAY_CONVARG(2, FGW_COORD, constraint, cons.move_length_mod = fgw_coord(&argv[2]));
			break;

		case ddraft_fields_reset:
			memset(&cons, 0, sizeof(cons));
			break;
		case ddraft_fields_SPHASH_INVALID:
			pcb_message(PCB_MSG_ERROR, "constraint: invalid field '%s'\n", stype);
			PCB_ACT_IRES(-1);
			return 0;
			break;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_perp_paral[] = "perp()";
static const char pcb_acth_perp_paral[] = "Draw a line perpendicular to another line";
static fgw_error_t pcb_act_perp_paral(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	double dx, dy;
	pcb_line_t *line;

	pcb_hid_get_coords("Select target object", &x, &y, 0);
	type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);

	if (type != PCB_OBJ_LINE) {
		pcb_hid_get_coords("Select target object", &x, &y, 1);
		type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
	}

	if (type != PCB_OBJ_LINE) {
		pcb_message(PCB_MSG_ERROR, "%s: target object must be a line\n", actname);
		PCB_ACT_IRES(-1);
		return 0;
	}

	line = (pcb_line_t *)ptr2;
	dx = line->Point2.X - line->Point1.X;
	dy = line->Point2.Y - line->Point1.Y;
	if ((dx == 0.0) && (dy == 0.0)) {
		pcb_message(PCB_MSG_ERROR, "%s: target line must be longer than 0\n", actname);
		PCB_ACT_IRES(-1);
		return 0;
	}

	cons.line_angle_len = 2;
	cons.line_angle[0] = atan2(-dy, dx) * PCB_RAD_TO_DEG;
	if (actname[1] == 'e') /* perp */
		cons.line_angle[0] += 90;
	cons.line_angle[1] = fmod(cons.line_angle[0]+180, 360);

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t ddraft_action_list[] = {
	{"trim", pcb_act_trim_split, pcb_acth_trim_split, pcb_acts_trim_split},
	{"split", pcb_act_trim_split, pcb_acth_trim_split, pcb_acts_trim_split},
	{"constraint", pcb_act_constraint, pcb_acth_constraint, pcb_acts_constraint},
	{"perp", pcb_act_perp_paral, pcb_acth_perp_paral, pcb_acts_perp_paral},
	{"paral", pcb_act_perp_paral, pcb_acth_perp_paral, pcb_acts_perp_paral}
};

PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)

int pplg_check_ver_ddraft(int ver_needed) { return 0; }

void pplg_uninit_ddraft(void)
{
	pcb_event_unbind_allcookie(ddraft_cookie);
	pcb_remove_actions_by_cookie(ddraft_cookie);
}

#include "dolists.h"
int pplg_init_ddraft(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)
	pcb_event_bind(PCB_EVENT_DRAW_CROSSHAIR_CHATT, cnst_enforce, NULL, ddraft_cookie);

	return 0;
}
