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

#ifndef PCB_OBJ_PADSTACK_INLINES_H
#define PCB_OBJ_PADSTACK_INLINES_H

#include "board.h"
#include "obj_padstack.h"
#include "vtpadstack.h"
#include "layer.h"
#include "thermal.h"

typedef enum {
	PCB_BB_NONE, /* no drill */
	PCB_BB_THRU, /* all way thru */
	PCB_BB_BB,
	PCB_BB_INVALID
} pcb_bb_type_t;

/* return the padstack prototype for a padstack reference - returns NULL if not found */
static inline PCB_FUNC_UNUSED pcb_padstack_proto_t *pcb_padstack_get_proto(pcb_padstack_t *ps)
{
	if (ps->proto >= ps->parent.data->ps_protos.used)
		return NULL;
	if (ps->parent.data->ps_protos.array[ps->proto].in_use == 0)
		return NULL;
	return ps->parent.data->ps_protos.array + ps->proto;
}

/* return the type of drill and optionally fill in group IDs of drill ends ;
   if proto_out is not NULL, also load it with the proto */
static inline PCB_FUNC_UNUSED pcb_bb_type_t pcb_padstack_bbspan(pcb_board_t *pcb, pcb_padstack_t *ps, pcb_layergrp_id_t *top, pcb_layergrp_id_t *bottom, pcb_padstack_proto_t **proto_out)
{
	pcb_bb_type_t res;
	int topi, boti;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);

	if (proto_out != NULL)
		*proto_out = proto;

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

/* return whether a given padstack drills a given group
  (does not consider plating, only drill!) */
static inline PCB_FUNC_UNUSED pcb_bool_t pcb_padstack_bb_drills(pcb_board_t *pcb, pcb_padstack_t *ps, pcb_layergrp_id_t grp, pcb_padstack_proto_t **proto_out)
{
	pcb_layergrp_id_t top, bot;
	pcb_bb_type_t res = pcb_padstack_bbspan(pcb, ps, &top, &bot, proto_out);
	switch(res) {
		case PCB_BB_THRU: return pcb_true;
		case PCB_BB_NONE: case PCB_BB_INVALID: return 0;
		case PCB_BB_BB: return (grp >= bot) && (grp <= top);
	}
	return pcb_false;
}

/* returns the shape the padstack has on the given layer group */
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

static inline PCB_FUNC_UNUSED pcb_padstack_shape_t *pcb_padstack_shape_at(pcb_board_t *pcb, pcb_padstack_t *ps, pcb_layer_t *layer)
{
	unsigned int lyt = pcb_layer_flags_(layer);
	pcb_layer_combining_t comb = 0;

	if (lyt & PCB_LYT_COPPER) {
		pcb_layer_id_t lid;
		if (lyt & PCB_LYT_INTERN) {
			/* apply internal only if that layer has drill */
			if (!pcb_padstack_bb_drills(pcb, ps, pcb_layer_get_group_(layer), NULL))
				return NULL;
		}

		/* special case: if thermal says 'no shape' on this layer, omit the shape */
		layer = pcb_layer_get_real(layer);
		if ((layer != NULL) && (layer->parent != NULL)) {
			lid = pcb_layer_id(layer->parent, layer);
			if (lid < ps->thermals.used) {
				if ((ps->thermals.shape[lid] & PCB_THERMAL_ON) && ((ps->thermals.shape[lid] & 3) == PCB_THERMAL_NOSHAPE))
					return NULL;
			}
		}
	}

	/* combining is always 0 for copper */
	if (lyt & PCB_LYT_COPPER)
		comb = 0;
	else
		comb = layer->comb;

	return pcb_padstack_shape(ps, lyt, comb);
}

#endif
