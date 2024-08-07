/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020,2021 Tibor 'Igor2' Palinkas
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
#include "data.h"
#include "find.h"
#include "netlist.h"
#include "query_exec.h"
#include "net_len.h"
#include "search.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_pstk.h"
#include "obj_term.h"

#include <librnd/core/error.h>
#include <librnd/core/actions.h>

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

#define CC_IGNORE -1024

/* set a cc bit for an object; returns 0 on success, 1 if the bit was already set */
static int set_cc(parent_net_len_t *ctx, pcb_any_obj_t *o, int val)
{
	htpi_entry_t *e = htpi_getentry(&ctx->ec->obj2lenseg_cc, o);

rnd_trace("set_cc: #%ld %d pstk: %d\n", o->ID, val, (o->type == PCB_OBJ_PSTK));

	if (e == NULL) {
		htpi_set(&ctx->ec->obj2lenseg_cc, o, val);
		return 0;
	}

	if (e->value == CC_IGNORE)
		return 0;

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

		case PCB_OBJ_RAT:
			/* not called if find was not configured so */
			ends[0] = l->Point1.X; ends[1] = l->Point1.Y;
			ends[2] = l->Point2.X; ends[3] = l->Point2.Y;
			*thick = 100; /* will be very close to the center but becauese of *4/5 it can't be 0 or 1 */
			return 0;

		case PCB_OBJ_PSTK:
			ends[0] = p->x; ends[1]  = p->y;
			ends[2] = ends[3] = RND_COORD_MAX;
			/* thickness estimation: smaller bbox size */
			*thick = RND_MIN(p->bbox_naked.X2-p->bbox_naked.X1, p->bbox_naked.Y2-p->bbox_naked.Y1);
			return 0;

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
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

		case PCB_OBJ_RAT:
		/* not called if find was not configured so */
			return pcb_line_length((pcb_line_t *)o);

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP:
			return -1;
	}
	return -1;
}

/* Returns whether two objects are a fully overlapping padstack-trace combination */
static int fully_under_padstack(pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_any_obj_t **trace_out, pcb_any_obj_t **pstk_out)
{
	pcb_any_obj_t *pstk, *trace;
	rnd_coord_t te[4], th;

	/* figure who is who, ignore non-pstk cases */
	if (arrived_from->type == PCB_OBJ_PSTK) {
		pstk = arrived_from;
		trace = new_obj;
	}
	else if (new_obj->type == PCB_OBJ_PSTK) {
		pstk = new_obj;
		trace = arrived_from;
	}
	else
		return 0;

	if (obj_ends(trace, te, &th) != 0)
		return 0;

	if (trace_out != NULL)
		*trace_out = trace;
	if (pstk_out != NULL)
		*pstk_out = pstk;

	/* fully under if both endpoints are under */
	return pcb_is_point_on_obj(te[0], te[1], th/4, pstk) && pcb_is_point_on_obj(te[2], te[3], th/4, pstk);
}

/* Return the object (either oa or ob) which has the junction on it: that is,
   one endpoint of the other object falls on it. Returns NULL on error */
static pcb_any_obj_t *junction_hub(pcb_any_obj_t *oa, rnd_coord_t ea[4], pcb_any_obj_t *ob, rnd_coord_t eb[4], double radius)
{
	if (pcb_is_point_on_obj(ea[0], ea[1], radius, ob)) return ob;
	if (pcb_is_point_on_obj(ea[2], ea[3], radius, ob)) return ob;
	if (pcb_is_point_on_obj(eb[0], eb[1], radius, oa)) return oa;
	if (pcb_is_point_on_obj(eb[2], eb[3], radius, oa)) return oa;
	return NULL;
}

static int endp_match(parent_net_len_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, double *dist, pcb_any_obj_t **hub_out)
{
	rnd_coord_t new_end[4], new_th, old_end[4], old_th;
	double thr, th, d2;
	int n, conns = 0, bad = 0, has_pstk = 0;

	*hub_out = NULL;

	if (obj_ends(new_obj, new_end, &new_th) != 0) {
		ctx->seglen->has_nontrace = 1;
		rnd_trace("NSL: badobj: #%ld\n", new_obj->ID);
		return -1;
	}
	obj_ends(arrived_from, old_end, &old_th);
	thr = RND_MIN(new_th, old_th);

	/* special case: a padstack should accept anything that it touches because
	   of fully overlapping short doglegs are ignored */
	if (arrived_from->type == PCB_OBJ_PSTK) {
		has_pstk = 1;
		thr = RND_MAX(thr, old_th);
	}
	if (new_obj->type == PCB_OBJ_PSTK) {
		has_pstk = 1;
		thr = RND_MAX(thr, new_th);
	}

	if (!has_pstk)
		thr = thr * 4 / 5;
	th = thr * thr;

	for(n = 0; n < 4; n+=2) {
		/* Ignore missing endpoints: padstacks have only one endpoint, so the other
		   is filled in as RND_COORD_MAX; if two padstacks are compared, this
		   makes any two of them touch unless the RND_COORD_MAX endpoint is ignored */
		if ((old_end[0] == RND_COORD_MAX) || (old_end[1] == RND_COORD_MAX))
			continue;
		if ((old_end[2] == RND_COORD_MAX) || (old_end[3] == RND_COORD_MAX))
			continue;

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
		if (dist == NULL) {
			pcb_any_obj_t *hub = junction_hub(new_obj, new_end, arrived_from, old_end, thr);
			rnd_trace("NSL: junction at: middle of #%ld (#%ld vs. #%ld)\n", hub==NULL ? 0 : hub->ID, new_obj->ID, arrived_from->ID);
			*hub_out = hub;
		}
		return -2;
	}
	if (conns > 1) {
		if (dist == NULL)
			rnd_trace("NSL: junction at: #%ld overlap with #%ld\n", new_obj->ID, arrived_from->ID);
		*hub_out = new_obj;
		return -2;
	}

	return 0;
}

static void remove_offender_from_open(pcb_find_t *fctx, pcb_any_obj_t *offender)
{
	parent_net_len_t *ctx = fctx->user_data;
	long n;

	/* remove anything from the open list that has contact with the offending object,
	   so we are not going to follow a thread that is also affected */
	for(n = 0; n < fctx->open.used; n++) {
		pcb_any_obj_t *o = fctx->open.array[n];

		if (ctx->ec->cfg_prefer_term && (o->term != NULL))
			continue;

		if (pcb_intersect_obj_obj(fctx, offender, o)) {
			rnd_trace(" REMOVE1: #%ld (because of offender #%ld)\n", o->ID, offender->ID);
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
		if (ctx->ec->cfg_prefer_term && (o->term != NULL))
			continue;
		if ((o != dont_remove) && (pcb_intersect_obj_obj(fctx, offender, o))) {
			rnd_trace(" REMOVE2: #%ld\n", o->ID);
			vtp0_remove(&ctx->ec->tmplst, n, 1);
			n--;
		}
	}
}

static void add_junction(parent_net_len_t *ctx, pcb_any_obj_t *junct, pcb_any_obj_t *seg_end)
{
	int i = 0;

/*	rnd_trace(" junction: obj=#%ld at end #%ld\n", junct->ID, seg_end->ID);*/

	/* do not add the same thing twice: keep only one junct object per seg end */
	if ((ctx->seglen->junc_at[0] == seg_end) || (ctx->seglen->junc_at[1] == seg_end)) {
		/* keep it as spare */
		i = 2;
	}
	else {
		if (ctx->seglen->junction[i] != NULL) i++;
		if (ctx->seglen->junction[i] != NULL) {
			ctx->seglen->hub = 1;
			return;
		}
	}

	ctx->seglen->junction[i] = junct;
	ctx->seglen->junc_at[i] = seg_end;
}

static int parent_net_len_found_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	parent_net_len_t *ctx = fctx->user_data;
	pcb_any_obj_t *hub_obj;
	htpi_entry_t *e = htpi_getentry(&ctx->ec->obj2lenseg_cc, new_obj);

rnd_trace("from: #%ld to #%ld\n", arrived_from == NULL ? 0 : arrived_from->ID, new_obj->ID);

	if ((e != NULL) && (e->value == CC_IGNORE)) {
		rnd_trace(" -> ignore object\n");
		return PCB_FIND_DROP_THREAD;
	}

	if (arrived_from != NULL) {
		int coll = endp_match(ctx, new_obj, arrived_from, NULL, &hub_obj);
		if (coll != 0) {
			ctx->seglen->has_junction = 1;
/*rnd_trace("BUMP new=#%ld from=#%ld last=#%ld coll=%d\n", new_obj->ID, arrived_from->ID, ((pcb_any_obj_t *)ctx->ec->tmplst.array[ctx->ec->tmplst.used-1])->ID, coll);*/
			

			if (coll == -1) {
				/* remove from new_obj: keeps the hub object we are at (only endpoint
				   of it is affected*/
				remove_offender_from_open(fctx, new_obj);
				remove_offender_from_closed(fctx, new_obj, arrived_from);
				add_junction(ctx, new_obj, arrived_from);
			}
			else if (coll == -2) {
				long n;
				int cnt;

				/* remove from arrived_from: removes the hub object */
				if (hub_obj == NULL) {
					remove_offender_from_open(fctx, arrived_from);
					ctx->seglen->has_invalid_hub = 1;
				}
				else
					remove_offender_from_open(fctx, hub_obj);

				remove_offender_from_closed(fctx, new_obj, arrived_from);


/* disabled: this seems to remove not the hub but the next, legit object */
#if 0
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
#endif

				/* plus remove anything but the first object that is in contact with it
				   (this removes started outgoing threads but not the object we once
				   came from to reach arrived_from) */
				if (hub_obj != NULL)
				for(n = 0, cnt = 0; n < ctx->ec->tmplst.used; n++) {
					pcb_any_obj_t *o = ctx->ec->tmplst.array[n];
					if (pcb_intersect_obj_obj(fctx, o, hub_obj)) {
						cnt++;
						if (cnt > 1) {
							rnd_trace(" REMOVE4: #%ld (from #%ld)\n", ((pcb_any_obj_t *)(ctx->ec->tmplst.array[n]))->ID, new_obj->ID);
							vtp0_remove(&ctx->ec->tmplst, n, 1);
							n--;
						}
						else {
							if (o != arrived_from)
								add_junction(ctx, arrived_from, o);
							else
								add_junction(ctx, new_obj, o);
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
			TODO("special case: padstack with copper only on top or bottom");
			return NULL;

		/* netlen segment breaking objects */
		case PCB_OBJ_VOID: case PCB_OBJ_POLY: case PCB_OBJ_TEXT: case PCB_OBJ_SUBC:
		case PCB_OBJ_RAT: case PCB_OBJ_GFX: case PCB_OBJ_NET: case PCB_OBJ_NET_TERM:
		case PCB_OBJ_LAYER: case PCB_OBJ_LAYERGRP:
			return NULL;
	}
	return NULL;
}

RND_INLINE pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg_(pcb_qry_exec_t *ec, pcb_any_obj_t *from, int find_rats)
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
	fctx.consider_rats = find_rats;
	pcb_find_from_obj(&fctx, ec->pcb->Data, from);
	pcb_find_free(&fctx);

	for(n = 0; n < ec->tmplst.used; n++) {
		pcb_any_obj_t *o = ec->tmplst.array[n];

		htpp_set(&ec->obj2lenseg, o, ctx.seglen);
		ctx.seglen->len += obj_len(o);
		vtp0_append(&ctx.seglen->objs, o);

		if (n > 0) {
			double offs = 0;
			pcb_any_obj_t *hub_obj;

			if (endp_match(&ctx, o, (pcb_any_obj_t *)ec->tmplst.array[n-1], &offs, &hub_obj) < 0) {
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
			rnd_coord_t vert = pcb_stack_thickness(ec->pcb, NULL, 0, lg - ec->pcb->LayerGroups.grp, 0, g - ec->pcb->LayerGroups.grp, 0, PCB_LYT_SUBSTRATE | PCB_LYT_COPPER);
			if ((vert <= 0) && (!ec->warned_missing_thickness)) {
				ec->warned_missing_thickness = 1;
				rnd_message(RND_MSG_ERROR, "Some of the substrate or copper layers are missing the 'thickness' attribute; network length does not include via length\n");
			}
			else
				ctx.seglen->len += vert;
			ctx.seglen->num_vias++;
		}
		lg = g;
	}

	/* make sure junctions are ordered the same way as the object array */
	if ((ctx.seglen->objs.used > 0) && (ctx.seglen->junc_at[0] != ctx.seglen->objs.array[0])) {
		rnd_swap(pcb_any_obj_t *, ctx.seglen->junc_at[0], ctx.seglen->junc_at[1]);
		rnd_swap(pcb_any_obj_t *, ctx.seglen->junction[0], ctx.seglen->junction[1]);
	}

	/* a hub segment is a single object with junction (sometimes in the spare field) */
	if ((ctx.seglen->objs.used == 1) && (ctx.seglen->junc_at[2] != NULL)) {
		ctx.seglen->hub = 1;
		/* recycle spare if possible */
		if (ctx.seglen->junc_at[0] == NULL) {
			ctx.seglen->junc_at[0] = ctx.seglen->junc_at[2];
			ctx.seglen->junction[0] = ctx.seglen->junction[2];
		}
		else if (ctx.seglen->junc_at[1] == NULL) {
			ctx.seglen->junc_at[1] = ctx.seglen->junc_at[2];
			ctx.seglen->junction[1] = ctx.seglen->junction[2];
		}
	}

	ec->tmplst.used = 0;
	return ctx.seglen;
}

static int mark_ps_overlap(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	pcb_any_obj_t *trace_obj;
	pcb_qry_exec_t *ec = fctx->user_data;

	if (arrived_from == NULL) /* the starting ps */
		return 0;

	if (fully_under_padstack(new_obj, arrived_from, &trace_obj, NULL)) {
		rnd_trace("*** OVERLAP: #%ld\n", trace_obj->ID);
		htpi_set(&ec->obj2lenseg_cc, trace_obj, CC_IGNORE);
	}
	return PCB_FIND_DROP_THREAD;
}

/* returns 1 if newly inited */
RND_INLINE int pcb_qry_parent_net_lenseg_init(pcb_qry_exec_t *ec)
{
	rnd_rtree_it_t it;
	pcb_any_obj_t *ps;
	pcb_find_t fctx = {0};
	rnd_rtree_box_t b;

	if (ec->obj2lenseg_inited)
		return 0;

	htpp_init(&ec->obj2lenseg, ptrhash, ptrkeyeq);
	htpi_init(&ec->obj2lenseg_cc, ptrhash, ptrkeyeq);
	vtp0_init(&ec->obj2lenseg_free);
	ec->obj2lenseg_inited = 1;

	/* find all fully overlapping padstack-trace combinations - the overlapping
	   trace objects need to be marked */
	fctx.found_cb = mark_ps_overlap;
	fctx.user_data = ec;
	b.x1 = b.y1 = -RND_COORD_MAX;
	b.x2 = b.y2 = +RND_COORD_MAX;
	if (ec->pcb->Data->padstack_tree != 0) {
		for(ps = rnd_rtree_first(&it, ec->pcb->Data->padstack_tree, &b); ps != NULL; ps = rnd_rtree_next(&it)) {
			pcb_find_from_obj(&fctx, ec->pcb->Data, ps);
			pcb_find_free(&fctx);
		}
	}

	return 1;
}

pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from)
{
	pcb_qry_netseg_len_t *res = NULL;

	if ((from->type != PCB_OBJ_LINE) && (from->type != PCB_OBJ_ARC) && (from->type != PCB_OBJ_PSTK))
		return NULL;

	if (!pcb_qry_parent_net_lenseg_init(ec))
		res = htpp_get(&ec->obj2lenseg, from);

	if (res == NULL)
		res = pcb_qry_parent_net_lenseg_(ec, from, 0);

	return res;
}


void pcb_qry_lenseg_free_fields(pcb_qry_netseg_len_t *ns)
{
	vtp0_uninit(&ns->objs);
}

pcb_qry_netseg_len_t *pcb_qry_parent_net_len_mapseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from, int find_rats)
{
	pcb_qry_parent_net_lenseg_init(ec);
	return pcb_qry_parent_net_lenseg_(ec, from, find_rats);
}

const char pcb_acts_QueryCalcNetLen[] = "QueryCalcNetLen(netname)";
const char pcb_acth_QueryCalcNetLen[] = "Calculates the network length by netname; returns an error message string or a positive coord with the length";
fgw_error_t pcb_act_QueryCalcNetLen(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *netname, *err = NULL;
	pcb_net_t *net;
	pcb_net_term_t *t;
	pcb_any_obj_t *obj;
	pcb_qry_netseg_len_t *nl;
	pcb_qry_exec_t ec;

	RND_ACT_CONVARG(1, FGW_STR, QueryCalcNetLen, netname = argv[1].val.str);

	pcb_qry_init(&ec, pcb, NULL, -1);

	net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
	if (net == NULL) {
		err = "not found";
		goto out;
	}

	if (pcb_termlist_length(&net->conns) > 2) {
		err = "more than 2 terminals";
		goto out;
	}

	t = pcb_termlist_first(&net->conns);
	if ((t == NULL) || ((obj = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL)) == NULL)) {
		err = "missing terminal";
		goto out;
	}

	nl = pcb_qry_parent_net_lenseg(&ec, obj);

	if (nl->has_nontrace) {
		err = "not a trace";
		goto out;
	}
	if (nl->has_junction) {
		err = "junction";
		goto out;
	}

	out:;
	if (err != NULL) {
		res->type = FGW_STR;
		res->val.cstr = err;
	}
	else {
		res->type = FGW_COORD;
		fgw_coord(res) = nl->len;
	}
	pcb_qry_uninit(&ec);
	return 0;
}
