/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
 *
 *  This module, io_kicad_legacy, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "board.h"
#include "data.h"
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "plug_import.h"
#include "rats_patch.h"

/* for pcb_sort_library only */
#include "plug_io.h"


static pcb_plug_import_t import_edif;

int edif_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	char buf[65];
	int len;
	char *p;
	FILE *fp;

	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	fp = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (fp == NULL)
		return 0; /* only importing from a file is supported */

	/* If the header contains "edif", it is supported */
	len = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[len] = '\0';
	for(p = buf; *p != '\0'; p++)
		*p = tolower((int) *p);
	if (strstr(buf, "edif") != NULL) {
		fclose(fp);
		return 100;
	}

	fclose(fp);
	/* Else don't even attempt to load it */
	return 0;
}


extern int ReadEdifNetlist(char *filename);
static int edif_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	int ret;

	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_edif: requires exactly 1 input file name\n");
		return -1;
	}

	ret = ReadEdifNetlist((char *)fns[0]);
	if (ret == 0)
		pcb_ratspatch_make_edited(PCB);
	return ret;
}

int pplg_check_ver_import_edif(int ver_needed) { return 0; }

void pplg_uninit_import_edif(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_edif);
}

int pplg_init_import_edif(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_edif.plugin_data = NULL;

	import_edif.fmt_support_prio = edif_support_prio;
	import_edif.import           = edif_import;
	import_edif.name             = "edif";
	import_edif.desc             = "EDIF netlist (flat)";
	import_edif.ui_prio          = 50;
	import_edif.single_arg       = 1;
	import_edif.all_filenames    = 1;
	import_edif.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_edif);

	return 0;
}

