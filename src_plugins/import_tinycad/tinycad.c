/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tinycad import HID
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

static const char *tinycad_cookie = "tinycad importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

static int tinycad_parse_net(FILE *fn)
{
	char line[1024];

	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE);
		if ((argc > 1) && (strcmp(argv[0], "NET") == 0)) {
			int n;
			for(n = 2; n < argc; n++) {
/*				pcb_trace("net-add '%s' '%s'\n", argv[1], argv[n]);*/
				pcb_hid_actionl("Netlist", "Add",  argv[1], argv[n], NULL);
			}
		}
	}

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

	return 0;
}


static int tinycad_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = tinycad_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadtinycadFrom[] = "LoadTinycadFrom(filename)";
static const char pcb_acth_LoadtinycadFrom[] = "Loads the specified tinycad .net and .asc file - the netlist must be mentor netlist.";
int pcb_act_LoadtinycadFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load tinycad netlist file...",
																"Picks a tinycad netlist file to load.\n",
																default_file, ".net", "tinycad", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadtinycadFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return tinycad_load(fname);
}

pcb_hid_action_t tinycad_action_list[] = {
	{"LoadtinycadFrom", 0, pcb_act_LoadtinycadFrom, pcb_acth_LoadtinycadFrom, pcb_acts_LoadtinycadFrom}
};

PCB_REGISTER_ACTIONS(tinycad_action_list, tinycad_cookie)

static void hid_tinycad_uninit()
{
	pcb_hid_remove_actions_by_cookie(tinycad_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_tinycad_init()
{
	PCB_REGISTER_ACTIONS(tinycad_action_list, tinycad_cookie)
	return hid_tinycad_uninit;
}
