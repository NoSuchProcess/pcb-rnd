/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  mentor graphics schematics import plugin
 *  pcb-rnd Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include "undo.h"
#include "plug_import.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <gensexpr/gsxl.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include "mentor_sch_conf.h"
#include <librnd/core/paths.h>
#include <librnd/core/safe_fs.h>

#include "netlist_helper.h"

#include "menu_internal.c"

conf_mentor_sch_t conf_mentor;


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

	nethlp_elem_done(&PCB->hidlib, &ectx);
/*	rnd_actionva(&PCB->hidlib, "ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);*/
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
						nethlp_net_add_term(&PCB->hidlib, &nctx, part, pin);
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
	int idx, res = 0, cnt = 0;
	rnd_conf_listitem_t *item;
	const char *item_str;

	nethlp_new(&nhctx);

	rnd_conf_loop_list_str(&conf_mentor.plugins.import_mentor_sch.map_search_paths, item, item_str, idx) {
		char *p; 
		rnd_path_resolve(&PCB->hidlib, item_str, &p, 0, rnd_false);
		if (p != NULL) {
			cnt += nethlp_load_part_map(&nhctx, p);
			free(p);
		}
	}
	if (cnt == 0)
		rnd_message(RND_MSG_WARNING, "Couldn't find any part map rules - check your map_search_paths and rule files\n");

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
		rnd_message(RND_MSG_ERROR, "Invalid mentor edf header: not an EDIF file\n");
		return -1;
	}

	pcb_undo_freeze_serial();

	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);

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

	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);

	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();

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

	fn = rnd_fopen(NULL, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
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
		rnd_message(RND_MSG_ERROR, "Invalid mentor edf: not a valid s-expression file near %d:%d\n", dom.parse.line, dom.parse.col);
		ret = -1;
	}

	/* clean up */
	gsxl_uninit(&dom);

	return ret;

}

static const char pcb_acts_Loadmentor_schFrom[] = "LoadMentorFrom(filename)";
static const char pcb_acth_Loadmentor_schFrom[] = "Loads the specified Mentor Graphics Design Capture schematics flat .edf file.";
fgw_error_t pcb_act_LoadMentorFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, Loadmentor_schFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load mentor edf netlist file...",
																"Picks a mentor edf file to load.\n",
																default_file, ".edf", NULL, "mentor_sch", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return mentor_sch_load(fname);
}

static int mentor_sch_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	if (aspects != IMPORT_ASPECT_NETLIST)
		return 0; /* only pure netlist import is supported */
	return 0; /* can't tell apart from normal edif; best if the user selects format explicitly */
}


static int mentor_sch_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_mentor_sch: requires exactly 1 input file name\n");
		return -1;
	}
	return mentor_sch_load(fns[0]);
}

static pcb_plug_import_t import_mentor_sch;


rnd_action_t mentor_sch_action_list[] = {
	{"LoadMentorFrom", pcb_act_LoadMentorFrom, pcb_acth_Loadmentor_schFrom, pcb_acts_Loadmentor_schFrom}
};

int pplg_check_ver_import_mentor_sch(int ver_needed) { return 0; }

void pplg_uninit_import_mentor_sch(void)
{
	rnd_remove_actions_by_cookie(mentor_sch_cookie);
	rnd_conf_unreg_fields("plugins/import_mentor_sch/");
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_mentor_sch);
	rnd_hid_menu_unload(rnd_gui, mentor_sch_cookie);
}

int pplg_init_import_mentor_sch(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_mentor, field,isarray,type_name,cpath,cname,desc,flags);
#include "mentor_sch_conf_fields.h"

	/* register the IO hook */
	import_mentor_sch.plugin_data = NULL;

	import_mentor_sch.fmt_support_prio = mentor_sch_support_prio;
	import_mentor_sch.import           = mentor_sch_import;
	import_mentor_sch.name             = "mentor_sch";
	import_mentor_sch.desc             = "schamtics: Mentor Graphics Design Design Caputre";
	import_mentor_sch.ui_prio          = 40;
	import_mentor_sch.single_arg       = 1;
	import_mentor_sch.all_filenames    = 1;
	import_mentor_sch.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_mentor_sch);

	RND_REGISTER_ACTIONS(mentor_sch_action_list, mentor_sch_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, mentor_sch_cookie, 185, NULL, 0, mentor_menu, "plugin: mentor_sch");
	return 0;
}
