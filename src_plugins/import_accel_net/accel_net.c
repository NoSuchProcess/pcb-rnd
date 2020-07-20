/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  sch import: ACCEL netlist
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

static const char *accel_net_cookie = "accel_net importer";

static int accel_net_parse_net(FILE *fn)
{
	gsxl_dom_t dom;
	gsxl_node_t *netlist, *n, *m, *footprint, *refdes, *noise, *net;
	int res, c, restore;
	gds_t tmp;
	char line[1024];

	gds_init(&tmp);
	gsxl_init(&dom, gsxl_node_t);

	fgets(line, sizeof(line), fn); /* eat up the header line, not part of the s-expr */

	dom.parse.line_comment_char = '#';
	dom.parse.brace_quote = 1;

 /* need to fake a root because there are multiple roots in the file */
	gsxl_parse_char(&dom, '(');
	gsxl_parse_char(&dom, 'R');
	gsxl_parse_char(&dom, ' ');

	do {
		c = fgetc(fn);
		if (c == EOF)
			gsxl_parse_char(&dom, ')'); /* close fake root first */
	} while((res = gsxl_parse_char(&dom, c)) == GSX_RES_NEXT);

	if (res != GSX_RES_EOE) {
		rnd_message(RND_MSG_ERROR, "accel: s-expression parse error\n");
		return -1;
	}

	/* compact and simplify the tree */
	gsxl_compact_tree(&dom);

	if ((dom.root->children == NULL) || (dom.root->children->next == NULL)) {
		rnd_message(RND_MSG_ERROR, "accel: missing root node or netlist\n");
		return -1;
	}

	if (strcmp(dom.root->children->str, "asciiHeader") != 0) {
		rnd_message(RND_MSG_ERROR, "accel: invalid root node; espected 'asciiHeader', got '%s'\n", dom.root->str);
		return -1;
	}

	netlist = dom.root->children->next;
	if (strcmp(netlist->str, "netlist") != 0) {
		rnd_message(RND_MSG_ERROR, "accel: invalid root node; espected 'asciiHeader', got '%s'\n", dom.root->str);
		return -1;
	}

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);


	netlist = netlist->children->next; /* first item was the netlist name */
	for(n = netlist; n != NULL; n = n->next) {
		if (strcmp(n->str, "compInst") == 0) {
			char *refdes = n->children->str, *footprint = NULL, *value = NULL;
			for(m = n->children; m != NULL; m = m->next) {
				if (strcmp(m->str, "originalName") == 0) footprint = m->children->str;
				if (strcmp(m->str, "compValue") == 0) value = m->children->str;
			}
			if (footprint != NULL)
				rnd_actionva(&PCB->hidlib, "ElementList", "Need", refdes, footprint, value, NULL);
			else
				rnd_message(RND_MSG_ERROR, "accel: can't import %s: no footprint\n", refdes);
		}
		else if (strcmp(n->str, "net") == 0) {
			char *netname = n->children->str;
			for(m = n->children; m != NULL; m = m->next) {
				char *refdes = NULL, *pin = NULL;
				if (strcmp(m->str, "node") == 0) {
					refdes = m->children->str;
					pin = m->children->next->str;
					tmp.used = 0;
					gds_append_str(&tmp, refdes);
					gds_append(&tmp, '-');
					gds_append_str(&tmp, pin);
					rnd_actionva(&PCB->hidlib, "Netlist", "Add",  netname, tmp.array, NULL);
				}
			}
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	gsxl_uninit(&dom);
	gds_uninit(&tmp);
	return 0;
}


static int accel_net_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = rnd_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = accel_net_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadAccelNetFrom[] = "LoadAccelNetFrom(filename)";
static const char pcb_acth_LoadAccelNetFrom[] = "Loads the specified Accel EDA netlist file.";
fgw_error_t pcb_act_LoadAccelNetFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadAccelNetFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = rnd_gui->fileselect(rnd_gui, "Load pads ascii netlist file...",
																"Picks a pads ascii netlist file to load.\n",
																default_file, ".net", NULL, "accel_net", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(0);
	return accel_net_load(fname);
}

static int accel_net_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	unsigned int good = 0, limit;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported from a single file*/

	f = rnd_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(limit = 0; limit < 4; limit++) {
		char *s, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		while(isspace(*s)) s++;
		if (strncmp(s, "ACCEL_ASCII", 11) == 0) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);

	return 0;
}


static int accel_net_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(RND_MSG_ERROR, "import_accel_net: requires exactly 1 input file name\n");
		return -1;
	}
	return accel_net_load(fns[0]);
}

static pcb_plug_import_t import_accel_net;

rnd_action_t accel_net_action_list[] = {
	{"LoadAccelNetFrom", pcb_act_LoadAccelNetFrom, pcb_acth_LoadAccelNetFrom, pcb_acts_LoadAccelNetFrom}
};

int pplg_check_ver_import_accel_net(int ver_needed) { return 0; }

void pplg_uninit_import_accel_net(void)
{
	rnd_remove_actions_by_cookie(accel_net_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_accel_net);
	rnd_hid_menu_unload(rnd_gui, accel_net_cookie);
}

int pplg_init_import_accel_net(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	import_accel_net.plugin_data = NULL;

	import_accel_net.fmt_support_prio = accel_net_support_prio;
	import_accel_net.import           = accel_net_import;
	import_accel_net.name             = "accel_net";
	import_accel_net.desc             = "schamtics from accel EDA netlist";
	import_accel_net.ui_prio          = 50;
	import_accel_net.single_arg       = 1;
	import_accel_net.all_filenames    = 1;
	import_accel_net.ext_exec         = 0;

	RND_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_accel_net);

	RND_REGISTER_ACTIONS(accel_net_action_list, accel_net_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, accel_net_cookie, 175, NULL, 0, accel_net_menu, "plugin: import accel_net");
	return 0;
}
