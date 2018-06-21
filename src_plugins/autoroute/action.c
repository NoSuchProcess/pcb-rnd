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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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
#include "action_helper.h"
#include "plugins.h"
#include "actions.h"
#include "event.h"

/* action routines for the autorouter
 */

static const char pcb_acts_AutoRoute[] = "AutoRoute(AllRats|SelectedRats)";

static const char pcb_acth_AutoRoute[] = "Auto-route some or all rat lines.";

/* %start-doc actions AutoRoute

@table @code

@item AllRats
Attempt to autoroute all rats.

@item SelectedRats
Attempt to autoroute the selected rats.

@end table

Before autorouting, it's important to set up a few things.  First,
make sure any layers you aren't using are disabled, else the
autorouter may use them.  Next, make sure the current line and via
styles are set accordingly.  Last, make sure "new lines clear
polygons" is set, in case you eventually want to add a copper pour.

Autorouting takes a while.  During this time, the program may not be
responsive.

%end-doc */

static int pcb_act_AutoRoute(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	pcb_event(PCB_EVENT_BUSY, NULL);
	if (function) {								/* one parameter */
		if (strcmp(function, "AllRats") == 0) {
			if (AutoRoute(pcb_false))
				pcb_board_set_changed_flag(pcb_true);
		}
		else if ((strcmp(function, "SelectedRats") == 0) || (strcmp(function, "Selected") == 0)) {
			if (AutoRoute(pcb_true))
				pcb_board_set_changed_flag(pcb_true);
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

static const char *autoroute_cookie = "autoroute plugin";

pcb_hid_action_t autoroute_action_list[] = {
	{"AutoRoute", pcb_act_AutoRoute, pcb_acth_AutoRoute, pcb_acts_AutoRoute},
};

PCB_REGISTER_ACTIONS(autoroute_action_list, autoroute_cookie)

int pplg_check_ver_autoroute(int ver_needed) { return 0; }

void pplg_uninit_autoroute(void)
{
	pcb_remove_actions_by_cookie(autoroute_cookie);
}

#include "dolists.h"
int pplg_init_autoroute(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(autoroute_action_list, autoroute_cookie)
	return 0;
}
