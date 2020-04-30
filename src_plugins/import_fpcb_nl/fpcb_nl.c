/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Freepcb netlist import
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
#include <qparse/qparse.h>

#include "board.h"
#include "data.h"
#include "plug_import.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

static const char *fpcb_nl_cookie = "fpcb_nl importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

static int fpcb_nl_load(const char *fn)
{
	FILE *f;
	char *line, *curr, *next, *fp, *tn, buff[8192], signame[512];
	enum { MODE_NONE, MODE_PART, MODE_NET, MODE_SIGNAL } mode = MODE_NONE;
	int anon = 0;

	f = rnd_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open %s for read\n", fn);
		return -1;
	}

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	while((line = fgets(buff, sizeof(buff), f)) != NULL) {
		rtrim(line);

		/* parse directive */
		if (*line == '*') {
			if (rnd_strcasecmp(line, "*END*") == 0)
				break;
			else if (rnd_strcasecmp(line, "*PART*") == 0)
				mode = MODE_PART;
			else if (rnd_strcasecmp(line, "*NET*") == 0)
				mode = MODE_NET;
			else if (rnd_strncasecmp(line, "*SIGNAL*", 8) == 0) {
				*signame = '\0';
				if ((mode == MODE_NET) || (mode == MODE_SIGNAL)) {
					int len;
					mode = MODE_SIGNAL;
					line += 9;
					ltrim(line);
					len = strlen(line);
					if (len == 0) {
						rnd_message(RND_MSG_ERROR, "Empty/missing net name in *SINGAL*\n");
						sprintf(signame, "pcbrndanonymous%d", anon++);
					}
					else {
						if (len > sizeof(signame)-1) {
							len = sizeof(signame)-1;
							rnd_message(RND_MSG_ERROR, "Net name %s is too long, truncating.\nThis may result in broken netlist, please use shorter names \n", line);
						}
						memcpy(signame, line, len);
						signame[len] = '\0';
					}
				}
				else
					mode = MODE_NONE;
			}
			continue;
		}

		switch(mode) {
			case MODE_PART:
				if (*line == '\0')
					continue;
				fp = strpbrk(line, " \t");
				if (fp != NULL) {
					*fp = '\0';
					fp++;
					ltrim(fp);
				}
				if ((fp != NULL) && (*fp != '\0')) {
					rnd_actionva(&PCB->hidlib, "ElementList", "Need", line, fp, "", NULL);
				}
				else
					rnd_message(RND_MSG_ERROR, "No footprint specified for %s\n", line);
				break;
			case MODE_SIGNAL:
				ltrim(line);
				for(curr = line; curr != NULL; curr = next) {
					next = strpbrk(curr, " \t");
					if (next != NULL) {
						*next = '\0';
						next++;
						ltrim(next);
					}
					if (*curr == '\0')
						continue;
					tn = strchr(curr, '.');
					if (tn == NULL) {
						rnd_message(RND_MSG_ERROR, "Syntax error in netlist: '%s' in net '%s' should be refdes.termid\n", curr, signame);
						continue;
					}
					*tn = '-';
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  signame, curr, NULL);
				}
				break;
			default: break; /* ignore line */
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	fclose(f);
	return 0;
}

static const char pcb_acts_LoadFpcbnlFrom[] = "LoadFpcbnlFrom(filename)";
static const char pcb_acth_LoadFpcbnlFrom[] = "Loads the specified freepcb netlist.";
fgw_error_t pcb_act_LoadFpcbnlFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadFpcbnlFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui,
			"Load freepcb netlist...", "Picks a freepcb netlist file to load.\n",
			default_file, ".net", NULL, "freepcb", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(fpcb_nl_load(fname));
	return 0;
}

static int fpcb_nl_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	int n;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported */

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(n = 0; n < 32; n++) {
		char line[1024];
		fgets(line, sizeof(line), f);
		if (strncmp(line, "*PADS-PCB*", 10) == 0) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);
	return 0;
}


static int fpcb_nl_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_fpcb_nl: requires exactly 1 input file name\n");
		return -1;
	}
	return fpcb_nl_load(fns[0]);
}

static pcb_plug_import_t import_fpcb_nl;

rnd_action_t fpcb_nl_action_list[] = {
	{"LoadFpcbnlFrom", pcb_act_LoadFpcbnlFrom, pcb_acth_LoadFpcbnlFrom, pcb_acts_LoadFpcbnlFrom}
};

int pplg_check_ver_import_fpcb_nl(int ver_needed) { return 0; }

void pplg_uninit_import_fpcb_nl(void)
{
	rnd_remove_actions_by_cookie(fpcb_nl_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_fpcb_nl);
}

int pplg_init_import_fpcb_nl(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_fpcb_nl.plugin_data = NULL;

	import_fpcb_nl.fmt_support_prio = fpcb_nl_support_prio;
	import_fpcb_nl.import           = fpcb_nl_import;
	import_fpcb_nl.name             = "FreePCB";
	import_fpcb_nl.desc             = "FreePCB/EasyEDA schematics";
	import_fpcb_nl.ui_prio          = 50;
	import_fpcb_nl.single_arg       = 1;
	import_fpcb_nl.all_filenames    = 1;
	import_fpcb_nl.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_fpcb_nl);


	RND_REGISTER_ACTIONS(fpcb_nl_action_list, fpcb_nl_cookie)
	return 0;
}
