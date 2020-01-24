/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "toolpath.h"

#include "board.h"
#include "data.h"
#include "layer.h"

const char *pcb_millpath_cookie = "millpath plugin";

static pcb_tlp_session_t ctx;
static pcb_coord_t tool_dias[] = {
	PCB_MM_TO_COORD(0.5),
	PCB_MM_TO_COORD(3)
};
static pcb_tlp_tools_t tools = { sizeof(tool_dias)/sizeof(tool_dias[0]), tool_dias};

static const char pcb_acts_mill[] = "mill([script])";
static const char pcb_acth_mill[] = "Calculate toolpath for milling away copper";
fgw_error_t pcb_act_mill(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *script = NULL;
	pcb_board_t *pcb = (pcb_board_t *)PCB_ACT_HIDLIB;
	ctx.edge_clearance = PCB_MM_TO_COORD(0.05);
	ctx.tools = &tools;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, mill, script = argv[1].val.str);

	if (script == NULL)
		PCB_ACT_IRES(pcb_tlp_mill_copper_layer(pcb, &ctx, pcb_get_layergrp(pcb, PCB_CURRLAYER(PCB)->meta.real.grp)));
	else
		PCB_ACT_IRES(pcb_tlp_mill_script(pcb, &ctx, pcb_get_layergrp(pcb, PCB_CURRLAYER(PCB)->meta.real.grp), script));

	return 0;
}


pcb_action_t millpath_action_list[] = {
	{"mill", pcb_act_mill, pcb_acth_mill, pcb_acts_mill}
};

int pplg_check_ver_millpath(int ver_needed) { return 0; }

void pplg_uninit_millpath(void)
{
	pcb_remove_actions_by_cookie(pcb_millpath_cookie);
}


int pplg_init_millpath(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(millpath_action_list, pcb_millpath_cookie)
	return 0;
}
