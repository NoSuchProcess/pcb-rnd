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

static const char *ltspice_cookie = "ltspice importer";

static int ltspice_hdr_net(FILE *f)
{
	char s[1024];
	while(fgets(s, sizeof(s), f) != NULL) {
		int argc;
		char **argv;

		/* split and print fields */
		argc = qparse(s, &argv);
		if ((argc > 0) && (pcb_strcasecmp(argv[0], "ExpressPCB Netlist") == 0))
			return 0;
	}
	return -1;
}

static int ltspice_load(const char *fname_net, const char *fname_asc)
{
	FILE *fn, *fa;
	int ret = 0;

	fn = fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}
	fa = fopen(fname_asc, "r");
	if (fa == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_asc);
		fclose(fn);
		return -1;
	}

	if (ltspice_hdr_net(fn)) {
		pcb_message(PCB_MSG_ERROR, "file '%s' doesn't look like an ExpressPCB netlist\n", fname_net);
		goto error;
	}
/*	if (ltspice_hdr_asc(fn)) goto error;*/


	quit:;
	fclose(fa);
	fclose(fn);
	return ret;

	error:
	ret = -1;
	goto quit;
}

static const char pcb_acts_LoadLtspiceFrom[] = "LoadLtspiceFrom(filename)";
static const char pcb_acth_LoadLtspiceFrom[] = "Loads the specified ltspice .net and .asc file - the netlist must be an ExpressPCB netlist.";
int pcb_act_LoadLtspiceFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL, *end;
	char *fname_asc, *fname_net, *fname_base;
	static char *default_file = NULL;
	int res;

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

	end = strrchr(fname, '.');
	if (strcmp(end, ".net") == 0)
		fname_base = pcb_strndup(fname, end - fname);
	else if (strcmp(end, ".asc") == 0)
		fname_base = pcb_strndup(fname, end - fname);
	else {
		pcb_message(PCB_MSG_ERROR, "file name '%s' doesn't end in .net or .asc\n", fname);
		PCB_ACT_FAIL(LoadLtspiceFrom);
	}

	fname_net = pcb_strdup_printf("%s.net", fname_base);
	fname_asc = pcb_strdup_printf("%s.asc", fname_base);
	free(fname_base);

	res = ltspice_load(fname_net, fname_asc);

	free(fname_asc);
	free(fname_net);

	return res;
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
