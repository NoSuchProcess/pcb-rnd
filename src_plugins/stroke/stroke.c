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
#include <librnd/core/hidlib_conf.h>
#include "crosshair.h"
#include <librnd/core/actions.h>
#include <librnd/core/unit.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>
#include <librnd/core/tool.h>

#include "../src_plugins/stroke/conf_internal.c"
#include "stroke_conf.h"

#define STROKE_CONF_FN "stroke.conf"

conf_stroke_t conf_stroke;

#define SIDE_X(hl, x)  ((pcbhl_conf.editor.view.flip_x ? hl->size_x - (x) : (x)))
#define SIDE_Y(hl, y)  ((pcbhl_conf.editor.view.flip_y ? hl->size_y - (y) : (y)))

static const char *pcb_stroke_cookie = "stroke plugin";

static pcb_coord_t stroke_first_x, stroke_first_y, stroke_last_x, stroke_last_y;
static pcb_bool pcb_mid_stroke = pcb_false;

static int pcb_stroke_exec(pcb_hidlib_t *hl, const char *seq)
{
	pcb_conf_listitem_t *item;
	int idx;

	conf_loop_list(&conf_stroke.plugins.stroke.gestures, item, idx) {
		if ((strcmp(seq, item->name) == 0) && (pcb_parse_actions(hl, item->val.string[0]) == 0))
			return 1;
	}
	if (conf_stroke.plugins.stroke.warn4unknown)
		pcb_message(PCB_MSG_WARNING, "Stroke: sequence '%s' is not configured.\n", seq);
	return 0;
}

static void pcb_stroke_finish(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	char msg[255];
	int *handled = argv[1].d.p;

	if (!pcb_mid_stroke)
		return;

	pcb_mid_stroke = pcb_false;
	if (stroke_trans(msg))
		*handled = pcb_stroke_exec(hidlib, msg);
}

static void pcb_stroke_record(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_coord_t ev_x = argv[1].d.c, ev_y = argv[2].d.c;

	if (!pcb_mid_stroke)
		return;

	stroke_last_x = ev_x;
	stroke_last_y = ev_y;
	ev_x = SIDE_X(hidlib, ev_x) - stroke_first_x;
	ev_y = SIDE_Y(hidlib, ev_y) - stroke_first_y;
	stroke_record(ev_x / (1 << 16), ev_y / (1 << 16));
	return;
}

static void pcb_stroke_start(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_coord_t ev_x = argv[1].d.c, ev_y = argv[2].d.c;
	pcb_mid_stroke = pcb_true;
	stroke_first_x = SIDE_X(hidlib, ev_x);
	stroke_first_y = SIDE_Y(hidlib, ev_y);
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
		pcb_stroke_exec(PCB_ACT_HIDLIB, arg);
	}
	else if (pcb_strcasecmp(cmd, "zoom") == 0) {
		fgw_arg_t args[5];
		fgw_func_t *f = fgw_func_lookup(&pcb_fgw, "zoomto");

		if (f == NULL) {
			pcb_message(PCB_MSG_ERROR, "zoomto action is not available");
			PCB_ACT_IRES(1);
			return 0;
		}

		args[0].type = FGW_FUNC; args[0].val.argv0.func = f; args[0].val.argv0.user_call_ctx = PCB_ACT_HIDLIB;
		args[1].type = FGW_COORD; fgw_coord(&args[1]) = stroke_first_x;
		args[2].type = FGW_COORD; fgw_coord(&args[2]) = stroke_first_y;
		args[3].type = FGW_COORD; fgw_coord(&args[3]) = stroke_last_x;
		args[4].type = FGW_COORD; fgw_coord(&args[4]) = stroke_last_y;
		return pcb_actionv_(f, res, 5, args);
	}
	else if (pcb_strcasecmp(cmd, "stopline") == 0)
		pcb_actionva(PCB_ACT_HIDLIB, "Tool", "Escape", NULL);
	else
		PCB_ACT_FAIL(stroke);

	PCB_ACT_IRES(0);
	return 0;
}

/*** administration ***/

pcb_action_t stroke_action_list[] = {
	{"stroke", pcb_act_stroke, pcb_acth_stroke, pcb_acts_stroke}
};

int pplg_check_ver_stroke(int ver_needed) { return 0; }

int pplg_uninit_stroke(void)
{
	pcb_conf_unreg_file(STROKE_CONF_FN, stroke_conf_internal);
	pcb_conf_unreg_fields("plugins/stroke/");
	pcb_remove_actions_by_cookie(pcb_stroke_cookie);
	pcb_event_unbind_allcookie(pcb_stroke_cookie);
	return 0;
}

int pplg_init_stroke(void)
{
	PCB_API_CHK_VER;
	stroke_init();
	pcb_conf_reg_file(STROKE_CONF_FN, stroke_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_stroke, field,isarray,type_name,cpath,cname,desc,flags);
#include "stroke_conf_fields.h"

	PCB_REGISTER_ACTIONS(stroke_action_list, pcb_stroke_cookie)

	pcb_event_bind(PCB_EVENT_STROKE_START, pcb_stroke_start, NULL, pcb_stroke_cookie);
	pcb_event_bind(PCB_EVENT_STROKE_RECORD, pcb_stroke_record, NULL, pcb_stroke_cookie);
	pcb_event_bind(PCB_EVENT_STROKE_FINISH, pcb_stroke_finish, NULL, pcb_stroke_cookie);

	return 0;
}
