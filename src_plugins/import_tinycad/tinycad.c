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
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "safe_fs.h"

#include "actions.h"
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

typedef struct {
	char *refdes;
	char *value;
	char *footprint;
} symattr_t;

#define null_empty(s) ((s) == NULL ? "" : (s))

static void sym_flush(symattr_t *sattr)
{
	if (sattr->refdes != NULL) {
/*		pcb_trace("tinycad sym: refdes=%s val=%s fp=%s\n", sattr->refdes, sattr->value, sattr->footprint);*/
		if (sattr->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "tinycad: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			pcb_actionl("ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
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

	pcb_actionl("ElementList", "start", NULL);
	pcb_actionl("Netlist", "Freeze", NULL);
	pcb_actionl("Netlist", "Clear", NULL);

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
/*					pcb_trace("net-add '%s' '%s'\n", argv[2], curr);*/
					pcb_actionl("Netlist", "Add",  argv[2], curr, NULL);
				}
			}
		}
		else if ((argc > 1) && (strcmp(argv[0], "COMPONENT") == 0)) {
			sym_flush(&sattr);
			free(sattr.refdes);
			sattr.refdes = pcb_strdup(argv[1]);
		}
		else if ((argc > 3) && (strcmp(argv[0], "OPTION") == 0)) {
			if (strcmp(argv[3], "..") != 0) {
				if (strcmp(argv[1], "Package") == 0) {
					free(sattr.footprint);
					sattr.footprint = pcb_strdup(argv[3]);
				}
				else if (strcmp(argv[1], "Value") == 0) {
					free(sattr.value);
					sattr.value = pcb_strdup(argv[3]);
				}
			}
		}
		qparse_free(argc, &argv);
	}

	sym_flush(&sattr);

	pcb_actionl("Netlist", "Sort", NULL);
	pcb_actionl("Netlist", "Thaw", NULL);
	pcb_actionl("ElementList", "Done", NULL);

	return 0;
}


static int tinycad_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = pcb_fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = tinycad_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadtinycadFrom[] = "LoadTinycadFrom(filename)";
static const char pcb_acth_LoadtinycadFrom[] = "Loads the specified tinycad .net file - the netlist must be tinycad netlist output.";
fgw_error_t pcb_act_LoadtinycadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadtinycadFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load tinycad netlist file...",
																"Picks a tinycad netlist file to load.\n",
																default_file, ".net", NULL, "tinycad", PCB_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	PCB_ACT_IRES(0);
	return tinycad_load(fname);
}

pcb_action_t tinycad_action_list[] = {
	{"LoadTinycadFrom", pcb_act_LoadtinycadFrom, pcb_acth_LoadtinycadFrom, pcb_acts_LoadtinycadFrom}
};

PCB_REGISTER_ACTIONS(tinycad_action_list, tinycad_cookie)

int pplg_check_ver_import_tinycad(int ver_needed) { return 0; }

void pplg_uninit_import_tinycad(void)
{
	pcb_remove_actions_by_cookie(tinycad_cookie);
}

#include "dolists.h"
int pplg_init_import_tinycad(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(tinycad_action_list, tinycad_cookie)
	return 0;
}
