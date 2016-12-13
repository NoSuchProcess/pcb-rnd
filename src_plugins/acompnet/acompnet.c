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
#include "layer_ui.h"
/*#include "acompnet_conf.h"*/
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "conf.h"
#include "error.h"

static const char pcb_acts_acompnet[] = "acompnet()\n" ;
static const char pcb_acth_acompnet[] = "Attempt to auto-complete the current network";

static int pcb_act_acompnet(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

pcb_hid_action_t acompnet_action_list[] = {
	{"acompnet", 0, pcb_act_acompnet,
	 pcb_acth_acompnet, pcb_acts_acompnet},
};

static const char *acompnet_cookie = "acompnet plugin";

PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)

static void hid_acompnet_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(acompnet_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_acompnet_init(void)
{
	PCB_REGISTER_ACTIONS(acompnet_action_list, acompnet_cookie)
	return hid_acompnet_uninit;
}
