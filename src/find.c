/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 */

/* Follow galvanic connections through overlapping objects */

#include "config.h"
#include "find.h"
#include "undo.h"
#include "obj_subc_parent.h"

TODO("find: this is the only non-reentrant part - pass it on!")
pcb_coord_t Bloat = 0;

#include "find_geo.c"
#include "find_any_isect.c"

/* trickery: keeping vtp0 is reentrant and is cheaper than keeping lists,
   at least for appending. But as long as only the last item is removed,
   it's also cheap on remove! */

/* Do everything that needs to be done for an object found */
static int pcb_find_found(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	if (ctx->list_found)
		vtp0_append(&ctx->found, obj);

	if ((ctx->flag_set != 0) || (ctx->flag_clr != 0)) {
		if (ctx->flag_chg_undoable)
			pcb_undo_add_obj_to_flag(obj);
		if (ctx->flag_set != 0)
			PCB_FLAG_SET(ctx->flag_set, obj);
		if (ctx->flag_clr != 0)
			PCB_FLAG_CLEAR(ctx->flag_clr, obj);
	}

	ctx->nfound++;

	if ((ctx->found_cb != NULL) && (ctx->found_cb(ctx, obj, arrived_from, ctype) != 0)) {
		ctx->aborted = 1;
		return 1;
	}

	return 0;
}


static int pcb_find_addobj(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype, int jump)
{
	PCB_DFLAG_SET(&obj->Flags, ctx->mark);
	if (jump)
		vtp0_append(&ctx->open, obj);

	if (pcb_find_found(ctx, obj, arrived_from, ctype) != 0) {
		ctx->aborted = 1;
		return 1;
	}
	return 0;
}

static void find_int_conn(pcb_find_t *ctx, pcb_any_obj_t *from_)
{
	void *from = from_; /* for warningless comparison */
	pcb_subc_t *s;
	int ic;

	s = pcb_obj_parent_subc(from_);
	if (s == NULL)
		return;

	ic = from_->intconn;

	PCB_PADSTACK_LOOP(s->data);
	{
		if ((padstack != from) && (padstack->term != NULL) && (padstack->intconn == ic) && (!(PCB_DFLAG_TEST(&(padstack->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)padstack, from_, PCB_FCT_INTCONN, 1) != 0)
				return;
	}
	PCB_END_LOOP;

	PCB_LINE_COPPER_LOOP(s->data);
	{
		if ((line != from) && (line->term != NULL) && (line->intconn == ic) && (!(PCB_DFLAG_TEST(&(line->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)line, from_, PCB_FCT_INTCONN, 1) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_COPPER_LOOP(s->data);
	{
		if ((arc != from) && (arc->term != NULL) && (arc->intconn == ic) && (!(PCB_DFLAG_TEST(&(arc->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)arc, from_, PCB_FCT_INTCONN, 1) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

	PCB_POLY_COPPER_LOOP(s->data);
	{
		if ((polygon != from) && (polygon->term != NULL) && (polygon->intconn == ic) && (!(PCB_DFLAG_TEST(&(polygon->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)polygon, from_, PCB_FCT_INTCONN, 1) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

TODO("find: no find through text yet")
#if 0
	PCB_TEXT_COPPER_LOOP(s->data);
	{
		if ((text != from) && (text->term != NULL) && (text->intconn == ic) && (!(PCB_DFLAG_TEST(&(text->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)text, from_, PCB_FCT_INTCONN, 1) != 0)
				return;
	}
	PCB_ENDALL_LOOP;
#endif
}

/* return whether a and b are in the same internal-no-connection group */
static pcb_bool int_noconn(pcb_any_obj_t *a, pcb_any_obj_t *b)
{
	pcb_subc_t *pa, *pb;

	/* cheap test: they need to have valid and matching intnoconn */
	if ((a->intnoconn == 0) || (a->intnoconn != b->intnoconn))
		return pcb_false;

	/* expensive tests: they need to be in the same subc */
	pa = pcb_obj_parent_subc(a);
	if (pa == NULL)
		return pcb_false;

	pb = pcb_obj_parent_subc(b);

	return (pa == pb);
}

#define INOCONN(a,b) int_noconn((pcb_any_obj_t *)a, (pcb_any_obj_t *)b)

#define PCB_FIND_CHECK(ctx, curr, obj, ctype, retstmt) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!(PCB_DFLAG_TEST(&(__obj__->Flags), ctx->mark))) { \
			if (!INOCONN(curr, obj) && (pcb_intersect_obj_obj(curr, __obj__))) {\
				if (pcb_find_addobj(ctx, __obj__, curr, ctype, 1) != 0) { retstmt; } \
				if ((__obj__->term != NULL) && (!ctx->ignore_intconn) && (__obj__->intconn > 0)) \
					find_int_conn(ctx, __obj__); \
			} \
		} \
	} while(0)

#define PCB_FIND_CHECK_RAT(ctx, curr, obj, ctype, retstmt) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!(PCB_DFLAG_TEST(&(__obj__->Flags), ctx->mark))) { \
			if (!INOCONN(curr, obj) && (pcb_intersect_obj_obj(curr, __obj__))) {\
				if (pcb_find_addobj(ctx, __obj__, curr, ctype, ctx->consider_rats) != 0) { retstmt; } \
				if ((__obj__->term != NULL) && (!ctx->ignore_intconn) && (__obj__->intconn > 0)) \
					find_int_conn(ctx, __obj__); \
			} \
		} \
	} while(0)

void pcb_find_on_layer(pcb_find_t *ctx, pcb_layer_t *l, pcb_any_obj_t *curr, pcb_rtree_box_t *sb, pcb_found_conn_type_t ctype)
{
	pcb_rtree_it_t it;
	pcb_box_t *n;

	if (l->line_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->line_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);
		pcb_r_end(&it);
	}

	if (l->arc_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->arc_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);
		pcb_r_end(&it);
	}

	if (l->polygon_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->polygon_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);
		pcb_r_end(&it);
	}

	if (l->text_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->text_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);
		pcb_r_end(&it);
	}
}

void pcb_find_on_layergrp(pcb_find_t *ctx, pcb_layergrp_t *g, pcb_any_obj_t *curr, pcb_rtree_box_t *sb, pcb_found_conn_type_t ctype)
{
	int n;
	if (g == NULL)
		return;
	for(n = 0; n < g->len; n++)
		pcb_find_on_layer(ctx, &ctx->data->Layer[g->lid[n]], curr, sb, ctype);
}

static void pcb_find_rat(pcb_find_t *ctx, pcb_rat_t *rat)
{
	pcb_rtree_box_t sb;

	sb.x1 = rat->Point1.X; sb.x2 = rat->Point1.X+1;
	sb.y1 = rat->Point1.Y; sb.y2 = rat->Point1.Y+1;
	pcb_find_on_layergrp(ctx, pcb_get_layergrp(ctx->pcb, rat->group1), (pcb_any_obj_t *)rat, &sb, PCB_FCT_RAT);

	sb.x1 = rat->Point2.X; sb.x2 = rat->Point2.X+1;
	sb.y1 = rat->Point2.Y; sb.y2 = rat->Point2.Y+1;
	pcb_find_on_layergrp(ctx, pcb_get_layergrp(ctx->pcb, rat->group2), (pcb_any_obj_t *)rat, &sb, PCB_FCT_RAT);
}

static unsigned long pcb_find_exec(pcb_find_t *ctx)
{
	pcb_any_obj_t *curr;
	pcb_found_conn_type_t ctype;

	if ((ctx->start_layergrp == NULL) && (ctx->open.used > 0) && (ctx->pcb != NULL)) {
		curr = ctx->open.array[0];
		if ((curr != NULL) && (curr->parent_type == PCB_PARENT_LAYER)) {
			pcb_layergrp_id_t gid = pcb_layer_get_group_(curr->parent.layer);
			ctx->start_layergrp = pcb_get_layergrp(ctx->pcb, gid);
			if (!ctx->allow_noncopper) {
				TODO("find.c: implement this; special case: starting layer object on non-copper can still jump on padstack!");
			}
		}
	}

	while((ctx->open.used > 0) && (!ctx->aborted)) {
		/* pop the last object, without reallocating to smaller, mark it found */
		ctx->open.used--;
		curr = ctx->open.array[ctx->open.used];
		ctype = curr->type == PCB_OBJ_RAT ? PCB_FCT_RAT : PCB_FCT_COPPER;

		{ /* search unmkared connections: iterative approach */
			pcb_rtree_it_t it;
			pcb_box_t *n;
			pcb_rtree_box_t *sb = (pcb_rtree_box_t *)&curr->bbox_naked;

			if (PCB->Data->padstack_tree != NULL) {
				for(n = pcb_rtree_first(&it, PCB->Data->padstack_tree, sb); n != NULL; n = pcb_rtree_next(&it))
					PCB_FIND_CHECK(ctx, curr, n, ctype, return ctx->nfound);
				pcb_r_end(&it);
			}

			if ((ctx->consider_rats || ctx->only_mark_rats) && (PCB->Data->rat_tree != NULL)) {
				if (PCB->Data->padstack_tree != NULL) {
					for(n = pcb_rtree_first(&it, PCB->Data->rat_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK_RAT(ctx, curr, n, PCB_FCT_RAT, return ctx->nfound);
					pcb_r_end(&it);
				}
			}

			if (curr->type == PCB_OBJ_PSTK) {
				int li;
				pcb_layer_t *l;
				if ((!ctx->stay_layergrp) || (ctx->start_layergrp == NULL)) {
					pcb_pstk_proto_t *proto = pcb_pstk_get_proto((const pcb_pstk_t *)curr);
					for(li = 0, l = ctx->data->Layer; li < ctx->data->LayerN; li++,l++) {
						if (!ctx->allow_noncopper) {
							/* skip anything that's not on a copper layer */
							pcb_layer_type_t lyt = pcb_layer_flags_(l);
							if (!(lyt & PCB_LYT_COPPER))
								continue;
						}
						if (pcb_pstk_shape_at_(ctx->pcb, (pcb_pstk_t *)curr, l, 0))
							pcb_find_on_layer(ctx, l, curr, sb, ctype);
						else if (proto->hplated) { /* consider plated slots */
							pcb_layer_t *rl = pcb_layer_get_real(l);
							if (pcb_pstk_bb_drills(ctx->pcb, (const pcb_pstk_t *)curr, rl->meta.real.grp, NULL))
								pcb_find_on_layer(ctx, l, curr, sb, ctype);
						}
					}
				}
				else {
					for(li = 0; li < ctx->start_layergrp->len; li++) {
						l = pcb_get_layer(ctx->data, ctx->start_layergrp->lid[li]);
						if (l != NULL)
							pcb_find_on_layer(ctx, l, curr, sb, ctype);
					}
				}
			}
			else if (curr->type == PCB_OBJ_RAT) {
				pcb_find_rat(ctx, (pcb_rat_t *)curr);
			}
			else {
				pcb_layergrp_t *g;
				/* layer objects need to be checked against objects in the same layer group only */
				assert(curr->parent_type == PCB_PARENT_LAYER);
				g = pcb_get_layergrp(PCB, pcb_layer_get_group_(curr->parent.layer));
				assert(g != NULL);
				pcb_find_on_layergrp(ctx, g, curr, sb, ctype);
			}
		}
	}

	if (ctx->flag_chg_undoable)
		pcb_undo_inc_serial();

	return ctx->nfound;
}

static int pcb_find_init_(pcb_find_t *ctx, pcb_data_t *data)
{
	if (ctx->in_use)
		return -1;
	ctx->in_use = 1;
	ctx->aborted = 0;
	ctx->mark = pcb_dynflag_alloc("pcb_find_from_obj");
	ctx->data = data;
	ctx->nfound = 0;
	ctx->start_layergrp = NULL;
	ctx->pcb = pcb_data_get_top(data);

	if (ctx->list_found)
		vtp0_init(&ctx->found);
	vtp0_init(&ctx->open);
	return 0;
}

unsigned long pcb_find_from_obj(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from)
{
	if (pcb_find_init_(ctx, data) < 0)
		return -1;

	pcb_data_dynflag_clear(data, ctx->mark);
	if (from != NULL) {
		pcb_find_addobj(ctx, from, NULL, PCB_FCT_START, 1); /* add the starting object with no 'arrived_from' */
		return pcb_find_exec(ctx);
	}
	return 0;
}

unsigned long pcb_find_from_obj_next(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from)
{
	pcb_find_addobj(ctx, from, NULL, PCB_FCT_START, 1); /* add the starting object with no 'arrived_from' */
	return pcb_find_exec(ctx);
}


unsigned long pcb_find_from_xy(pcb_find_t *ctx, pcb_data_t *data, pcb_coord_t x, pcb_coord_t y)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *obj;
	int type;

	type = pcb_search_obj_by_location(PCB_LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, x, y, PCB_SLOP * pcb_pixel_slop);
	if (type == PCB_OBJ_VOID)
		type = pcb_search_obj_by_location(PCB_LOOKUP_MORE, &ptr1, &ptr2, &ptr3, x, y, PCB_SLOP * pcb_pixel_slop);

	if (type == PCB_OBJ_VOID)
		return -1;

	obj = ptr2;
	if ((obj->parent_type == PCB_PARENT_LAYER) && ((pcb_layer_flags_(obj->parent.layer) & PCB_LYT_COPPER) == 0))
		return -1; /* non-conductive object */

	return pcb_find_from_obj(ctx, data, obj);
}


void pcb_find_free(pcb_find_t *ctx)
{
	if (!ctx->in_use)
		return;
	if (ctx->list_found)
		vtp0_uninit(&ctx->found);
	vtp0_uninit(&ctx->open);
	pcb_dynflag_free(ctx->mark);
	ctx->in_use = 0;
}

