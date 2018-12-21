/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "config.h"
#include "props.h"
#include "propsel.h"
#include "propdlg.h"
#include "actions.h"
#include "pcb-printf.h"
#include "error.h"
#include "layer.h"
#include "layer_grp.h"
#include "search.h"
#include "crosshair.h"


int prop_scope_add(pcb_propedit_t *pe, const char *cmd, int quiet)
{
	pcb_idpath_t *idp;
	long id;

	if (strncmp(cmd, "object", 6) == 0) {
		if (cmd[6] == ':') {
			idp = pcb_str2idpath(cmd+7);
			if (idp == NULL) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Failed to convert object ID: '%s'\n", cmd+7);
				return FGW_ERR_ARG_CONV;
			}
			pcb_idpath_list_append(&pe->objs, idp);
		}
		else {
			void *o1, *o2, *o3;
			pcb_objtype_t type;
			type = pcb_search_obj_by_location(PCB_OBJ_CLASS_REAL, &o1, &o2, &o3, pcb_crosshair.X, pcb_crosshair.Y, PCB_SLOP);
			if (type == 0) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "No object under the cursor\n");
				return FGW_ERR_ARG_CONV;
			}
			idp = pcb_obj2idpath(o2);
			if (idp == NULL) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Object under the cursor has no idpath\n");
				return FGW_ERR_ARG_CONV;
			}
			pcb_idpath_list_append(&pe->objs, idp);
		}
	}
	else if (strncmp(cmd, "layer", 5) == 0) {
		if (cmd[5] == ':') {
			id = pcb_layer_str2id(pe->pcb->Data, cmd+6);
			if (id < 0) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Invalid layer ID '%s'\n", cmd+6);
				return FGW_ERR_ARG_CONV;
			}
			vtl0_append(&pe->layers, id);
		}
		else {
			vtl0_append(&pe->layers, INDEXOFCURRENT);
		}
	}
	else if (strncmp(cmd, "layergrp", 8) == 0) {
		if (cmd[8] == ':') {
			id = pcb_layergrp_str2id(pe->pcb, cmd+9);
			if (id < 0) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Invalid layergrp ID '%s'\n", cmd+9);
				return FGW_ERR_ARG_CONV;
			}
			vtl0_append(&pe->layergrps, id);
		}
		else {
			vtl0_append(&pe->layergrps, CURRENT->meta.real.grp);
		}
	}
	else if (strcmp(cmd, "pcb") == 0)
		pe->board = 1;
	else if (strcmp(cmd, "selection") == 0)
		pe->selection = 1;
	else {
		if (!quiet)
			pcb_message(PCB_MSG_ERROR, "Invalid scope: %s\n", cmd);
		return FGW_ERR_ARG_CONV;
	}
	return 0;
}


extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;

static const char pcb_acts_propset[] = "propset(name, value)";
static const char pcb_acth_propset[] = "Change the named property of all selected objects to/by value";
fgw_error_t pcb_act_propset(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name, *val;

	PCB_ACT_CONVARG(1, FGW_STR, propset, name = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, propset, val = argv[2].val.str);

	PCB_ACT_IRES(pcb_propsel_set(name, val));
	return 0;
}

static const char *propedit_cookie = "propedit";

pcb_action_t propedit_action_list[] = {
	{"propedit", pcb_act_propedit, pcb_acth_propedit, pcb_acts_propedit},
	{"propset", pcb_act_propset, pcb_acth_propset, pcb_acts_propset}
};

PCB_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)

int pplg_check_ver_propedit(int ver_needed) { return 0; }

void pplg_uninit_propedit(void)
{
	pcb_remove_actions_by_cookie(propedit_cookie);
}

#include "dolists.h"
int pplg_init_propedit(void)
{
	PCB_API_CHK_VER;

	if (sizeof(long) < sizeof(pcb_layer_id_t)) {
		pcb_message(PCB_MSG_ERROR, "can't load propedig: layer id type wider than long\n");
		return -1;
	}

	if (sizeof(long) < sizeof(pcb_layergrp_id_t)) {
		pcb_message(PCB_MSG_ERROR, "can't load propedig: layergrp id type wider than long\n");
		return -1;
	}

	PCB_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)
	return 0;
}
