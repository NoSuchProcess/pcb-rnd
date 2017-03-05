/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist.h"

#include "plugins.h"
#include "hid.h"
#include "hid_actions.h"


static const char *tedax_cookie = "tEDAx IO";

pcb_hid_action_t tedax_action_list[] = {
	{"LoadTedaxFrom", 0, pcb_act_LoadtedaxFrom, pcb_acth_LoadtedaxFrom, pcb_acts_LoadtedaxFrom}
};

PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)

static void hid_tedax_uninit()
{
	pcb_hid_remove_actions_by_cookie(tedax_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_io_tedax_init()
{
	PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)
	return hid_tedax_uninit;
}
