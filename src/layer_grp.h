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

typedef long int pcb_layergrp_id_t;

#include "layer.h"

/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
struct pcb_layer_group_s {
	pcb_cardinal_t Number[PCB_MAX_LAYERGRP],      /* number of entries per groups */
	  Entries[PCB_MAX_LAYERGRP][PCB_MAX_LAYER + 2];
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

/* Same as pcb_layer_list but lists layer groups. A group is matching
   if any layer in that group matches mask. */
int pcb_layer_group_list(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);
int pcb_layer_group_list_any(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len);

/* Looks up which group a layer is in. Returns group ID. */
pcb_layergrp_id_t pcb_layer_lookup_group(pcb_layer_id_t layer_id);

/* Put a layer in a group (the layer should not be in any other group) */
void pcb_layer_add_in_group(pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id);

/********* OBSOLETE functions, do not use in new code *********/
/* parses the group definition string which is a colon separated list of
   comma separated layer numbers (1,2,b:4,6,8,t); oldfmt is 0 or 1
   depending on PCB() or PCB[] in the file header.

   OBSOLETE, do not use in new code: only the conf system and io_pcb
   may need this. */
int pcb_layer_parse_group_string(const char *s, pcb_layer_group_t *LayerGroup, int LayerN, int oldfmt);

#endif
