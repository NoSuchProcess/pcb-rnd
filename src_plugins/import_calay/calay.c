/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  calay import HID
 *  pcb-rnd Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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

#define NETEXT ".net"
#define CMPEXT ".cmp"

#include "menu_internal.c"

static const char *calay_cookie = "calay importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n') || (*end == ' ')); end--) \
			*end = '\0'; \
	} while(0)


static int calay_parse_net(FILE *fn)
{
	char line[512];
	char *curr = NULL;

	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		char *s, *next, *num;

		s = line;
		ltrim(s);
		if (*s == '/') {
			s++;
			next = strpbrk(s, " \t\r\n");
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			free(curr);
			curr = rnd_strdup(s);
			s = next;
		}

		for(;;) {
			ltrim(s);
			if (*s == '\0')
				break;
			next = strchr(s, ')');
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			num = strchr(s, '(');
			if (num != NULL) {
				*num = '-';
				if (curr != NULL)
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  curr, s, NULL);
				else
					rnd_message(RND_MSG_ERROR, "Calay syntax error: %s is after a ;, not in any net\n", s);
			}
			else
				rnd_message(RND_MSG_ERROR, "Calay syntax error: %s should have been refdes(pin)\n", s);

			if ((next == NULL) || (*next == '\0'))
				break;
			switch(*next) {
				case ' ':
				case '\t':
				case ',': next++; break;
				case ';': next++; free(curr); curr = NULL; next++; break;
				default:
					rnd_message(RND_MSG_ERROR, "Calay syntax error: invalid separator: %s %d (expected , or ;)\n", next, *next);
			}
			s = next;
		}
	}

	free(curr);
	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);

	return 0;
}

static int calay_parse_comp(FILE *f)
{
	char line[512];
	char *val, *refdes, *footprint, *end;
	int len;
	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);

	while(fgets(line, sizeof(line), f) != NULL) {
		len = strlen(line);
		if ((len > 2) && (len < 54)) {
			rnd_message(RND_MSG_ERROR, "Calay component syntax error: short line: '%s'\n", line);
			continue;
		}
		val = line;

		refdes = strpbrk(val, " \t\r\n");
		if (refdes == NULL)
			continue;
		*refdes = 0;
		refdes++;
		ltrim(refdes);

		footprint = strpbrk(refdes, " \t\r\n");
		if (footprint == NULL)
			continue;
		*footprint = 0;
		footprint++;
		ltrim(footprint);

		end = strpbrk(footprint, " \t\r\n");
		if (end != NULL)
			*end = '\0';

		rnd_actionva(&PCB->hidlib, "ElementList", "Need", refdes, footprint, val, NULL);
	}
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);
	return 0;
}


static int calay_load(const char *fname_net, const char *fname_cmp)
{
	FILE *f;
	int ret = 0;

	f = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open calay netlist file '%s' for read\n", fname_net);
		return -1;
	}
	ret = calay_parse_net(f);
	fclose(f);

	f = rnd_fopen(&PCB->hidlib, fname_cmp, "r");
	if (f == NULL)
		rnd_message(RND_MSG_ERROR, "can't open calay component file '%s' for read\n(non-fatal, but footprints will not be placed)\n", fname_cmp);

	pcb_undo_freeze_serial();
	ret = calay_parse_comp(f);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();

	fclose(f);
	return ret;
}

static const char pcb_acts_LoadCalayFrom[] = "LoadCalayFrom(filename)";
static const char pcb_acth_LoadCalayFrom[] = "Loads the specified calay netlist/component file pair.";
fgw_error_t pcb_act_LoadCalayFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname_net = NULL;
	char *fname_cmp, *end;
	static char *default_file = NULL;
	

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadCalayFrom, fname_net = argv[1].val.str);

	if (!fname_net || !*fname_net) {
		fname_net = rnd_gui->fileselect(rnd_gui,
			"Load calay netlist file...", "Picks a calay netlist file to load.\n",
			default_file, NETEXT, NULL, "calay", RND_HID_FSD_READ, NULL);
		if (fname_net == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	fname_cmp = malloc(strlen(fname_net) + strlen(CMPEXT) + 4);
	strcpy(fname_cmp, fname_net);
	end = strrchr(fname_cmp, '.');
	if (end == NULL)
		end = fname_cmp + strlen(fname_cmp);
	strcpy(end, CMPEXT);

	RND_ACT_IRES(calay_load(fname_net, fname_cmp));
	free(fname_cmp);
	return 0;
}

static int calay_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;

	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	{
		char *s, line[16];

		s = fgets(line, sizeof(line), f);
		fclose(f);
		if ((s != NULL) && (s[0] == '/') && (s[7] == ' '))
			return 100;
	}
	return 0;
}


static int calay_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_calay: requires exactly 1 input file name\n");
		return -1;
	}
	return rnd_actionva(&PCB->hidlib, "LoadCalayFrom", fns[0], NULL);
}

static pcb_plug_import_t import_calay;

rnd_action_t calay_action_list[] = {
	{"LoadCalayFrom", pcb_act_LoadCalayFrom, pcb_acth_LoadCalayFrom, pcb_acts_LoadCalayFrom}
};

int pplg_check_ver_import_calay(int ver_needed) { return 0; }

void pplg_uninit_import_calay(void)
{
	rnd_remove_actions_by_cookie(calay_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_calay);
	rnd_hid_menu_unload(rnd_gui, calay_cookie);
}

int pplg_init_import_calay(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_calay.plugin_data = NULL;

	import_calay.fmt_support_prio = calay_support_prio;
	import_calay.import           = calay_import;
	import_calay.name             = "calay";
	import_calay.desc             = "schamtics from calay";
	import_calay.ui_prio          = 20;
	import_calay.single_arg       = 1;
	import_calay.all_filenames    = 1;
	import_calay.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_calay);


	RND_REGISTER_ACTIONS(calay_action_list, calay_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, calay_cookie, 170, NULL, 0, calay_menu, "plugin: import calay");
	return 0;
}
