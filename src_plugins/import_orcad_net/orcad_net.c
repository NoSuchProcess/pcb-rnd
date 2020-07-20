/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  sch import: PADS netlist ASCII (powerpcb?)
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#include "plug_import.h"

#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include <gensexpr/gsxl.h>
#include <genvector/gds_char.h>

#include "menu_internal.c"

static const char *orcad_net_cookie = "orcad_net importer";

static int orcad_net_parse_net(FILE *fn)
{
	gsxl_dom_t dom;
	gsxl_node_t *n, *footprint, *refdes, *noise, *net;
	int res, c, restore;
	gds_t tmp;

	gds_init(&tmp);
	gsxl_init(&dom, gsxl_node_t);

	dom.parse.line_comment_char = '#';
	dom.parse.brace_quote = 1;
	do {
		c = fgetc(fn);
	} while((res = gsxl_parse_char(&dom, c)) == GSX_RES_NEXT);

	if (res != GSX_RES_EOE) {
		rnd_message(RND_MSG_ERROR, "orcad: s-expression parse error\n");
		return -1;
	}

	/* compact and simplify the tree */
	gsxl_compact_tree(&dom);


	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	for(n = dom.root->children; n != NULL; n = n->next) {
		footprint = n->children;
		if ((footprint == NULL) || (footprint->next == NULL) || (footprint->next->next == NULL)) {
			rnd_message(RND_MSG_ERROR, "orcad: missing footprint or refdes in %d:%d\n", n->line, n->col);
			continue;
		}
		refdes = footprint->next;
		noise = refdes->next;

/*pcb_trace("@ '%s' '%s'\n", footprint->str, refdes->str);*/
		rnd_actionva(&PCB->hidlib, "ElementList", "Need", refdes->str, footprint->str, "", NULL);

		tmp.used = 0;
		gds_append_str(&tmp, refdes->str);
		gds_append(&tmp, '-');
		restore = tmp.used;
	
		for(net = noise->next; net != NULL; net = net->next) {
			if (net->children == NULL) {
				rnd_message(RND_MSG_ERROR, "orcad: missing terminal ID in %d:%d\n", n->line, n->col);
				continue;
			}

			tmp.used = restore;
			gds_append_str(&tmp, net->str);
			rnd_actionva(&PCB->hidlib, "Netlist", "Add",  net->children->str, tmp.array, NULL);
/*pcb_trace(" net %s %s\n", tmp.array, net->children->str);*/
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	gsxl_uninit(&dom);
	gds_uninit(&tmp);
	return 0;
}


static int orcad_net_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = orcad_net_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadOrcadNetFrom[] = "LoadOrcadNetFrom(filename)";
static const char pcb_acth_LoadOrcadNetFrom[] = "Loads the specified pads ascii netlist .net file.";
fgw_error_t pcb_act_LoadOrcadNetFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadOrcadNetFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load pads ascii netlist file...",
																"Picks a pads ascii netlist file to load.\n",
																default_file, ".net", NULL, "orcad_net", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return orcad_net_load(fname);
}

static int orcad_net_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	unsigned int good = 0, limit;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported from a single file*/

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(limit = 0; limit < 8; limit++) {
		char *s, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		while(isspace(*s)) s++;
		if (strstr(s, "OrCAD/PCB II Netlist") == 0) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);

	return 0;
}


static int orcad_net_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_orcad_net: requires exactly 1 input file name\n");
		return -1;
	}
	return orcad_net_load(fns[0]);
}

static pcb_plug_import_t import_orcad_net;

rnd_action_t orcad_net_action_list[] = {
	{"LoadOrcadNetFrom", pcb_act_LoadOrcadNetFrom, pcb_acth_LoadOrcadNetFrom, pcb_acts_LoadOrcadNetFrom}
};

int pplg_check_ver_import_orcad_net(int ver_needed) { return 0; }

void pplg_uninit_import_orcad_net(void)
{
	rnd_remove_actions_by_cookie(orcad_net_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_orcad_net);
	rnd_hid_menu_unload(rnd_gui, orcad_net_cookie);
}

int pplg_init_import_orcad_net(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_orcad_net.plugin_data = NULL;

	import_orcad_net.fmt_support_prio = orcad_net_support_prio;
	import_orcad_net.import           = orcad_net_import;
	import_orcad_net.name             = "orcad_net";
	import_orcad_net.desc             = "schamtics from pads ascii netlist";
	import_orcad_net.ui_prio          = 50;
	import_orcad_net.single_arg       = 1;
	import_orcad_net.all_filenames    = 1;
	import_orcad_net.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_orcad_net);

	RND_REGISTER_ACTIONS(orcad_net_action_list, orcad_net_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, orcad_net_cookie, 175, NULL, 0, orcad_net_menu, "plugin: import orcad_net");
	return 0;
}
