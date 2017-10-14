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

typedef long int pcb_layer_id_t;
typedef long int pcb_layergrp_id_t;

/* Layer type bitfield */
typedef enum {
	/* Stack-up (vertical position): */
	PCB_LYT_TOP      = 0x00000001, /* layer is on the top side of the board */
	PCB_LYT_BOTTOM   = 0x00000002, /* layer is on the bottom side of the board */
	PCB_LYT_INTERN   = 0x00000004, /* layer is internal */
	PCB_LYT_LOGICAL  = 0x00000008, /* does not depend on the layer stackup (typically aux drawing layer) */
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
	PCB_LYT_PDRILL   = 0x00020000, /* (virtual) plated drills (affects all physical layers) */
	PCB_LYT_UDRILL   = 0x00040000, /* (virtual) unplated drills (affects all physical layers) */
	PCB_LYT_UI       = 0x00080000, /* (virtual) user interface drawings (feature plugins use this for displaying states or debug info) */
	PCB_LYT_CSECT    = 0x00100000, /* (virtual) cross-section drawing (displaying layer groups) */
	PCB_LYT_SUBSTRATE= 0x00200000, /* substrate / insulator */
	PCB_LYT_MISC     = 0x00400000, /* misc physical layers (e.g. adhesive) */
	PCB_LYT_DIALOG   = 0x00800000, /* (virtual) dialog box drawings (e.g. font selector) - not to be interpreted in the board space */
	PCB_LYT_ANYTHING = 0x00FFFF00, /* MASK: layers consist anything */

	/* misc properties */
	PCB_LYT_VIRTUAL  = 0x01000000, /* the layer is not in the layer array (generated layer) */
	PCB_LYT_NOEXPORT = 0x02000000, /* should not show up in any of the exports */
	PCB_LYT_ANYPROP  = 0x7F000000  /* MASK: misc layer properties */
} pcb_layer_type_t;


typedef enum {
	PCB_VLY_INVISIBLE,
	PCB_VLY_RATS,
	PCB_VLY_TOP_ASSY,
	PCB_VLY_BOTTOM_ASSY,
	PCB_VLY_FAB,
	PCB_VLY_PLATED_DRILL,
	PCB_VLY_UNPLATED_DRILL,
	PCB_VLY_CSECT,
	PCB_VLY_DIALOG,

	/* for determining the range, do not use */
	PCB_VLY_end,
	PCB_VLY_first = PCB_VLY_INVISIBLE
} pcb_virtual_layer_t;

typedef enum { /* bitfield */
	PCB_LYC_SUB = 1,        /* bit 0 is 0 for add or 1 for sub */
	PCB_LYC_AUTO = 2        /* bit 1 is 0 for manual drawn layer or 1 for auto-generated (from pins, pads, elements) */
} pcb_layer_combining_t;


#include "globalconst.h"
#include "global_typedefs.h"
#include "attrib.h"
#include "obj_all_list.h"

struct pcb_layer_s {              /* holds information about one layer */
	linelist_t Line;
	textlist_t Text;
	polylist_t Polygon;
	arclist_t Arc;
	pcb_data_t *parent;

	const char *name;              /* layer name */

	pcb_layer_combining_t comb;    /* how to combine this layer with other layers in the group */

	/* for bound layers these point to the board layer's*/
	pcb_rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;

	union {
		struct { /* A real board layer */
			pcb_layergrp_id_t grp;         /* the group this layer is in (cross-reference) */
			pcb_bool vis;                  /* visible flag */
			const char *color;             /* color */
			const char *selected_color;
			pcb_attribute_list_t Attributes;
			int no_drc;                    /* whether to ignore the layer when checking the design rules */
			const char *cookie;            /* for UI layers: registration cookie; NULL for unused UI layers */
		} real;
		struct { /* A subcircuit layer binding; list data are local but everything else is coming from board layers */
			pcb_layer_t *real;             /* NULL if unbound */

			/* matching rules */
			pcb_layer_type_t type;
			/* for comb, use the ->comb from the common part */
			int stack_offs;                /* offset in the stack for PCB_LYT_INNER: positive is counted from primary side, negative from the opposite side */

			unsigned user_specified:1;     /* 1 if the user forced the binding */
			pcb_layer_id_t user_lid;
		} bound;
	} meta;

	unsigned is_bound:1;
};

/* returns the layer number for the passed copper or silk layer pointer */
pcb_layer_id_t pcb_layer_id(pcb_data_t *Data, pcb_layer_t *Layer);


/* the offsets of the two additional special layers (e.g. silk) for 'component'
   and 'solder'. The offset of PCB_MAX_LAYER is not added here. Also can be
   used to address side of the board without referencing to groups or layers. */
typedef enum {
	PCB_SOLDER_SIDE    = 0,
	PCB_COMPONENT_SIDE = 1
} pcb_side_t;

/* Returns whether an auto paste layer is empty - it may be nonempty
   only because of element-side-effects */
pcb_bool pcb_layer_is_paste_auto_empty(pcb_board_t *pcb, pcb_side_t side);

/* Cached lookup of the first silk layer in the bottom or top group */
pcb_layer_id_t pcb_layer_get_bottom_silk();
pcb_layer_id_t pcb_layer_get_top_silk();

/* Return the board the layer is under */
#define pcb_layer_get_top(layer) pcb_data_get_top((layer)->parent)

/************ OLD API - new code should not use these **************/

#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[pcb_layer_stack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT	(LAYER_ON_STACK(0))
#define	INDEXOFCURRENT	(pcb_layer_stack[0])
#define SILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? pcb_layer_get_bottom_silk() : pcb_layer_get_top_silk())]
#define BACKSILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? pcb_layer_get_top_silk() : pcb_layer_get_bottom_silk())]

#define LAYER_LOOP(data, ml) do { \
        pcb_cardinal_t n; \
	for (n = 0; n < ml; n++) \
	{ \
	   pcb_layer_t *layer = (&data->Layer[(n)]);

/* Swap two layers in pcb; useful only in writing the old .pcb format,
   because silk layers must be the last 2 layers there */
int pcb_layer_swap(pcb_board_t *pcb, pcb_layer_id_t lid1, pcb_layer_id_t lid2);

/************ NEW API - new code should use these **************/

/* Return the layer pointer (or NULL on invalid or virtual layers) for an id */
pcb_layer_t *pcb_get_layer(pcb_data_t *data, pcb_layer_id_t id);

/* Return the name of a layer (real or virtual) or NULL on error
   NOTE: layer names may not be unique; returns the first case sensitive hit;
   slow linear search */
pcb_layer_id_t pcb_layer_by_name(pcb_data_t *data, const char *name);

/* Returns pcb_true if the given layer is empty (there are no objects on the layer) */
pcb_bool pcb_layer_is_empty_(pcb_board_t *pcb, pcb_layer_t *ly);
pcb_bool pcb_layer_is_empty(pcb_board_t *pcb, pcb_layer_id_t ly);

/* Returns pcb_true if the given layer is empty - non-recursive variant (doesn't deal with side effects) */
pcb_bool pcb_layer_is_pure_empty(pcb_layer_t *layer);


/* call the gui to set a virtual layer or the UI layer group */
int pcb_layer_gui_set_vlayer(pcb_board_t *pcb, pcb_virtual_layer_t vid, int is_empty);
int pcb_layer_gui_set_g_ui(pcb_layer_t *first, int is_empty);


/* returns a bitfield of pcb_layer_type_t; returns 0 if layer_idx or layer is invalid. */
unsigned int pcb_layer_flags(pcb_board_t *pcb, pcb_layer_id_t layer_idx);
unsigned int pcb_layer_flags_(pcb_layer_t *layer);

/* map bits of a layer type (call cb for each bit set); return number of bits
   found. */
int pcb_layer_type_map(pcb_layer_type_t type, void *ctx, void (*cb)(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name));
int pcb_layer_comb_map(pcb_layer_combining_t type, void *ctx, void (*cb)(void *ctx, pcb_layer_combining_t bit, const char *name));

/* return 0 or the flag value correspoding to name (linear search) */
pcb_layer_type_t pcb_layer_type_str2bit(const char *name);
pcb_layer_combining_t pcb_layer_comb_str2bit(const char *name);

/* Various fixes for old/plain/flat layer stack imports vs. composite layers */
void pcb_layer_auto_fixup(pcb_board_t *pcb);

/* List layer IDs that matches mask - write the first res_len items in res,
   if res is not NULL. Returns:
    - the total number of layers matching the query, if res is NULL
    - the number of layers copied into res if res is not NULL
   The plain version require exact match (look for a specific layer),
   the  _any version allows partial match so work with PCB_LYT_ANY*.
*/
int pcb_layer_list(pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len);
int pcb_layer_list_any(pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layer_id_t *res, int res_len);

/**** layer creation (for load/import code) ****/

/* Reset layers to the bare minimum (double sided board) */
void pcb_layers_reset(pcb_board_t *pcb);

/* Create a new layer and put it in an existing group (if grp is not -1). */
pcb_layer_id_t pcb_layer_create(pcb_board_t *pcb, pcb_layergrp_id_t grp, const char *lname);

/* Return the name of a layer (resolving the true name of virtual layers too) */
const char *pcb_layer_name(pcb_data_t *data, pcb_layer_id_t id);

/* Rename an existing layer by idx */
int pcb_layer_rename(pcb_data_t *data, pcb_layer_id_t layer, const char *lname);

/* changes the name of a layer; memory has to be already allocated */
int pcb_layer_rename_(pcb_layer_t *Layer, char *Name);


/* index is 0..PCB_MAX_LAYER-1.  If old_index is -1, a new layer is
   inserted at that index.  If new_index is -1, the specified layer is
   deleted.  Returns non-zero on error, zero if OK.  */
int pcb_layer_move(pcb_board_t *pcb, pcb_layer_id_t old_index, pcb_layer_id_t new_index, pcb_layergrp_id_t new_in_grp);


/* Set up dst so that it's a non-real layer bound to src */
void pcb_layer_real2bound(pcb_layer_t *dst, pcb_layer_t *src, int share_rtrees);

/* Assume src is a bound layer; find the closest match in pcb's layer stack */
pcb_layer_t *pcb_layer_resolve_binding(pcb_board_t *pcb, pcb_layer_t *src);

/* Allocate a new bound layer within data, set it up, but do not do the binding */
pcb_layer_t *pcb_layer_new_bound(pcb_data_t *data, pcb_layer_type_t type, const char *name);

/* Modify tree pointers in dst to point to src's; allocates trees for src if they are not yet allocated */
void pcb_layer_link_trees(pcb_layer_t *dst, pcb_layer_t *src);

/* Open the attribute editor for a layer */
void pcb_layer_edit_attrib(pcb_layer_t *layer);

/* list of virtual layers: these are generated by the draw code but do not
   have a real layer in the array */
typedef struct pcb_virt_layer_s {
	char *name;
	pcb_layer_id_t new_id;
	pcb_layer_type_t type;
} pcb_virt_layer_t;

extern pcb_virt_layer_t pcb_virt_layers[];

/* Return the first virtual layer that fully matches mask */
const pcb_virt_layer_t *pcb_vlayer_get_first(pcb_layer_type_t mask);

/* Returns whether the given unsigned int layer flags corresponds to a layer
   that's on the visible side of the board at the moment. */
#define PCB_LAYERFLG_ON_VISIBLE_SIDE(uint_flags) \
	(!!(conf_core.editor.show_solder_side ? ((uint_flags) & PCB_LYT_BOTTOM) : ((uint_flags) & PCB_LYT_TOP)))

#define PCB_LYT_VISIBLE_SIDE() \
	((conf_core.editor.show_solder_side ? PCB_LYT_BOTTOM : PCB_LYT_TOP))

#define PCB_LYT_INVISIBLE_SIDE() \
	((conf_core.editor.show_solder_side ? PCB_LYT_TOP : PCB_LYT_BOTTOM))

#endif
