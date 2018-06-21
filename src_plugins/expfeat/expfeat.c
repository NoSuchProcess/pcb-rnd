/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "actions.h"


static const char pcb_acts_ExpFeatTmp[] = "ExpFeatTmp(...)";
static const char pcb_acth_ExpFeatTmp[] = "Experimental Feature Template.";
/* %start-doc actions Debug
This action is a template, copy and rename this to create new actions.
%end-doc */
static int pcb_act_ExpFeatTmp(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_message(PCB_MSG_INFO, "Hello world from expfeat!\n");
	return 0;
	PCB_OLD_ACT_END;
}


pcb_hid_action_t expfeat_action_list[] = {
	{"ExpFeatTmp", pcb_act_ExpFeatTmp, pcb_acth_ExpFeatTmp, pcb_acts_ExpFeatTmp}
};

static const char *expfeat_cookie = "experimental features plugin";

PCB_REGISTER_ACTIONS(expfeat_action_list, expfeat_cookie)

int pplg_check_ver_expfeat(int ver_needed) { return 0; }

void pplg_uninit_expfeat(void)
{
	pcb_hid_remove_actions_by_cookie(expfeat_cookie);
}

#include "dolists.h"
int pplg_init_expfeat(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(expfeat_action_list, expfeat_cookie)
	return 0;
}
