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

/* for win32 paths: */
#include <librnd/core/hid_init.h>

#include "board.h"
#include "undo.h"
#include "plug_import.h"

#include "import_gnetlist_conf.h"
#include "../src_plugins/import_gnetlist/conf_internal.c"

#define IMPORT_GNETLIST_CONF_FN "import_gnetlist.conf"

conf_import_gnetlist_t conf_import_gnetlist;

static pcb_plug_import_t import_gnetlist, import_lepton;


int gnetlist_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	char line[128], *s;
	int spc = 0;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs < 1))
		return 0; /* only pure netlist import is supported, only if there are files */

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	line[0] = '\0';
	fgets(line, sizeof(line), f);
	fclose(f);

	/* must start with "v ", followed by digits and one space */
	if ((line[0] != 'v') || (line[1] != ' '))
		return 0;
	for(s = line+2; *s != '\0'; s++) {
		if ((*s == '\r') || (*s == '\n')) break;
		else if (*s == ' ') spc++;
		else if (!isdigit(*s))
			return 0;
	}
	if (spc != 1)
		return 0;

	return 100;
}

static int gnetlist_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	char **cmd;
	int n, res, verbose;
	fgw_arg_t rs;
	char *tmpfn = rnd_tempfile_name_new("gnetlist_output");

	PCB_IMPORT_SCH_VERBOSE(verbose);

	if (tmpfn == NULL) {
		rnd_message(RND_MSG_ERROR, "Could not create temp file for gnetlist output");
		return -1;
	}


	cmd = malloc((numfns+9) * sizeof(char *));
	if (ctx == &import_gnetlist) {
		cmd[0] = (char *)conf_import_gnetlist.plugins.import_gnetlist.gnetlist_program; /* won't be free'd */
		cmd[4] = "pcbrndfwd";
	}
	else if (ctx == &import_lepton) {
		cmd[0] = (char *)conf_import_gnetlist.plugins.import_gnetlist.lepton_netlist_program; /* won't be free'd */
		cmd[4] = "tEDAx";
	}
	else {
		rnd_message(RND_MSG_ERROR, "gnetlist_import(): invalid context\n");
		return -1;
	}

	cmd[1] = "-L";
	cmd[2] = PCBLIBDIR;
	cmd[3] = "-g";
	/* cmd[4] = "pcbrndfwd"; */
	cmd[5] = "-o";
	cmd[6] = tmpfn;
	cmd[7] = "--";
	for(n = 0; n < numfns; n++)
		cmd[n+8] = rnd_build_fn(&PCB->hidlib, fns[n]);
	cmd[numfns+8] = NULL;

	if (verbose) {
		rnd_message(RND_MSG_DEBUG, "import_gnetlist:  running gnetlist:\n");
		for(n = 0; n < numfns+8; n++)
			rnd_message(RND_MSG_DEBUG, " %s", cmd[n]);
		rnd_message(RND_MSG_DEBUG, "\n");
	}

	res = rnd_spawnvp((const char **)cmd);
	if (res == 0) {
		if (verbose)
			rnd_message(RND_MSG_DEBUG, "pcb_gnetlist:  about to run pcb_act_ExecuteFile, file = %s\n", tmpfn);
		if (ctx == &import_gnetlist) {
			pcb_undo_freeze_serial();
			fgw_uvcall(&rnd_fgw, &PCB->hidlib, &rs, "executefile", FGW_STR, tmpfn, 0);
			pcb_undo_unfreeze_serial();
			pcb_undo_inc_serial();
		}
		else
			rnd_actionva(&PCB->hidlib, "LoadTedaxFrom", "Netlist", tmpfn, NULL);
	}
	for(n = 0; n < numfns; n++)
		free(cmd[n+8]);
	rnd_unlink(&PCB->hidlib, tmpfn);
	free(cmd);
	return res;
}

int pplg_check_ver_import_gnetlist(int ver_needed) { return 0; }

void pplg_uninit_import_gnetlist(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_gnetlist);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_lepton);
	rnd_conf_unreg_file(IMPORT_GNETLIST_CONF_FN, import_gnetlist_conf_internal);
	rnd_conf_unreg_fields("plugins/import_gnetlist/");
}

int pplg_init_import_gnetlist(void)
{
	RND_API_CHK_VER;

	/* register the IO hooks */
	import_gnetlist.plugin_data = NULL;
	import_gnetlist.fmt_support_prio = gnetlist_support_prio;
	import_gnetlist.import           = gnetlist_import;
	import_gnetlist.name             = "gnetlist";
	import_gnetlist.desc             = "gEDA sch using gnetlist";
	import_gnetlist.ui_prio          = 50;
	import_gnetlist.single_arg       = 0;
	import_gnetlist.all_filenames    = 1;
	import_gnetlist.ext_exec         = 0;
	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_gnetlist);

	import_lepton.plugin_data = NULL;
	import_lepton.fmt_support_prio = gnetlist_support_prio;
	import_lepton.import           = gnetlist_import;
	import_lepton.name             = "lepton";
	import_lepton.desc             = "lepton-eda sch using lepton-netlist";
	import_lepton.ui_prio          = 51;
	import_lepton.single_arg       = 0;
	import_lepton.all_filenames    = 1;
	import_lepton.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_lepton);

	rnd_conf_reg_file(IMPORT_GNETLIST_CONF_FN, import_gnetlist_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_import_gnetlist, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_gnetlist_conf_fields.h"

	return 0;
}

