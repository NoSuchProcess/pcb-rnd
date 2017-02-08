/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  LTSpice import HID
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"

static const char *ltspice_cookie = "ltspice importer";

static const char pcb_acts_LoadLtspiceFrom[] = "LoadLtspiceFrom(filename)";
static const char pcb_acth_LoadLtspiceFrom[] = "Loads the specified ltspice routing file.";
int pcb_act_LoadLtspiceFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load ltspice net+asc file pair...",
																"Picks a ltspice net or asc file to load.\n",
																default_file, ".asc", "ltspice", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadLtspiceFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return 0;
}

pcb_hid_action_t ltspice_action_list[] = {
	{"LoadLtspiceFrom", 0, pcb_act_LoadLtspiceFrom, pcb_acth_LoadLtspiceFrom, pcb_acts_LoadLtspiceFrom}
};

PCB_REGISTER_ACTIONS(ltspice_action_list, ltspice_cookie)

static void hid_ltspice_uninit()
{
	pcb_hid_remove_actions_by_cookie(ltspice_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_ltspice_init()
{
	PCB_REGISTER_ACTIONS(ltspice_action_list, ltspice_cookie)
		return hid_ltspice_uninit;
}
