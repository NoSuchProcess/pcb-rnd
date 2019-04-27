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

#include <stdlib.h>
#include "plugins.h"
#include "conf_hid.h"
#include "event.h"

#include "layer_menu.h"
#include "routest.h"
#include "toolbar.h"
#include "util.c"


static const char *layer_cookie = "lib_hid_pcbui/layer";
static const char *rst_cookie = "lib_hid_pcbui/route_style";
static const char *toolbar_cookie = "lib_hid_pcbui/toolbar";

static pcb_action_t rst_action_list[] = {
	{"AdjustStyle", pcb_act_AdjustStyle, pcb_acth_AdjustStyle, pcb_acts_AdjustStyle}
};
PCB_REGISTER_ACTIONS(rst_action_list, rst_cookie)

int pplg_check_ver_lib_hid_pcbui(int ver_needed) { return 0; }

static conf_hid_id_t conf_id;

void pplg_uninit_lib_hid_pcbui(void)
{
	pcb_remove_actions_by_cookie(rst_cookie);
	pcb_event_unbind_allcookie(layer_cookie);
	pcb_event_unbind_allcookie(rst_cookie);
	conf_hid_unreg(rst_cookie);
}

#include "dolists.h"

int pplg_init_lib_hid_pcbui(void)
{
TODO("padstack: remove some paths when route style has proto")
	const char **rp, *rpaths[] = {"design/line_thickness", "design/via_thickness", "design/via_drilling_hole", "design/clearance", NULL};
	static conf_hid_callbacks_t rcb[sizeof(rpaths)/sizeof(rpaths[0])];
	conf_native_t *nat;
	int n;

	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(rst_action_list, rst_cookie);

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layer_menu_vis_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_rst_gui_init_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_toolbar_gui_init_ev, NULL, toolbar_cookie);

	conf_id = conf_hid_reg(rst_cookie, NULL);
	for(rp = rpaths, n = 0; *rp != NULL; rp++, n++) {
		memset(&rcb[n], 0, sizeof(rcb[0]));
		rcb[n].val_change_post = pcb_rst_update_conf;
		nat = conf_get_field(*rp);
		if (nat != NULL)
			conf_hid_set_cb(nat, conf_id, &rcb[n]);
	}

	return 0;
}
