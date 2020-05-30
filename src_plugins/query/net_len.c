/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <genht/htpp.h>
#include <genht/hash.h>
#include <assert.h>

#include "board.h"
#include "find.h"
#include "netlist.h"
#include "query_exec.h"
#include "net_len.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_pstk.h"

#include <librnd/core/error.h>

#define PCB dontuse

/* evaluates to true if obj was marked on list (fa or fb) */
#define IS_FOUND(obj, list) (PCB_DFLAG_TEST(&(obj->Flags), ctx->list.mark))

/*** cached object -> net length-segment ***/

/* Whenever a net tag of an object is queried:
   1. look at the cache - if the object is already mapped, return result from
      cache (ec->obj2lenseg)
   2. else map the whole network segment starting from obj, using galvanic
      connections only, up to the first junction; remember every object
      being in the segment (using ec->tmplst)
   3. assign the result struct to all objects on the net in
      the cache (ec->obj2lenseg) so subsequent calls on any segment object
      will bail out in step 1
*/

typedef struct {
	pcb_qry_exec_t *ec;
	pcb_any_obj_t *start, *best;
	pcb_qry_netseg_len_t *seglen;
} parent_net_len_t;

/* set a cc bit for an object; returns 0 on success, 1 if the bit was already set */
static int set_cc(parent_net_len_t *ctx, pcb_any_obj_t *o, int val)
{
	htpi_entry_t *e = htpi_getentry(&ctx->ec->obj2lenseg_cc, o);

rnd_trace("set_cc: #%ld %d pstk: %d\n", o->ID, val, (o->type == PCB_OBJ_PSTK));

	if (e == NULL) {
		htpi_set(&ctx->ec->obj2lenseg_cc, o, val);
		return 0;
	}

	if (o->type == PCB_OBJ_PSTK) { /* padstacks have only one end */
		e->value++;
		if (e->value > 2)
			return 1;
	}
	else {
		if ((e->value & val) == val)
			return 1;

		e->value |= val;
	}
	return 0;
}

static int obj_ends(pcb_any_obj_t *o, rnd_coord_t ends[4], rnd_coord_t *thick)
{
	pcb_arc_t *a  = (pcb_arc_t *)o;
	pcb_line_t *l = (pcb_line_t *)o;
	pcb_pstk_t *p = (pcb_pstk_t *)o;

	switch(o->type) {
		case PCB_OBJ_ARC:
			pcb_arc_get_end(a, 0, &ends[0], &ends[1]);
			pcb_arc_get_end(a, 1, &ends[2], &ends[3]);
			*thick = a->Thickness;
			return 0;

		case PCB_OBJ_LINE:
			ends[0] = l->Point1.X; ends[1] = l->Point1.Y;
			ends[2] = l->Point2.X; ends[3] = l->Point2.Y;
			*thick = l->Thickness;
			return 0;

		case PCB_OBJ_PSTK:
			ends[0] = p->x; ends[1]  = p->y;
			ends[2] = ends[3] = RND_COORD_MAX;
			/* thickness estimation: smaller bbox size */
			*thick = RND_MIN(p->bbox_naked.X2-p->bbox_naked.X1, p->bbox_naked.Y2-p->bbox_naked.Y1);
			return 0;

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_RAT: case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP:
			return -1;
	}
	return -1;
}

static rnd_coord_t obj_len(pcb_any_obj_t *o)
{
	switch(o->type) {
		case PCB_OBJ_ARC:  return pcb_arc_length((pcb_arc_t *)o);
		case PCB_OBJ_LINE: return pcb_line_length((pcb_line_t *)o);
		case PCB_OBJ_PSTK: return 0;

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_RAT: case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP:
			return -1;
	}
	return -1;
}

static int endp_match(parent_net_len_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, double *dist)
{
	rnd_coord_t new_end[4], new_th, old_end[4], old_th;
	double th, d2;
	int n, conns = 0, bad = 0;

	if (obj_ends(new_obj, new_end, &new_th) != 0) {
		rnd_trace("NSL: badobj: #%ld\n", new_obj->ID);
		return -1;
	}
	obj_ends(arrived_from, old_end, &old_th);
	th = RND_MIN(new_th, old_th) * 4 / 5;
	th = th * th;

	for(n = 0; n < 4; n+=2) {
		d2 = rnd_distance2(old_end[0], old_end[1], new_end[0+n], new_end[1+n]);
		if (d2 < th) {
			if (dist != NULL) {
				*dist = sqrt(d2);
				return 0;
			}
			if (set_cc(ctx, arrived_from, 1) != 0) {
				rnd_trace("NSL: junction at end: 0 of #%ld at %$$mm;%$$mm\n", arrived_from->ID, old_end[0], old_end[1]);
				bad = 1;
			}
			if (set_cc(ctx, new_obj, n == 0 ? 1 : 2) != 0) {
				rnd_trace("NSL: junction at end: %d of #%ld at %$$mm;%$$mm\n", n, new_obj->ID, new_end[0+n], new_end[1+n]);
				bad = 1;
			}
			if (bad)
				return -1;
			conns++;
		}

		d2 = rnd_distance2(old_end[2], old_end[3], new_end[0+n], new_end[1+n]);
		if (d2 < th) {
			if (dist != NULL) {
				*dist = sqrt(d2);
				return 0;
			}
			if (set_cc(ctx, arrived_from, 2) != 0) {
				rnd_trace("NSL: junction at end: 0 of #%ld at %$$mm;%$$mm\n", arrived_from->ID, old_end[2], old_end[3]);
				bad = 1;
			}
			if (set_cc(ctx, new_obj, n == 0 ? 1 : 2) != 0) {
				rnd_trace("NSL: junction at end: %d of #%ld at %$$mm;%$$mm\n", n, new_obj->ID, new_end[0+n], new_end[1+n]);
				bad = 1;
			}
			if (bad)
				return -1;
			conns++;
		}
	}

	if (conns == 0) {
		if (dist == NULL)
			rnd_trace("NSL: junction at: middle of #%ld vs. #%ld\n", new_obj->ID, arrived_from->ID);
		return -2;
	}
	if (conns > 1) {
		if (dist == NULL)
			rnd_trace("NSL: junction at: #%ld overlap with #%ld\n", new_obj->ID, arrived_from->ID);
		return -2;
	}

	return 0;
}

static void remove_offender_from_open(pcb_find_t *fctx, pcb_any_obj_t *offender)
{
	long n;

	/* remove anything from the open list has contect with the offending object,
	   so we are not going to follow a thread that is also affected */
	for(n = 0; n < fctx->open.used; n++) {
		pcb_any_obj_t *o = fctx->open.array[n];

		if (pcb_intersect_obj_obj(fctx, offender, o)) {
			rnd_trace(" REMOVE1: #%ld\n", o->ID);
			vtp0_remove(&fctx->open, n, 1);
			n--;
		}
	}
}

/* also remove affected objects from the found list */
static void remove_offender_from_closed(pcb_find_t *fctx, pcb_any_obj_t *offender, pcb_any_obj_t *dont_remove)
{
	long n;
	parent_net_len_t *ctx = fctx->user_data;

	for(n = 0; n < ctx->ec->tmplst.used; n++) {
		pcb_any_obj_t *o = ctx->ec->tmplst.array[n];
		if ((o != dont_remove) && (pcb_intersect_obj_obj(fctx, offender, o))) {
			rnd_trace(" REMOVE2: #%ld\n", o->ID);
			vtp0_remove(&ctx->ec->tmplst, n, 1);
			n--;
		}
	}
}


static int parent_net_len_found_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	parent_net_len_t *ctx = fctx->user_data;

rnd_trace("from: #%ld to #%ld\n", arrived_from == NULL ? 0 : arrived_from->ID, new_obj->ID);

	if (arrived_from != NULL) {
		int coll = endp_match(ctx, new_obj, arrived_from, NULL);
		if (coll != 0) {
			ctx->seglen->has_junction = 1;
/*rnd_trace("BUMP new=#%ld from=#%ld last=#%ld coll=%d\n", new_obj->ID, arrived_from->ID, ((pcb_any_obj_t *)ctx->ec->tmplst.array[ctx->ec->tmplst.used-1])->ID, coll);*/
			

			if (coll == -1) {
				/* remove from new_obj: keeps the hub object we are at (only endpoint
				   of it is affected*/
				remove_offender_from_open(fctx, new_obj);
				remove_offender_from_closed(fctx, new_obj, arrived_from);
			}
			else if (coll == -2) {
				long n;
				int cnt;

				/* remove from arrived_from: removes the hub object */
				remove_offender_from_open(fctx, arrived_from);
				remove_offender_from_closed(fctx, new_obj, arrived_from);


				/* plus remove the hub itself from found if it's not the startin object  */
				if (arrived_from != ctx->start) {
					for(n = 0; n < ctx->ec->tmplst.used; n++) {
						pcb_any_obj_t *o = ctx->ec->tmplst.array[n];
						if (o == arrived_from) {
							vtp0_remove(&ctx->ec->tmplst, n, 1);
							break;
						}
					}
				}

				/* plus remove anything but the first object that is in contact with it
				   (this removes started outgoing threads but not the object we once
				   came from to reach arrived_from) */
				for(n = 0, cnt = 0; n < ctx->ec->tmplst.used; n++) {
					pcb_any_obj_t *o = ctx->ec->tmplst.array[n];
					if (pcb_intersect_obj_obj(fctx, o, arrived_from)) {
						cnt++;
						if (cnt > 1) {
							vtp0_remove(&ctx->ec->tmplst, n, 1);
							n--;
						}
					}
				}

			}
			return PCB_FIND_DROP_THREAD;
		}
	}

	vtp0_append(&ctx->ec->tmplst, new_obj);

	if ((ctx->best == NULL) || (new_obj < ctx->best))
		ctx->best = new_obj;

	return 0;
}

/* returns 1 if o1 and o2 are not ordered; ordered means left->right (x+)
   or top->bottom (y+) direction between o1 and o2 center */
static int seg_dir_reversed(pcb_any_obj_t *o1, pcb_any_obj_t *o2)
{
	rnd_coord_t x1, y1, x2, y2, dx, dy, adx, ady;

	pcb_obj_center(o1, &x1, &y1);
	pcb_obj_center(o2, &x2, &y2);
	dx = x2 - x1;
	dy = y2 - y1;

	adx = RND_ABS(dx);
	ady = RND_ABS(dy);

	/* determine main direction */
	if (adx > (ady+ady/4)) /* horizontal */
		return dx < 0;
	if (ady > (adx+adx/4)) /* vertical */
		return dy < 0;

	/* diagonal */
	return dx < 0;
}

static pcb_layergrp_t *get_obj_grp(pcb_board_t *pcb, pcb_any_obj_t *o)
{
	switch(o->type) {
		case PCB_OBJ_ARC:
		case PCB_OBJ_LINE:
			/* plain layer obj */
			assert(o->parent_type == PCB_PARENT_LAYER);
			return pcb_get_layergrp(pcb, pcb_layer_get_group_(o->parent.layer));

		case PCB_OBJ_PSTK:
			
			return NULL;

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_RAT: case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP:
			return NULL;
	}
	return NULL;
}

RND_INLINE pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg_(pcb_qry_exec_t *ec, pcb_any_obj_t *from)
{
	pcb_find_t fctx;
	parent_net_len_t ctx;
	long n, revp = -1;
	pcb_layergrp_t *lg;

	assert(ec->tmplst.used == 0); /* temp list must be empty so we are nto getting void * ptrs to anything else than we find */

	ctx.ec = ec;
	ctx.start = from;
	ctx.best = NULL;
	ctx.seglen = calloc(sizeof(pcb_qry_netseg_len_t), 1);
	vtp0_append(&ec->obj2lenseg_free, ctx.seglen);

	memset(&fctx, 0, sizeof(fctx));
	fctx.user_data = &ctx;
	fctx.found_cb = parent_net_len_found_cb;
	pcb_find_from_obj(&fctx, ec->pcb->Data, from);
	pcb_find_free(&fctx);

	for(n = 0; n < ec->tmplst.used; n++) {
		pcb_any_obj_t *o = ec->tmplst.array[n];

		htpp_set(&ec->obj2lenseg, o, ctx.seglen);
		ctx.seglen->len += obj_len(o);
		vtp0_append(&ctx.seglen->objs, o);

		if (n > 0) {
			double offs = 0;
			if (endp_match(&ctx, o, (pcb_any_obj_t *)ec->tmplst.array[n-1], &offs) < 0) {
				/* remember the first place in the list where endpoints don't meet - this
				   must be the place where the search started in the other direction from
				   the starting object - see below at "reversing" */
				if (revp > 0)
					rnd_message(RND_MSG_ERROR, "Internal error: pcb_qry_parent_net_lenseg_(): multiple revp's\n");
				else
					revp = n;
			}
/*			rnd_trace("offs: #%ld #%ld: %$mm (%f) %d\n", ((pcb_any_obj_t *)ec->tmplst.array[n-1])->ID, o->ID, (rnd_coord_t)(offs), offs);*/
			ctx.seglen->len += offs;
		}
	}

	/* reversing: take the following thread of lines by ID:
	   #1 #5 #8 #11 #14
	   A search starting from 8 will yield the following raw list:
	   #8 #5 #1  #11 #14
	   If there's no loop or other error, #1 and #11 won't have a common
	   endpoint and thus revp will be set to 3, which is #11's IDX.
	   When that happens, we need to reverse the order of items before revp to
	   get:
	   #1 #5 #8  #11 #14
	*/
	if (revp > 0) {
		for(n = 0; n < revp/2; n++)
			rnd_swap(void *, ctx.seglen->objs.array[n], ctx.seglen->objs.array[revp-n-1]);
	}

	/* make sure objects are always returned in the same order for the same net,
	   no matter which object was the start of the mapping */
	if ((ctx.seglen->objs.used > 1) && (seg_dir_reversed(ctx.seglen->objs.array[0], ctx.seglen->objs.array[ctx.seglen->objs.used-1]))) {
		for(n = 0; n < ctx.seglen->objs.used/2; n++)
			rnd_swap(void *, ctx.seglen->objs.array[n], ctx.seglen->objs.array[ctx.seglen->objs.used-n-1]);
	}

	/* count vias (layer jumps) */
	lg = NULL;
	ctx.seglen->num_vias = 0;
	for(n = 0; n < ctx.seglen->objs.used; n++) {
		pcb_any_obj_t *o = ctx.seglen->objs.array[n];
		pcb_layergrp_t *g = get_obj_grp(ec->pcb, o);

		if (g == NULL)
			continue; /* multi-layer padstacks */
		if ((lg != NULL) && (g != lg)) { /* layer group change other than the initial layer group */
			ctx.seglen->num_vias++;
			TODO("Add via length between the two layer groups lg and g to the total net length?");
		}
		lg = g;
	}


	ec->tmplst.used = 0;
	return ctx.seglen;
}


pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from)
{
	pcb_qry_netseg_len_t *res;

	if ((from->type != PCB_OBJ_LINE) && (from->type != PCB_OBJ_ARC) && (from->type != PCB_OBJ_PSTK))
		return NULL;

	if (!ec->obj2lenseg_inited) {
		htpp_init(&ec->obj2lenseg, ptrhash, ptrkeyeq);
		htpi_init(&ec->obj2lenseg_cc, ptrhash, ptrkeyeq);
		vtp0_init(&ec->obj2lenseg_free);
		ec->obj2lenseg_inited = 1;
		res = NULL;
	}
	else
		res = htpp_get(&ec->obj2lenseg, from);

	if (res == NULL)
		res = pcb_qry_parent_net_lenseg_(ec, from);

	return res;
}


void pcb_qry_lenseg_free_fields(pcb_qry_netseg_len_t *ns)
{
	vtp0_uninit(&ns->objs);
}
