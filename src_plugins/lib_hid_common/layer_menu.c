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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "board.h"
#include "data.h"
#include "hid.h"
#include "layer.h"
#include "layer_grp.h"
#include "pcb-printf.h"

void pcb_layer_menu_create(const char *path_prefix, const char *cookie)
{
	char path[1024], *bn;
	int plen = strlen(path_prefix), len_avail = sizeof(path) - plen;
	pcb_layergrp_id_t gid;


	memcpy(path, path_prefix, plen);
	bn = path + plen;
	if ((plen == 0) || (bn[-1] != '/')) {
		*bn = '/';
		bn++;
		len_avail--;
	}

	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
		int n;

		if (g->type & PCB_LYT_SUBSTRATE)
			continue;

		pcb_snprintf(bn, len_avail, "[%s]", g->name);
		pcb_gui->create_menu(path, "TODO: action", "", "accel", "Layer group", cookie);

		for(n = 0; n < g->len; n++) {
			pcb_layer_t *l = pcb_get_layer(g->lid[n]);

			pcb_snprintf(bn, len_avail, "  %s", l->meta.real.name);
			pcb_gui->create_menu(path, "TODO: action", "", "accel", "Layer", cookie);
		}
	}
}

