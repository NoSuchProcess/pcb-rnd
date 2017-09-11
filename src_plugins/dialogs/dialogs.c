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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"
#include "hid.h"
#include "hid_attrib.h"
#include "hid_dad.h"
#include "action_helper.h"

/* include them all for static inlines */
#include "dlg_test.c"
#include "dlg_layer_binding.c"

pcb_hid_action_t dialogs_action_list[] = {
	{"dlg_test", 0, pcb_act_dlg_test,
	 dlg_test_help, dlg_test_syntax},
	{"LayerBinding", 0, pcb_act_LayerBinding,
	 pcb_acth_LayerBinding, pcb_acts_LayerBinding}
};

static const char *dialogs_cookie = "dialogs plugin";

PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)

int pplg_check_ver_dialogs(int ver_needed) { return 0; }

void pplg_uninit_dialogs(void)
{
	pcb_hid_remove_actions_by_cookie(dialogs_cookie);
}

#include "dolists.h"
int pplg_init_dialogs(void)
{
	PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)
	return 0;
}
