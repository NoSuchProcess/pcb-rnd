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
#warning layer TODO: rewrite this
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
#warning layer TODO: rewrite this; also do layer->grp cross ref
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

int pcb_layergrp_free(pcb_layer_stack_t *stack, pcb_layergrp_id_t id)
{
	if ((id >= 0) && (id < stack->len)) {
		pcb_layer_group_t *g = stack->grp + id;
		if (g->name != NULL)
			free(g->name);
#warning unlink layers from the group
		memset(g, 0, sizeof(pcb_layer_group_t));
		return 0;
	}
	return -1;
}

int pcb_layergrp_move(pcb_layer_stack_t *stack, pcb_layergrp_id_t dst, pcb_layergrp_id_t src)
{
	pcb_layer_group_t *d, *s;

	if ((src < 0) || (src >= stack->len))
		return -1;
	if (pcb_layergrp_free(stack, dst) != 0)
		return -1;
	d = stack->grp + dst;
	s = stack->grp + src;
	memcpy(d, s, sizeof(pcb_layer_group_t));
#warning relink layers in d
	memset(s, 0, sizeof(pcb_layer_group_t));
	return 0;
}

static int flush_item(const char *s, const char *start, pcb_layer_id_t *lids, int *lids_len, pcb_layer_type_t *loc)
{
	char *end;
	pcb_layer_id_t lid;
	switch (*start) {
		case 'c': case 'C': case 't': case 'T': *loc = PCB_LYT_TOP; break;
		case 's': case 'S': case 'b': case 'B': *loc = PCB_LYT_BOTTOM; break;
		default:
			lid = strtol(start, &end, 10)-1;
			if (end != s)
				return -1;
			if ((*lids_len) >= PCB_MAX_LAYER)
				return -1;
			lids[*lids_len] = lid;
			(*lids_len)++;
	}
	return 0;
}

static pcb_layer_group_t *get_grp(pcb_layer_stack_t *stack, pcb_layer_type_t loc, pcb_layer_type_t typ)
{
	int n;
	for(n = 0; n < stack->len; n++)
		if ((stack->grp[n].type & loc) && (stack->grp[n].type & typ))
			return &(stack->grp[n]);
	return NULL;
}

static pcb_layer_group_t *get_grp_new_intern(pcb_layer_stack_t *stack)
{
	int bl, n;

	if (stack->len+2 >= PCB_MAX_LAYERGRP)
		return NULL;

	/* seek the bottom copper layer */
	for(bl = stack->len; bl >= 0; bl--) {
		if ((stack->grp[bl].type & PCB_LYT_BOTTOM) && (stack->grp[bl].type & PCB_LYT_COPPER)) {

			/* insert a new internal layer: move existing layers to make room */
			for(n = stack->len-1; n >= bl; n--)
				pcb_layergrp_move(stack, n+2, n);

			stack->len += 2;

			stack->grp[bl].name = pcb_strdup("Intern");
			stack->grp[bl].type = PCB_LYT_INTERN | PCB_LYT_COPPER;
			stack->grp[bl].valid = 1;
			bl++;
			stack->grp[bl].type = PCB_LYT_INTERN | PCB_LYT_SUBSTRATE;
			stack->grp[bl].valid = 1;
			return &stack->grp[bl-1];
		}
	}
	return NULL;
}

static void pcb_layer_group_setup_default(pcb_layer_stack_t *newg);
int pcb_layer_parse_group_string(const char *grp_str, pcb_layer_stack_t *LayerGroup, int LayerN, int oldfmt)
{
	const char *s, *start;
	pcb_layer_id_t lids[PCB_MAX_LAYER];
	int lids_len = 0;
	pcb_layer_type_t loc = PCB_LYT_INTERN;
	pcb_layer_group_t *g;
	int n;

	/* clear struct */
	pcb_layer_group_setup_default(LayerGroup);

	for(start = s = grp_str; ; s++) {
		switch(*s) {
			case ',':
				if (flush_item(s, start, lids, &lids_len, &loc) != 0)
					goto error;
				start = s+1;
				break;
			case '\0':
			case ':':
				if (flush_item(s, start, lids, &lids_len, &loc) != 0)
					goto error;
				/* finalize group */
				if (loc & PCB_LYT_INTERN)
					g = get_grp_new_intern(LayerGroup);
				else
					g = get_grp(LayerGroup, loc, PCB_LYT_COPPER);

				for(n = 0; n < lids_len; n++)
					g->lid[n] = lids[n];
				g->len = lids_len;
				lids_len = 0;

				/* prepare for next iteration */
				loc = PCB_LYT_INTERN;
				start = s+1;
				break;
		}
		if (*s == '\0')
			break;
	}

	/* set the two silks */
	g = get_grp(LayerGroup, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
	g->len = 1;
	g->lid[0] = LayerN;

	g = get_grp(LayerGroup, PCB_LYT_TOP, PCB_LYT_COPPER);
	g->len = 1;
	g->lid[0] = LayerN+1;


	return 0;

	/* reset structure on error */
error:
	memset(LayerGroup, 0, sizeof(pcb_layer_stack_t));
	return 1;
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
#warning layer TODO: cross ref layer-to-group
}

#define NEWG(g, flags, gname) \
do { \
	g = &(newg->grp[newg->len]); \
	g->valid = 1; \
	if (gname != NULL) \
		g->name = pcb_strdup(gname); \
	else \
		g->name = NULL; \
	g->type = flags; \
	newg->len++; \
} while(0)

static void pcb_layer_group_setup_default(pcb_layer_stack_t *newg)
{
	pcb_layer_group_t *g;

	memset(newg, 0, sizeof(pcb_layer_stack_t));

	NEWG(g, PCB_LYT_TOP | PCB_LYT_PASTE, "top paste");
	NEWG(g, PCB_LYT_TOP | PCB_LYT_SILK, "top silk");
	NEWG(g, PCB_LYT_TOP | PCB_LYT_MASK, "top mask");
	NEWG(g, PCB_LYT_TOP | PCB_LYT_COPPER, "top copper");
	NEWG(g, PCB_LYT_INTERN | PCB_LYT_SUBSTRATE, NULL);

	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_COPPER, "bottom copper");
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_MASK, "bottom mask");
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_SILK, "bottom silk");
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_PASTE, "bottom paste");

	NEWG(g, PCB_LYT_INTERN | PCB_LYT_OUTLINE, "outline");
}

