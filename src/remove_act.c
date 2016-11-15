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
#include "set.h"
#include "remove.h"
#include "funchash_core.h"

/* --------------------------------------------------------------------------- */

static const char delete_syntax[] = "Delete(Object|Selected)\n" "Delete(AllRats|SelectedRats)";

static const char delete_help[] = "Delete stuff.";

/* %start-doc actions Delete

%end-doc */

static int ActionDelete(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	int id = pcb_funchash_get(function, NULL);

	Note.X = Crosshair.X;
	Note.Y = Crosshair.Y;

	if (id == -1) {								/* no arg */
		if (RemoveSelected() == pcb_false)
			id = F_Object;
	}

	switch (id) {
	case F_Object:
		SaveMode();
		SetMode(PCB_MODE_REMOVE);
		pcb_notify_mode();
		RestoreMode();
		break;
	case F_Selected:
		RemoveSelected();
		break;
	case F_AllRats:
		if (pcb_rats_destroy(pcb_false))
			SetChangedFlag(pcb_true);
		break;
	case F_SelectedRats:
		if (pcb_rats_destroy(pcb_true))
			SetChangedFlag(pcb_true);
		break;
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char removeselected_syntax[] = "RemoveSelected()";

static const char removeselected_help[] = "Removes any selected objects.";

/* %start-doc actions RemoveSelected

%end-doc */

static int ActionRemoveSelected(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (RemoveSelected())
		SetChangedFlag(pcb_true);
	return 0;
}


pcb_hid_action_t remove_action_list[] = {
	{"Delete", 0, ActionDelete,
	 delete_help, delete_syntax}
	,
	{"RemoveSelected", 0, ActionRemoveSelected,
	 removeselected_help, removeselected_syntax}
	,
};

PCB_REGISTER_ACTIONS(remove_action_list, NULL)
