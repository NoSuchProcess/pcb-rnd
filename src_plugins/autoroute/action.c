/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "autoroute.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/event.h>
#include "funchash_core.h"

static const char pcb_acts_AutoRoute[] = "AutoRoute(AllRats|SelectedRats)";
static const char pcb_acth_AutoRoute[] = "Auto-route some or all rat lines.";
/* DOC: autoroute.html */
static fgw_error_t pcb_act_AutoRoute(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	RND_ACT_CONVARG(1, FGW_KEYWORD, AutoRoute, op = fgw_keyword(&argv[1]));

	rnd_hid_busy(PCB, 1);
	switch(op) {
		case F_AllRats:
		case F_All:
			if (AutoRoute(rnd_false))
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (AutoRoute(rnd_true))
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;
		default:
			RND_ACT_FAIL(AutoRoute);
			return 1;
	}
	rnd_hid_busy(PCB, 0);
	RND_ACT_IRES(0);
	return 0;
}

static const char *autoroute_cookie = "autoroute plugin";

rnd_action_t autoroute_action_list[] = {
	{"AutoRoute", pcb_act_AutoRoute, pcb_acth_AutoRoute, pcb_acts_AutoRoute},
};

int pplg_check_ver_autoroute(int ver_needed) { return 0; }

void pplg_uninit_autoroute(void)
{
	rnd_remove_actions_by_cookie(autoroute_cookie);
}

int pplg_init_autoroute(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(autoroute_action_list, autoroute_cookie)
	return 0;
}
