/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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
#include <genht/ht_utils.h>
#include "find.h"
#include "undo.h"
#include "obj_subc_parent.h"

const pcb_find_t pcb_find0_, *pcb_find0 = &pcb_find0_;

#include "find_geo.c"
#include "find_any_isect.c"

/* Multimark flags for a given object. This struct is used to store
   partial marks for objects that have multiple independently connected
   components: when such an object has the mark set that only means at
   least one component is found. But then which components are found needs
   to be looked up in an object-type-specific way from this struct. Multiple
   component objects:
    - padstacks: each (copper) layer may have a shape that is connected or not
                 (through a hole/slot) to others shapes; there can be only one
                 group of connections realized by the hole/slot and zero or
                 more independent shapes
    - text: each drawing primitive should be a separate component (not yet implemented)
    - poly: if fullpoly is on, each island is a separate component (not yet implemented)
*/
typedef union pcb_find_mm_s {
	unsigned char layers[(PCB_MAX_LAYER / 8)+1]; /* for pstk */
} pcb_find_mm_t;

/* Retrieve (or allocate) the multimark structure for a given object, storing
   it in a cache in ctx. Also init the cache as needed */
RND_INLINE pcb_find_mm_t *pcb_find_get_mm(pcb_find_t *ctx, pcb_any_obj_t *obj)
{
	pcb_find_mm_t *mm;

	if (!ctx->multimark_inited) {
		htpp_init(&ctx->multimark, ptrhash, ptrkeyeq);
		ctx->multimark_inited = 1;
		mm = NULL;
	}
	else
		mm = htpp_get(&ctx->multimark, obj);

	if (mm != NULL)
		return mm;

	mm = calloc(sizeof(pcb_find_mm_t), 1);
	htpp_set(&ctx->multimark, obj, mm);
	return mm;
}

/* Padstack: take all layers that are connected by a hole/slot and mark all
   them. Useful when any of them gets marked, this is how the mark propagates
   through the plated hole/slot */
RND_INLINE void mark_all_hole_connections(pcb_find_t *ctx, pcb_pstk_t *ps, pcb_find_mm_t *mm)
{
	rnd_layer_id_t lid;

	for(lid = 0; lid < ctx->pcb->Data->LayerN; lid++) {
		pcb_layer_t *ly = &ctx->pcb->Data->Layer[lid];
		pcb_pstk_shape_t *shp = pcb_pstk_shape_at(ctx->pcb, ps, ly);
		if ((shp != NULL) && shp->hconn)
			mm->layers[lid / 8] |= (1 << (lid % 8));
	}
}

/* Set mark on components of a padstak: on the component that's on the same
   layer as arrived_from, and any hole/slot connected other shapes */
RND_INLINE void pcb_find_mark_set_pstk(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	pcb_find_mm_t *mm;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto((pcb_pstk_t *)obj);
	pcb_layer_t *ly;
	pcb_data_t *data;
	rnd_layer_id_t lid;
	pcb_pstk_shape_t *shp;

	/* do anything only if there are disjoint copper shapes */
	if ((proto == NULL) || (proto->all_copper_connd && proto->hplated) || ((pcb_pstk_t *)obj)->term)
		return;

	mm = pcb_find_get_mm(ctx, obj);
	if (arrived_from != NULL) {
		switch(arrived_from->type) {
			case PCB_OBJ_LINE:
			case PCB_OBJ_ARC:
			case PCB_OBJ_TEXT:
			case PCB_OBJ_POLY:
			case PCB_OBJ_GFX:
				/* single layer */
				ly = arrived_from->parent.layer;
				assert(arrived_from->parent_type == PCB_PARENT_LAYER);
				data = ly->parent.data;
				lid = ly - data->Layer;
				if ((lid < 0) || (lid >= PCB_MAX_LAYER)) {
					assert(!"pcb_find_mark_set: invalid layer to arrive from");
					goto fallback1;
				}
				mm->layers[lid / 8] |= (1 << (lid % 8));

				/* propagate the mark if the shape is connected to the hole/slot
				   (hconn) and the hole/slot is 
				   plated */
				if ((ctx->pcb != NULL) && (proto->hplated)) {
					shp = pcb_pstk_shape_at(ctx->pcb, (pcb_pstk_t *)obj, ly);
					if (shp->hconn)
						mark_all_hole_connections(ctx, (pcb_pstk_t *)obj, mm);
				}
				break;
			case PCB_OBJ_PSTK:
				if (arrived_from != obj)
					rnd_message(RND_MSG_ERROR, "BUG: disjoint padstack overlap with disjoint padstack - not handled correctly\nPlease report htis bug with your board file!\n");
				goto fallback1;
			default:
				assert(!"pcb_find_mark_set: invalid arrived_from object type");
				goto fallback1;
		}
	}
	else {
		/* coming from NULL means starting object - mark all layers found */
		fallback1:;
		memset(&mm->layers, 0xff, sizeof(mm->layers));
	}
}


/* Set found mark on an object. If arrived_from is NULL, obj is a starting
   point for the search */
RND_INLINE void pcb_find_mark_set(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	PCB_DFLAG_SET(&obj->Flags, ctx->mark);
	switch(obj->type) {
		case PCB_OBJ_PSTK: pcb_find_mark_set_pstk(ctx, obj, arrived_from); break;
		default: break;
	}
}

/* Return whether pstk is marked on the layer(s) specified by arrived_from.
   If arrived_from is NULL, obj is a starting object and is always found. */
RND_INLINE int pcb_find_mark_get_pstk(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	pcb_find_mm_t *mm;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto((pcb_pstk_t *)obj);
	pcb_layer_t *ly;
	pcb_data_t *data;
	rnd_layer_id_t lid;

	/* do anything only if there are disjoint copper shapes */
	if ((proto == NULL) || (proto->all_copper_connd && proto->hplated) || ((pcb_pstk_t *)obj)->term)
		return 1;

	mm = pcb_find_get_mm(ctx, obj);
	if (arrived_from != NULL) {
		switch(arrived_from->type) {
			case PCB_OBJ_LINE:
			case PCB_OBJ_ARC:
			case PCB_OBJ_TEXT:
			case PCB_OBJ_POLY:
			case PCB_OBJ_GFX:
				/* single layer */
				ly = arrived_from->parent.layer;
				assert(arrived_from->parent_type == PCB_PARENT_LAYER);
				data = ly->parent.data;
				lid = ly - data->Layer;
				if ((lid < 0) || (lid >= PCB_MAX_LAYER)) {
					assert(!"pcb_find_mark_get: invalid layer to arrive from");
					return 1;
				}
				return !!(mm->layers[lid / 8] & (1 << (lid % 8)));
			case PCB_OBJ_PSTK:
				if (arrived_from != obj)
					rnd_message(RND_MSG_ERROR, "BUG: disjoint padstack overlap with disjoint padstack - not handled correctly\nPlease report htis bug with your board file!\n");
				return 1;
			default:
				assert(!"pcb_find_mark_get: invalid arrived_from object type");
				return 1;
		}
	}
	/* coming from NULL means starting object - assume all layers affected */
	return 1;
}

/* a variant of pcb_find_mark_get_pstk() where the target layer is explicitly
   named instead of using an arrived_from object. ly can not be NULL */
RND_INLINE int pcb_find_mark_get_pstk_on_layer(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_layer_t *ly)
{
	pcb_find_mm_t *mm;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto((pcb_pstk_t *)obj);
	pcb_data_t *data;
	rnd_layer_id_t lid;

	/* do anything only if there are disjoint copper shapes */
	if ((proto == NULL) || (proto->all_copper_connd && proto->hplated) || ((pcb_pstk_t *)obj)->term)
		return 1;

	mm = pcb_find_get_mm(ctx, obj);

	data = ly->parent.data;
	lid = ly - data->Layer;
	if ((lid < 0) || (lid >= PCB_MAX_LAYER)) {
		assert(!"pcb_find_mark_get: invalid layer to arrive from");
		return 1;
	}
	return !!(mm->layers[lid / 8] & (1 << (lid % 8)));
}

/* Return whether object is marked on any component related to arrived_from.
   If arrived_from is NULL, obj is a starting object and is always found. */
RND_INLINE int pcb_find_mark_get(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from)
{
	if (PCB_DFLAG_TEST(&obj->Flags, ctx->mark) == 0)
		return 0;

	switch(obj->type) {
		case PCB_OBJ_PSTK: return pcb_find_mark_get_pstk(ctx, obj, arrived_from);
		default: break;
	}

	return 1;
}

/* trickery: keeping vtp0 is reentrant and is cheaper than keeping lists,
   at least for appending. But as long as only the last item is removed,
   it's also cheap on remove! */

/* Do everything that needs to be done for an object found */
static int pcb_find_found(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	int fr;

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

	if ((ctx->found_cb != NULL) && ((fr = ctx->found_cb(ctx, obj, arrived_from, ctype)) != 0)) {
		if (fr == PCB_FIND_DROP_THREAD)
			return 0;
		ctx->aborted = 1;
		return 1;
	}

	return 0;
}

static int pcb_find_addobj(pcb_find_t *ctx, pcb_any_obj_t *obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype, int jump)
{
	pcb_find_mark_set(ctx, obj, arrived_from);

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

	/* it is okay to test for mark directly here: internal connection
	   assumes all-layer affection as it is a special case mostly used for SMD pads
	   and thru-hole pins; the flag means "found in any way" */

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
static rnd_bool int_noconn(pcb_any_obj_t *a, pcb_any_obj_t *b)
{
	pcb_subc_t *pa, *pb;

	/* cheap test: they need to have valid and matching intnoconn */
	if ((a->intnoconn == 0) || (a->intnoconn != b->intnoconn))
		return rnd_false;

	/* expensive tests: they need to be in the same subc */
	pa = pcb_obj_parent_subc(a);
	if (pa == NULL)
		return rnd_false;

	pb = pcb_obj_parent_subc(b);

	return (pa == pb);
}

#define INOCONN(a,b) int_noconn((pcb_any_obj_t *)a, (pcb_any_obj_t *)b)

#define PCB_FIND_CHECK(ctx, curr, obj, ctype, retstmt) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!pcb_find_mark_get(ctx, __obj__, (curr))) { \
			if (!INOCONN(curr, obj) && (pcb_intersect_obj_obj(ctx, curr, __obj__))) {\
				if (pcb_find_addobj(ctx, __obj__, curr, ctype, 1) != 0) { retstmt; } \
				if ((__obj__->term != NULL) && (!ctx->ignore_intconn) && (__obj__->intconn > 0)) \
					find_int_conn(ctx, __obj__); \
			} \
		} \
	} while(0)

#define PCB_FIND_CHECK_RAT(ctx, curr, obj, ctype, retstmt) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)obj; \
		if (!pcb_find_mark_get(ctx, __obj__, (curr))) { \
			if (!INOCONN(curr, obj) && (pcb_intersect_obj_obj(ctx, curr, __obj__))) {\
				if (pcb_find_addobj(ctx, __obj__, curr, ctype, ctx->consider_rats) != 0) { retstmt; } \
				if ((__obj__->term != NULL) && (!ctx->ignore_intconn) && (__obj__->intconn > 0)) \
					find_int_conn(ctx, __obj__); \
			} \
		} \
	} while(0)

void pcb_find_on_layer(pcb_find_t *ctx, pcb_layer_t *l, pcb_any_obj_t *curr, rnd_rtree_box_t *sb, pcb_found_conn_type_t ctype)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;

	if (l->line_tree != NULL)
		for(n = rnd_rtree_first(&it, l->line_tree, sb); n != NULL; n = rnd_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);

	if (l->arc_tree != NULL)
		for(n = rnd_rtree_first(&it, l->arc_tree, sb); n != NULL; n = rnd_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);

	if (l->polygon_tree != NULL)
		for(n = rnd_rtree_first(&it, l->polygon_tree, sb); n != NULL; n = rnd_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);

	if (l->text_tree != NULL)
		for(n = rnd_rtree_first(&it, l->text_tree, sb); n != NULL; n = rnd_rtree_next(&it))
			PCB_FIND_CHECK(ctx, curr, n, ctype, return);
}

void pcb_find_on_layergrp(pcb_find_t *ctx, pcb_layergrp_t *g, pcb_any_obj_t *curr, rnd_rtree_box_t *sb, pcb_found_conn_type_t ctype)
{
	int n;
	if (g == NULL)
		return;
	for(n = 0; n < g->len; n++)
		pcb_find_on_layer(ctx, &ctx->data->Layer[g->lid[n]], curr, sb, ctype);
}

static void pcb_find_rat(pcb_find_t *ctx, pcb_rat_t *rat)
{
	rnd_rtree_box_t sb;

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
			rnd_layergrp_id_t gid = pcb_layer_get_group_(curr->parent.layer);
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
			rnd_rtree_it_t it;
			rnd_box_t *n;
			rnd_rtree_box_t *sb = (rnd_rtree_box_t *)&curr->bbox_naked;

			if (PCB->Data->padstack_tree != NULL)
				for(n = rnd_rtree_first(&it, PCB->Data->padstack_tree, sb); n != NULL; n = rnd_rtree_next(&it))
					PCB_FIND_CHECK(ctx, curr, n, ctype, return ctx->nfound);

			if ((ctx->consider_rats || ctx->only_mark_rats) && (PCB->Data->rat_tree != NULL))
				if (PCB->Data->padstack_tree != NULL)
					for(n = rnd_rtree_first(&it, PCB->Data->rat_tree, sb); n != NULL; n = rnd_rtree_next(&it))
						PCB_FIND_CHECK_RAT(ctx, curr, n, PCB_FCT_RAT, return ctx->nfound);

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
						if (!pcb_find_mark_get_pstk_on_layer(ctx, curr, l))
							continue; /* padstack is not really found on this specific layer (it is marked found on other layers) */
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
				if (g != NULL) /* g==NULL for inbound subc layers */
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


unsigned long pcb_find_from_xy(pcb_find_t *ctx, pcb_data_t *data, rnd_coord_t x, rnd_coord_t y)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *obj;
	int type;

	type = pcb_search_obj_by_location(PCB_LOOKUP_FIRST, &ptr1, &ptr2, &ptr3, x, y, PCB_SLOP * rnd_pixel_slop);
	if (type == PCB_OBJ_VOID)
		type = pcb_search_obj_by_location(PCB_LOOKUP_MORE, &ptr1, &ptr2, &ptr3, x, y, PCB_SLOP * rnd_pixel_slop);

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
	if (ctx->multimark_inited) {
		genht_uninit_deep(htpp, &ctx->multimark, {
			free(htent->value);
		}); \
		ctx->multimark_inited = 0;
	}
	if (ctx->list_found)
		vtp0_uninit(&ctx->found);
	vtp0_uninit(&ctx->open);
	pcb_dynflag_free(ctx->mark);
	ctx->in_use = 0;
}

