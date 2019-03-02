/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

#ifndef PCB_LAYER_GRP_H
#define PCB_LAYER_GRP_H

#include "layer.h"

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */

struct pcb_layergrp_s {
	PCB_ANY_OBJ_FIELDS;
	pcb_cardinal_t len;                    /* number of layer IDs in use */
	pcb_layer_id_t lid[PCB_MAX_LAYER];     /* lid=layer ID */
	char *name;                            /* name of the physical layer (independent of the name of the layer groups) */

	pcb_layer_type_t ltype;
	char *purpose;                         /* what a doc/mech layer is used for */
	int purpi;                             /* integer version of purpose from the funtion hash (cache) */

	unsigned valid:1;                      /* 1 if it's a new-style, valid layer group; 0 after loading old files with no layer stackup info */
	unsigned vis:1;                        /* 1 if layer group is visible on the GUI */
	unsigned open:1;                       /* 1 if the group is open (expanded) on the GUI */

	/* internal: temporary states */
	int intern_id;                         /* for the old layer import mechanism */
};


/* layer stack: an ordered list of layer groups (physical layers). */
struct pcb_layer_stack_s {
	pcb_cardinal_t len;
	pcb_layergrp_t grp[PCB_MAX_LAYERGRP];
	struct { /* cache copper groups from top to bottom for fast padstack ("bbvia") lookup */
		int copper_len, copper_alloced;
		pcb_layergrp_id_t *copper;
		unsigned copper_valid:1; /* whether the cache is valid */
	} cache;
};

/* Free all fields of a layer stack */
void pcb_layergroup_free_stack(pcb_layer_stack_t *st);

/* Return the layer group for an id, or NULL on error (range check) */
pcb_layergrp_t *pcb_get_layergrp(pcb_board_t *pcb, pcb_layergrp_id_t gid);

/* Return the gid if grp is in the stackup of pcb (else return -1) */
pcb_layergrp_id_t pcb_layergrp_id(const pcb_board_t *pcb, const pcb_layergrp_t *grp);

/* Free a layer group (don't free the layers but detach them) */
int pcb_layergrp_free(pcb_board_t *pcb, pcb_layergrp_id_t id);

/* lookup the group to which a layer belongs to returns -1 if no group is found */
pcb_layergrp_id_t pcb_layer_get_group(pcb_board_t *pcb, pcb_layer_id_t Layer);
pcb_layergrp_id_t pcb_layer_get_group_(pcb_layer_t *Layer);

/* Returns group actually moved to (i.e. either group or previous) */
pcb_layergrp_id_t pcb_layer_move_to_group(pcb_board_t *pcb, pcb_layer_id_t layer, pcb_layergrp_id_t group);

/* Returns pcb_true if all layers in a group are empty */
pcb_bool pcb_layergrp_is_empty(pcb_board_t *pcb, pcb_layergrp_id_t lgrp);
pcb_bool pcb_layergrp_is_pure_empty(pcb_board_t *pcb, pcb_layergrp_id_t num);

/* call the gui to set a layer group */
int pcb_layer_gui_set_layer(pcb_layergrp_id_t gid, const pcb_layergrp_t *grp, int is_empty, pcb_xform_t **xform);
int pcb_layer_gui_set_glayer(pcb_board_t *pcb, pcb_layergrp_id_t grp, int is_empty, pcb_xform_t **xform);

/* returns a bitfield of pcb_layer_type_t; returns 0 if layer_idx is invalid. */
unsigned int pcb_layergrp_flags(const pcb_board_t *pcb, pcb_layergrp_id_t group_idx);
const char *pcb_layergrp_name(pcb_board_t *pcb, pcb_layergrp_id_t gid);

/* Same as pcb_layer_list but lists layer groups. A group is matching
   if any layer in that group matches mask. */
int pcb_layergrp_list(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);
int pcb_layergrp_listp(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len, int purpi, const char *purpose);
int pcb_layergrp_list_any(const pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);

/* Put a layer in a group (the layer should not be in any other group);
   returns 0 on success */
int pcb_layer_add_in_group(pcb_board_t *pcb, pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id);
int pcb_layer_add_in_group_(pcb_board_t *pcb, pcb_layergrp_t *grp, pcb_layergrp_id_t group_id, pcb_layer_id_t layer_id);

/* Remove a layer group; if del_layers is zero, layers are kept but detached
   (.grp = -1), else layers are deleted too */
int pcb_layergrp_del(pcb_board_t *pcb, pcb_layergrp_id_t gid, int del_layers);

/* Remove a layer from a group */
int pcb_layergrp_del_layer(pcb_board_t *pcb, pcb_layergrp_id_t gid, pcb_layer_id_t lid);

/* Duplicate a layer group (with no layers); if auto_substrate is set, insert
   a substrate layer automatically if needed */
pcb_layergrp_id_t pcb_layergrp_dup(pcb_board_t *pcb, pcb_layergrp_id_t gid, int auto_substrate);

/* Move gfrom to to_before and shift the stack as necessary. Return -1 on range error */
int pcb_layergrp_move(pcb_board_t *pcb, pcb_layergrp_id_t gfrom, pcb_layergrp_id_t to_before);

/* Move src onto dst, not shifting the stack, free()'ing and overwriting dst, leaving a gap (0'd slot) at src */
int pcb_layergrp_move_onto(pcb_board_t *pcb, pcb_layergrp_id_t dst, pcb_layergrp_id_t src);


/* Insert a new layer group in the layer stack after the specified group */
pcb_layergrp_t *pcb_layergrp_insert_after(pcb_board_t *pcb, pcb_layergrp_id_t where);

/* Move lid 1 step towards the front (delta=-1) or end (delta=+1) of the
   layer list of the group. Return 0 on success (even when already reached
   the end of the list) or -1 on error */
int pcb_layergrp_step_layer(pcb_board_t *pcb, pcb_layergrp_t *grp, pcb_layer_id_t lid, int delta);

/* Return the array index of lid within the grp's lid list or -1 if not on the list */
int pcb_layergrp_index_in_grp(pcb_layergrp_t *grp, pcb_layer_id_t lid);

/* Calculate the distance between gid1 and gid2 in the stack, in number of
   layer groups matching mask. The result is placed in *diff. Returns 0 on success.
   Typical use case: gid1 is an internal copper layer, gid2 is the top copper layer,
   mask is PCB_LYT_COPPER -> *diff is the copper index of the internal layer from top */
int pcb_layergrp_dist(pcb_board_t *pcb, pcb_layergrp_id_t gid1, pcb_layergrp_id_t gid2, pcb_layer_type_t mask, int *diff);

/* walk the given amount steps on the stack among layer groups matching mask.
   A negative step goes upward.
   Typical use: "return the 2nd copper layer below this one" */
pcb_layergrp_id_t pcb_layergrp_step(pcb_board_t *pcb, pcb_layergrp_id_t gid, int steps, pcb_layer_type_t mask);


/* Enable/disable inhibition of layer changed events during layer group updates */
void pcb_layergrp_inhibit_inc(void);
void pcb_layergrp_inhibit_dec(void);

/* Send out a layer changed event if not inhibited */
void pcb_layergrp_notify(pcb_board_t *pcb);

/* Rename an existing layer by idx */
int pcb_layergrp_rename(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *lname);

/* changes the name of a layer; memory has to be already allocated */
int pcb_layergrp_rename_(pcb_layergrp_t *grp, char *name);

/* Change the purpose field and recalc purpi (not undoable) */
int pcb_layergrp_set_purpose__(pcb_layergrp_t *lg, char *purpose); /* no strdup, no event */
int pcb_layergrp_set_purpose_(pcb_layergrp_t *lg, char *purpose); /* no strdup, send layer change event */
int pcb_layergrp_set_purpose(pcb_layergrp_t *lg, const char *purpose); /* strdup, send event */


/* Slow linear search for a layer group by name */
pcb_layergrp_id_t pcb_layergrp_by_name(pcb_board_t *pcb, const char *name);

/* Create a layer for each unbindable layers from the layer array */
int pcb_layer_create_all_for_recipe(pcb_board_t *pcb, pcb_layer_t *layer, int num_layer);

/* Upgrade the current layer stack to incldue all layers for
   fully representaing standard padstacks. This includes reusing or
   creating layer groups and layers, altering layer group names (but
   not removing layers or groups) */
void pcb_layergrp_upgrade_to_pstk(pcb_board_t *pcb);

/* Compatibility helper: insert a substrate group in between any two adjacent
   copper groups. */
void pcb_layergrp_create_missing_substrate(pcb_board_t *pcb);


/********* OBSOLETE functions, do not use in new code *********/
/* parses the group definition string which is a colon separated list of
   comma separated layer numbers (1,2,b:4,6,8,t); oldfmt is 0 or 1
   depending on PCB() or PCB[] in the file header.

   OBSOLETE, do not use in new code: only the conf system and io_pcb
   may need this. */
int pcb_layer_parse_group_string(pcb_board_t *pcb, const char *s, int LayerN, int oldfmt);

#define PCB_COPPER_GROUP_LOOP(data, group) do { 	\
	pcb_cardinal_t entry; \
	pcb_board_t *cgl__pcb = pcb_data_get_top(data); \
	if (cgl__pcb == NULL) \
		cgl__pcb = PCB; \
        for (entry = 0; entry < ((pcb_board_t *)(cgl__pcb))->LayerGroups.grp[(group)].len; entry++) \
        { \
		pcb_layer_t *layer;		\
		pcb_layer_id_t number; 		\
		number = ((pcb_board_t *)(cgl__pcb))->LayerGroups.grp[(group)].lid[entry]; \
		if (!(pcb_layer_flags(PCB, number) & PCB_LYT_COPPER)) \
		  continue;			\
		layer = &data->Layer[number];

/* for parsing old files with old layer descriptions, with no layer groups */
void pcb_layer_group_setup_default(pcb_board_t *pcb); /* default layer groups, no layers */
void pcb_layer_group_setup_silks(pcb_board_t *pcb); /* make sure we have two silk layers, add them if needed */
pcb_layergrp_t *pcb_get_grp(pcb_layer_stack_t *stack, pcb_layer_type_t loc, pcb_layer_type_t typ);
pcb_layergrp_t *pcb_get_grp_new_intern(pcb_board_t *pcb, int intern_id);
pcb_layergrp_t *pcb_get_grp_new_misc(pcb_board_t *pcb);

/* append a layer group without any smart side effects - use only from io loaders */
pcb_layergrp_t *pcb_get_grp_new_raw(pcb_board_t *pcb);

/* ugly hack: remove the extra substrate we added after the outline layer */
void pcb_layergrp_fix_old_outline(pcb_board_t *pcb);

/* ugly hack: turn an old intern layer group into an outline group after realizing it is really an outline (reading the old layers) */
void pcb_layergrp_fix_turn_to_outline(pcb_layergrp_t *g);

/* ugly hack: look at layers and convert g into an outline group if needed */
void pcb_layergrp_fix_old_outline_detect(pcb_board_t *pcb, pcb_layergrp_t *g);


/* Cached layer group lookups foir a few common cases */
pcb_layergrp_id_t pcb_layergrp_get_bottom_mask();
pcb_layergrp_id_t pcb_layergrp_get_top_mask();
pcb_layergrp_id_t pcb_layergrp_get_bottom_paste();
pcb_layergrp_id_t pcb_layergrp_get_top_paste();
pcb_layergrp_id_t pcb_layergrp_get_bottom_silk();
pcb_layergrp_id_t pcb_layergrp_get_top_silk();
pcb_layergrp_id_t pcb_layergrp_get_bottom_copper();
pcb_layergrp_id_t pcb_layergrp_get_top_copper();


/* return whether any silk or mask or paste layer group is visible */
int pcb_silk_on(pcb_board_t *pcb);
int pcb_mask_on(pcb_board_t *pcb);
int pcb_paste_on(pcb_board_t *pcb);

/* recalculate the copper cache */
void pcb_layergrp_copper_cache_update(pcb_layer_stack_t *st);

/* default layer group map */
typedef struct pcb_dflgmap_s {
	const char *name;
	pcb_layer_type_t lyt;
	const char *purpose;
	pcb_layer_combining_t comb;
	int force_end;
} pcb_dflgmap_t;

extern const pcb_dflgmap_t pcb_dflgmap[]; /* the whole map */

/* pointers into the array marking boundaries */
extern const pcb_dflgmap_t *pcb_dflgmap_last_top_noncopper;
extern const pcb_dflgmap_t *pcb_dflgmap_first_bottom_noncopper;

/* predefined common default map entries for building a stack */
extern const pcb_dflgmap_t pcb_dflg_top_copper;
extern const pcb_dflgmap_t pcb_dflg_int_copper;
extern const pcb_dflgmap_t pcb_dflg_substrate;
extern const pcb_dflgmap_t pcb_dflg_bot_copper;
extern const pcb_dflgmap_t pcb_dflg_outline;


/* Overwrite an existing group from a default layer group map entry and create
   a layer in the group */
void pcb_layergrp_set_dflgly(pcb_board_t *pcb, pcb_layergrp_t *grp, const pcb_dflgmap_t *src, const char *grname, const char *lyname);

pcb_layergrp_id_t pcb_layergrp_str2id(pcb_board_t *pcb, const char *str);

/* Return true if the board has an outline layer with at least one object on it */
pcb_bool pcb_has_explicit_outline(pcb_board_t *pcb);

#endif
