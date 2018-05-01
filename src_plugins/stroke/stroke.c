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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Configurable mouse gestures using libstroke */

#include "config.h"

#include <stroke.h>

#include "action_helper.h"
#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "hid_actions.h"
#include "unit.h"
#include "plugins.h"
#include "stub_stroke.h"

#include "../src_plugins/stroke/conf_internal.c"
#include "stroke_conf.h"

#define STROKE_CONF_FN "export_xy.conf"

conf_stroke_t conf_stroke;

#define SIDE_X(x)  ((conf_core.editor.view.flip_x ? PCB->MaxWidth - (x) : (x)))
#define SIDE_Y(y)  ((conf_core.editor.view.flip_y ? PCB->MaxHeight - (y) : (y)))

static const char *pcb_stroke_cookie = "stroke plugin";

static pcb_coord_t stroke_first_x, stroke_first_y, stroke_last_x, stroke_last_y;

static int pcb_stroke_exec(const char *seq)
{
	conf_listitem_t *item;
	int idx;

	conf_loop_list(&conf_stroke.plugins.stroke.gestures, item, idx) {
		if ((strcmp(seq, item->name) == 0) && (pcb_hid_parse_actions(item->val.string[0]) == 0))
			return 0;
	}
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
static int pcb_act_stroke(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc < 1)
		PCB_ACT_FAIL(stroke);

	if (strcmp(argv[0], "gesture") == 0) {
		if (argc < 2)
			PCB_ACT_FAIL(stroke);
		pcb_stroke_exec(argv[1]);
	}
	else
		PCB_ACT_FAIL(stroke);
	return 0;
}

/*** administration ***/

pcb_hid_action_t stroke_action_list[] = {
	{"stroke", 0, pcb_act_stroke, pcb_acth_stroke, pcb_acts_stroke}
};
PCB_REGISTER_ACTIONS(stroke_action_list, pcb_stroke_cookie)


int pplg_check_ver_stroke(int ver_needed) { return 0; }

int pplg_uninit_stroke(void)
{
	conf_unreg_file(STROKE_CONF_FN, stroke_conf_internal);
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
