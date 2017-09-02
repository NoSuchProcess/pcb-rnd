/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  HP-GL import HID
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
#include <qparse/qparse.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"

static const char *hpgl_cookie = "hpgl importer";

static int hpgl_load(const char *fname)
{
	return -1;
}

static const char pcb_acts_LoadHpglFrom[] = "LoadHpglFrom(filename)";
static const char pcb_acth_LoadHpglFrom[] = "Loads the specified hpgl .net and .asc file - the netlist must be mentor netlist.";
int pcb_act_LoadHpglFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL, *end;
	char *fname_asc, *fname_net, *fname_base;
	static char *default_file = NULL;
	int res;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load HP-GL file...",
																"Picks a HP-GL plot file to load.\n",
																default_file, ".hpgl", "hpgl", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadHpglFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return hpgl_load(fname);
}

pcb_hid_action_t hpgl_action_list[] = {
	{"LoadHpglFrom", 0, pcb_act_LoadHpglFrom, pcb_acth_LoadHpglFrom, pcb_acts_LoadHpglFrom}
};

PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)

int pplg_check_ver_import_hpgl(int ver_needed) { return 0; }

void pplg_uninit_import_hpgl(void)
{
	pcb_hid_remove_actions_by_cookie(hpgl_cookie);
}

#include "dolists.h"
int pplg_init_import_hpgl(void)
{
	PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)
	return 0;
}
