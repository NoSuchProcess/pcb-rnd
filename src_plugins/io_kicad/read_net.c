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

#define if_strval(node, name) \
	if (strcmp(node->str, #name) == 0) { \
		if (name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Invalid eechema: multiple %s subtrees\n", name); \
			return -1; \
		} \
		if (node->children != NULL) \
			name = node->children->str; \
	}

#define if_subtree(node, name) \
	if (strcmp(node->str, #name) == 0) { \
		if (name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Invalid eechema: multiple %s subtrees\n", name); \
			return -1; \
		} \
		name = node; \
	}

#define req_subtree(name) \
	if (name == NULL) { \
		pcb_message(PCB_MSG_ERROR, "Invalid eechema: missing %s subtree\n", name); \
		return -1; \
	} \


static int eeschema_parse_net(gsxl_dom_t *dom)
{
	gsxl_node_t *c, *n, *version = NULL, *components = NULL, *nets = NULL;

	/* check the header */
	if (strcmp(dom->root->str, "export") != 0) {
		pcb_message(PCB_MSG_ERROR, "Invalid eechema netlist header: not an export\n");
		return -1;
	}

	for(n = dom->root->children; n != NULL; n = n->next) {
		if_subtree(n, version)
		else if_subtree(n, components)
		else if_subtree(n, nets)
	}

	req_subtree(version);
	req_subtree(components);
	req_subtree(nets);

	if ((version->children == NULL) || (strcmp(version->children->str, "D") != 0)) {
		pcb_message(PCB_MSG_ERROR, "Invalid eechema version: expected 'D', got '%s'\n", version->children->str);
		return -1;
	}

	pcb_hid_actionl("ElementList", "start", NULL);
	for(c = components->children; c != NULL; c = c->next) {
		const char *ref = NULL, *value = NULL, *footprint = NULL;
		if (strcmp(c->str, "comp") != 0)
			continue;
		for(n = c->children; n != NULL; n = n->next) {
			if_strval(n, ref)
			else if_strval(n, value)
			else if_strval(n, footprint)
		}
		if (ref == NULL) {
			pcb_message(PCB_MSG_ERROR, "eechema: ignoring component with no refdes\n");
			continue;
		}
		if (footprint == NULL) {
			pcb_message(PCB_MSG_ERROR, "eechema: ignoring component %s with no footprint\n", ref);
			continue;
		}
		pcb_hid_actionl("ElementList", "Need", ref, footprint, value == NULL ? "" : value, NULL);
	}

	pcb_hid_actionl("ElementList", "Done", NULL);


	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);

/*	pcb_hid_actionl("Netlist", "Add",  argv[2], curr, NULL);*/

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

	printf("dom=%p\n", dom);

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

