/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
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
#include "board.h"
#include "data.h"
#include "layer.h"
#include "diag_conf.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"

static const char dump_conf_syntax[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"

	;

static const char dump_conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static int pcb_act_DumpConf(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *cmd = argc > 0 ? argv[0] : NULL;

	if (PCB_NSTRCMP(cmd, "native") == 0) {
		int verbose;
		const char *prefix = "";
		if (argc > 1)
			verbose = atoi(argv[1]);
		if (argc > 2)
			prefix = argv[2];
		conf_dump(stdout, prefix, verbose, NULL);
	}

	else if (PCB_NSTRCMP(cmd, "lihata") == 0) {
		conf_role_t role;
		const char *prefix = "";
		if (argc <= 1) {
			pcb_message(PCB_MSG_ERROR, "conf(dumplht) needs a role");
			return 1;
		}
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", argv[1]);
			return 1;
		}
		if (argc > 2)
			prefix = argv[2];
		if (conf_root[role] != NULL)
			lht_dom_export(conf_root[role]->root, stdout, prefix);
		else
			printf("%s <empty>\n", prefix);
	}

	else {
		pcb_message(PCB_MSG_ERROR, "Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}

static const char eval_conf_syntax[] =
	"EvalConf(path) - evaluate a config path in different config sources to figure how it ended up in the native database\n"
	;

static const char eval_conf_help[] = "Perform various operations on the configuration tree.";

static int pcb_act_EvalConf(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *path = argc > 0 ? argv[0] : NULL;
	conf_native_t *nat;
	int role;

	if (path == NULL) {
		pcb_message(PCB_MSG_ERROR, "EvalConf needs a path\n");
		return 1;
	}

	nat = conf_get_field(path);
	if (nat == NULL) {
		pcb_message(PCB_MSG_ERROR, "EvalConf: invalid path %s - no such config setting\n", path);
		return 1;
	}

	printf("Conf node %s\n", path);
	for(role = 0; role < CFR_max_real; role++) {
		lht_node_t *n;
		printf(" Role: %s\n", conf_role_name(role));
		n = conf_lht_get_at(role, path, 0);
		if (n != NULL) {
			conf_policy_t pol = -1;
			long prio = conf_default_prio[role];


			if (conf_get_policy_prio(n, &pol, &prio) == 0)
				printf("  * policy=%s\n  * prio=%ld\n", conf_policy_name(pol), prio);

			if (n->file_name != NULL)
				printf("  * from=%s:%d.%d\n", n->file_name, n->line, n->col);
			else
				printf("  * from=(unknown)\n");

			lht_dom_export(n, stdout, "  ");
		}
		else
			printf("  * not present\n");
	}

	printf(" Native:\n");
	conf_print_native((conf_pfn)pcb_fprintf, stdout, "  ", 1, nat);

	return 0;
}

static const char dump_layers_syntax[] =
	"dumplayers()\n"
	;

static const char dump_layers_help[] = "Print info about each layer";

extern lht_doc_t *conf_root[];
static int pcb_act_DumpLayers(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int g, n, used;
	pcb_layer_id_t arr[128]; /* WARNING: this assumes we won't have more than 128 layers */
	pcb_layergrp_id_t garr[128]; /* WARNING: this assumes we won't have more than 128 layers */


	printf("Max: theoretical=%d current_board=%d\n", PCB_MAX_LAYER+2, pcb_max_layer);
	used = pcb_layer_list_any(PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL, arr, sizeof(arr)/sizeof(arr[0]));
	for(n = 0; n < used; n++) {
		pcb_layer_id_t layer_id = arr[n];
		pcb_layergrp_id_t grp = pcb_layer_get_group(PCB, layer_id);
		printf(" [%lx] %04x group=%ld %s\n", layer_id, pcb_layer_flags(layer_id), grp, pcb_layer_name(layer_id));
	}

	/* query by logical layer: any bottom copper */
	used = pcb_layer_list(PCB_LYT_COPPER | PCB_LYT_BOTTOM, arr, sizeof(arr)/sizeof(arr[0]));
	printf("All %d bottom copper layers are:\n", used);
	for(n = 0; n < used; n++) {
		pcb_layer_id_t layer_id = arr[n];
		printf(" [%lx] %s \n", layer_id, PCB->Data->Layer[layer_id].Name);
	}

	/* query by groups (physical layers): any copper in group */
	used = pcb_layergrp_list(PCB, PCB_LYT_COPPER, garr, sizeof(garr)/sizeof(garr[0]));
	printf("All %d groups containing copper layers are:\n", used);
	for(g = 0; g < used; g++) {
		pcb_layergrp_id_t group_id = garr[g];
		printf(" group %ld (%d layers)\n", group_id, PCB->LayerGroups.grp[group_id].len);
		for(n = 0; n < PCB->LayerGroups.grp[group_id].len; n++) {
			pcb_layer_id_t layer_id = PCB->LayerGroups.grp[group_id].lid[n];
			printf("  [%lx] %s\n", layer_id, PCB->Data->Layer[layer_id].Name);
		}
	}

	return 0;
}


static void print_font(pcb_font_t *f, const char *prefix)
{
	int n, g = 0, gletter = 0, gdigit = 0;
	const char *name;

	/* count valid glyphs and classes */
	for(n = 0; n < PCB_MAX_FONTPOSITION + 1; n++) {
		if (f->Symbol[n].Valid) {
			g++;
			if (isalpha(n)) gletter++;
			if (isdigit(n)) gdigit++;
		}
	}

	name = f->name == NULL ? "<anon>" : f->name;
	pcb_printf("%s: %d %s; dim: %$$mm * %$$mm glyphs: %d (letter: %d, digit: %d)\n", prefix, f->id, name, f->MaxWidth, f->MaxHeight, g, gletter, gdigit);
}

static const char dump_fonts_syntax[] = "dumpfonts()\n";
static const char dump_fonts_help[] = "Print info about fonts";
static int pcb_act_DumpFonts(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	printf("Font summary:\n");
	print_font(&PCB->fontkit.dflt, " Default");
	if (PCB->fontkit.hash_inited) {
		htip_entry_t *e;
		for (e = htip_first(&PCB->fontkit.fonts); e; e = htip_next(&PCB->fontkit.fonts, e))
			print_font(e->value, " Extra  ");
	}
	else
		printf(" <no extra font loaded>\n");
}

#ifndef NDEBUG
static const char dump_undo_syntax[] = "dumpfonts()\n";
static const char dump_undo_help[] = "Print info about fonts";
static int pcb_act_DumpUndo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	printf("Undo:\n");
	undo_dump();
}
#endif


pcb_hid_action_t diag_action_list[] = {
	{"dumpconf", 0, pcb_act_DumpConf,
	 dump_conf_help, dump_conf_syntax},
	{"dumplayers", 0, pcb_act_DumpLayers,
	 dump_layers_help, dump_layers_syntax},
	{"dumpfonts", 0, pcb_act_DumpFonts,
	 dump_fonts_help, dump_fonts_syntax},
#ifndef NDEBUG
	{"dumpundo", 0, pcb_act_DumpUndo,
	 dump_undo_help, dump_undo_syntax},
#endif
	{"EvalConf", 0, pcb_act_EvalConf,
	 eval_conf_help, eval_conf_syntax}

};

static const char *diag_cookie = "diag plugin";

PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)

int pplg_check_ver_diag(int ver_needed) { return 0; }

void pplg_uninit_diag(void)
{
	pcb_hid_remove_actions_by_cookie(diag_cookie);
}

#include "dolists.h"
int pplg_init_diag(void)
{
	PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)
	return 0;
}
