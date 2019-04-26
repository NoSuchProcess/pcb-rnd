/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "conf.h"
#include "conf_core.h"
#include "route_style.h"
#include "event.h"
#include "hid_cfg.h"
#include "hid.h"

#include "routest.h"

#define ANCH "@routestyles"

static void rst_install_menu(void *ctx, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	pcb_menu_prop_t props;
	char *end = path + strlen(path);
	char act[256], chk[256];
	int idx;

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.checked = chk;
	props.update_on = "";
	props.cookie = ANCH;

	pcb_hid_cfg_del_anchor_menus(node, ANCH);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	/* have to go reverse to keep order because this will insert items */
	for(idx = vtroutestyle_len(&PCB->RouteStyle)-1; idx >= 0; idx--) {
		sprintf(act, "RouteStyle(%d)", idx+1); /* for historical reasons this action counts from 1 */
		sprintf(chk, "ChkRst(%d)", idx);
		strcpy(end, PCB->RouteStyle.array[idx].name);
		pcb_gui->create_menu(path, &props);
	}
}

static int rst_lock = 0;
static void rst_update(void)
{
	if (rst_lock) return;
	rst_lock++;
	pcb_hid_cfg_map_anchor_menus(ANCH, rst_install_menu, NULL);
	rst_lock--;
}

void pcb_rst_update_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rst_update();
}

void pcb_rst_update_conf(conf_native_t *cfg, int arr_idx)
{
	if ((pcb_gui != NULL) && (pcb_gui->update_menu_checkbox != NULL))
		pcb_gui->update_menu_checkbox(NULL);
}
