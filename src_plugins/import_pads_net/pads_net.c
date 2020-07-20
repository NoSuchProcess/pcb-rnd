/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  sch import: PADS netlist ASCII (powerpcb?)
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qparse/qparse.h>

#include "board.h"
#include "data.h"
#include "plug_import.h"

#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

#include "menu_internal.c"

static const char *pads_net_cookie = "pads_net importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)
static int pads_net_parse_net(FILE *fn)
{
	char line[1024], signal[1024];
	enum { NONE, NET, PART } mode = NONE;

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	*signal = '\0';

	while(fgets(line, sizeof(line), fn) != NULL) {
		int argc;
		char **argv, *s, *next, *pin;

		s = line;
		ltrim(s);
		rtrim(s);

		if (strcmp(s, "*NET*") == 0)  { mode = NET; continue; }
		if (strncmp(s, "*PART*", 6) == 0) { mode = PART; continue; }
		if (strcmp(s, "*END*") == 0) { break; }

		if (strncmp(s, "*SIGNAL*", 8) == 0) {
			s = line + 8;
			ltrim(s);
			strncpy(signal, s, sizeof(signal));
			continue;
		}

		switch(mode) {
			case PART:
				argc = qparse2(s, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE | QPARSE_MULTISEP);
				rnd_actionva(&PCB->hidlib, "ElementList", "Need", argv[0], argv[1], "", NULL);
				qparse_free(argc, &argv);
				break;
			case NET:
				if (*signal == '\0') {
					rnd_message(RND_MSG_ERROR, "pads_net: not importing net=%s: no signal specified\n", line);
					continue;
				}
				for(s = line; (s != NULL) && (*s != '\0'); s = next) {
					next = strpbrk(s, " \t");
					if (next != NULL) {
						*next = '\0';
						next++;
						ltrim(next);
					}
					pin = strchr(s, '.');
					if (pin == NULL) {
						rnd_message(RND_MSG_ERROR, "pads_net: not importing pin='%s' for net %s: no terminal ID\n", s, signal);
						continue;
					}
					*pin = '-';
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  signal, s, NULL);
				}
				break;
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	return 0;
}


static int pads_net_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = pads_net_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadPadsNetFrom[] = "LoadPadsNetFrom(filename)";
static const char pcb_acth_LoadPadsNetFrom[] = "Loads the specified pads ascii netlist .asc file.";
fgw_error_t pcb_act_LoadPadsNetFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadPadsNetFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load pads ascii netlist file...",
																"Picks a pads ascii netlist file to load.\n",
																default_file, ".asc", NULL, "pads_net", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return pads_net_load(fname);
}

static int pads_net_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	unsigned int good = 0;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported from a single file*/

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(;;) {
		char *s, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		while(isspace(*s)) s++;
		if (strncmp(s, "!PADS-", 6) == 0)
			good |= 1;
		else if (strncmp(s, "*PART*", 6) == 0)
			good |= 2;
		else if (strncmp(s, "*NET*", 5) == 0)
			good |= 4;
		if (good == (1|2|4)) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);

	return 0;
}


static int pads_net_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_pads_net: requires exactly 1 input file name\n");
		return -1;
	}
	return pads_net_load(fns[0]);
}

static pcb_plug_import_t import_pads_net;

rnd_action_t pads_net_action_list[] = {
	{"LoadPadsNetFrom", pcb_act_LoadPadsNetFrom, pcb_acth_LoadPadsNetFrom, pcb_acts_LoadPadsNetFrom}
};

int pplg_check_ver_import_pads_net(int ver_needed) { return 0; }

void pplg_uninit_import_pads_net(void)
{
	rnd_remove_actions_by_cookie(pads_net_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_pads_net);
	rnd_hid_menu_unload(rnd_gui, pads_net_cookie);
}

int pplg_init_import_pads_net(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_pads_net.plugin_data = NULL;

	import_pads_net.fmt_support_prio = pads_net_support_prio;
	import_pads_net.import           = pads_net_import;
	import_pads_net.name             = "pads_net";
	import_pads_net.desc             = "schamtics from pads ascii netlist";
	import_pads_net.ui_prio          = 50;
	import_pads_net.single_arg       = 1;
	import_pads_net.all_filenames    = 1;
	import_pads_net.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_pads_net);

	RND_REGISTER_ACTIONS(pads_net_action_list, pads_net_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, pads_net_cookie, 175, NULL, 0, pads_net_menu, "plugin: import pads_net");
	return 0;
}
