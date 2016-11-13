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

#include "config.h"
#include "autoplace.h"
#include "plugins.h"
#include "set.h"
#include "hid_actions.h"
#include "compat_nls.h"

static const char autoplace_syntax[] = "AutoPlaceSelected()";

static const char autoplace_help[] = "Auto-place selected components.";

/* %start-doc actions AutoPlaceSelected

Attempts to re-arrange the selected components such that the nets
connecting them are minimized.  Note that you cannot undo this.

%end-doc */

static int ActionAutoPlaceSelected(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	hid_action("Busy");
	if (gui->confirm_dialog(_("Auto-placement can NOT be undone.\n" "Do you want to continue anyway?\n"), 0)) {
		if (AutoPlaceSelected())
			SetChangedFlag(pcb_true);
	}
	return 0;
}

static const char *autoplace_cookie = "autoplace plugin";

pcb_hid_action_t autoplace_action_list[] = {
	{"AutoPlaceSelected", 0, ActionAutoPlaceSelected,
	 autoplace_help, autoplace_syntax}
	,
};

PCB_REGISTER_ACTIONS(autoplace_action_list, autoplace_cookie)

static void hid_autoplace_uninit(void)
{
	hid_remove_actions_by_cookie(autoplace_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_autoplace_init(void)
{
	PCB_REGISTER_ACTIONS(autoplace_action_list, autoplace_cookie)
	return hid_autoplace_uninit;
}
