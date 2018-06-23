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

#include "config.h"

#include "action_helper.h"
#include "plugins.h"
#include "actions.h"


const char *pcb_sketch_route_cookie = "sketch_route plugin";

static const char pcb_acts_skroute[] = "skroute(TODO)";
static const char pcb_acth_skroute[] = "TODO";
fgw_error_t pcb_act_skroute(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t sketch_route_action_list[] = {
	{"skroute", pcb_act_skroute, pcb_acth_skroute, pcb_acts_skroute}
};

PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

int pplg_check_ver_sketch_route(int ver_needed) { return 0; }

void pplg_uninit_sketch_route(void)
{
	pcb_remove_actions_by_cookie(pcb_sketch_route_cookie);
}


#include "dolists.h"
int pplg_init_sketch_route(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

	return 0;
}
