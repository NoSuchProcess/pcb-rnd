/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2010 Thomas Nau
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

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "global_typedefs.h"
#include "polyarea.h"
#include "math_helper.h"

#include "polygon1_gen.h"

/* kept to ensure nanometer compatibility */
#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

static double rotate_circle_seg[4];
int rotate_circle_seg_inited = 0;

static void init_rotate_cache(void)
{
	if (!rotate_circle_seg_inited) {
		double cos_ang = cos(2.0 * M_PI / PCB_POLY_CIRC_SEGS_F);
		double sin_ang = sin(2.0 * M_PI / PCB_POLY_CIRC_SEGS_F);

		rotate_circle_seg[0] = cos_ang;
		rotate_circle_seg[1] = -sin_ang;
		rotate_circle_seg[2] = sin_ang;
		rotate_circle_seg[3] = cos_ang;
		rotate_circle_seg_inited = 1;
	}
}

pcb_polyarea_t *pcb_poly_from_contour(pcb_pline_t * contour)
{
	pcb_polyarea_t *p;
	pcb_poly_contour_pre(contour, pcb_true);
	assert(contour->Flags.orient == PCB_PLF_DIR);
	if (!(p = pcb_polyarea_create()))
		return NULL;
	pcb_polyarea_contour_include(p, contour);
	assert(pcb_poly_valid(p));
	return p;
}

#define ARC_ANGLE 5
static pcb_polyarea_t *ArcPolyNoIntersect(pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t astart, pcb_angle_t adelta, pcb_coord_t thick, int end_caps)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v, v2;
	int i, segs;
	double ang, da, rx, ry;
	long half;
	double radius_adj;
	pcb_coord_t edx, edy, endx1, endx2, endy1, endy2;

	if (thick <= 0)
		return NULL;
	if (adelta < 0) {
		astart += adelta;
		adelta = -adelta;
	}
	half = (thick + 1) / 2;

	pcb_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &endx1, &endy1);
	pcb_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &endx2, &endy2);

	/* start with inner radius */
	rx = MAX(width - half, 0);
	ry = MAX(height - half, 0);
	segs = 1;
	if (thick > 0)
		segs = MAX(segs, adelta * M_PI / 360 *
							 sqrt(sqrt((double) rx * rx + (double) ry * ry) / PCB_POLY_ARC_MAX_DEVIATION / 2 / thick));
	segs = MAX(segs, adelta / ARC_ANGLE);

	ang = astart;
	da = (1.0 * adelta) / segs;
	radius_adj = (M_PI * da / 360) * (M_PI * da / 360) / 2;
	v[0] = cx - rx * cos(ang * PCB_M180);
	v[1] = cy + ry * sin(ang * PCB_M180);
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return 0;
	for (i = 0; i < segs - 1; i++) {
		ang += da;
		v[0] = cx - rx * cos(ang * PCB_M180);
		v[1] = cy + ry * sin(ang * PCB_M180);
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	}
	/* find last point */
	ang = astart + adelta;
	v[0] = cx - rx * cos(ang * PCB_M180) * (1 - radius_adj);
	v[1] = cy + ry * sin(ang * PCB_M180) * (1 - radius_adj);

	/* add the round cap at the end */
	if (end_caps)
		pcb_poly_frac_circle(contour, endx2, endy2, v, 2);

	/* and now do the outer arc (going backwards) */
	rx = (width + half) * (1 + radius_adj);
	ry = (width + half) * (1 + radius_adj);
	da = -da;
	for (i = 0; i < segs; i++) {
		v[0] = cx - rx * cos(ang * PCB_M180);
		v[1] = cy + ry * sin(ang * PCB_M180);
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
		ang += da;
	}

	/* explicitly draw the last point if the manhattan-distance is large enough */
	ang = astart;
	v2[0] = cx - rx * cos(ang * PCB_M180) * (1 - radius_adj);
	v2[1] = cy + ry * sin(ang * PCB_M180) * (1 - radius_adj);
	edx = (v[0] - v2[0]);
	edy = (v[1] - v2[1]);
	if (edx < 0) edx = -edx;
	if (edy < 0) edy = -edy;
	if (edx+edy > PCB_MM_TO_COORD(0.001))
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v2));


	/* now add other round cap */
	if (end_caps)
		pcb_poly_frac_circle(contour, endx1, endy1, v2, 2);

	/* now we have the whole contour */
	if (!(np = pcb_poly_from_contour(contour)))
		return NULL;
	return np;
}

pcb_polyarea_t *pcb_poly_from_rect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2)
{
	pcb_pline_t *contour = NULL;
	pcb_vector_t v;

	/* Return NULL for zero or negatively sized rectangles */
	if (x2 <= x1 || y2 <= y1)
		return NULL;

	v[0] = x1;
	v[1] = y1;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	v[0] = x2;
	v[1] = y1;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	v[0] = x2;
	v[1] = y2;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	v[0] = x1;
	v[1] = y2;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	return pcb_poly_from_contour(contour);
}

pcb_polyarea_t *pcb_poly_from_octagon(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style)
{
	pcb_pline_t *contour = NULL;
	pcb_vector_t v;
	double xm[8], ym[8];

	pcb_poly_square_pin_factors(style, xm, ym);

TODO(": rewrite this to use the same table as the square/oct pin draw function")
	/* point 7 */
	v[0] = x + ROUND(radius * 0.5) * xm[7];
	v[1] = y + ROUND(radius * PCB_TAN_22_5_DEGREE_2) * ym[7];
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	/* point 6 */
	v[0] = x + ROUND(radius * PCB_TAN_22_5_DEGREE_2) * xm[6];
	v[1] = y + ROUND(radius * 0.5) * ym[6];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 5 */
	v[0] = x - ROUND(radius * PCB_TAN_22_5_DEGREE_2) * xm[5];
	v[1] = y + ROUND(radius * 0.5) * ym[5];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 4 */
	v[0] = x - ROUND(radius * 0.5) * xm[4];
	v[1] = y + ROUND(radius * PCB_TAN_22_5_DEGREE_2) * ym[4];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 3 */
	v[0] = x - ROUND(radius * 0.5) * xm[3];
	v[1] = y - ROUND(radius * PCB_TAN_22_5_DEGREE_2) * ym[3];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 2 */
	v[0] = x - ROUND(radius * PCB_TAN_22_5_DEGREE_2) * xm[2];
	v[1] = y - ROUND(radius * 0.5) * ym[2];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 1 */
	v[0] = x + ROUND(radius * PCB_TAN_22_5_DEGREE_2) * xm[1];
	v[1] = y - ROUND(radius * 0.5) * ym[1];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	/* point 0 */
	v[0] = x + ROUND(radius * 0.5) * xm[0];
	v[1] = y - ROUND(radius * PCB_TAN_22_5_DEGREE_2) * ym[0];
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	return pcb_poly_from_contour(contour);
}

static void pcb_poly_frac_circle_(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range, int add_last)
{
	double oe1, oe2, e1, e2, t1;
	int i, orange = range;

	init_rotate_cache();

	oe1 = (v[0] - X);
	oe2 = (v[1] - Y);

	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));

	/* move vector to origin */
	e1 = (v[0] - X) * PCB_POLY_CIRC_RADIUS_ADJ;
	e2 = (v[1] - Y) * PCB_POLY_CIRC_RADIUS_ADJ;

	/* NB: the caller adds the last vertex, hence the -1 */
	range = PCB_POLY_CIRC_SEGS / range - 1;
	for (i = 0; i < range; i++) {
		/* rotate the vector */
		t1 = rotate_circle_seg[0] * e1 + rotate_circle_seg[1] * e2;
		e2 = rotate_circle_seg[2] * e1 + rotate_circle_seg[3] * e2;
		e1 = t1;
		v[0] = X + ROUND(e1);
		v[1] = Y + ROUND(e2);
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	}

	if ((add_last) && (orange == 4)) {
		v[0] = X - ROUND(oe2);
		v[1] = Y + ROUND(oe1);
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	}
}


/* add vertices in a fractional-circle starting from v
 * centered at X, Y and going counter-clockwise
 * does not include the first point
 * last argument is 1 for a full circle
 * 2 for a half circle
 * or 4 for a quarter circle
 */
void pcb_poly_frac_circle(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range)
{
	pcb_poly_frac_circle_(c, X, Y, v, range, 0);
}

/* same but adds the last vertex */
void pcb_poly_frac_circle_end(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range)
{
	pcb_poly_frac_circle_(c, X, Y, v, range, 1);
}


/* create a circle approximation from lines */
pcb_polyarea_t *pcb_poly_from_circle(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius)
{
	pcb_pline_t *contour;
	pcb_vector_t v;

	if (radius <= 0)
		return NULL;
	v[0] = x + radius;
	v[1] = y;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	pcb_poly_frac_circle(contour, x, y, v, 1);
	contour->is_round = pcb_true;
	contour->cx = x;
	contour->cy = y;
	contour->radius = radius;
	return pcb_poly_from_contour(contour);
}

/* make a rounded-corner rectangle with radius t beyond x1,x2,y1,y2 rectangle */
pcb_polyarea_t *RoundRect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2, pcb_coord_t t)
{
	pcb_pline_t *contour = NULL;
	pcb_vector_t v;

	assert(x2 > x1);
	assert(y2 > y1);
	v[0] = x1 - t;
	v[1] = y1;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	pcb_poly_frac_circle_end(contour, x1, y1, v, 4);
	v[0] = x2;
	v[1] = y1 - t;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	pcb_poly_frac_circle_end(contour, x2, y1, v, 4);
	v[0] = x2 + t;
	v[1] = y2;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	pcb_poly_frac_circle_end(contour, x2, y2, v, 4);
	v[0] = x1;
	v[1] = y2 + t;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	pcb_poly_frac_circle_end(contour, x1, y2, v, 4);
	return pcb_poly_from_contour(contour);
}


pcb_polyarea_t *pcb_poly_from_line(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick, pcb_bool square)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v;
	double d, dx, dy;
	long half;

	if (thick <= 0)
		return NULL;
	half = (thick + 1) / 2;
	d = sqrt(PCB_SQUARE(x1 - x2) + PCB_SQUARE(y1 - y2));
	if (!square)
		if (d == 0)									/* line is a point */
			return pcb_poly_from_circle(x1, y1, half);
	if (d != 0) {
		d = half / d;
		dx = (y1 - y2) * d;
		dy = (x2 - x1) * d;
	}
	else {
		dx = half;
		dy = 0;
	}
	if (square) {	/* take into account the ends */
		x1 -= dy;
		y1 += dx;
		x2 += dy;
		y2 -= dx;
	}
	v[0] = x1 - dx;
	v[1] = y1 - dy;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return 0;
	v[0] = x2 - dx;
	v[1] = y2 - dy;
	if (square)
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	else
		pcb_poly_frac_circle(contour, x2, y2, v, 2);
	v[0] = x2 + dx;
	v[1] = y2 + dy;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	v[0] = x1 + dx;
	v[1] = y1 + dy;
	if (square)
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	else
		pcb_poly_frac_circle(contour, x1, y1, v, 2);
	/* now we have the line contour */
	if (!(np = pcb_poly_from_contour(contour)))
		return NULL;
	return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
pcb_polyarea_t *pcb_poly_from_arc(pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t astart, pcb_angle_t adelta, pcb_coord_t thick)
{
	double delta;
	pcb_coord_t half;

	delta = (adelta < 0) ? -adelta : adelta;

	half = (thick + 1) / 2;

	/* corner case: can't even calculate the end cap properly because radius
	   is so small that there's no inner arc of the clearance */
	if ((width - half <= 0) || (height - half <= 0)) {
		pcb_coord_t lx1, ly1;
		pcb_polyarea_t *tmp_arc, *tmp1, *tmp2, *res, *ends;

		tmp_arc = ArcPolyNoIntersect(cx, cy, width, height, astart, adelta, thick, 0);

		pcb_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &lx1, &ly1);
		tmp1 = pcb_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		pcb_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &lx1, &ly1);
		tmp2 = pcb_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		pcb_polyarea_boolean_free(tmp1, tmp2, &ends, PCB_PBO_UNITE);
		pcb_polyarea_boolean_free(ends, tmp_arc, &res, PCB_PBO_UNITE);
		return res;
	}

	/* If the arc segment would self-intersect, we need to construct it as the union of
	   two non-intersecting segments */
	if (2 * M_PI * width * (1. - (double) delta / 360.) - thick < MIN_CLEARANCE_BEFORE_BISECT) {
		pcb_polyarea_t *tmp1, *tmp2, *res;
		int half_delta = adelta / 2;

		tmp1 = ArcPolyNoIntersect(cx, cy, width, height, astart, half_delta, thick, 1);
		tmp2 = ArcPolyNoIntersect(cx, cy, width, height, astart+half_delta, adelta-half_delta, thick, 1);
		pcb_polyarea_boolean_free(tmp1, tmp2, &res, PCB_PBO_UNITE);
		return res;
	}

	return ArcPolyNoIntersect(cx, cy, width, height, astart, adelta, thick, 1);
}


/* set up x and y multiplier for an octa poly, depending on square pin style
   (used in early versions of pcb-rnd, before custom shape padstacks) */
void pcb_poly_square_pin_factors(int style, double *xm, double *ym)
{
	int i;
	const double factor = 2.0;

	/* reset multipliers */
	for (i = 0; i < 8; i++) {
		xm[i] = 1;
		ym[i] = 1;
	}

	style--;
	if (style & 1)
		xm[0] = xm[1] = xm[6] = xm[7] = factor;
	if (style & 2)
		xm[2] = xm[3] = xm[4] = xm[5] = factor;
	if (style & 4)
		ym[4] = ym[5] = ym[6] = ym[7] = factor;
	if (style & 8)
		ym[0] = ym[1] = ym[2] = ym[3] = factor;
}

