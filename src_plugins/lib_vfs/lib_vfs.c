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

#include "board.h"
#include "data.h"
#include "plugins.h"

#include "lib_vfs.h"

static void vfs_list_layers(pcb_board_t *pcb, pcb_vfs_list_cb cb, void *ctx)
{
	gds_t path;
	pcb_layer_id_t lid;
	size_t orig_used;

	cb(ctx, "data/layers", 1);
	gds_init(&path);
	gds_append_str(&path, "data/layers/");
	orig_used = path.used;
	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		path.used = orig_used;
		pcb_append_printf(&path, "%ld", lid);
		cb(ctx, path.array, 1);
	}
	gds_uninit(&path);
}

int pcb_vfs_list(pcb_board_t *pcb, pcb_vfs_list_cb cb, void *ctx)
{
	cb(ctx, "data", 1);
	vfs_list_layers(pcb, cb, ctx);
	cb(ctx, "data/layer_groups", 1);
	cb(ctx, "route_styles", 1);
	cb(ctx, "netlist", 1);

	return 0;
}


int pplg_check_ver_lib_vfs(int ver_needed) { return 0; }

void pplg_uninit_lib_vfs(void)
{
}

int pplg_init_lib_vfs(void)
{
	PCB_API_CHK_VER;
	return 0;
}
