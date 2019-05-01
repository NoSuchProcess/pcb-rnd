/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* Configurable mouse gestures using libstroke */

#include "config.h"

#include <stroke.h>

#include "board.h"
#include "conf_core.h"
#include "hidlib_conf.h"
#include "crosshair.h"
#include "actions.h"
#include "unit.h"
#include "plugins.h"
#include "stub_stroke.h"
#include "compat_misc.h"
#include "tool.h"

#include "../src_plugins/stroke/conf_internal.c"
#include "stroke_conf.h"

#define STROKE_CONF_FN "export_xy.conf"

conf_stroke_t conf_stroke;

#define SIDE_X(x)  ((pcbhl_conf.editor.view.flip_x ? PCB->hidlib.size_x - (x) : (x)))
#define SIDE_Y(y)  ((pcbhl_conf.editor.view.flip_y ? PCB->hidlib.size_y - (y) : (y)))

static const char *pcb_stroke_cookie = "stroke plugin";

static pcb_coord_t stroke_first_x, stroke_first_y, stroke_last_x, stroke_last_y;

static int pcb_stroke_exec(const char *seq)
{
	conf_listitem_t *item;
	int idx;

	conf_loop_list(&conf_stroke.plugins.stroke.gestures, item, idx) {
		if ((strcmp(seq, item->name) == 0) && (pcb_parse_actions(item->val.string[0]) == 0))
			return 0;
	}
	if (conf_stroke.plugins.stroke.warn4unknown)
		pcb_message(PCB_MSG_WARNING, "Stroke: sequence '%s' is not configured.\n", seq);
	return -1;
}

static int pcb_stroke_finish(void)
{
	char msg[255];

	pcb_mid_stroke = pcb_false;
	if (stroke_trans(msg))
		return pcb_stroke_exec(msg);
	return -1;
}

static void pcb_stroke_record(pcb_coord_t ev_x, pcb_coord_t ev_y)
{
	stroke_last_x = ev_x;
	stroke_last_y = ev_y;
	ev_x = SIDE_X(ev_x) - stroke_first_x;
	ev_y = SIDE_Y(ev_y) - stroke_first_y;
	stroke_record(ev_x / (1 << 16), ev_y / (1 << 16));
	return;
}

static void pcb_stroke_start(void)
{
	pcb_mid_stroke = pcb_true;
	stroke_first_x = SIDE_X(pcb_crosshair.X);
	stroke_first_y = SIDE_Y(pcb_crosshair.Y);
}

/*** action ***/

static const char pcb_acts_stroke[] = "stroke(gesture, seq)";
static const char pcb_acth_stroke[] = "Various gesture recognition related functions";
static fgw_error_t pcb_act_stroke(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *arg = NULL;

	PCB_ACT_CONVARG(1, FGW_STR, stroke,  cmd = argv[1].val.str);

	if (strcmp(cmd, "gesture") == 0) {
		PCB_ACT_MAY_CONVARG(2, FGW_STR, stroke,  arg = argv[2].val.str);
		if (arg == NULL)
			PCB_ACT_FAIL(stroke);
		pcb_stroke_exec(arg);
	}
	else if (pcb_strcasecmp(cmd, "zoom") == 0) {
		fgw_arg_t args[5];
		fgw_func_t *f = fgw_func_lookup(&pcb_fgw, "zoomto");

		if (f == NULL) {
			pcb_message(PCB_MSG_ERROR, "zoomto action is not available");
			PCB_ACT_IRES(1);
			return 0;
		}

		args[0].type = FGW_FUNC; args[0].val.func = f;
		args[1].type = FGW_COORD; fgw_coord(&args[1]) = stroke_first_x;
		args[2].type = FGW_COORD; fgw_coord(&args[2]) = stroke_first_y;
		args[3].type = FGW_COORD; fgw_coord(&args[3]) = stroke_last_x;
		args[4].type = FGW_COORD; fgw_coord(&args[4]) = stroke_last_y;
		return pcb_actionv_(f, res, 5, args);
	}
	else if (pcb_strcasecmp(cmd, "stopline") == 0) {
		if (conf_core.editor.mode == PCB_MODE_LINE)
			pcb_tool_select_by_id(PCB_MODE_LINE);
		else if (conf_core.editor.mode == PCB_MODE_POLYGON)
			pcb_tool_select_by_id(PCB_MODE_POLYGON);
		else if (conf_core.editor.mode == PCB_MODE_POLYGON_HOLE)
			pcb_tool_select_by_id(PCB_MODE_POLYGON_HOLE);
	}
	else
		PCB_ACT_FAIL(stroke);

	PCB_ACT_IRES(0);
	return 0;
}

/*** administration ***/

pcb_action_t stroke_action_list[] = {
	{"stroke", pcb_act_stroke, pcb_acth_stroke, pcb_acts_stroke}
};
PCB_REGISTER_ACTIONS(stroke_action_list, pcb_stroke_cookie)


int pplg_check_ver_stroke(int ver_needed) { return 0; }

int pplg_uninit_stroke(void)
{
	conf_unreg_file(STROKE_CONF_FN, stroke_conf_internal);
	conf_unreg_fields("plugins/stroke/");
	pcb_remove_actions_by_cookie(pcb_stroke_cookie);
	return 0;
}


#include "dolists.h"
int pplg_init_stroke(void)
{
	PCB_API_CHK_VER;
	stroke_init();
	conf_reg_file(STROKE_CONF_FN, stroke_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_stroke, field,isarray,type_name,cpath,cname,desc,flags);
#include "stroke_conf_fields.h"

	PCB_REGISTER_ACTIONS(stroke_action_list, pcb_stroke_cookie)

	pcb_stub_stroke_record = pcb_stroke_record;
	pcb_stub_stroke_start  = pcb_stroke_start;
	pcb_stub_stroke_finish = pcb_stroke_finish;
	return 0;
}
