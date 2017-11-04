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

#ifndef PCB_OBJ_PSTK_INLINES_H
#define PCB_OBJ_PSTK_INLINES_H

#include "board.h"
#include "data.h"
#include "obj_pstk.h"
#include "vtpadstack.h"
#include "layer.h"
#include "thermal.h"

typedef enum {
	PCB_BB_NONE, /* no drill */
	PCB_BB_THRU, /* all way thru */
	PCB_BB_BB,
	PCB_BB_INVALID
} pcb_bb_type_t;

/* Returns the ID of a proto within its parent's cache */
static inline PCB_FUNC_UNUSED pcb_cardinal_t pcb_pstk_get_proto_id(pcb_pstk_proto_t *proto)
{
	pcb_data_t *data = proto->parent;
	if ((proto >= data->ps_protos.array) && (proto < data->ps_protos.array + data->ps_protos.used))
		return proto - data->ps_protos.array;
	assert(!"padstack proto is not in its own parent!");
	return PCB_PADSTACK_INVALID;
}

/* return the padstack prototype for a padstack reference - returns NULL if not found */
static inline PCB_FUNC_UNUSED pcb_pstk_proto_t *pcb_pstk_get_proto_(const pcb_data_t *data, pcb_cardinal_t proto)
{
	if (proto >= data->ps_protos.used)
		return NULL;
	if (data->ps_protos.array[proto].in_use == 0)
		return NULL;
	return data->ps_protos.array + proto;
}

/* return the padstack prototype for a padstack reference - returns NULL if not found */
static inline PCB_FUNC_UNUSED pcb_pstk_proto_t *pcb_pstk_get_proto(pcb_pstk_t *ps)
{
	return pcb_pstk_get_proto_(ps->parent.data, ps->proto);
}

/* return the padstack transformed shape. Returns NULL if the proto or the
   tshape is not. */
static inline PCB_FUNC_UNUSED pcb_pstk_tshape_t *pcb_pstk_get_tshape_(const pcb_data_t *data, pcb_cardinal_t proto, int protoi)
{
	pcb_pstk_proto_t *pr = pcb_pstk_get_proto_(data, proto);
	if (protoi < 0)
		return NULL;
	if (pr == NULL)
		return NULL;
	if (protoi >= pr->tr.used)
		return NULL;
	return &pr->tr.array[protoi];
}

/* return the padstack prototype for a padstack reference - returns NULL if not found */
static inline PCB_FUNC_UNUSED pcb_pstk_tshape_t *pcb_pstk_get_tshape(pcb_pstk_t *ps)
{
	if (ps->protoi < 0) { /* need to update this */
		pcb_pstk_proto_t *pr = pcb_pstk_get_proto_(ps->parent.data, ps->proto);
		if (pr == NULL)
			return NULL;
		return pcb_pstk_make_tshape(ps->parent.data, pr, ps->rot, ps->xmirror, &ps->protoi);
	}
	return pcb_pstk_get_tshape_(ps->parent.data, ps->proto, ps->protoi);
}

/* return the type of drill and optionally fill in group IDs of drill ends ;
   if proto_out is not NULL, also load it with the proto */
static inline PCB_FUNC_UNUSED pcb_bb_type_t pcb_pstk_bbspan(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layergrp_id_t *top, pcb_layergrp_id_t *bottom, pcb_pstk_proto_t **proto_out)
{
	pcb_bb_type_t res;
	int topi, boti;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

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

	if (proto->htop >= 0) {
		/* positive: count from top copper down, bump at bottom */
		if (proto->htop < pcb->LayerGroups.cache.copper_len)
			topi = proto->htop;
		else
			topi = pcb->LayerGroups.cache.copper_len - 1;
	}
	else {
		/* negative: count from bottom copper up, bump at top */
		topi = pcb->LayerGroups.cache.copper_len - 1 - proto->htop;
		if (topi < 0)
			topi = 0;
	}

	if (proto->hbottom >= 0) {
		/* positive: count from bottom copper up, bump at top */
		if (proto->hbottom < pcb->LayerGroups.cache.copper_len-1)
			boti = pcb->LayerGroups.cache.copper_len-1-proto->hbottom;
		else
			boti = 0;
	}
	else {
		/* positive: count from top copper down, bump at bottom */
		boti = -proto->hbottom;
		if (boti > pcb->LayerGroups.cache.copper_len - 1)
			boti = pcb->LayerGroups.cache.copper_len - 1;
	}

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
static inline PCB_FUNC_UNUSED pcb_bool_t pcb_pstk_bb_drills(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layergrp_id_t grp, pcb_pstk_proto_t **proto_out)
{
	pcb_layergrp_id_t top, bot;
	pcb_bb_type_t res = pcb_pstk_bbspan(pcb, ps, &top, &bot, proto_out);
	switch(res) {
		case PCB_BB_THRU: return pcb_true;
		case PCB_BB_NONE: case PCB_BB_INVALID: return 0;
		case PCB_BB_BB: return (grp <= bot) && (grp >= top);
	}
	return pcb_false;
}

/* returns the shape the padstack has on the given layer group;
   WARNING: does not respect the NOSHAPE thermal, should NOT be
   called directly; use pcb_pstk_shape_*() instead. */
static inline PCB_FUNC_UNUSED pcb_pstk_shape_t *pcb_pstk_shape(pcb_pstk_t *ps, pcb_layer_type_t lyt, pcb_layer_combining_t comb)
{
	int n;
	pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(ps);
	if (ts == NULL)
		return NULL;

	lyt &= (PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE);
	for(n = 0; n < ts->len; n++)
		if ((lyt == ts->shape[n].layer_mask) && (comb == ts->shape[n].comb))
			return ts->shape+n;

	return 0;
}

static inline PCB_FUNC_UNUSED pcb_pstk_shape_t *pcb_pstk_shape_at(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_t *layer)
{
	unsigned int lyt = pcb_layer_flags_(layer);
	pcb_layer_combining_t comb = layer->comb;

	if (lyt & PCB_LYT_COPPER) {
		pcb_layer_id_t lid;
		if (lyt & PCB_LYT_INTERN) {
			/* apply internal only if that layer has drill */
			if (!pcb_pstk_bb_drills(pcb, ps, pcb_layer_get_group_(layer), NULL))
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

	return pcb_pstk_shape(ps, lyt, comb);
}

static inline PCB_FUNC_UNUSED pcb_pstk_shape_t *pcb_pstk_shape_gid(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layergrp_id_t gid, pcb_layer_combining_t comb)
{
	pcb_layergrp_t *grp = pcb_get_layergrp(pcb, gid);

	if ((grp == NULL) || (grp->len < 1))
		return NULL;

	if (grp->type & PCB_LYT_COPPER) {
		int n, nosh;

		/* blind/buried: intern layer has no shape if no hole */
		if (grp->type & PCB_LYT_INTERN) {
			/* apply internal only if that layer has drill */
			if (!pcb_pstk_bb_drills(pcb, ps, gid, NULL))
				return NULL;
		}

			/* if all layers of the group says no-shape, don't have a shape */
		for(n = 0, nosh = 0; n < grp->len; n++) {
			pcb_layer_id_t lid = grp->lid[n];
			if ((lid < ps->thermals.used) && (ps->thermals.shape[lid] & PCB_THERMAL_ON) && ((ps->thermals.shape[lid] & 3) == PCB_THERMAL_NOSHAPE))
				nosh++;
		}
		if (nosh == grp->len)
			return NULL;
	}

	/* normal procedure: go by group flags */
	return pcb_pstk_shape(ps, grp->type, comb);
}


#endif
