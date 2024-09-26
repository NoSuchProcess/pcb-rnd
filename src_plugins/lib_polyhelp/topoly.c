/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020,2023 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <genvector/vtp0.h>
#include <genvector/vti0.h>

#include "topoly.h"

#include <librnd/core/error.h>
#include <librnd/poly/rtree.h>
#include "data.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_poly.h"
#include "obj_poly_draw.h"
#include "obj_pstk_inlines.h"
#include "find.h"
#include "polygon.h"
#include "search.h"
#include <librnd/hid/hid.h>
#include <librnd/core/actions.h>
#include "funchash_core.h"

#define VALID_TYPE(obj) (((obj)->type == PCB_OBJ_ARC)  || ((obj)->type == PCB_OBJ_LINE))
#define CONT_TYPE (PCB_OBJ_LINE | PCB_OBJ_ARC)

#define NEAR(x1, x2, y1, y2) (((double)(x1-x2)*(double)(x1-x2) + (double)(y1-y2)*(double)(y1-y2)) < 4)

/*** map the contour ***/
typedef struct {
	vtp0_t *list;
	vti0_t *endlist;
	pcb_any_obj_t *curr, *result;
	rnd_coord_t tx, ty;
	pcb_dynf_t mark;
} next_conn_t;

static rnd_rtree_dir_t next_conn_found_arc(void *cl, void *obj_, const rnd_rtree_box_t *box)
{
	rnd_coord_t ex, ey;
	next_conn_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)obj_;
	int n;

	if (PCB_DFLAG_TEST(&obj->Flags, ctx->mark))
		return rnd_RTREE_DIR_NOT_FOUND_CONT; /* object already mapped */

	for(n = 0; n < 2; n++) {
		pcb_arc_get_end((pcb_arc_t *)obj, n, &ex, &ey);
		if (NEAR(ctx->tx, ex, ctx->ty, ey)) {
			vti0_append(ctx->endlist, n);
			vtp0_append(ctx->list, obj);
			PCB_DFLAG_SET(&obj->Flags, ctx->mark);
			ctx->result = obj;
			return rnd_RTREE_DIR_FOUND_CONT;
		}
	}

	return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

static rnd_rtree_dir_t next_conn_found_line(void *cl, void *obj_, const rnd_rtree_box_t *box)
{
	next_conn_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)obj_;
	pcb_line_t *l = (pcb_line_t *)box;

	if (PCB_DFLAG_TEST(&obj->Flags, ctx->mark))
		return rnd_RTREE_DIR_NOT_FOUND_CONT; /* object already mapped */

	if (NEAR(ctx->tx, l->Point1.X, ctx->ty, l->Point1.Y)) {
		vti0_append(ctx->endlist, 0);
		vtp0_append(ctx->list, obj);
		PCB_DFLAG_SET(&obj->Flags, ctx->mark);
		ctx->result = obj;
		return rnd_RTREE_DIR_FOUND_CONT;;
	}

	if (NEAR(ctx->tx, l->Point2.X, ctx->ty, l->Point2.Y)) {
		vti0_append(ctx->endlist, 1);
		vtp0_append(ctx->list, obj);
		PCB_DFLAG_SET(&obj->Flags, ctx->mark);
		ctx->result = obj;
		return rnd_RTREE_DIR_FOUND_CONT;
	}

	return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

static pcb_any_obj_t *next_conn(vtp0_t *list, vti0_t *endlist, pcb_any_obj_t *curr, pcb_dynf_t df)
{
	pcb_line_t *l;
	int n;
	next_conn_t ctx;
	rnd_coord_t cx[2], cy[2];

	/* determine the endpoints of the current object */
	switch(curr->type) {
		case PCB_OBJ_ARC:
			pcb_arc_get_end((pcb_arc_t *)curr, 0, &cx[0], &cy[0]);
			pcb_arc_get_end((pcb_arc_t *)curr, 1, &cx[1], &cy[1]);
			break;
		case PCB_OBJ_LINE:
			l = (pcb_line_t *)curr;
			cx[0] = l->Point1.X; cy[0] = l->Point1.Y;
			cx[1] = l->Point2.X; cy[1] = l->Point2.Y;
			break;
		default:
			return NULL;
	}

	ctx.curr = curr;
	ctx.list = list;
	ctx.endlist = endlist;
	ctx.result = NULL;
	ctx.mark = df;

	for(n = 0; n < 2; n++) {
		rnd_box_t region;
		long len;

		region.X1 = cx[n]-1;
		region.Y1 = cy[n]-1;
		region.X2 = cx[n]+1;
		region.Y2 = cy[n]+1;
		ctx.tx = cx[n];
		ctx.ty = cy[n];

		rnd_rtree_search_any(curr->parent.layer->arc_tree, (rnd_rtree_box_t *)&region, NULL, next_conn_found_arc, &ctx, &len);
		if (len > 1) {
			rnd_message(RND_MSG_ERROR, "map_contour(): contour is not a clean loop: it contains at least one stub or subloop\n");
			return NULL;
		}
		if (ctx.result != NULL)
			return ctx.result;

		rnd_rtree_search_any(curr->parent.layer->line_tree, (rnd_rtree_box_t *)&region, NULL, next_conn_found_line, &ctx, &len);
		if (len > 1) {
			rnd_message(RND_MSG_ERROR, "map_contour(): contour is not a clean loop: it contains at least one stub or subloop\n");
			return NULL;
		}
		if (ctx.result != NULL)
			return ctx.result;
	}

	return NULL; /* nothing found */
}

static int map_contour_with(pcb_data_t *data, vtp0_t *list, vti0_t *endlist, pcb_any_obj_t *start, pcb_dynf_t df)
{
	long i;
	pcb_any_obj_t *n;

/*rnd_trace("loop start: %d\n", start->ID);*/
	vtp0_append(list, start);
	PCB_DFLAG_SET(&start->Flags, df);
	for(i = 0, n = next_conn(list, endlist, start, df); n != start; n = next_conn(list, endlist, n, df), i++) {
		if (n == NULL) {
/*			rnd_trace("      broken trace\n");*/
			return -1;
		}
		if (i == 1)
			PCB_DFLAG_CLR(&start->Flags, df); /* allow finding the start object again for proper closing */
	}

	return 0;
}

static int map_contour(pcb_data_t *data, vtp0_t *list, vti0_t *endlist, pcb_any_obj_t *start)
{
	int res;
	pcb_dynf_t df;
	df = pcb_dynflag_alloc("topoly_map_contour");
	pcb_data_dynflag_clear(data, df);

	res = map_contour_with(data, list, endlist, start, df);

	pcb_dynflag_free(df);
	return res;
}

static int contour2poly_cb(void *uctx, rnd_coord_t x, rnd_coord_t y)
{
	pcb_poly_t *poly = uctx;
	pcb_poly_point_new(poly, x, y);
	return 0;
}

pcb_poly_t *contour2poly(pcb_board_t *pcb, vtp0_t *objs, vti0_t *ends, pcb_topoly_t how)
{
	int n;
	pcb_any_obj_t **obj = (pcb_any_obj_t **)(&objs->array[0]);
	int *end = &ends->array[0];
	pcb_layer_t *layer = (*obj)->parent.layer;
	pcb_poly_t *poly = pcb_poly_alloc(layer);

	obj++;
	for(n = 1; n < vtp0_len(objs); obj++,end++,n++) {
		pcb_line_t *l = (pcb_line_t *)*obj;
		pcb_arc_t *a = (pcb_arc_t *)*obj;

		/* end[0] is the starting endpoint of the current *obj,
		   end[1] is the starting endpoint of the next *obj */
		switch((*obj)->type) {
			case PCB_OBJ_LINE:
				switch(end[0]) {
					case 0: pcb_poly_point_new(poly, l->Point1.X, l->Point1.Y); break;
					case 1: pcb_poly_point_new(poly, l->Point2.X, l->Point2.Y); break;
					default: abort();
				}
				break;
			case PCB_OBJ_ARC:
				pcb_arc_approx(a, 0, end[0] != 0, poly, contour2poly_cb);
				poly->PointN--; /* remove the arc endpoint to avoid redundant point on the start of the next object */
				break;
			default:
				pcb_poly_free(poly);
				return NULL;
		}
	}

	if (!(how & PCB_TOPOLY_FLOATING)) {
		pcb_add_poly_on_layer(layer, poly);
		pcb_poly_init_clip(pcb->Data, layer, poly);
		pcb_poly_invalidate_draw(layer, poly);
		pcb_board_set_changed_flag(pcb, rnd_true);
	}

	return poly;
}

pcb_poly_t *pcb_topoly_conn_with(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how, pcb_dynf_t df)
{
	vtp0_t objs;
	vti0_t ends;
	int res;
	pcb_poly_t *poly;

	if (!VALID_TYPE(start)) {
		rnd_message(RND_MSG_ERROR, "pcb_topoly_conn(): starting object is not a line or arc\n");
		return NULL;
	}

	vtp0_init(&objs);
	vti0_init(&ends);
	if (df < 0)
		res = map_contour(pcb->Data, &objs, &ends, start);
	else
		res = map_contour_with(pcb->Data, &objs, &ends, start, df);
	if (res != 0) {
		rnd_message(RND_MSG_ERROR, "pcb_topoly_conn(): failed to find a closed loop of lines and arcs\n");
		vtp0_uninit(&objs);
		vti0_uninit(&ends);
		return NULL;
	}

	poly = contour2poly(pcb, &objs, &ends, how);

	vtp0_uninit(&objs);
	vti0_uninit(&ends);
	return poly;
}

pcb_poly_t *pcb_topoly_conn(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how)
{
	return pcb_topoly_conn_with(pcb, start, how, -1);
}

typedef struct {
	pcb_board_t *pcb;
	pcb_poly_t *poly;
	pcb_dynf_t df;
	vtp0_t pstks;  /* a list of (pcb_pstk_t *) of padstacks having mech shape crossing the poly border */

	/* temporary, per pline callback fields */
	pcb_pstk_t *ps;
	pcb_pstk_shape_t *shape;
} pstk_on_outline_t;

/* rtree callback on pline segment crossing padstack bbox */
static rnd_rtree_dir_t pcb_topoly_check_pstk_on_pline_cb(void *cl, void *obj_, const rnd_rtree_box_t *box)
{
	pstk_on_outline_t *ctx = cl;
	pcb_find_t fctx = {0};
	pcb_line_t line = {0};
	rnd_vnode_t *vn = rnd_pline_seg2vnode((void *)obj_);

	line.Point1.X = vn->point[0]; line.Point1.Y = vn->point[1];
	line.Point2.X = vn->next->point[0]; line.Point2.Y = vn->next->point[1];
	if (pcb_isc_pstk_line_shp(&fctx, ctx->ps, &line, ctx->shape)) {
		if (ctx->df != -1)
			PCB_DFLAG_SET(&ctx->ps->Flags, ctx->df);
		vtp0_append(&ctx->pstks, ctx->ps);
	}

	return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

/* rtree callback on generated polygon overlapping padstack */
static rnd_rtree_dir_t pcb_topoly_check_pstk_on_outline_cb(void *cl, void *obj_, const rnd_rtree_box_t *box)
{
	pstk_on_outline_t *ctx = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)obj_;
	pcb_pstk_shape_t holetmp;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_shape_t *shape = pcb_pstk_shape_mech_or_hole_(ps, proto, &holetmp);

	if (shape == NULL)
		return rnd_RTREE_DIR_NOT_FOUND_CONT;

	ctx->ps = ps;
	ctx->shape = shape;
	rnd_rtree_search_any(ctx->poly->Clipped->contours->tree, (rnd_rtree_box_t *)&ps->bbox_naked, NULL, pcb_topoly_check_pstk_on_pline_cb, ctx, NULL);
	ctx->ps = NULL;
	ctx->shape = NULL;

	return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

#define check(x, y, obj) \
do { \
	double dist = (double)x*(double)x + (double)y*(double)y; \
	if (dist < bestd) { \
		bestd = dist; \
		best = (pcb_any_obj_t *)obj; \
	} \
} while(0)

pcb_any_obj_t *pcb_topoly_find_1st_outline(pcb_board_t *pcb)
{
	rnd_layer_id_t lid;
	pcb_layer_t *layer;
	pcb_any_obj_t *best = NULL;
	rnd_coord_t x, y;
	double bw = rnd_dwg_get_size_x(&pcb->hidlib), bh = rnd_dwg_get_size_y(&pcb->hidlib);
	double bestd = bw*bw + bh*bh;

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		if (!PCB_LAYER_IS_OUTLINE(pcb_layer_flags(PCB, lid), pcb_layer_purpose(PCB, lid, NULL)))
			continue;

		layer = pcb_get_layer(PCB->Data, lid);
		PCB_LINE_LOOP(layer) {
			check(line->Point1.X, line->Point1.Y, line);
			check(line->Point2.X, line->Point2.Y, line);
		}
		PCB_END_LOOP;

		PCB_ARC_LOOP(layer) {
			pcb_arc_get_end(arc, 0, &x, &y);
			check(x, y, arc);
			pcb_arc_get_end(arc, 1, &x, &y);
			check(x, y, arc);
			pcb_arc_middle(arc, &x, &y);
			check(x, y, arc);
		}
		PCB_END_LOOP;
	}

	return best;
}
#undef check

pcb_poly_t *pcb_topoly_1st_outline_with(pcb_board_t *pcb, pcb_topoly_t how, pcb_dynf_t df)
{
	pcb_poly_t *poly;
	pcb_any_obj_t *start = pcb_topoly_find_1st_outline(pcb);
	pstk_on_outline_t pctx;
	int need_full;
	long n;

	if (start == NULL) {
		poly = pcb_poly_alloc(pcb->Data->Layer);
		pcb_poly_point_new(poly, pcb->hidlib.dwg.X1, pcb->hidlib.dwg.Y1);
		pcb_poly_point_new(poly, pcb->hidlib.dwg.X2, pcb->hidlib.dwg.Y1);
		pcb_poly_point_new(poly, pcb->hidlib.dwg.X2, pcb->hidlib.dwg.Y2);
		pcb_poly_point_new(poly, pcb->hidlib.dwg.X1, pcb->hidlib.dwg.Y2);
	}
	else
		poly = pcb_topoly_conn_with(pcb, start, how, df);

	if (poly == NULL)
		return NULL;

	pcb_poly_bbox(poly);
	poly->Clipped = pcb_poly_to_polyarea(poly, &need_full);

	if (need_full)
		rnd_message(RND_MSG_ERROR, "pcb_topoly_1st_outline_with: internal error: need full poly\nPlease report this bug with an example file!\n");

	/* padstacks crossing polygon boundary need to be subtracted from the poly as
	   they modify the contour */
	pctx.pcb = pcb;
	pctx.poly = poly;
	pctx.df = df;
	vtp0_init(&pctx.pstks);

	/* map: first search (and mark) all offending padstacks */
	rnd_rtree_search_any(pcb->Data->padstack_tree, (rnd_rtree_box_t *)&poly->bbox_naked, NULL, pcb_topoly_check_pstk_on_outline_cb, &pctx, NULL);

	/* apply: subtract all padstacks found while mapping */
	for(n = 0; n < pctx.pstks.used; n++) {
		pcb_pstk_t *ps = pctx.pstks.array[n];
		pcb_pstk_shape_t holetmp;
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		pcb_pstk_shape_t *shape = pcb_pstk_shape_mech_or_hole_(ps, proto, &holetmp);
		rnd_polyarea_t *mpa = pcb_pstk_shape2polyarea(ps, shape);
		if (mpa != NULL)
			pcb_poly_subtract(mpa, poly, 1);
	}

	vtp0_uninit(&pctx.pstks);
	return poly;
}

pcb_poly_t *pcb_topoly_1st_outline(pcb_board_t *pcb, pcb_topoly_t how)
{
	return pcb_topoly_1st_outline_with(pcb, how, -1);
}

#include "topoly_cutout.c"
#include "topoly_solid.c"

const char pcb_acts_topoly[] = "ToPoly()\nToPoly(outline)\n";
const char pcb_acth_topoly[] = "convert a closed loop of lines and arcs into a polygon";
fgw_error_t pcb_act_topoly(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = NULL;
	void *r1, *r2, *r3;

	RND_ACT_MAY_CONVARG(1, FGW_STR, topoly, op = argv[1].val.str);

	if (op == NULL) {
		rnd_coord_t x, y;
		rnd_hid_get_coords("Click on a line or arc of the contour loop", &x, &y, 0);
		if (pcb_search_screen(x, y, CONT_TYPE, &r1, &r2, &r3) == 0) {
			rnd_message(RND_MSG_ERROR, "ToPoly(): failed to find a line or arc there\n");
			RND_ACT_IRES(1);
			return 0;
		}
	}
	else {
		if (strcmp(op, "outline") == 0) {
			pcb_topoly_1st_outline(PCB, 0);
			RND_ACT_IRES(0);
			return 0;
		}
		else {
			rnd_message(RND_MSG_ERROR, "Invalid first argument\n");
			RND_ACT_IRES(1);
			return 0;
		}
	}

	pcb_topoly_conn(PCB, r2, 0);
	RND_ACT_IRES(0);
	return 0;
}
