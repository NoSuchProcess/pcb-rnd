/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tinycad import HID
 *  pcb-rnd Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
#include "undo.h"
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

typedef struct {
	char *refdes;
	char *value;
	char *footprint;
} symattr_t;

#define null_empty(s) ((s) == NULL ? "" : (s))

static void sym_flush(symattr_t *sattr)
{
	if (sattr->refdes != NULL) {
/*		rnd_trace("tinycad sym: refdes=%s val=%s fp=%s\n", sattr->refdes, sattr->value, sattr->footprint);*/
		if (sattr->footprint == NULL)
			rnd_message(RND_MSG_ERROR, "tinycad: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			rnd_actionva(&PCB->hidlib, "ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
	}
	free(sattr->refdes); sattr->refdes = NULL;
	free(sattr->value); sattr->value = NULL;
	free(sattr->footprint); sattr->footprint = NULL;
}


static int tinycad_parse_net(FILE *fn)
{
	char line[1024];
	symattr_t sattr;

	memset(&sattr, 0, sizeof(sattr));

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		if (*s == ';') /* comment */
			continue;
		rtrim(s);
		argc = qparse2(s, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE);
		if ((argc > 1) && (strcmp(argv[0], "NET") == 0)) {
			char *curr, *next, *sep;

			for(curr = argv[5]; (curr != NULL) && (*curr != '\0'); curr = next) {
				next = strchr(curr, ')');
				if (next != NULL) {
					*next = '\0';
					next++;
					if (*next == ',')
						next++;
				}
				if (*curr == '(')
					curr++;
				sep = strchr(curr, ',');
				if (sep != NULL) {
					*sep = '-';
/*					rnd_trace("net-add '%s' '%s'\n", argv[2], curr);*/
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  argv[2], curr, NULL);
				}
			}
		}
		else if ((argc > 1) && (strcmp(argv[0], "COMPONENT") == 0)) {
			sym_flush(&sattr);
			free(sattr.refdes);
			sattr.refdes = rnd_strdup(argv[1]);
		}
		else if ((argc > 3) && (strcmp(argv[0], "OPTION") == 0)) {
			if (strcmp(argv[3], "..") != 0) {
				if (strcmp(argv[1], "Package") == 0) {
					free(sattr.footprint);
					sattr.footprint = rnd_strdup(argv[3]);
				}
				else if (strcmp(argv[1], "Value") == 0) {
					free(sattr.value);
					sattr.value = rnd_strdup(argv[3]);
				}
			}
		}
		qparse_free(argc, &argv);
	}

	sym_flush(&sattr);

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	return 0;
}


static int tinycad_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	pcb_undo_freeze_serial();
	ret = tinycad_parse_net(fn);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadtinycadFrom[] = "LoadTinycadFrom(filename)";
static const char pcb_acth_LoadtinycadFrom[] = "Loads the specified tinycad .net file - the netlist must be tinycad netlist output.";
fgw_error_t pcb_act_LoadtinycadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadtinycadFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load tinycad netlist file...",
																"Picks a tinycad netlist file to load.\n",
																default_file, ".net", NULL, "tinycad", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return tinycad_load(fname);
}

static int tinycad_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
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
		if (strncmp(s, "COMPONENT", 9) == 0)
			good |= 1;
		else if (strncmp(s, "OPTION", 6) == 0)
			good |= 2;
		else if (strncmp(s, "NET", 3) == 0)
			good |= 4;
		if (good == (1|2|4)) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);

	return 0;
}


static int tinycad_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_tinycad: requires exactly 1 input file name\n");
		return -1;
	}
	return tinycad_load(fns[0]);
}

static pcb_plug_import_t import_tinycad;

rnd_action_t tinycad_action_list[] = {
	{"LoadTinycadFrom", pcb_act_LoadtinycadFrom, pcb_acth_LoadtinycadFrom, pcb_acts_LoadtinycadFrom}
};

int pplg_check_ver_import_tinycad(int ver_needed) { return 0; }

void pplg_uninit_import_tinycad(void)
{
	rnd_remove_actions_by_cookie(tinycad_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_tinycad);
	rnd_hid_menu_unload(rnd_gui, tinycad_cookie);
}

int pplg_init_import_tinycad(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_tinycad.plugin_data = NULL;

	import_tinycad.fmt_support_prio = tinycad_support_prio;
	import_tinycad.import           = tinycad_import;
	import_tinycad.name             = "tinycad";
	import_tinycad.desc             = "schamtics from tinycad";
	import_tinycad.ui_prio          = 50;
	import_tinycad.single_arg       = 1;
	import_tinycad.all_filenames    = 1;
	import_tinycad.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_tinycad);

	RND_REGISTER_ACTIONS(tinycad_action_list, tinycad_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, tinycad_cookie, 175, NULL, 0, tinycad_menu, "plugin: import tinycad");
	return 0;
}
