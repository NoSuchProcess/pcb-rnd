/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  mentor graphics schematics import plugin
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

static const char *mentor_sch_cookie = "mentor_sch importer";


static int mentor_sch_load(const char *fname_net)
{
	return -1;
}

static const char pcb_acts_Loadmentor_schFrom[] = "LoadMentorFrom(filename)";
static const char pcb_acth_Loadmentor_schFrom[] = "Loads the specified mentor schematics .edf file.";
int pcb_act_LoadMentorFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load mentor edf netlist file...",
																"Picks a mentor edf file to load.\n",
																default_file, ".edf", "mentor_sch", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(Loadmentor_schFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return mentor_sch_load(fname);
}

pcb_hid_action_t mentor_sch_action_list[] = {
	{"LoadMentorFrom", 0, pcb_act_LoadMentorFrom, pcb_acth_Loadmentor_schFrom, pcb_acts_Loadmentor_schFrom}
};

PCB_REGISTER_ACTIONS(mentor_sch_action_list, mentor_sch_cookie)

static void hid_mentor_sch_uninit()
{
	pcb_hid_remove_actions_by_cookie(mentor_sch_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_mentor_sch_init()
{
	PCB_REGISTER_ACTIONS(mentor_sch_action_list, mentor_sch_cookie)
	return hid_mentor_sch_uninit;
}
