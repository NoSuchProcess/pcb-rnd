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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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

pcb_tlp_session_t ctx;

static const char pcb_acts_mill[] = "mill()";
static const char pcb_acth_mill[] = "Calculate toolpath for milling away copper";
int pcb_act_mill(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_tlp_mill_copper_layer(&ctx, CURRENT);
}


pcb_hid_action_t millpath_action_list[] = {
	{"mill", 0, pcb_act_mill, pcb_acth_mill, pcb_acts_mill}
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
	PCB_REGISTER_ACTIONS(millpath_action_list, pcb_millpath_cookie)
	return 0;
}
