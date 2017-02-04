/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Specctra .dsn import HID
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"
#include "data.h"

#include "action_helper.h"
#include "hid.h"
#include "plugins.h"

static const char *dsn_cookie = "dsn importer";

static const char load_dsn_syntax[] = "LoadDsnFrom(filename)";

static const char load_dsn_help[] = "Loads the specified routed dsn file.";

int pcb_act_LoadDsnFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	pcb_coord_t clear;

	fname = argc ? argv[0] : 0;

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect(
			"Load a routed dsn file...",
			"Select dsn file to load.\nThe file could be generated using the tool downloaded from freeroute.net\n",
			NULL, /* default file name */
			".dsn", "dsn", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_dsn);
	}

	clear = PCB->RouteStyle.array[0].Clearance * 2;

	return 0;
}

pcb_hid_action_t dsn_action_list[] = {
	{"LoadDsnFrom", 0, pcb_act_LoadDsnFrom, load_dsn_help, load_dsn_syntax}
};

PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)

static void hid_dsn_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_import_dsn_init()
{
	PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	return hid_dsn_uninit;
}

