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
#include "hid_dad.h"
#include "event.h"

#include "order.h"

static const char *order_cookie = "order plugin";

#define ANCH "@feature_plugins"

vtp0_t pcb_order_imps;

void pcb_order_reg(const pcb_order_imp_t *imp)
{
	vtp0_append(&pcb_order_imps, (void *)imp);
}


#include "order_dlg.c"

static const char pcb_acts_OrderPCB[] = "orderPCB([gui])";
static const char pcb_acth_OrderPCB[] = "Order the board from a fab";
fgw_error_t pcb_act_OrderPCB(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = "gui";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, OrderPCB, cmd = argv[1].val.str);

	if (strcmp(cmd, "gui") == 0) {
		PCB_ACT_IRES(order_dialog());
		return 0;
	}
	
	pcb_message(PCB_MSG_ERROR, "CLI version of OrderPCB() not yet implemented\n");
	PCB_ACT_IRES(-1);
	return 0;
}

static pcb_action_t order_action_list[] = {
	{"OrderPCB", pcb_act_OrderPCB, pcb_acth_OrderPCB, pcb_acts_OrderPCB}
};

PCB_REGISTER_ACTIONS(order_action_list, order_cookie)

static void order_install_menu(void *ctx, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	char *end = path + strlen(path);
	pcb_menu_prop_t props;
	char act[256];

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.tip = "Order your PCB from a fab\nusing a dialog box";
	props.accel = "<Key>f;<Key>x;<Key>o";
	props.cookie = ANCH;

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	strcpy(end, "Order PCB"); strcpy(act, "OrderPCB(gui)");
	pcb_gui->create_menu(pcb_gui, path, &props);
}


static void order_menu_init(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_hid_cfg_map_anchor_menus(ANCH, order_install_menu, NULL);
}

int pplg_check_ver_order(int ver_needed) { return 0; }

void pplg_uninit_order(void)
{
	pcb_remove_actions_by_cookie(order_cookie);
	pcb_event_unbind_allcookie(order_cookie);
}

#include "dolists.h"

int pplg_init_order(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(order_action_list, order_cookie)
	pcb_event_bind(PCB_EVENT_GUI_INIT, order_menu_init, NULL, order_cookie);
	return 0;
}
