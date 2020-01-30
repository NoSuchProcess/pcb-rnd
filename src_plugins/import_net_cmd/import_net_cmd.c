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
#include <librnd/core/actions.h>

#include "board.h"
#include "plug_import.h"

static pcb_plug_import_t import_net_cmd;


int net_cmd_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, FILE *fp, const char *filename)
{
	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	return 100;
}


static int net_cmd_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	const char **cmd;
	int n, res, verbose;
	fgw_arg_t rs;
	char *outfile, *tmpfn = pcb_tempfile_name_new("gnetlist_output");

	PCB_IMPORT_SCH_VERBOSE(verbose);

	if (tmpfn == NULL) {
		pcb_message(PCB_MSG_ERROR, "Could not create temp file for gnetlist output");
		return -1;
	}

	outfile = tmpfn;

	cmd = malloc((numfns+1) * sizeof(char *));
	for(n = 0; n < numfns; n++)
		cmd[n] = fns[n];
	cmd[numfns+1] = NULL;

	if (verbose) {
		pcb_message(PCB_MSG_DEBUG, "import_net_cmd:  running cmd:\n");
		for(n = 0; n < numfns; n++)
			pcb_message(PCB_MSG_DEBUG, " %s", cmd[n]);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}


	pcb_setenv("IMPORT_NET_CMD_PCB", PCB->hidlib.filename, 1);
	pcb_setenv("IMPORT_NET_CMD_OUT", outfile, 1);
	res = pcb_spawnvp(cmd);
	if (res == 0) {
		if (verbose)
			pcb_message(PCB_MSG_DEBUG, "pcb_net_cmd:  about to run pcb_act_ExecuteFile, file = %s\n", tmpfile);
		fgw_uvcall(&pcb_fgw, &PCB->hidlib, &rs, "executefile", FGW_STR, tmpfile, 0);
	}
	if (tmpfn != NULL)
		pcb_unlink(&PCB->hidlib, tmpfn);
	free(cmd);
	return res;

	return -1;
}

int pplg_check_ver_import_net_cmd(int ver_needed) { return 0; }

void pplg_uninit_import_net_cmd(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_cmd);
}

int pplg_init_import_net_cmd(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	import_net_cmd.plugin_data = NULL;

	import_net_cmd.fmt_support_prio = net_cmd_support_prio;
	import_net_cmd.import           = net_cmd_import;
	import_net_cmd.name             = "sch/netlist by cmd";
	import_net_cmd.single_arg       = 0;
	import_net_cmd.all_filenames    = 0;
	import_net_cmd.ext_exec         = 1;

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_net_cmd);

	return 0;
}

