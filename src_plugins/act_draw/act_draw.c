/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "conf_core.h"

#include "actions.h"
#include "board.h"
#include "flag_str.h"
#include "obj_line.h"
#include "plugins.h"

static const char pcb_acts_GetValue[] = "GetValue(input, units, relative, default_unit)";
static const char pcb_acth_GetValue[] = "Convert a coordinate value. Returns an unitless double or FGW_ERR_ARG_CONV. The 3rd parameter controls whether to require relative coordinates (+- prefix). Wraps pcb_get_value_ex().";
static fgw_error_t pcb_act_GetValue(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *input, *units, *def_unit;
	int relative, a;
	double v;
	pcb_bool success;

	PCB_ACT_CONVARG(1, FGW_STR, GetValue, input = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, GetValue, units = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_INT, GetValue, relative = argv[3].val.nat_int);
	PCB_ACT_CONVARG(4, FGW_STR, GetValue, def_unit = argv[1].val.str);

	if (*units == '\0')
		units = NULL;

	v = pcb_get_value_ex(input, units, &a, NULL, def_unit, &success);
	if (!success || (relative && a))
		return FGW_ERR_ARG_CONV;

	res->type = FGW_DOUBLE;
	res->val.nat_double = v;
	return 0;
}

static int flg_error(const char *msg)
{
	pcb_message(PCB_MSG_ERROR, "act_draw flag conversion error: %s\n", msg);
}

static const char pcb_acts_LineNew[] = "LineNew(data, layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags)";
static const char pcb_acth_LineNew[] = "Create a pcb line segment on a layer. For now data must be \"pcb\". Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_LineNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_line_t *line;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t x1, y1, x2, y2, th, cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, LineNew, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, LineNew, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, LineNew, x1 = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, LineNew, y1 = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, LineNew, x2 = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_COORD, LineNew, y2 = fgw_coord(&argv[6]));
	PCB_ACT_CONVARG(7, FGW_COORD, LineNew, th = fgw_coord(&argv[7]));
	PCB_ACT_CONVARG(8, FGW_COORD, LineNew, cl = fgw_coord(&argv[8]));
	PCB_ACT_CONVARG(9, FGW_STR, LineNew, sflg = argv[9].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	line = pcb_line_new(layer, x1, y1, x2, y2, th, cl*2, flags);

	if (line != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = line->ID;
	}
	return 0;
}


pcb_action_t act_draw_action_list[] = {
	{"GetValue", pcb_act_GetValue, pcb_acth_GetValue, pcb_acts_GetValue},
	{"LineNew", pcb_act_LineNew, pcb_acth_LineNew, pcb_acts_LineNew}
};

static const char *act_draw_cookie = "act_draw";

PCB_REGISTER_ACTIONS(act_draw_action_list, act_draw_cookie)

int pplg_check_ver_act_draw(int ver_needed) { return 0; }

void pplg_uninit_act_draw(void)
{
	pcb_remove_actions_by_cookie(act_draw_cookie);
}

#include "dolists.h"
int pplg_init_act_draw(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(act_draw_action_list, act_draw_cookie)
	return 0;
}
