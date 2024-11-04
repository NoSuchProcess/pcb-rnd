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
#include <librnd/core/error.h>

#include "ptcloud.h"

#include "ptcloud_debug.c"

/* unit is ctx->target_dist; so value 7 means 7 * ctx->target_dist, making
   the area of a single grid tile (7*ctx->target_dist)^2 */
#define GRID_SIZE 10
#define PROXIMITY 1.42


#define WEIGHT_CONTOUR 300
#define WEIGHT_INTERNAL 100

#define TRACE_ANNEAL 0

/* returns grid idx for coords x;y */
RND_INLINE long xy2gidx(pcb_ptcloud_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
	long gx = (x - ctx->pl->xmin) / (GRID_SIZE * ctx->target_dist) + 1; /* +1 for the border */
	long gy = (y - ctx->pl->ymin) / (GRID_SIZE * ctx->target_dist) + 1; /* +1 for the border */
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
	gdl_append(&ctx->points, pt, all);
}

RND_INLINE void grid_alloc(pcb_ptcloud_ctx_t *ctx)
{
	ctx->gsx = (ctx->pl->xmax - ctx->pl->xmin) / (GRID_SIZE * ctx->target_dist) + 3;
	ctx->gsy = (ctx->pl->ymax - ctx->pl->ymin) / (GRID_SIZE * ctx->target_dist) + 3;
	ctx->glen = ctx->gsx * ctx->gsy;
	ctx->grid = calloc(sizeof(gdl_list_t), ctx->glen);
}

RND_INLINE rnd_coord_t edge_x_for_y(rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t y)
{
	double dx = (double)(lx2 - lx1) / (double)(ly2 - ly1);
	return rnd_round((double)lx1 + (double)(y - ly1) * dx);
}

RND_INLINE void ptcloud_pline_create_points(pcb_ptcloud_ctx_t *ctx, rnd_pline_t *pl)
{
	rnd_vnode_t *vn = pl->head;

	do {
		double vx, vy, len, x, y, l, n, step;

		vx = vn->next->point[0] - vn->point[0];
		vy = vn->next->point[1] - vn->point[1];
		len = sqrt(vx*vx + vy*vy);
		if (len == 0)
			continue;

		n = ceil(len / ctx->target_dist);
		step = len / n;

		vx = vx/len * step;
		vy = vy/len * step;

		pt_create(ctx, vn->point[0], vn->point[1], WEIGHT_CONTOUR, 1);
		x = vn->point[0];
		y = vn->point[1];
		for(l = step; l < len; l += step) {
			x += vx;
			y += vy;
			pt_create(ctx, rnd_round(x), rnd_round(y), WEIGHT_CONTOUR, 1);
		}
	} while((vn = vn->next) != pl->head);
}

RND_INLINE void ptcloud_contour_create_points(pcb_ptcloud_ctx_t *ctx)
{
	rnd_pline_t *pl;
	for(pl = ctx->pl; pl != NULL; pl = pl->next)
		ptcloud_pline_create_points(ctx, pl);
}

static rnd_rtree_dir_t ptcloud_ray_cb(void *udata, void *obj, const rnd_rtree_box_t *box)
{
	pcb_ptcloud_ctx_t *ctx = udata;
	rnd_vnode_t *vn = rnd_pline_seg2vnode(obj);
	rnd_coord_t x;

	/* ignore edges with endpoint hit if the edge is not going upward;
	   emulate that the ray is a tiny bit above the integer y coordinate) */
	if ((ctx->ray_y == vn->next->point[1]) && (vn->next->point[1] >= vn->point[1]))
		return 0;
	if ((ctx->ray_y == vn->point[1]) && (vn->next->point[1] <= vn->point[1]))
		return 0;

	/* add x coord of the crossing to the (yet unodreded) edge vector */
	x = edge_x_for_y(vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[1], ctx->ray_y);
	vtc0_append(&ctx->edges, x);

	return 0;
}

static int cmp_crd(const void *A, const void *B)
{
	const rnd_coord_t *a = A, *b = B;
	return (*a < *b) ? -1 : +1;
}

static void ptcloud_ray_create_points(pcb_ptcloud_ctx_t *ctx)
{
	long n;
	for(n = 0; n+1 < ctx->edges.used; n+=2) {
		rnd_coord_t x, x1 = ctx->edges.array[n], x2 = ctx->edges.array[n+1];

		TODO("verify there's no horizontal line overlapping");
		for(x = x1 + ctx->target_dist; x <= x2 - ctx->target_dist; x += ctx->target_dist)
			pt_create(ctx, x, ctx->ray_y, WEIGHT_INTERNAL, 0);
	}
}

RND_INLINE void ptcloud_anneal_compute_pt(pcb_ptcloud_ctx_t *ctx, long gidx, pcb_ptcloud_pt_t *pt0)
{
	pcb_ptcloud_pt_t *pt;

	for(pt = gdl_first(&ctx->points); pt != NULL; pt = pt->all.next) {
		rnd_coord_t dx, dy;
		double dx2, dy2, d2, err, px, py;

	}
}


RND_INLINE void ptcloud_anneal_compute(pcb_ptcloud_ctx_t *ctx)
{
	pcb_ptcloud_pt_t *pt;

	for(pt = gdl_first(&ctx->points); pt != NULL; pt = pt->all.next) {
		long gidx;

		if (pt->fixed)
			continue;

		if (TRACE_ANNEAL)
			rnd_trace("ANN: %.06mm %.06mm\n", pt->x, pt->y);

		pt->dx = pt->dy = 0;
		gidx = pt2gidx(ctx, pt);

return;
	}
}

RND_INLINE double ptcloud_anneal_execute(pcb_ptcloud_ctx_t *ctx)
{
	pcb_ptcloud_pt_t *pt;
	double err = 0;

	for(pt = gdl_first(&ctx->points); pt != NULL; pt = pt->all.next) {

		if (pt->fixed || ((pt->dx == 0) && (pt->dy == 0)))
			continue;

		err += fabs(pt->dx) + fabs(pt->dy);
		pt_move(ctx, pt);
	}
	return err;
}

RND_INLINE int ptcloud_triangulate(pcb_ptcloud_ctx_t *ctx)
{
	size_t mem_req;
	long n, num_pt = gdl_length(&ctx->points);
	pcb_ptcloud_pt_t *pt;

	mem_req = fp2t_memory_required(num_pt);
	ctx->tri_mem = calloc(mem_req, 1);

	if (!fp2t_init(&ctx->tri, ctx->tri_mem, num_pt)) {
		free(ctx->tri_mem);
		return -1;
	}

	for(pt = gdl_first(&ctx->points), n = 0; pt != NULL; pt = pt->all.next, n++) {
		fp2t_point_t *fpt = fp2t_push_point(&ctx->tri);
		fpt->X = pt->x;
		fpt->Y = pt->y;
		if (n == ctx->num_pt_edge)
			fp2t_add_edge(&ctx->tri);

		assert(num_pt-- > 0);
	}

	fp2t_triangulate(&ctx->tri);

	return 0;
}

void pcb_ptcloud(pcb_ptcloud_ctx_t *ctx)
{
	rnd_coord_t y, half = ctx->target_dist/2;
	double err, min_err;
	long n;

	grid_alloc(ctx);
	ctx->closed = ceil(PROXIMITY * ctx->target_dist);
	ctx->closed2 = (double)ctx->closed * (double)ctx->closed;
	ctx->target2 = (double)ctx->target_dist * (double)ctx->target_dist;
	min_err = (double)ctx->target_dist / 20.0; /* 5% */

	ptcloud_contour_create_points(ctx);

	ctx->num_pt_edge = gdl_length(&ctx->points);

	/* horizontal rays */
	for(y = ctx->pl->ymin + ctx->target_dist; y <= ctx->pl->ymax - ctx->target_dist; y += ctx->target_dist) {
		rnd_rtree_box_t sb;

		sb.x1 = ctx->pl->xmin - half; sb.y1 = y;
		sb.x2 = RND_COORD_MAX; sb.y2 = y+1;
		ctx->edges.used = 0;
		ctx->ray_y = y;
		rnd_rtree_search_obj(ctx->pl->tree, &sb, ptcloud_ray_cb, ctx);

rnd_trace(" y: %06mm hits: %d\n", y, ctx->edges.used);

		if (ctx->edges.used == 0)
			continue;

		assert((ctx->edges.used % 2) == 0);

		qsort(ctx->edges.array, ctx->edges.used, sizeof(rnd_coord_t), cmp_crd);
/*		ptcloud_ray_create_points(ctx);*/
	}


	ptcloud_triangulate(ctx);
	ptcloud_debug_draw(ctx, "PTcloud.svg");

/*
	for(n = 0; n < 1; n++) {
		char fn[128];
		
		ptcloud_anneal_compute(ctx);

		if ((n % 10) == 0) {
			sprintf(fn, "PTcloud%04ld.svg", n);
			ptcloud_debug_draw(ctx, fn);
		}

		err = ptcloud_anneal_execute(ctx);
		rnd_trace("[%ld] err=%f\n", n, err);
	}
	*/


	vtc0_uninit(&ctx->edges);
}

void pcb_ptcloud_free(pcb_ptcloud_ctx_t *ctx)
{
	free(ctx->tri_mem);
	TODO("free points and grid");
}

