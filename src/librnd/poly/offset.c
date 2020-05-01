/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

/* Polygon contour offset calculation */

#include <librnd/core/compat_misc.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/polyarea.h>
#include <librnd/poly/self_isc.h>
#include <librnd/poly/offset.h>

#if 0
#define pcbo_trace pcb_trace
#else
static void pcbo_trace(char *fmt, ...) {}
#endif

void pcb_polo_norm(double *nx, double *ny, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	double dx = x2 - x1, dy = y2 - y1, len = sqrt(dx*dx + dy*dy);
	*nx = -dy / len;
	*ny = dx / len;
}

void pcb_polo_normd(double *nx, double *ny, double x1, double y1, double x2, double y2)
{
	double dx = x2 - x1, dy = y2 - y1, len = sqrt(dx*dx + dy*dy);
	*nx = -dy / len;
	*ny = dx / len;
}

static long warp(long n, long len)
{
	if (n < 0) n += len;
	else if (n >= len) n -= len;
	return n;
}

void rnd_polo_edge_shift(double offs,
	double *x0, double *y0, double nx, double ny,
	double *x1, double *y1,
	double prev_x, double prev_y, double prev_nx, double prev_ny,
	double next_x, double next_y, double next_nx, double next_ny)
{
	double ax, ay, al, a1l, a1x, a1y;

	offs = -offs;

	/* previous edge's endpoint offset */
	ax = (*x0) - prev_x;
	ay = (*y0) - prev_y;
	al = sqrt(ax*ax + ay*ay);
	ax /= al;
	ay /= al;
	a1l = ax*nx + ay*ny;
	a1x = offs / a1l * ax;
	a1y = offs / a1l * ay;

	(*x0) += a1x;
	(*y0) += a1y;

	/* next edge's endpoint offset */
	ax = next_x - (*x1);
	ay = next_y - (*y1);
	al = sqrt(ax*ax + ay*ay);
	ax /= al;
	ay /= al;

	a1l = ax*nx + ay*ny;
	a1x = offs / a1l * ax;
	a1y = offs / a1l * ay;

	(*x1) += a1x;
	(*y1) += a1y;
}

void rnd_polo_offs(double offs, rnd_polo_t *pcsh, long num_pts)
{
	long n;

	for(n = 0; n < num_pts; n++) {
		long np = warp(n-1, num_pts), nn1 = warp(n+1, num_pts), nn2 = warp(n+2, num_pts);
		rnd_polo_edge_shift(offs,
			&pcsh[n].x, &pcsh[n].y, pcsh[n].nx, pcsh[n].ny,
			&pcsh[nn1].x, &pcsh[nn1].y,
			pcsh[np].x, pcsh[np].y, pcsh[np].nx, pcsh[np].ny,
			pcsh[nn2].x, pcsh[nn2].y, pcsh[nn2].nx, pcsh[nn2].ny
		);
	}
}


void rnd_polo_norms(rnd_polo_t *pcsh, long num_pts)
{
	long n;

	for(n = 0; n < num_pts; n++) {
		long nn = warp(n+1, num_pts);
		pcb_polo_normd(&pcsh[n].nx, &pcsh[n].ny, pcsh[n].x, pcsh[n].y, pcsh[nn].x, pcsh[nn].y);
	}
}

double rnd_polo_2area(rnd_polo_t *pcsh, long num_pts)
{
	double a = 0;
	long n;

	for(n = 0; n < num_pts-1; n++) {
		long nn = warp(n+1, num_pts);
		a += pcsh[n].x * pcsh[nn].y;
		a -= pcsh[n].y * pcsh[nn].x;
	}
	return a;
}

void rnd_pline_dup_offsets(vtp0_t *dst, const rnd_pline_t *src, rnd_coord_t offs)
{
	const rnd_vnode_t *v;
	rnd_vector_t tmp;
	rnd_pline_t *res = NULL;
	long num_pts, n, from;
	rnd_polo_t *pcsh;

	/* count corners */
	v = src->head;
	num_pts = 0;
	do {
		num_pts++;
	} while((v = v->next) != src->head);

	/* allocate the cache and copy all data */
	pcsh = malloc(sizeof(rnd_polo_t) * num_pts);
	for(n = 0, v = src->head; n < num_pts; n++, v = v->next) {
		pcsh[n].x = v->point[0];
		pcsh[n].y = v->point[1];
		pcb_polo_norm(&pcsh[n].nx, &pcsh[n].ny, v->point[0], v->point[1], v->next->point[0], v->next->point[1]);
	}

	/* offset the cache */
	rnd_polo_offs(offs, pcsh, num_pts);


	/* create a new pline by copying the cache */
	tmp[0] = rnd_round(pcsh[0].x);
	tmp[1] = rnd_round(pcsh[0].y);
	res = rnd_poly_contour_new(tmp);
	for(n = 1; n < num_pts; n++) {
		tmp[0] = rnd_round(pcsh[n].x);
		tmp[1] = rnd_round(pcsh[n].y);
		rnd_poly_vertex_include(res->head->prev, rnd_poly_node_create(tmp));
	}

	free(pcsh);

	from = dst->used;
	if (rnd_pline_is_selfint(res)) {
		rnd_pline_split_selfint(res, dst);
		rnd_poly_contour_del(&res);
	}
	else
		vtp0_append(dst, res);

	for(n = from; n < dst->used; n++) {
		res = dst->array[n];
		rnd_poly_contour_pre(res, 1);
		rnd_pline_keepout_offs(res, src, offs); /* avoid self-intersection */
		res->tree = rnd_poly_make_edge_tree(res);
		dst->array[n] = res;
	}
}

rnd_pline_t *rnd_pline_dup_offset(const rnd_pline_t *src, rnd_coord_t offs)
{
	vtp0_t selfi;
	rnd_pline_t *res = NULL;
	int n;
	double best = 0;

	vtp0_init(&selfi);
	rnd_pline_dup_offsets(&selfi, src, offs);

	for(n = 0; n < selfi.used; n++) {
		rnd_pline_t *pl = selfi.array[n];
		if (pl->area > best) {
			best = pl->area;
			res = pl;
		}
	}
	pcbo_trace("best area: %f out of %d\n", best, selfi.used);
	for(n = 0; n < selfi.used; n++) {
		rnd_pline_t *pl = selfi.array[n];
		if (res != pl)
			rnd_poly_contour_del(&pl);
	}
	vtp0_uninit(&selfi);
	return res;
}


TODO("this should be coming from gengeo2d");
/* Return the square of distance between point x0;y0 and line x1;y1 - x2;y2 */
static double dist_line_to_pt(double x0, double y0, double x1, double y1, double x2, double y2, double *odx, double *ody)
{
	double ax = x0 - x1, ay = y0 - y1;
	double dx = x2 - x1, dy = y2 - y1;
	double len = sqrt(dx*dx+dy*dy);
	double o, dxn, dyn;
	double tmp1, d2;


	dxn = dx / len;
	dyn = dy / len;

	o = (ax * dxn + ay * dyn) / len;
	if (o <= 0.0) {
		/* beyond x1;y1 */
		return ax * ax + ay * ay;
	}
	if (o >= 1.0) {
		/* beyond x1;y1 */
		double bx = x0 - x2, by = y0 - y2;
		return bx * bx + by * by;
	}

	/* in range: normal line-point dist */
	tmp1 = dy*x0 - dx*y0 + x2*y1 - y2*x1;
	d2 = dx*dx + dy*dy;

	*odx = dxn;
	*ody = dyn;
	return (tmp1 * tmp1) / d2;
}

/* Modify v, pulling it back toward vp so that the distance to line ldx;ldy is increased by tune */
RND_INLINE int pull_back(rnd_vnode_t *v, const rnd_vnode_t *vp, double tune, double ldx, double ldy, double prjx, double prjy, int inside)
{
	rnd_coord_t ox, oy;
	double c, vx, vy, vlen, prx, pry, prlen;

	vx = v->point[0] - vp->point[0];
	vy = v->point[1] - vp->point[1];
	if ((vx == 0) && (vy == 0))
		return -1;

	vlen = sqrt(vx*vx + vy*vy);
	vx /= vlen;
	vy /= vlen;

	prx = v->point[0] - prjx;
	pry = v->point[1] - prjy;
	prlen = sqrt(prx*prx + pry*pry);
	prx /= prlen;
	pry /= prlen;

	c = (ldy * vx - ldx * vy);
	if ((c < 0.0001) && (c > -0.0001)) {
		pcbo_trace("   par1: vp=%.12mm;%.12mm v=%.12mm;%.12mm\n", vp->point[0], vp->point[1], v->point[0], v->point[1]);
		pcbo_trace("   par2: vx=%f;%f ld=%f;%f\n", vx, vy, ldx, ldy);
		return -1; /* perpendicular; no pullbakc could help */
	}

	c = tune * ((-pry * ldx + prx * ldy) / c);

	pcbo_trace("   vect: vp=%mm;%mm v=%mm;%mm\n", vp->point[0], vp->point[1], v->point[0], v->point[1]);
	pcbo_trace("   vect: vx=%f;%f prx=%f;%f tune=%.012mm\n", vx, vy, prx, pry, (rnd_coord_t)tune);
	pcbo_trace("   MOVE: c=%.012mm (%f) %mm;%mm\n", (rnd_coord_t)c, c, (rnd_coord_t)(v->point[0] + c * vx), (rnd_coord_t)(v->point[1] + c * vy));

	if (c < 0) {
		v->point[0] = vp->point[0];
		v->point[1] = vp->point[1];
		return 0; /* not enough room - cut back line to zero length so it gets removed */
	}

	if (inside)
		c = -c;
	ox = v->point[0]; oy = v->point[1];
	v->point[0] = rnd_round(v->point[0] + c * vx);
	v->point[1] = rnd_round(v->point[1] + c * vy);

	if ((ox == v->point[0]) && (oy == v->point[1]))
		return -1; /* too close, can't pull any more */

	return 0;
}

void rnd_pline_keepout_offs(rnd_pline_t *dst, const rnd_pline_t *src, rnd_coord_t offs)
{
	rnd_vnode_t *v;
	double offs2 = (double)offs * (double)offs;
	int negoffs = offs < 0;

	if (offs < 0)
		offs = -offs;

	if (offs2 > dst->area)
		return; /* degenerate case: polygon too small */

	/* there are two ways dst can get too close to src: */

	/* case #1: a point in dst is too close to a line in src */
	v = dst->head;
	do {
		rnd_rtree_it_t it;
		rnd_rtree_box_t pb;
		void *seg;
		int inside = 0;


		retry:;
		pb.x1 = v->point[0] - offs+1; pb.y1 = v->point[1] - offs+1;
		pb.x2 = v->point[0] + offs-1; pb.y2 = v->point[1] + offs-1;
		if (!negoffs)
			inside = rnd_poly_contour_inside(src, v->point);

		for(seg = rnd_rtree_first(&it, src->tree, &pb); seg != NULL; seg = rnd_rtree_next(&it)) {
			rnd_coord_t x1, y1, x2, y2;
			double dist, tune, prjx, prjy, dx, dy, ax, ay, dotp, prevx, prevy, prevl;

			rnd_polyarea_get_tree_seg(seg, &x1, &y1, &x2, &y2);
			dist = dist_line_to_pt(v->point[0], v->point[1], x1, y1, x2, y2, &dx, &dy);
			if ((offs2 - dist) > 10) {
				rnd_vector_t nv_;
				rnd_vnode_t *nv;

				/* calculate x0;y0 projected onto the line */
				ax = v->point[0] - x1;
				ay = v->point[1] - y1;
				dotp = ax * dx + ay * dy;
				prjx = x1 + dx * dotp;
				prjy = y1 + dy * dotp;
				pcbo_trace("dotp=%f dx=%f dy=%f res: %mm %mm inside=%d\n", dotp, dx, dy, (rnd_coord_t)prjx, (rnd_coord_t)prjy, inside);

				/* this is how much the point needs to be moved away from the line */
				if (inside)
					tune = offs + sqrt(dist);
				else
					tune = offs - sqrt(dist);
				if (tune < 5)
					continue;

				pcbo_trace("close: %mm;%mm to %mm;%mm %mm;%mm: tune=%.012mm prj: %mm;%mm\n", v->point[0], v->point[1], x1, y1, x2, y2, (rnd_coord_t)tune, (rnd_coord_t)prjx, (rnd_coord_t)prjy);
				pcbo_trace(" tune=%.012mm dist=%.012mm\n", (rnd_coord_t)tune, (rnd_coord_t)sqrt(dist));


				/* corner case: if next segment is parallel to what we are compesing to
				   (chamfed V with bottom horizontal being too close to target horizontal line),
				   do not insert a new point because that would retain the horizontal line
				   which can not be moved because it is parallel to the target line */
				prevx = v->prev->point[0] - v->point[0];
				prevy = v->prev->point[1] - v->point[1];
				prevl = sqrt(prevx*prevx + prevy*prevy);
				prevx /= prevl;
				prevy /= prevl;

				if (((prevx == dx) || (prevx == -dx)) && ((prevy == dy) || (prevy == -dy))) {
					/* apply direct shift of the whole parallel line */
					v->point[0] = rnd_round(v->point[0] + prjx * tune);
					v->point[1] = rnd_round(v->point[1] + prjy * tune);
					v->prev->point[0] = rnd_round(v->prev->point[0] + prjx * tune);
					v->prev->point[1] = rnd_round(v->prev->point[1] + prjy * tune);
					v = v->next;
					goto retry;
				}

				nv_[0] = v->point[0];
				nv_[1] = v->point[1];
				nv = rnd_poly_node_create(nv_);
				rnd_poly_vertex_include_force(v, nv);

				if (pull_back(v, v->prev, tune, dx, dy, prjx, prjy, inside) != 0) {
					rnd_poly_vertex_exclude(dst, nv);
					v = v->next;
					goto retry;
				}

				if (pull_back(nv, nv->next, tune, dx, dy, prjx, prjy, inside) != 0) {
					rnd_poly_vertex_exclude(dst, nv);
					v = v->next;
					goto retry;
				}

				v = v->next;
				goto retry;
			}
		}
	} while((v = v->next) != dst->head);

	/* case #2: a line in dst is too close to a point in src */


	/* cleanup: remove redundant points */
	v = dst->head;
	do {
		if ((v->prev->point[0] == v->point[0]) && (v->prev->point[1] == v->point[1])) {
			if (v->prev == dst->head) {
				rnd_vnode_t *nv = v->next;
				rnd_poly_vertex_exclude(dst, v);
				v = nv;
				continue;
			}
			rnd_poly_vertex_exclude(dst, v->prev);
		}
	} while((v = v->next) != dst->head);
}

