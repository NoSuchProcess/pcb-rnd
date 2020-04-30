/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/plugins.h>
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>

#include "board.h"
#include "plug_import.h"

static pcb_plug_import_t import_net_cmd;


int net_cmd_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	return 0; /* autodetect should always fail to avoid accidental command execution - the import shall work only directly */
}


static int net_cmd_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	int res, verbose;
	const char *cmdline, *outfn;
	char *tmpfn = NULL;

	PCB_IMPORT_SCH_VERBOSE(verbose);

	if (numfns != 2) {
		rnd_message(RND_MSG_ERROR, "net_cmd_import: requires exactly two arguments:\nfirst argument must be the output file name or -\nsecond argument must be a full command line\n");
		return -1;
	}

	outfn = fns[0];
	cmdline = fns[1];

	if ((outfn == NULL) || (*outfn == '\0')) {
		rnd_message(RND_MSG_ERROR, "net_cmd_import: Could not create temp file for the netlist output");
		return -1;
	}
	if ((outfn[0] == '-') && (outfn[1] == '\0')) {
		tmpfn = rnd_tempfile_name_new("net_cmd_output");
		outfn = tmpfn;
	}

	if (verbose)
		rnd_message(RND_MSG_DEBUG, "import_net_cmd:  running cmd: '%s' outfn='%s'\n", cmdline, outfn);

	rnd_setenv("IMPORT_NET_CMD_PCB", PCB->hidlib.filename, 1);
	rnd_setenv("IMPORT_NET_CMD_OUT", outfn, 1);
	res = rnd_system(&PCB->hidlib, cmdline);
	if (res == 0) {
		if (verbose)
			rnd_message(RND_MSG_DEBUG, "pcb_net_cmd:  about to run pcb_act_ExecuteFile, outfn='%s'\n", outfn);
		pcb_import_netlist(&PCB->hidlib, outfn);
	}
	if (tmpfn != NULL)
		rnd_tempfile_unlink(tmpfn);
	return res;
}

int pplg_check_ver_import_net_cmd(int ver_needed) { return 0; }

void pplg_uninit_import_net_cmd(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_cmd);
}

int pplg_init_import_net_cmd(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_net_cmd.plugin_data = NULL;

	import_net_cmd.fmt_support_prio = net_cmd_support_prio;
	import_net_cmd.import           = net_cmd_import;
	import_net_cmd.name             = "cmd";
	import_net_cmd.desc             = "sch/netlist by command line";
	import_net_cmd.ui_prio          = 90;
	import_net_cmd.single_arg       = 0;
	import_net_cmd.all_filenames    = 0;
	import_net_cmd.ext_exec         = 1;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_cmd);

	return 0;
}

