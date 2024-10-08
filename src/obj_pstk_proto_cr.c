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
#include "find.h"

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

/* returns if line lx1;ly1 lx2;ly2 crosses the outer contour of either round
   end cap of ls (lsr is ls's radius, ls_quad is the quadrangle of ls body) */
RND_INLINE int cress_geo_line_roundcap_crossing(double lx1, double ly1, double lx2, double ly2, pcb_pstk_shape_t *ls, double lsr, rnd_point_t ls_quad[4])
{
	rnd_point_t pt;
	int in1, in2;
	assert(ls->shape == PCB_PSSH_LINE);
	assert(ls->data.line.square = 0);

	/* if both ends of the line are within the body of the line any circle
	   intersections are inside too, which means the line is not intersecting
	   the outer contour of the round cap but the invisible inner contour */
	
	pt.X = rnd_round(lx1); pt.Y = rnd_round(ly1);
	in1 = pcb_geo_pt_in_quadrangle(ls_quad, &pt);

	pt.X = rnd_round(lx2); pt.Y = rnd_round(ly2);
	in2 = pcb_geo_pt_in_quadrangle(ls_quad, &pt);

	if (in1 && in2)
		return 0;

	/* Note: in theory an in-out line may cross the invisible section of the cap,
	   but that line would also need to cross the side of the line. Since
	   this function is called in a special case only, when side-side intersections
	   are already checked, we won't need to deal with that here */
	if (cress_geo_circ_crossing_line(ls->data.line.x1, ls->data.line.y1, lsr, lx1, ly1, lx2, ly2))
		return 1;
	if (cress_geo_circ_crossing_line(ls->data.line.x2, ls->data.line.y2, lsr, lx1, ly1, lx2, ly2))
		return 1;

	return 0;
}

/* Returns if pt is inside a line shape; lsr2 is square(line radius) */
RND_INLINE int cress_geo_pt_in_line(rnd_coord_t ptx, rnd_coord_t pty, pcb_pstk_shape_t *ls, double lsr2, rnd_point_t ls_quad[4])
{
	rnd_point_t pt;

	assert(ls->shape == PCB_PSSH_LINE);

	/* check if it's in the body of the line */
	pt.X = ptx; pt.Y = pty;
	if (pcb_geo_pt_in_quadrangle(ls_quad, &pt))
		return 1;

	/* check round end caps */
	if (!ls->data.line.square) {
		if (cress_geo_pt_insied_circle2(ptx, pty, ls->data.line.x1, ls->data.line.y1, lsr2))
			return 1;
		if (cress_geo_pt_insied_circle2(ptx, pty, ls->data.line.x2, ls->data.line.y2, lsr2))
			return 1;
	}

	return 0;
}

/* Returns whether a's round cap and b's round cap are crossing. Note:
   half of the cap's circle is within the body of the line and crossing is
   not valid in that section. So even if end cap circles cross there is a
   special case when the cap of a tiny line in a large line crosses the
   invisible section of the large line's cap; that happens only if the
   tiny line's cap center is within the body of the large line */
RND_INLINE int cress_geo_roundcap_roundcap_crossing(pcb_pstk_shape_t *a, double ar, rnd_point_t qa[4], pcb_pstk_shape_t *b, double br, rnd_point_t qb[4])
{
	rnd_point_t apt, bpt;

	apt.X = a->data.line.x1; apt.Y = a->data.line.y1;
	bpt.X = b->data.line.x1; bpt.Y = b->data.line.y1;
	if (cress_geo_circ_circ(apt.X, apt.Y, ar, bpt.X, bpt.Y, br) == CRES_ST_CROSSING) {
		if (ar < br) {
			if (!pcb_geo_pt_in_quadrangle(qa, &bpt))
				return 1;
		}
		else {
			if (!pcb_geo_pt_in_quadrangle(qb, &apt))
				return 1;
		}
	}

	apt.X = a->data.line.x1; apt.Y = a->data.line.y1;
	bpt.X = b->data.line.x2; bpt.Y = b->data.line.y2;
	if (cress_geo_circ_circ(apt.X, apt.Y, ar, bpt.X, bpt.Y, br) == CRES_ST_CROSSING) {
		if (ar < br) {
			if (!pcb_geo_pt_in_quadrangle(qa, &bpt))
				return 1;
		}
		else {
			if (!pcb_geo_pt_in_quadrangle(qb, &apt))
				return 1;
		}
	}

	apt.X = a->data.line.x2; apt.Y = a->data.line.y2;
	bpt.X = b->data.line.x1; bpt.Y = b->data.line.y1;
	if (cress_geo_circ_circ(apt.X, apt.Y, ar, bpt.X, bpt.Y, br) == CRES_ST_CROSSING) {
		if (ar < br) {
			if (!pcb_geo_pt_in_quadrangle(qa, &bpt))
				return 1;
		}
		else {
			if (!pcb_geo_pt_in_quadrangle(qb, &apt))
				return 1;
		}
	}

	apt.X = a->data.line.x2; apt.Y = a->data.line.y2;
	bpt.X = b->data.line.x2; bpt.Y = b->data.line.y2;
	if (cress_geo_circ_circ(apt.X, apt.Y, ar, bpt.X, bpt.Y, br) == CRES_ST_CROSSING) {
		if (ar < br) {
			if (!pcb_geo_pt_in_quadrangle(qa, &bpt))
				return 1;
		}
		else {
			if (!pcb_geo_pt_in_quadrangle(qb, &apt))
				return 1;
		}
	}

	return 0;
}

/* returns whether one of a's square cap and b's round cap are crossing */
RND_INLINE int cress_geo_square_roundcap_crossing_(pcb_pstk_shape_t *a, double ar, rnd_point_t qa[4], int aend, pcb_pstk_shape_t *b, double br2, rnd_point_t qb[4])
{
	int in1, in2, n;
	rnd_coord_t lx1, ly1, lx2, ly2, cx, cy;
	rnd_point_t pt;
	double adist2;

	assert((aend == 1) || (aend == 2));

	if (aend == 1) {
		lx1 = qa[0].X; ly1 = qa[0].Y;
		lx2 = qa[3].X; ly2 = qa[3].Y;
	}
	else {
		lx1 = qa[0].X; ly1 = qa[0].Y;
		lx2 = qa[3].X; ly2 = qa[3].Y;
	}

	/* if both ends of 'a' square cap are within the body of the (larger) 'b',
	   that means all it could do is intersecting the invisible section of the
	   round cap circle of 'b' */
	pt.X = lx1; pt.Y = ly1;
	in1 = pcb_geo_pt_in_quadrangle(qb, &pt);
	pt.X = lx2; pt.Y = ly2;
	in2 = pcb_geo_pt_in_quadrangle(qb, &pt);
	if (in1 && in2)
		return 0;

	/* check both end circles of 'b' for crossing */
	for(n = 1; n <= 2; n++) {
		if (n == 1) {
			cx = b->data.line.x1;
			cy = b->data.line.y1;
		}
		else {
			cx = b->data.line.x2;
			cy = b->data.line.y2;
		}

		/* if one end in and other end is out, they are crossing */
		in1 = cress_geo_pt_insied_circle2(lx1, ly1, cx, cy, br2);
		in2 = cress_geo_pt_insied_circle2(lx2, ly2, cx, cy, br2);
		if (in1 != in2)
			return 1;

		/* check circle vs. line middle section crossing (line is closer to circle than circle radius) */
		adist2 = pcb_geo_point_line_dist2(cx, cy, lx1, ly1, lx2, ly2, NULL, NULL, NULL);
		if (adist2 < br2)
			return 1;
	}

	return 0;
}

RND_INLINE int cress_geo_square_roundcap_crossing(pcb_pstk_shape_t *a, double ar, rnd_point_t qa[4], pcb_pstk_shape_t *b, double br, rnd_point_t bs[4])
{
	double br2 = br*br;
	if (cress_geo_square_roundcap_crossing_(a, ar, qa, 1, b, br2, bs)) return 1;
	if (cress_geo_square_roundcap_crossing_(a, ar, qa, 2, b, br2, bs)) return 1;
	return 0;
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
	double sx1 = shape->data.line.x1, sy1 = shape->data.line.y1;
	double sx2 = shape->data.line.x2, sy2 = shape->data.line.y2;
	double hx1 = hole->data.line.x1, hy1 = hole->data.line.y1;
	double hx2 = hole->data.line.x2, hy2 = hole->data.line.y2;
	double sr = shape->data.line.thickness / 2.0, hr = hole->data.line.thickness / 2.0;
	double sax1, say1, sax2, say2, sbx1, sby1, sbx2, sby2, snx, sny;
	double hax1, hay1, hax2, hay2, hbx1, hby1, hbx2, hby2, hnx, hny;
	rnd_point_t qh[4], qs[4];

	assert(shape->shape == PCB_PSSH_LINE);
	assert(hole->shape == PCB_PSSH_LINE);

	/* precompute side lines: sa and sb for side, ha and hb for hole */
	cress_geo_line_normal(shape, &snx, &sny);
	cress_geo_line_normal(hole, &hnx, &hny);
	sax1 = sx1 + sr * snx; say1 = sy1 + sr * sny;
	sax2 = sx2 + sr * snx; say2 = sy2 + sr * sny;
	sbx1 = sx1 - sr * snx; sby1 = sy1 - sr * sny;
	sbx2 = sx2 - sr * snx; sby2 = sy2 - sr * sny;
	hax1 = hx1 + hr * hnx; hay1 = hy1 + hr * hny;
	hax2 = hx2 + hr * hnx; hay2 = hy2 + hr * hny;
	hbx1 = hx1 - hr * hnx; hby1 = hy1 - hr * hny;
	hbx2 = hx2 - hr * hnx; hby2 = hy2 - hr * hny;


	/* First check if they are crossing */
	/* check side-side crossings */
	if (pcb_geo_line_line(sax1, say1, sax2, say2, hax1, hay1, hax2, hay2))
		return CRES_ST_CROSSING;
	if (pcb_geo_line_line(sax1, say1, sax2, say2, hbx1, hby1, hbx2, hby2))
		return CRES_ST_CROSSING;
	if (pcb_geo_line_line(sbx1, sby1, sbx2, sby2, hax1, hay1, hax2, hay2))
		return CRES_ST_CROSSING;
	if (pcb_geo_line_line(sbx1, sby1, sbx2, sby2, hbx1, hby1, hbx2, hby2))
		return CRES_ST_CROSSING;

	qh[0].X = rnd_round(hax1); qh[0].Y = rnd_round(hay1);
	qh[1].X = rnd_round(hax2); qh[1].Y = rnd_round(hay2);
	qh[2].X = rnd_round(hbx2); qh[2].Y = rnd_round(hby2);
	qh[3].X = rnd_round(hbx1); qh[3].Y = rnd_round(hby1);

	qs[0].X = rnd_round(sax1); qs[0].Y = rnd_round(say1);
	qs[1].X = rnd_round(sax2); qs[1].Y = rnd_round(say2);
	qs[2].X = rnd_round(sbx2); qs[2].Y = rnd_round(sby2);
	qs[3].X = rnd_round(sbx1); qs[3].Y = rnd_round(sby1);

	/* check endcap-side crossings; square cap line endcap can't cross without
	   a side-side crossing */
	if (!hole->data.line.square) {
		if (cress_geo_line_roundcap_crossing(sax1, say1, sax2, say2, hole, hr, qh))
			return CRES_ST_CROSSING;
		if (cress_geo_line_roundcap_crossing(sbx1, sby1, sbx2, sby2, hole, hr, qh))
			return CRES_ST_CROSSING;
	}
	if (!shape->data.line.square) {
		if (cress_geo_line_roundcap_crossing(hax1, hay1, hax2, hay2, shape, sr, qs))
			return CRES_ST_CROSSING;
		if (cress_geo_line_roundcap_crossing(hbx1, hby1, hbx2, hby2, shape, sr, qs))
			return CRES_ST_CROSSING;
	}

	/* check endcap-endcap crossings */
	if (!shape->data.line.square && !hole->data.line.square) {
		/* round round */
		if (cress_geo_roundcap_roundcap_crossing(hole, hr, qh, shape, sr, qs))
			return CRES_ST_CROSSING;
	}
	else if (shape->data.line.square && !hole->data.line.square) {
		/* square round */
		if (cress_geo_square_roundcap_crossing(hole, hr, qh, shape, sr, qs))
			return CRES_ST_CROSSING;
	}
	else if (!shape->data.line.square && hole->data.line.square) {
		/* round square */
		if (cress_geo_square_roundcap_crossing(shape, sr, qs, hole, hr, qh))
			return CRES_ST_CROSSING;
	}
	else {
		/* square-sqaure: it's not possible to have a crossing without the sides also crossing */
	}


	/* They are not crossing so if both ends are within the other line that
	   means inside. If there are no crossing this also means either both
	   or neither ends are inside so it's enough to check one end */
	if (cress_geo_pt_in_line(shape->data.line.x1, shape->data.line.y1, hole, hr*hr, qh))
		return CRES_ST_SHAPE_IN_HOLE;

	if (cress_geo_pt_in_line(hole->data.line.x1, hole->data.line.y1, shape, sr*sr, qs))
		return CRES_ST_HOLE_IN_SHAPE;

	return CRES_ST_DISJOINT;
}

RND_INLINE cres_st_t cress_st_line_poly(pcb_pstk_shape_t *shape, pcb_pstk_shape_t *hole)
{
	rnd_vnode_t *vn;
	double sr = shape->data.line.thickness / 2.0;
	double sx1 = shape->data.line.x1, sy1 = shape->data.line.y1;
	double sx2 = shape->data.line.x2, sy2 = shape->data.line.y2;
	double snx, sny, sax1, say1, sax2, say2, sbx1, sby1, sbx2, sby2;
	rnd_vector_t v;
	rnd_point_t qs[4];

	assert(shape->shape == PCB_PSSH_LINE);
	assert(hole->shape == PCB_PSSH_POLY);

	if (hole->data.poly.pa == NULL)
		return CRES_ST_DISJOINT;

	/* generate side coords: sa and sb */
	cress_geo_line_normal(shape, &snx, &sny);
	sax1 = sx1 + sr * snx; say1 = sy1 + sr * sny;
	sax2 = sx2 + sr * snx; say2 = sy2 + sr * sny;
	sbx1 = sx1 - sr * snx; sby1 = sy1 - sr * sny;
	sbx2 = sx2 - sr * snx; sby2 = sy2 - sr * sny;

	/* first check for crossing; simplepoly: enough to check a single island */
	vn = hole->data.poly.pa->contours->head;
	do {
		/* check for side crossings */
		if (pcb_geo_line_line(sax1, say1, sax2, say2, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
			return CRES_ST_CROSSING;
		if (pcb_geo_line_line(sbx1, sby1, sbx2, sby2, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
			return CRES_ST_CROSSING;

		/* check end cap */
		if (shape->data.line.square) {
			/* a spike of the poly can hit the flat end cap without touching any sides... */
			if (pcb_geo_line_line(sax1, say1, sbx1, sby1, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
				return CRES_ST_CROSSING;
			if (pcb_geo_line_line(sax2, say2, sbx2, sby2, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
				return CRES_ST_CROSSING;
		}
		else {
			/* round cap - check full circles */
			if (cress_geo_circ_crossing_line(sx1, sy1, sr, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
				return CRES_ST_CROSSING;
			if (cress_geo_circ_crossing_line(sx2, sy2, sr, vn->point[0], vn->point[1], vn->next->point[0], vn->next->point[0]))
				return CRES_ST_CROSSING;
		}
	} while((vn = vn->next) != hole->data.poly.pa->contours->head);

	/* there's no crossing; if any of the line is in the poly, the whole line is in the poly */
	v[0] = shape->data.line.x1; v[1] = shape->data.line.y1;
	if (rnd_poly_contour_inside(hole->data.poly.pa->contours, v))
		return CRES_ST_SHAPE_IN_HOLE;

	/* there's no crossing; if any of the poly is in the line, the whole poly is in the line */
	qs[0].X = rnd_round(sax1); qs[0].Y = rnd_round(say1);
	qs[1].X = rnd_round(sax2); qs[1].Y = rnd_round(say2);
	qs[2].X = rnd_round(sbx2); qs[2].Y = rnd_round(sby2);
	qs[3].X = rnd_round(sbx1); qs[3].Y = rnd_round(sby1);

	if (cress_geo_pt_in_line(vn->point[0], vn->point[1], shape, sr*sr, qs))
		return CRES_ST_HOLE_IN_SHAPE;

	return CRES_ST_DISJOINT;
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
