/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"
#include "global.h"
#include "data.h"
#include "plugins.h"
#include "plug_import.h"


static plug_import_t import_edif;

int edif_support_prio(plug_import_t *ctx, unsigned int aspects, FILE *fp, const char *filename)
{
	char buf[65];
	int len;
	char *p;

	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */

	if (fp == NULL)
		return 0; /* only importing from a file is supported */

	/* If the header contains "edif", it is supported */
	len = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[len] = '\0';
	for(p = buf; *p != '\0'; p++)
		*p = tolower((int) *p);
	if (strstr(buf, "edif") != NULL)
		return 100;

	/* Else don't even attempt to load it */
	return 0;
}


extern int ReadEdifNetlist(char *filename);
static int edif_import(plug_import_t *ctx, unsigned int aspects, const char *fn)
{
	int ret = ReadEdifNetlist((char *)fn);
	if (ret == 0) {
		sort_netlist();
		rats_patch_make_edited(PCB);
	}
	return ret;
}

static void hid_import_edif_uninit(void)
{
}

pcb_uninit_t hid_import_edif_init(void)
{

	/* register the IO hook */
	import_edif.plugin_data = NULL;

	import_edif.fmt_support_prio = edif_support_prio;
	import_edif.import           = edif_import;

	HOOK_REGISTER(plug_import_t, plug_import_chain, &import_edif);

	return hid_import_edif_uninit;
}

