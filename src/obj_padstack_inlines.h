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

#include "obj_padstack.h"
#include "vtpadstack.h"

typedef enum {
	PCB_BB_NONE, /* no drill */
	PCB_BB_THRU, /* all way thru */
	PCB_BB_BB,
	PCB_BB_INVALID
} pcb_bb_type_t;

static inline PCB_FUNC_UNUSED pcb_padstack_proto_t *pcb_padstack_get_proto(pcb_padstack_t *ps)
{
	if (ps->proto >= ps->parent.data->ps_protos.used)
		return NULL;
	return ps->parent.data->ps_protos.array + ps->proto;
}

static inline PCB_FUNC_UNUSED pcb_bb_type_t pcb_padstack_bbspan(pcb_board_t *pcb, pcb_padstack_t *ps, pcb_layergrp_id_t *top, pcb_layergrp_id_t *bottom)
{
	pcb_bb_type_t res;
	int topi, boti;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	if (proto == NULL)
		return PCB_BB_INVALID;

	/* most common case should be quick */
	if (proto->hdia == 0) {
		if (top != NULL) *top = -1;
		if (bottom != NULL) *bottom = -1;
		return PCB_BB_NONE;
	}

	if ((proto->htop == 0) && (proto->hbottom == 0)) {
		if ((top == NULL) && (bottom == NULL))
			return PCB_BB_THRU;
		res = PCB_BB_THRU;
	}
	else
		res = PCB_BB_BB;

	/* slower case: need to look up start and end */
	if (!pcb->LayerGroups.cache.copper_valid)
		pcb_layergrp_copper_cache_update(&pcb->LayerGroups);

	if (proto->htop < pcb->LayerGroups.cache.copper_len)
		topi = proto->htop;
	else
		topi = pcb->LayerGroups.cache.copper_len - 1;

	if (proto->hbottom < pcb->LayerGroups.cache.copper_len-1)
		boti = pcb->LayerGroups.cache.copper_len-1-proto->hbottom;
	else
		boti = 0;

	if (boti <= topi) {
		if (top != NULL) *top = -1;
		if (bottom != NULL) *bottom = -1;
		return PCB_BB_INVALID;
	}

	if (top != NULL) *top = pcb->LayerGroups.cache.copper[topi];
	if (bottom != NULL) *bottom = pcb->LayerGroups.cache.copper[boti];

	return res;
}

static inline PCB_FUNC_UNUSED pcb_bool_t pcb_padstack_bb_drills(pcb_board_t *pcb, pcb_padstack_t *ps, pcb_layergrp_id_t grp)
{
	pcb_layergrp_id_t top, bot;
	pcb_bb_type_t res = pcb_padstack_bbspan(pcb, ps, &top, &bot);
	switch(res) {
		case PCB_BB_THRU: return 1;
		case PCB_BB_NONE: case PCB_BB_INVALID: return 0;
		case PCB_BB_BB: return (grp >= bot) && (grp <= top);
	}
	return 0;
}

static inline PCB_FUNC_UNUSED pcb_padstack_shape_t *pcb_padstack_shape(pcb_padstack_t *ps, pcb_layer_type_t lyt, pcb_layer_combining_t comb)
{
	int n;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	if (proto == NULL)
		return NULL;

	lyt &= (PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE);
	for(n = 0; n < proto->len; n++)
		if ((lyt == proto->shape[n].layer_mask) && (comb == proto->shape[n].comb))
			return proto->shape+n;

	return 0;
}
