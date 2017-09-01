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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <genvector/vtp0.h>

#include "topoly.h"

#include "error.h"
#include "rtree.h"
#include "search.h"
#include "hid.h"

#define VALID_TYPE(obj) (((obj)->type == PCB_OBJ_ARC)  || ((obj)->type == PCB_OBJ_LINE))
#define CONT_TYPE (PCB_TYPE_LINE | PCB_TYPE_ARC)

#define NEAR(x1, x2, y1, y2) (((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) < 4)

/*** map the contour ***/
typedef struct {
	vtp0_t *list;
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
		ctx->result = obj;
		return PCB_R_DIR_FOUND_CONTINUE;
	}

	if (NEAR(ctx->tx, l->Point2.X, ctx->ty, l->Point2.Y)) {
		ctx->result = obj;
		return PCB_R_DIR_FOUND_CONTINUE;
	}

	return PCB_R_DIR_NOT_FOUND;
}

static pcb_any_obj_t *next_conn(vtp0_t *list, pcb_any_obj_t *curr)
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

static int map_contour(vtp0_t *list, pcb_any_obj_t *start)
{
	pcb_any_obj_t *n;

pcb_trace("loop start: %d\n", start->ID);
	vtp0_append(list, start);
	for(n = next_conn(list, start); n != start; n = next_conn(list, n)) {
		if (n == NULL)
			return -1;
pcb_trace("      next: %d\n", n->ID);
		vtp0_append(list, n);
	}
	return 0;
}

pcb_polygon_t *pcb_topoly_conn(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how)
{
	vtp0_t objs;
	int res;

	if (!VALID_TYPE(start)) {
		pcb_message(PCB_MSG_ERROR, "pcb_topoly_conn(): starting object is not a line or arc\n");
		return NULL;
	}

	vtp0_init(&objs);
	res = map_contour(&objs, start);
	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "pcb_topoly_conn(): failed to find a closed loop of lines and arcs\n");
		vtp0_uninit(&objs);
		return NULL;
	}

	vtp0_uninit(&objs);
	return NULL;
}



const char pcb_acts_topoly[] = "ToPoly()\n";
const char pcb_acth_topoly[] = "convert a closed loop of lines and arcs into a polygon";
int pcb_act_topoly(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	void *r1, *r2, *r3;

	pcb_gui->get_coords("Click on a line or arc of the contour loop", &x, &y);
	if (pcb_search_screen(x, y, CONT_TYPE, &r1, &r2, &r3) == 0) {
		pcb_message(PCB_MSG_ERROR, "ToPoly(): failed to find a line or arc there\n");
		return 1;
	}

	pcb_topoly_conn(PCB, r3, 0);
	return 0;
}
