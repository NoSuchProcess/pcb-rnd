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

#warning TODO: rename config.h VERSION to PCB_VERSION
#undef VERSION

#include "hyp_l.h"
#include "hyp_y.h"

static const char *hyp_cookie = "hyp importer";

static const char load_hyp_syntax[] = "LoadhypFrom(filename)";

static const char load_hyp_help[] = "Loads the specified hyp resource file.";

int ActionLoadhypFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	fname = argc ? argv[0] : 0;

	if ((fname == NULL) || (*fname == '\0')) {
		fname = gui->fileselect(_("Load .hyp file..."),
														_("Picks a hyperlynx file to load.\n"),
														"default.hyp", ".hyp", "hyp", HID_FILESELECT_READ);
	}

	if (fname == NULL)
		PCB_AFAIL(load_hyp);

	hyp_in =  fopen(fname, "r");
	if (hyp_in == NULL)
		PCB_AFAIL(load_hyp);

	hyp_parse();

	fclose(hyp_in);
	return 0;
}

pcb_hid_action_t hyp_action_list[] = {
	{"LoadhypFrom", 0, ActionLoadhypFrom, load_hyp_help, load_hyp_syntax}
};

REGISTER_ACTIONS(hyp_action_list, hyp_cookie)

static void hid_hyp_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_import_hyp_init()
{
#warning TODO: rather register an importer than an action
	REGISTER_ACTIONS(hyp_action_list, hyp_cookie)
	return hid_hyp_uninit;
}

