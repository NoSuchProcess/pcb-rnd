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
#include "status.h"
#include "act.h"
#include "util.c"


static const char *layer_cookie = "lib_hid_pcbui/layer";
static const char *rst_cookie = "lib_hid_pcbui/route_style";
static const char *act_cookie = "lib_hid_pcbui/actions";
static const char *toolbar_cookie = "lib_hid_pcbui/toolbar";
static const char *status_cookie = "lib_hid_pcbui/status";
static const char *status_rd_cookie = "lib_hid_pcbui/status/readouts";

static pcb_action_t rst_action_list[] = {
	{"AdjustStyle", pcb_act_AdjustStyle, pcb_acth_AdjustStyle, pcb_acts_AdjustStyle}
};
PCB_REGISTER_ACTIONS(rst_action_list, rst_cookie)

static pcb_action_t status_action_list[] = {
	{"StatusSetText", pcb_act_StatusSetText, pcb_acth_StatusSetText, pcb_acts_StatusSetText},
	{"DescribeLocation", pcb_act_DescribeLocation, pcb_acth_DescribeLocation, pcb_acts_DescribeLocation}
};
PCB_REGISTER_ACTIONS(status_action_list, status_cookie)

static pcb_action_t act_action_list[] = {
	{"Zoom", pcb_act_Zoom, pcb_acth_Zoom, pcb_acts_Zoom},
	{"ZoomTo", pcb_act_Zoom, pcb_acth_Zoom, pcb_acts_Zoom},
	{"Pan", pcb_act_Pan, pcb_acth_Pan, pcb_acts_Pan},
	{"Center", pcb_act_Center, pcb_acth_Center, pcb_acts_Center},
	{"Scroll", pcb_act_Scroll, pcb_acth_Scroll, pcb_acts_Scroll}
};
PCB_REGISTER_ACTIONS(act_action_list, act_cookie)

int pplg_check_ver_lib_hid_pcbui(int ver_needed) { return 0; }


void pplg_uninit_lib_hid_pcbui(void)
{
	pcb_remove_actions_by_cookie(rst_cookie);
	pcb_remove_actions_by_cookie(status_cookie);
	pcb_remove_actions_by_cookie(act_cookie);
	pcb_event_unbind_allcookie(layer_cookie);
	pcb_event_unbind_allcookie(rst_cookie);
	pcb_event_unbind_allcookie(toolbar_cookie);
	pcb_event_unbind_allcookie(status_cookie);
	conf_hid_unreg(rst_cookie);
	conf_hid_unreg(toolbar_cookie);
	conf_hid_unreg(status_cookie);
	conf_hid_unreg(status_rd_cookie);
}

#include "dolists.h"


static conf_hid_id_t install_events(const char *cookie, const char *paths[], conf_hid_callbacks_t cb[], void (*update_cb)(conf_native_t*,int))
{
	const char **rp;
	conf_native_t *nat;
	int n;
	conf_hid_id_t conf_id;

	conf_id = conf_hid_reg(cookie, NULL);
	for(rp = paths, n = 0; *rp != NULL; rp++, n++) {
		memset(&cb[n], 0, sizeof(cb[0]));
		cb[n].val_change_post = update_cb;
		nat = conf_get_field(*rp);
		if (nat != NULL)
			conf_hid_set_cb(nat, conf_id, &cb[n]);
	}

	return conf_id;
}

int pplg_init_lib_hid_pcbui(void)
{
TODO("padstack: remove some paths when route style has proto")
	const char *rpaths[] = {"design/line_thickness", "design/via_thickness", "design/via_drilling_hole", "design/clearance", NULL};
	const char *tpaths[] = {"editor/mode",  NULL};
	const char *stpaths[] = { "editor/show_solder_side", "design/line_thickness", "editor/all_direction_lines", "editor/line_refraction", "editor/rubber_band_mode", "design/via_thickness", "design/via_drilling_hole", "design/clearance", "design/text_scale", "design/text_thickness", "editor/buffer_number", "editor/grid_unit", "appearance/compact", NULL };
	const char *rdpaths[] = { "editor/grid_unit", "appearance/compact", NULL };
	static conf_hid_callbacks_t rcb[sizeof(rpaths)/sizeof(rpaths[0])];
	static conf_hid_callbacks_t tcb[sizeof(tpaths)/sizeof(tpaths[0])];
	static conf_hid_callbacks_t stcb[sizeof(stpaths)/sizeof(stpaths[0])];
	static conf_hid_callbacks_t rdcb[sizeof(rdpaths)/sizeof(rdpaths[0])];

	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(rst_action_list, rst_cookie);
	PCB_REGISTER_ACTIONS(status_action_list, status_cookie);
	PCB_REGISTER_ACTIONS(act_action_list, act_cookie);

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layer_menu_vis_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_rst_gui_init_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_toolbar_gui_init_ev, NULL, toolbar_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_status_gui_init_ev, NULL, status_cookie);
	pcb_event_bind(PCB_EVENT_USER_INPUT_KEY, pcb_status_st_update_ev, NULL, status_cookie);
	pcb_event_bind(PCB_EVENT_CROSSHAIR_MOVE, pcb_status_rd_update_ev, NULL, status_cookie);

	install_events(rst_cookie, rpaths, rcb, pcb_rst_update_conf);
	install_events(toolbar_cookie, tpaths, tcb, pcb_toolbar_update_conf);
	install_events(status_cookie, stpaths, stcb, pcb_status_st_update_conf);
	install_events(status_rd_cookie, rdpaths, rdcb, pcb_status_rd_update_conf);

	return 0;
}
