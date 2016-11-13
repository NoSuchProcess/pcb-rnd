/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
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
static int ActionDumpConf(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *cmd = argc > 0 ? argv[0] : NULL;

	if (NSTRCMP(cmd, "native") == 0) {
		int verbose;
		const char *prefix = "";
		if (argc > 1)
			verbose = atoi(argv[1]);
		if (argc > 2)
			prefix = argv[2];
		conf_dump(stdout, prefix, verbose, NULL);
	}

	else if (NSTRCMP(cmd, "lihata") == 0) {
		conf_role_t role;
		const char *prefix = "";
		if (argc <= 1) {
			pcb_message(PCB_MSG_DEFAULT, "conf(dumplht) needs a role");
			return 1;
		}
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_DEFAULT, "Invalid role: '%s'", argv[1]);
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

static int ActionEvalConf(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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
static int ActionDumpLayers(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int g, n, used, arr[128]; /* WARNING: this assumes we won't have more than 128 layers */

	printf("Max: theoretical=%d current_board=%d\n", MAX_LAYER+2, max_copper_layer);
	for(n = 0; n < MAX_LAYER+2; n++) {
		int grp = GetGroupOfLayer(n);
		printf(" [%d] %04x group=%d %s\n", n, pcb_layer_flags(n), grp, PCB->Data->Layer[n].Name);
	}

	/* query by logical layer: any bottom copper */
	used = pcb_layer_list(PCB_LYT_COPPER | PCB_LYT_BOTTOM, arr, sizeof(arr)/sizeof(arr[0]));
	printf("All %d bottom copper layers are:\n", used);
	for(n = 0; n < used; n++) {
		int layer_id = arr[n];
		printf(" [%d] %s \n", layer_id, PCB->Data->Layer[layer_id].Name);
	}

	/* query by groups (physical layers): any copper in group */
	used = pcb_layer_group_list(PCB_LYT_COPPER, arr, sizeof(arr)/sizeof(arr[0]));
	printf("All %d groups containing copper layers are:\n", used);
	for(g = 0; g < used; g++) {
		int group_id = arr[g];
		printf(" group %d\n", group_id);
		for(n = 0; n < PCB->LayerGroups.Number[group_id]; n++) {
			int layer_id = PCB->LayerGroups.Entries[group_id][n];
			printf("  [%d] %s\n", layer_id, PCB->Data->Layer[layer_id].Name);
		}
	}

	return 0;
}


pcb_hid_action_t diag_action_list[] = {
	{"dumpconf", 0, ActionDumpConf,
	 dump_conf_help, dump_conf_syntax},
	{"dumplayers", 0, ActionDumpLayers,
	 dump_layers_help, dump_layers_syntax},
	{"EvalConf", 0, ActionEvalConf,
	 eval_conf_help, eval_conf_syntax}
};

static const char *diag_cookie = "debug plugin";

PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)

static void hid_diag_uninit(void)
{
	hid_remove_actions_by_cookie(diag_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_diag_init(void)
{
	PCB_REGISTER_ACTIONS(diag_action_list, diag_cookie)
	return hid_diag_uninit;
}
