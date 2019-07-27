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

#include "conf.h"
#include "hidlib_conf.h"
#include "grid.h"
#include "event.h"
#include "hid_cfg.h"
#include "hid.h"

#include "grid_menu.h"

#define ANCH "@grid"

static pcb_conf_resolve_t grids_idx = {"editor.grids_idx", CFN_INTEGER, 0, NULL};

static void grid_install_menu(void *ctx, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	conf_native_t *nat;
	pcb_conflist_t *lst;
	pcb_conf_listitem_t *li;
	char *end = path + strlen(path);
	pcb_menu_prop_t props;
	char act[256], chk[256];
	int idx;

	nat = pcb_conf_get_field("editor/grids");
	if (nat == NULL)
		return;

	if (nat->type != CFN_LIST) {
		pcb_message(PCB_MSG_ERROR, "grid_install_menu(): conf node editor/grids should be a list\n");
		return;
	}

	lst = nat->val.list;

	pcb_conf_resolve(&grids_idx);

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.checked = chk;
	props.update_on = "editor/grids_idx";
	props.cookie = ANCH;

	pcb_hid_cfg_del_anchor_menus(node, ANCH);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	/* have to go reverse to keep order because this will insert items */
	idx = pcb_conflist_length(lst)-1;
	for(li = pcb_conflist_last(lst); li != NULL; li = pcb_conflist_prev(li),idx--) {
		sprintf(act, "grid(#%d)", idx);
		sprintf(chk, "conf(iseq, editor/grids_idx, %d)", idx);
		strcpy(end, li->val.string[0]);
		pcb_gui->create_menu(pcb_gui, path, &props);
	}

}

void pcb_grid_install_menu(void)
{
	pcb_hid_cfg_map_anchor_menus(ANCH, grid_install_menu, NULL);
}

static int grid_lock = 0;

void pcb_grid_update_conf(conf_native_t *cfg, int arr_idx)
{
	if (grid_lock) return;
	grid_lock++;
	pcb_grid_install_menu();
	grid_lock--;
}

void pcb_grid_update_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (grid_lock) return;
	grid_lock++;
	pcb_grid_install_menu();

	/* to get the right menu checked */
	if ((grids_idx.nat != NULL) && (grids_idx.nat->val.integer[0] >= 0))
		pcb_grid_list_step(hidlib, 0);
	grid_lock--;
}


