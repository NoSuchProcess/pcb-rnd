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

pcb_virt_layer_t pcb_virt_layers[] = {
	{"invisible",      PCB_LYT_VIRTUAL + 1, PCB_LYT_VIRTUAL | PCB_LYT_INVIS | PCB_LYT_LOGICAL },
	{"rats",           PCB_LYT_VIRTUAL + 2, PCB_LYT_VIRTUAL | PCB_LYT_RAT },
	{"topassembly",    PCB_LYT_VIRTUAL + 3, PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_TOP},
	{"bottomassembly", PCB_LYT_VIRTUAL + 4, PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_BOTTOM },
	{"fab",            PCB_LYT_VIRTUAL + 5, PCB_LYT_VIRTUAL | PCB_LYT_FAB  | PCB_LYT_LOGICAL },
	{"plated-drill",   PCB_LYT_VIRTUAL + 6, PCB_LYT_VIRTUAL | PCB_LYT_PDRILL },
	{"unplated-drill", PCB_LYT_VIRTUAL + 7, PCB_LYT_VIRTUAL | PCB_LYT_UDRILL },
	{"csect",          PCB_LYT_VIRTUAL + 8, PCB_LYT_VIRTUAL | PCB_LYT_CSECT | PCB_LYT_LOGICAL },
	{"fontsel",        PCB_LYT_VIRTUAL + 9, PCB_LYT_VIRTUAL | PCB_LYT_FONTSEL | PCB_LYT_LOGICAL | PCB_LYT_NOEXPORT },
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

#define PCB_LAYER_VIRT_MIN (PCB_LYT_VIRTUAL + PCB_VLY_first + 1)
#define PCB_LAYER_VIRT_MAX (PCB_LYT_VIRTUAL + PCB_VLY_end)

#define layer_if_too_many(fail_cmd) \
do { \
	if (PCB->Data->LayerN >= PCB_MAX_LAYER) { \
		pcb_message(PCB_MSG_ERROR, "Too many layers - can't have more than %d\n", PCB_MAX_LAYER); \
		fail_cmd; \
	} \
} while(0)

pcb_bool pcb_layer_is_empty_(pcb_layer_t *layer)
{
	unsigned int flags;
	pcb_layer_id_t lid = pcb_layer_id(PCB->Data, layer);

	if (lid < 0)
		return 1;

	flags = pcb_layer_flags(lid);

	if ((flags & PCB_LYT_COPPER) && (flags & PCB_LYT_TOP)) { /* if our layer is the top copper layer and we have an element pad on it, it's non-empty */
		PCB_PAD_ALL_LOOP(PCB->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad))
				return 0;
		}
		PCB_ENDALL_LOOP;
	}


	if ((flags & PCB_LYT_COPPER) && (flags & PCB_LYT_BOTTOM)) { /* if our layer is the bottom copper layer and we have an element pad on it, it's non-empty */
		PCB_PAD_ALL_LOOP(PCB->Data);
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

pcb_bool pcb_layer_is_empty(pcb_layer_id_t num)
{
	if ((num >= 0) && (num < pcb_max_layer))
		return pcb_layer_is_empty_(PCB->Data->Layer + num);
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

pcb_bool pcb_layer_is_paste_empty(pcb_side_t side)
{
	pcb_bool paste_empty = pcb_true;
	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		if (PCB_ON_SIDE(pad, side) && !PCB_FLAG_TEST(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			paste_empty = pcb_false;
			break;
		}
	}
	PCB_ENDALL_LOOP;
	return paste_empty;
}

unsigned int pcb_layer_flags(pcb_layer_id_t layer_idx)
{
	pcb_layer_t *l;

	if (layer_idx & PCB_LYT_UI)
		return PCB_LYT_UI | PCB_LYT_VIRTUAL;

	if ((layer_idx >= PCB_LAYER_VIRT_MIN) && (layer_idx <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].type;

	if ((layer_idx < 0) || (layer_idx >= pcb_max_layer))
		return 0;

	l = &PCB->Data->Layer[layer_idx];
	return pcb_layergrp_flags(PCB, l->grp);
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
		if ((pcb_layer_flags(n) & mask) == mask)
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
		if ((pcb_layer_flags(n) & mask))
			APPEND(n);

	if (mask & PCB_LYT_UI)
		for (n = 0; n < vtlayer_len(&pcb_uilayer); n++)
			APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}

pcb_layer_id_t pcb_layer_by_name(const char *name)
{
	int n;
	for (n = 0; n < pcb_max_layer; n++)
		if (strcmp(PCB->Data->Layer[n].Name, name) == 0)
			return n;
	return -1;
}

void pcb_layers_reset()
{
	int n;

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

pcb_layer_id_t pcb_layer_create_old(pcb_layer_type_t type, pcb_bool reuse_layer, pcb_bool_t reuse_group, const char *lname)
{
	pcb_layer_id_t id;
	pcb_layergrp_id_t grp = -1;
	int found;
	unsigned long loc  = type & PCB_LYT_ANYWHERE;
	pcb_layer_type_t role = type & PCB_LYT_ANYTHING;

	if ((type & PCB_LYT_VIRTUAL) || (type & PCB_LYT_LOGICAL))
		return -1;

	/* look for an existing layer if reuse is enabled */
	if (reuse_layer) {
		switch(role) {
			case PCB_LYT_MASK:
			case PCB_LYT_PASTE:
			case PCB_LYT_SILK:
			case PCB_LYT_FAB:
			case PCB_LYT_ASSY:
			case PCB_LYT_RAT:
			case PCB_LYT_INVIS:
			case PCB_LYT_VIRTUAL:
			case PCB_LYT_ANYTHING:
			case PCB_LYT_ANYWHERE:
			case PCB_LYT_ANYPROP:
			case PCB_LYT_UDRILL:
			case PCB_LYT_PDRILL:
			case PCB_LYT_UI:
			case PCB_LYT_CSECT:
			case PCB_LYT_FONTSEL:
			case PCB_LYT_SUBSTRATE:
			case PCB_LYT_MISC:
			case PCB_LYT_NOEXPORT:
				return -1; /* do not create virtual layers */

			case PCB_LYT_INTERN:
			case PCB_LYT_TOP:
			case PCB_LYT_BOTTOM:
			case PCB_LYT_LOGICAL:
				return -1; /* suppress compiler warnings */

			case PCB_LYT_COPPER:
				switch(loc) {
					case PCB_LYT_TOP:    return PCB_COMPONENT_SIDE;
					case PCB_LYT_BOTTOM: return PCB_SOLDER_SIDE;
					case PCB_LYT_INTERN:
						for(grp = 2; grp < PCB_MAX_LAYERGRP; grp++) {
							if (PCB->LayerGroups.grp[grp].len > 0) {
								id = PCB->LayerGroups.grp[grp].lid[0];
								if (strcmp(PCB->Data->Layer[id].Name, "outline") != 0)
									return id;
							}
						}
						return -1;
				}
				break;

			case PCB_LYT_OUTLINE:
				for(grp = 2; grp < PCB_MAX_LAYERGRP; grp++) {
					if (PCB->LayerGroups.grp[grp].len > 0) {
						id = PCB->LayerGroups.grp[grp].lid[0];
						if (strcmp(PCB->Data->Layer[id].Name, "outline") == 0)
							return id;
					}
				}
				return -1;
		}
		return -1;
	}

	/* Need to create a new layers, look for an existing group first */
	if (role == PCB_LYT_OUTLINE) {
		lname = "outline";

		for(id = 0; id < PCB->Data->LayerN; id++)
			if (strcmp(PCB->Data->Layer[id].Name, lname) == 0)
				return id; /* force reuse outline */

		/* not found: create a new layer for outline */
		grp = -1;
		reuse_group = pcb_false;
	}

	/* there's only one top and bottom group, always reuse them */
	if (role == PCB_LYT_COPPER) {
		switch(loc) {
			case PCB_LYT_TOP:    grp = PCB_COMPONENT_SIDE; reuse_group = 0; break;
			case PCB_LYT_BOTTOM: grp = PCB_SOLDER_SIDE; reuse_group = 0; break;
		}
	}

	if (reuse_group) { /* can't use group find here, it depends on existing silks! */
		switch(role) {
			case PCB_LYT_MASK:
			case PCB_LYT_PASTE:
			case PCB_LYT_SILK:
			case PCB_LYT_FAB:
			case PCB_LYT_ASSY:
			case PCB_LYT_RAT:
			case PCB_LYT_INVIS:
			case PCB_LYT_VIRTUAL:
			case PCB_LYT_ANYTHING:
			case PCB_LYT_ANYWHERE:
			case PCB_LYT_ANYPROP:
			case PCB_LYT_UDRILL:
			case PCB_LYT_PDRILL:
			case PCB_LYT_UI:
			case PCB_LYT_CSECT:
			case PCB_LYT_FONTSEL:
			case PCB_LYT_SUBSTRATE:
			case PCB_LYT_MISC:
			case PCB_LYT_NOEXPORT:
				return -1; /* do not create virtual layers */

			case PCB_LYT_INTERN:
			case PCB_LYT_TOP:
			case PCB_LYT_BOTTOM:
			case PCB_LYT_LOGICAL:
				return -1; /* suppress compiler warnings */

			case PCB_LYT_COPPER:
				switch(loc) {
					case PCB_LYT_TOP:
					case PCB_LYT_BOTTOM:
						abort(); /* can't get here */
					case PCB_LYT_INTERN:
						/* find the first internal layer */
						for(found = 0, grp = 2; grp < PCB_MAX_LAYERGRP; grp++) {
							if (PCB->LayerGroups.grp[grp].len > 0) {
								id = PCB->LayerGroups.grp[grp].lid[0];
								if (strcmp(PCB->Data->Layer[id].Name, "outline") != 0) {
									found = 1;
									break;
								}
							}
						}
						if (!found)
							return -1;
						id = -1;
				}
				break;
			case PCB_LYT_OUTLINE:
				abort(); /* can't get here */
		}
	}

	if (grp < 0) {
		/* Also need to create a group */
		for(grp = 0; grp < PCB_MAX_LAYERGRP; grp++)
			if (PCB->LayerGroups.grp[grp].len == 0)
				break;
		if (grp >= PCB_MAX_LAYERGRP)
			return -2;
	}

	layer_if_too_many(return -1);

	id = PCB->Data->LayerN++;

	if (lname != NULL) {
		if (PCB->Data->Layer[id].Name != NULL)
			free((char *)PCB->Data->Layer[id].Name);
		PCB->Data->Layer[id].Name = pcb_strdup(lname);
	}

	/* add layer to group */
	PCB->LayerGroups.grp[grp].lid[PCB->LayerGroups.grp[grp].len] = id;
	PCB->LayerGroups.grp[grp].len++;

	return id;
}

pcb_layer_id_t pcb_layer_create(pcb_layergrp_id_t grp, const char *lname)
{
	pcb_layer_id_t id;

	layer_if_too_many(return -1);

	id = PCB->Data->LayerN++;

	if (lname != NULL) {
		if (PCB->Data->Layer[id].Name != NULL)
			free((char *)PCB->Data->Layer[id].Name);
		PCB->Data->Layer[id].Name = pcb_strdup(lname);
	}

	/* add layer to group */
	if (grp >= 0) {
		PCB->LayerGroups.grp[grp].lid[PCB->LayerGroups.grp[grp].len] = id;
		PCB->LayerGroups.grp[grp].len++;
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

/* ---------------------------------------------------------------------------
 * move layer
 * moves the selected layers to a new index in the layer list.
 */

static void move_one_thermal(int old_index, int new_index, pcb_pin_t * pin)
{
	int t1 = 0, i;
	int oi = old_index, ni = new_index;

	if (old_index != -1)
		t1 = PCB_FLAG_THERM_GET(old_index, pin);

	if (oi == -1)
		oi = PCB_MAX_LAYER - 1;					/* inserting a layer */
	if (ni == -1)
		ni = PCB_MAX_LAYER - 1;					/* deleting a layer */

	if (oi < ni) {
		for (i = oi; i < ni; i++)
			PCB_FLAG_THERM_ASSIGN(i, PCB_FLAG_THERM_GET(i + 1, pin), pin);
	}
	else {
		for (i = oi; i > ni; i--)
			PCB_FLAG_THERM_ASSIGN(i, PCB_FLAG_THERM_GET(i - 1, pin), pin);
	}

	if (new_index != -1)
		PCB_FLAG_THERM_ASSIGN(new_index, t1, pin);
	else
		PCB_FLAG_THERM_ASSIGN(ni, 0, pin);
}

static void move_all_thermals(int old_index, int new_index)
{
	PCB_VIA_LOOP(PCB->Data);
	{
		move_one_thermal(old_index, new_index, via);
	}
	PCB_END_LOOP;

	PCB_PIN_ALL_LOOP(PCB->Data);
	{
		move_one_thermal(old_index, new_index, pin);
	}
	PCB_ENDALL_LOOP;
}

static int is_last_top_copper_layer(int layer)
{
	pcb_layergrp_id_t cgroup = pcb_layer_get_group(PCB, pcb_max_group + PCB_COMPONENT_SIDE);
	pcb_layergrp_id_t lgroup = pcb_layer_get_group(PCB, layer);
	if (cgroup == lgroup && PCB->LayerGroups.grp[lgroup].len == 1)
		return 1;
	return 0;
}

static int is_last_bottom_copper_layer(int layer)
{
	int sgroup = pcb_layer_get_group(PCB, pcb_max_group + PCB_SOLDER_SIDE);
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

/* empty and detach a layer - must be initialized or another layer moved over it later */
static void layer_clear(pcb_layer_t *dst)
{
	memset(dst, 0, sizeof(pcb_layer_t));
	dst->grp = -1;
}

/* Initialize a new layer with safe initial values */
static void layer_init(pcb_layer_t *lp, int idx)
{
	memset(lp, 0, sizeof(pcb_layer_t));
	lp->grp = -1;
	lp->On = 1;
	lp->Name = pcb_strdup("New Layer");
	lp->Color = conf_core.appearance.color.layer[idx];
	lp->SelectedColor = conf_core.appearance.color.layer_selected[idx];
}

/* Recalculate the group->layer cross-links using the layer->group links
   (useful when layer positions change but groups don't) */
static void layer_sync_groups(pcb_board_t *pcb)
{
	pcb_layergrp_id_t g;
	pcb_layer_id_t l;

	for (g = 0; g < pcb_max_group; g++)
		pcb->LayerGroups.grp[g].len = 0;

	for (l = 0; l < pcb_max_layer; l++) {
		int i;
		g = pcb->Data->Layer[l].grp;
		if (g >= 0) {
			i = pcb->LayerGroups.grp[g].len++;
			pcb->LayerGroups.grp[g].lid[i] = l;
		}
	}
}

int pcb_layer_move(pcb_layer_id_t old_index, pcb_layer_id_t new_index)
{
	pcb_layer_id_t l;
	pcb_layer_t saved_layer;

	/* sanity checks */
	if (old_index < -1 || old_index >= pcb_max_layer) {
		pcb_message(PCB_MSG_ERROR, "Invalid old layer %d for move: must be -1..%d\n", old_index, pcb_max_layer - 1);
		return 1;
	}

	if (new_index < -1 || new_index > pcb_max_layer || new_index >= PCB_MAX_LAYER) {
		pcb_message(PCB_MSG_ERROR, "Invalid new layer %d for move: must be -1..%d\n", new_index, pcb_max_layer);
		return 1;
	}

	if (old_index == new_index)
		return 0;

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

	if (old_index == -1) { /* insert new layer */
		pcb_layergrp_id_t gid;
		pcb_layer_t *lp;
		if (pcb_max_layer == PCB_MAX_LAYER) {
			pcb_message(PCB_MSG_ERROR, "No room for new layers\n");
			return 1;
		}
		/* Create a new layer at new_index - by shifting right all layers above it. */
		lp = &PCB->Data->Layer[new_index];
		for(l = pcb_max_layer; l > new_index; l--)
			layer_move(&PCB->Data->Layer[l], &PCB->Data->Layer[l-1]);
		pcb_max_layer++;

		layer_init(lp, new_index);

		/* insert the new layer into the top copper group (or if that fails, in
		   any copper group) */
		gid = -1;
		if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &gid, 1) < 1)
			pcb_layergrp_list(PCB, PCB_LYT_COPPER, &gid, 1);
		lp->grp = gid;

		for (l = 0; l < pcb_max_layer; l++)
			if (pcb_layer_stack[l] >= new_index)
				pcb_layer_stack[l]++;
		pcb_layer_stack[pcb_max_layer - 1] = new_index;
	}
	else if (new_index == -1) { /* Delete the layer at old_index */
#warning layer TODO remove objects, free fields layer_free(&PCB->Data->Layer[old_index]);
		for(l = old_index; l < pcb_max_layer-1; l++) {
			layer_move(&PCB->Data->Layer[l], &PCB->Data->Layer[l+1]);
			layer_clear(&PCB->Data->Layer[l+1]);
		}

		for (l = 0; l < pcb_max_layer; l++)
			if (pcb_layer_stack[l] == old_index)
				memmove(pcb_layer_stack + l, pcb_layer_stack + l + 1, (pcb_max_layer - l - 1) * sizeof(pcb_layer_stack[0]));
		pcb_max_layer--;
		for (l = 0; l < pcb_max_layer; l++)
			if (pcb_layer_stack[l] > old_index)
				pcb_layer_stack[l]--;
	}
	else {
		/* Move an existing layer */
		layer_move(&saved_layer, &PCB->Data->Layer[old_index]);

		if (old_index < new_index) {
			for(l = old_index; l < new_index; l++)
				layer_move(&PCB->Data->Layer[l], &PCB->Data->Layer[l+1]);
		}
		else {
			for(l = old_index; l > new_index; l--)
				layer_move(&PCB->Data->Layer[l], &PCB->Data->Layer[l-1]);
		}

		layer_move(&PCB->Data->Layer[new_index], &saved_layer);
	}

	move_all_thermals(old_index, new_index);

	layer_sync_groups(PCB);

	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	pcb_gui->invalidate_all();
	return 0;
}

const char *pcb_layer_name(pcb_layer_id_t id)
{
	if (id < 0)
		return NULL;
	if (id < pcb_max_layer)
		return PCB->Data->Layer[id].Name;
	if ((id >= PCB_LAYER_VIRT_MIN) && (id <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[id-PCB_LAYER_VIRT_MIN].name;
	return NULL;
}

pcb_layer_t *pcb_get_layer(pcb_layer_id_t id)
{
	if ((id >= 0) && (id < pcb_max_layer))
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
	pcb_layer_group_t *g;

	if (*cache < pcb_max_layer) { /* check if the cache is still pointing to the right layer */
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
