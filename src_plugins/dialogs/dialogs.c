/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "hid.h"
#include "hid_attrib.h"
#include "actions.h"
#include "hid_dad.h"
#include "plugins.h"
#include "funchash_core.h"

/* include them all for static inlines */
#include "dlg_test.c"
#include "dlg_layer_binding.c"
#include "dlg_flag_edit.c"
#include "dlg_padstack.c"
#include "dlg_about.c"

pcb_action_t dialogs_action_list[] = {
	{"dlg_test", pcb_act_dlg_test, dlg_test_help, dlg_test_syntax},
	{"LayerBinding", pcb_act_LayerBinding, pcb_acth_LayerBinding, pcb_acts_LayerBinding},
	{"FlagEdit", pcb_act_FlagEdit, pcb_acth_FlagEdit, pcb_acts_FlagEdit},
	{"PadstackEdit", pcb_act_PadstackEdit, pcb_acth_PadstackEdit, pcb_acts_PadstackEdit},
	{"About2", pcb_act_About, pcb_acth_About, pcb_acts_About}
};

static const char *dialogs_cookie = "dialogs plugin";

PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)

int pplg_check_ver_dialogs(int ver_needed) { return 0; }

void pplg_uninit_dialogs(void)
{
	pcb_remove_actions_by_cookie(dialogs_cookie);
}

#include "dolists.h"
int pplg_init_dialogs(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)
	return 0;
}
