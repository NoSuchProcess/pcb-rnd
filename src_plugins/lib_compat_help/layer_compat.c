/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
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
 *
 */

#include "config.h"

#include "layer_compat.h"
#include "pstk_compat.h"

#include "board.h"
#include "data.h"

static void swap_one_thermal(pcb_pstk_t *pstk, int lid1, int lid2, int udoable)
{
	unsigned char *p1 = pcb_pstk_get_thermal(pstk, lid1, 0);
	unsigned char *p2 = pcb_pstk_get_thermal(pstk, lid2, 0);
	unsigned char t1, t2;

	t1 = (p1 == NULL) ? 0 : *p1;
	t2 = (p2 == NULL) ? 0 : *p2;

	if (t1 == t2) return;

	pcb_pstk_set_thermal(pstk, lid1, t2, udoable);
	pcb_pstk_set_thermal(pstk, lid2, t1, udoable);
}

int pcb_layer_swap(pcb_board_t *pcb, rnd_layer_id_t lid1, rnd_layer_id_t lid2)
{
	rnd_rtree_it_t it;
	pcb_layer_t l1tmp, l2tmp;
	rnd_layergrp_id_t gid;
	rnd_box_t *n;

	if (lid1 == lid2)
		return 0;

	pcb_layer_move_(&l1tmp, &pcb->Data->Layer[lid1]);
	pcb_layer_move_(&l2tmp, &pcb->Data->Layer[lid2]);

	pcb_layer_move_(&pcb->Data->Layer[lid1], &l2tmp);
	pcb_layer_move_(&pcb->Data->Layer[lid2], &l1tmp);

	/* thermals are referenced by layer IDs which are going to change now */
	for(n = rnd_r_first(pcb->Data->padstack_tree, &it); n != NULL; n = rnd_r_next(&it))
		swap_one_thermal((pcb_pstk_t *)n, lid1, lid2, 0);

	for(gid = 0; gid < pcb_max_group(pcb); gid++) {
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[gid];
		int n;

		for(n = 0; n < g->len; n++) {
			if (g->lid[n] == lid1)
				g->lid[n] = lid2;
			else if (g->lid[n] == lid2)
				g->lid[n] = lid1;
		}
	}

	return 0;
}

