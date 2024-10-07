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

/* included from obj_pstk_proto.c; entry points:

   pcb_pstk_shape_crescent_init()
   pcb_pstk_shape_crescent_render()

*/

#include "search.h"

typedef enum {
	CRES_ST_SHAPE_IN_HOLE = 1, /* shape is fully within hole (shape is fully "drilled away") */
	CRES_ST_HOLE_IN_SHAPE,     /* hole is fully within shape ("donut", maybe assymetric) */
	CRES_ST_CROSSING,          /* hole and shape have crossing contour */
	CRES_ST_DISJOINT           /* hole and shape have no common point */
} cres_st_t;

/* adjust st if (shape, hole) order was swapped for the call */
#define CRESS_INVERT_ST(st) \
do { \
	if (st == CRES_ST_SHAPE_IN_HOLE) st = CRES_ST_HOLE_IN_SHAPE; \
	else if (st == CRES_ST_HOLE_IN_SHAPE) st = CRES_ST_SHAPE_IN_HOLE;\
} while(0)



/*** low level "geo vs. geo to state" converters ***/
RND_INLINE cres_st_t cress_geo_circ_circ(rnd_coord_t scx, rnd_coord_t scy, double sr, rnd_coord_t hcx, rnd_coord_t hcy, double hr)
{
	if ((scx == hcx) && (scy == hcy)) {
		/* optimisation for a common case when centers match */
		if (hr >= sr) return CRES_ST_SHAPE_IN_HOLE;
		return CRES_ST_HOLE_IN_SHAPE;
	}
	else if (hr >= sr) {
		double dist2, scx2, scy2, rdmin, rdmax;

		/* transform everything into holes's center */
		if ((hcx != 0) || (hcy != 0)) {
			scx -= hcx;
			scy -= hcy;
			hcx = hcy = 0;
		}

		scx2 = (double)scx * (double)scx;
		scy2 = (double)scy * (double)scy;
		dist2 = scx2 + scy2;

		rdmin = hr - sr;
		if (dist2 <= rdmin*rdmin)
			return CRES_ST_SHAPE_IN_HOLE;

		rdmax = hr + sr;
		if (dist2 >= rdmax*rdmax)
			return CRES_ST_DISJOINT;

		return CRES_ST_CROSSING;
	}
	else { /* (hr < sr): swap them so "hole" is the bigger */
		cres_st_t st = cress_geo_circ_circ(hcx, hcy, hr, scx, scy, sr);
		CRESS_INVERT_ST(st);
		return st;
	}
}

/* Compute the normal vector of a line shape; return line length */
RND_INLINE double cress_geo_line_normal(pcb_pstk_shape_t *line, double *nx, double *ny)
{
	double len, dx, dy, dx2, dy2;

	assert(line->shape = PCB_PSSH_LINE);
	dx = line->data.line.x2 - line->data.line.x1;
	dy = line->data.line.y2 - line->data.line.y1;
	if ((dx == 0) && (dy == 0)) {
		zero_length:;
		*nx = *ny = 0;
		return 0.0;
	}
	dx2 = dx*dx;
	dy2 = dy*dy;
	len = dx2+dy2;
	if (len == 0) goto zero_length;

	len = sqrt(len);
	*nx = -dy / len;
	*ny = dx / len;

	return len;
}

/* Return the line of a square end cap of a line shape; end is 1 for x1;y2
   or 2 for x2;y2. If nxp;nyp are non-NULL they are the normal of the line;
   input value 0;0 indicates it is not yet calculated. Result is written to
   ex1;ey1 and ex2;ey2 */
RND_INLINE void cress_geo_line_sqcap_line(pcb_pstk_shape_t *line, int end, double *ex1, double *ey1, double *ex2, double *ey2, double *nxp, double *nyp)
{
	double nx, ny, ex, ey, r;

	assert(line->shape = PCB_PSSH_LINE);
	assert((end == 1) || (end == 2));

	/* ensure line normal vectors */
	if ((nxp == NULL) || (nyp == NULL) || ((*nxp == 0) && (*nyp == 0))) {
		cress_geo_line_normal(line, &nx, &ny);
		if (nxp != NULL) *nxp = nx;
		if (nyp != NULL) *nyp = ny;
	}
	else {
		nx = *nxp;
		ny = *nyp;
	}

	switch(end) {
		case 1: ex = line->data.line.x1; ey = line->data.line.y1; break;
		case 2: ex = line->data.line.x2; ey = line->data.line.y2; break;
	}

	/* invalid */
	if ((nx == 0) && (ny == 0)) {
		*ex1 = *ex2 = ex;
		*ey1 = *ey2 = ey;
		return;
	}

	r = line->data.line.thickness / 2.0;
	*ex1 = ex + nx*r;
	*ey1 = ey + ny*r;
	*ex2 = ex - nx*r;
	*ey2 = ey - ny*r;
}

/* Returns whether px;py is inside a circle (not on perimeter and not outside);
   cr2 is circle radius squared */
RND_INLINE int cress_geo_pt_insied_circle2(double px, double py, double cx, double cy, double cr2)
{
	double dx = cx - px, dy = cy - py;
	return (dx*dx + dy*dy) < cr2;
}

/* Returns whether a circle is crossing a line segment (contour cross but no touch) */
RND_INLINE int cress_geo_circ_crossing_line(double cx, double cy, double cr, double lx1, double ly1, double lx2, double ly2)
{
	double offs, cr2 = cr*cr, dist2;
	int in1, in2;

	in1 = cress_geo_pt_insied_circle2(lx1, ly1, cx, cy, cr2);
	in2 = cress_geo_pt_insied_circle2(lx2, ly2, cx, cy, cr2);

	if (in1 && in2) return 0; /* the whole line is inside */
	if (in1 != in2) return 1; /* one endpoint in, another out */

	/* both endpoints out: check if circle crosses the middle section of the line */
	dist2 = pcb_geo_point_line_dist2(cx, cy, lx1, ly1, lx2, ly2, &offs, NULL, NULL);
	if (dist2 >= cr2) /* circle is too far */
		return 0;

	return 1; /* circle is close enough so it must have crossed the middle section */
}

/*** "shape vs. shape to state" converters ***/
RND_INLINE cres_st_t cress_st_circ_circ(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	assert(shape->shape == PCB_PSSH_CIRC);
	assert(hole->shape == PCB_PSSH_CIRC);
	return cress_geo_circ_circ(shape->data.circ.x, shape->data.circ.y, shape->data.circ.dia/2.0, hole->data.circ.x, hole->data.circ.y, hole->data.circ.dia/2.0);
}

RND_INLINE cres_st_t cress_st_circ_line(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	double dist, dist2, adist2, cr = shape->data.circ.dia/2.0, hr = hole->data.line.thickness/2.0;

	assert(shape->shape == PCB_PSSH_CIRC);
	assert(hole->shape == PCB_PSSH_LINE);


	if (cr <= hr) {
		/* check if small shape circle is in hole line */
		double offs, prx, pry, ex1, ey1, ex2, ey2;

		/* actual distance between circle center and line centerline */
		adist2 = pcb_geo_point_line_dist2(shape->data.circ.x, shape->data.circ.y, hole->data.line.x1, hole->data.line.y1, hole->data.line.x2, hole->data.line.y2, &offs, &prx, &pry);

		/* shape center too far */
		dist = hr + cr;
		dist2 = dist*dist;
		if (adist2 >= dist2)
			return CRES_ST_DISJOINT;

		/* shape center too close: it's surely in */
		dist = hr - cr;
		dist2 = dist*dist;
		if (adist2 <= dist2)
			return CRES_ST_SHAPE_IN_HOLE;

		/* round cap is the regular case; if the circle is not too far or too close, it must be crossing */
		if (!hole->data.line.square)
			return CRES_ST_CROSSING;

		/* square cap: if circle is within the normal range it is crossing the side of the line */
		if ((offs >= 0) && (offs <= 1))
			return CRES_ST_CROSSING;

		/* square cap: the circle is beyond normal range; it's crossing only if it is
		   crossing and endcap line; else it must be out because the projection is not
		   within range */
		cress_geo_line_sqcap_line(hole, (offs < 0) ? 1 : 2, &ex1, &ey1, &ex2, &ey2, NULL, NULL);
		if (cress_geo_circ_crossing_line(shape->data.circ.x, shape->data.circ.y, cr, ex1, ey1, ex2, ey2))
			return CRES_ST_CROSSING;

		return CRES_ST_DISJOINT;
	}
	else {
		/* check if small hole line is in big shape circle */
		int in1, in2;
		double cr2 = cr*cr;

		in1 = cress_geo_pt_insied_circle2(hole->data.line.x1, hole->data.line.y1, shape->data.circ.x, shape->data.circ.y, cr2);
		in2 = cress_geo_pt_insied_circle2(hole->data.line.x2, hole->data.line.y2, shape->data.circ.x, shape->data.circ.y, cr2);

		if (in1 != in2)
			return CRES_ST_CROSSING;

		/* check for line end cap vs. circle crossings */
		if (hole->data.line.square) {
			/* square: crossing if end cap lines are crossing the circle */
			double ex1, ey1, ex2, ey2;
			cress_geo_line_sqcap_line(hole, 1, &ex1, &ey1, &ex2, &ey2, NULL, NULL);
			if (cress_geo_circ_crossing_line(shape->data.circ.x, shape->data.circ.y, cr, ex1, ey1, ex2, ey2))
				return CRES_ST_CROSSING;
			cress_geo_line_sqcap_line(hole, 2, &ex1, &ey1, &ex2, &ey2, NULL, NULL);
			if (cress_geo_circ_crossing_line(shape->data.circ.x, shape->data.circ.y, cr, ex1, ey1, ex2, ey2))
				return CRES_ST_CROSSING;
		}
		else {
			if (cress_geo_circ_circ(shape->data.circ.x, shape->data.circ.y, cr, hole->data.line.x1, hole->data.line.y1, hr) == CRES_ST_CROSSING)
				return CRES_ST_CROSSING;
			if (cress_geo_circ_circ(shape->data.circ.x, shape->data.circ.y, cr, hole->data.line.x2, hole->data.line.y2, hr) == CRES_ST_CROSSING)
				return CRES_ST_CROSSING;
		}

		if (in1 && in2) {
			/* both ends are in, plus neither endcap crossed the circle */
			return CRES_ST_HOLE_IN_SHAPE;
		}

		/* small line is fully outside of the circle; the only way they could cross
		   is the cricle crossing the side of the line */

		/* actual distance between circle center and line centerline */
		adist2 = pcb_geo_point_line_dist2(shape->data.circ.x, shape->data.circ.y, hole->data.line.x1, hole->data.line.y1, hole->data.line.x2, hole->data.line.y2, NULL, NULL, NULL);
		dist = cr - hr;
		dist2 = dist*dist;
		if (adist2 < dist2)
			return CRES_ST_CROSSING;

		return CRES_ST_DISJOINT;
	}
	abort(); /* can't get here */
}

RND_INLINE cres_st_t cress_st_line_line(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	
}

RND_INLINE cres_st_t cress_st_line_poly(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	
}

RND_INLINE cres_st_t cress_st_circ_poly(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	
}

RND_INLINE cres_st_t cress_st_poly_poly(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	
}

/* assume ->hfullcover and ->hcrescnet are set to 0 before the call */
RND_INLINE void cress_output(pcb_pstk_shape_t *dst, cres_st_t st)
{
	switch(st) {
		case CRES_ST_SHAPE_IN_HOLE: dst->hfullcover = 1; break; /* drilled away the whole shape */
		case CRES_ST_HOLE_IN_SHAPE: break; /* the normal via case */
		case CRES_ST_CROSSING: dst->hcrescent = 1; break;
		case CRES_ST_DISJOINT: assert(dst->hconn == 0); TODO("could set it instead"); break;
	}
}

RND_INLINE void cres_class_poly_in_poly(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_poly_poly(shape, hole);
	cress_output(dst, st);
}


RND_INLINE void cres_class_poly_in_line(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_line_poly(hole, shape);
	CRESS_INVERT_ST(st);
	cress_output(dst, st);
}

RND_INLINE void cres_class_poly_in_circ(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_circ_poly(hole, shape);
	CRESS_INVERT_ST(st);
	cress_output(dst, st);
}

RND_INLINE void cres_class_line_in_poly(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_line_poly(shape, hole);
	cress_output(dst, st);
}

RND_INLINE void cres_class_line_in_line(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_line_line(shape, hole);
	cress_output(dst, st);
}

RND_INLINE void cres_class_line_in_circ(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_circ_line(hole, shape);
	CRESS_INVERT_ST(st);
	cress_output(dst, st);
}

RND_INLINE void cres_class_circ_in_poly(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_circ_poly(shape, hole);
	cress_output(dst, st);
}


RND_INLINE void cres_class_circ_in_line(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_circ_line(shape, hole);
	cress_output(dst, st);
}

RND_INLINE void cres_class_circ_in_circ(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	cres_st_t st = cress_st_circ_circ(shape, hole);
	cress_output(dst, st);
}


/* Initialize ->hfullcover, ->hcrescent and ->pa_crescent; called only from
   pcb_pstk_proto_update() */
RND_INLINE void pcb_pstk_shape_crescent_init(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *hole, pcb_pstk_proto_t *proto, pcb_pstk_t *dummy_ps)
{
	int cres, olap;

	dst->hcrescent = dst->hfullcover = 0;
	if (hole->shape == PCB_PSSH_HSHADOW) {
		assert("invalid slot shape");
		return;
	}

	switch(dst->shape) {
		case PCB_PSSH_POLY:
			switch(hole->shape) {
				case PCB_PSSH_POLY: cres_class_poly_in_poly(dst, dst, hole); break;
				case PCB_PSSH_LINE: cres_class_poly_in_line(dst, dst, hole); break;
				case PCB_PSSH_CIRC: cres_class_poly_in_circ(dst, dst, hole); break;
			}
			break;
		case PCB_PSSH_LINE:
			switch(hole->shape) {
				case PCB_PSSH_POLY: cres_class_line_in_poly(dst, dst, hole); break;
				case PCB_PSSH_LINE: cres_class_line_in_line(dst, dst, hole); break;
				case PCB_PSSH_CIRC: cres_class_line_in_circ(dst, dst, hole); break;
			}
			break;
		case PCB_PSSH_CIRC:
			switch(hole->shape) {
				case PCB_PSSH_POLY: cres_class_circ_in_poly(dst, dst, hole); break;
				case PCB_PSSH_LINE: cres_class_circ_in_line(dst, dst, hole); break;
				case PCB_PSSH_CIRC: cres_class_circ_in_circ(dst, dst, hole); break;
			}
			break;
		case PCB_PSSH_HSHADOW:
			assert("caller should have handled this");
			break;
	}
}


rnd_polyarea_t *pcb_pstk_shape_crescent_render(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *hole)
{
	TODO("implement me");
	return NULL;
}
