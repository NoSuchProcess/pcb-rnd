/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  calay import HID
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "safe_fs.h"

#include "actions.h"
#include "plugins.h"
#include "hid.h"

static const char *calay_cookie = "calay importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)


static int calay_parse_net(FILE *fn)
{
	char line[512];
	char *curr = NULL;

	pcb_actionl("Netlist", "Freeze", NULL);
	pcb_actionl("Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		char *s, *next;

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
			curr = pcb_strdup(s);
			printf("curr='%s'\n", curr);
			s = next;
		}

/*		pcb_actionl("Netlist", "Add",  argv[2], curr, NULL);*/

	}

	free(curr);
	pcb_actionl("Netlist", "Sort", NULL);
	pcb_actionl("Netlist", "Thaw", NULL);

	return 0;
}

static int calay_parse_comp(FILE *fn)
{
	pcb_actionl("ElementList", "start", NULL);

	pcb_actionl("ElementList", "Done", NULL);

}


static int calay_load(const char *fname_net)
{
	FILE *f;
	int ret = 0;

	f = pcb_fopen(fname_net, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = calay_parse_net(f);

	fclose(f);
	return ret;
}

static const char pcb_acts_LoadCalayFrom[] = "LoadCalayFrom(filename)";
static const char pcb_acth_LoadCalayFrom[] = "Loads the specified calay netlist/component file pair.";
fgw_error_t pcb_act_LoadCalayFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadCalayFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load calay netlist file...",
																"Picks a calay netlist file to load.\n",
																default_file, ".net", "calay", HID_FILESELECT_READ);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	PCB_ACT_IRES(0);
	return calay_load(fname);
}

pcb_action_t calay_action_list[] = {
	{"LoadCalayFrom", pcb_act_LoadCalayFrom, pcb_acth_LoadCalayFrom, pcb_acts_LoadCalayFrom}
};

PCB_REGISTER_ACTIONS(calay_action_list, calay_cookie)

int pplg_check_ver_import_calay(int ver_needed) { return 0; }

void pplg_uninit_import_calay(void)
{
	pcb_remove_actions_by_cookie(calay_cookie);
}

#include "dolists.h"
int pplg_init_import_calay(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(calay_action_list, calay_cookie)
	return 0;
}
