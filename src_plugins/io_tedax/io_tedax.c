/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - plugin coordination
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
#include "netlist.h"
#include "footprint.h"

#include "plugins.h"
#include "hid.h"
#include "hid_actions.h"
#include "action_helper.h"


static const char *tedax_cookie = "tEDAx IO";

static const char pcb_acts_Savetedax[] = "SaveTedax(type, filename)";
static const char pcb_acth_Savetedax[] = "Saves the specific type of data in a tEDAx file";
static int pcb_act_Savetedax(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return tedax_fp_save(PCB->Data, argv[1]);
}

static const char pcb_acts_LoadtedaxFrom[] = "LoadTedaxFrom(filename)";
static const char pcb_acth_LoadtedaxFrom[] = "Loads the specified tedax netlist file.";
static int pcb_act_LoadtedaxFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load tedax netlist file...",
																"Picks a tedax netlist file to load.\n",
																default_file, ".net", "tedax", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadtedaxFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return tedax_net_load(fname);
}


pcb_hid_action_t tedax_action_list[] = {
	{"LoadTedaxFrom", 0, pcb_act_LoadtedaxFrom, pcb_acth_LoadtedaxFrom, pcb_acts_LoadtedaxFrom},
	{"SaveTedax", 0, pcb_act_Savetedax, pcb_acth_Savetedax, pcb_acts_Savetedax}
};

PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)

static void hid_tedax_uninit()
{
	pcb_hid_remove_actions_by_cookie(tedax_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_io_tedax_init()
{
	PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)
	return hid_tedax_uninit;
}
