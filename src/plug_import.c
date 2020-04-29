/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015,2016 Tibor 'Igor2' Palinkas
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
 *
 */

/* This used to be file.c; it's a hook based plugin API now */

#include "config.h"
#include "conf_core.h"

#include <locale.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librnd/core/plugins.h>
#include "plug_import.h"
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_fs.h>


pcb_plug_import_t *pcb_plug_import_chain = NULL;

typedef struct {
	pcb_plug_import_t *plug;
	int prio;
} find_t;

/* Find the plugin that offers the highest write prio for the format */
static pcb_plug_import_t *find_importer(unsigned int aspects, const char **args, int numargs)
{
	find_t available[32]; /* wish we had more than 32 import plugins... */
	int n, len = 0, best = 0, bestidx = -1;

#define cb_append(pl, pr) \
	do { \
		if (pr > 0) { \
			assert(len < sizeof(available)/sizeof(available[0])); \
			available[len].plug = pl; \
			available[len++].prio = pr; \
		} \
	} while(0)


	PCB_HOOK_CALL_ALL(pcb_plug_import_t, pcb_plug_import_chain, fmt_support_prio, cb_append,   (self, aspects, args, numargs));
	if (len == 0)
		return NULL;

	for(n = 0; n < len; n++) {
		if (available[n].prio > best) {
			bestidx = n;
			best = available[n].prio;
		}
	}

	if (best == 0)
		return NULL;

	return available[bestidx].plug;
#undef cb_append
}

pcb_plug_import_t *pcb_lookup_importer(const char *name)
{
	pcb_plug_import_t *p;

	if (name == NULL)
		return NULL;

	for(p = pcb_plug_import_chain; p != NULL; p = p->next)
		if (strcmp(p->name, name) == 0)
			return p;

	return NULL;
}


int pcb_import(rnd_hidlib_t *hidlib, const char *filename, unsigned int aspect)
{
	pcb_plug_import_t *plug;

	if (!rnd_file_readable(filename)) {
		rnd_message(RND_MSG_ERROR, "Error: can't find a suitable netlist parser for %s - might be related: can't open %s for reading\n", filename, filename);
		return 1;
	}

	if (!filename) {
		rnd_message(RND_MSG_ERROR, "Error: need a file name for pcb_import_netlist()\n");
		return 1; /* nothing to do */
	}

	plug = find_importer(aspect, &filename, 1);
	if (plug == NULL) {
		rnd_message(RND_MSG_ERROR, "Error: can't find a suitable netlist parser for %s\n", filename);
		return 1;
	}

	return plug->import(plug, aspect, &filename, 1);
}

int pcb_import_netlist(rnd_hidlib_t *hidlib, const char *filename)
{
	return pcb_import(hidlib, filename, IMPORT_ASPECT_NETLIST);
}

void pcb_import_uninit(void)
{
	if (pcb_plug_import_chain != NULL)
		rnd_message(RND_MSG_ERROR, "pcb_plug_import_chain is not empty; a plugin did not remove itself from the chain. Fix your plugins!\n");
}
