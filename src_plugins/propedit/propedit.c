/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdlib.h>
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "config.h"
#include "props.h"
#include "propsel.h"
#include "hid_actions.h"
#include "pcb-printf.h"
#include "error.h"

/* ************************************************************ */

extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;

#warning TODO
static const char propedit_syntax[] = "propedit()";
static const char propedit_help[] = "Run the property editor";
int propedit_action(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pe_ctx_t ctx;
	htsp_entry_t *pe;
	pcb_layer_t *ly = NULL;
	pcb_layergrp_t *lg = NULL;
	pcb_layer_id_t lid;
	pcb_layergrp_id_t gid;

	if ((pcb_gui == NULL) || (pcb_gui->propedit_start == NULL)) {
		pcb_message(PCB_MSG_ERROR, "Error: there's no GUI or the active GUI can't edit properties.\n");
		return 1;
	}

	ctx.core_props = pcb_props_init();
	for(lid = 0; lid < PCB->Data->LayerN; lid++)
		PCB->Data->Layer[lid].propedit = 0;
	for(gid = 0; gid < PCB->LayerGroups.len; gid++)
		PCB->LayerGroups.grp[gid].propedit = 0;
	propedit_board = 0;

	if (argc > 0) {
		int n;
		for(n = 0; n < argc; n++) {
			if (strcmp(argv[n], "board") == 0) {
				propedit_board = 1;
			}
			else if (strcmp(argv[n], "layers") == 0) {
				for(lid = 0; lid < PCB->Data->LayerN; lid++)
					PCB->Data->Layer[lid].propedit = 1;
			}
			else if (strncmp(argv[n], "layer:", 6) == 0) {
				const char *id = argv[n]+6;
				if (strcmp(id, "current") == 0)
					ly = CURRENT;
				else
					ly = pcb_get_layer(PCB->Data, atoi(id));
				if (ly == NULL) {
					pcb_message(PCB_MSG_ERROR, "Invalid layer index %s\n", argv[n]);
					goto err;
				}
				ly->propedit = 1;
			}
			if (strcmp(argv[n], "layer_groups") == 0) {
				for(gid = 0; gid < PCB->LayerGroups.len; gid++)
					PCB->LayerGroups.grp[gid].propedit = 1;
			}
			else if (strncmp(argv[n], "layer_group:", 12) == 0) {
				const char *id = argv[n]+12;
				if (strcmp(id, "current") == 0) {
					lg = pcb_get_layergrp(PCB, pcb_actd_EditGroup_gid);
					if ((lg == NULL) && (CURRENT != NULL) && (!CURRENT->is_bound))
						lg = pcb_get_layergrp(PCB, CURRENT->meta.real.grp);
				}
				else
					lg = pcb_get_layergrp(PCB, atoi(id));
				if (lg == NULL) {
					pcb_message(PCB_MSG_ERROR, "Invalid layer group index %s\n", argv[n]);
					goto err;
				}
				lg->propedit = 1;
			}
		}
	}

	pcb_propsel_map_core(ctx.core_props);

	pcb_gui->propedit_start(&ctx, ctx.core_props->fill, propedit_query);
	for (pe = htsp_first(ctx.core_props); pe; pe = htsp_next(ctx.core_props, pe))
		propedit_ins_prop(&ctx, pe);

	err:;
	pcb_gui->propedit_end(&ctx);
	pcb_props_uninit(ctx.core_props);
	return 0;
}

static const char propset_syntax[] = "propset(name, value)";
static const char propset_help[] = "Change the named property of all selected objects to/by value";
int propset_action(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int res;
/*
	if (argc != 2)
*/
	printf("argc=%d '%s'\n", argc, argv[0]);
	res = pcb_propsel_set(argv[0], argv[1]);
	printf("res=%d\n", res);
	return 0;
}

static const char *propedit_cookie = "propedit";

pcb_hid_action_t propedit_action_list[] = {
	{"propedit", 0, propedit_action,
	 propedit_help, propedit_syntax},
	{"propset", 0, propset_action,
	 propset_help, propset_syntax},
};

PCB_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)

int pplg_check_ver_propedit(int ver_needed) { return 0; }

void pplg_uninit_propedit(void)
{
	pcb_hid_remove_actions_by_cookie(propedit_cookie);
}

#include "dolists.h"
int pplg_init_propedit(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)
	return 0;
}
