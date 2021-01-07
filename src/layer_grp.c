/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016,2019,2020 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
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
#include "layer_grp.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/rnd_printf.h>
#include "event.h"
#include <librnd/core/funchash.h>
#include "funchash_core.h"
#include "undo.h"

static const char core_layergrp_cookie[] = "core-layergrp";

/* notify the rest of the code after layer group changes so that the GUI
   and other parts sync up */
static int inhibit_notify = 0;
#define NOTIFY(pcb) \
do { \
	pcb->LayerGroups.cache.copper_valid = 0; \
	if (!inhibit_notify) { \
		rnd_event(&pcb->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL); \
		if ((rnd_gui != NULL) && (rnd_exporter == NULL)) \
			rnd_gui->invalidate_all(rnd_gui); \
		pcb_board_set_changed_flag(pcb, rnd_true); \
	} \
} while(0)

void pcb_layergrp_notify_chg(pcb_board_t *pcb)
{
	NOTIFY(pcb);
}

void pcb_layergrp_inhibit_inc(void)
{
	inhibit_notify++;
}

void pcb_layergrp_inhibit_dec(void)
{
	inhibit_notify--;
}

int pcb_layergrp_is_inhibited(void)
{
	return inhibit_notify > 0;
}

void pcb_layergrp_notify(pcb_board_t *pcb)
{
	NOTIFY(pcb);
}

rnd_layergrp_id_t pcb_layergrp_id(const pcb_board_t *pcb, const pcb_layergrp_t *grp)
{
	if ((grp >= &pcb->LayerGroups.grp[0]) && (grp < &pcb->LayerGroups.grp[pcb->LayerGroups.len]))
		return grp - &pcb->LayerGroups.grp[0];
	return -1;
}

rnd_layergrp_id_t pcb_layer_get_group_(pcb_layer_t *Layer)
{
	if (!Layer->is_bound)
		return Layer->meta.real.grp;

	if (Layer->meta.bound.real == NULL) /* bound layer without a binding */
		return -1;

	return Layer->meta.bound.real->meta.real.grp;
}

pcb_layergrp_t *pcb_get_layergrp(pcb_board_t *pcb, rnd_layergrp_id_t gid)
{
	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return NULL;
	return pcb->LayerGroups.grp + gid;
}


rnd_layergrp_id_t pcb_layer_get_group(pcb_board_t *pcb, rnd_layer_id_t lid)
{
	if ((lid < 0) || (lid >= pcb->Data->LayerN))
		return -1;

	return pcb_layer_get_group_(&pcb->Data->Layer[lid]);
}

int pcb_layergrp_del_layer(pcb_board_t *pcb, rnd_layergrp_id_t gid, rnd_layer_id_t lid)
{
	int n;
	pcb_layergrp_t *grp;
	pcb_layer_t *layer;

	if ((lid < 0) || (lid >= pcb->Data->LayerN))
		return -1;

	layer = pcb->Data->Layer + lid;
	if (gid < 0) {
		gid = layer->meta.real.grp;
		if (gid == -1)
			return 0; /* already deleted from groups */
	}
	if (gid >= pcb->LayerGroups.len)
		return -1;

	grp = &pcb->LayerGroups.grp[gid];

	if (layer->meta.real.grp != gid)
		return -1;

	for(n = 0; n < grp->len; n++) {
		if (grp->lid[n] == lid) {
			int remain = grp->len - n - 1;
			if (remain > 0)
				memmove(&grp->lid[n], &grp->lid[n+1], remain * sizeof(rnd_layer_id_t));
			grp->len--;
			layer->meta.real.grp = -1;
			NOTIFY(pcb);
			return 0;
		}
	}


	/* also: broken layer group list */
	return -1;
}

static void make_substrate(pcb_board_t *pcb, pcb_layergrp_t *g)
{
	g->ltype = PCB_LYT_INTERN | PCB_LYT_SUBSTRATE;
	pcb_layergrp_setup(g, pcb);
}

rnd_layergrp_id_t pcb_layergrp_dup(pcb_board_t *pcb, rnd_layergrp_id_t gid, int auto_substrate, rnd_bool undoable)
{
	pcb_layergrp_t *ng, *og = pcb_get_layergrp(pcb, gid);
	rnd_layergrp_id_t after;

	if (og == NULL)
		return -1;

	inhibit_notify++;
	if (auto_substrate && (og->ltype & PCB_LYT_COPPER) && !(og->ltype & PCB_LYT_BOTTOM)) {
		ng = pcb_layergrp_insert_after(pcb, gid);
		make_substrate(pcb, ng);
		after = ng - pcb->LayerGroups.grp;
		if (undoable) pcb_layergrp_undoable_created(ng);
	}
	else
		after = gid;

	ng = pcb_layergrp_insert_after(pcb, after);
	if (og->name != NULL)
		ng->name = rnd_strdup(og->name);
	ng->ltype = og->ltype;
	if (og->purpose != NULL)
		ng->purpose = rnd_strdup(og->purpose);
	ng->purpi = og->purpi;
	ng->valid = ng->open = ng->vis = 1;
	pcb_layergrp_setup(ng, pcb);
	if (undoable) pcb_layergrp_undoable_created(ng);

	inhibit_notify--;
	NOTIFY(pcb);
	return ng - pcb->LayerGroups.grp;
}


rnd_layergrp_id_t pcb_layer_move_to_group(pcb_board_t *pcb, rnd_layer_id_t lid, rnd_layergrp_id_t gid)
{
	if (pcb_layergrp_del_layer(pcb, -1, lid) != 0)
		return -1;
	pcb_layer_add_in_group(pcb, lid, gid);
	rnd_event(&pcb->hidlib, PCB_EVENT_LAYER_CHANGED_GRP, "p", &pcb->Data->Layer[lid]);
	NOTIFY(pcb);
	return gid;
}

unsigned int pcb_layergrp_flags(const pcb_board_t *pcb, rnd_layergrp_id_t gid)
{

	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return 0;

	return pcb->LayerGroups.grp[gid].ltype;
}

const char *pcb_layergrp_name(pcb_board_t *pcb, rnd_layergrp_id_t gid)
{

	if ((gid < 0) || (gid >= pcb->LayerGroups.len))
		return 0;

	return pcb->LayerGroups.grp[gid].name;
}

rnd_bool pcb_layergrp_is_empty(pcb_board_t *pcb, rnd_layergrp_id_t num)
{
	int i;
	pcb_layergrp_t *g = &pcb->LayerGroups.grp[num];

	/* some layers are never empty */
	if (g->ltype & PCB_LYT_MASK)
		return rnd_false;

	if (!pcb_pstk_is_group_empty(pcb, num))
		return rnd_false;

	for (i = 0; i < g->len; i++)
		if (!pcb_layer_is_empty(pcb, g->lid[i]))
			return rnd_false;
	return rnd_true;
}

rnd_bool pcb_layergrp_is_pure_empty(pcb_board_t *pcb, rnd_layergrp_id_t num)
{
	int i;
	pcb_layergrp_t *g = &pcb->LayerGroups.grp[num];

	for (i = 0; i < g->len; i++)
		if (!pcb_layer_is_pure_empty(pcb_get_layer(pcb->Data, g->lid[i])))
			return rnd_false;
	return rnd_true;
}

static void pcb_layergrp_free_fields(pcb_layergrp_t *g)
{
	free(g->name);
	free(g->purpose);
	g->name = g->purpose = NULL;
	g->purpi = -1;
}


int pcb_layergrp_free(pcb_board_t *pcb, rnd_layergrp_id_t id)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	if ((id >= 0) && (id < stack->len)) {
		int n;
		pcb_layergrp_t *g = stack->grp + id;
		for(n = 0; n < g->len; n++) {
			pcb_layer_t *layer = pcb->Data->Layer + g->lid[n];
			layer->meta.real.grp = -1;
		}
		pcb_layergrp_free_fields(g);
		memset(g, 0, sizeof(pcb_layergrp_t));
		return 0;
	}
	return -1;
}

int pcb_layergrp_move_onto(pcb_board_t *pcb, rnd_layergrp_id_t dst, rnd_layergrp_id_t src)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	pcb_layergrp_t *d, *s;
	int n;

	if ((src < 0) || (src >= stack->len))
		return -1;
	if ((dst < stack->len) && (pcb_layergrp_free(pcb, dst) != 0))
		return -1;
	d = stack->grp + dst;
	s = stack->grp + src;
	memcpy(d, s, sizeof(pcb_layergrp_t));

	/* update layer's group refs to the new grp */
	for(n = 0; n < d->len; n++) {
		pcb_layer_t *layer = pcb->Data->Layer + d->lid[n];
		layer->meta.real.grp = dst;
	}

	memset(s, 0, sizeof(pcb_layergrp_t));
	NOTIFY(pcb);
	return 0;
}

pcb_layergrp_t *pcb_get_grp(pcb_layer_stack_t *stack, pcb_layer_type_t loc, pcb_layer_type_t typ)
{
	int n;
	for(n = 0; n < stack->len; n++)
		if ((stack->grp[n].ltype & loc) && (stack->grp[n].ltype & typ))
			return &(stack->grp[n]);
	return NULL;
}

pcb_layergrp_t *pcb_layergrp_insert_after(pcb_board_t *pcb, rnd_layergrp_id_t where)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	pcb_layergrp_t *g;
	int n;
	if ((where <= 0) || (where >= stack->len))
		return NULL;

	for(n = stack->len-1; n > where; n--)
		pcb_layergrp_move_onto(pcb, n+1, n);

	stack->len++;
	g = stack->grp+where+1;
	PCB_SET_PARENT(g, board, pcb);
	NOTIFY(pcb);
	return g;
}

static void layergrp_post_change(pcb_attribute_list_t *list, const char *name, const char *value)
{
	pcb_layergrp_t *g = (pcb_layergrp_t *)(((char *)list) - offsetof(pcb_layergrp_t, Attributes));

	if (strcmp(name, "init-invis") == 0) {
		int newv;
		if ((value == NULL) || (*value == '\0'))
			return;
		if ((rnd_strcasecmp(value, "true") == 0) || (rnd_strcasecmp(value, "yes") == 0) || (rnd_strcasecmp(value, "on") == 0) || (strcmp(value, "1") == 0)) newv = 1;
		else if ((rnd_strcasecmp(value, "false") == 0) || (rnd_strcasecmp(value, "no") == 0) || (rnd_strcasecmp(value, "off") == 0) || (strcmp(value, "0") == 0)) newv = 0;
		else {
			rnd_message(RND_MSG_ERROR, "unrecognized value '%s' of layer group %s's init-invis attribute", value, g->name == NULL ? "" : g->name);
			return;
		}
		g->init_invis = newv;
	}

}

void pcb_layergrp_setup(pcb_layergrp_t *g, pcb_board_t *parent)
{
	g->valid = 1;
	g->parent_type = PCB_PARENT_BOARD;
	g->parent.board = parent;
	g->type = PCB_OBJ_LAYERGRP;
	g->Attributes.post_change = layergrp_post_change;
}

static pcb_layergrp_t *pcb_get_grp_new_intern_insert(pcb_board_t *pcb, int room, int bl, int omit_substrate)
{
	int n;
	pcb_layer_stack_t *stack = &pcb->LayerGroups;

	if (bl < stack->len-1)
		for(n = stack->len-1; n >= bl; n--)
			pcb_layergrp_move_onto(pcb, n+room, n);

	stack->len += room;

	stack->grp[bl].name = rnd_strdup("Intern");
	stack->grp[bl].ltype = PCB_LYT_INTERN | PCB_LYT_COPPER;
	stack->grp[bl].len = 0;
	pcb_layergrp_setup(&stack->grp[bl], pcb);
	bl++;
	if (!omit_substrate)
		make_substrate(pcb, &stack->grp[bl]);
	return &stack->grp[bl-1];
}

static pcb_layergrp_t *pcb_get_grp_new_intern_(pcb_board_t *pcb, int omit_substrate, int force_end)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	int bl;
	int room = omit_substrate ? 1 : 2;

	if ((stack->len + room) >= PCB_MAX_LAYERGRP)
		return NULL;

	/* seek the bottom copper layer */
	if (!force_end)
	for(bl = stack->len; bl >= 0; bl--) {
		if ((stack->grp[bl].ltype & PCB_LYT_BOTTOM) && (stack->grp[bl].ltype & PCB_LYT_COPPER)) {
			pcb_get_grp_new_intern_insert(pcb, room, bl, omit_substrate);
			return &stack->grp[bl];
		}
	}

	/* bottom copper did not exist - insert at the end */
	bl = stack->len;
	pcb_get_grp_new_intern_insert(pcb, room, bl, omit_substrate);
	return &stack->grp[bl];
}

pcb_layergrp_t *pcb_get_grp_new_intern(pcb_board_t *pcb, int intern_id)
{
	pcb_layer_stack_t *stack = &pcb->LayerGroups;
	pcb_layergrp_t *g;

	if (intern_id > 0) { /* look for existing intern layer first */
		int n;
		for(n = 0; n < stack->len; n++)
			if (stack->grp[n].intern_id == intern_id)
				return &stack->grp[n];
	}

	inhibit_notify++;
	g = pcb_get_grp_new_intern_(pcb, 0, 0);
	inhibit_notify--;
	if (g != NULL) {
		g->intern_id = intern_id;
		NOTIFY(pcb);
	}
	return g;
}

pcb_layergrp_t *pcb_get_grp_new_misc(pcb_board_t *pcb)
{
	pcb_layergrp_t *g;
	inhibit_notify++;
	g = pcb_get_grp_new_intern_(pcb, 1, 0);
	inhibit_notify--;
	NOTIFY(pcb);
	return g;
}

pcb_layergrp_t *pcb_get_grp_new_raw(pcb_board_t *pcb, int prepend)
{
	pcb_layergrp_t *g;
	inhibit_notify++;
	g = pcb_get_grp_new_intern_insert(pcb, 1, prepend ? 0 : pcb->LayerGroups.len, 1);
	inhibit_notify--;
	NOTIFY(pcb);
	return g;
}


/* Move an inclusive block of groups [from..to] by delta on the stack, assuming
   target is already cleared and the new hole will be cleared by the caller */
static void move_grps(pcb_board_t *pcb, pcb_layer_stack_t *stk, rnd_layergrp_id_t from, rnd_layergrp_id_t to, int delta)
{
	int g, remaining, n;

	for(g = from; g <= to; g++) {
		for(n = 0; n < stk->grp[g].len; n++) {
			rnd_layer_id_t lid =stk->grp[g].lid[n];
			if ((lid >= 0) && (lid < pcb->Data->LayerN)) {
				pcb_layer_t *l = &pcb->Data->Layer[lid];
				if (l->meta.real.grp > 0)
					l->meta.real.grp += delta;
			}
		}
	}

	remaining = to - from+1;
	if (remaining > 0)
		memmove(&stk->grp[from + delta], &stk->grp[from], sizeof(pcb_layergrp_t) * remaining);
}

int pcb_layergrp_index_in_grp(pcb_layergrp_t *grp, rnd_layer_id_t lid)
{
	int idx;
	for(idx = 0; idx < grp->len; idx++)
		if (grp->lid[idx] == lid)
			return idx;
	return -1;
}

/*** undoable step within the group ***/

static int pcb_layergrp_step_layer_(pcb_board_t *pcb, pcb_layergrp_t *grp, rnd_layer_id_t lid, int delta)
{
	int idx, idx2;
	rnd_layer_id_t tmp;

	for(idx = 0; idx < grp->len; idx++) {
		if (grp->lid[idx] == lid) {
			if (delta > 0) {
				if (idx == grp->len - 1) /* already the last */
					return 0;
				idx2 = idx+1;
				goto swap;
			}
			else if (delta < 0) {
				if (idx == 0) /* already the last */
					return 0;
				idx2 = idx-1;
				goto swap;
			}
			else
				return -1;
		}
	}
	return -1;

	swap:;
	tmp = grp->lid[idx];
	grp->lid[idx] =grp->lid[idx2];
	grp->lid[idx2] = tmp;
	NOTIFY(pcb);
	return 0;
}

typedef struct {
	pcb_board_t *pcb;
	rnd_layergrp_id_t gid;
	rnd_layer_id_t lid;
	int delta;
} undo_layergrp_steply_t;

static int undo_layergrp_steply_swap(void *udata)
{
	undo_layergrp_steply_t *s = udata;
	int res;
	pcb_layergrp_t *grp = pcb_get_layergrp(s->pcb, s->gid);
	if (grp == NULL) return -1;
	res = pcb_layergrp_step_layer_(s->pcb, grp, s->lid, s->delta);
	if (res == 0)
		s->delta = -s->delta;
	return res;
}

static void undo_layergrp_steply_print(void *udata, char *dst, size_t dst_len)
{
	undo_layergrp_steply_t *s = udata;
	rnd_snprintf(dst, dst_len, "step layer in grp: #%ld/#%ld: %+d", s->gid, s->lid, s->delta);
}

static const uundo_oper_t undo_layergrp_steply = {
	core_layergrp_cookie,
	NULL,
	undo_layergrp_steply_swap,
	undo_layergrp_steply_swap,
	undo_layergrp_steply_print
};

int pcb_layergrp_step_layer(pcb_board_t *pcb, pcb_layergrp_t *grp, rnd_layer_id_t lid, int delta)
{
	undo_layergrp_steply_t *s = pcb_undo_alloc(pcb, &undo_layergrp_steply, sizeof(undo_layergrp_steply_t));
	int res;

	s->pcb = pcb;
	s->gid = grp - pcb->LayerGroups.grp;
	s->lid = lid;
	s->delta = delta;
	res = undo_layergrp_steply_swap(s);
	pcb_undo_inc_serial();
	return res;
}


static void grp_move_struct(pcb_layergrp_t *dst, pcb_layergrp_t *src)
{
	*dst = *src;
	memset(src, 0, sizeof(pcb_layergrp_t));
}

static void pcb_layergrp_del_1(pcb_board_t *pcb, rnd_layergrp_id_t gid, int del_layers, rnd_bool undoable)
{
	int n;
	pcb_layer_stack_t *stk = &pcb->LayerGroups;

	for(n = 0; n < stk->grp[gid].len; n++) {
		pcb_layer_t *l = pcb_get_layer(pcb->Data, stk->grp[gid].lid[n]);
		if (l != NULL) {
			if (del_layers) {
				pcb_layer_move(pcb, stk->grp[gid].lid[n], -1, -1, undoable);
				n = -1; /* restart counting because the layer remove code may have changed the order */
			}
			else {
				/* detach only */
				l->meta.real.grp = -1;
			}
		}
	}
}

static void pcb_layergrp_del_2(pcb_board_t *pcb, rnd_layergrp_id_t gid)
{
	pcb_layer_stack_t *stk = &pcb->LayerGroups;

	pcb_layergrp_free(pcb, gid);
	move_grps(pcb, stk, gid+1, stk->len-1, -1);
	stk->len--;
	NOTIFY(pcb);
}

static void pcb_layergrp_ins(pcb_board_t *pcb, rnd_layergrp_id_t gid, pcb_layergrp_t *src)
{
	pcb_layer_stack_t *stk = &pcb->LayerGroups;
	move_grps(pcb, stk, gid, stk->len-1, +1);
	stk->len++;
	grp_move_struct(&stk->grp[gid], src);
	NOTIFY(pcb);
}


typedef struct {
	pcb_board_t *pcb;
	rnd_layergrp_id_t gid;
	unsigned int del:1;
	pcb_layergrp_t save;
} undo_layergrp_del_t;

static int undo_layergrp_del_swap(void *udata)
{
	undo_layergrp_del_t *r = udata;
	pcb_layer_stack_t *stk = &r->pcb->LayerGroups;

	if (r->del) { /* undo a delete by inserting the group back at its place */
		pcb_layergrp_ins(r->pcb, r->gid, &r->save);
	}
	else { /* redo delete */
		pcb_layergrp_del_1(r->pcb, r->gid, 1, 0);
		grp_move_struct(&r->save, &stk->grp[r->gid]);
		pcb_layergrp_del_2(r->pcb, r->gid);
	}

	r->del = !r->del;
	return 0;
}

static void undo_layergrp_del_free(void *udata)
{
	undo_layergrp_del_t *r = udata;
	pcb_layergrp_free_fields(&r->save);
}


static void undo_layergrp_del_print(void *udata, char *dst, size_t dst_len)
{
	undo_layergrp_del_t *r = udata;
	rnd_snprintf(dst, dst_len, "layergrp %s: '%s'", r->del ? "delete" : "insert", r->del ? r->save.name : pcb_layergrp_name(r->pcb, r->gid));
}

static const uundo_oper_t undo_layergrp_del = {
	core_layergrp_cookie,
	undo_layergrp_del_free,
	undo_layergrp_del_swap,
	undo_layergrp_del_swap,
	undo_layergrp_del_print
};


int pcb_layergrp_del(pcb_board_t *pcb, rnd_layergrp_id_t gid, int del_layers, rnd_bool undoable)
{
	undo_layergrp_del_t *r;
	pcb_layer_stack_t *stk = &pcb->LayerGroups;

	if ((gid < 0) || (gid >= stk->len))
		return -1;

	if (!undoable || !del_layers) {
		pcb_layergrp_del_1(pcb, gid, del_layers, undoable);
		pcb_layergrp_del_2(pcb, gid);
		return 0;
	}

	/* undoable */
	pcb_undo_freeze_serial();
	pcb_layergrp_del_1(pcb, gid, 1, 1);
	r = pcb_undo_alloc(pcb, &undo_layergrp_del, sizeof(undo_layergrp_del_t));
	r->pcb = pcb;
	r->gid = gid;
	r->del = 1;
	grp_move_struct(&r->save, &stk->grp[gid]);
	pcb_layergrp_del_2(pcb, gid);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();
	return 0;
}

void pcb_layergrp_undoable_created(pcb_layergrp_t *grp)
{
	pcb_board_t *pcb = grp->parent.board;
	undo_layergrp_del_t *r;

	assert(grp->parent_type == PCB_PARENT_BOARD);
	r = pcb_undo_alloc(pcb, &undo_layergrp_del, sizeof(undo_layergrp_del_t));
	memset(r, 0, sizeof(undo_layergrp_del_t));
	r->pcb = pcb;
	r->gid = grp - pcb->LayerGroups.grp;
	r->del = 0;
	pcb_undo_inc_serial();
}

int pcb_layergrp_move(pcb_board_t *pcb, rnd_layergrp_id_t from, rnd_layergrp_id_t to_before)
{
	pcb_layer_stack_t *stk = &pcb->LayerGroups;
	pcb_layergrp_t tmp;
	int n;

	if ((from < 0) || (from >= stk->len))
		return -1;

	if ((to_before < 0) || (to_before >= stk->len))
		return -1;

	if ((to_before == from + 1) || (to_before == from))
		return 0;

	memcpy(&tmp, &stk->grp[from], sizeof(pcb_layergrp_t));
	memset(&stk->grp[from], 0, sizeof(pcb_layergrp_t));
	if (to_before < from + 1) {
		move_grps(pcb, stk, to_before, from-1, +1);
		memcpy(&stk->grp[to_before], &tmp, sizeof(pcb_layergrp_t));
	}
	else {
		move_grps(pcb, stk, from+1, to_before-1, -1);
		memcpy(&stk->grp[to_before-1], &tmp, sizeof(pcb_layergrp_t));
	}

	/* fix up the group id for the layers of the group moved */
	for(n = 0; n < stk->grp[to_before].len; n++) {
		pcb_layer_t *l = pcb_get_layer(pcb->Data, stk->grp[to_before].lid[n]);
		if ((l != NULL) && (l->meta.real.grp > 0))
			l->meta.real.grp = to_before;
	}

	NOTIFY(pcb);
	return 0;
}


	/* ugly hack: remove the extra substrate we added after the outline layer */
void pcb_layergrp_fix_old_outline(pcb_board_t *pcb)
{
	pcb_layer_stack_t *LayerGroup = &pcb->LayerGroups;
	pcb_layergrp_t *g = pcb_get_grp(LayerGroup, PCB_LYT_ANYWHERE, PCB_LYT_BOUNDARY);
	if ((g != NULL) && (g[1].ltype & PCB_LYT_SUBSTRATE)) {
		rnd_layergrp_id_t gid = g - LayerGroup->grp + 1;
		pcb_layergrp_del(pcb, gid, 0, 0);
	}
}

void pcb_layergrp_fix_turn_to_outline(pcb_layergrp_t *g)
{
	g->ltype |= PCB_LYT_BOUNDARY;
	g->ltype &= ~PCB_LYT_COPPER;
	g->ltype &= ~(PCB_LYT_ANYWHERE);
	free(g->name);
	g->name = rnd_strdup("global_outline");
	pcb_layergrp_set_purpose__(g, rnd_strdup("uroute"), 0);
}


/* old outline rules from geda/pcb */
#define LAYER_IS_OUTLINE(idx) ((pcb->Data->Layer[idx].name != NULL) && ((strcmp(pcb->Data->Layer[idx].name, "route") == 0 || rnd_strcasecmp(pcb->Data->Layer[(idx)].name, "outline") == 0)))

void pcb_layergrp_fix_old_outline_detect(pcb_board_t *pcb, pcb_layergrp_t *g)
{
	int n, converted = 0;

	for(n = 0; n < g->len; n++) {
		if (g->lid[n] < 0)
			continue;
		if (LAYER_IS_OUTLINE(g->lid[n])) {
			if (!converted) {
				pcb_layergrp_fix_turn_to_outline(g);
				converted = 1;
				break;
			}
		}
	}

	if (converted) {
		for(n = 0; n < g->len; n++)
			pcb->Data->Layer[g->lid[n]].comb = PCB_LYC_AUTO;
	}
}

int pcb_layer_gui_set_layer(rnd_layergrp_id_t gid, const pcb_layergrp_t *grp, int is_empty, rnd_xform_t **xform)
{
	/* if there's no GUI, that means no draw should be done */
	if (rnd_gui == NULL)
		return 0;

	if (xform != NULL)
		*xform = NULL;

	if (rnd_render->set_layer_group != NULL)
		return rnd_render->set_layer_group(rnd_gui, gid, grp->purpose, grp->purpi, grp->lid[0], grp->ltype, is_empty, xform);

	/* if the GUI doesn't have a set_layer, assume it wants to draw all layers */
	return 1;
}

int pcb_layer_gui_set_glayer(pcb_board_t *pcb, rnd_layergrp_id_t grp, int is_empty, rnd_xform_t **xform)
{
	return pcb_layer_gui_set_layer(grp, &pcb->LayerGroups.grp[grp], is_empty, xform);
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

int pcb_layergrp_list(const pcb_board_t *pcb, pcb_layer_type_t mask, rnd_layergrp_id_t *res, int res_len)
{
	int group, used = 0;
	for (group = 0; group < pcb->LayerGroups.len; group++) {
		if ((pcb_layergrp_flags(pcb, group) & mask) == mask)
				APPEND(group);
	}
	return used;
}

int pcb_layergrp_listp(const pcb_board_t *pcb, pcb_layer_type_t mask, rnd_layergrp_id_t *res, int res_len, int purpi, const char *purpose)
{
	int group, used = 0;
	const pcb_layergrp_t *g;
	for (group = 0, g = pcb->LayerGroups.grp; group < pcb->LayerGroups.len; group++,g++) {
		if ((pcb_layergrp_flags(pcb, group) & mask) == mask) {
			if (((purpose == NULL) || ((g->purpose != NULL) && (strcmp(purpose, g->purpose) == 0))) && ((purpi == -1) || (purpi == g->purpi))) {
				APPEND(group);
			}
		}
	}
	return used;
}

int pcb_layergrp_list_any(const pcb_board_t *pcb, pcb_layer_type_t mask, rnd_layergrp_id_t *res, int res_len)
{
	int group, used = 0;
	for (group = 0; group < pcb->LayerGroups.len; group++) {
		if ((pcb_layergrp_flags(pcb, group) & mask))
				APPEND(group);
	}
	return used;
}

int pcb_layer_add_in_group_(pcb_board_t *pcb, pcb_layergrp_t *grp, rnd_layergrp_id_t group_id, rnd_layer_id_t layer_id)
{
	if ((layer_id < 0) || (layer_id >= pcb->Data->LayerN))
		return -1;

	grp->lid[grp->len] = layer_id;
	grp->len++;
	pcb->Data->Layer[layer_id].meta.real.grp = group_id;

	return 0;
}

int pcb_layer_add_in_group(pcb_board_t *pcb, rnd_layer_id_t layer_id, rnd_layergrp_id_t group_id)
{
	if ((group_id < 0) || (group_id >= pcb->LayerGroups.len))
		return -1;

	return pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[group_id], group_id, layer_id);
}

#define NEWG(g, flags, gname, pcb) \
do { \
	g = &(newg->grp[newg->len]); \
	pcb_layergrp_setup(g, pcb); \
	if (gname != NULL) \
		g->name = rnd_strdup(gname); \
	else \
		g->name = NULL; \
	g->ltype = flags; \
	newg->len++; \
} while(0)

void pcb_layer_group_setup_default(pcb_board_t *pcb)
{
	pcb_layer_stack_t *newg = &pcb->LayerGroups;
	pcb_layergrp_t *g;

	memset(newg, 0, sizeof(pcb_layer_stack_t));

	NEWG(g, PCB_LYT_TOP | PCB_LYT_PASTE, "top_paste", pcb);
	NEWG(g, PCB_LYT_TOP | PCB_LYT_SILK, "top_silk", pcb);
	NEWG(g, PCB_LYT_TOP | PCB_LYT_MASK, "top_mask", pcb);
	NEWG(g, PCB_LYT_TOP | PCB_LYT_COPPER, "top_copper", pcb);
	NEWG(g, PCB_LYT_INTERN | PCB_LYT_SUBSTRATE, NULL, pcb);

	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_COPPER, "bottom_copper", pcb);
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_MASK, "bottom_mask", pcb);
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_SILK, "bottom_silk", pcb);
	NEWG(g, PCB_LYT_BOTTOM | PCB_LYT_PASTE, "bottom_paste", pcb);

/*	NEWG(g, PCB_LYT_INTERN | PCB_LYT_BOUNDARY, "outline");*/
}

void pcb_layer_group_setup_silks(pcb_board_t *pcb)
{
	rnd_layergrp_id_t gid;
	for(gid = 0; gid < pcb->LayerGroups.len; gid++)
		if ((pcb->LayerGroups.grp[gid].ltype & PCB_LYT_SILK) && (pcb->LayerGroups.grp[gid].len == 0))
			pcb_layer_create(pcb, gid, "silk", 0);
}

/*** undoable rename ***/

typedef struct {
	pcb_layergrp_t *grp;
	char *name;
} undo_layergrp_rename_t;

static int undo_layergrp_rename_swap(void *udata)
{
	char *old;
	undo_layergrp_rename_t *r = udata;

	old = (char *)r->grp->name;
	r->grp->name = r->name;
	r->name = old;

	rnd_event(&r->grp->parent.board->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	return 0;
}

static void undo_layergrp_rename_print(void *udata, char *dst, size_t dst_len)
{
	undo_layergrp_rename_t *r = udata;
	rnd_snprintf(dst, dst_len, "rename layergrp: '%s' -> '%s'", r->grp->name, r->name);
}

static void undo_layergrp_rename_free(void *udata)
{
	undo_layergrp_rename_t *r = udata;
	free(r->name);
}


static const uundo_oper_t undo_layergrp_rename = {
	core_layergrp_cookie,
	undo_layergrp_rename_free,
	undo_layergrp_rename_swap,
	undo_layergrp_rename_swap,
	undo_layergrp_rename_print
};


int pcb_layergrp_rename_(pcb_layergrp_t *grp, char *name, rnd_bool undoable)
{
	undo_layergrp_rename_t rtmp, *r = &rtmp;

	assert(grp->parent_type == PCB_PARENT_BOARD);

	if (undoable)
		r = pcb_undo_alloc(grp->parent.board, &undo_layergrp_rename, sizeof(undo_layergrp_rename_t));

	r->grp = grp;
	r->name = name;

	undo_layergrp_rename_swap(r);
	if (undoable)
		pcb_undo_inc_serial();
	else
		undo_layergrp_rename_free(r);

	return 0;
}

int pcb_layergrp_rename(pcb_board_t *pcb, rnd_layergrp_id_t gid, const char *name, rnd_bool undoable)
{
	pcb_layergrp_t *grp = pcb_get_layergrp(pcb, gid);
	if (grp == NULL) return -1;
	return pcb_layergrp_rename_(grp, rnd_strdup(name), undoable);
}

/*** undoable set-purpose ***/

typedef struct {
	pcb_layergrp_t *grp;
	char *purps;
	int purpi;
} undo_layergrp_repurp_t;

static int undo_layergrp_repurp_swap(void *udata)
{
	char *olds;
	int oldi;
	undo_layergrp_repurp_t *r = udata;

	olds = (char *)r->grp->purpose;
	oldi = r->grp->purpi;
	r->grp->purpose = r->purps;
	r->grp->purpi = r->purpi;
	r->purps = olds;
	r->purpi = oldi;

	return 0;
}

static void undo_layergrp_repurp_print(void *udata, char *dst, size_t dst_len)
{
	undo_layergrp_repurp_t *r = udata;
	rnd_snprintf(dst, dst_len, "repurpose layergrp: '%s' -> '%s'", r->grp->purpose, r->purps);
}

static void undo_layergrp_repurp_free(void *udata)
{
	undo_layergrp_repurp_t *r = udata;
	free(r->purps);
}


static const uundo_oper_t undo_layergrp_repurp = {
	core_layergrp_cookie,
	undo_layergrp_repurp_free,
	undo_layergrp_repurp_swap,
	undo_layergrp_repurp_swap,
	undo_layergrp_repurp_print
};


int pcb_layergrp_set_purpose__(pcb_layergrp_t *grp, char *purpose, rnd_bool undoable)
{
	undo_layergrp_repurp_t rtmp, *r = &rtmp;

	if (undoable)
		r = pcb_undo_alloc(grp->parent.board, &undo_layergrp_repurp, sizeof(undo_layergrp_repurp_t));

	r->grp = grp;
	if (purpose == NULL) {
		r->purps = NULL;
		r->purpi = F_user;
	}
	else {
		r->purps = purpose;
		r->purpi = rnd_funchash_get(purpose, NULL);
		if (r->purpi < 0)
			r->purpi = F_user;
	}

	undo_layergrp_repurp_swap(r);
	if (undoable)
		pcb_undo_inc_serial();
	else
		undo_layergrp_repurp_free(r);

	return 0;
}

int pcb_layergrp_set_purpose_(pcb_layergrp_t *lg, char *purpose, rnd_bool undoable)
{
	int ret = pcb_layergrp_set_purpose__(lg, purpose, undoable);
	assert(lg->parent_type == PCB_PARENT_BOARD);
	rnd_event(&lg->parent.board->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	return ret;
}

int pcb_layergrp_set_purpose(pcb_layergrp_t *lg, const char *purpose, rnd_bool undoable)
{
	return pcb_layergrp_set_purpose_(lg, rnd_strdup(purpose), undoable);
}

/*** undoable flags/type set ***/

typedef struct {
	pcb_layergrp_t *grp;
	pcb_layer_type_t ltype;
} undo_layergrp_ltype_t;

static int undo_layergrp_ltype_swap(void *udata)
{
	pcb_layer_type_t old;
	undo_layergrp_ltype_t *r = udata;

	old = r->grp->ltype;
	r->grp->ltype = r->ltype;
	r->ltype = old;

	return 0;
}

static void undo_layergrp_ltype_print(void *udata, char *dst, size_t dst_len)
{
	undo_layergrp_ltype_t *r = udata;
	rnd_snprintf(dst, dst_len, "layergrp type: '%lx' -> '%lx'", r->grp->ltype, r->ltype);
}

static const uundo_oper_t undo_layergrp_ltype = {
	core_layergrp_cookie,
	NULL,
	undo_layergrp_ltype_swap,
	undo_layergrp_ltype_swap,
	undo_layergrp_ltype_print
};



int pcb_layergrp_set_ltype(pcb_layergrp_t *grp, pcb_layer_type_t lyt, rnd_bool undoable)
{
	undo_layergrp_ltype_t rtmp, *r = &rtmp;

	if (undoable)
		r = pcb_undo_alloc(grp->parent.board, &undo_layergrp_ltype, sizeof(undo_layergrp_ltype_t));

	r->grp = grp;
	r->ltype = lyt;

	undo_layergrp_ltype_swap(r);
	if (undoable)
		pcb_undo_inc_serial();

	return 0;

}

rnd_layergrp_id_t pcb_layergrp_by_name(pcb_board_t *pcb, const char *name)
{
	rnd_layergrp_id_t n;
	for (n = 0; n < pcb->LayerGroups.len; n++)
		if ((pcb->LayerGroups.grp[n].name != NULL) && (strcmp(pcb->LayerGroups.grp[n].name, name) == 0))
			return n;
	return -1;
}

int pcb_layergrp_dist(pcb_board_t *pcb, rnd_layergrp_id_t gid1, rnd_layergrp_id_t gid2, pcb_layer_type_t mask, int *diff)
{
	int gid, d, cnt;

	if ((gid1 < 0) || (gid2 < 0))
		return -1;
	if (gid1 == gid2) {
		*diff = 0;
		return 0;
	}

	d = (gid1 < gid2) ? +1 : -1;
	cnt = 0;
	for(gid = gid1; gid != gid2; gid += d) {
		if ((gid < 0) || (gid >= pcb->LayerGroups.len))
			return -1;
		if ((pcb->LayerGroups.grp[gid].ltype & mask) == mask)
			cnt++;
	}
	*diff = cnt;
	return 0;
}

rnd_layergrp_id_t pcb_layergrp_step(pcb_board_t *pcb, rnd_layergrp_id_t gid, int steps, pcb_layer_type_t mask)
{
	int d;

	if (gid < 0)
		return -1;

	if (steps == 0)
		return gid;

	if (steps < 0) {
		d = -1;
		steps = -steps;
	}
	else
		d = 1;

	for(gid += d;; gid += d) {
		if ((gid < 0) || (gid >= pcb->LayerGroups.len))
			return -1;
		if ((pcb->LayerGroups.grp[gid].ltype & mask) == mask) {
			steps--;
			if (steps == 0)
				return gid;
		}
	}
	return -1;
}

void pcb_layergrp_create_missing_substrate(pcb_board_t *pcb)
{
	rnd_layergrp_id_t g;
	for(g = 0; g < ((rnd_layergrp_id_t)pcb->LayerGroups.len)-2; g++) {
		pcb_layergrp_t *g0 = &pcb->LayerGroups.grp[g], *g1 = &pcb->LayerGroups.grp[g+1];
		if ((g < ((rnd_layergrp_id_t)pcb->LayerGroups.len)-3) && (g1->ltype & PCB_LYT_BOUNDARY)) g1++;
		if ((g0->ltype & PCB_LYT_COPPER) && (g1->ltype & PCB_LYT_COPPER)) {
			pcb_layergrp_t *ng = pcb_layergrp_insert_after(pcb, g);
			ng->ltype = PCB_LYT_INTERN | PCB_LYT_SUBSTRATE;
			ng->name = rnd_strdup("implicit_subst");
			pcb_layergrp_setup(ng, pcb);
		}
	}
}

int pcb_layer_create_all_for_recipe(pcb_board_t *pcb, pcb_layer_t *layer, int num_layer)
{
	int n, existing_intern = 0, want_intern = 0;
	static char *anon = "anon";

	for(n = 0; n < pcb->LayerGroups.len; n++)
		if ((pcb->LayerGroups.grp[n].ltype & PCB_LYT_COPPER) && (pcb->LayerGroups.grp[n].ltype & PCB_LYT_INTERN))
			existing_intern++;

	for(n = 0; n < num_layer; n++) {
		pcb_layer_t *ly = layer + n;
		pcb_layer_t *dst = pcb_layer_resolve_binding(pcb, ly);
		pcb_layergrp_t *grp;

		if ((ly->meta.bound.type & PCB_LYT_COPPER) && (ly->meta.bound.type & PCB_LYT_INTERN)) {
			want_intern++;
			continue;
		}

		if (dst != NULL)
			continue;

		if (ly->meta.bound.type & PCB_LYT_VIRTUAL) {
			/* no chance to have this yet */
			continue;
		}

		if (ly->meta.bound.type & PCB_LYT_BOUNDARY) {
			pcb_layergrp_t *grp = pcb_get_grp_new_misc(pcb);
			rnd_layer_id_t nlid;
			pcb_layer_t *nly;
			grp->ltype = PCB_LYT_BOUNDARY;
			grp->name = rnd_strdup("outline");
			if (ly->meta.bound.purpose != NULL)
				pcb_layergrp_set_purpose__(grp, rnd_strdup(ly->meta.bound.purpose), 0);
			nlid = pcb_layer_create(pcb, pcb_layergrp_id(pcb, grp), ly->name, 0);
			nly = pcb_get_layer(pcb->Data, nlid);
			if (nly != NULL)
				nly->comb = ly->comb;
			continue;
		}


		if (ly->meta.bound.type & PCB_LYT_COPPER) { /* top or bottom copper */
			grp = pcb_get_grp(&pcb->LayerGroups, ly->meta.bound.type & PCB_LYT_ANYWHERE, PCB_LYT_COPPER);
			if (grp != NULL) {
				pcb_layer_create(pcb, pcb_layergrp_id(pcb, grp), ly->name, 0);
				continue;
			}
		}

		grp = pcb_get_grp(&pcb->LayerGroups, ly->meta.bound.type & PCB_LYT_ANYWHERE, ly->meta.bound.type & PCB_LYT_ANYTHING);
		if (grp != NULL) {
			rnd_layer_id_t lid = pcb_layer_create(pcb, pcb_layergrp_id(pcb, grp), ly->name, 0);
			pcb_layer_t *nly = pcb_get_layer(pcb->Data, lid);
			nly->comb = ly->comb;
			continue;
		}

		if (!((ly->meta.bound.type & PCB_LYT_DOC) || (ly->meta.bound.type & PCB_LYT_MECH))) /* doc layers are created later */
			rnd_message(RND_MSG_ERROR, "Failed to create layer from recipe %s\n", ly->name);
	}

	if (want_intern > existing_intern) {
		int int_ofs = 0;
/*rnd_trace("want: %d have: %d\n", want_intern, existing_intern);*/
		/* create enough dummy internal layers, mark them by name anon */
		while(want_intern > existing_intern) {
			pcb_layergrp_t *grp = pcb_get_grp_new_intern(pcb, -1);
			grp->name = anon;
			existing_intern++;
		}
		/* rename new interns from the recipe layer names */
		for(n = 0; n < pcb->LayerGroups.len; n++) {
			if (pcb->LayerGroups.grp[n].name == anon) {
				int m;
				int_ofs++;
				for(m = 0; m < num_layer; m++) {
					pcb_layer_t *ly = layer + m;
					if ((ly->meta.bound.type & PCB_LYT_COPPER) && (ly->meta.bound.type & PCB_LYT_INTERN)) {
						int offs = ly->meta.bound.stack_offs;
/*rnd_trace("offs: %d (%d) == %d\n", offs, existing_intern + offs + 1, int_ofs);*/
						if (offs < 0)
							offs = existing_intern + offs + 1;
						if ((offs == int_ofs) && (ly->name != NULL)) {
							pcb->LayerGroups.grp[n].name = rnd_strdup("internal");
							pcb_layer_create(pcb, n, ly->name, 0);
							goto found;
						}
					}
				}
				pcb->LayerGroups.grp[n].name = rnd_strdup("anon-recipe");
				found:;
			}
		}
	}


	/* create doc and mech layers */
	for(n = 0; n < num_layer; n++) {
		pcb_layer_t *ly = layer + n;
		if ((ly->meta.bound.type & PCB_LYT_DOC) || (ly->meta.bound.type & PCB_LYT_MECH)) {
			pcb_layergrp_t *grp = pcb_get_grp_new_misc(pcb);
			if (grp != NULL) {
				grp->ltype = ly->meta.bound.type;
				grp->name = rnd_strdup(ly->name);
				pcb_layer_create(pcb, pcb_layergrp_id(pcb, grp), ly->name, 0);
				pcb_layer_resolve_binding(pcb, ly);
			}
		}
	}
	return 0;
}

void pcb_layergroup_free_stack(pcb_layer_stack_t *st)
{
	int n;

	for(n = 0; n < st->len; n++)
		pcb_layergrp_free_fields(st->grp + n);

	free(st->cache.copper);
	st->cache.copper = NULL;
	st->cache.copper_len = st->cache.copper_alloced = 0;
}

const pcb_dflgmap_t pcb_dflg_top_noncop[] = {
	{"top_paste",           PCB_LYT_TOP | PCB_LYT_PASTE,     NULL, PCB_LYC_AUTO, 0},
	{"top_silk",            PCB_LYT_TOP | PCB_LYT_SILK,      NULL, PCB_LYC_AUTO, 0},
	{"top_mask",            PCB_LYT_TOP | PCB_LYT_MASK,      NULL, PCB_LYC_SUB | PCB_LYC_AUTO, 0},
	{NULL, 0}
};

const pcb_dflgmap_t pcb_dflg_bot_noncop[] = {
	{"bottom_mask",         PCB_LYT_BOTTOM | PCB_LYT_MASK,   NULL, PCB_LYC_SUB | PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
	{"bottom_silk",         PCB_LYT_BOTTOM | PCB_LYT_SILK,   NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
	{"bottom_paste",        PCB_LYT_BOTTOM | PCB_LYT_PASTE,  NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
	{NULL, 0}
};

const pcb_dflgmap_t pcb_dflg_glob_noncop[] = {
	{"outline",             PCB_LYT_BOUNDARY,                "uroute", 0, 0},
	{"pmech",               PCB_LYT_MECH,                    "proute", PCB_LYC_AUTO, 0},
	{"umech",               PCB_LYT_MECH,                    "uroute", PCB_LYC_AUTO, 0},
	{NULL, 0}
};

const pcb_dflgmap_t pcb_dflgmap[] = {
	{"top_paste",           PCB_LYT_TOP | PCB_LYT_PASTE,     NULL, PCB_LYC_AUTO, 0},
	{"top_silk",            PCB_LYT_TOP | PCB_LYT_SILK,      NULL, PCB_LYC_AUTO, 0},
	{"top_mask",            PCB_LYT_TOP | PCB_LYT_MASK,      NULL, PCB_LYC_SUB | PCB_LYC_AUTO, 0},
	{"top_copper",          PCB_LYT_TOP | PCB_LYT_COPPER,    NULL, 0, 0},
	{"any_internal_copper", PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0, 0},
	{"bottom_copper",       PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0, 0},
	{"bottom_mask",         PCB_LYT_BOTTOM | PCB_LYT_MASK,   NULL, PCB_LYC_SUB | PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
	{"bottom_silk",         PCB_LYT_BOTTOM | PCB_LYT_SILK,   NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},
	{"bottom_paste",        PCB_LYT_BOTTOM | PCB_LYT_PASTE,  NULL, PCB_LYC_AUTO, PCB_DFLGMAP_FORCE_END},

	{"outline",             PCB_LYT_BOUNDARY,                "uroute", 0, 0},
	{"pmech",               PCB_LYT_MECH,                    "proute", PCB_LYC_AUTO, 0},
	{"umech",               PCB_LYT_MECH,                    "uroute", PCB_LYC_AUTO, 0},

	{NULL, 0}
};

const pcb_dflgmap_t *pcb_dflgmap_last_top_noncopper = pcb_dflgmap+2;
const pcb_dflgmap_t *pcb_dflgmap_first_bottom_noncopper = pcb_dflgmap+6;
const pcb_dflgmap_t pcb_dflg_top_copper = {
	"top_copper",          PCB_LYT_TOP | PCB_LYT_COPPER, NULL, 0, 0
};
const pcb_dflgmap_t pcb_dflg_int_copper = {
	"int_copper",          PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0, 0
};
const pcb_dflgmap_t pcb_dflg_substrate = {
	"substrate",           PCB_LYT_INTERN | PCB_LYT_SUBSTRATE, NULL, 0, 0
};
const pcb_dflgmap_t pcb_dflg_bot_copper = {
	"bot_copper",          PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0, 0
};

const pcb_dflgmap_t pcb_dflg_outline = {
	"outline",             PCB_LYT_BOUNDARY, "uroute", 0, 0
};

const pcb_dflgmap_t pcb_dflgmap_doc[] = {
	{"top_assy",           PCB_LYT_TOP | PCB_LYT_DOC,     "assy", 0,            PCB_DFLGMAP_INIT_INVIS},
	{"bottom_assy",        PCB_LYT_BOTTOM | PCB_LYT_DOC,  "assy", 0,            PCB_DFLGMAP_INIT_INVIS},
	{"fab",                PCB_LYT_TOP | PCB_LYT_DOC,     "fab",  PCB_LYC_AUTO, PCB_DFLGMAP_INIT_INVIS},
	{NULL, 0}
};

void pcb_layergrp_set_dflgly(pcb_board_t *pcb, pcb_layergrp_t *grp, const pcb_dflgmap_t *src, const char *grname, const char *lyname)
{
	rnd_layergrp_id_t gid = grp - pcb->LayerGroups.grp;

	if (grname == NULL)
		grname = src->name;
	if (lyname == NULL)
		lyname = src->name;

	grp->name = rnd_strdup(grname);
	grp->ltype = src->lyt;

	free(grp->purpose);
	grp->purpose = NULL;
	if (src->purpose != NULL)
		pcb_layergrp_set_purpose__(grp, rnd_strdup(src->purpose), 1);

	if (grp->len == 0) {
		rnd_layer_id_t lid = pcb_layer_create(pcb, gid, lyname, 0);
		if (lid >= 0) {
			pcb->Data->Layer[lid].comb = src->comb;
		}
	}
	grp->valid = 1;
}

static void pcb_layergrp_upgrade_by_map_(pcb_board_t *pcb, const pcb_dflgmap_t *map, int force)
{
	const pcb_dflgmap_t *m;
	pcb_layergrp_t *grp;
	rnd_layergrp_id_t gid;

	inhibit_notify++;
	for(m = map; m->name != NULL; m++) {
		int found = 0;

		if (!force) {
			if (m->purpose == NULL)
				found = pcb_layergrp_list(pcb, m->lyt, &gid, 1);
			else
				found = pcb_layergrp_listp(pcb, m->lyt, &gid, 1, 0, m->purpose);
		}

		if (found == 1) {
			grp = &pcb->LayerGroups.grp[gid];
			free(grp->name);
		}
		else
			grp = pcb_get_grp_new_intern_(pcb, 1, (m->flags & PCB_DFLGMAP_FORCE_END));
		if (m->flags & PCB_DFLGMAP_INIT_INVIS)
			pcb_attribute_put(&grp->Attributes, "init-invis", "1");
		pcb_layergrp_set_dflgly(pcb, grp, m, NULL, NULL);
	}
	inhibit_notify--;
	NOTIFY(pcb);
}

void pcb_layergrp_upgrade_by_map(pcb_board_t *pcb, const pcb_dflgmap_t *map)
{
	pcb_layergrp_upgrade_by_map_(pcb, map, 0);
}

void pcb_layergrp_create_by_map(pcb_board_t *pcb, const pcb_dflgmap_t *map)
{
	pcb_layergrp_upgrade_by_map_(pcb, map, 1);
}


void pcb_layergrp_upgrade_to_pstk(pcb_board_t *pcb)
{
	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap);
}

static rnd_layergrp_id_t pcb_layergrp_get_cached(pcb_board_t *pcb, rnd_layer_id_t *cache, unsigned int loc, unsigned int typ)
{
	pcb_layergrp_t *g;

	/* check if the last known value is still accurate */
	if ((*cache >= 0) && (*cache < pcb->LayerGroups.len)) {
		g = &(pcb->LayerGroups.grp[*cache]);
		if ((g->ltype & loc) && (g->ltype & typ))
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

rnd_layergrp_id_t pcb_layergrp_get_bottom_mask()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_MASK);
}

rnd_layergrp_id_t pcb_layergrp_get_top_mask()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_MASK);
}

rnd_layergrp_id_t pcb_layergrp_get_bottom_paste()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_PASTE);
}

rnd_layergrp_id_t pcb_layergrp_get_top_paste()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_PASTE);
}

rnd_layergrp_id_t pcb_layergrp_get_bottom_silk()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_SILK);
}

rnd_layergrp_id_t pcb_layergrp_get_top_silk()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_SILK);
}

rnd_layergrp_id_t pcb_layergrp_get_bottom_copper()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
}

rnd_layergrp_id_t pcb_layergrp_get_top_copper()
{
	static rnd_layer_id_t cache = -1;
	return pcb_layergrp_get_cached(PCB, &cache, PCB_LYT_TOP, PCB_LYT_COPPER);
}

/* Note: these function is in this file mainly to access the static inlines */
int pcb_silk_on(pcb_board_t *pcb)
{
	static rnd_layer_id_t ts = -1, bs = -1;
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
	static rnd_layer_id_t tm = -1, bm = -1;
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
	static rnd_layer_id_t tp = -1, bp = -1;
	pcb_layergrp_get_cached(pcb, &tp, PCB_LYT_TOP, PCB_LYT_PASTE);
	if ((tp >= 0) && (pcb->LayerGroups.grp[tp].vis))
		return 1;
	pcb_layergrp_get_cached(pcb, &bp, PCB_LYT_BOTTOM, PCB_LYT_PASTE);
	if ((bp >= 0) && (pcb->LayerGroups.grp[bp].vis))
		return 1;
	return 0;
}


void pcb_layergrp_copper_cache_update(pcb_layer_stack_t *st)
{
	rnd_layergrp_id_t n;
	if (st->len > st->cache.copper_alloced) {
		st->cache.copper_alloced = st->len + 64;
		st->cache.copper = realloc(st->cache.copper, sizeof(st->cache.copper[0]) * st->cache.copper_alloced);
	}

	st->cache.copper_len = 0;
	for(n = 0; n < st->len; n++)
		if (st->grp[n].ltype & PCB_LYT_COPPER)
			st->cache.copper[st->cache.copper_len++] = n;

	st->cache.copper_valid = 1;
}

rnd_bool pcb_has_explicit_outline(pcb_board_t *pcb)
{
	int i;
	pcb_layergrp_t *g;
	for(i = 0, g = pcb->LayerGroups.grp; i < pcb->LayerGroups.len; i++,g++)
		if (PCB_LAYER_IS_OUTLINE(g->ltype, g->purpi) && !pcb_layergrp_is_pure_empty(pcb, i))
			return 1;
	return 0;
}

const char *pcb_layergrp_thickness_attr(pcb_layergrp_t *grp, const char *namespace)
{
	const char *s = NULL;
	if (namespace != NULL)
		s = pcb_attribute_get_namespace(&grp->Attributes, namespace, "thickness");
	if (s == NULL)
		s = pcb_attribute_get(&grp->Attributes, "thickness");
	return s;
}
