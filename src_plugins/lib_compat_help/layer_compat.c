/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include "layer_compat.h"
#include "pstk_compat.h"

#include "board.h"
#include "data.h"
#include "obj_pinvia.h"

static void swap_one_thermal(int lid1, int lid2, pcb_pin_t * pin)
{
	int was_on_l1 = !!PCB_FLAG_THERM_GET(lid1, pin);
	int was_on_l2 = !!PCB_FLAG_THERM_GET(lid2, pin);

	PCB_FLAG_THERM_ASSIGN(lid2, was_on_l1, pin);
	PCB_FLAG_THERM_ASSIGN(lid1, was_on_l2, pin);
}

int pcb_layer_swap(pcb_board_t *pcb, pcb_layer_id_t lid1, pcb_layer_id_t lid2)
{
	pcb_layer_t l1tmp, l2tmp;
	pcb_layergrp_id_t gid;

	if (lid1 == lid2)
		return 0;

	pcb_layer_move_(&l1tmp, &pcb->Data->Layer[lid1]);
	pcb_layer_move_(&l2tmp, &pcb->Data->Layer[lid2]);

	pcb_layer_move_(&pcb->Data->Layer[lid1], &l2tmp);
	pcb_layer_move_(&pcb->Data->Layer[lid2], &l1tmp);

	PCB_VIA_LOOP(pcb->Data);
	{
		swap_one_thermal(lid1, lid2, via);
	}
	PCB_END_LOOP;

	PCB_PIN_ALL_LOOP(pcb->Data);
	{
		swap_one_thermal(lid1, lid2, pin);
	}
	PCB_ENDALL_LOOP;

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

