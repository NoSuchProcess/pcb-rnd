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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdlib.h>
#include "plugins.h"
#include "conf_hid.h"

#include "grid_menu.h"
#include "layer_menu.h"

static const char *grid_cookie = "lib_hid_common/grid";
static const char *layer_cookie = "lib_hid_common/layer";

int pplg_check_ver_lib_hid_common(int ver_needed) { return 0; }

static conf_hid_id_t conf_id;

void pplg_uninit_lib_hid_common(void)
{
	pcb_event_unbind_allcookie(grid_cookie);
	pcb_event_unbind_allcookie(layer_cookie);
	conf_hid_unreg(grid_cookie);
}

int pplg_init_lib_hid_common(void)
{
	static conf_hid_callbacks_t ccb;
	conf_native_t *nat;

	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_grid_update_ev, NULL, grid_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_layer_menu_update_ev, NULL, layer_cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_layer_menu_vis_update_ev, NULL, layer_cookie);

	conf_id = conf_hid_reg(grid_cookie, NULL);
	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = pcb_grid_update_conf;
	nat = conf_get_field("editor/grids");

	if (nat != NULL)
		conf_hid_set_cb(nat, conf_id, &ccb);
	return 0;
}
