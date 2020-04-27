/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "funchash_core.h"
#include "route_style.h"
#include <librnd/core/error.h>
#include <librnd/core/tool.h>

/*------------ get/chk (check flag actions for menus) ------------------*/
static const char pcb_acts_GetStyle[] = "GetStyle()" ;
static const char pcb_acth_GetStyle[] = "Return integer index (>=0) of the currently active style or -1 if no style is selected (== custom style)";
fgw_error_t pcb_act_GetStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL));
	return 0;
}

static const char pcb_acts_ChkSubcID[] = "ChkSubcID(pattern)\n";
static const char pcb_acth_ChkSubcID[] = "Return 1 if currently shown subc ID matches the requested pattern";
static fgw_error_t pcb_act_ChkSubcID(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *have = conf_core.editor.subc_id, *expected = "";

	if (have == NULL) have = "";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, ChkSubcID, expected = argv[1].val.str);

	PCB_ACT_IRES(strcmp(expected, have) == 0);
	return 0;
}

static const char pcb_acts_ChkTermID[] = "ChkTermID(pattern)\n";
static const char pcb_acth_ChkTermID[] = "Return 1 if currently shown term ID matches the requested pattern";
static fgw_error_t pcb_act_ChkTermID(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *have = conf_core.editor.term_id, *expected = "";

	if (have == NULL) have = "";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, ChkTermID, expected = argv[1].val.str);

	PCB_ACT_IRES(strcmp(expected, have) == 0);
	return 0;
}

static const char pcb_acts_ChkBuffer[] = "ChkBuffer(idx)";
static const char pcb_acth_ChkBuffer[] = "Return 1 if currently selected buffer's index matches idx";
static fgw_error_t pcb_act_ChkBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int expected;
	PCB_ACT_CONVARG(1, FGW_INT, ChkBuffer, expected = argv[1].val.nat_int);
	PCB_ACT_IRES((conf_core.editor.buffer_number + 1) == expected);
	return 0;
}

static pcb_action_t conf_action_list[] = {
	{"GetStyle", pcb_act_GetStyle, pcb_acth_GetStyle, pcb_acts_GetStyle},
	{"ChkSubcID", pcb_act_ChkSubcID, pcb_acth_ChkSubcID, pcb_acts_ChkSubcID},
	{"ChkTermID", pcb_act_ChkTermID, pcb_acth_ChkTermID, pcb_acts_ChkTermID},
	{"ChkBuffer", pcb_act_ChkBuffer, pcb_acth_ChkBuffer, pcb_acts_ChkBuffer}
};

void pcb_conf_act_init2(void)
{
	RND_REGISTER_ACTIONS(conf_action_list, NULL);
}



