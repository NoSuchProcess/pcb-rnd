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
	{"invisible",      PCB_LYT_VIRTUAL + 1,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_INVIS | PCB_LYT_LOGICAL },
	{"topmask",        PCB_LYT_VIRTUAL + 2,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_MASK | PCB_LYT_TOP },
	{"bottommask",     PCB_LYT_VIRTUAL + 3,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_MASK | PCB_LYT_BOTTOM },
	{"topsilk",        PCB_LYT_VIRTUAL + 4,  +PCB_COMPONENT_SIDE, PCB_LYT_SILK | PCB_LYT_TOP },
	{"bottomsilk",     PCB_LYT_VIRTUAL + 5,  +PCB_SOLDER_SIDE,    PCB_LYT_SILK | PCB_LYT_BOTTOM },
	{"rats",           PCB_LYT_VIRTUAL + 6,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_RAT },
	{"toppaste",       PCB_LYT_VIRTUAL + 7,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_PASTE | PCB_LYT_TOP },
	{"bottompaste",    PCB_LYT_VIRTUAL + 8,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_PASTE | PCB_LYT_BOTTOM },
	{"topassembly",    PCB_LYT_VIRTUAL + 9,  -1,                  PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_TOP},
	{"bottomassembly", PCB_LYT_VIRTUAL + 10, -1,                  PCB_LYT_VIRTUAL | PCB_LYT_ASSY | PCB_LYT_BOTTOM },
	{"fab",            PCB_LYT_VIRTUAL + 11, -1,                  PCB_LYT_VIRTUAL | PCB_LYT_FAB  | PCB_LYT_LOGICAL },
	{"plated-drill",   PCB_LYT_VIRTUAL + 12, -1,                  PCB_LYT_VIRTUAL | PCB_LYT_PDRILL },
	{"unplated-drill", PCB_LYT_VIRTUAL + 13, -1,                  PCB_LYT_VIRTUAL | PCB_LYT_UDRILL },
	{"csect",          PCB_LYT_VIRTUAL + 14, -1,                  PCB_LYT_VIRTUAL | PCB_LYT_CSECT | PCB_LYT_LOGICAL },
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
#define PCB_LAYER_VIRT_MAX (PCB_LYT_VIRTUAL + PCB_VLY_end - 1)


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
	if ((Layer >= Data->Layer) && (Layer < (Data->Layer + PCB_MAX_LAYER + 2)))
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

#warning layer TODO: remove this once we have explicit outline layer
#define LAYER_IS_OUTLINE(idx) (((idx) > 0) && (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[(idx)].Name, "outline") == 0))

unsigned int pcb_layer_flags(pcb_layer_id_t layer_idx)
{
	unsigned int res = 0;

	if (layer_idx & PCB_LYT_UI)
		return PCB_LYT_UI | PCB_LYT_VIRTUAL;

	if (layer_idx == pcb_solder_silk_layer)
		return PCB_LYT_SILK | PCB_LYT_BOTTOM;

	if (layer_idx == pcb_component_silk_layer)
		return PCB_LYT_SILK | PCB_LYT_TOP;

	if ((layer_idx >= PCB_LAYER_VIRT_MIN) && (layer_idx <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].type;

	if (layer_idx >= pcb_max_layer)
		return 0;

	if (layer_idx < pcb_max_copper_layer) {
		if (!LAYER_IS_OUTLINE(layer_idx)) {
			/* check whether it's top, bottom or internal */
			int group, entry;
			for (group = 0; group < pcb_max_group; group++) {
				if (PCB->LayerGroups.grp[group].len) {
					unsigned int my_group = 0, gf = 0;
					for (entry = 0; entry < PCB->LayerGroups.grp[group].len; entry++) {
						pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[entry];
						if (layer == layer_idx)
							my_group = 1;
						if (layer == pcb_component_silk_layer)
							gf |= PCB_LYT_TOP;
						else if (layer == pcb_solder_silk_layer)
							gf |= PCB_LYT_BOTTOM;
					}
					if (my_group) {
						res |= gf;
						if (gf == 0)
							res |= PCB_LYT_INTERN;
						break; /* stop searching groups */
					}
				}
			}
			res |= PCB_LYT_COPPER;
		}
		else
			res |= PCB_LYT_OUTLINE;
	}

	return res;
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

/* For now, return only non-silks */
#define APPEND_VIRT(v) \
do { \
	if (v->data_layer_offs < 0) \
		APPEND(v->new_id); \
} while(0)

/* this would be how we returned silks from here
		APPEND(pcb_max_copper_layer + v->data_layer_offs); \
	else \
*/


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

	for (n = 0; n < PCB_MAX_LAYER + 2; n++)
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

	for (n = 0; n < PCB_MAX_LAYER + 2; n++)
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

	/* reset layer names */
	for(n = 2; n < PCB_MAX_LAYER; n++) {
		if (PCB->Data->Layer[n].Name != NULL)
			free((char *)PCB->Data->Layer[n].Name);
		PCB->Data->Layer[n].Name = pcb_strdup("<pcb_layers_reset>");
	}

	/* reset layer groups */
	for(n = 0; n < PCB_MAX_LAYERGRP; n++) {
		PCB->LayerGroups.grp[n].len = 0;
		PCB->LayerGroups.grp[n].type = 0;
		PCB->LayerGroups.grp[n].valid = 0;
	}

	/* set up one copper layer on top and one on bottom */
#warning layer TODO: this should use a separate group for silk
	PCB->Data->LayerN = 2;
	PCB->LayerGroups.grp[PCB_SOLDER_SIDE].len = 1;
	PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].len = 1;
	PCB->LayerGroups.grp[PCB_SOLDER_SIDE].lid[0] = PCB_SOLDER_SIDE;
	PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].lid[0] = PCB_COMPONENT_SIDE;

	/* Name top and bottom layers */
	if (PCB->Data->Layer[PCB_COMPONENT_SIDE].Name != NULL)
		free((char *)PCB->Data->Layer[PCB_COMPONENT_SIDE].Name);
	PCB->Data->Layer[PCB_COMPONENT_SIDE].Name = pcb_strdup("<top>");

	if (PCB->Data->Layer[PCB_SOLDER_SIDE].Name != NULL)
		free((char *)PCB->Data->Layer[PCB_SOLDER_SIDE].Name);
	PCB->Data->Layer[PCB_SOLDER_SIDE].Name = pcb_strdup("<bottom>");
}

pcb_layer_id_t pcb_layer_create(pcb_layer_type_t type, pcb_bool reuse_layer, pcb_bool_t reuse_group, const char *lname)
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

#warning layer TODO: remove this hack
/* Temporary hack: silk layers have to be added as the last entry in the top and bottom layer groups, if they are not already in */
static void hack_in_silks()
{
	pcb_layer_id_t sl, cl;
	int found, n;

	sl = PCB_SOLDER_SIDE + PCB->Data->LayerN;
	for(found = 0, n = 0; n < PCB->LayerGroups.grp[PCB_SOLDER_SIDE].len; n++)
		if (PCB->LayerGroups.grp[PCB_SOLDER_SIDE].lid[n] == sl)
			found = 1;

	if (!found) {
		PCB->LayerGroups.grp[PCB_SOLDER_SIDE].lid[PCB->LayerGroups.grp[PCB_SOLDER_SIDE].len] = sl;
		PCB->LayerGroups.grp[PCB_SOLDER_SIDE].len++;
		if (PCB->Data->Layer[sl].Name != NULL)
			free((char *)PCB->Data->Layer[sl].Name);
		PCB->Data->Layer[sl].Name = pcb_strdup("silk");
	}


	cl = PCB_COMPONENT_SIDE + PCB->Data->LayerN;
	for(found = 0, n = 0; n < PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].len; n++)
		if (PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].lid[n] == cl)
			found = 1;

	if (!found) {
		PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].lid[PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].len] = cl;
		PCB->LayerGroups.grp[PCB_COMPONENT_SIDE].len++;
		if (PCB->Data->Layer[cl].Name != NULL)
			free((char *)PCB->Data->Layer[cl].Name);
		PCB->Data->Layer[cl].Name = pcb_strdup("silk");
	}
}

int pcb_layer_rename(pcb_layer_id_t layer, const char *lname)
{
	if (PCB->Data->Layer[layer].Name != NULL)
		free((char *)PCB->Data->Layer[layer].Name);
	PCB->Data->Layer[layer].Name = pcb_strdup(lname);
	return 0;
}

void pcb_layers_finalize()
{
	hack_in_silks();
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

static int LastLayerInComponentGroup(int layer)
{
	pcb_layergrp_id_t cgroup = pcb_layer_get_group(pcb_max_group + PCB_COMPONENT_SIDE);
	pcb_layergrp_id_t lgroup = pcb_layer_get_group(layer);
#warning layer TODO: remove this silk-specific hack?
	if (cgroup == lgroup && PCB->LayerGroups.grp[lgroup].len == 2)
		return 1;
	return 0;
}

static int LastLayerInSolderGroup(int layer)
{
	int sgroup = pcb_layer_get_group(pcb_max_group + PCB_SOLDER_SIDE);
	int lgroup = pcb_layer_get_group(layer);
#warning layer TODO: remove this silk-specific hack?
	if (sgroup == lgroup && PCB->LayerGroups.grp[lgroup].len == 2)
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

int pcb_layer_move(pcb_layer_id_t old_index, pcb_layer_id_t new_index)
{
	int groups[PCB_MAX_LAYERGRP + 2], l, g;
	pcb_layer_t saved_layer;
	int saved_group;

	pcb_undo_add_layer_move(old_index, new_index);
	pcb_undo_inc_serial();

	if (old_index < -1 || old_index >= pcb_max_copper_layer) {
		pcb_message(PCB_MSG_ERROR, "Invalid old layer %d for move: must be -1..%d\n", old_index, pcb_max_copper_layer - 1);
		return 1;
	}
	if (new_index < -1 || new_index > pcb_max_copper_layer || new_index >= PCB_MAX_LAYER) {
		pcb_message(PCB_MSG_ERROR, "Invalid new layer %d for move: must be -1..%d\n", new_index, pcb_max_copper_layer);
		return 1;
	}
	if (old_index == new_index)
		return 0;

	if (new_index == -1 && LastLayerInComponentGroup(old_index)) {
		pcb_gui->confirm_dialog("You can't delete the last top-side layer\n", "Ok", NULL);
		return 1;
	}

	if (new_index == -1 && LastLayerInSolderGroup(old_index)) {
		pcb_gui->confirm_dialog("You can't delete the last bottom-side layer\n", "Ok", NULL);
		return 1;
	}

	for (g = 0; g < PCB_MAX_LAYERGRP + 2; g++)
		groups[g] = -1;

	for (g = 0; g < PCB_MAX_LAYERGRP; g++)
		for (l = 0; l < PCB->LayerGroups.grp[g].len; l++)
			groups[PCB->LayerGroups.grp[g].lid[l]] = g;

	if (old_index == -1) {
		pcb_layer_t *lp;
		if (pcb_max_copper_layer == PCB_MAX_LAYER) {
			pcb_message(PCB_MSG_ERROR, "No room for new layers\n");
			return 1;
		}
		/* Create a new layer at new_index. */
		lp = &PCB->Data->Layer[new_index];
		memmove(&PCB->Data->Layer[new_index + 1],
						&PCB->Data->Layer[new_index], (pcb_max_copper_layer - new_index + 2) * sizeof(pcb_layer_t));
		memmove(&groups[new_index + 1], &groups[new_index], (pcb_max_copper_layer - new_index + 2) * sizeof(int));
		pcb_max_copper_layer++;
		memset(lp, 0, sizeof(pcb_layer_t));
		lp->On = 1;
		lp->Name = pcb_strdup("New Layer");
		lp->Color = conf_core.appearance.color.layer[new_index];
		lp->SelectedColor = conf_core.appearance.color.layer_selected[new_index];
		for (l = 0; l < pcb_max_copper_layer; l++)
			if (pcb_layer_stack[l] >= new_index)
				pcb_layer_stack[l]++;
		pcb_layer_stack[pcb_max_copper_layer - 1] = new_index;
	}
	else if (new_index == -1) {
		/* Delete the layer at old_index */
		memmove(&PCB->Data->Layer[old_index],
						&PCB->Data->Layer[old_index + 1], (pcb_max_copper_layer - old_index + 2 - 1) * sizeof(pcb_layer_t));
		memset(&PCB->Data->Layer[pcb_max_copper_layer + 1], 0, sizeof(pcb_layer_t));
		memmove(&groups[old_index], &groups[old_index + 1], (pcb_max_copper_layer - old_index + 2 - 1) * sizeof(int));
		for (l = 0; l < pcb_max_copper_layer; l++)
			if (pcb_layer_stack[l] == old_index)
				memmove(pcb_layer_stack + l, pcb_layer_stack + l + 1, (pcb_max_copper_layer - l - 1) * sizeof(pcb_layer_stack[0]));
		pcb_max_copper_layer--;
		for (l = 0; l < pcb_max_copper_layer; l++)
			if (pcb_layer_stack[l] > old_index)
				pcb_layer_stack[l]--;
	}
	else {
		/* Move an existing layer */
		memcpy(&saved_layer, &PCB->Data->Layer[old_index], sizeof(pcb_layer_t));
		saved_group = groups[old_index];
		if (old_index < new_index) {
			memmove(&PCB->Data->Layer[old_index], &PCB->Data->Layer[old_index + 1], (new_index - old_index) * sizeof(pcb_layer_t));
			memmove(&groups[old_index], &groups[old_index + 1], (new_index - old_index) * sizeof(int));
		}
		else {
			memmove(&PCB->Data->Layer[new_index + 1], &PCB->Data->Layer[new_index], (old_index - new_index) * sizeof(pcb_layer_t));
			memmove(&groups[new_index + 1], &groups[new_index], (old_index - new_index) * sizeof(int));
		}
		memcpy(&PCB->Data->Layer[new_index], &saved_layer, sizeof(pcb_layer_t));
		groups[new_index] = saved_group;
	}

	move_all_thermals(old_index, new_index);

	for (g = 0; g < PCB_MAX_LAYERGRP; g++)
		PCB->LayerGroups.grp[g].len = 0;
	for (l = 0; l < pcb_max_layer; l++) {
		int i;
		g = groups[l];
		if (g >= 0) {
			i = PCB->LayerGroups.grp[g].len++;
			PCB->LayerGroups.grp[g].lid[i] = l;
		}
	}

	for (g = 0; g < PCB_MAX_LAYERGRP; g++)
		if (PCB->LayerGroups.grp[g].len == 0) {
			memmove(&PCB->LayerGroups.grp[g].len,
							&PCB->LayerGroups.grp[g + 1].len, (PCB_MAX_LAYERGRP - g - 1) * sizeof(PCB->LayerGroups.grp[g].len));
			memmove(&PCB->LayerGroups.grp[g],
							&PCB->LayerGroups.grp[g + 1], (PCB_MAX_LAYERGRP - g - 1) * sizeof(PCB->LayerGroups.grp[g]));
		}

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
		pcb_layer_id_t lid = v->data_layer_offs;
		pcb_layergrp_id_t grp;
		if (lid >= 0)
			lid += pcb_max_copper_layer;
		grp = pcb_layer_lookup_group(lid);
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

