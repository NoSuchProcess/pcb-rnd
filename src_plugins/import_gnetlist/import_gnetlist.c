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

#include "plug_import.h"

static pcb_plug_import_t import_gnetlist;


int gnetlist_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, FILE *fp, const char *filename)
{
	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	return 100;
}


static int gnetlist_import(pcb_plug_import_t *ctx, unsigned int aspects, const char *fn)
{
	return -1;
}

int pplg_check_ver_import_gnetlist(int ver_needed) { return 0; }

void pplg_uninit_import_gnetlist(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_gnetlist);
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

	return 0;
}

