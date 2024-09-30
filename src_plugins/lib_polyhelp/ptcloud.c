/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

/* Fill up a polyarea with more or less evenly spaced points */

#include "config.h"
#include <assert.h>

#include "ptcloud.h"

#include "ptcloud_debug.c"

/* unit is ctx->target_dist; so value 7 means 7 * ctx->target_dist, making
   the area of a single grid tile (7*ctx->target_dist)^2 */
#define GRID_SIZE 10

/* returns grid idx for coords x;y */
RND_INLINE long xy2gidx(pcb_ptcloud_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
	long gx = (x - ctx->pl->xmin / (GRID_SIZE * ctx->target_dist)) + 1; /* +1 for the border */
	long gy = (y - ctx->pl->ymin / (GRID_SIZE * ctx->target_dist)) + 1; /* +1 for the border */
	return gy * ctx->gsx + gx;
}

/* returns grid idx where the point is linked in */
RND_INLINE long pt2gidx(pcb_ptcloud_ctx_t *ctx, pcb_ptcloud_pt_t *pt)
{
	assert(pt->link.parent != NULL);
	return pt->link.parent - ctx->grid;
}

RND_INLINE void pt_move(pcb_ptcloud_ctx_t *ctx, pcb_ptcloud_pt_t *pt)
{
	long curr_gidx, new_gidx;

	if ((pt->dx == 0) && (pt->dy == 0))
		return;

	curr_gidx = pt2gidx(ctx, pt);
	pt->x += pt->dx;
	pt->y += pt->dy;
	pt->dx = pt->dy = 0;
	new_gidx = xy2gidx(ctx, pt->x, pt->y);

	if (curr_gidx != new_gidx) {
		gdl_remove(&ctx->grid[curr_gidx], pt, link);
		gdl_append(&ctx->grid[new_gidx], pt, link);
	}
}

RND_INLINE void pt_create(pcb_ptcloud_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y, int weight, int fixed)
{
	pcb_ptcloud_pt_t *pt = calloc(sizeof(pcb_ptcloud_pt_t), 1);
	long gidx = xy2gidx(ctx, x, y);
	assert(gidx >= 0);
	assert(gidx < ctx->glen);

	pt->x = x;
	pt->y = y;
	pt->weight = weight;
	pt->fixed = fixed;

	gdl_append(&ctx->grid[gidx], pt, link);
}

RND_INLINE void grid_alloc(pcb_ptcloud_ctx_t *ctx)
{
	ctx->gsx = (ctx->pl->xmax - ctx->pl->xmin) / (GRID_SIZE * ctx->target_dist) + 2;
	ctx->gsy = (ctx->pl->ymax - ctx->pl->ymin) / (GRID_SIZE * ctx->target_dist) + 2;
	ctx->glen = ctx->gsx * ctx->gsy;
	ctx->grid = calloc(sizeof(gdl_list_t), ctx->glen);
}

RND_INLINE rnd_coord_t edge_x_for_y(rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t y)
{
	double dx = (double)(lx2 - lx1) / (double)(ly2 - ly1);
	return rnd_round((double)lx1 + (double)(y - ly1) * dx);
}

static rnd_rtree_dir_t ptcloud_ray_cb(void *udata, void *obj, const rnd_rtree_box_t *box)
{
	pcb_ptcloud_ctx_t *ctx = udata;
	rnd_vnode_t *vn = rnd_pline_seg2vnode(obj);

	/* consider edges going up (emulate that the ray is a tiny bit above the integer y coordinate) */
	if (vn->next->point[1] < vn->point[1]) {
		rnd_coord_t x = edge_x_for_y(vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[1], ctx->ray_y);
		vtc0_append(&ctx->edges, x);
	}

	return 0;
}

static int cmp_crd(const void *A, const void *B)
{
	const rnd_coord_t *a = A, *b = B;
	return (*a < *b) ? -1 : +1;
}

void pcb_ptcloud(pcb_ptcloud_ctx_t *ctx)
{
	rnd_coord_t y, half = ctx->target_dist/2;

	grid_alloc(ctx);

	/* horizontal rays */
	for(y = ctx->pl->ymin + half; y < ctx->pl->ymax; y += ctx->target_dist) {
		rnd_rtree_box_t sb;

		sb.x1 = ctx->pl->xmin - half; sb.y1 = y;
		sb.x2 = RND_COORD_MAX; sb.y2 = y+1;
		ctx->edges.used = 0;
		ctx->ray_y = y;
		rnd_rtree_search_obj(ctx->pl->tree, &sb, ptcloud_ray_cb, ctx);

		if (ctx->edges.used == 0)
			continue;

		qsort(&ctx->edges.array, ctx->edges.used, sizeof(rnd_coord_t), cmp_crd);
	}

	ptcloud_debug_draw(ctx, "PTcloud.svg");

	vtc0_uninit(&ctx->edges);
}
