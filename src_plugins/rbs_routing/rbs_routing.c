/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
#include "data.h"

#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/hid/hid_menu.h>
#include "menu_internal.c"

static const char pcb_acts_RbsConnect[] = "RbsConnect()";
static const char pcb_acth_RbsConnect[] = "Make a new rubber band stretch connection between two points";
/* DOC: ... */
static fgw_error_t pcb_act_RbsConnect(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
}

static const char pcb_acts_RbsStretch[] = "RbsStretch()";
static const char pcb_acth_RbsStretch[] = "Make a new rubber band stretch connection between two points";
/* DOC: ... */
static fgw_error_t pcb_act_RbsStretch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
}

rnd_action_t rbs_routing_action_list[] = {
	{"RbsConnect", pcb_act_RbsConnect, pcb_acth_RbsConnect, pcb_acts_RbsConnect},
	{"RbsStretch", pcb_act_RbsStretch, pcb_acth_RbsStretch, pcb_acts_RbsStretch}
};


static const char *rbs_routing_cookie = "rbs_routing plugin";

int pplg_check_ver_rbs_routing(int ver_needed) { return 0; }

void pplg_uninit_rbs_routing(void)
{
	rnd_remove_actions_by_cookie(rbs_routing_cookie);
	rnd_hid_menu_unload(rnd_gui, rbs_routing_cookie);
}

int pplg_init_rbs_routing(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(rbs_routing_action_list, rbs_routing_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, rbs_routing_cookie, 175, NULL, 0, rbs_routing_menu, "plugin: rbs_routing");
	return 0;
}
