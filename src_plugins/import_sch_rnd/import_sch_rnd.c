/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hidlib.h>
#include <librnd/core/conf_multi.h>

/* for win32 paths: */
#include <librnd/hid/hid_init.h>

#include "board.h"
#include "undo.h"
#include "plug_import.h"
#include "plug_io.h"

#include "import_sch_rnd_conf.h"
#include "../src_plugins/import_sch_rnd/conf_internal.c"

conf_import_sch_rnd_t conf_import_sch_rnd;

static pcb_plug_import_t import_sch_rnd;
static char import_sch_rnd_cookie[] = "import_sch_rnd";

int sch_rnd_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	/* Native file format would be lihata sch-rnd schematics;
	   but we do not attempt to directly load a lihata files as generic netlist
	   import, only through import_sch2 so refuse this */
	return 0;
}

static int sch_rnd_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	char **cmd;
	int n, res, verbose;
	char *tmpfn = rnd_tempfile_name_new("sch_rnd_output");

	PCB_IMPORT_SCH_VERBOSE(verbose);

	if (tmpfn == NULL) {
		rnd_message(RND_MSG_ERROR, "Could not create temp file for sch-rnd output");
		return -1;
	}

	cmd = malloc((numfns+6) * sizeof(char *));
	if (ctx != &import_sch_rnd) {
		rnd_message(RND_MSG_ERROR, "sch_rnd_import(): invalid context\n");
		return -1;
	}

	cmd[0] = (char *)conf_import_sch_rnd.plugins.import_sch_rnd.sch_rnd_program; /* won't be free'd */
	cmd[1] = "-x";
	cmd[2] = "tedax";
	cmd[3] = "--outfile";
	cmd[4] = tmpfn;
	for(n = 0; n < numfns; n++)
		cmd[n+5] = rnd_build_fn(&PCB->hidlib, fns[n]);
	cmd[numfns+5] = NULL;

	if (verbose) {
		rnd_message(RND_MSG_DEBUG, "import_sch_rnd:  running sch-rnd:\n");
		for(n = 0; n < numfns+5; n++)
			rnd_message(RND_MSG_DEBUG, " %s", cmd[n]);
		rnd_message(RND_MSG_DEBUG, "\n");
	}

	res = rnd_spawnvp((const char **)cmd);
	if (res == 0) {
		if (verbose)
			rnd_message(RND_MSG_DEBUG, "pcb_sch_rnd:  about to run pcb_act_ExecuteFile, file = %s\n", tmpfn);
		if (rnd_actionva(&PCB->hidlib, "LoadTedaxFrom", "Netlist", tmpfn, NULL) != 0)
			goto execerr;
	}
	else {
		execerr:;
		rnd_message(RND_MSG_ERROR, "Netlist failed to produce usable output. Refer to stderr for details\n");
		rnd_message(RND_MSG_ERROR, "Command line was:");
		for(n = 0; n < numfns+5; n++)
			rnd_message(RND_MSG_ERROR, " %s", cmd[n]);
		rnd_message(RND_MSG_ERROR, "\n");
	}
	for(n = 0; n < numfns; n++)
		free(cmd[n+5]);
	rnd_unlink(&PCB->hidlib, tmpfn);
	free(cmd);
	return res;
}

int pplg_check_ver_import_sch_rnd(int ver_needed) { return 0; }

void pplg_uninit_import_sch_rnd(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_sch_rnd);
	rnd_conf_plug_unreg("plugins/import_sch_rnd/", import_sch_rnd_conf_internal, import_sch_rnd_cookie);
}

int pplg_init_import_sch_rnd(void)
{
	RND_API_CHK_VER;

	/* register the IO hooks */
	import_sch_rnd.plugin_data = NULL;
	import_sch_rnd.fmt_support_prio = sch_rnd_support_prio;
	import_sch_rnd.import           = sch_rnd_import;
	import_sch_rnd.name             = "sch-rnd";
	import_sch_rnd.desc             = "sch-rnd sheet(s) or Ringdove/sch-rnd project";
	import_sch_rnd.ui_prio          = 200;
	import_sch_rnd.single_arg       = 0;
	import_sch_rnd.all_filenames    = 1;
	import_sch_rnd.ext_exec         = 0;
	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_sch_rnd);

	rnd_conf_plug_reg(conf_import_sch_rnd, import_sch_rnd_conf_internal, import_sch_rnd_cookie);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_import_sch_rnd, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_sch_rnd_conf_fields.h"

	return 0;
}

