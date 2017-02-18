/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Schematics/netlist import action for KiCad's eeschema */

#include "config.h"

#include <assert.h>
#include <gensexpr/gsxl.h>
#include "compat_misc.h"

#include "read_net.h"

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"

#if 0
typedef struct {
	char *refdes;
	char *value;
	char *footprint;
} symattr_t;

#define null_empty(s) ((s) == NULL ? "" : (s))

static void sym_flush(symattr_t *sattr)
{
	if (sattr->refdes != NULL) {
/*		pcb_trace("eeschema sym: refdes=%s val=%s fp=%s\n", sattr->refdes, sattr->value, sattr->footprint);*/
		if (sattr->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "eeschema: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			pcb_hid_actionl("ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
	}
	free(sattr->refdes); sattr->refdes = NULL;
	free(sattr->value); sattr->value = NULL;
	free(sattr->footprint); sattr->footprint = NULL;
}

#endif


static int eeschema_parse_net(gsxl_dom_t *dom)
{
/*	symattr_t sattr;
	memset(&sattr, 0, sizeof(sattr));*/

	pcb_hid_actionl("ElementList", "start", NULL);
	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);


	printf("dom=%p\n", dom);

/*	pcb_hid_actionl("Netlist", "Add",  argv[2], curr, NULL);*/

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);
	pcb_hid_actionl("ElementList", "Done", NULL);

	return 0;
}



static int eeschema_load(const char *fname_net)
{
	FILE *fn;
	gsxl_dom_t dom;
	int c, ret = 0;
	gsx_parse_res_t res;

	fn = fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	gsxl_init(&dom, gsxl_node_t);

	dom.parse.line_comment_char = '#';
	do {
		c = fgetc(fn);
	} while((res = gsxl_parse_char(&dom, c)) == GSX_RES_NEXT);
	fclose(fn);

	if (res == GSX_RES_EOE) {
		/* compact and simplify the tree */
		gsxl_compact_tree(&dom);

		/* recursively parse the dom */
		ret = eeschema_parse_net(&dom);
	}
	else
		ret = -1;

	/* clean up */
	gsxl_uninit(&dom);

	return ret;
}

const char pcb_acts_LoadeeschemaFrom[] = "LoadEeschemaFrom(filename)";
const char pcb_acth_LoadeeschemaFrom[] = "Loads the specified eeschema .net file - the netlist must be an s-expression.";
int pcb_act_LoadeeschemaFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load eeschema netlist file...",
																"Picks a eeschema netlist file to load.\n",
																default_file, ".net", "eeschema", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadeeschemaFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return eeschema_load(fname);
}

