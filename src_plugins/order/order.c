/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>

#include "actions.h"
#include "pcb-printf.h"
#include "plugins.h"

static const char *order_cookie = "order plugin";

static const char pcb_acts_OrderPCB[] = "orderPCB()";
static const char pcb_acth_OrderPCB[] = "Order the board from a fab";
fgw_error_t pcb_act_OrderPCB(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_message(PCB_MSG_ERROR, "OrderPCB() not yet implemented\n");
	PCB_ACT_IRES(-1);
	return 0;
}

static pcb_action_t order_action_list[] = {
	{"OrderPCB", pcb_act_OrderPCB, pcb_acth_OrderPCB, pcb_acts_OrderPCB}
};

PCB_REGISTER_ACTIONS(order_action_list, order_cookie)


int pplg_check_ver_order(int ver_needed) { return 0; }

void pplg_uninit_order(void)
{
	pcb_remove_actions_by_cookie(order_cookie);
}

#include "dolists.h"

int pplg_init_order(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(order_action_list, order_cookie)
	return 0;
}
