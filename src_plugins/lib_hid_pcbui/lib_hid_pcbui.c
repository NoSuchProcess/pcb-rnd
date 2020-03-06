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
#include <librnd/core/plugins.h>
#include <librnd/core/conf_hid.h>
#include "event.h"

#include "layer_menu.h"
#include "layersel.h"
#include "routest.h"
#include "../../src_plugins/lib_hid_common/toolbar.h"
#include "status.h"
#include "act.h"

#include "util.c"
#include "rendering.c"
#include "infobar.c"
#include "title.c"


static const char *layer_cookie = "lib_hid_pcbui/layer";
static const char *rst_cookie = "lib_hid_pcbui/route_style";
static const char *act_cookie = "lib_hid_pcbui/actions";
static const char *status_cookie = "lib_hid_pcbui/status";
static const char *status_rd_cookie = "lib_hid_pcbui/status/readouts";
static const char *rendering_cookie = "lib_hid_pcbui/rendering";
static const char *infobar_cookie = "lib_hid_pcbui/infobar";
static const char *title_cookie = "lib_hid_pcbui/title";
static const char *layersel_cookie = "lib_hid_pcbui/layersel";

static pcb_action_t rst_action_list[] = {
	{"AdjustStyle", pcb_act_AdjustStyle, pcb_acth_AdjustStyle, pcb_acts_AdjustStyle}
};

static pcb_action_t status_action_list[] = {
	{"StatusSetText", pcb_act_StatusSetText, pcb_acth_StatusSetText, pcb_acts_StatusSetText},
	{"DescribeLocation", pcb_act_DescribeLocation, pcb_acth_DescribeLocation, pcb_acts_DescribeLocation}
};

static pcb_action_t act_action_list[] = {
	{"Zoom", pcb_act_Zoom, pcb_acth_Zoom, pcb_acts_Zoom},
	{"ZoomTo", pcb_act_Zoom, pcb_acth_Zoom, pcb_acts_Zoom},
	{"SwapSides", pcb_act_SwapSides, pcb_acth_SwapSides, pcb_acts_SwapSides},
	{"Popup", pcb_act_Popup, pcb_acth_Popup, pcb_acts_Popup},
	{"LayerHotkey", pcb_act_LayerHotkey, pcb_acth_LayerHotkey, pcb_acts_LayerHotkey}
};

int pplg_check_ver_lib_hid_pcbui(int ver_needed) { return 0; }


void pplg_uninit_lib_hid_pcbui(void)
{
	pcb_remove_actions_by_cookie(rst_cookie);
	pcb_remove_actions_by_cookie(status_cookie);
	pcb_remove_actions_by_cookie(act_cookie);
	pcb_event_unbind_allcookie(layer_cookie);
	pcb_event_unbind_allcookie(rst_cookie);
	pcb_event_unbind_allcookie(status_cookie);
	pcb_event_unbind_allcookie(rendering_cookie);
	pcb_event_unbind_allcookie(infobar_cookie);
	pcb_event_unbind_allcookie(title_cookie);
	pcb_event_unbind_allcookie(layersel_cookie);
	pcb_conf_hid_unreg(rst_cookie);
	pcb_conf_hid_unreg(status_cookie);
	pcb_conf_hid_unreg(status_rd_cookie);
	pcb_conf_hid_unreg(infobar_cookie);
	rnd_toolbar_uninit();
}

static conf_hid_id_t install_events(const char *cookie, const char *paths[], conf_hid_callbacks_t cb[], void (*update_cb)(conf_native_t*,int))
{
	const char **rp;
	conf_native_t *nat;
	int n;
	conf_hid_id_t conf_id;

	conf_id = pcb_conf_hid_reg(cookie, NULL);
	for(rp = paths, n = 0; *rp != NULL; rp++, n++) {
		memset(&cb[n], 0, sizeof(cb[0]));
		cb[n].val_change_post = update_cb;
		nat = pcb_conf_get_field(*rp);
		if (nat != NULL)
			pcb_conf_hid_set_cb(nat, conf_id, &cb[n]);
	}

	return conf_id;
}

int pplg_init_lib_hid_pcbui(void)
{
TODO("padstack: remove some paths when route style has proto")
	const char *rpaths[] = {"design/line_thickness", "design/via_thickness", "design/via_drilling_hole", "design/clearance", NULL};
	const char *stpaths[] = { "editor/show_solder_side", "design/line_thickness", "editor/all_direction_lines", "editor/line_refraction", "editor/rubber_band_mode", "design/via_thickness", "design/via_drilling_hole", "design/clearance", "design/text_scale", "design/text_thickness", "editor/buffer_number", "editor/grid_unit", "editor/grid", "appearance/compact", NULL };
	const char *rdpaths[] = { "editor/grid_unit", "appearance/compact", NULL };
	const char *ibpaths[] = { "rc/file_changed_interval", NULL };
	static conf_hid_callbacks_t rcb[sizeof(rpaths)/sizeof(rpaths[0])];
	static conf_hid_callbacks_t stcb[sizeof(stpaths)/sizeof(stpaths[0])];
	static conf_hid_callbacks_t rdcb[sizeof(rdpaths)/sizeof(rdpaths[0])];
	static conf_hid_callbacks_t ibcb[sizeof(rdpaths)/sizeof(ibpaths[0])];

	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(rst_action_list, rst_cookie);
	PCB_REGISTER_ACTIONS(status_action_list, status_cookie);
	PCB_REGISTER_ACTIONS(act_action_list, act_cookie);

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layersel_stack_chg_ev, NULL, layersel_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layer_menu_vis_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layersel_vis_chg_ev, NULL, layersel_cookie);
	pcb_event_bind(PCB_EVENT_LAYER_KEY_CHANGE, pcb_layer_menu_key_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_layersel_gui_init_ev, NULL, layersel_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_rst_gui_init_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_status_gui_init_ev, NULL, status_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_rendering_gui_init_ev, NULL, rendering_cookie);
	pcb_event_bind(PCB_EVENT_USER_INPUT_KEY, pcb_status_st_update_ev, NULL, status_cookie);
	pcb_event_bind(PCB_EVENT_CROSSHAIR_MOVE, pcb_status_rd_update_ev, NULL, status_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_infobar_brdchg_ev, NULL, infobar_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_FN_CHANGED, pcb_infobar_fn_chg_ev, NULL, infobar_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_infobar_gui_init_ev, NULL, infobar_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_title_gui_init_ev, NULL, title_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_title_board_changed_ev, NULL, title_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, pcb_title_meta_changed_ev, NULL, title_cookie);

	install_events(rst_cookie, rpaths, rcb, pcb_rst_update_conf);
	install_events(status_cookie, stpaths, stcb, pcb_status_st_update_conf);
	install_events(status_rd_cookie, rdpaths, rdcb, pcb_status_rd_update_conf);
	install_events(infobar_cookie, ibpaths, ibcb, pcb_infobar_update_conf);

	rnd_toolbar_init();

	return 0;
}
