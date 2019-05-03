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
#include "data.h"
#include "actions.h"
#include "tool.h"
#include "remove.h"
#include "board.h"
#include "funchash_core.h"

static const char pcb_acts_Delete[] = "Delete(Object|Selected)\n" "Delete(AllRats|SelectedRats)";
static const char pcb_acth_Delete[] = "Delete stuff.";
static fgw_error_t pcb_act_Delete(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, Delete, id = fgw_keyword(&argv[1]));

	if (id == -1) { /* no arg */
		if (pcb_remove_selected(pcb_false) == pcb_false)
			id = F_Object;
	}

	switch(id) {
	case F_Object:
		pcb_hid_get_coords("Click on object to delete", &pcb_tool_note.X, &pcb_tool_note.Y, 0);
		pcb_tool_save(&PCB->hidlib);
		pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_REMOVE);
		pcb_notify_mode(&PCB->hidlib);
		pcb_tool_restore(&PCB->hidlib);
		break;
	case F_Selected:
		pcb_remove_selected(pcb_false);
		break;
	case F_AllRats:
		if (pcb_rats_destroy(pcb_false))
			pcb_board_set_changed_flag(pcb_true);
		break;
	case F_SelectedRats:
		if (pcb_rats_destroy(pcb_true))
			pcb_board_set_changed_flag(pcb_true);
		break;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_RemoveSelected[] = "pcb_remove_selected()";
static const char pcb_acth_RemoveSelected[] = "Removes any selected objects.";
static fgw_error_t pcb_act_RemoveSelected(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (pcb_remove_selected(pcb_false))
		pcb_board_set_changed_flag(pcb_true);
	PCB_ACT_IRES(0);
	return 0;
}


pcb_action_t remove_action_list[] = {
	{"Delete", pcb_act_Delete, pcb_acth_Delete, pcb_acts_Delete},
	{"RemoveSelected", pcb_act_RemoveSelected, pcb_acth_RemoveSelected, pcb_acts_RemoveSelected}
};

PCB_REGISTER_ACTIONS(remove_action_list, NULL)
