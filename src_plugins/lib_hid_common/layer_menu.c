/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "board.h"
#include "data.h"
#include "hid.h"
#include "layer.h"
#include "layer_grp.h"
#include "pcb-printf.h"
#include "hid_cfg.h"
#include "hid.h"

typedef struct {
	const char *anch;
	int view;
} ly_ctx_t;

static void layer_install_menu(void *ctx_, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	ly_ctx_t *ctx = ctx_;
	int plen = strlen(path);
	int len_avail = 125;
	char *end = path + plen;
	pcb_menu_prop_t props;
	char act[256], chk[256];
	int idx;
	pcb_layergrp_id_t gid;

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.checked = chk;
	props.update_on = "";
	props.cookie = ctx->anch;

	pcb_hid_cfg_del_anchor_menus(node, ctx->anch);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	/* have to go reverse to keep order because this will insert items */

	for(gid = pcb_max_group(PCB); gid >= 0; gid--) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
		pcb_menu_prop_t props;
		int n;

		if (g->type & PCB_LYT_SUBSTRATE)
			continue;

		for(n = g->len-1; n >= 0; n--) {
			pcb_layer_t *l = pcb_get_layer(PCB->Data, g->lid[n]);

			pcb_snprintf(end, len_avail, "  %s", l->name);
			pcb_gui->create_menu(path, &props);
		}

		pcb_snprintf(end, len_avail, "[%s]", g->name);
		pcb_gui->create_menu(path, &props);

	}
}


void pcb_layer_install_menu(void)
{
	ly_ctx_t ctx;

	ctx.view = 1;
	ctx.anch = "@layerview";
	pcb_hid_cfg_map_anchor_menus(ctx.anch, layer_install_menu, &ctx);

	ctx.view = 0;
	ctx.anch = "@layerpick";
	pcb_hid_cfg_map_anchor_menus(ctx.anch, layer_install_menu, &ctx);
}
