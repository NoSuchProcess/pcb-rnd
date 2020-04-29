/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  netlist action import HID
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
#include <ctype.h>

#include "board.h"
#include "actions_pcb.h"
#include "plug_import.h"

#include <librnd/core/error.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

static int net_action_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	unsigned int good = 0, l;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported from a single file*/

	f = pcb_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(l = 0; l < 32; l++) {
		char *s, *n, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		for(n = s; *n != '\0'; n++)
			*n = tolower(*n);
		if (strstr(s, "netlist") != NULL) {
			if (strstr(s, "freeze") != NULL) good |= 1;
			if (strstr(s, "clear") != NULL) good |= 2;
			if (strstr(s, "thaw") != NULL) good |= 2;
		}
		if (strstr(s, "elementlist") != NULL) {
			if (strstr(s, "start") != NULL) good |= 1;
			if (strstr(s, "need") != NULL) good |= 2;
		}
		if (good == (1|2)) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);
	return 0;
}


static int net_action_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	if (numargs != 1) {
		rnd_message(RND_MSG_ERROR, "import_net_action: requires exactly 1 input file name\n");
		return -1;
	}
	pcb_act_execute_file(&PCB->hidlib, args[0]);
	return 0; /* some parts of the script may fail (e.g. footprint not found) - that doesn't mean a real error */
}

static pcb_plug_import_t import_net_action;

int pplg_check_ver_import_net_action(int ver_needed) { return 0; }

void pplg_uninit_import_net_action(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_action);
}

int pplg_init_import_net_action(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	import_net_action.plugin_data = NULL;

	import_net_action.fmt_support_prio = net_action_support_prio;
	import_net_action.import           = net_action_import;
	import_net_action.name             = "action";
	import_net_action.desc             = "schamtics from pcb-rnd action script";
	import_net_action.ui_prio          = 95;
	import_net_action.single_arg       = 1;
	import_net_action.all_filenames    = 1;
	import_net_action.ext_exec         = 0;

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_action);

	return 0;
}
