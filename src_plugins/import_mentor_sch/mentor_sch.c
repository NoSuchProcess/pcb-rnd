/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  mentor graphics schematics import plugin
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include <gensexpr/gsxl.h>

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"

#include "netlist_helper.h"

static const char *mentor_sch_cookie = "mentor_sch importer";

/* Return the nth child's string of the subtree called subtree_name under node */
static const char *get_by_name(gsxl_node_t *node, const char *subtree_name, int child_idx)
{
	gsxl_node_t *n;
	for(n = node->children; n != NULL; n = n->next) {
		if (strcmp(n->str, subtree_name) == 0) {
			for(n = n->children; (n != NULL) && (child_idx > 0); n = n->next, child_idx--) ;
			if (n == NULL)
				return NULL;
			return n->str;
		}
	}
	return NULL;
}

static int parse_netlist_instance(nethlp_ctx_t *nhctx, gsxl_node_t *inst)
{
	gsxl_node_t *n;
	const char *refdes = NULL;
	nethlp_elem_ctx_t ectx;

/*	printf("inst %s\n", inst->children->str); */

	nethlp_elem_new(nhctx, &ectx, inst->children->str);

	for(n = inst->children; n != NULL; n = n->next) {
		if (strcmp(n->str, "designator") == 0) {
			refdes = n->children->str;
/*			printf(" refdes=%s\n", refdes); */
			nethlp_elem_refdes(&ectx, refdes);
		}
		else if (strcmp(n->str, "property") == 0) {
			const char *key, *val;
			key = get_by_name(n, "rename", 1);
			val = get_by_name(n, "string", 0);
/*			printf(" property '%s'='%s'\n", key, val); */
			if ((key != NULL) && (val != NULL))
				nethlp_elem_attr(&ectx, key, val);
		}
	}

	nethlp_elem_done(&ectx);
/*	pcb_hid_actionl("ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);*/
	return 0;
}

static int parse_netlist_net(nethlp_ctx_t *nhctx, gsxl_node_t *net)
{
	gsxl_node_t *n, *p;
	const char *netname = get_by_name(net, "rename", 1);
	nethlp_net_ctx_t nctx;

	nethlp_net_new(nhctx, &nctx, netname);

	for(n = net->children; n != NULL; n = n->next) {
		if (strcmp(n->str, "joined") == 0) {
			for(p = n->children; p != NULL; p = p->next) {
				if (strcmp(p->str, "portRef") == 0) {
					const char *part, *pin;
					pin = p->children->str;
					part = get_by_name(p, "instanceRef", 0);
					if ((part != NULL) && (pin != NULL)) {
						if (*pin == '&')
							pin++;
						nethlp_net_add_term(&nctx, part, pin);
					}
				}
			}
		}
	}
	
	nethlp_net_destroy(&nctx);
	return 0;
}


static int parse_netlist_view(gsxl_node_t *view)
{
	gsxl_node_t *contents, *n;
	nethlp_ctx_t nhctx;
	int res = 0;
	
	nethlp_new(&nhctx);
	nethlp_load_part_map(&nhctx, "mentor_parts.map");

	for(contents = view->children; contents != NULL; contents = contents->next) {
		if (strcmp(contents->str, "contents") == 0) {
			printf("--- view\n");
			for(n = contents->children; n != NULL; n = n->next) {
				if (strcmp(n->str, "instance") == 0)
					res |= parse_netlist_instance(&nhctx, n);
				if (strcmp(n->str, "net") == 0)
					res |= parse_netlist_net(&nhctx, n);
			}
		}
	}

	nethlp_destroy(&nhctx);
	return res;
}

static int mentor_parse_tree(gsxl_dom_t *dom)
{
	gsxl_node_t *view, *cell, *library, *vtype;

	/* check the header */
	if (strcmp(dom->root->str, "edif") != 0) {
		pcb_message(PCB_MSG_ERROR, "Invalid mentor edf header: not an EDIF file\n");
		return -1;
	}

	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);
	pcb_hid_actionl("ElementList", "start", NULL);

	for(library = dom->root->children; library != NULL; library = library->next) {
		if (strcmp(library->str, "library") == 0) {
			if (strcmp(library->children->str, "hierarchical") == 0) {
				for(cell = library->children; cell != NULL; cell = cell->next) {
					if (strcmp(cell->str, "cell") == 0) {
						for(view = cell->children; view != NULL; view = view->next) {
							if (strcmp(view->children->str, "v1") == 0) {
								vtype = view->children->next;
								if ((strcmp(vtype->str, "viewType") == 0) && (strcmp(vtype->children->str, "netlist") == 0))
									parse_netlist_view(view);
							}
						}
					}
				}
			}
		}
	}

	pcb_hid_actionl("ElementList", "Done", NULL);
	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

/*	for(n = library->children; n != NULL; n = n->next) {
		printf("n=%s\n", n->str);
		if (strcmp(n->str, "cell") == 0) { 
			printf("cell\n");
		}
	}*/


	return -1;
}


static int mentor_sch_load(const char *fname_net)
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
		ret = mentor_parse_tree(&dom);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid mentor edf: not a valid s-expression file near %d:%d\n", dom.parse.line, dom.parse.col);
		ret = -1;
	}

	/* clean up */
	gsxl_uninit(&dom);

	return ret;

}

static const char pcb_acts_Loadmentor_schFrom[] = "LoadMentorFrom(filename)";
static const char pcb_acth_Loadmentor_schFrom[] = "Loads the specified mentor schematics .edf file.";
int pcb_act_LoadMentorFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load mentor edf netlist file...",
																"Picks a mentor edf file to load.\n",
																default_file, ".edf", "mentor_sch", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(Loadmentor_schFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return mentor_sch_load(fname);
}

pcb_hid_action_t mentor_sch_action_list[] = {
	{"LoadMentorFrom", 0, pcb_act_LoadMentorFrom, pcb_acth_Loadmentor_schFrom, pcb_acts_Loadmentor_schFrom}
};

PCB_REGISTER_ACTIONS(mentor_sch_action_list, mentor_sch_cookie)

static void hid_mentor_sch_uninit()
{
	pcb_hid_remove_actions_by_cookie(mentor_sch_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_mentor_sch_init()
{
	PCB_REGISTER_ACTIONS(mentor_sch_action_list, mentor_sch_cookie)
	return hid_mentor_sch_uninit;
}
