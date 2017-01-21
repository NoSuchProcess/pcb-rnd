/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_LAYER_GRP_H
#define PCB_LAYER_GRP_H

typedef struct pcb_layer_group_s pcb_layer_group_t;

#include "layer.h"

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */

struct pcb_layer_group_s {
	pcb_cardinal_t len;                    /* number of layer IDs in use */
	pcb_layer_id_t lid[PCB_MAX_LAYER];     /* lid=layer ID */
	char *name;                            /* name of the physical layer (independent of the name of the layer groups) */
	pcb_layer_type_t type;

	unsigned valid:1;                      /* 1 if it's a new-style, valid layer group; 0 after loading old files with no layer stackup info */
	unsigned vis:1;                        /* 1 if layer group is visible on the GUI */
};


/* layer stack: an ordered list of layer groups (physical layers). */
struct pcb_layer_stack_s {
	pcb_cardinal_t len;
	pcb_layer_group_t grp[PCB_MAX_LAYERGRP];
};

/* lookup the group to which a layer belongs to returns -1 if no group is found */
pcb_layergrp_id_t pcb_layer_get_group(pcb_layer_id_t Layer);
pcb_layergrp_id_t pcb_layer_get_group_(pcb_layer_t *Layer);

/* Returns group actually moved to (i.e. either group or previous) */
pcb_layergrp_id_t pcb_layer_move_to_group(pcb_layer_id_t layer, pcb_layergrp_id_t group);

/* Returns pcb_true if all layers in a group are empty */
pcb_bool pcb_is_layergrp_empty(pcb_layergrp_id_t lgrp);

/* call the gui to set a layer group */
int pcb_layer_gui_set_glayer(pcb_layergrp_id_t grp, int is_empty);

/* returns a bitfield of pcb_layer_type_t; returns 0 if layer_idx is invalid. */
unsigned int pcb_layergrp_flags(pcb_layergrp_id_t group_idx);
const char *pcb_layergrp_name(pcb_layergrp_id_t gid);

/* Same as pcb_layer_list but lists layer groups. A group is matching
   if any layer in that group matches mask. */
int pcb_layer_group_list(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);
int pcb_layer_group_list_any(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);

/* Looks up which group a layer is in. Returns group ID. */
pcb_layergrp_id_t pcb_layer_lookup_group(pcb_layer_id_t layer_id);

/* Put a layer in a group (the layer should not be in any other group);
   returns 0 on success */
int pcb_layer_add_in_group(pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id);

/* Remove a layer group; if del_layers is zero, layers are kept but detached
   (.grp = -1), else layers are deleted too */
int pcb_layergrp_del(pcb_layer_stack_t *stk, pcb_layergrp_id_t gid, int del_layers);


/********* OBSOLETE functions, do not use in new code *********/
/* parses the group definition string which is a colon separated list of
   comma separated layer numbers (1,2,b:4,6,8,t); oldfmt is 0 or 1
   depending on PCB() or PCB[] in the file header.

   OBSOLETE, do not use in new code: only the conf system and io_pcb
   may need this. */
int pcb_layer_parse_group_string(const char *s, pcb_layer_stack_t *LayerGroup, int LayerN, int oldfmt);

#define PCB_COPPER_GROUP_LOOP(data, group) do { 	\
	pcb_cardinal_t entry; \
        for (entry = 0; entry < ((pcb_board_t *)(data->pcb))->LayerGroups.grp[(group)].len; entry++) \
        { \
		pcb_layer_t *layer;		\
		pcb_layer_id_t number; 		\
		number = ((pcb_board_t *)(data->pcb))->LayerGroups.grp[(group)].lid[entry]; \
		if (!(pcb_layer_flags(number) & PCB_LYT_COPPER)) \
		  continue;			\
		layer = &data->Layer[number];

/* transitional code: old loaders load the old layer stack, convert to the new
   (and back before save) */
void pcb_layer_group_from_old(pcb_board_t *pcb);
void pcb_layer_group_to_old(pcb_board_t *pcb);

/* for parsing old files with old layer descriptions, with no layer groups */
void pcb_layer_group_setup_default(pcb_layer_stack_t *newg);
pcb_layer_group_t *pcb_get_grp(pcb_layer_stack_t *stack, pcb_layer_type_t loc, pcb_layer_type_t typ);
pcb_layer_group_t *pcb_get_grp_new_intern(pcb_layer_stack_t *stack);
pcb_layer_group_t *pcb_get_grp_new_misc(pcb_layer_stack_t *stack);

/* ugly hack: remove the extra substrate we added after the outline layer */
void pcb_layergrp_fix_old_outline(pcb_layer_stack_t *LayerGroup);

/* ugly hack: turn an old intern layer group into an outline group after realizing it is really an outline (reading the old layers) */
void pcb_layergrp_fix_turn_to_outline(pcb_layer_group_t *g);



/* Cached layer group lookups foir a few common cases */
pcb_layergrp_id_t pcb_layergrp_get_bottom_mask();
pcb_layergrp_id_t pcb_layergrp_get_top_mask();
pcb_layergrp_id_t pcb_layergrp_get_bottom_paste();
pcb_layergrp_id_t pcb_layergrp_get_top_paste();

#endif
