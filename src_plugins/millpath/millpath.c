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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "action_helper.h"
#include "plugins.h"
#include "hid_actions.h"
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

static const char pcb_acts_mill[] = "mill()";
static const char pcb_acth_mill[] = "Calculate toolpath for milling away copper";
int pcb_act_mill(int argc, const char **argv)
{
	ctx.edge_clearance = PCB_MM_TO_COORD(0.05);
	ctx.tools = &tools;
	return pcb_tlp_mill_copper_layer(&ctx, CURRENT);
}


pcb_hid_action_t millpath_action_list[] = {
	{"mill", pcb_act_mill, pcb_acth_mill, pcb_acts_mill}
};

PCB_REGISTER_ACTIONS(millpath_action_list, pcb_millpath_cookie)

int pplg_check_ver_millpath(int ver_needed) { return 0; }

void pplg_uninit_millpath(void)
{
	pcb_hid_remove_actions_by_cookie(pcb_millpath_cookie);
}


#include "dolists.h"
int pplg_init_millpath(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(millpath_action_list, pcb_millpath_cookie)
	return 0;
}
