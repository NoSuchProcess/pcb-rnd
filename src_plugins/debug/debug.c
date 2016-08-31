/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
#include "global.h"
#include "data.h"
#include "layer.h"
#include "debug_conf.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"

static const char conf_syntax[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	;

static const char conf_help[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
static int ActionDumpConf(int argc, char **argv, Coord x, Coord y)
{
	char *cmd = argc > 0 ? argv[0] : 0;

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
			Message("conf(dumplht) needs a role");
			return 1;
		}
		role = conf_role_parse(argv[1]);
		if (role == CFR_invalid) {
			Message("Invalid role: '%s'", argv[1]);
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
		Message("Invalid conf command '%s'\n", argv[0]);
		return 1;
	}
	return 0;
}

static const char dump_layers_syntax[] =
	"dumplayers()\n"
	;

static const char dump_layers_help[] = "Print info about each layer";

extern lht_doc_t *conf_root[];
static int ActionDumpLayers(int argc, char **argv, Coord x, Coord y)
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


HID_Action debug_action_list[] = {
	{"dumpconf", 0, ActionDumpConf,
	 conf_help, conf_syntax},
	{"dumplayers", 0, ActionDumpLayers,
	 dump_layers_help, dump_layers_syntax}
};

static const char *debug_cookie = "debug plugin";

REGISTER_ACTIONS(debug_action_list, debug_cookie)

static void hid_debug_uninit(void)
{
	hid_remove_actions_by_cookie(debug_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_debug_init(void)
{
	REGISTER_ACTIONS(debug_action_list, debug_cookie)
	return hid_debug_uninit;
}

