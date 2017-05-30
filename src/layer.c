/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
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
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "layer.h"
#include "compat_misc.h"
#include "undo.h"
#include "event.h"
#include "layer_ui.h"
#include "layer_vis.h"

pcb_virt_layer_t pcb_virt_layers[] = {
	{"invisible",      PCB_LYT_VIRTUAL + 1, PCB_LYT_VIRTUAL | PCB_LYT_INVIS },
	{"rats",           PCB_LYT_VIRTUAL + 2, PCB_LYT_VIRTUAL | PCB_LYT_RAT },
	{"topassembly",    PCB_LYT_VIRTUAL + 3, PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_TOP},
	{"bottomassembly", PCB_LYT_VIRTUAL + 4, PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_BOTTOM },
	{"fab",            PCB_LYT_VIRTUAL + 5, PCB_LYT_VIRTUAL | PCB_LYT_FAB  | PCB_LYT_LOGICAL },
	{"plated-drill",   PCB_LYT_VIRTUAL + 6, PCB_LYT_VIRTUAL | PCB_LYT_PDRILL },
	{"unplated-drill", PCB_LYT_VIRTUAL + 7, PCB_LYT_VIRTUAL | PCB_LYT_UDRILL },
	{"csect",          PCB_LYT_VIRTUAL + 8, PCB_LYT_VIRTUAL | PCB_LYT_CSECT | PCB_LYT_LOGICAL },
	{"dialog",         PCB_LYT_VIRTUAL + 9, PCB_LYT_VIRTUAL | PCB_LYT_DIALOG | PCB_LYT_LOGICAL | PCB_LYT_NOEXPORT },
	{ NULL, 0 },
};


typedef struct {
	pcb_layer_type_t type;
	int class;
	const char *name;
} pcb_layer_type_name_t;

static const pcb_layer_type_name_t pcb_layer_type_names[] = {
	{ PCB_LYT_TOP,     1, "top" },
	{ PCB_LYT_BOTTOM,  1, "bottom" },
	{ PCB_LYT_INTERN,  1, "intern" },
	{ PCB_LYT_LOGICAL, 1, "logical" },
	{ PCB_LYT_COPPER,  2, "copper" },
	{ PCB_LYT_SILK,    2, "silk" },
	{ PCB_LYT_MASK,    2, "mask" },
	{ PCB_LYT_PASTE,   2, "paste" },
	{ PCB_LYT_OUTLINE, 2, "outline" },
	{ PCB_LYT_RAT,     2, "rat" },
	{ PCB_LYT_INVIS,   2, "invis" },
	{ PCB_LYT_ASSY,    2, "assy" },
	{ PCB_LYT_FAB,     2, "fab" },
	{ PCB_LYT_PDRILL,  2, "plateddrill" },
	{ PCB_LYT_UDRILL,  2, "unplateddrill" },
	{ PCB_LYT_CSECT,   2, "cross-section" },
	{ PCB_LYT_SUBSTRATE,2,"substrate" },
	{ PCB_LYT_MISC,    2, "misc" },
	{ PCB_LYT_UI,      2, "userinterface" },
	{ PCB_LYT_VIRTUAL, 3, "virtual" },
	{ 0, 0, NULL }
};

static const char *pcb_layer_type_class_names[] = {
	"INVALID", "location", "purpose", "property"
};

static const pcb_layer_type_name_t pcb_layer_comb_names[] = {
	{ PCB_LYC_SUB,  0, "sub"},
	{ PCB_LYC_AUTO, 0, "auto"},
	{ 0, 0, NULL }
};


#define PCB_LAYER_VIRT_MIN (PCB_LYT_VIRTUAL + PCB_VLY_first + 1)
#define PCB_LAYER_VIRT_MAX (PCB_LYT_VIRTUAL + PCB_VLY_end)

#define layer_if_too_many(pcb, fail_cmd) \
do { \
	if (pcb->Data->LayerN >= PCB_MAX_LAYER) { \
		pcb_message(PCB_MSG_ERROR, "Too many layers - can't have more than %d\n", PCB_MAX_LAYER); \
		fail_cmd; \
	} \
} while(0)

pcb_bool pcb_layer_is_empty_(pcb_board_t *pcb, pcb_layer_t *layer)
{
	unsigned int flags;
	pcb_layer_id_t lid = pcb_layer_id(pcb->Data, layer);

	if (lid < 0)
		return 1;

	flags = pcb_layer_flags(pcb, lid);

	if ((flags & PCB_LYT_COPPER) && (flags & PCB_LYT_TOP)) { /* if our layer is the top copper layer and we have an element pad on it, it's non-empty */
		PCB_PAD_ALL_LOOP(pcb->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad))
				return 0;
		}
		PCB_ENDALL_LOOP;
	}


	if ((flags & PCB_LYT_COPPER) && (flags & PCB_LYT_BOTTOM)) { /* if our layer is the bottom copper layer and we have an element pad on it, it's non-empty */
		PCB_PAD_ALL_LOOP(pcb->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad))
				return 0;
		}
		PCB_ENDALL_LOOP;
	}

#warning TODO: check top silk and bottom silk for elements

	/* normal case: a layer is empty if all lists are empty */
	return
		(linelist_length(&layer->Line) == 0) &&
		(arclist_length(&layer->Arc) == 0) &&
		(polylist_length(&layer->Polygon) == 0) &&
		(textlist_length(&layer->Text) == 0);
}

pcb_bool pcb_layer_is_empty(pcb_board_t *pcb, pcb_layer_id_t num)
{
	if ((num >= 0) && (num < pcb->Data->LayerN))
		return pcb_layer_is_empty_(pcb, pcb->Data->Layer + num);
	return pcb_false;
}

pcb_layer_id_t pcb_layer_id(pcb_data_t *Data, pcb_layer_t *Layer)
{
	if ((Layer >= Data->Layer) && (Layer < (Data->Layer + PCB_MAX_LAYER)))
		return Layer - Data->Layer;

	if ((Layer >= pcb_uilayer.array) && (Layer < pcb_uilayer.array + vtlayer_len(&pcb_uilayer)))
		return (Layer - pcb_uilayer.array) | PCB_LYT_UI;

	return -1;
}

#warning layer TODO: with editables this is not enough, check user layers too
pcb_bool pcb_layer_is_paste_empty(pcb_board_t *pcb, pcb_side_t side)
{
	pcb_bool paste_empty = pcb_true;
	PCB_PAD_ALL_LOOP(pcb->Data);
	{
		if (PCB_ON_SIDE(pad, side) && !PCB_FLAG_TEST(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			paste_empty = pcb_false;
			break;
		}
	}
	PCB_ENDALL_LOOP;
	return paste_empty;
}

unsigned int pcb_layer_flags(pcb_board_t *pcb, pcb_layer_id_t layer_idx)
{
	pcb_layer_t *l;

	if (layer_idx & PCB_LYT_UI)
		return PCB_LYT_UI | PCB_LYT_VIRTUAL;

	if ((layer_idx >= PCB_LAYER_VIRT_MIN) && (layer_idx <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].type;

	if ((layer_idx < 0) || (layer_idx >= pcb->Data->LayerN))
		return 0;

	l = &pcb->Data->Layer[layer_idx];
	return pcb_layergrp_flags(pcb, l->grp);
}

#define APPEND(n) \
	do { \
		if (res != NULL) { \
			if (used < res_len) { \
				res[used] = n; \
				used++; \
			} \
		} \
		else \
			used++; \
	} while(0)

#define APPEND_VIRT(v) \
do { \
	APPEND(v->new_id); \
} while(0)

const pcb_virt_layer_t *pcb_vlayer_get_first(pcb_layer_type_t mask)
{
	const pcb_virt_layer_t *v;
	mask &= (~PCB_LYT_VIRTUAL);
	for(v = pcb_virt_layers; v->name != NULL; v++)
		if (((v->type & (~PCB_LYT_VIRTUAL)) & mask) == mask)
			return v;
	return NULL;
}


int pcb_layer_list(pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len)
{
	int n, used = 0;
	pcb_virt_layer_t *v;

	for(v = pcb_virt_layers; v->name != NULL; v++)
		if ((v->type & mask) == mask)
			APPEND_VIRT(v);

	for (n = 0; n < PCB_MAX_LAYER; n++)
		if ((pcb_layer_flags(PCB, n) & mask) == mask)
			APPEND(n);

	if (mask == PCB_LYT_UI)
		for (n = 0; n < vtlayer_len(&pcb_uilayer); n++)
			APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}

int pcb_layer_list_any(pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len)
{
	int n, used = 0;
	pcb_virt_layer_t *v;

	for(v = pcb_virt_layers; v->name != NULL; v++)
		if ((v->type & mask))
			APPEND_VIRT(v);

	for (n = 0; n < PCB_MAX_LAYER; n++)
		if ((pcb_layer_flags(PCB, n) & mask))
			APPEND(n);

	if (mask & PCB_LYT_UI)
		for (n = 0; n < vtlayer_len(&pcb_uilayer); n++)
			APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}

pcb_layer_id_t pcb_layer_by_name(const char *name)
{
	pcb_layer_id_t n;
	for (n = 0; n < PCB->Data->LayerN; n++)
		if (strcmp(PCB->Data->Layer[n].Name, name) == 0)
			return n;
	return -1;
}

void pcb_layers_reset()
{
	pcb_layer_id_t n;

	/* reset everything to empty, even if that's invalid (no top/bottom copper)
	   for the rest of the code: the (embedded) default design will overwrite this. */
	/* reset layers */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		if (PCB->Data->Layer[n].Name != NULL)
			free((char *)PCB->Data->Layer[n].Name);
		PCB->Data->Layer[n].Name = pcb_strdup("<pcb_layers_reset>");
		PCB->Data->Layer[n].grp = -1;
	}

	/* reset layer groups */
	for(n = 0; n < PCB_MAX_LAYERGRP; n++) {
		PCB->LayerGroups.grp[n].len = 0;
		PCB->LayerGroups.grp[n].type = 0;
		PCB->LayerGroups.grp[n].valid = 0;
	}
	PCB->LayerGroups.len = 0;
	PCB->Data->LayerN = 0;
}

/* empty and detach a layer - must be initialized or another layer moved over it later */
static void layer_clear(pcb_layer_t *dst)
{
	memset(dst, 0, sizeof(pcb_layer_t));
	dst->grp = -1;
}

pcb_layer_id_t pcb_layer_create(pcb_layergrp_id_t grp, const char *lname)
{
	pcb_layer_id_t id;

	layer_if_too_many(PCB, return -1);

	id = PCB->Data->LayerN++;

	if (lname != NULL) {
		if (PCB->Data->Layer[id].Name != NULL)
			free((char *)PCB->Data->Layer[id].Name);
	}

	layer_clear(&PCB->Data->Layer[id]);
	PCB->Data->Layer[id].Name = pcb_strdup(lname);

	/* add layer to group */
	if (grp >= 0) {
		PCB->LayerGroups.grp[grp].lid[PCB->LayerGroups.grp[grp].len] = id;
		PCB->LayerGroups.grp[grp].len++;
		PCB->Data->Layer[id].On = PCB->Data->Layer[PCB->LayerGroups.grp[grp].lid[0]].On;
	}
	PCB->Data->Layer[id].grp = grp;

	return id;
}

int pcb_layer_rename(pcb_layer_id_t layer, const char *lname)
{
	if (PCB->Data->Layer[layer].Name != NULL)
		free((char *)PCB->Data->Layer[layer].Name);
	PCB->Data->Layer[layer].Name = pcb_strdup(lname);
	return 0;
}

#undef APPEND

static void swap_one_thermal(int lid1, int lid2, pcb_pin_t * pin)
{
	int was_on_l1 = !!PCB_FLAG_THERM_GET(lid1, pin);
	int was_on_l2 = !!PCB_FLAG_THERM_GET(lid2, pin);

	PCB_FLAG_THERM_ASSIGN(lid2, was_on_l1, pin);
	PCB_FLAG_THERM_ASSIGN(lid1, was_on_l2, pin);
}


static int is_last_top_copper_layer(int layer)
{
	pcb_layergrp_id_t cgroup = pcb_layer_get_group(PCB, PCB->LayerGroups.len + PCB_COMPONENT_SIDE);
	pcb_layergrp_id_t lgroup = pcb_layer_get_group(PCB, layer);
	if (cgroup == lgroup && PCB->LayerGroups.grp[lgroup].len == 1)
		return 1;
	return 0;
}

static int is_last_bottom_copper_layer(int layer)
{
	int sgroup = pcb_layer_get_group(PCB, PCB->LayerGroups.len + PCB_SOLDER_SIDE);
	int lgroup = pcb_layer_get_group(PCB, layer);
	if (sgroup == lgroup && PCB->LayerGroups.grp[lgroup].len == 1)
		return 1;
	return 0;
}

int pcb_layer_rename_(pcb_layer_t *Layer, char *Name)
{
	free((char*)Layer->Name);
	Layer->Name = Name;
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return 0;
}

/* Safe move of a layer within a layer array, updaging all fields (list->parents) */
static void layer_move(pcb_layer_t *dst, pcb_layer_t *src)
{
	pcb_line_t *li;
	pcb_text_t *te;
	pcb_polygon_t *po;
	pcb_arc_t *ar;

	memcpy(dst, src, sizeof(pcb_layer_t));

	/* reparent all the lists: each list item has a ->parent pointer that needs to point to the new place */
	for(li = linelist_first(&dst->Line); li != NULL; li = linelist_next(li))
		li->link.parent = &dst->Line.lst;
	for(te = textlist_first(&dst->Text); te != NULL; te = textlist_next(te))
		te->link.parent = &dst->Text.lst;
	for(po = polylist_first(&dst->Polygon); po != NULL; po = polylist_next(po))
		po->link.parent = &dst->Polygon.lst;
	for(ar = arclist_first(&dst->Arc); ar != NULL; ar = arclist_next(ar))
		ar->link.parent = &dst->Arc.lst;
}

/* Initialize a new layer with safe initial values */
static void layer_init(pcb_layer_t *lp, pcb_layer_id_t idx, pcb_layergrp_id_t gid)
{
	memset(lp, 0, sizeof(pcb_layer_t));
	lp->grp = gid;
	lp->On = 1;
	lp->Name = pcb_strdup("New Layer");
	lp->Color = conf_core.appearance.color.layer[idx];
	lp->SelectedColor = conf_core.appearance.color.layer_selected[idx];
	if ((gid >= 0) && (PCB->LayerGroups.grp[gid].len == 0)) { /*When adding the first layer in a group, set up comb flags automatically */
		switch((PCB->LayerGroups.grp[gid].type) & PCB_LYT_ANYTHING) {
			case PCB_LYT_MASK:  lp->comb = PCB_LYC_AUTO | PCB_LYC_SUB; break;
			case PCB_LYT_SILK:  lp->comb = PCB_LYC_AUTO;
			case PCB_LYT_PASTE: lp->comb = PCB_LYC_AUTO;
			default: break;
		}
	}
}

int pcb_layer_move(pcb_layer_id_t old_index, pcb_layer_id_t new_index, pcb_layergrp_id_t new_in_grp)
{
	pcb_layer_id_t l;

	/* sanity checks */
	if (old_index < -1 || old_index >= PCB->Data->LayerN) {
		pcb_message(PCB_MSG_ERROR, "Invalid old layer %d for move: must be -1..%d\n", old_index, PCB->Data->LayerN - 1);
		return 1;
	}

	if (new_index < -1 || new_index > PCB->Data->LayerN || new_index >= PCB_MAX_LAYER) {
		pcb_message(PCB_MSG_ERROR, "Invalid new layer %d for move: must be -1..%d\n", new_index, PCB->Data->LayerN);
		return 1;
	}

	if (new_index == -1 && is_last_top_copper_layer(old_index)) {
		pcb_gui->confirm_dialog("You can't delete the last top-side layer\n", "Ok", NULL);
		return 1;
	}

	if (new_index == -1 && is_last_bottom_copper_layer(old_index)) {
		pcb_gui->confirm_dialog("You can't delete the last bottom-side layer\n", "Ok", NULL);
		return 1;
	}

	pcb_undo_add_layer_move(old_index, new_index);
	pcb_undo_inc_serial();

	if (old_index == -1) { /* append new layer at the end of the logical layer list, put it in the current group */
		pcb_layergrp_t *g;
		pcb_layer_t *lp;
		pcb_layer_id_t new_lid = PCB->Data->LayerN++;
		int grp_idx;

		lp = &PCB->Data->Layer[new_lid];
		if (new_in_grp >= 0)
			layer_init(lp, new_lid, new_in_grp);
		else
			layer_init(lp, new_lid, PCB->Data->Layer[new_index].grp);

		g = pcb_get_layergrp(PCB, lp->grp);

		if (new_in_grp >= 0) {
			if (new_index == 0)
				grp_idx = 0;
			else
				grp_idx = g->len;
		}
		else
			grp_idx = pcb_layergrp_index_in_grp(g, new_index);

		/* shift group members up and insert the new group */
		if ((g->len > grp_idx) && (grp_idx >= 0))
			memmove(g->lid+grp_idx+1, g->lid+grp_idx, (g->len - grp_idx) * sizeof(pcb_layer_id_t));
		if (grp_idx < 0)
			grp_idx = 0;
		g->lid[grp_idx] = new_lid;
		g->len++;
		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
		pcb_layervis_change_group_vis(new_lid, 1, 1);
		pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
	else if (new_index == -1) { /* Delete the layer at old_index */
		pcb_layergrp_id_t gid;
		pcb_layergrp_t *g;
		int grp_idx, remaining;

#warning layer TODO remove objects, free fields layer_free(&PCB->Data->Layer[old_index]);

		/* remove the current lid from its group */
		g = pcb_get_layergrp(PCB, PCB->Data->Layer[old_index].grp);
		grp_idx = pcb_layergrp_index_in_grp(g, old_index);
		if (grp_idx < 0) {
			pcb_message(PCB_MSG_ERROR, "Internal error; layer not in group\n");
			return -1;
		}

		remaining = (g->len - grp_idx - 1);
		if (remaining > 0)
			memmove(g->lid+grp_idx, g->lid+grp_idx+1, remaining * sizeof(pcb_layer_id_t));
		g->len--;

		/* update lids in all groups (shifting down idx) */
		for(gid = 0; gid < pcb_max_group(PCB); gid++) {
			int n;
			g = &PCB->LayerGroups.grp[gid];
			for(n = 0; n < g->len; n++)
				if (g->lid[n] > old_index)
					g->lid[n]--;
		}

		/* update visibility */
		for(l = old_index; l < PCB->Data->LayerN-1; l++) {
			layer_move(&PCB->Data->Layer[l], &PCB->Data->Layer[l+1]);
			layer_clear(&PCB->Data->Layer[l+1]);
		}

		for (l = 0; l < PCB->Data->LayerN; l++)
			if (pcb_layer_stack[l] == old_index)
				memmove(pcb_layer_stack + l, pcb_layer_stack + l + 1, (PCB->Data->LayerN - l - 1) * sizeof(pcb_layer_stack[0]));

		/* remove layer from the logical layer array */
		pcb_max_layer--;
		for (l = 0; l < PCB->Data->LayerN; l++)
			if (pcb_layer_stack[l] > old_index)
				pcb_layer_stack[l]--;

		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Logical layer move is not supported any more. This function should have not been called. Please report this error.\n");
		/* Removed r8686:
		   The new layer design presents the layers by groups to preserve physical
		   order. In this system the index of the logical layer on the logical
		   layer list is insignificant, thus we shouldn't try to change it. */
	}

	return 0;
}

int pcb_layer_swap(pcb_layer_id_t lid1, pcb_layer_id_t lid2)
{
	pcb_layer_t l1tmp, l2tmp;
	pcb_layergrp_id_t gid;

	if (lid1 == lid2)
		return 0;

	printf("SWAP: %d %d\n", lid1, lid2);

	layer_move(&l1tmp, &PCB->Data->Layer[lid1]);
	layer_move(&l2tmp, &PCB->Data->Layer[lid2]);

	layer_move(&PCB->Data->Layer[lid1], &l2tmp);
	layer_move(&PCB->Data->Layer[lid2], &l1tmp);

	PCB_VIA_LOOP(PCB->Data);
	{
		swap_one_thermal(lid1, lid2, via);
	}
	PCB_END_LOOP;

	PCB_PIN_ALL_LOOP(PCB->Data);
	{
		swap_one_thermal(lid1, lid2, pin);
	}
	PCB_ENDALL_LOOP;

	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
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



const char *pcb_layer_name(pcb_layer_id_t id)
{
	if (id < 0)
		return NULL;
	if (id < PCB->Data->LayerN)
		return PCB->Data->Layer[id].Name;
	if ((id >= PCB_LAYER_VIRT_MIN) && (id <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[id-PCB_LAYER_VIRT_MIN].name;
	return NULL;
}

pcb_layer_t *pcb_get_layer(pcb_layer_id_t id)
{
	if ((id >= 0) && (id < PCB->Data->LayerN))
		return &PCB->Data->Layer[id];
	if (id & PCB_LYT_UI) {
		id &= ~(PCB_LYT_VIRTUAL | PCB_LYT_UI);
		if ((id >= 0) && (id < vtlayer_len(&pcb_uilayer)))
			return &(pcb_uilayer.array[id]);
	}
	return NULL;
}

int pcb_layer_type_map(pcb_layer_type_t type, void *ctx, void (*cb)(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name))
{
	const pcb_layer_type_name_t *n;
	int found = 0;
	for(n = pcb_layer_type_names; n->name != NULL; n++) {
		if (type & n->type) {
			cb(ctx, n->type, n->name, n->class, pcb_layer_type_class_names[n->class]);
			found++;
		}
	}
	return found;
}

int pcb_layer_comb_map(pcb_layer_combining_t type, void *ctx, void (*cb)(void *ctx, pcb_layer_combining_t bit, const char *name))
{
	const pcb_layer_type_name_t *n;
	int found = 0;
	for(n = pcb_layer_comb_names; n->name != NULL; n++) {
		if (type & n->type) {
			cb(ctx, n->type, n->name);
			found++;
		}
	}
	return found;
}


pcb_layer_type_t pcb_layer_type_str2bit(const char *name)
{
	const pcb_layer_type_name_t *n;
	for(n = pcb_layer_type_names; n->name != NULL; n++)
		if (strcmp(n->name, name) == 0)
			return n->type;
	return 0;
}

pcb_layer_combining_t pcb_layer_comb_str2bit(const char *name)
{
	const pcb_layer_type_name_t *n;
	for(n = pcb_layer_comb_names; n->name != NULL; n++)
		if (strcmp(n->name, name) == 0)
			return n->type;
	return 0;
}

void pcb_layer_auto_fixup(pcb_board_t *pcb)
{
	pcb_layer_id_t n;

	/* old silk layers are always auto */
	for(n = 0; n < pcb_max_layer; n++)
		if (pcb_layer_flags(PCB, n) & PCB_LYT_SILK)
			pcb->Data->Layer[n].comb |= PCB_LYC_AUTO;
}

int pcb_layer_gui_set_vlayer(pcb_virtual_layer_t vid, int is_empty)
{
	pcb_virt_layer_t *v = &pcb_virt_layers[vid];
	assert((vid >= 0) && (vid < PCB_VLY_end));

	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

#warning TODO: need to pass the flags of the group, not the flags of the layer once we have a group for each layer
	if (pcb_gui->set_layer_group != NULL) {
		pcb_layergrp_id_t grp;
		pcb_layer_id_t lid = v->new_id;
		grp = pcb_layer_get_group(PCB, lid);
		return pcb_gui->set_layer_group(grp, lid, v->type, is_empty);
	}

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
}

int pcb_layer_gui_set_g_ui(pcb_layer_t *first, int is_empty)
{
	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

	if (pcb_gui->set_layer_group != NULL)
		return pcb_gui->set_layer_group(-1, pcb_layer_id(PCB->Data, first), PCB_LYT_VIRTUAL | PCB_LYT_UI, is_empty);

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
}

static pcb_layer_id_t pcb_layer_get_cached(pcb_layer_id_t *cache, unsigned int loc, unsigned int typ)
{
	pcb_layergrp_t *g;

	if (*cache < PCB->Data->LayerN) { /* check if the cache is still pointing to the right layer */
		pcb_layergrp_id_t gid = PCB->Data->Layer[*cache].grp;
		if ((gid >= 0) && (gid < PCB->LayerGroups.len)) {
			g = &(PCB->LayerGroups.grp[gid]);
			if ((g->type & loc) && (g->type & typ) && (g->lid[0] == *cache))
				return *cache;
		}
	}

	/* nope: need to resolve it again */
	g = pcb_get_grp(&PCB->LayerGroups, loc, typ);
	if ((g == NULL) || (g->len == 0)) {
		*cache = -1;
		return -1;
	}
	*cache = g->lid[0];
	return *cache;
}

pcb_layer_id_t pcb_layer_get_bottom_silk()
{
	static pcb_layer_id_t cache = -1;
	pcb_layer_id_t id = pcb_layer_get_cached(&cache, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	assert(id >= 0);
	return id;
}

pcb_layer_id_t pcb_layer_get_top_silk()
{
	static pcb_layer_id_t cache = -1;
	pcb_layer_id_t id =  pcb_layer_get_cached(&cache, PCB_LYT_TOP, PCB_LYT_SILK);
	assert(id >= 0);
	return id;
}
