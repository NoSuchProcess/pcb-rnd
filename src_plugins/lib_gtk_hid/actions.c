/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include "gui.h"

#include "actions.h"
#include "search.h"
#include "../src/actions.h"
#include "compat_misc.h"

static const char *ghid_act_cookie = "gtk HID actions";

static const char pcb_acts_DoWindows[] =
	"DoWindows(1|2|3|4|5|6|7 [,false])\n"
	"DoWindows(Layout|Library|Log|Search [,false])";
static const char pcb_acth_DoWindows[] = "Open various GUI windows. With false, do not raise the window (no focus stealing).";
/* DOC: dowindows.html */
static fgw_error_t pcb_act_DoWindows(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a = "";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DoWindows, a = argv[1].val.str);

	if (strcmp(a, "1") == 0 || pcb_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || pcb_strcasecmp(a, "Library") == 0) {
		pcb_message(PCB_MSG_ERROR, "Please use the new LibraryDialog() action instead\n");
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		pcb_actionl("LogDialog", NULL);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		pcb_actionl("NetlistDialog", NULL);
	}
	else if (strcmp(a, "5") == 0 || pcb_strcasecmp(a, "Preferences") == 0) {
		pcb_message(PCB_MSG_ERROR, "Please use the new drc preferences() action instead\n");
	}
	else if (strcmp(a, "6") == 0 || pcb_strcasecmp(a, "DRC") == 0) {
		pcb_message(PCB_MSG_ERROR, "Please use the new drc action instead\n");
	}
	else if (strcmp(a, "7") == 0 || pcb_strcasecmp(a, "Search") == 0) {
		pcb_actionl("SearchDialog", NULL);
	}
	else {
		PCB_ACT_FAIL(DoWindows);
	}

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t ghid_main_action_list[] = {
	{"DoWindows", pcb_act_DoWindows, pcb_acth_DoWindows, pcb_acts_DoWindows},
};

PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
#include "dolists.h"

void pcb_gtk_action_reg(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
}

void pcb_gtk_action_unreg(void)
{
	pcb_remove_actions_by_cookie(ghid_act_cookie);
	pcb_hid_remove_attributes_by_cookie(ghid_act_cookie);
}
