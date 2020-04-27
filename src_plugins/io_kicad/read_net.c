/*
 *				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Schematics/netlist import action for KiCad's eeschema */

#include "config.h"

#include <assert.h>
#include <gensexpr/gsxl.h>

#include "read_net.h"

#include "board.h"
#include "data.h"
#include "plug_import.h"
#include <librnd/core/error.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

#define if_strval(node, name) \
	if (strcmp(node->str, #name) == 0) { \
		if (name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Invalid eeschema: multiple %s subtrees\n", #name); \
			return -1; \
		} \
		if (node->children != NULL) \
			name = node->children->str; \
	}

#define if_subtree(node, name) \
	if (strcmp(node->str, #name) == 0) { \
		if (name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Invalid eeschema: multiple %s subtrees\n", #name); \
			return -1; \
		} \
		name = node; \
	}

#define req_subtree(name) \
	if (name == NULL) { \
		pcb_message(PCB_MSG_ERROR, "Invalid eeschema: missing %s subtree\n", #name); \
		return -1; \
	} \


static int eeschema_parse_net(gsxl_dom_t *dom)
{
	gsxl_node_t *i, *net, *c, *n, *version = NULL, *components = NULL, *nets = NULL;

	/* check the header */
	if (strcmp(dom->root->str, "export") != 0) {
		pcb_message(PCB_MSG_ERROR, "Invalid eeschema netlist header: not an export\n");
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
		pcb_message(PCB_MSG_ERROR, "Invalid eeschema version: expected 'D', got '%s'\n", version->children->str);
		return -1;
	}

	/* Load the elements */
	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);

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
			pcb_message(PCB_MSG_WARNING, "eeschema: ignoring component with no refdes\n");
			continue;
		}
		if (footprint == NULL) {
			pcb_message(PCB_MSG_WARNING, "eeschema: ignoring component %s with no footprint\n", ref);
			continue;
		}
		rnd_actionva(&PCB->hidlib, "ElementList", "Need", ref, footprint, value == NULL ? "" : value, NULL);
	}

	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);

	/* Load the netlist */

	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	for(net = nets->children; net != NULL; net = net->next) {
		const char *netname = NULL, *code = NULL, *name = NULL;
		char refpin[256];

		if (strcmp(net->str, "net") != 0)
			continue;

		for(n = net->children; n != NULL; n = n->next) {
			if_strval(n, code)
			else if_strval(n, name)
			else if (strcmp(n->str, "node") == 0) {
				const char *ref = NULL, *pin = NULL;

				/* load node params */
				for(i = n->children; i != NULL; i = i->next) {
					if_strval(i, ref)
					else if_strval(i, pin)
				}

				/* find out the net name */
				if (netname == NULL) {
					if ((name != NULL) && (*name != '\0'))
						netname = name;
					else
						netname = code;
				}
				if (netname == NULL) {
					pcb_message(PCB_MSG_WARNING, "eeschema: ignoring pins of incomplete net\n");
					continue;
				}

				/* do the binding */
				if ((ref == NULL) || (pin == NULL)) {
					pcb_message(PCB_MSG_WARNING, "eeschema: ignoring incomplete connection to net %s: refdes=%s pin=%s \n", netname, ref, pin);
					continue;
				}
				pcb_snprintf(refpin, sizeof(refpin), "%s-%s", ref, pin);
				rnd_actionva(&PCB->hidlib, "Netlist", "Add",  netname, refpin, NULL);
			}
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);

	return 0;
}



static int eeschema_load(const char *fname_net)
{
	FILE *fn;
	gsxl_dom_t dom;
	int c, ret = 0;
	gsx_parse_res_t res;

	fn = pcb_fopen(&PCB->hidlib, fname_net, "r");
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
fgw_error_t pcb_act_LoadeeschemaFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadeeschemaFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect(pcb_gui, "Load eeschema netlist file...",
					"Picks a eeschema netlist file to load.\n",
					default_file, ".net", NULL, "eeschema", PCB_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	RND_ACT_IRES(eeschema_load(fname));
	return 0;
}


static int eeschema_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	unsigned int n, good = 0;
	FILE *f;

	if ((aspects != IMPORT_ASPECT_NETLIST) && (numargs == 1))
		return 0; /* only pure netlist import is supported with a single input file */

	f = pcb_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	for(n = 0; n < 32; n++) {
		char *s, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		while(isspace(*s)) s++;
		if (strstr(s, "(design") != NULL)
			good |= 1;
		else if (strstr(s, "(export") != NULL)
			good |= 2;
		if (good == (1|2)) {
			fclose(f);
			return 100;
		}
	}

	fclose(f);
	return 0;
}


static int eeschema_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		pcb_message(PCB_MSG_ERROR, "import_eeschema: requires exactly 1 input file name\n");
		return -1;
	}
	return eeschema_load(fns[0]);
}

static pcb_plug_import_t import_eeschema;

void pcb_eeschema_uninit(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_eeschema);
}

void pcb_eeschema_init(void)
{
	/* register the IO hook */
	import_eeschema.plugin_data = NULL;

	import_eeschema.fmt_support_prio = eeschema_support_prio;
	import_eeschema.import           = eeschema_import;
	import_eeschema.name             = "eeschema";
	import_eeschema.desc             = "schamtics from KiCad's eeschema";
	import_eeschema.ui_prio          = 50;
	import_eeschema.single_arg       = 1;
	import_eeschema.all_filenames    = 1;
	import_eeschema.ext_exec         = 0;

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_eeschema);
}
