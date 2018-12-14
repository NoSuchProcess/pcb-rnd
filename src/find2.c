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
#include "find2.h"

/* trickery: keeping vtp0 is reentrant and is cheaper than keeping lists,
   at least for appending. But as long as only the last item is removed,
   it's also cheap on remove! */

/* Do everything that needs to be done for an object found */
static int pcb_find_found(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	if (ctx->list_found)
		vtp0_append(&ctx->found, obj);

	if (ctx->flag_set != 0) {
		if (ctx->flag_set_undoable)
			pcb_undo_add_obj_to_flag(obj);
		PCB_FLAG_SET(ctx->flag_set, obj);
	}

	ctx->nfound++;

	if ((ctx->found_cb != NULL) && (ctx->found_cb(ctx, obj, arrived_from) != 0)) {
		ctx->aborted = 1;
		return 1;
	}

	return 0;
}


static int pcb_find_addobj(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	PCB_DFLAG_SET(&obj->Flags, ctx->mark);
	vtp0_append(&ctx->open, obj);

	if (pcb_find_found(ctx, obj, arrived_from) != 0) {
		ctx->aborted = 1;
		return 1;
	}
	return 0;
}

static void int_conn(pcb_find_t *ctx, pcb_any_obj_t *from_)
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
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)padstack, from_) != 0)
				return;
	}
	PCB_END_LOOP;

	PCB_LINE_COPPER_LOOP(s->data);
	{
		if ((line != from) && (line->term != NULL) && (line->intconn == ic) && (!(PCB_DFLAG_TEST(&(line->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)line, from_) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_COPPER_LOOP(s->data);
	{
		if ((arc != from) && (arc->term != NULL) && (arc->intconn == ic) && (!(PCB_DFLAG_TEST(&(arc->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)arc, from_) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

	PCB_POLY_COPPER_LOOP(s->data);
	{
		if ((polygon != from) && (polygon->term != NULL) && (polygon->intconn == ic) && (!(PCB_DFLAG_TEST(&(polygon->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)polygon, from_) != 0)
				return;
	}
	PCB_ENDALL_LOOP;

TODO("find: no find through text yet")
#if 0
	PCB_TEXT_COPPER_LOOP(s->data);
	{
		if ((text != from) && (text->term != NULL) && (text->intconn == ic) && (!(PCB_DFLAG_TEST(&(text->Flags), ctx->mark))))
			if (pcb_find_addobj(ctx, (pcb_any_obj_t *)text, from_) != 0)
				return;
	}
	PCB_ENDALL_LOOP;
#endif
}

TODO("find: remove the undef once the old API is gone")
#undef INOCN
#define INOCN(a,b) int_noconn((pcb_any_obj_t *)a, (pcb_any_obj_t *)b)

#define PCB_FIND_CHECK(ctx, curr, obj) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!(PCB_DFLAG_TEST(&(__obj__->Flags), ctx->mark))) { \
			if (!INOCN(curr, obj) && (pcb_intersect_obj_obj(curr, __obj__))) {\
				if (pcb_find_addobj(ctx, __obj__, curr) != 0) return; \
				if ((__obj__->term != NULL) && (!ctx->ignore_intconn) && (__obj__->intconn > 0)) \
					int_conn(ctx, __obj__); \
			} \
		} \
	} while(0)

void pcb_find_on_layer(pcb_find_t *ctx, pcb_layer_t *l, pcb_any_obj_t *curr, pcb_rtree_box_t *sb)
{
	pcb_rtree_it_t it;
	pcb_box_t *n;

	if (l->line_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->line_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n);
		pcb_r_end(&it);
	}

	if (l->arc_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->arc_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n);
		pcb_r_end(&it);
	}

	if (l->polygon_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->polygon_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n);
		pcb_r_end(&it);
	}

	if (l->text_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->text_tree, sb); n != NULL; n = pcb_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n);
		pcb_r_end(&it);
	}
}

void pcb_find_on_layergrp(pcb_find_t *ctx, pcb_layergrp_t *g, pcb_any_obj_t *curr, pcb_rtree_box_t *sb)
{
	int n;
	if (g == NULL)
		return;
	for(n = 0; n < g->len; n++)
		pcb_find_on_layer(ctx, &ctx->data->Layer[g->lid[n]], curr, sb);
}

static void pcb_find_rat(pcb_find_t *ctx, pcb_rat_t *rat)
{
	pcb_rtree_box_t sb;

	sb.x1 = rat->Point1.X; sb.x2 = rat->Point1.X+1;
	sb.y1 = rat->Point1.Y; sb.y2 = rat->Point1.Y+1;
	pcb_find_on_layergrp(ctx, pcb_get_layergrp(ctx->pcb, rat->group1), (pcb_any_obj_t *)rat, &sb);

	sb.x1 = rat->Point2.X; sb.x2 = rat->Point2.X+1;
	sb.y1 = rat->Point2.Y; sb.y2 = rat->Point2.Y+1;
	pcb_find_on_layergrp(ctx, pcb_get_layergrp(ctx->pcb, rat->group2), (pcb_any_obj_t *)rat, &sb);
}

static unsigned long pcb_find_exec(pcb_find_t *ctx)
{
	pcb_any_obj_t *curr;

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

		{ /* search unmkared connections: iterative approach */
			pcb_rtree_it_t it;
			pcb_box_t *n;
			pcb_rtree_box_t *sb = (pcb_rtree_box_t *)&curr->bbox_naked;

			if (PCB->Data->padstack_tree != NULL) {
				for(n = pcb_rtree_first(&it, PCB->Data->padstack_tree, sb); n != NULL; n = pcb_rtree_next(&it))
					PCB_FIND_CHECK(ctx, curr, n);
				pcb_r_end(&it);
			}

			if ((ctx->consider_rats) && (PCB->Data->rat_tree != NULL)) {
				if (PCB->Data->padstack_tree != NULL) {
					for(n = pcb_rtree_first(&it, PCB->Data->rat_tree, sb); n != NULL; n = pcb_rtree_next(&it))
						PCB_FIND_CHECK(ctx, curr, n);
					pcb_r_end(&it);
				}
			}

			if (curr->type == PCB_OBJ_PSTK) {
				int li;
				pcb_layer_t *l;
				if ((!ctx->stay_layergrp) || (ctx->start_layergrp == NULL)) {
					for(li = 0, l = ctx->data->Layer; li < ctx->data->LayerN; li++,l++) {
						if (!ctx->allow_noncopper) {
							TODO("find.c: implement this");
						}
						if (pcb_pstk_shape_at_(ctx->pcb, (pcb_pstk_t *)curr, l, 0))
							pcb_find_on_layer(ctx, l, curr, sb);
					}
				}
				else {
					for(li = 0; li < ctx->start_layergrp->len; li++) {
						l = pcb_get_layer(ctx->data, ctx->start_layergrp->lid[li]);
						if (l != NULL)
							pcb_find_on_layer(ctx, l, curr, sb);
					}
				}
			}
			else if (curr->type == PCB_OBJ_RAT) {
				pcb_find_rat(ctx, (pcb_rat_t *)curr);
			}
			else {
				/* layer objects need to be checked against same layer objects only */
				assert(curr->parent_type == PCB_PARENT_LAYER);
				pcb_find_on_layer(ctx, curr->parent.layer, curr, sb);
			}
		}
	}

	if (ctx->flag_set_undoable)
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
	pcb_find_addobj(ctx, from, NULL); /* add the starting object with no 'arrived_from' */
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

