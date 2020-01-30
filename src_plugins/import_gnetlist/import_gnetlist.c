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
#include <librnd/core/conf.h>
#include <librnd/core/paths.h>
#include <librnd/core/actions.h>

#include "board.h"
#include "plug_import.h"

#include "import_gnetlist_conf.h"

conf_import_gnetlist_t conf_import_gnetlist;

static pcb_plug_import_t import_gnetlist;


int gnetlist_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, FILE *fp, const char *filename)
{
	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	return 100;
}

static int gnetlist_import_files(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	char **cmd;
	int n, res, verbose;
	fgw_arg_t rs;
	char *tmpfn = pcb_tempfile_name_new("gnetlist_output");

	PCB_IMPORT_SCH_VERBOSE(verbose);

	if (tmpfn == NULL) {
		pcb_message(PCB_MSG_ERROR, "Could not create temp file for gnetlist output");
		return -1;
	}


	cmd = malloc((numfns+9) * sizeof(char *));
	cmd[0] = conf_import_gnetlist.plugins.import_gnetlist.gnetlist_program;
	cmd[1] = "-L";
	cmd[2] = PCBLIBDIR;
	cmd[3] = "-g";
	cmd[4] = "pcbrndfwd";
	cmd[5] = "-o";
	cmd[6] = tmpfn;
	cmd[7] = "--";
	for(n = 0; n < numfns; n++)
		cmd[n+8] = pcb_build_fn(&PCB->hidlib, fns[n]);
	cmd[numfns+8] = NULL;

	if (verbose) {
		pcb_message(PCB_MSG_DEBUG, "import_gnetlist:  running gnetlist:\n");
		for(n = 0; n < numfns+8; n++)
			pcb_message(PCB_MSG_DEBUG, " %s", cmd[n]);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	res = pcb_spawnvp((const char **)cmd);
	if (res == 0) {
		if (verbose)
			pcb_message(PCB_MSG_DEBUG, "pcb_gnetlist:  about to run pcb_act_ExecuteFile, file = %s\n", tmpfile);
		fgw_uvcall(&pcb_fgw, &PCB->hidlib, &rs, "executefile", FGW_STR, tmpfile, 0);
	}
	for(n = 0; n < numfns; n++)
		free(cmd[n+8]);
	pcb_unlink(&PCB->hidlib, tmpfn);
	return res;
}

static int gnetlist_import(pcb_plug_import_t *ctx, unsigned int aspects, const char *fn)
{
	return gnetlist_import_files(ctx, aspects, &fn, 1);
}

int pplg_check_ver_import_gnetlist(int ver_needed) { return 0; }

void pplg_uninit_import_gnetlist(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_gnetlist);
	pcb_conf_unreg_fields("plugins/import_gnetlist/");
}

int pplg_init_import_gnetlist(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	import_gnetlist.plugin_data = NULL;

	import_gnetlist.fmt_support_prio = gnetlist_support_prio;
	import_gnetlist.import           = gnetlist_import;
	import_gnetlist.name             = "gEDA sch using gnetlist";

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_gnetlist);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_import_gnetlist, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_gnetlist_conf_fields.h"

	return 0;
}

