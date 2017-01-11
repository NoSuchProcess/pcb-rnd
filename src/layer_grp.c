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
#include "layer_grp.h"
#include "compat_misc.h"

pcb_layergrp_id_t pcb_layer_get_group(pcb_layer_id_t Layer)
{
	pcb_layergrp_id_t group, i;

	if ((Layer < 0) || (Layer >= pcb_max_layer))
		return -1;

	for (group = 0; group < pcb_max_group; group++)
		for (i = 0; i < PCB->LayerGroups.grp[group].len; i++)
			if (PCB->LayerGroups.grp[group].lid[i] == Layer)
				return (group);

	return -1;
}

pcb_layergrp_id_t pcb_layer_get_group_(pcb_layer_t *Layer)
{
	return pcb_layer_get_group(pcb_layer_id(PCB->Data, Layer));
}

pcb_layergrp_id_t pcb_layer_move_to_group(pcb_layer_id_t layer, pcb_layergrp_id_t group)
{
	pcb_layergrp_id_t prev, i, j;

	if (layer < 0 || layer >= pcb_max_layer)
		return -1;
	prev = pcb_layer_get_group(layer);
	if ((layer == pcb_solder_silk_layer && group == pcb_layer_get_group(pcb_component_silk_layer))
			|| (layer == pcb_component_silk_layer && group == pcb_layer_get_group(pcb_solder_silk_layer))
			|| (group < 0 || group >= pcb_max_group) || (prev == group))
		return prev;

	/* Remove layer from prev group */
	for (j = i = 0; i < PCB->LayerGroups.grp[prev].len; i++)
		if (PCB->LayerGroups.grp[prev].lid[i] != layer)
			PCB->LayerGroups.grp[prev].lid[j++] = PCB->LayerGroups.grp[prev].lid[i];
	PCB->LayerGroups.grp[prev].len--;

	/* Add layer to new group.  */
	i = PCB->LayerGroups.grp[group].len++;
	PCB->LayerGroups.grp[group].lid[i] = layer;

	return group;
}

unsigned int pcb_layergrp_flags(pcb_layergrp_id_t group)
{
	unsigned int res = 0;
	int layeri;

	for (layeri = 0; layeri < PCB->LayerGroups.grp[group].len; layeri++)
		res |= pcb_layer_flags(PCB->LayerGroups.grp[group].lid[layeri]);

	return res;
}

pcb_bool pcb_is_layergrp_empty(pcb_layergrp_id_t num)
{
	int i;
	for (i = 0; i < PCB->LayerGroups.grp[num].len; i++)
		if (!pcb_layer_is_empty(PCB->LayerGroups.grp[num].lid[i]))
			return pcb_false;
	return pcb_true;
}

int pcb_layer_parse_group_string(const char *s, pcb_layer_stack_t *LayerGroup, int LayerN, int oldfmt)
{
	int group, member, layer;
	pcb_bool c_set = pcb_false,						/* flags for the two special layers to */
		s_set = pcb_false;							/* provide a default setting for old formats */
	int groupnum[PCB_MAX_LAYERGRP + 2];

	/* clear struct */
	memset(LayerGroup, 0, sizeof(pcb_layer_stack_t));

	/* Clear assignments */
	for (layer = 0; layer < PCB_MAX_LAYER + 2; layer++)
		groupnum[layer] = -1;

	/* loop over all groups */
	for (group = 0; s && *s && group < LayerN; group++) {
		while (*s && isspace((int) *s))
			s++;

		/* loop over all group members */
		for (member = 0; *s; s++) {
			/* ignore white spaces and get layernumber */
			while (*s && isspace((int) *s))
				s++;
			switch (*s) {
			case 'c':
			case 'C':
			case 't':
			case 'T':
				layer = LayerN + PCB_COMPONENT_SIDE;
				c_set = pcb_true;
				break;

			case 's':
			case 'S':
			case 'b':
			case 'B':
				layer = LayerN + PCB_SOLDER_SIDE;
				s_set = pcb_true;
				break;

			default:
				if (!isdigit((int) *s))
					goto error;
				layer = atoi(s) - 1;
				break;
			}
			if (member >= LayerN + 1)
				goto error;
			if (oldfmt) {
				/* the old format didn't always have the silks */
				if (layer > LayerN + MAX(PCB_SOLDER_SIDE, PCB_COMPONENT_SIDE)) {
					/* UGLY HACK: we assume oldfmt is 1 only when called from io_pcb .y */
					PCB->Data->LayerN++;
					LayerN++;
				}
				if (layer > LayerN + MAX(PCB_SOLDER_SIDE, PCB_COMPONENT_SIDE) + 2)
					goto error;
			}
			else {
				if (layer > LayerN + MAX(PCB_SOLDER_SIDE, PCB_COMPONENT_SIDE))
					goto error;
			}
			groupnum[layer] = group;
			LayerGroup->grp[group].lid[member++] = layer;
			while (*++s && isdigit((int) *s));

			/* ignore white spaces and check for separator */
			while (*s && isspace((int) *s))
				s++;
			if (!*s || *s == ':')
				break;
			if (*s != ',')
				goto error;
		}
		LayerGroup->grp[group].len = member;
		if (*s == ':')
			s++;
	}
	if (!s_set)
		LayerGroup->grp[PCB_SOLDER_SIDE].lid[LayerGroup->grp[PCB_SOLDER_SIDE].len++] = LayerN + PCB_SOLDER_SIDE;
	if (!c_set)
		LayerGroup->grp[PCB_COMPONENT_SIDE].lid[LayerGroup->grp[PCB_COMPONENT_SIDE].len++] = LayerN + PCB_COMPONENT_SIDE;

	for (layer = 0; layer < LayerN && group < LayerN; layer++)
		if (groupnum[layer] == -1) {
			LayerGroup->grp[group].lid[0] = layer;
			LayerGroup->grp[group].len = 1;
			group++;
		}
	return (0);

	/* reset structure on error */
error:
	memset(LayerGroup, 0, sizeof(pcb_layer_stack_t));
	return (1);
}

int pcb_layer_gui_set_glayer(pcb_layergrp_id_t grp, int is_empty)
{
	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

	if (pcb_gui->set_layer_group != NULL)
		return pcb_gui->set_layer_group(grp, PCB->LayerGroups.grp[grp].lid[0], pcb_layergrp_flags(grp), is_empty);

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
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

int pcb_layer_group_list(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len)
{
	int group, layeri, used = 0;
	for (group = 0; group < pcb_max_group; group++) {
		for (layeri = 0; layeri < PCB->LayerGroups.grp[group].len; layeri++) {
			pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[layeri];
			if ((pcb_layer_flags(layer) & mask) == mask) {
/*				printf(" lf: %x %x & %x\n", layer, pcb_layer_flags(layer), mask);*/
				APPEND(group);
				goto added; /* do not add a group twice */
			}
		}
		added:;
	}
	return used;
}

int pcb_layer_group_list_any(pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len)
{
	int group, layeri, used = 0;
	for (group = 0; group < pcb_max_group; group++) {
		for (layeri = 0; layeri < PCB->LayerGroups.grp[group].len; layeri++) {
			pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[layeri];
			if ((pcb_layer_flags(layer) & mask)) {
				APPEND(group);
				goto added; /* do not add a group twice */
			}
		}
		added:;
	}
	return used;
}

pcb_layergrp_id_t pcb_layer_lookup_group(pcb_layer_id_t layer_id)
{
	int group, layeri;
	for (group = 0; group < pcb_max_group; group++) {
		for (layeri = 0; layeri < PCB->LayerGroups.grp[group].len; layeri++) {
			pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[layeri];
			if (layer == layer_id)
				return group;
		}
	}
	return -1;
}

void pcb_layer_add_in_group(pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id)
{
	int glen = PCB->LayerGroups.grp[group_id].len;
	PCB->LayerGroups.grp[group_id].lid[glen] = layer_id;
	PCB->LayerGroups.grp[group_id].len++;
}

/*
old [0]
  [0] component
  [2] comp-GND
  [3] comp-power
  [9] silk
old [1]
  [1] solder
  [4] sold-GND
  [5] sold-power
  [8] silk
old [2]
  [6] signal3
old [3]
  [7] outline
*/


#define NEWG(g, flags, gname) \
do { \
	g = &(newg.grp[newg.len]); \
	g->valid = 1; \
	if (gname != NULL) \
		g->name = pcb_strdup(gname); \
	else \
		g->name = NULL; \
	g->type = flags; \
	newg.len++; \
} while(0)

#define APPEND_ALL(_grp_, src_grp_id, flags) \
do { \
	int __n__; \
	pcb_layer_group_t *__s__ = &pcb->LayerGroups.grp[src_grp_id]; \
	for(__n__ = 0; __n__ < (__s__)->len; __n__++) { \
		if (pcb_layer_flags((__s__)->lid[__n__]) & flags) { \
			_grp_->lid[_grp_->len] = __s__->lid[__n__]; \
			_grp_->len++; \
		} \
	} \
} while(0)

void pcb_layer_group_from_old(pcb_board_t *pcb)
{
	pcb_layer_stack_t newg;
	pcb_layer_group_t *g;
	int n;

	memset(&newg, 0, sizeof(newg));

	NEWG(g, PCB_LYT_TOP | PCB_LYT_PASTE, "top paste");
	NEWG(g, PCB_LYT_TOP | PCB_LYT_SILK, "top silk");      APPEND_ALL(g, 0, PCB_LYT_SILK);
	NEWG(g, PCB_LYT_TOP | PCB_LYT_MASK, "top mask");
	NEWG(g, PCB_LYT_TOP | PCB_LYT_COPPER, "top copper");  APPEND_ALL(g, 0, PCB_LYT_COPPER);
	NEWG(g, PCB_LYT_INTERN | PCB_LYT_SUBSTRATE, NULL);


	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_COPPER, "bottom copper");  APPEND_ALL(g, 1, PCB_LYT_COPPER);
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_MASK, "bottom mask");
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_SILK, "bottom silk");      APPEND_ALL(g, 1, PCB_LYT_SILK);
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_PASTE, "bottom paste");


	memcpy(&pcb->LayerGroups, &newg, sizeof(newg));

}

void pcb_layer_group_to_old(pcb_board_t *pcb)
{

}

