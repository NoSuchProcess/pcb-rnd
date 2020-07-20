/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  sch import: Protel netlist 2.0
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

static const char *protel_net_cookie = "protel_net importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

#define LOAD(dst) \
do { \
	dst.used = 0; \
	*line = '\0'; \
	fgets(line, sizeof(line), fn); \
	s = line; \
	ltrim(s); \
	rtrim(s); \
	gds_append_str(&dst, s); \
} while(0)

#define FREE(dst) gds_truncate(&dst, 0)

static int protel_net_parse_net(FILE *fn)
{
	char line[1024];
	enum { NONE, NET, PART, CFG } mode = NONE;
	gds_t refdes, footprint, value, netname;

	gds_init(&refdes);
	gds_init(&footprint);
	gds_init(&value);
	gds_init(&netname);

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		int argc;
		char **argv, *s, *end;

		s = line;
		ltrim(s);
		rtrim(s);

		switch(mode) {
			case NONE:
				if (*s == '[')
					mode = PART;
				else if (*s == '(') {
					LOAD(netname);
					mode = NET;
				}
				else if (*s == '{')
					mode = CFG;
				break;

			case PART:
				if (*s == ']') {
					rnd_actionva(&PCB->hidlib, "ElementList", "Need", refdes.array, footprint.array, value.array, NULL);
					FREE(refdes);
					FREE(footprint);
					FREE(value);
					mode = NONE;
				}
				else if (strcmp(s, "DESIGNATOR") == 0)
					LOAD(refdes);
				else if (strcmp(s, "FOOTPRINT") == 0)
					LOAD(footprint);
				else if (strcmp(s, "PARTTYPE") == 0)
					LOAD(value);
				break;

			case NET:
				if (*s == ')') {
					FREE(netname);
					mode = NONE;
				}
				else {
					end = strchr(s, ' ');
					if (end != NULL)
						*end = '\0';
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  netname.array, s, NULL);
				}
				break;

			case CFG:
				if (*s == '}')
					mode = NONE;
				break;
		}
	}

	if (mode != NONE)
		rnd_message(RND_MSG_ERROR, "protel: last block is not closed\n");

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	gds_uninit(&refdes);
	gds_uninit(&footprint);
	gds_uninit(&value);
	gds_uninit(&netname);

	return 0;
}


static int protel_net_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = protel_net_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadProtelNetFrom[] = "LoadProtelNetFrom(filename)";
static const char pcb_acth_LoadProtelNetFrom[] = "Loads the specified protel netlist 2.0 file.";
fgw_error_t pcb_act_LoadProtelNetFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadProtelNetFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load pads ascii netlist file...",
																"Picks a pads ascii netlist file to load.\n",
																default_file, ".net", NULL, "protel_net", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return protel_net_load(fname);
}

static int protel_net_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
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
		if (strncmp(s, "PROTEL NETLIST 2.0", 18) == 0) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);

	return 0;
}


static int protel_net_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_protel_net: requires exactly 1 input file name\n");
		return -1;
	}
	return protel_net_load(fns[0]);
}

static pcb_plug_import_t import_protel_net;

rnd_action_t protel_net_action_list[] = {
	{"LoadProtelNetFrom", pcb_act_LoadProtelNetFrom, pcb_acth_LoadProtelNetFrom, pcb_acts_LoadProtelNetFrom}
};

int pplg_check_ver_import_protel_net(int ver_needed) { return 0; }

void pplg_uninit_import_protel_net(void)
{
	rnd_remove_actions_by_cookie(protel_net_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_protel_net);
	rnd_hid_menu_unload(rnd_gui, protel_net_cookie);
}

int pplg_init_import_protel_net(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_protel_net.plugin_data = NULL;

	import_protel_net.fmt_support_prio = protel_net_support_prio;
	import_protel_net.import           = protel_net_import;
	import_protel_net.name             = "protel_net";
	import_protel_net.desc             = "schamtics from protel netlist 2.0";
	import_protel_net.ui_prio          = 50;
	import_protel_net.single_arg       = 1;
	import_protel_net.all_filenames    = 1;
	import_protel_net.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_protel_net);

	RND_REGISTER_ACTIONS(protel_net_action_list, protel_net_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, protel_net_cookie, 175, NULL, 0, protel_net_menu, "plugin: import protel_net");
	return 0;
}
