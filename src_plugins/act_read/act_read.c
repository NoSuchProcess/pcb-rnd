/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "board.h"
#include "data_parent.h"
#include <librnd/core/actions.h>
#include "crosshair.h"
#include <librnd/core/plugins.h>
#include <librnd/core/misc_util.h>
#include "idpath.h"
#include "obj_subc.h"

#include "keywords_sphash.h"

#define PCB (do_not_use_PCB)

#include "act_idpath.c"
#include "act_geo.c"
#include "act_layer.c"

static const char pcb_acts_GetValue[] = "GetValue(input, units, relative, default_unit)";
static const char pcb_acth_GetValue[] = "Convert a coordinate value. Returns an unitless double or FGW_ERR_ARG_CONV. The 3rd parameter controls whether to require relative coordinates (+- prefix). Wraps rnd_get_value_ex().";
static fgw_error_t pcb_act_GetValue(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *input, *units, *def_unit;
	int relative, a;
	double v;
	rnd_bool success;

	RND_ACT_CONVARG(1, FGW_STR, GetValue, input = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, GetValue, units = argv[2].val.str);
	RND_ACT_CONVARG(3, FGW_INT, GetValue, relative = argv[3].val.nat_int);
	RND_ACT_CONVARG(4, FGW_STR, GetValue, def_unit = argv[1].val.str);

	if (*units == '\0')
		units = NULL;

	v = rnd_get_value_ex(input, units, &a, NULL, def_unit, &success);
	if (!success || (relative && a))
		return FGW_ERR_ARG_CONV;

	res->type = FGW_DOUBLE;
	res->val.nat_double = v;
	return 0;
}

static const char pcb_acts_GetMark[] = "GetMark(active|user_placed|x|y)";
static const char pcb_acth_GetMark[] = "Return mark properties in numeric form.";
static fgw_error_t pcb_act_GetMark(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd;

	RND_ACT_CONVARG(1, FGW_STR, GetMark, cmd = argv[1].val.str);
	switch(*cmd) {
		case 'a': res->type = FGW_INT; res->val.nat_int = pcb_marked.status; break;
		case 'u': res->type = FGW_INT; res->val.nat_int = pcb_marked.user_placed; break;
		case 'x': res->type = FGW_DOUBLE; res->val.nat_double = pcb_marked.X; break;
		case 'y': res->type = FGW_DOUBLE; res->val.nat_double = pcb_marked.Y; break;
		default:
			RND_ACT_FAIL(GetMark);
	}
	return 0;
}

static const char pcb_acts_ReadBoard[] ="ReadBoard(width|size_x|height|size_y)\n";
static const char pcb_acth_ReadBoard[] = "Length returns the number of groups on the current PCB. Field returns one of the fields of the group named in groupid. Layerid returns the integer layer ID (as interpreted within data) for the idxth layer of the group.";
static fgw_error_t pcb_act_ReadBoard(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int cmd, fld;
	char *cmds, *flds, *target;
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	pcb_layer_type_t lyt;
	long idx;

	RND_ACT_CONVARG(1, FGW_STR, ReadBoard, cmds = argv[1].val.str);
	cmd = act_read_keywords_sphash(cmds);
	switch(cmd) {
		case act_read_keywords_width:
		case act_read_keywords_size_x:
			res->type = FGW_COORD; fgw_coord(res) = pcb->hidlib.size_x;
			return 0;
		case act_read_keywords_height:
		case act_read_keywords_size_y:
			res->type = FGW_COORD; fgw_coord(res) = pcb->hidlib.size_y;
			return 0;
		default:
			return FGW_ERR_ARG_CONV;
	}

	return FGW_ERR_ARG_CONV;
}


static int flg_error(const char *msg)
{
	rnd_message(RND_MSG_ERROR, "act_read flag conversion error: %s\n", msg);
	return 0;
}

#define DRAWOPTARG \
	int noundo = 0, ao = 0; \
	if ((argv[1].type == FGW_STR) && (strcmp(argv[1].val.str, "noundo") == 0)) { \
		noundo = 1; \
		ao++; \
	}


rnd_action_t act_read_action_list[] = {
	{"GetValue", pcb_act_GetValue, pcb_acth_GetValue, pcb_acts_GetValue},
	{"IDPList", pcb_act_IDPList, pcb_acth_IDPList, pcb_acts_IDPList},
	{"IDP", pcb_act_IDP, pcb_acth_IDP, pcb_acts_IDP},
	{"GetParentData", pcb_act_GetParentData, pcb_acth_GetParentData, pcb_acts_GetParentData},

	{"GetMark", pcb_act_GetMark, pcb_acth_GetMark, pcb_acts_GetMark},

	{"ReadGroup", pcb_act_ReadGroup, pcb_acth_ReadGroup, pcb_acts_ReadGroup},

	{"ReadBoard", pcb_act_ReadBoard, pcb_acth_ReadBoard, pcb_acts_ReadBoard},

	{"IsPointOnArc", pcb_act_IsPointOnArc, pcb_acth_IsPointOnArc, pcb_acts_IsPointOnArc},
	{"IsPointOnLine", pcb_act_IsPointOnLine, pcb_acth_IsPointOnLine, pcb_acts_IsPointOnLine},
	{"IntersectObjObj", pcb_act_IntersectObjObj, pcb_acth_IntersectObjObj, pcb_acts_IntersectObjObj},
	{"ObjCenter", pcb_act_ObjCenter, pcb_acth_ObjCenter, pcb_acts_ObjCenter}
};

static const char *act_read_cookie = "act_read";

int pplg_check_ver_act_read(int ver_needed) { return 0; }

void pplg_uninit_act_read(void)
{
	rnd_remove_actions_by_cookie(act_read_cookie);
}

int pplg_init_act_read(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(act_read_action_list, act_read_cookie)
	return 0;
}
