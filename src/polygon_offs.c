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

/* Polygon contour offset calculation */

#include "compat_misc.h"
#include "polygon_offs.h"

void pcb_polo_norm(double *nx, double *ny, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
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

void pcb_polo_edge_shift(double offs,
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

void pcb_polo_offs(double offs, pcb_polo_t *pcsh, long num_pts)
{
	long n;

	for(n = 0; n < num_pts; n++) {
		long np = warp(n-1, num_pts), nn1 = warp(n+1, num_pts), nn2 = warp(n+2, num_pts);
		pcb_polo_edge_shift(offs,
			&pcsh[n].x, &pcsh[n].y, pcsh[n].nx, pcsh[n].ny,
			&pcsh[nn1].x, &pcsh[nn1].y,
			pcsh[np].x, pcsh[np].y, pcsh[np].nx, pcsh[np].ny,
			pcsh[nn2].x, pcsh[nn2].y, pcsh[nn2].nx, pcsh[nn2].ny
		);
	}
}


void pcb_polo_norms(pcb_polo_t *pcsh, long num_pts)
{
	long n;

	for(n = 0; n < num_pts; n++) {
		long nn = warp(n+1, num_pts);
		pcb_polo_normd(&pcsh[n].nx, &pcsh[n].ny, pcsh[n].x, pcsh[n].y, pcsh[nn].x, pcsh[nn].y);
	}
}

double pcb_polo_2area(pcb_polo_t *pcsh, long num_pts)
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

pcb_pline_t *pcb_pline_dup_offset(const pcb_pline_t *src, pcb_coord_t offs)
{
	const pcb_vnode_t *v;
	pcb_vector_t tmp;
	pcb_pline_t *res = NULL;
	long num_pts, n;
	pcb_polo_t *pcsh;

	/* count corners */
	v = &src->head;
	num_pts = 0;
	do {
		num_pts++;
	} while((v = v->next) != &src->head);

	/* allocate the cache and copy all data */
	pcsh = malloc(sizeof(pcb_polo_t) * num_pts);
	for(n = 0, v = &src->head; n < num_pts; n++, v = v->next) {
		pcsh[n].x = v->point[0];
		pcsh[n].y = v->point[1];
		pcb_polo_norm(&pcsh[n].nx, &pcsh[n].ny, v->point[0], v->point[1], v->next->point[0], v->next->point[1]);
	}

	/* offset the cache */
	pcb_polo_offs(offs, pcsh, num_pts);


	/* create a new pline by copying the cache */
	tmp[0] = pcb_round(pcsh[0].x);
	tmp[1] = pcb_round(pcsh[0].y);
	res = pcb_poly_contour_new(tmp);
	for(n = 1; n < num_pts; n++) {
		tmp[0] = pcb_round(pcsh[n].x);
		tmp[1] = pcb_round(pcsh[n].y);
		pcb_poly_vertex_include(res->head.prev, pcb_poly_node_create(tmp));
	}

	free(pcsh);
	pcb_pline_keepout_offs(res, src, offs); /* avoid self-intersection */
	res->tree = pcb_poly_make_edge_tree(res);
	return res;
}

TODO("this should be coming from gengeo2d");
/* Return the square of distance between point x0;y0 and line x1;y1 - x2;y2 */
static double dist_line_to_pt(double x0, double y0, double x1, double y1, double x2, double y2, double *odx, double *ody, double *dd2)
{
	double tmp1, d2, dx, dy;

	dx = x2 - x1; dy = y2 - y1;
	tmp1 = dy*x0 - dx*y0 + x2*y1 - y2*x1;
	d2 = dx*dx + dy*dy;

	*dd2 = d2;
	*odx = dx;
	*ody = dy;
	return (tmp1 * tmp1) / d2;
}

/* Modify v, pulling it back toward vp so that the distance to line ldx;ldy is increased by tune */
PCB_INLINE void pull_back(pcb_vnode_t *v, const pcb_vnode_t *vp, double tune, double ldx, double ldy, double prjx, double prjy)
{
	double c, vx, vy, vlen, prx, pry, prlen;

	vx = vp->point[0] - v->point[0];
	vy = vp->point[1] - v->point[1];
	vlen = sqrt(vx*vx + vy*vy);
	vx /= vlen;
	vy /= vlen;

	prx = vp->point[0] - prjx;
	pry = vp->point[1] - prjy;
	prlen = sqrt(prx*prx + pry*pry);
	prx /= prlen;
	pry /= prlen;

	c = tune * (-pry * ldx + prx * ldy) / (ldy * vx - ldx * vy);
	pcb_printf("   vect: vx=%f;%f prx=%f;%f tune=%f\n", vx, vy, prx, pry, tune);
	pcb_printf("   MOVE: %f %mm;%mm\n", c, (pcb_coord_t)(v->point[0] + c * vx), (pcb_coord_t)(v->point[1] + c * vy));

	v->point[0] += c * vx;
	v->point[1] += c * vy;
}

void pcb_pline_keepout_offs(pcb_pline_t *dst, const pcb_pline_t *src, pcb_coord_t offs)
{
	const pcb_vnode_t *v;
	double offs2 = (double)offs * (double)offs;

	/* there are two ways dst can get too close to src: */

	/* case #1: a point in dst is too close to a line in src */
	v = &dst->head;
	do {
		pcb_rtree_it_t it;
		pcb_rtree_box_t pb;
		void *seg;


		retry:;
		pb.x1 = v->point[0] - offs+1; pb.y1 = v->point[1] - offs+1;
		pb.x2 = v->point[0] + offs-1; pb.y2 = v->point[1] + offs-1;
		for(seg = pcb_rtree_first(&it, src->tree, &pb); seg != NULL; seg = pcb_rtree_next(&it)) {
			pcb_coord_t x1, y1, x2, y2;
			double dist, tune, prjx, prjy, dx, dy, dlen, dlen2, ax, ay, dotp;

			pcb_polyarea_get_tree_seg(seg, &x1, &y1, &x2, &y2);
			dist = dist_line_to_pt(v->point[0], v->point[1], x1, y1, x2, y2, &dx, &dy, &dlen2);
			if ((offs2 - dist) > 10) {
				pcb_vector_t nv_;
				pcb_vnode_t *nv;

				/* calculate x0;y0 projected onto the line */
				ax = v->point[0] - x1;
				ay = v->point[1] - y1;
				dlen = sqrt(dlen2);
				dx /= dlen;
				dy /= dlen;
				dotp = ax * dx + ay * dy;
/*			printf("  dotp=%f dx=%f dy=%f\n", dotp, dx, dy);*/
				prjx = x1 + dx * dotp;
				prjy = y1 + dy * dotp;

				/* this is how much the point needs to be moved away from the line */
				tune = offs - sqrt(dist);
				pcb_printf("close: %mm;%mm to %mm;%mm %mm;%mm: tune=%mm prj: %mm;%mm\n", v->point[0], v->point[1], x1, y1, x2, y2, (pcb_coord_t)tune, (pcb_coord_t)prjx, (pcb_coord_t)prjy);


				nv_[0] = pcb_round(v->point[0]);
				nv_[1] = pcb_round(v->point[1]);
				nv = pcb_poly_node_create(&nv_);
				pcb_poly_vertex_include_force(v, nv);

				pull_back(v, v->prev, tune, dx, dy, prjx, prjy);
				pull_back(nv, nv->next, tune, dx, dy, prjx, prjy);

/*				v = v->next;*/
/*				goto retry;*/
			}
		}
	} while((v = v->next) != &dst->head);

	/* case #2: a line in dst is too close to a point in src */

}

