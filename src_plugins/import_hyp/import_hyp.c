/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  hyperlynx .hyp importer, plugin entry
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <string.h>

#include "action_helper.h"
#include "compat_nls.h"
#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "plugins.h"
#include "event.h"

#warning TODO: rename config.h VERSION to PCB_VERSION
#undef VERSION

#include "parser.h"
#include "board.h"

static const char *hyp_cookie = "hyp importer";

static const char load_hyp_syntax[] = "LoadhypFrom(filename[, \"debug\"]...)";

static const char load_hyp_help[] = "Loads the specified Hyperlynx file.";

int pcb_act_LoadhypFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	int debug = 0;
	int i = 0;
	pcb_bool_t retval;

	fname = argc ? argv[0] : 0;

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect(_("Load .hyp file..."),
																_("Picks a hyperlynx file to load.\n"), "default.hyp", ".hyp", "hyp", HID_FILESELECT_READ);
	}

	if (fname == NULL)
		PCB_AFAIL(load_hyp);


	/* 
	 * debug level.
	 * one "debug" argument: hyperlynx logging.
	 * two "debug" arguments: hyperlynx and bison logging.
	 * three "debug" arguments: hyperlynx, bison and flex logging.
	 */

	for (i = 0; i < argc; i++)
		debug += (strcmp(argv[i], "debug") == 0);

	if (debug > 0)
		pcb_message(PCB_MSG_INFO, _("Importing Hyperlynx file '%s', debug level %d\n"), fname, debug);

	pcb_event(PCB_EVENT_BUSY, NULL);

	retval = hyp_parse(PCB->Data, fname, debug);

	/* notify GUI */
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	pcb_event(PCB_EVENT_BOARD_CHANGED, NULL);

	if (retval)
		PCB_AFAIL(load_hyp);

	return 0;
}

pcb_hid_action_t hyp_action_list[] = {
	{"LoadhypFrom", 0, pcb_act_LoadhypFrom, load_hyp_help, load_hyp_syntax}
};

PCB_REGISTER_ACTIONS(hyp_action_list, hyp_cookie)

		 static void hid_import_hyp_uninit()
{
	pcb_hid_remove_actions_by_cookie(hyp_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_hyp_init()
{
#warning TODO: rather register an importer than an action
	PCB_REGISTER_ACTIONS(hyp_action_list, hyp_cookie)
		return hid_import_hyp_uninit;
}

/* not truncated */
