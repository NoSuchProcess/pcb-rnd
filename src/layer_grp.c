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
#include "event.h"

/* notify the rest of the code after layer group changes so that the GUI
   and other parts sync up */
static int inhibit_notify = 0;
#define NOTIFY() \
do { \
	if (!inhibit_notify) { \
		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL); \
		if (pcb_gui != NULL) \
			pcb_gui->invalidate_all(); \
	} \
} while(0)

void pcb_layergrp_inhibit_inc(void)
{
	inhibit_notify++;
}

void pcb_layergrp_inhibit_dec(void)
{
	inhibit_notify--;
}

pcb_layergrp_id_t pcb_layer_get_group_(pcb_layer_t *Layer)
{
	return Layer->grp;
}

pcb_layer_group_t *pcb_get_layergrp(pcb_board_t *pcb, pcb_layergrp_id_t gid)
{
	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return NULL;
	return pcb->LayerGroups.grp + gid;
}


pcb_layergrp_id_t pcb_layer_get_group(pcb_board_t *pcb, pcb_layer_id_t lid)
{
	if ((lid < 0) || (lid >= pcb->Data->LayerN))
		return -1;

	return pcb_layer_get_group_(&pcb->Data->Layer[lid]);
}

int pcb_layergrp_del_layer(pcb_board_t *pcb, pcb_layergrp_id_t gid, pcb_layer_id_t lid)
{
	int n;
	pcb_layer_group_t *grp;
	pcb_layer_t *layer;

	if ((lid < 0) || (lid >= pcb->Data->LayerN))
		return -1;

	layer = pcb->Data->Layer + lid;
	if (gid < 0)
		gid = layer->grp;
	if (gid >= pcb->LayerGroups.len)
		return -1;

	grp = &pcb->LayerGroups.grp[gid];

	if (layer->grp != gid)
		return -1;

	for(n = 0; n < grp->len; n++) {
		if (grp->lid[n] == lid) {
			int remain = grp->len - n - 1;
			if (remain > 0)
				memmove(&grp->lid[n], &grp->lid[n+1], remain * sizeof(pcb_layer_id_t));
			grp->len--;
			layer->grp = -1;
			NOTIFY();
			return 0;
		}
	}


	/* also: broken layer group list */
	return -1;
}

pcb_layergrp_id_t pcb_layer_move_to_group(pcb_board_t *pcb, pcb_layer_id_t lid, pcb_layergrp_id_t gid)
{
	if (pcb_layergrp_del_layer(pcb, -1, lid) != 0)
		return -1;
	pcb_layer_add_in_group(pcb, lid, gid);
	NOTIFY();
	return gid;
}

unsigned int pcb_layergrp_flags(pcb_board_t *pcb, pcb_layergrp_id_t gid)
{

	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return 0;

	return pcb->LayerGroups.grp[gid].type;
}

const char *pcb_layergrp_name(pcb_board_t *pcb, pcb_layergrp_id_t gid)
{

	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return 0;

	return pcb->LayerGroups.grp[gid].name;
}

pcb_bool pcb_layergrp_is_empty(pcb_board_t *pcb, pcb_layergrp_id_t num)
{
	int i;
	pcb_layer_group_t *g = &pcb->LayerGroups.grp[num];

	/* some layers are never empty */
	if (g->type & PCB_LYT_MASK)
		return pcb_false;

	if (g->type & PCB_LYT_PASTE) {
		if (g->type & PCB_LYT_TOP)
			return pcb_layer_is_paste_empty(pcb, PCB_COMPONENT_SIDE);
		if (g->type & PCB_LYT_BOTTOM)
			return pcb_layer_is_paste_empty(pcb, PCB_SOLDER_SIDE);
	}

	for (i = 0; i < g->len; i++)
		if (!pcb_layer_is_empty(pcb, g->lid[i]))
			return pcb_false;
	return pcb_true;
}

int pcb_layergrp_free(pcb_board_t *pcb, pcb_layergrp_id_t id)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	if ((id >= 0) && (id < stack->len)) {
		int n;
		pcb_layer_group_t *g = stack->grp + id;
		if (g->name != NULL)
			free(g->name);
		for(n = 0; n < g->len; n++) {
			pcb_layer_t *layer = pcb->Data->Layer + g->lid[n];
			layer->grp = -1;
		}
		memset(g, 0, sizeof(pcb_layer_group_t));
		return 0;
	}
	return -1;
}

int pcb_layergrp_move_onto(pcb_board_t *pcb, pcb_layergrp_id_t dst, pcb_layergrp_id_t src)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	pcb_layer_group_t *d, *s;
	int n;

	if ((src < 0) || (src >= stack->len))
		return -1;
	if ((dst < stack->len) && (pcb_layergrp_free(pcb, dst) != 0))
		return -1;
	d = stack->grp + dst;
	s = stack->grp + src;
	memcpy(d, s, sizeof(pcb_layer_group_t));

	/* update layer's group refs to the new grp */
	for(n = 0; n < d->len; n++) {
		pcb_layer_t *layer = pcb->Data->Layer + d->lid[n];
		layer->grp = dst;
	}

	memset(s, 0, sizeof(pcb_layer_group_t));
	NOTIFY();
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

pcb_layer_group_t *pcb_get_grp(pcb_layer_stack_t *stack, pcb_layer_type_t loc, pcb_layer_type_t typ)
{
	int n;
	for(n = 0; n < stack->len; n++)
		if ((stack->grp[n].type & loc) && (stack->grp[n].type & typ))
			return &(stack->grp[n]);
	return NULL;
}

pcb_layer_group_t *pcb_layergrp_insert_after(pcb_board_t *pcb, pcb_layergrp_id_t where)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	int n;
	if ((where <= 0) || (where >= stack->len))
		return NULL;

	for(n = stack->len-1; n > where; n--)
		pcb_layergrp_move_onto(pcb, n+1, n);

	stack->len++;
	NOTIFY();
	return stack->grp+where+1;
}

static pcb_layer_group_t *pcb_get_grp_new_intern_(pcb_board_t *pcb, int omit_substrate)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	int bl, n;

	if (stack->len+2 >= PCB_MAX_LAYERGRP)
		return NULL;

	/* seek the bottom copper layer */
	for(bl = stack->len; bl >= 0; bl--) {
		if ((stack->grp[bl].type & PCB_LYT_BOTTOM) && (stack->grp[bl].type & PCB_LYT_COPPER)) {

			/* insert a new internal layer: move existing layers to make room */
			for(n = stack->len-1; n >= bl; n--)
				pcb_layergrp_move_onto(pcb, n+2, n);

			stack->len += 2;

			stack->grp[bl].name = pcb_strdup("Intern");
			stack->grp[bl].type = PCB_LYT_INTERN | PCB_LYT_COPPER;
			stack->grp[bl].valid = 1;
			bl++;
			if (!omit_substrate) {
				stack->grp[bl].type = PCB_LYT_INTERN | PCB_LYT_SUBSTRATE;
				stack->grp[bl].valid = 1;
			}
			return &stack->grp[bl-1];
		}
	}
	return NULL;
}

pcb_layer_group_t *pcb_get_grp_new_intern(pcb_board_t *pcb, int intern_id)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	pcb_layer_group_t *g;

	if (intern_id > 0) { /* look for existing intern layer first */
		int n;
		for(n = 0; n < stack->len; n++)
			if (stack->grp[n].intern_id == intern_id)
				return &stack->grp[n];
	}

	inhibit_notify++;
	g = pcb_get_grp_new_intern_(pcb, 0);
	inhibit_notify--;
	if (g != NULL) {
		g->intern_id = intern_id;
		NOTIFY();
	}
	return g;
}

pcb_layer_group_t *pcb_get_grp_new_misc(pcb_board_t *pcb)
{
	pcb_layer_group_t *g;
	inhibit_notify++;
	g = pcb_get_grp_new_intern_(pcb, 1);
	inhibit_notify--;
	NOTIFY();
	return g;
}

/* Move an inclusive block of groups [from..to] by delta on the stack, assuming
   target is already cleared and the new hole will be cleared by the caller */
static void move_grps(pcb_board_t *pcb, pcb_layer_stack_t *stk, pcb_layergrp_id_t from, pcb_layergrp_id_t to, int delta)
{
	int g, remaining, n;

	for(g = from; g <= to; g++) {
		for(n = 0; n < stk->grp[g].len; n++) {
			pcb_layer_id_t lid =stk->grp[g].lid[n];
			if ((lid >= 0) && (lid < pcb->Data->LayerN)) {
				pcb_layer_t *l = &pcb->Data->Layer[lid];
				if (l->grp > 0)
					l->grp += delta;
			}
		}
	}

	remaining = to - from+1;
	if (remaining > 0)
		memmove(&stk->grp[from + delta], &stk->grp[from], sizeof(pcb_layer_group_t) * remaining);
}

int pcb_layergrp_del(pcb_board_t *pcb, pcb_layergrp_id_t gid, int del_layers)
{
	int n;
	pcb_layer_stack_t *stk = &pcb->LayerGroups;

	if ((gid < 0) || (gid >= stk->len))
		return -1;

	for(n = 0; n < stk->grp[gid].len; n++) {
		pcb_layer_t *l = pcb_get_layer(stk->grp[gid].lid[n]);
		if (l != NULL) {
			if (del_layers) {
				pcb_layer_move(stk->grp[gid].lid[n], -1);
				n = -1; /* restart counting because the layer remove code may have changed the order */
			}
			else {
				/* detach only */
				l->grp = -1;
			}
		}
	}

	pcb_layergrp_free(pcb, gid);
	move_grps(pcb, stk, gid+1, stk->len-1, -1);
	stk->len--;
	NOTIFY();
	return 0;
}

int pcb_layergrp_move(pcb_board_t *pcb, pcb_layergrp_id_t from, pcb_layergrp_id_t to_before)
{
	pcb_layer_stack_t *stk = &pcb->LayerGroups;
	pcb_layer_group_t tmp;
	int n;

	if ((from < 0) || (from >= stk->len))
		return -1;

	if ((to_before < 0) || (to_before >= stk->len))
		return -1;

	if ((to_before == from + 1) || (to_before == from))
		return 0;

	memcpy(&tmp, &stk->grp[from], sizeof(pcb_layer_group_t));
	memset(&stk->grp[from], 0, sizeof(pcb_layer_group_t));
	if (to_before < from + 1) {
		move_grps(pcb, stk, to_before, from-1, +1);
		memcpy(&stk->grp[to_before], &tmp, sizeof(pcb_layer_group_t));
	}
	else {
		move_grps(pcb, stk, from+1, to_before-1, -1);
		memcpy(&stk->grp[to_before-1], &tmp, sizeof(pcb_layer_group_t));
	}

	/* fix up the group id for the layers of the group moved */
	for(n = 0; n < stk->grp[to_before].len; n++) {
#warning TODO: use pcb_get_layer when it becomes pcb-safe
/*		pcb_layer_t *l = pcb_get_layer(stk->grp[to_before].lid[n]);*/
		pcb_layer_t *l = &pcb->Data->Layer[stk->grp[to_before].lid[n]];
		if ((l != NULL) && (l->grp > 0))
			l->grp = to_before;
	}

	NOTIFY();
	return 0;
}


	/* ugly hack: remove the extra substrate we added after the outline layer */
void pcb_layergrp_fix_old_outline(pcb_board_t *pcb)
{
	pcb_layer_stack_t *LayerGroup = &pcb->LayerGroups;
	pcb_layer_group_t *g = pcb_get_grp(LayerGroup, PCB_LYT_ANYWHERE, PCB_LYT_OUTLINE);
	if ((g != NULL) && (g[1].type & PCB_LYT_SUBSTRATE)) {
		pcb_layergrp_id_t gid = g - LayerGroup->grp + 1;
		pcb_layergrp_del(pcb, gid, 0);
	}
}

void pcb_layergrp_fix_turn_to_outline(pcb_layer_group_t *g)
{
	g->type |= PCB_LYT_OUTLINE;
	g->type &= ~PCB_LYT_COPPER;
	free(g->name);
	g->name = pcb_strdup("global outline");
}


#define LAYER_IS_OUTLINE(idx) ((pcb->Data->Layer[idx].Name != NULL) && ((strcmp(pcb->Data->Layer[idx].Name, "route") == 0 || strcmp(pcb->Data->Layer[(idx)].Name, "outline") == 0)))
int pcb_layer_parse_group_string(pcb_board_t *pcb, const char *grp_str, int LayerN, int oldfmt)
{
	const char *s, *start;
	pcb_layer_id_t lids[PCB_MAX_LAYER];
	int lids_len = 0;
	pcb_layer_type_t loc = PCB_LYT_INTERN;
	pcb_layer_group_t *g;
	int n;
	pcb_layer_stack_t *LayerGroup = &pcb->LayerGroups;

	inhibit_notify++;

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
					g = pcb_get_grp_new_intern(pcb, -1);
				else
					g = pcb_get_grp(LayerGroup, loc, PCB_LYT_COPPER);

				for(n = 0; n < lids_len; n++) {
					if (lids[n] < 0)
						continue;
					if (LAYER_IS_OUTLINE(lids[n])) {
						if (g->type & PCB_LYT_INTERN)
							pcb_layergrp_fix_turn_to_outline(g);
						else
							pcb_message(PCB_MSG_ERROR, "outline layer can not be on the solder or component side - converting it into a copper layer\n");
					}
					pcb_layer_add_in_group_(pcb, g, g - LayerGroup->grp, lids[n]);
				}

				lids_len = 0;

				/* prepare for next iteration */
				loc = PCB_LYT_INTERN;
				start = s+1;
				break;
		}
		if (*s == '\0')
			break;
	}

	pcb_layergrp_fix_old_outline(pcb);

	/* set the two silks */
	g = pcb_get_grp(LayerGroup, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - LayerGroup->grp, LayerN-2);

	g = pcb_get_grp(LayerGroup, PCB_LYT_TOP, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - LayerGroup->grp, LayerN-1);

	inhibit_notify--;
	return 0;

	/* reset structure on error */
error:
	inhibit_notify--;
	memset(LayerGroup, 0, sizeof(pcb_layer_stack_t));
	return 1;
}

int pcb_layer_gui_set_glayer(pcb_layergrp_id_t grp, int is_empty)
{
	/* if there's no GUI, that means no draw should be done */
	if (pcb_gui == NULL)
		return 0;

	if (pcb_gui->set_layer_group != NULL)
		return pcb_gui->set_layer_group(grp, PCB->LayerGroups.grp[grp].lid[0], pcb_layergrp_flags(PCB, grp), is_empty);

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

int pcb_layergrp_list(pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len)
{
	int group, used = 0;
	for (group = 0; group < pcb->LayerGroups.len; group++) {
		if ((pcb_layergrp_flags(pcb, group) & mask) == mask)
				APPEND(group);
	}
	return used;
}

int pcb_layer_group_list_any(pcb_board_t *pcb, pcb_layer_type_t mask, pcb_layergrp_id_t *res, int res_len)
{
	int group, used = 0;
	for (group = 0; group < pcb->LayerGroups.len; group++) {
		if ((pcb_layergrp_flags(pcb, group) & mask))
				APPEND(group);
	}
	return used;
}

int pcb_layer_add_in_group_(pcb_board_t *pcb, pcb_layer_group_t *grp, pcb_layergrp_id_t group_id, pcb_layer_id_t layer_id)
{
	if ((layer_id < 0) || (layer_id >= pcb->Data->LayerN))
		return -1;

	grp->lid[grp->len] = layer_id;
	grp->len++;
	pcb->Data->Layer[layer_id].grp = group_id;

	return 0;
}

int pcb_layer_add_in_group(pcb_board_t *pcb, pcb_layer_id_t layer_id, pcb_layergrp_id_t group_id)
{
	if ((group_id < 0) || (group_id >= pcb->LayerGroups.len))
		return -1;

	return pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[group_id], group_id, layer_id);
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

void pcb_layer_group_setup_default(pcb_layer_stack_t *newg)
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

/*	NEWG(g, PCB_LYT_INTERN | PCB_LYT_OUTLINE, "outline");*/
}



int pcb_layergrp_rename_(pcb_layer_group_t *grp, char *name)
{
	free(grp->name);
	grp->name = name;
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return 0;
}

int pcb_layergrp_rename(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *name)
{
	pcb_layer_group_t *grp = pcb_get_layergrp(pcb, gid);
	if (grp == NULL) return -1;
	return pcb_layergrp_rename_(grp, pcb_strdup(name));
}


static pcb_layergrp_id_t pcb_layergrp_get_cached(pcb_board_t *pcb, pcb_layer_id_t *cache, unsigned int loc, unsigned int typ)
{
	pcb_layer_group_t *g;

	/* check if the last known value is still accurate */
	if ((*cache >= 0) && (*cache < pcb->LayerGroups.len)) {
		g = &(pcb->LayerGroups.grp[*cache]);
		if ((g->type & loc) && (g->type & typ))
			return *cache;
	}

	/* nope: need to resolve it again */
	g = pcb_get_grp(&pcb->LayerGroups, loc, typ);
	if (g == NULL)
		*cache = -1;
	else
		*cache = (g - pcb->LayerGroups.grp);
	return *cache;
}

pcb_layergrp_id_t pcb_layergrp_get_bottom_mask()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_MASK);
}

pcb_layergrp_id_t pcb_layergrp_get_top_mask()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_MASK);
}

pcb_layergrp_id_t pcb_layergrp_get_bottom_paste()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_PASTE);
}

pcb_layergrp_id_t pcb_layergrp_get_top_paste()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_PASTE);
}

pcb_layergrp_id_t pcb_layergrp_get_bottom_silk()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_SILK);
}

pcb_layergrp_id_t pcb_layergrp_get_top_silk()
{
	static pcb_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_SILK);
}

/* Note: these function is in this file mainly to access the static inlines */
int pcb_silk_on(pcb_board_t *pcb)
{
	static pcb_layer_id_t ts = -1, bs = -1;
	pcb_layergrp_get_cached(pcb, &ts, PCB_LYT_TOP, PCB_LYT_SILK);
	if ((ts >= 0) && (pcb->LayerGroups.grp[ts].vis))
		return 1;
	pcb_layergrp_get_cached(pcb, &bs, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	if ((bs >= 0) && (pcb->LayerGroups.grp[bs].vis))
		return 1;
	return 0;
}

int pcb_mask_on(pcb_board_t *pcb)
{
	static pcb_layer_id_t tm = -1, bm = -1;
	pcb_layergrp_get_cached(pcb, &tm, PCB_LYT_TOP, PCB_LYT_MASK);
	if ((tm >= 0) && (pcb->LayerGroups.grp[tm].vis))
		return 1;
	pcb_layergrp_get_cached(pcb, &bm, PCB_LYT_BOTTOM, PCB_LYT_MASK);
	if ((bm >= 0) && (pcb->LayerGroups.grp[bm].vis))
		return 1;
	return 0;
}


int pcb_paste_on(pcb_board_t *pcb)
{
	static pcb_layer_id_t tp = -1, bp = -1;
	pcb_layergrp_get_cached(pcb, &tp, PCB_LYT_TOP, PCB_LYT_PASTE);
	if ((tp >= 0) && (pcb->LayerGroups.grp[tp].vis))
		return 1;
	pcb_layergrp_get_cached(pcb, &bp, PCB_LYT_BOTTOM, PCB_LYT_PASTE);
	if ((bp >= 0) && (pcb->LayerGroups.grp[bp].vis))
		return 1;
	return 0;
}

