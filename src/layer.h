/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

#ifndef PCB_LAYER_H
#define PCB_LAYER_H

#include "globalconst.h"
#include "global_typedefs.h"
#include "attrib.h"
#include "obj_all_list.h"

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
struct pcb_layer_group_s {
	pcb_cardinal_t Number[PCB_MAX_LAYERGRP],      /* number of entries per groups */
	  Entries[PCB_MAX_LAYERGRP][PCB_MAX_LAYER + 2];
};

struct pcb_layer_s {              /* holds information about one layer */
	const char *Name;               /* layer name */
	linelist_t Line;
	textlist_t Text;
	polylist_t Polygon;
	arclist_t Arc;
	pcb_rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;
	pcb_bool On;                   /* visible flag */
	const char *Color;             /* color */
	const char *SelectedColor;
	pcb_attribute_list_t Attributes;
	int no_drc;                    /* whether to ignore the layer when checking the design rules */
};

/************ OLD API - new code should not use these **************/

int ParseGroupString(const char *, pcb_layer_group_t *, int /* LayerN */ );

int GetLayerNumber(pcb_data_t *, pcb_layer_t *);
int GetLayerGroupNumberByPointer(pcb_layer_t *);
int GetLayerGroupNumberByNumber(pcb_cardinal_t);
int GetGroupOfLayer(int);
int ChangeGroupVisibility(int, pcb_bool, pcb_bool);
void LayerStringToLayerStack(const char *);

pcb_bool IsPasteEmpty(int side);

void ResetStackAndVisibility(void);
void SaveStackAndVisibility(void);
void RestoreStackAndVisibility(void);

/* Layer Group Functions */

/* Returns group actually moved to (i.e. either group or previous) */
int MoveLayerToGroup(int layer, int group);

/* Returns pointer to private buffer */
char *LayerGroupsToString(pcb_layer_group_t *);

#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[pcb_layer_stack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT			(PCB->SilkActive ? &PCB->Data->Layer[ \
				(conf_core.editor.show_solder_side ? pcb_solder_silk_layer : pcb_component_silk_layer)] \
				: LAYER_ON_STACK(0))
#define	INDEXOFCURRENT		(PCB->SilkActive ? \
				(conf_core.editor.show_solder_side ? pcb_solder_silk_layer : pcb_component_silk_layer) \
				: pcb_layer_stack[0])
#define SILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? pcb_solder_silk_layer : pcb_component_silk_layer)]
#define BACKSILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? pcb_component_silk_layer : pcb_solder_silk_layer)]

#define TEST_SILK_LAYER(layer)	(GetLayerNumber (PCB->Data, layer) >= pcb_max_copper_layer)

/* These decode the set_layer index. */
#define SL_TYPE(x) ((x) < 0 ? (x) & 0x0f0 : 0)
#define SL_SIDE(x) ((x) & 0x00f)
#define SL_MYSIDE(x) ((((x) & SL_BOTTOM_SIDE)!=0) == (PCB_SWAP_IDENT != 0))

#define SL_0_SIDE	0x0000
#define SL_TOP_SIDE	0x0001
#define SL_BOTTOM_SIDE	0x0002
#define SL_SILK		0x0010
#define SL_MASK		0x0020
#define SL_PDRILL	0x0030
#define SL_UDRILL	0x0040
#define SL_PASTE	0x0050
#define SL_INVISIBLE	0x0060
#define SL_FAB		0x0070
#define SL_ASSY		0x0080
#define SL_RATS		0x0090
/* Callers should use this.  */
#define SL(type,side) (~0xfff | SL_##type | SL_##side##_SIDE)


#define LAYER_IS_OUTLINE(idx) (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[idx].Name, "outline") == 0)

#define GROUP_LOOP(data, group) do { 	\
	pcb_cardinal_t entry; \
        for (entry = 0; entry < ((pcb_board_t *)(data->pcb))->LayerGroups.Number[(group)]; entry++) \
        { \
		pcb_layer_t *layer;		\
		pcb_cardinal_t number; 		\
		number = ((pcb_board_t *)(data->pcb))->LayerGroups.Entries[(group)][entry]; \
		if (number >= pcb_max_copper_layer)	\
		  continue;			\
		layer = &data->Layer[number];

#define LAYER_LOOP(data, ml) do { \
        pcb_cardinal_t n; \
	for (n = 0; n < ml; n++) \
	{ \
	   pcb_layer_t *layer = (&data->Layer[(n)]);


#define PCB_LAYER_IS_EMPTY(layer) PCB_LAYER_IS_EMPTY_((layer))
#define PCB_LAYER_IS_EMPTY_(layer) \
 ((linelist_length(&layer->Line) == 0) && (arclist_length(&layer->Arc) == 0) && (polylist_length(&layer->Polygon) == 0) && (textlist_length(&layer->Text) == 0))


/* ---------------------------------------------------------------------------
 * the layer-numbers of the two additional special layers
 * 'component' and 'solder'. The offset of PCB_MAX_LAYER is not added
 */
#define	SOLDER_LAYER		0
#define	COMPONENT_LAYER		1


/************ NEW API - new code should use these **************/

typedef long int pcb_layer_id_t;
typedef long int pcb_layergrp_id_t;

/* Return the layer pointer (or NULL on invalid or virtual layers) for an id */
pcb_layer_t *pcb_get_layer(pcb_layer_id_t id);

/* Return the name of a layer (real or virtual) or NULL on error */
const char *pcb_layer_name(pcb_layer_id_t id);

/* Returns pcb_true if the given layer is empty (there are no objects on the layer) */
pcb_bool pcb_layer_is_empty_(pcb_layer_t *ly);
pcb_bool pcb_layer_is_empty(pcb_layer_id_t ly);

/* Returns pcb_true if all layers in a group are empty */
pcb_bool pcb_is_layergrp_empty(pcb_layergrp_id_t lgrp);

/* Layer type bitfield */
typedef enum {
	/* Stack-up (vertical position): */
	PCB_LYT_TOP      = 0x00000001, /* layer is on the top side of the board */
	PCB_LYT_BOTTOM   = 0x00000002, /* layer is on the bottom side of the board */
	PCB_LYT_INTERN   = 0x00000004, /* layer is internal */
	PCB_LYT_LOGICAL  = 0x00000008, /* not in the layer stackup (typically aux drawing layer ) */
	PCB_LYT_ANYWHERE = 0x000000FF, /* MASK: layer is anywhere on the stack */

	/* What the layer consists of */
	PCB_LYT_COPPER   = 0x00000100, /* copper objects */
	PCB_LYT_SILK     = 0x00000200, /* silk objects */
	PCB_LYT_MASK     = 0x00000400, /* solder mask */
	PCB_LYT_PASTE    = 0x00000800, /* paste */
	PCB_LYT_OUTLINE  = 0x00001000, /* outline (contour of the board) */
	PCB_LYT_RAT      = 0x00002000, /* (virtual) rats nest (one, not in the stackup) */
	PCB_LYT_INVIS    = 0x00004000, /* (virtual) layer is invisible (one, not in the stackup) */
	PCB_LYT_ASSY     = 0x00008000, /* (virtual) assembly drawing (top and bottom) */
	PCB_LYT_FAB      = 0x00010000, /* (virtual) fab drawing (one, not in the stackup) */
	PCB_LYT_ANYTHING = 0x00FFFF00, /* MASK: layers consist anything */

	/* misc properties */
	PCB_LYT_VIRTUAL  = 0x01000000, /* the layer is not in the layer array (generated layer) */
	PCB_LYT_ANYPROP  = 0x7F000000  /* MASK: misc layer properties */
} pcb_layer_type_t;


/* returns a bitfield of pcb_layer_type_t; returns 0 if layer_idx is invalid. */
unsigned int pcb_layer_flags(pcb_layer_id_t layer_idx);

/* map bits of a layer type (call cb for each bit set); return number of bits
   found. */
int pcb_layer_type_map(pcb_layer_type_t type, void *ctx, void (*cb)(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name));

/* List layer IDs that matches mask - write the first res_len items in res,
   if res is not NULL. Returns:
    - the total number of layers matching the query, if res is NULL
    - the number of layers copied into res if res is not NULL
   The plain version require exact match (look for a specific layer),
   the  _any version allows partial match so work with PCB_LYT_ANY*.
*/
int pcb_layer_list(pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len);
int pcb_layer_list_any(pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len);

/* Same as pcb_layer_list but lists layer groups. A group is matching
   if any layer in that group matches mask. */
int pcb_layer_group_list(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);
int pcb_layer_group_list_any(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);

/* Looks up which group a layer is in. Returns group ID. */
pcb_layergrp_id_t pcb_layer_lookup_group(pcb_layer_id_t layer_id);

/* Put a layer in a group (the layer should not be in any other group) */
void pcb_layer_add_in_group(pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id);

/* Slow linear search for a layer by name */
pcb_layer_id_t pcb_layer_by_name(const char *name);

/**** layer creation (for load/import code) ****/

/* Reset layers to the bare minimum (double sided board) */
void pcb_layers_reset();

/* If reuse_layer is false, create a new layer of the given type; if
   reuse_group is true, try to put the new layer on an existing group.
   If reuse_layer is 1, first try to return an already exiting layer that
   matches type and create a new one if that fails.
   Upon creating a new layer, name it according to lname if it is not NULL
   Returns a layer index (or -1 on error)
   Do not create: mask, silk, paste; they are special layers.
   */
pcb_layer_id_t pcb_layer_create(pcb_layer_type_t type, pcb_bool reuse_layer, pcb_bool_t reuse_group, const char *lname);

/* Rename an existing layer by idx */
int pcb_layer_rename(pcb_layer_id_t layer, const char *lname);

/* changes the name of a layer; memory has to be already allocated */
int pcb_layer_rename_(pcb_layer_t *Layer, char *Name);


/* Needs to be called once at the end, when all layers has been added */
void pcb_layers_finalize();


/* index is 0..PCB_MAX_LAYER-1.  If old_index is -1, a new layer is
   inserted at that index.  If new_index is -1, the specified layer is
   deleted.  Returns non-zero on error, zero if OK.  */
int pcb_layer_move(pcb_layer_id_t old_index, pcb_layer_id_t new_index);



/* list of virtual layers: these are generated by the draw code but do not
   have a real layer in the array */
typedef struct pcb_virt_layer_s {
	char *name;
	unsigned int old_id;
	pcb_layer_id_t new_id;
	pcb_layer_type_t type;
} pcb_virt_layer_t;

extern pcb_virt_layer_t pcb_virt_layers[];



#endif
