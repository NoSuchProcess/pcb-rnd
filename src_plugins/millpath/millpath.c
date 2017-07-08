/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "action_helper.h"
#include "plugins.h"
#include "hid_actions.h"

static const char *millpath_cookie = "millpath plugin";


pcb_hid_action_t millpath_action_list[] = {
0
};

PCB_REGISTER_ACTIONS(millpath_action_list, millpath_cookie)

int pplg_check_ver_millpath(int ver_needed) { return 0; }

void pplg_uninit_millpath(void)
{
	pcb_hid_remove_actions_by_cookie(millpath_cookie);
}


#include "dolists.h"
int pplg_init_millpath(void)
{
	PCB_REGISTER_ACTIONS(millpath_action_list, millpath_cookie)
	return 0;
}
