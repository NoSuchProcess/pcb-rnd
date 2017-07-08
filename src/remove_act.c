/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "const.h"
#include "config.h"
#include "data.h"
#include "action_helper.h"
#include "remove.h"
#include "board.h"
#include "funchash_core.h"

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Delete[] = "Delete(Object|Selected)\n" "Delete(AllRats|SelectedRats)";

static const char pcb_acth_Delete[] = "Delete stuff.";

/* %start-doc actions Delete

%end-doc */

static int pcb_act_Delete(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	int id = pcb_funchash_get(function, NULL);

	if (id == -1) {								/* no arg */
		if (pcb_remove_selected() == pcb_false)
			id = F_Object;
	}

	switch (id) {
	case F_Object:
		pcb_gui->get_coords("Click on object to delete", &Note.X, &Note.Y);
		pcb_crosshair_save_mode();
		pcb_crosshair_set_mode(PCB_MODE_REMOVE);
		pcb_notify_mode();
		pcb_crosshair_restore_mode();
		break;
	case F_Selected:
		pcb_remove_selected();
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

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_RemoveSelected[] = "pcb_remove_selected()";

static const char pcb_acth_RemoveSelected[] = "Removes any selected objects.";

/* %start-doc actions RemoveSelected

%end-doc */

static int pcb_act_RemoveSelected(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (pcb_remove_selected())
		pcb_board_set_changed_flag(pcb_true);
	return 0;
}


pcb_hid_action_t remove_action_list[] = {
	{"Delete", 0, pcb_act_Delete,
	 pcb_acth_Delete, pcb_acts_Delete}
	,
	{"RemoveSelected", 0, pcb_act_RemoveSelected,
	 pcb_acth_RemoveSelected, pcb_acts_RemoveSelected}
	,
};

PCB_REGISTER_ACTIONS(remove_action_list, NULL)
