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

/* ************************************************************ */

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
	PCB_REGISTER_ACTIONS(propedit_action_list, propedit_cookie)
	return 0;
}
