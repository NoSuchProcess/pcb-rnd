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
#include "autoplace.h"
#include "plugins.h"
#include "actions.h"
#include "board.h"
#include "event.h"

static const char autoplace_syntax[] = "AutoPlaceSelected()";
static const char autoplace_help[] = "Auto-place selected components.";
/* DOC: autoplaceselected */
static fgw_error_t pcb_act_AutoPlaceSelected(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_hid_busy(PCB, 1);
	if (pcb_hid_message_box("question", "Autoplace start", "Auto-placement can NOT be undone.\nDo you want to continue anyway?", "no", 0, "yes", 1, NULL) == 1) {
		if (AutoPlaceSelected())
			pcb_board_set_changed_flag(pcb_true);
	}
	PCB_ACT_IRES(0);
	return 0;
}

static const char *autoplace_cookie = "autoplace plugin";

pcb_action_t autoplace_action_list[] = {
	{"AutoPlaceSelected", pcb_act_AutoPlaceSelected, autoplace_help, autoplace_syntax}
};

PCB_REGISTER_ACTIONS(autoplace_action_list, autoplace_cookie)

int pplg_check_ver_autoplace(int ver_needed) { return 0; }

void pplg_uninit_autoplace(void)
{
	pcb_remove_actions_by_cookie(autoplace_cookie);
}

#include "dolists.h"
int pplg_init_autoplace(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(autoplace_action_list, autoplace_cookie)
	return 0;
}
