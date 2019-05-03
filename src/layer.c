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
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "layer.h"
#include "compat_misc.h"
#include "undo.h"
#include "event.h"
#include "layer_ui.h"
#include "layer_vis.h"
#include "funchash_core.h"
#include "rtree.h"
#include "obj_pstk_inlines.h"
#include "list_common.h"

pcb_virt_layer_t pcb_virt_layers[] = {
	{"invisible",      PCB_LYT_VIRTUAL + 1, PCB_LYT_VIRTUAL | PCB_LYT_INVIS, NULL, -1 },
	{"rats",           PCB_LYT_VIRTUAL + 2, PCB_LYT_VIRTUAL | PCB_LYT_RAT, NULL, -1 },
	{"topassembly",    PCB_LYT_VIRTUAL + 3, PCB_LYT_VIRTUAL | PCB_LYT_TOP, "assy", F_assy },
	{"bottomassembly", PCB_LYT_VIRTUAL + 4, PCB_LYT_VIRTUAL | PCB_LYT_BOTTOM, "assy", F_assy },
	{"fab",            PCB_LYT_VIRTUAL + 5, PCB_LYT_VIRTUAL | PCB_LYT_LOGICAL, "fab", F_fab },
	{"plated-drill",   PCB_LYT_VIRTUAL + 6, PCB_LYT_VIRTUAL, "pdrill", F_pdrill },
	{"unplated-drill", PCB_LYT_VIRTUAL + 7, PCB_LYT_VIRTUAL, "udrill", F_udrill },
	{"csect",          PCB_LYT_VIRTUAL + 8, PCB_LYT_VIRTUAL | PCB_LYT_LOGICAL, "csect", F_csect },
	{"dialog",         PCB_LYT_VIRTUAL + 9, PCB_LYT_VIRTUAL | PCB_LYT_DIALOG | PCB_LYT_LOGICAL | PCB_LYT_NOEXPORT, NULL, -1 },
	{ NULL, 0 },
};

const pcb_menu_layers_t pcb_menu_layers[] = {
	{ "Subcircuits", "subc",     &conf_core.appearance.color.subc,              NULL,       offsetof(pcb_board_t, SubcOn),             0 },
	{ "Subc. parts", "subcpart", &conf_core.appearance.color.subc,              NULL,       offsetof(pcb_board_t, SubcPartsOn),        0 },
	{ "Padstacks",   "padstacks",&conf_core.appearance.color.pin,               NULL,       offsetof(pcb_board_t, pstk_on),            0 },
	{ "Holes",       "holes",    &conf_core.appearance.color.pin,               NULL,       offsetof(pcb_board_t, hole_on),            0 },
	{ "Pstk. marks", "pstkmark", &conf_core.appearance.color.padstackmark,      NULL,       offsetof(pcb_board_t, padstack_mark_on),   0 },
	{ "Far side",    "farside",  &conf_core.appearance.color.invisible_objects, NULL,       offsetof(pcb_board_t, InvisibleObjectsOn), 0 },
	{ "Rats",        "rats",     &conf_core.appearance.color.rat,               "rats",     offsetof(pcb_board_t, RatOn),              offsetof(pcb_board_t, RatDraw) },
	{ NULL,          NULL,       NULL,                                          NULL,       0}
};


typedef struct {
	pcb_layer_type_t type;
	int class;
	const char *name;
} pcb_layer_type_name_t;

/* update docs: lihata format tree */
static const pcb_layer_type_name_t pcb_layer_type_names[] = {
	{ PCB_LYT_TOP,     1, "top" },
	{ PCB_LYT_BOTTOM,  1, "bottom" },
	{ PCB_LYT_INTERN,  1, "intern" },
	{ PCB_LYT_LOGICAL, 1, "logical" },
	{ PCB_LYT_COPPER,  2, "copper" },
	{ PCB_LYT_SILK,    2, "silk" },
	{ PCB_LYT_MASK,    2, "mask" },
	{ PCB_LYT_PASTE,   2, "paste" },
	{ PCB_LYT_BOUNDARY,2, "boundary" },
	{ PCB_LYT_RAT,     2, "rat" },
	{ PCB_LYT_INVIS,   2, "invis" },
	{ PCB_LYT_SUBSTRATE,2,"substrate" },
	{ PCB_LYT_MISC,    2, "misc" },
	{ PCB_LYT_DOC,     2, "doc" },
	{ PCB_LYT_MECH,    2, "mech" },
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

#define PURP_MATCH(ps, pi) (((purpi == -1) || (purpi == pi)) && ((purpose == NULL) || ((ps != NULL) && (strcmp(purpose, ps) == 0))))

void pcb_layer_free_fields(pcb_layer_t *layer)
{
	if (!layer->is_bound)
		pcb_attribute_free(&layer->Attributes);
	PCB_TEXT_LOOP(layer);
	{
		free(text->TextString);
	}
	PCB_END_LOOP;

	list_map0(&layer->Line, pcb_line_t, pcb_line_free);
	list_map0(&layer->Arc,  pcb_arc_t,  pcb_arc_free);
	list_map0(&layer->Text, pcb_text_t, pcb_text_free);
	PCB_POLY_LOOP(layer);
	{
		pcb_poly_free_fields(polygon);
	}
	PCB_END_LOOP;
	list_map0(&layer->Polygon, pcb_poly_t, pcb_poly_free);
	if (!layer->is_bound) {
		if (layer->line_tree)
			pcb_r_destroy_tree(&layer->line_tree);
		if (layer->arc_tree)
			pcb_r_destroy_tree(&layer->arc_tree);
		if (layer->text_tree)
			pcb_r_destroy_tree(&layer->text_tree);
		if (layer->polygon_tree)
			pcb_r_destroy_tree(&layer->polygon_tree);
	}
	free((char *)layer->name);
	memset(layer, 0, sizeof(pcb_layer_t));
}

pcb_bool pcb_layer_is_pure_empty(pcb_layer_t *layer)
{
	/* if any local list is non-empty, the layer is non-empty */
	if (layer->Line.lst.length > 0) return pcb_false;
	if (layer->Arc.lst.length > 0) return pcb_false;
	if (layer->Polygon.lst.length > 0) return pcb_false;
	if (layer->Text.lst.length > 0) return pcb_false;

	/* if the layer is locally empty, it might be a board layer that has
	   objects from subcircuits so also check the rtrees */
	return
		PCB_RTREE_EMPTY(layer->line_tree) && 
		PCB_RTREE_EMPTY(layer->arc_tree) && 
		PCB_RTREE_EMPTY(layer->polygon_tree) && 
		PCB_RTREE_EMPTY(layer->text_tree);
}

static pcb_bool pcb_layer_is_empty_glob(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *layer)
{
	/* if any padstack has a shape on this layer, it is not empty */
	PCB_PADSTACK_LOOP(data);
	{
		if (pcb_pstk_shape_at(pcb, padstack, layer) != NULL)
			return 0;
	}
	PCB_END_LOOP;

	/* need to recurse to subc */
	PCB_SUBC_LOOP(data);
	{
		pcb_layer_id_t n;
		pcb_layer_t *sl;

		for(sl = subc->data->Layer, n = 0; n < subc->data->LayerN; sl++,n++) {
			if (sl->meta.bound.real == layer) {
				if (!pcb_layer_is_empty_(pcb, sl))
					return 0;
				/* can't break here: multiple bound layers may point to the same real layer! */
			}
		}

		if (!pcb_layer_is_empty_glob(pcb, subc->data, layer))
			return 0;
	}
	PCB_END_LOOP;

	return 1;
}

pcb_bool pcb_layer_is_empty_(pcb_board_t *pcb, pcb_layer_t *layer)
{
	pcb_layer_type_t flags = pcb_layer_flags_(layer);
	if (flags == 0)
		return 1;

	/* fast check: direct object */
	if (!pcb_layer_is_pure_empty(layer))
		return 0;

	if (!pcb_layer_is_empty_glob(pcb, pcb->Data, layer))
		return 0;

	/* found nothing: layer is empty */
	return 1;
}

pcb_bool pcb_layer_is_empty(pcb_board_t *pcb, pcb_layer_id_t num)
{
	if ((num >= 0) && (num < pcb->Data->LayerN))
		return pcb_layer_is_empty_(pcb, pcb->Data->Layer + num);
	return pcb_false;
}

pcb_layer_id_t pcb_layer_id(const pcb_data_t *Data, const pcb_layer_t *Layer)
{
	if (Layer->parent_type == PCB_PARENT_UI)
		return pcb_uilayer_get_id(Layer);

	if (Layer->parent.data != Data) {
		/* the only case this makes sense is when we are resolving a bound layer */
		if ((Layer->is_bound) && (Layer->meta.bound.real != NULL))
			return pcb_layer_id(Data, Layer->meta.bound.real);
		/* failed binding, ignore the layer */
		return -1;
	}
	if ((Layer >= Data->Layer) && (Layer < (Data->Layer + PCB_MAX_LAYER)))
		return Layer - Data->Layer;

	return -1;
}

unsigned int pcb_layer_flags(const pcb_board_t *pcb, pcb_layer_id_t layer_idx)
{
	pcb_layer_t *l;

	if (layer_idx & PCB_LYT_UI)
		return PCB_LYT_UI | PCB_LYT_VIRTUAL;

	if ((layer_idx >= PCB_LAYER_VIRT_MIN) && (layer_idx <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].type;

	if ((layer_idx < 0) || (layer_idx >= pcb->Data->LayerN))
		return 0;

	l = &pcb->Data->Layer[layer_idx];
	return pcb_layergrp_flags(pcb, l->meta.real.grp);
}

unsigned int pcb_layer_flags_(const pcb_layer_t *layer)
{

	/* real layer: have to do a layer stack based lookup; but at least we have a real layer ID  */
	if (!layer->is_bound) {
		pcb_data_t *data = layer->parent.data;
		pcb_layer_id_t lid;

		if (data == NULL)
			return 0;

		lid = pcb_layer_id(data, layer);;
		if (lid < 0)
			return 0;

		assert(data->parent_type == PCB_PARENT_BOARD);
		return pcb_layer_flags(data->parent.board, lid);
	}

	/* bound layer: if it is already bound to a real layer, use that, whatever it is (manual binding may override our local type match pattern) */
	if (layer->meta.bound.real != NULL) {
		layer = pcb_layer_get_real(layer);
		if (layer == NULL)
			return 0;
		return pcb_layer_flags_(layer); /* tail recursion */
	}

	/* bound layer without a real layer binding: use the type match */
	return layer->meta.bound.type;
}

int pcb_layer_purpose(const pcb_board_t *pcb, pcb_layer_id_t layer_idx, const char **out)
{
	pcb_layergrp_t *grp;

	if (layer_idx & PCB_LYT_UI)
		return PCB_LYT_UI | PCB_LYT_VIRTUAL;

	if ((layer_idx >= PCB_LAYER_VIRT_MIN) && (layer_idx <= PCB_LAYER_VIRT_MAX)) {
		if (out != NULL)
			*out = pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].purpose;
		return pcb_virt_layers[layer_idx - PCB_LAYER_VIRT_MIN].purpi;
	}

	if ((layer_idx < 0) || (layer_idx >= pcb->Data->LayerN)) {
		if (out != NULL)
			*out = NULL;
		return -1;
	}

	grp = pcb_get_layergrp((pcb_board_t *)pcb, pcb->Data->Layer[layer_idx].meta.real.grp);

	if (out != NULL) {
		if (grp == NULL) {
			*out = NULL;
			return -1;
		}
		*out = grp->purpose;
		return grp->purpi;
	}

	if (grp == NULL)
		return -1;

	return grp->purpi;
}

int pcb_layer_purpose_(const pcb_layer_t *layer, const char **out)
{
	/* real layer: have to do a layer stack based lookup; but at least we have a real layer ID  */
	if (!layer->is_bound) {
		pcb_data_t *data = layer->parent.data;
		pcb_layer_id_t lid;

		if (data == NULL)
			return 0;

		lid = pcb_layer_id(data, layer);;
		if (lid < 0)
			return 0;

		assert(data->parent_type == PCB_PARENT_BOARD);
		return pcb_layer_purpose(data->parent.board, lid, out);
	}

	/* bound layer: if it is already bound to a real layer, use that, whatever it is (manual binding may override our local type match pattern) */
	if (layer->meta.bound.real != NULL) {
		layer = pcb_layer_get_real(layer);
		if (layer == NULL)
			return 0;
		return pcb_layer_purpose_(layer, out); /* tail recursion */
	}

	/* bound layer without a real layer binding: use the type match */
	if (out != NULL)
		*out = layer->meta.bound.purpose;
	return pcb_funchash_get(layer->meta.bound.purpose, NULL);
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

const pcb_virt_layer_t *pcb_vlayer_get_first(pcb_layer_type_t mask, const char *purpose, int purpi)
{
	const pcb_virt_layer_t *v;
	mask &= (~PCB_LYT_VIRTUAL);
	for(v = pcb_virt_layers; v->name != NULL; v++)
		if ((((v->type & (~PCB_LYT_VIRTUAL)) & mask) == mask) && PURP_MATCH(v->purpose, v->purpi))
			return v;
	return NULL;
}



int pcb_layer_list(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len)
{
	int n, used = 0;
	pcb_virt_layer_t *v;

	for(v = pcb_virt_layers; v->name != NULL; v++)
		if ((v->type & mask) == mask)
			APPEND_VIRT(v);

	for (n = 0; n < PCB_MAX_LAYER; n++)
		if ((pcb_layer_flags(pcb, n) & mask) == mask)
			APPEND(n);

	if (mask == PCB_LYT_UI)
		for (n = 0; n < vtp0_len(&pcb_uilayers); n++)
			if (pcb_uilayers.array[n] != NULL)
				APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}

/* optimization: code dup for speed */
int pcb_layer_listp(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len, int purpi, const char *purpose)
{
	int n, used = 0;
	pcb_virt_layer_t *v;


	for(v = pcb_virt_layers; v->name != NULL; v++)
		if (((v->type & mask) == mask) && (PURP_MATCH(v->purpose, v->purpi)))
			APPEND_VIRT(v);

	for (n = 0; n < PCB_MAX_LAYER; n++) {
		if ((pcb_layer_flags(pcb, n) & mask) == mask) {
			const char *lpurp;
			int lpurpi = pcb_layer_purpose(pcb, n, &lpurp);
			if (PURP_MATCH(lpurp, lpurpi))
				APPEND(n);
		}
	}

	if (mask == PCB_LYT_UI)
		for (n = 0; n < vtp0_len(&pcb_uilayers); n++)
			if ((pcb_uilayers.array[n] != NULL))
				APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}


int pcb_layer_list_any(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len)
{
	int n, used = 0;
	pcb_virt_layer_t *v;

	for(v = pcb_virt_layers; v->name != NULL; v++)
		if ((v->type & mask))
			APPEND_VIRT(v);

	for (n = 0; n < PCB_MAX_LAYER; n++)
		if ((pcb_layer_flags(pcb, n) & mask))
			APPEND(n);

	if (mask & PCB_LYT_UI)
		for (n = 0; n < vtp0_len(&pcb_uilayers); n++)
			if (pcb_uilayers.array[n] != NULL)
				APPEND(n | PCB_LYT_UI | PCB_LYT_VIRTUAL);

	return used;
}

pcb_layer_id_t pcb_layer_by_name(pcb_data_t *data, const char *name)
{
	pcb_layer_id_t n;
	for (n = 0; n < data->LayerN; n++)
		if (strcmp(data->Layer[n].name, name) == 0)
			return n;
	return -1;
}

void pcb_layers_reset(pcb_board_t *pcb)
{
	pcb_layer_id_t n;

	/* reset everything to empty, even if that's invalid (no top/bottom copper)
	   for the rest of the code: the (embedded) default design will overwrite this. */
	/* reset layers */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		if (pcb->Data->Layer[n].name != NULL)
			free((char *)pcb->Data->Layer[n].name);
		pcb->Data->Layer[n].name = pcb_strdup("<pcb_layers_reset>");
		pcb->Data->Layer[n].meta.real.grp = -1;
	}

	/* reset layer groups */
	for(n = 0; n < PCB_MAX_LAYERGRP; n++) {
		pcb->LayerGroups.grp[n].len = 0;
		pcb->LayerGroups.grp[n].ltype = 0;
		pcb->LayerGroups.grp[n].valid = 0;
	}
	pcb->LayerGroups.len = 0;
	pcb->Data->LayerN = 0;
}

/* empty and detach a layer - must be initialized or another layer moved over it later */
static void layer_clear(pcb_layer_t *dst)
{
	memset(dst, 0, sizeof(pcb_layer_t));
	dst->meta.real.grp = -1;
}

pcb_layer_id_t pcb_layer_create(pcb_board_t *pcb, pcb_layergrp_id_t grp, const char *lname)
{
	pcb_layer_id_t id;

	layer_if_too_many(pcb, return -1);

	id = pcb->Data->LayerN++;

	if (lname != NULL) {
		if (pcb->Data->Layer[id].name != NULL)
			free((char *)pcb->Data->Layer[id].name);
	}

	layer_clear(&pcb->Data->Layer[id]);
	pcb->Data->Layer[id].name = pcb_strdup(lname);

	/* add layer to group */
	if (grp >= 0) {
		pcb->LayerGroups.grp[grp].lid[pcb->LayerGroups.grp[grp].len] = id;
		pcb->LayerGroups.grp[grp].len++;
		pcb->Data->Layer[id].meta.real.vis = pcb->Data->Layer[pcb->LayerGroups.grp[grp].lid[0]].meta.real.vis;
	}
	pcb->Data->Layer[id].meta.real.grp = grp;
	pcb->Data->Layer[id].meta.real.color = *pcb_layer_default_color(id, (grp < 0) ? 0 : pcb->LayerGroups.grp[grp].ltype);

	pcb->Data->Layer[id].parent_type = PCB_PARENT_DATA;
	pcb->Data->Layer[id].parent.data = pcb->Data;
	pcb->Data->Layer[id].type = PCB_OBJ_LAYER;

	if (pcb == PCB)
		pcb_board_set_changed_flag(1);

	return id;
}

int pcb_layer_rename_(pcb_layer_t *Layer, char *Name)
{
	free((char*)Layer->name);
	Layer->name = Name;
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	return 0;
}

int pcb_layer_rename(pcb_data_t *data, pcb_layer_id_t layer, const char *lname)
{
	return pcb_layer_rename_(&data->Layer[layer], pcb_strdup(lname));
}

int pcb_layer_recolor_(pcb_layer_t *Layer, const pcb_color_t *color)
{
	if (Layer->is_bound)
		return -1;
	Layer->meta.real.color = *color;
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	return 0;
}

int pcb_layer_recolor(pcb_data_t *data, pcb_layer_id_t layer, const char *color)
{
	pcb_color_t clr;
	pcb_color_load_str(&clr, color);
	return pcb_layer_recolor_(&data->Layer[layer], &clr);
}

#undef APPEND

static int is_last_top_copper_layer(pcb_board_t *pcb, int layer)
{
	pcb_layergrp_id_t cgroup = pcb_layer_get_group(pcb, pcb->LayerGroups.len + PCB_COMPONENT_SIDE);
	pcb_layergrp_id_t lgroup = pcb_layer_get_group(pcb, layer);
	if (cgroup == lgroup && pcb->LayerGroups.grp[lgroup].len == 1)
		return 1;
	return 0;
}

static int is_last_bottom_copper_layer(pcb_board_t *pcb, int layer)
{
	int sgroup = pcb_layer_get_group(pcb, pcb->LayerGroups.len + PCB_SOLDER_SIDE);
	int lgroup = pcb_layer_get_group(pcb, layer);
	if (sgroup == lgroup && pcb->LayerGroups.grp[lgroup].len == 1)
		return 1;
	return 0;
}

/* Safe move of a layer within a layer array, updaging all fields (list->parents) */
void pcb_layer_move_(pcb_layer_t *dst, pcb_layer_t *src)
{
	pcb_line_t *li;
	pcb_text_t *te;
	pcb_poly_t *po;
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

const pcb_color_t *pcb_layer_default_color(int idx, pcb_layer_type_t lyt)
{
	const int clrs = sizeof(conf_core.appearance.color.layer) / sizeof(conf_core.appearance.color.layer[0]);

	if (lyt & PCB_LYT_MASK)
		return &conf_core.appearance.color.mask;
	if (lyt & PCB_LYT_PASTE)
		return &conf_core.appearance.color.paste;
	if (lyt & PCB_LYT_SILK)
		return &conf_core.appearance.color.element;

	return &conf_core.appearance.color.layer[idx % clrs];
}

/* Initialize a new layer with safe initial values */
static void layer_init(pcb_board_t *pcb, pcb_layer_t *lp, pcb_layer_id_t idx, pcb_layergrp_id_t gid, pcb_data_t *parent)
{
	memset(lp, 0, sizeof(pcb_layer_t));
	lp->meta.real.grp = gid;
	lp->meta.real.vis = 1;
	lp->name = pcb_strdup("New Layer");
	lp->meta.real.color = *pcb_layer_default_color(idx, (gid >= 0) ? pcb->LayerGroups.grp[gid].ltype : 0);
	if ((gid >= 0) && (pcb->LayerGroups.grp[gid].len == 0)) { /*When adding the first layer in a group, set up comb flags automatically */
		switch((pcb->LayerGroups.grp[gid].ltype) & PCB_LYT_ANYTHING) {
			case PCB_LYT_MASK:  lp->comb = PCB_LYC_AUTO | PCB_LYC_SUB; break;
			case PCB_LYT_SILK:  lp->comb = PCB_LYC_AUTO;
			case PCB_LYT_PASTE: lp->comb = PCB_LYC_AUTO;
			default: break;
		}
	}
	lp->parent.data = parent;
	lp->parent_type = PCB_PARENT_DATA;
	lp->type = PCB_OBJ_LAYER;
}

int pcb_layer_move(pcb_board_t *pcb, pcb_layer_id_t old_index, pcb_layer_id_t new_index, pcb_layergrp_id_t new_in_grp)
{
	pcb_layer_id_t l, at = -1;

	/* sanity checks */
	if (old_index < -1 || old_index >= pcb->Data->LayerN) {
		pcb_message(PCB_MSG_ERROR, "Invalid old layer %d for move: must be -1..%d\n", old_index, pcb->Data->LayerN - 1);
		return 1;
	}

	if (new_index < -1 || new_index > pcb->Data->LayerN || new_index >= PCB_MAX_LAYER) {
		pcb_message(PCB_MSG_ERROR, "Invalid new layer %d for move: must be -1..%d\n", new_index, pcb->Data->LayerN);
		return 1;
	}

	if (new_index == -1 && is_last_top_copper_layer(pcb, old_index)) {
		pcb_hid_message_box("warning", "Layer delete", "You can't delete the last top-side layer\n", "cancel", 0, NULL);
		return 1;
	}

	if (new_index == -1 && is_last_bottom_copper_layer(pcb, old_index)) {
		pcb_hid_message_box("warning", "Layer delete", "You can't delete the last bottom-side layer\n", "cancel", 0, NULL);
		return 1;
	}


	if (old_index == -1) { /* append new layer at the end of the logical layer list, put it in the current group */
		pcb_layergrp_t *g;
		pcb_layer_t *lp;
		pcb_layer_id_t new_lid = pcb->Data->LayerN++;
		int grp_idx;

		lp = &pcb->Data->Layer[new_lid];
		if (new_in_grp >= 0)
			layer_init(pcb, lp, new_lid, new_in_grp, pcb->Data);
		else
			layer_init(pcb, lp, new_lid, pcb->Data->Layer[new_index].meta.real.grp, pcb->Data);

		g = pcb_get_layergrp(pcb, lp->meta.real.grp);

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
		pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
		pcb_layervis_change_group_vis(new_lid, 1, 1);
		pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
		at = new_lid;
	}
	else if (new_index == -1) { /* Delete the layer at old_index */
		pcb_layergrp_id_t gid;
		pcb_layergrp_t *g;
		int grp_idx, remaining;

		/* remove the current lid from its group */
		g = pcb_get_layergrp(pcb, pcb->Data->Layer[old_index].meta.real.grp);
		grp_idx = pcb_layergrp_index_in_grp(g, old_index);
		if (grp_idx < 0) {
			pcb_message(PCB_MSG_ERROR, "Internal error; layer not in group\n");
			return -1;
		}

		pcb_layer_free_fields(&pcb->Data->Layer[old_index]);

		remaining = (g->len - grp_idx - 1);
		if (remaining > 0)
			memmove(g->lid+grp_idx, g->lid+grp_idx+1, remaining * sizeof(pcb_layer_id_t));
		g->len--;

		/* update lids in all groups (shifting down idx) */
		for(gid = 0; gid < pcb_max_group(pcb); gid++) {
			int n;
			g = &pcb->LayerGroups.grp[gid];
			for(n = 0; n < g->len; n++)
				if (g->lid[n] > old_index)
					g->lid[n]--;
		}

		/* update visibility */
		for(l = old_index; l < pcb->Data->LayerN-1; l++) {
			pcb_layer_move_(&pcb->Data->Layer[l], &pcb->Data->Layer[l+1]);
			layer_clear(&pcb->Data->Layer[l+1]);
		}

		for (l = 0; l < pcb->Data->LayerN; l++)
			if (pcb_layer_stack[l] == old_index)
				memmove(pcb_layer_stack + l, pcb_layer_stack + l + 1, (pcb->Data->LayerN - l - 1) * sizeof(pcb_layer_stack[0]));

		/* remove layer from the logical layer array */
		pcb_max_layer--;
		for (l = 0; l < pcb->Data->LayerN; l++)
			if (pcb_layer_stack[l] > old_index)
				pcb_layer_stack[l]--;

		pcb_event(&pcb->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Logical layer move is not supported any more. This function should have not been called. Please report this error.\n");
		/* Removed r8686:
		   The new layer design presents the layers by groups to preserve physical
		   order. In this system the index of the logical layer on the logical
		   layer list is insignificant, thus we shouldn't try to change it. */
	}

	if (at >= 0) {
		pcb_undo_add_layer_move(old_index, new_index, at);
		pcb_undo_inc_serial();
	}
	else
		pcb_message(PCB_MSG_WARNING, "this operation is not undoable.\n");

	return 0;
}

const char *pcb_layer_name(pcb_data_t *data, pcb_layer_id_t id)
{
	if (id < 0)
		return NULL;
	if (id < data->LayerN)
		return data->Layer[id].name;
	if ((id >= PCB_LAYER_VIRT_MIN) && (id <= PCB_LAYER_VIRT_MAX))
		return pcb_virt_layers[id-PCB_LAYER_VIRT_MIN].name;
	return NULL;
}

pcb_layer_t *pcb_get_layer(pcb_data_t *data, pcb_layer_id_t id)
{
	if ((id >= 0) && (id < data->LayerN))
		return &data->Layer[id];
	if (id & PCB_LYT_UI)
		return pcb_uilayer_get(id);
	return NULL;
}

pcb_layer_id_t pcb_layer_str2id(pcb_data_t *data, const char *str)
{
	char *end;
	pcb_layer_id_t id;
	if (*str == '#') {
		id = strtol(str+1, &end, 10);
		if ((*end == '\0') && (id >= 0) && (id < data->LayerN))
			return id;
	}
TODO("layer: do the same that cam does; test with propedit");
	return -1;
}


void pcb_layer_link_trees(pcb_layer_t *dst, pcb_layer_t *src)
{
	/* we can't link non-existing trees - make sure src does have the trees initialized */
	if (src->line_tree == NULL) src->line_tree = pcb_r_create_tree();
	if (src->arc_tree == NULL) src->arc_tree = pcb_r_create_tree();
	if (src->text_tree == NULL) src->text_tree = pcb_r_create_tree();
	if (src->polygon_tree == NULL) src->polygon_tree = pcb_r_create_tree();

	dst->line_tree = src->line_tree;
	dst->arc_tree = src->arc_tree;
	dst->text_tree = src->text_tree;
	dst->polygon_tree = src->polygon_tree;
}


void pcb_layer_real2bound_offs(pcb_layer_t *dst, pcb_board_t *src_pcb, pcb_layer_t *src)
{
	dst->meta.bound.real = NULL;
	dst->is_bound = 1;
	if ((dst->meta.bound.type & PCB_LYT_INTERN) && (dst->meta.bound.type & PCB_LYT_COPPER)) {
		int from_top, from_bottom, res;
		pcb_layergrp_t *tcop = pcb_get_grp(&src_pcb->LayerGroups, PCB_LYT_TOP, PCB_LYT_COPPER);
		pcb_layergrp_t *bcop = pcb_get_grp(&src_pcb->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
		pcb_layergrp_id_t tcop_id = pcb_layergrp_id(src_pcb, tcop);
		pcb_layergrp_id_t bcop_id = pcb_layergrp_id(src_pcb, bcop);

		res = pcb_layergrp_dist(src_pcb, src->meta.real.grp, tcop_id, PCB_LYT_COPPER, &from_top);
		res |= pcb_layergrp_dist(src_pcb, src->meta.real.grp, bcop_id, PCB_LYT_COPPER, &from_bottom);
		if (res == 0) {
			if (from_top <= from_bottom)
				dst->meta.bound.stack_offs = from_top;
			else
				dst->meta.bound.stack_offs = -from_bottom;
		}
		else
			pcb_message(PCB_MSG_ERROR, "Internal error: can't figure the inter copper\nlayer offset for %s\n", src->name);
	}
	else
		dst->meta.bound.stack_offs = 0;
}

void pcb_layer_real2bound(pcb_layer_t *dst, pcb_layer_t *src, int share_rtrees)
{
	pcb_board_t *pcb;
	pcb_layergrp_t *grp;

	assert(!src->is_bound);

	pcb = pcb_layer_get_top(src);
	grp = pcb_get_layergrp(pcb, src->meta.real.grp);
	dst->comb = src->comb;

	dst->meta.bound.type = pcb_layergrp_flags(pcb, src->meta.real.grp);
	if (src->name != NULL)
		dst->name = pcb_strdup(src->name);
	else
		dst->name = NULL;

	if ((grp != NULL) && (grp->purpose != NULL))
		dst->meta.bound.purpose = pcb_strdup(grp->purpose);
	else
		dst->meta.bound.purpose = NULL;

	pcb_layer_real2bound_offs(dst, pcb, src);

	if (!src->is_bound) {
		dst->meta.bound.real = src;
		if (share_rtrees)
			pcb_layer_link_trees(dst, src);
	}
}

static int strcmp_score(const char *s1, const char *s2)
{
	int score = 0;

	if (s1 == s2) /* mainly for NULL = NULL */
		score += 4;
	
	if ((s1 != NULL) && (s2 != NULL)) {
		if (strcmp(s1, s2) == 0)
			score += 4;
		else if (pcb_strcasecmp(s1, s2) == 0)
			score += 2;
	}

	return score;
}

void pcb_layer_resolve_best(pcb_board_t *pcb, pcb_layergrp_t *grp, pcb_layer_t *src, int *best_score, pcb_layer_t **best)
{
	int l, score;

	for(l = 0; l < grp->len; l++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, grp->lid[l]);
		if ((ly->comb & PCB_LYC_SUB) == (src->comb & PCB_LYC_SUB)) {
			score = 1;
			if (ly->comb == src->comb)
				score++;

			score += strcmp_score(ly->name, src->name);
			score += strcmp_score(grp->purpose, src->meta.bound.purpose);

			if (score > *best_score) {
				*best = ly;
				*best_score = score;
			}
		}
	}
}

pcb_layer_t *pcb_layer_resolve_binding(pcb_board_t *pcb, pcb_layer_t *src)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	int best_score = 0;
	pcb_layer_t *best = NULL;


	assert(src->is_bound);

	/* look up the layer group; for internal copper this means counting the offset */
	if ((src->meta.bound.type & PCB_LYT_INTERN) && (src->meta.bound.type & PCB_LYT_COPPER) && (src->meta.bound.stack_offs != 0)) {
		if (src->meta.bound.stack_offs < 0)
			gid = pcb_layergrp_get_bottom_copper();
		else
			gid = pcb_layergrp_get_top_copper();
		gid = pcb_layergrp_step(pcb, gid, src->meta.bound.stack_offs, PCB_LYT_COPPER | PCB_LYT_INTERN);
		/* target group identified; pick the closest match layer within that group */
		grp = pcb->LayerGroups.grp+gid;
		pcb_layer_resolve_best(pcb, grp, src, &best_score, &best);
	}
	else {
		pcb_layer_type_t lyt = src->meta.bound.type;
		if ((lyt & PCB_LYT_BOUNDARY) && (lyt & PCB_LYT_ANYWHERE)) {
			lyt = PCB_LYT_BOUNDARY;
			pcb_message(PCB_MSG_WARNING, "Ignoring invalid layer flag combination for %s: boundary layer must be global\n(fixed up by removing location specifier bits)\n", src->name);
		}
		for(gid = 0, grp = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,grp++)
			if ((grp->ltype & lyt) == lyt)
				pcb_layer_resolve_best(pcb, grp, src, &best_score, &best);
	}

	return best;
}

pcb_layer_t *pcb_layer_new_bound(pcb_data_t *data, pcb_layer_type_t type, const char *name, const char *purpose)
{
	pcb_layer_t *lay = &data->Layer[data->LayerN++];

	memset(lay, 0, sizeof(pcb_layer_t));
	lay->is_bound = 1;
	lay->name = pcb_strdup(name);
	lay->meta.bound.type = type;
	if (purpose == NULL)
		lay->meta.bound.purpose = NULL;
	else
		lay->meta.bound.purpose = pcb_strdup(purpose);
	lay->parent.data = data;
	lay->parent_type = PCB_PARENT_DATA;
	lay->type = PCB_OBJ_LAYER;
	return lay;
}

unsigned int pcb_layer_hash_bound(pcb_layer_t *ly, pcb_bool smirror)
{
	unsigned int hash;
	pcb_layer_type_t lyt;
	int offs;

	assert(ly->is_bound);

	lyt = ly->meta.bound.type;
	offs = ly->meta.bound.stack_offs;

	if (smirror) {
		lyt = pcb_layer_mirror_type(lyt);
		offs = -offs;
	}

	hash  = (unsigned long)arclist_length(&ly->Arc);
	hash ^= (unsigned long)linelist_length(&ly->Line);
	hash ^= (unsigned long)textlist_length(&ly->Text);
	hash ^= (unsigned long)polylist_length(&ly->Polygon);
	hash ^= (unsigned long)ly->comb ^ (unsigned long)lyt;

	if (ly->meta.bound.type & PCB_LYT_INTERN)
		hash ^= (unsigned int)offs;

	return hash;
}

pcb_layer_type_t pcb_layer_mirror_type(pcb_layer_type_t lyt)
{
	if (lyt & PCB_LYT_TOP)
		return (lyt & ~PCB_LYT_TOP) | PCB_LYT_BOTTOM;
	else if (lyt & PCB_LYT_BOTTOM)
		return (lyt & ~PCB_LYT_BOTTOM) | PCB_LYT_TOP;
	return lyt;
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

const char *pcb_layer_type_bit2str(pcb_layer_type_t type)
{
	const pcb_layer_type_name_t *n;
	for(n = pcb_layer_type_names; n->name != NULL; n++)
		if (type & n->type)
			return n->name;
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
	for(n = 0; n < pcb->Data->LayerN; n++)
		if (pcb_layer_flags(pcb, n) & PCB_LYT_SILK)
			pcb->Data->Layer[n].comb |= PCB_LYC_AUTO;
}

int pcb_layer_gui_set_vlayer(pcb_board_t *pcb, pcb_virtual_layer_t vid, int is_empty, pcb_xform_t **xform)
{
	pcb_virt_layer_t *v = &pcb_virt_layers[vid];
	assert((vid >= 0) && (vid < PCB_VLY_end));

	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

	if (xform != NULL)
		*xform = NULL;

TODO("layer: need to pass the flags of the group, not the flags of the layer once we have a group for each layer")
	if (pcb_gui->set_layer_group != NULL) {
		pcb_layergrp_id_t grp;
		pcb_layer_id_t lid = v->new_id;
		grp = pcb_layer_get_group(pcb, lid);
		return pcb_gui->set_layer_group(grp, v->purpose, v->purpi, lid, v->type, is_empty, xform);
	}

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
}

int pcb_layer_gui_set_g_ui(pcb_layer_t *first, int is_empty, pcb_xform_t **xform)
{
	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

	if (xform != NULL)
		*xform = NULL;

	if (pcb_gui->set_layer_group != NULL)
		return pcb_gui->set_layer_group(-1, NULL, -1, pcb_layer_id(first->parent.data, first), PCB_LYT_VIRTUAL | PCB_LYT_UI, is_empty, xform);

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
}

void pcb_layer_edit_attrib(pcb_layer_t *layer)
{
	char *buf = pcb_strdup_printf("Layer %s Attributes", layer->name);
	pcb_gui->edit_attributes(buf, &(layer->Attributes));
	free(buf);
}

const pcb_menu_layers_t *pcb_menu_layer_find(const char *name_or_abbrev)
{
	const pcb_menu_layers_t *ml;

	for(ml = pcb_menu_layers; ml->name != NULL; ml++)
		if (pcb_strcasecmp(name_or_abbrev, ml->abbrev) == 0)
			return ml;

	for(ml = pcb_menu_layers; ml->name != NULL; ml++)
		if (pcb_strcasecmp(name_or_abbrev, ml->name) == 0)
			return ml;

	return NULL;
}


static pcb_layer_id_t pcb_layer_get_cached(pcb_board_t *pcb, pcb_layer_id_t *cache, unsigned int loc, unsigned int typ)
{
	pcb_layergrp_t *g;

	if (*cache < pcb->Data->LayerN) { /* check if the cache is still pointing to the right layer */
		pcb_layergrp_id_t gid = pcb->Data->Layer[*cache].meta.real.grp;
		if ((gid >= 0) && (gid < pcb->LayerGroups.len)) {
			g = &(pcb->LayerGroups.grp[gid]);
			if ((g->ltype & loc) && (g->ltype & typ) && (g->lid[0] == *cache))
				return *cache;
		}
	}

	/* nope: need to resolve it again */
	g = pcb_get_grp(&pcb->LayerGroups, loc, typ);
	if ((g == NULL) || (g->len == 0)) {
		*cache = -1;
		return -1;
	}
	*cache = g->lid[0];
	return *cache;
}

pcb_layer_id_t pcb_layer_get_bottom_silk(void)
{
	static pcb_layer_id_t cache = -1;
	pcb_layer_id_t id = pcb_layer_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	return id;
}

pcb_layer_id_t pcb_layer_get_top_silk(void)
{
	static pcb_layer_id_t cache = -1;
	pcb_layer_id_t id = pcb_layer_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_SILK);
	return id;
}
