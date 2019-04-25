/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "error.h"
#include "rtree.h"
#include "data.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_poly.h"
#include "obj_poly_draw.h"
#include "polygon.h"
#include "search.h"
#include "hid.h"
#include "actions.h"
#include "funchash_core.h"

#define VALID_TYPE(obj) (((obj)->type == PCB_OBJ_ARC)  || ((obj)->type == PCB_OBJ_LINE))
#define CONT_TYPE (PCB_OBJ_LINE | PCB_OBJ_ARC)

#define NEAR(x1, x2, y1, y2) (((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) < 4)

/*** map the contour ***/
typedef struct {
	vtp0_t *list;
	vti0_t *endlist;
	pcb_any_obj_t *curr, *result;
	pcb_coord_t tx, ty;
} next_conn_t;

/* Return whether the object is already on the list (but ignore the start object
   so we can do a full circle) */
static int already_found(next_conn_t *ctx, pcb_any_obj_t *obj)
{
	int n;
	for(n = 1; n < vtp0_len(ctx->list); n++)
		if (ctx->list->array[n] == obj)
			return 1;
	return 0;
}

static pcb_r_dir_t next_conn_found_arc(const pcb_box_t *box, void *cl)
{
	pcb_coord_t ex, ey;
	next_conn_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;
	int n;

	if (obj == ctx->curr)
		return PCB_R_DIR_NOT_FOUND; /* need the object connected to the other endpoint */

	if (already_found(ctx, obj))
		return PCB_R_DIR_NOT_FOUND;

	for(n = 0; n < 2; n++) {
		pcb_arc_get_end((pcb_arc_t *)obj, n, &ex, &ey);
		if (NEAR(ctx->tx, ex, ctx->ty, ey)) {
			vti0_append(ctx->endlist, n);
			vtp0_append(ctx->list, obj);
			ctx->result = obj;
			return PCB_R_DIR_FOUND_CONTINUE;
		}
	}

	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t next_conn_found_line(const pcb_box_t *box, void *cl)
{
	next_conn_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;
	pcb_line_t *l = (pcb_line_t *)box;

	if (obj == ctx->curr)
		return PCB_R_DIR_NOT_FOUND; /* need the object connected to the other endpoint */

	if (already_found(ctx, obj))
		return PCB_R_DIR_NOT_FOUND;

	if (NEAR(ctx->tx, l->Point1.X, ctx->ty, l->Point1.Y)) {
		vti0_append(ctx->endlist, 0);
		vtp0_append(ctx->list, obj);
		ctx->result = obj;
		return PCB_R_DIR_FOUND_CONTINUE;
	}

	if (NEAR(ctx->tx, l->Point2.X, ctx->ty, l->Point2.Y)) {
		vti0_append(ctx->endlist, 1);
		vtp0_append(ctx->list, obj);
		ctx->result = obj;
		return PCB_R_DIR_FOUND_CONTINUE;
	}

	return PCB_R_DIR_NOT_FOUND;
}

static pcb_any_obj_t *next_conn(vtp0_t *list, vti0_t *endlist, pcb_any_obj_t *curr)
{
	pcb_line_t *l;
	int n;
	next_conn_t ctx;
	pcb_coord_t cx[2], cy[2];

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

	for(n = 0; n < 2; n++) {
		pcb_box_t region;
		int len;

		region.X1 = cx[n]-1;
		region.Y1 = cy[n]-1;
		region.X2 = cx[n]+1;
		region.Y2 = cy[n]+1;
		ctx.tx = cx[n];
		ctx.ty = cy[n];

		pcb_r_search(curr->parent.layer->arc_tree, &region, NULL, next_conn_found_arc, &ctx, &len);
		if (len > 1) {
			pcb_message(PCB_MSG_ERROR, "map_contour(): contour is not a clean loop: it contains at least one stub or subloop\n");
			return NULL;
		}
		if (ctx.result != NULL)
			return ctx.result;

		pcb_r_search(curr->parent.layer->line_tree, &region, NULL, next_conn_found_line, &ctx, &len);
		if (len > 1) {
			pcb_message(PCB_MSG_ERROR, "map_contour(): contour is not a clean loop: it contains at least one stub or subloop\n");
			return NULL;
		}
		if (ctx.result != NULL)
			return ctx.result;
	}

	return NULL; /* nothing found */
}

static int map_contour(vtp0_t *list, vti0_t *endlist, pcb_any_obj_t *start)
{
	pcb_any_obj_t *n;

pcb_trace("loop start: %d\n", start->ID);
	vtp0_append(list, start);
	for(n = next_conn(list, endlist, start); n != start; n = next_conn(list, endlist, n)) {
		if (n == NULL)
			return -1;
pcb_trace("      next: %d\n", n->ID);
	}
pcb_trace("    (next): %d\n", n->ID);

	return 0;
}

pcb_poly_t *contour2poly(pcb_board_t *pcb, vtp0_t *objs, vti0_t *ends, pcb_topoly_t how)
{
	int n;
	pcb_any_obj_t **obj = (pcb_any_obj_t **)(&objs->array[0]);
	int *end = (int *)(&ends->array[0]);
	pcb_layer_t *layer = (*obj)->parent.layer;
	pcb_poly_t *poly = pcb_poly_alloc(layer);
	pcb_coord_t x, y;

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
TODO(": fix this with a real approx")
				pcb_arc_get_end(a, end[0], &x, &y);
				pcb_poly_point_new(poly, x, y);
				pcb_arc_middle(a, &x, &y);
				pcb_poly_point_new(poly, x, y);
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
TODO(": use pcb_board_set_changed_flag(), but decouple that from PCB")
		pcb->Changed = pcb_true;
	}

	return poly;
}

pcb_poly_t *pcb_topoly_conn(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how)
{
	vtp0_t objs;
	vti0_t ends;
	int res;
	pcb_poly_t *poly;

	if (!VALID_TYPE(start)) {
		pcb_message(PCB_MSG_ERROR, "pcb_topoly_conn(): starting object is not a line or arc\n");
		return NULL;
	}

	vtp0_init(&objs);
	vti0_init(&ends);
	res = map_contour(&objs, &ends, start);
	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "pcb_topoly_conn(): failed to find a closed loop of lines and arcs\n");
		vtp0_uninit(&objs);
		vti0_uninit(&ends);
		return NULL;
	}

	poly = contour2poly(pcb, &objs, &ends, how);

	vtp0_uninit(&objs);
	vti0_uninit(&ends);
	return poly;
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
	pcb_layer_id_t lid;
	pcb_layer_t *layer;
	pcb_any_obj_t *best = NULL;
	pcb_coord_t x, y;
	double bestd = (double)pcb->hidlib.size_y*(double)pcb->hidlib.size_y + (double)pcb->hidlib.size_x*(double)pcb->hidlib.size_x;

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

const char pcb_acts_topoly[] = "ToPoly()\nToPoly(outline)\n";
const char pcb_acth_topoly[] = "convert a closed loop of lines and arcs into a polygon";
fgw_error_t pcb_act_topoly(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = NULL;
	void *r1, *r2, *r3;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, topoly, op = argv[1].val.str);

	if (op == NULL) {
		pcb_coord_t x, y;
		pcb_hid_get_coords("Click on a line or arc of the contour loop", &x, &y, 0);
		if (pcb_search_screen(x, y, CONT_TYPE, &r1, &r2, &r3) == 0) {
			pcb_message(PCB_MSG_ERROR, "ToPoly(): failed to find a line or arc there\n");
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else {
		if (strcmp(op, "outline") == 0) {
			r2 = pcb_topoly_find_1st_outline(PCB);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Invalid first argument\n");
			PCB_ACT_IRES(1);
			return 0;
		}
	}

	pcb_topoly_conn(PCB, r2, 0);
	PCB_ACT_IRES(0);
	return 0;
}
