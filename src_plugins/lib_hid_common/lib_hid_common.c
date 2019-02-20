/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

#include "grid_menu.h"
#include "layer_menu.h"
#include "routest_menu.h"
#include "cli_history.h"
#include "util.c"

#include "lib_hid_common.h"
#include "lib_hid_common_conf.h"

conf_lib_hid_common_t lib_hid_common_conf;


static const char *grid_cookie = "lib_hid_common/grid";
static const char *layer_cookie = "lib_hid_common/layer";
static const char *rst_cookie = "lib_hid_common/route_style";

int pplg_check_ver_lib_hid_common(int ver_needed) { return 0; }

static conf_hid_id_t conf_id;

void pplg_uninit_lib_hid_common(void)
{
	pcb_clihist_save();
	pcb_clihist_uninit();
	pcb_event_unbind_allcookie(grid_cookie);
	pcb_event_unbind_allcookie(layer_cookie);
	pcb_event_unbind_allcookie(rst_cookie);
	conf_hid_unreg(grid_cookie);
	conf_hid_unreg(rst_cookie);
	conf_unreg_fields("plugins/lib_hid_common/");
}

int pplg_init_lib_hid_common(void)
{
TODO("padstack: remove some paths when route style has proto")
	const char **rp, *rpaths[] = {"design/line_thickness", "design/via_thickness", "design/via_drilling_hole", "design/clearance", NULL};
	static conf_hid_callbacks_t ccb, rcb[sizeof(rpaths)/sizeof(rpaths[0])];
	conf_native_t *nat;
	int n;

	PCB_API_CHK_VER;
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(lib_hid_common_conf, field,isarray,type_name,cpath,cname,desc,flags);
#include "lib_hid_common_conf_fields.h"

	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_grid_update_ev, NULL, grid_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layer_menu_vis_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pcb_rst_update_ev, NULL, rst_cookie);
	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_rst_update_ev, NULL, rst_cookie);

	conf_id = conf_hid_reg(grid_cookie, NULL);
	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = pcb_grid_update_conf;
	nat = conf_get_field("editor/grids");

	if (nat != NULL)
		conf_hid_set_cb(nat, conf_id, &ccb);

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
