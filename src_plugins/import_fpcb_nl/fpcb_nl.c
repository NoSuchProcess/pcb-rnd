/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Freepcb netlist import
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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

	return -1;
}

static const char pcb_acts_LoadFpcbnlFrom[] = "LoadFpcbnlFrom(filename)";
static const char pcb_acth_LoadFpcbnlFrom[] = "Loads the specified freepcb netlist.";
fgw_error_t pcb_act_LoadFpcbnlFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *end;
	char *fname_asc, *fname_net, *fname_base;
	static char *default_file = NULL;
	int rs;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadFpcbnlFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load freepcb netlist...",
																"Picks a freepcb netlist file to load.\n",
																default_file, ".net", "freepcb", HID_FILESELECT_READ);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	PCB_ACT_IRES(fpcb_nl_load(fname));
	return 0;
}

pcb_action_t fpcb_nl_action_list[] = {
	{"LoadFpcbnlFrom", pcb_act_LoadFpcbnlFrom, pcb_acth_LoadFpcbnlFrom, pcb_acts_LoadFpcbnlFrom}
};

PCB_REGISTER_ACTIONS(fpcb_nl_action_list, fpcb_nl_cookie)

int pplg_check_ver_import_fpcb_nl(int ver_needed) { return 0; }

void pplg_uninit_import_fpcb_nl(void)
{
	pcb_remove_actions_by_cookie(fpcb_nl_cookie);
}

#include "dolists.h"
int pplg_init_import_fpcb_nl(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(fpcb_nl_action_list, fpcb_nl_cookie)
	return 0;
}
