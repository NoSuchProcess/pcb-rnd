/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/*

Here's a brief tour of the data and life of a polygon, courtesy of Ben
Jackson:

A PCB pcb_polygon_t contains an array of points outlining the polygon.
This is what is manipulated by the UI and stored in the saved PCB.

A pcb_polygon_t also contains a pcb_polyarea_t called 'Clipped' which is
computed dynamically by InitClip every time a board is loaded.  The
point array is converted to a pcb_polyarea_t by original_poly and then holes
are cut in it by clearPoly.  After that it is maintained dynamically
as parts are added, moved or removed (this is why sometimes bugs can
be fixed by just re-loading the board).

A pcb_polyarea_t consists of a linked list of pcb_pline_t structures.  The head of
that list is pcb_polyarea_t.contours.  The first contour is an outline of a
filled region.  All of the subsequent pcb_pline_ts are holes cut out of that
first contour.  pcb_polyarea_ts are in a doubly-linked list and each member
of the list is an independent (non-overlapping) area with its own
outline and holes.  The function biggest() finds the largest pcb_polyarea_t
so that pcb_polygon_t.Clipped points to that shape.  The rest of the
polygon still exists, it's just ignored when turning the polygon into
copper.

The first pcb_polyarea_t in pcb_polygon_t.Clipped is what is used for the vast
majority of Polygon related tests.  The basic logic for an
intersection is "is the target shape inside pcb_polyarea_t.contours and NOT
fully enclosed in any of pcb_polyarea_t.contours.next... (the holes)".

The polygon dicer (NoHolesPolygonDicer and r_NoHolesPolygonDicer)
emits a series of "simple" pcb_pline_t shapes.  That is, the pcb_pline_t isn't
linked to any other "holes" oulines).  That's the meaning of the first
test in r_NoHolesPolygonDicer.  It is testing to see if the pcb_pline_t
contour (the first, making it a solid outline) has a valid next
pointer (which would point to one or more holes).  The dicer works by
recursively chopping the polygon in half through the first hole it
sees (which is guaranteed to eliminate at least that one hole).  The
dicer output is used for HIDs which cannot render things with holes
(which would require erasure).

*/

/* special polygon editing routines
 */

#include "config.h"
#include "conf_core.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <setjmp.h>

#include "board.h"
#include "box.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "polygon.h"
#include "remove.h"
#include "search.h"
#include "set.h"
#include "obj_pinvia_therm.h"
#include "undo.h"
#include "layer.h"
#include "compat_nls.h"
#include "obj_all.h"
#include "obj_poly_draw.h"

#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

#define UNSUBTRACT_BLOAT 10
#define SUBTRACT_PIN_VIA_BATCH_SIZE 100
#define SUBTRACT_LINE_BATCH_SIZE 20

static double rotate_circle_seg[4];

void polygon_init(void)
{
	double cos_ang = cos(2.0 * M_PI / POLY_CIRC_SEGS_F);
	double sin_ang = sin(2.0 * M_PI / POLY_CIRC_SEGS_F);

	rotate_circle_seg[0] = cos_ang;
	rotate_circle_seg[1] = -sin_ang;
	rotate_circle_seg[2] = sin_ang;
	rotate_circle_seg[3] = cos_ang;
}

pcb_cardinal_t polygon_point_idx(pcb_polygon_t *polygon, pcb_point_t *point)
{
	assert(point >= polygon->Points);
	assert(point <= polygon->Points + polygon->PointN);
	return ((char *) point - (char *) polygon->Points) / sizeof(pcb_point_t);
}

/* Find contour number: 0 for outer, 1 for first hole etc.. */
pcb_cardinal_t polygon_point_contour(pcb_polygon_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t i;
	pcb_cardinal_t contour = 0;

	for (i = 0; i < polygon->HoleIndexN; i++)
		if (point >= polygon->HoleIndex[i])
			contour = i + 1;
	return contour;
}

pcb_cardinal_t next_contour_point(pcb_polygon_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t contour;
	pcb_cardinal_t this_contour_start;
	pcb_cardinal_t next_contour_start;

	contour = polygon_point_contour(polygon, point);

	this_contour_start = (contour == 0) ? 0 : polygon->HoleIndex[contour - 1];
	next_contour_start = (contour == polygon->HoleIndexN) ? polygon->PointN : polygon->HoleIndex[contour];

	/* Wrap back to the start of the contour we're in if we pass the end */
	if (++point == next_contour_start)
		point = this_contour_start;

	return point;
}

pcb_cardinal_t prev_contour_point(pcb_polygon_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t contour;
	pcb_cardinal_t prev_contour_end;
	pcb_cardinal_t this_contour_end;

	contour = polygon_point_contour(polygon, point);

	prev_contour_end = (contour == 0) ? 0 : polygon->HoleIndex[contour - 1];
	this_contour_end = (contour == polygon->HoleIndexN) ? polygon->PointN - 1 : polygon->HoleIndex[contour] - 1;

	/* Wrap back to the start of the contour we're in if we pass the end */
	if (point == prev_contour_end)
		point = this_contour_end;
	else
		point--;

	return point;
}

static void add_noholes_polyarea(pcb_pline_t * pline, void *user_data)
{
	pcb_polygon_t *poly = (pcb_polygon_t *) user_data;

	/* Prepend the pline into the NoHoles linked list */
	pline->next = poly->NoHoles;
	poly->NoHoles = pline;
}

void ComputeNoHoles(pcb_polygon_t * poly)
{
	poly_FreeContours(&poly->NoHoles);
	if (poly->Clipped)
		NoHolesPolygonDicer(poly, NULL, add_noholes_polyarea, poly);
	else
		printf("Compute_noholes caught poly->Clipped = NULL\n");
	poly->NoHolesValid = 1;
}

static pcb_polyarea_t *biggest(pcb_polyarea_t * p)
{
	pcb_polyarea_t *n, *top = NULL;
	pcb_pline_t *pl;
	pcb_rtree_t *tree;
	double big = -1;
	if (!p)
		return NULL;
	n = p;
	do {
#if 0
		if (n->contours->area < PCB->IsleArea) {
			n->b->f = n->f;
			n->f->b = n->b;
			pcb_poly_contour_del(&n->contours);
			if (n == p)
				p = n->f;
			n = n->f;
			if (!n->contours) {
				free(n);
				return NULL;
			}
		}
#endif
		if (n->contours->area > big) {
			top = n;
			big = n->contours->area;
		}
	}
	while ((n = n->f) != p);
	assert(top);
	if (top == p)
		return p;
	pl = top->contours;
	tree = top->contour_tree;
	top->contours = p->contours;
	top->contour_tree = p->contour_tree;
	p->contours = pl;
	p->contour_tree = tree;
	assert(pl);
	assert(p->f);
	assert(p->b);
	return p;
}

pcb_polyarea_t *ContourToPoly(pcb_pline_t * contour)
{
	pcb_polyarea_t *p;
	pcb_poly_contour_pre(contour, pcb_true);
	assert(contour->Flags.orient == PLF_DIR);
	if (!(p = poly_Create()))
		return NULL;
	pcb_poly_contour_include(p, contour);
	assert(poly_Valid(p));
	return p;
}

static pcb_polyarea_t *original_poly(pcb_polygon_t * p)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_cardinal_t n;
	pcb_vector_t v;
	int hole = 0;

	if ((np = poly_Create()) == NULL)
		return NULL;

	/* first make initial polygon contour */
	for (n = 0; n < p->PointN; n++) {
		/* No current contour? Make a new one starting at point */
		/*   (or) Add point to existing contour */

		v[0] = p->Points[n].X;
		v[1] = p->Points[n].Y;
		if (contour == NULL) {
			if ((contour = pcb_poly_contour_new(v)) == NULL)
				return NULL;
		}
		else {
			pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
		}

		/* Is current point last in contour? If so process it. */
		if (n == p->PointN - 1 || (hole < p->HoleIndexN && n == p->HoleIndex[hole] - 1)) {
			pcb_poly_contour_pre(contour, pcb_true);

			/* make sure it is a positive contour (outer) or negative (hole) */
			if (contour->Flags.orient != (hole ? PLF_INV : PLF_DIR))
				pcb_poly_contour_inv(contour);
			assert(contour->Flags.orient == (hole ? PLF_INV : PLF_DIR));

			pcb_poly_contour_include(np, contour);
			contour = NULL;
			assert(poly_Valid(np));

			hole++;
		}
	}
	return biggest(np);
}

pcb_polyarea_t *PolygonToPoly(pcb_polygon_t * p)
{
	return original_poly(p);
}

pcb_polyarea_t *RectPoly(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2)
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
	return ContourToPoly(contour);
}

/* set up x and y multiplier for an octa poly, depending on square pin style */
void square_pin_factors(int style, double *xm, double *ym)
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


pcb_polyarea_t *OctagonPoly(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style)
{
	pcb_pline_t *contour = NULL;
	pcb_vector_t v;
	double xm[8], ym[8];

	square_pin_factors(style, xm, ym);

#warning TODO: rewrite this to use the same table as the square/oct pin draw function
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
	return ContourToPoly(contour);
}

/* add vertices in a fractional-circle starting from v
 * centered at X, Y and going counter-clockwise
 * does not include the first point
 * last argument is 1 for a full circle
 * 2 for a half circle
 * or 4 for a quarter circle
 */
void frac_circle(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range)
{
	double e1, e2, t1;
	int i;

	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	/* move vector to origin */
	e1 = (v[0] - X) * POLY_CIRC_RADIUS_ADJ;
	e2 = (v[1] - Y) * POLY_CIRC_RADIUS_ADJ;

	/* NB: the caller adds the last vertex, hence the -1 */
	range = POLY_CIRC_SEGS / range - 1;
	for (i = 0; i < range; i++) {
		/* rotate the vector */
		t1 = rotate_circle_seg[0] * e1 + rotate_circle_seg[1] * e2;
		e2 = rotate_circle_seg[2] * e1 + rotate_circle_seg[3] * e2;
		e1 = t1;
		v[0] = X + ROUND(e1);
		v[1] = Y + ROUND(e2);
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	}
}

/* create a circle approximation from lines */
pcb_polyarea_t *CirclePoly(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius)
{
	pcb_pline_t *contour;
	pcb_vector_t v;

	if (radius <= 0)
		return NULL;
	v[0] = x + radius;
	v[1] = y;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	frac_circle(contour, x, y, v, 1);
	contour->is_round = pcb_true;
	contour->cx = x;
	contour->cy = y;
	contour->radius = radius;
	return ContourToPoly(contour);
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
	frac_circle(contour, x1, y1, v, 4);
	v[0] = x2;
	v[1] = y1 - t;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, x2, y1, v, 4);
	v[0] = x2 + t;
	v[1] = y2;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, x2, y2, v, 4);
	v[0] = x1;
	v[1] = y2 + t;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, x1, y2, v, 4);
	return ContourToPoly(contour);
}

#define ARC_ANGLE 5
static pcb_polyarea_t *ArcPolyNoIntersect(pcb_arc_t * a, pcb_coord_t thick)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v;
	pcb_box_t *ends;
	int i, segs;
	double ang, da, rx, ry;
	long half;
	double radius_adj;

	if (thick <= 0)
		return NULL;
	if (a->Delta < 0) {
		a->StartAngle += a->Delta;
		a->Delta = -a->Delta;
	}
	half = (thick + 1) / 2;
	ends = pcb_arc_get_ends(a);
	/* start with inner radius */
	rx = MAX(a->Width - half, 0);
	ry = MAX(a->Height - half, 0);
	segs = 1;
	if (thick > 0)
		segs = MAX(segs, a->Delta * M_PI / 360 *
							 sqrt(sqrt((double) rx * rx + (double) ry * ry) / POLY_ARC_MAX_DEVIATION / 2 / thick));
	segs = MAX(segs, a->Delta / ARC_ANGLE);

	ang = a->StartAngle;
	da = (1.0 * a->Delta) / segs;
	radius_adj = (M_PI * da / 360) * (M_PI * da / 360) / 2;
	v[0] = a->X - rx * cos(ang * PCB_M180);
	v[1] = a->Y + ry * sin(ang * PCB_M180);
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return 0;
	for (i = 0; i < segs - 1; i++) {
		ang += da;
		v[0] = a->X - rx * cos(ang * PCB_M180);
		v[1] = a->Y + ry * sin(ang * PCB_M180);
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	}
	/* find last point */
	ang = a->StartAngle + a->Delta;
	v[0] = a->X - rx * cos(ang * PCB_M180) * (1 - radius_adj);
	v[1] = a->Y + ry * sin(ang * PCB_M180) * (1 - radius_adj);
	/* add the round cap at the end */
	frac_circle(contour, ends->X2, ends->Y2, v, 2);
	/* and now do the outer arc (going backwards) */
	rx = (a->Width + half) * (1 + radius_adj);
	ry = (a->Width + half) * (1 + radius_adj);
	da = -da;
	for (i = 0; i < segs; i++) {
		v[0] = a->X - rx * cos(ang * PCB_M180);
		v[1] = a->Y + ry * sin(ang * PCB_M180);
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
		ang += da;
	}
	/* now add other round cap */
	ang = a->StartAngle;
	v[0] = a->X - rx * cos(ang * PCB_M180) * (1 - radius_adj);
	v[1] = a->Y + ry * sin(ang * PCB_M180) * (1 - radius_adj);
	frac_circle(contour, ends->X1, ends->Y1, v, 2);
	/* now we have the whole contour */
	if (!(np = ContourToPoly(contour)))
		return NULL;
	return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
pcb_polyarea_t *ArcPoly(pcb_arc_t * a, pcb_coord_t thick)
{
	double delta;
	pcb_arc_t seg1, seg2;
	pcb_polyarea_t *tmp1, *tmp2, *res;

	delta = (a->Delta < 0) ? -a->Delta : a->Delta;

	/* If the arc segment would self-intersect, we need to construct it as the union of
	   two non-intersecting segments */

	if (2 * M_PI * a->Width * (1. - (double) delta / 360.) - thick < MIN_CLEARANCE_BEFORE_BISECT) {
		int half_delta = a->Delta / 2;

		seg1 = seg2 = *a;
		seg1.Delta = half_delta;
		seg2.Delta -= half_delta;
		seg2.StartAngle += half_delta;

		tmp1 = ArcPolyNoIntersect(&seg1, thick);
		tmp2 = ArcPolyNoIntersect(&seg2, thick);
		poly_Boolean_free(tmp1, tmp2, &res, PBO_UNITE);
		return res;
	}

	return ArcPolyNoIntersect(a, thick);
}

pcb_polyarea_t *LinePoly(pcb_line_t * L, pcb_coord_t thick)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v;
	double d, dx, dy;
	long half;
	pcb_line_t _l = *L, *l = &_l;

	if (thick <= 0)
		return NULL;
	half = (thick + 1) / 2;
	d = sqrt(PCB_SQUARE(l->Point1.X - l->Point2.X) + PCB_SQUARE(l->Point1.Y - l->Point2.Y));
	if (!PCB_FLAG_TEST(PCB_FLAG_SQUARE, l))
		if (d == 0)									/* line is a point */
			return CirclePoly(l->Point1.X, l->Point1.Y, half);
	if (d != 0) {
		d = half / d;
		dx = (l->Point1.Y - l->Point2.Y) * d;
		dy = (l->Point2.X - l->Point1.X) * d;
	}
	else {
		dx = half;
		dy = 0;
	}
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, l)) {	/* take into account the ends */
		l->Point1.X -= dy;
		l->Point1.Y += dx;
		l->Point2.X += dy;
		l->Point2.Y -= dx;
	}
	v[0] = l->Point1.X - dx;
	v[1] = l->Point1.Y - dy;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return 0;
	v[0] = l->Point2.X - dx;
	v[1] = l->Point2.Y - dy;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, l))
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	else
		frac_circle(contour, l->Point2.X, l->Point2.Y, v, 2);
	v[0] = l->Point2.X + dx;
	v[1] = l->Point2.Y + dy;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	v[0] = l->Point1.X + dx;
	v[1] = l->Point1.Y + dy;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, l))
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	else
		frac_circle(contour, l->Point1.X, l->Point1.Y, v, 2);
	/* now we have the line contour */
	if (!(np = ContourToPoly(contour)))
		return NULL;
	return np;
}

/* make a rounded-corner rectangle */
pcb_polyarea_t *SquarePadPoly(pcb_pad_t * pad, pcb_coord_t clear)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v;
	double d;
	double tx, ty;
	double cx, cy;
	pcb_pad_t _t = *pad, *t = &_t;
	pcb_pad_t _c = *pad, *c = &_c;
	int halfthick = (pad->Thickness + 1) / 2;
	int halfclear = (clear + 1) / 2;

	d = sqrt(PCB_SQUARE(pad->Point1.X - pad->Point2.X) + PCB_SQUARE(pad->Point1.Y - pad->Point2.Y));
	if (d != 0) {
		double a = halfthick / d;
		tx = (t->Point1.Y - t->Point2.Y) * a;
		ty = (t->Point2.X - t->Point1.X) * a;
		a = halfclear / d;
		cx = (c->Point1.Y - c->Point2.Y) * a;
		cy = (c->Point2.X - c->Point1.X) * a;

		t->Point1.X -= ty;
		t->Point1.Y += tx;
		t->Point2.X += ty;
		t->Point2.Y -= tx;
		c->Point1.X -= cy;
		c->Point1.Y += cx;
		c->Point2.X += cy;
		c->Point2.Y -= cx;
	}
	else {
		tx = halfthick;
		ty = 0;
		cx = halfclear;
		cy = 0;

		t->Point1.Y += tx;
		t->Point2.Y -= tx;
		c->Point1.Y += cx;
		c->Point2.Y -= cx;
	}

	v[0] = c->Point1.X - tx;
	v[1] = c->Point1.Y - ty;
	if ((contour = pcb_poly_contour_new(v)) == NULL)
		return 0;
	frac_circle(contour, (t->Point1.X - tx), (t->Point1.Y - ty), v, 4);

	v[0] = t->Point2.X - cx;
	v[1] = t->Point2.Y - cy;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, (t->Point2.X - tx), (t->Point2.Y - ty), v, 4);

	v[0] = c->Point2.X + tx;
	v[1] = c->Point2.Y + ty;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, (t->Point2.X + tx), (t->Point2.Y + ty), v, 4);

	v[0] = t->Point1.X + cx;
	v[1] = t->Point1.Y + cy;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	frac_circle(contour, (t->Point1.X + tx), (t->Point1.Y + ty), v, 4);

	/* now we have the line contour */
	if (!(np = ContourToPoly(contour)))
		return NULL;
	return np;
}

/* clear np1 from the polygon */
static int Subtract(pcb_polyarea_t * np1, pcb_polygon_t * p, pcb_bool fnp)
{
	pcb_polyarea_t *merged = NULL, *np = np1;
	int x;
	assert(np);
	assert(p);
	if (!p->Clipped) {
		if (fnp)
			poly_Free(&np);
		return 1;
	}
	assert(poly_Valid(p->Clipped));
	assert(poly_Valid(np));
	if (fnp)
		x = poly_Boolean_free(p->Clipped, np, &merged, PBO_SUB);
	else {
		x = poly_Boolean(p->Clipped, np, &merged, PBO_SUB);
		poly_Free(&p->Clipped);
	}
	assert(!merged || poly_Valid(merged));
	if (x != err_ok) {
		fprintf(stderr, "Error while clipping PBO_SUB: %d\n", x);
		poly_Free(&merged);
		p->Clipped = NULL;
		if (p->NoHoles)
			printf("Just leaked in Subtract\n");
		p->NoHoles = NULL;
		return -1;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || poly_Valid(p->Clipped));
	if (!p->Clipped)
		pcb_message(PCB_MSG_DEFAULT, "Polygon cleared out of existence near (%d, %d)\n",
						(p->BoundingBox.X1 + p->BoundingBox.X2) / 2, (p->BoundingBox.Y1 + p->BoundingBox.Y2) / 2);
	return 1;
}

/* create a polygon of the pin clearance */
pcb_polyarea_t *PinPoly(pcb_pin_t * pin, pcb_coord_t thick, pcb_coord_t clear)
{
	int size;

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
		if (PCB_FLAG_SQUARE_GET(pin) <= 1) {
			size = (thick + 1) / 2;
			return RoundRect(pin->X - size, pin->X + size, pin->Y - size, pin->Y + size, (clear + 1) / 2);
		}
		else {
			size = (thick + clear + 1) / 2;
			return OctagonPoly(pin->X, pin->Y, size + size, PCB_FLAG_SQUARE_GET(pin));
		}

	}
	else {
		size = (thick + clear + 1) / 2;
		if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin)) {
			return OctagonPoly(pin->X, pin->Y, size + size, PCB_FLAG_SQUARE_GET(pin));
		}
	}
	return CirclePoly(pin->X, pin->Y, size);
}

pcb_polyarea_t *BoxPolyBloated(pcb_box_t * box, pcb_coord_t bloat)
{
	return RectPoly(box->X1 - bloat, box->X2 + bloat, box->Y1 - bloat, box->Y2 + bloat);
}

/* return the clearance polygon for a pin */
static pcb_polyarea_t *pin_clearance_poly(pcb_cardinal_t layernum, pcb_board_t *pcb, pcb_pin_t * pin)
{
	pcb_polyarea_t *np;
	if (PCB_FLAG_THERM_TEST(layernum, pin))
		np = ThermPoly(pcb, pin, layernum);
	else
		np = PinPoly(pin, PIN_SIZE(pin), pin->Clearance);
	return np;
}

/* remove the pin clearance from the polygon */
static int SubtractPin(pcb_data_t * d, pcb_pin_t * pin, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;
	pcb_cardinal_t i;

	if (pin->Clearance == 0)
		return 0;
	i = GetLayerNumber(d, l);
	np = pin_clearance_poly(i, d->pcb, pin);

	if (PCB_FLAG_THERM_TEST(i, pin)) {
		if (!np)
			return 0;
	}
	else {
		if (!np)
			return -1;
	}

	return Subtract(np, p, pcb_true);
}

static int SubtractLine(pcb_line_t * line, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line))
		return 0;
	if (!(np = LinePoly(line, line->Thickness + line->Clearance)))
		return -1;
	return Subtract(np, p, pcb_true);
}

static int SubtractArc(pcb_arc_t * arc, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc))
		return 0;
	if (!(np = ArcPoly(arc, arc->Thickness + arc->Clearance)))
		return -1;
	return Subtract(np, p, pcb_true);
}

static int SubtractText(pcb_text_t * text, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;
	const pcb_box_t *b = &text->BoundingBox;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;
	if (!(np = RoundRect(b->X1 + PCB->Bloat, b->X2 - PCB->Bloat, b->Y1 + PCB->Bloat, b->Y2 - PCB->Bloat, PCB->Bloat)))
		return -1;
	return Subtract(np, p, pcb_true);
}

static int SubtractPad(pcb_pad_t * pad, pcb_polygon_t * p)
{
	pcb_polyarea_t *np = NULL;

	if (pad->Clearance == 0)
		return 0;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pad)) {
		if (!(np = SquarePadPoly(pad, pad->Thickness + pad->Clearance)))
			return -1;
	}
	else {
		if (!(np = LinePoly((pcb_line_t *) pad, pad->Thickness + pad->Clearance)))
			return -1;
	}
	return Subtract(np, p, pcb_true);
}

struct cpInfo {
	const pcb_box_t *other;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_polygon_t *polygon;
	pcb_bool solder;
	pcb_polyarea_t *accumulate;
	int batch_size;
	jmp_buf env;
};

static void subtract_accumulated(struct cpInfo *info, pcb_polygon_t *polygon)
{
	if (info->accumulate == NULL)
		return;
	Subtract(info->accumulate, polygon, pcb_true);
	info->accumulate = NULL;
	info->batch_size = 0;
}

static pcb_r_dir_t pin_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_polygon_t *polygon;
	pcb_polyarea_t *np;
	pcb_polyarea_t *merged;
	pcb_cardinal_t i;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return R_DIR_NOT_FOUND;
	polygon = info->polygon;

	if (pin->Clearance == 0)
		return R_DIR_NOT_FOUND;
	i = GetLayerNumber(info->data, info->layer);
	if (PCB_FLAG_THERM_TEST(i, pin)) {
		np = ThermPoly((pcb_board_t *) (info->data->pcb), pin, i);
		if (!np)
			return R_DIR_FOUND_CONTINUE;
	}
	else {
		np = PinPoly(pin, PIN_SIZE(pin), pin->Clearance);
		if (!np)
			longjmp(info->env, 1);
	}

	poly_Boolean_free(info->accumulate, np, &merged, PBO_UNITE);
	info->accumulate = merged;

	info->batch_size++;

	if (info->batch_size == SUBTRACT_PIN_VIA_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t arc_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_polygon_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return R_DIR_NOT_FOUND;
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc))
		return R_DIR_NOT_FOUND;
	polygon = info->polygon;
	if (SubtractArc(arc, polygon) < 0)
		longjmp(info->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t pad_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_polygon_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return R_DIR_NOT_FOUND;
	if (pad->Clearance == 0)
		return R_DIR_NOT_FOUND;
	polygon = info->polygon;
	if (PCB_XOR(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad), !info->solder)) {
		if (SubtractPad(pad, polygon) < 0)
			longjmp(info->env, 1);
		return R_DIR_FOUND_CONTINUE;
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t line_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_polygon_t *polygon;
	pcb_polyarea_t *np;
	pcb_polyarea_t *merged;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return R_DIR_NOT_FOUND;
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line))
		return R_DIR_NOT_FOUND;
	polygon = info->polygon;

	if (!(np = LinePoly(line, line->Thickness + line->Clearance)))
		longjmp(info->env, 1);

	poly_Boolean_free(info->accumulate, np, &merged, PBO_UNITE);
	info->accumulate = merged;
	info->batch_size++;

	if (info->batch_size == SUBTRACT_LINE_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t text_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_polygon_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return R_DIR_NOT_FOUND;
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return R_DIR_NOT_FOUND;
	polygon = info->polygon;
	if (SubtractText(text, polygon) < 0)
		longjmp(info->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

static int Group(pcb_data_t *Data, pcb_cardinal_t layer)
{
	pcb_cardinal_t i, j;
	for (i = 0; i < max_group; i++)
		for (j = 0; j < ((pcb_board_t *) (Data->pcb))->LayerGroups.Number[i]; j++)
			if (layer == ((pcb_board_t *) (Data->pcb))->LayerGroups.Entries[i][j])
				return i;
	return i;
}

static int clearPoly(pcb_data_t *Data, pcb_layer_t *Layer, pcb_polygon_t * polygon, const pcb_box_t * here, pcb_coord_t expand)
{
	int r = 0, seen;
	pcb_box_t region;
	struct cpInfo info;
	pcb_cardinal_t group;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon)
			|| GetLayerNumber(Data, Layer) >= max_copper_layer)
		return 0;
	group = Group(Data, GetLayerNumber(Data, Layer));
	info.solder = (group == Group(Data, solder_silk_layer));
	info.data = Data;
	info.other = here;
	info.layer = Layer;
	info.polygon = polygon;
	if (here)
		region = pcb_clip_box(here, &polygon->BoundingBox);
	else
		region = polygon->BoundingBox;
	region = pcb_bloat_box(&region, expand);

	if (setjmp(info.env) == 0) {
		r = 0;
		info.accumulate = NULL;
		info.batch_size = 0;
		if (info.solder || group == Group(Data, component_silk_layer)) {
			r_search(Data->pad_tree, &region, NULL, pad_sub_callback, &info, &seen);
			r += seen;
		}
		GROUP_LOOP(Data, group);
		{
			r_search(layer->line_tree, &region, NULL, line_sub_callback, &info, &seen);
			r += seen;
			subtract_accumulated(&info, polygon);
			r_search(layer->arc_tree, &region, NULL, arc_sub_callback, &info, &seen);
			r += seen;
			r_search(layer->text_tree, &region, NULL, text_sub_callback, &info, &seen);
			r += seen;
		}
		END_LOOP;
		r_search(Data->via_tree, &region, NULL, pin_sub_callback, &info, &seen);
		r += seen;
		r_search(Data->pin_tree, &region, NULL, pin_sub_callback, &info, &seen);
		r += seen;
		subtract_accumulated(&info, polygon);
	}
	polygon->NoHolesValid = 0;
	return r;
}

static int Unsubtract(pcb_polyarea_t * np1, pcb_polygon_t * p)
{
	pcb_polyarea_t *merged = NULL, *np = np1;
	pcb_polyarea_t *orig_poly, *clipped_np;
	int x;
	assert(np);
	assert(p && p->Clipped);

	orig_poly = original_poly(p);

	x = poly_Boolean_free(np, orig_poly, &clipped_np, PBO_ISECT);
	if (x != err_ok) {
		fprintf(stderr, "Error while clipping PBO_ISECT: %d\n", x);
		poly_Free(&clipped_np);
		goto fail;
	}

	x = poly_Boolean_free(p->Clipped, clipped_np, &merged, PBO_UNITE);
	if (x != err_ok) {
		fprintf(stderr, "Error while clipping PBO_UNITE: %d\n", x);
		poly_Free(&merged);
		goto fail;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || poly_Valid(p->Clipped));
	return 1;

fail:
	p->Clipped = NULL;
	if (p->NoHoles)
		printf("Just leaked in Unsubtract\n");
	p->NoHoles = NULL;
	return 0;
}

static int UnsubtractPin(pcb_pin_t * pin, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	/* overlap a bit to prevent gaps from rounding errors */
	np = BoxPolyBloated(&pin->BoundingBox, UNSUBTRACT_BLOAT * 400000);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;

	clearPoly(PCB->Data, l, p, (const pcb_box_t *) pin, 2 * UNSUBTRACT_BLOAT * 400000);
	return 1;
}

static int UnsubtractArc(pcb_arc_t * arc, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc))
		return 0;

	/* overlap a bit to prevent gaps from rounding errors */
	np = BoxPolyBloated(&arc->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) arc, 2 * UNSUBTRACT_BLOAT);
	return 1;
}

static int UnsubtractLine(pcb_line_t * line, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = BoxPolyBloated(&line->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) line, 2 * UNSUBTRACT_BLOAT);
	return 1;
}

static int UnsubtractText(pcb_text_t * text, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = BoxPolyBloated(&text->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return -1;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) text, 2 * UNSUBTRACT_BLOAT);
	return 1;
}

static int UnsubtractPad(pcb_pad_t * pad, pcb_layer_t * l, pcb_polygon_t * p)
{
	pcb_polyarea_t *np;

	/* overlap a bit to prevent notches from rounding errors */
	np = BoxPolyBloated(&pad->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) pad, 2 * UNSUBTRACT_BLOAT);
	return 1;
}

static pcb_bool inhibit = pcb_false;

int InitClip(pcb_data_t *Data, pcb_layer_t *layer, pcb_polygon_t * p)
{
	if (inhibit)
		return 0;
	if (p->Clipped)
		poly_Free(&p->Clipped);
	p->Clipped = original_poly(p);
	poly_FreeContours(&p->NoHoles);
	if (!p->Clipped)
		return 0;
	assert(poly_Valid(p->Clipped));
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, p))
		clearPoly(Data, layer, p, NULL, 0);
	else
		p->NoHolesValid = 0;
	return 1;
}

/* --------------------------------------------------------------------------
 * remove redundant polygon points. Any point that lies on the straight
 * line between the points on either side of it is redundant.
 * returns pcb_true if any points are removed
 */
pcb_bool RemoveExcessPolygonPoints(pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_point_t *p;
	pcb_cardinal_t n, prev, next;
	pcb_line_t line;
	pcb_bool changed = pcb_false;

	if (Undoing())
		return (pcb_false);

	for (n = 0; n < Polygon->PointN; n++) {
		prev = prev_contour_point(Polygon, n);
		next = next_contour_point(Polygon, n);
		p = &Polygon->Points[n];

		line.Point1 = Polygon->Points[prev];
		line.Point2 = Polygon->Points[next];
		line.Thickness = 0;
		if (IsPointOnLine(p->X, p->Y, 0.0, &line)) {
			RemoveObject(PCB_TYPE_POLYGON_POINT, Layer, Polygon, p);
			changed = pcb_true;
		}
	}
	return (changed);
}

/* ---------------------------------------------------------------------------
 * returns the index of the polygon point which is the end
 * point of the segment with the lowest distance to the passed
 * coordinates
 */
pcb_cardinal_t GetLowestDistancePolygonPoint(pcb_polygon_t *Polygon, pcb_coord_t X, pcb_coord_t Y)
{
	double mindistance = (double) MAX_COORD * MAX_COORD;
	pcb_point_t *ptr1, *ptr2;
	pcb_cardinal_t n, result = 0;

	/* we calculate the distance to each segment and choose the
	 * shortest distance. If the closest approach between the
	 * given point and the projected line (i.e. the segment extended)
	 * is not on the segment, then the distance is the distance
	 * to the segment end point.
	 */

	for (n = 0; n < Polygon->PointN; n++) {
		double u, dx, dy;
		ptr1 = &Polygon->Points[prev_contour_point(Polygon, n)];
		ptr2 = &Polygon->Points[n];

		dx = ptr2->X - ptr1->X;
		dy = ptr2->Y - ptr1->Y;
		if (dx != 0.0 || dy != 0.0) {
			/* projected intersection is at P1 + u(P2 - P1) */
			u = ((X - ptr1->X) * dx + (Y - ptr1->Y) * dy) / (dx * dx + dy * dy);

			if (u < 0.0) {						/* ptr1 is closest point */
				u = PCB_SQUARE(X - ptr1->X) + PCB_SQUARE(Y - ptr1->Y);
			}
			else if (u > 1.0) {				/* ptr2 is closest point */
				u = PCB_SQUARE(X - ptr2->X) + PCB_SQUARE(Y - ptr2->Y);
			}
			else {										/* projected intersection is closest point */
				u = PCB_SQUARE(X - ptr1->X * (1.0 - u) - u * ptr2->X) + PCB_SQUARE(Y - ptr1->Y * (1.0 - u) - u * ptr2->Y);
			}
			if (u < mindistance) {
				mindistance = u;
				result = n;
			}
		}
	}
	return (result);
}

/* ---------------------------------------------------------------------------
 * go back to the  previous point of the polygon
 */
void GoToPreviousPoint(void)
{
	switch (Crosshair.AttachedPolygon.PointN) {
		/* do nothing if mode has just been entered */
	case 0:
		break;

		/* reset number of points and 'PCB_MODE_LINE' state */
	case 1:
		Crosshair.AttachedPolygon.PointN = 0;
		Crosshair.AttachedLine.State = STATE_FIRST;
		addedLines = 0;
		break;

		/* back-up one point */
	default:
		{
			pcb_point_t *points = Crosshair.AttachedPolygon.Points;
			pcb_cardinal_t n = Crosshair.AttachedPolygon.PointN - 2;

			Crosshair.AttachedPolygon.PointN--;
			Crosshair.AttachedLine.Point1.X = points[n].X;
			Crosshair.AttachedLine.Point1.Y = points[n].Y;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * close polygon if possible
 */
void ClosePolygon(void)
{
	pcb_cardinal_t n = Crosshair.AttachedPolygon.PointN;

	/* check number of points */
	if (n >= 3) {
		/* if 45 degree lines are what we want do a quick check
		 * if closing the polygon makes sense
		 */
		if (!conf_core.editor.all_direction_lines) {
			pcb_coord_t dx, dy;

			dx = coord_abs(Crosshair.AttachedPolygon.Points[n - 1].X - Crosshair.AttachedPolygon.Points[0].X);
			dy = coord_abs(Crosshair.AttachedPolygon.Points[n - 1].Y - Crosshair.AttachedPolygon.Points[0].Y);
			if (!(dx == 0 || dy == 0 || dx == dy)) {
				pcb_message(PCB_MSG_ERROR, _("Cannot close polygon because 45 degree lines are requested.\n"));
				return;
			}
		}
		CopyAttachedPolygonToLayer();
		pcb_draw();
	}
	else
		pcb_message(PCB_MSG_DEFAULT, _("A polygon has to have at least 3 points\n"));
}

/* ---------------------------------------------------------------------------
 * moves the data of the attached (new) polygon to the current layer
 */
void CopyAttachedPolygonToLayer(void)
{
	pcb_polygon_t *polygon;
	int saveID;

	/* move data to layer and clear attached struct */
	polygon = pcb_poly_new(CURRENT, pcb_no_flags());
	saveID = polygon->ID;
	*polygon = Crosshair.AttachedPolygon;
	polygon->ID = saveID;
	PCB_FLAG_SET(PCB_FLAG_CLEARPOLY, polygon);
	if (conf_core.editor.full_poly)
		PCB_FLAG_SET(PCB_FLAG_FULLPOLY, polygon);
	memset(&Crosshair.AttachedPolygon, 0, sizeof(pcb_polygon_t));
	pcb_poly_bbox(polygon);
	if (!CURRENT->polygon_tree)
		CURRENT->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(CURRENT->polygon_tree, (pcb_box_t *) polygon, 0);
	InitClip(PCB->Data, CURRENT, polygon);
	DrawPolygon(CURRENT, polygon);
	SetChangedFlag(pcb_true);

	/* reset state of attached line */
	Crosshair.AttachedLine.State = STATE_FIRST;
	addedLines = 0;

	/* add to undo list */
	AddObjectToCreateUndoList(PCB_TYPE_POLYGON, CURRENT, polygon, polygon);
	IncrementUndoSerialNumber();
}

/* find polygon holes in range, then call the callback function for
 * each hole. If the callback returns non-zero, stop
 * the search.
 */
int
PolygonHoles(pcb_polygon_t * polygon, const pcb_box_t * range, int (*callback) (pcb_pline_t * contour, void *user_data), void *user_data)
{
	pcb_polyarea_t *pa = polygon->Clipped;
	pcb_pline_t *pl;
	/* If this hole is so big the polygon doesn't exist, then it's not
	 * really a hole.
	 */
	if (!pa)
		return 0;
	for (pl = pa->contours->next; pl; pl = pl->next) {
		if (range != NULL && (pl->xmin > range->X2 || pl->xmax < range->X1 || pl->ymin > range->Y2 || pl->ymax < range->Y1))
			continue;
		if (callback(pl, user_data)) {
			return 1;
		}
	}
	return 0;
}

struct plow_info {
	int type;
	void *ptr1, *ptr2;
	pcb_layer_t *layer;
	pcb_data_t *data;
	pcb_r_dir_t (*callback) (pcb_data_t *, pcb_layer_t *, pcb_polygon_t *, int, void *, void *);
};

static pcb_r_dir_t subtract_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_polygon_t *Polygon, int type, void *ptr1, void *ptr2)
{
	if (!Polygon->Clipped)
		return 0;
	switch (type) {
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
		SubtractPin(Data, (pcb_pin_t *) ptr2, Layer, Polygon);
		Polygon->NoHolesValid = 0;
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_LINE:
		SubtractLine((pcb_line_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_ARC:
		SubtractArc((pcb_arc_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_PAD:
		SubtractPad((pcb_pad_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_TEXT:
		SubtractText((pcb_text_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return R_DIR_FOUND_CONTINUE;
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t add_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_polygon_t *Polygon, int type, void *ptr1, void *ptr2)
{
	switch (type) {
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
		UnsubtractPin((pcb_pin_t *) ptr2, Layer, Polygon);
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_LINE:
		UnsubtractLine((pcb_line_t *) ptr2, Layer, Polygon);
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_ARC:
		UnsubtractArc((pcb_arc_t *) ptr2, Layer, Polygon);
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_PAD:
		UnsubtractPad((pcb_pad_t *) ptr2, Layer, Polygon);
		return R_DIR_FOUND_CONTINUE;
	case PCB_TYPE_TEXT:
		UnsubtractText((pcb_text_t *) ptr2, Layer, Polygon);
		return R_DIR_FOUND_CONTINUE;
	}
	return R_DIR_NOT_FOUND;
}

static pcb_r_dir_t plow_callback(const pcb_box_t * b, void *cl)
{
	struct plow_info *plow = (struct plow_info *) cl;
	pcb_polygon_t *polygon = (pcb_polygon_t *) b;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon))
		return plow->callback(plow->data, plow->layer, polygon, plow->type, plow->ptr1, plow->ptr2);
	return R_DIR_NOT_FOUND;
}

int
PlowsPolygon(pcb_data_t * Data, int type, void *ptr1, void *ptr2,
						 pcb_r_dir_t (*call_back) (pcb_data_t *data, pcb_layer_t *lay, pcb_polygon_t *poly, int type, void *ptr1, void *ptr2))
{
	pcb_box_t sb = ((pcb_pin_t *) ptr2)->BoundingBox;
	int r = 0, seen;
	struct plow_info info;

	info.type = type;
	info.ptr1 = ptr1;
	info.ptr2 = ptr2;
	info.data = Data;
	info.callback = call_back;
	switch (type) {
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
		if (type == PCB_TYPE_PIN || ptr1 == ptr2 || ptr1 == NULL) {
			LAYER_LOOP(Data, max_copper_layer);
			{
				info.layer = layer;
				r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
				r += seen;
			}
			END_LOOP;
		}
		else {
			GROUP_LOOP(Data, GetLayerGroupNumberByNumber(GetLayerNumber(Data, ((pcb_layer_t *) ptr1))));
			{
				info.layer = layer;
				r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
				r += seen;
			}
			END_LOOP;
		}
		break;
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
		/* the cast works equally well for lines and arcs */
		if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (pcb_line_t *) ptr2))
			return 0;
		/* silk doesn't plow */
		if (GetLayerNumber(Data, (pcb_layer_t *) ptr1) >= max_copper_layer)
			return 0;
		GROUP_LOOP(Data, GetLayerGroupNumberByNumber(GetLayerNumber(Data, ((pcb_layer_t *) ptr1))));
		{
			info.layer = layer;
			r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
			r += seen;
		}
		END_LOOP;
		break;
	case PCB_TYPE_PAD:
		{
			pcb_cardinal_t group = GetLayerGroupNumberByNumber(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (pcb_pad_t *) ptr2) ?
																									 solder_silk_layer : component_silk_layer);
			GROUP_LOOP(Data, group);
			{
				info.layer = layer;
				r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
				r += seen;
			}
			END_LOOP;
		}
		break;

	case PCB_TYPE_ELEMENT:
		{
			PCB_PIN_LOOP((pcb_element_t *) ptr1);
			{
				PlowsPolygon(Data, PCB_TYPE_PIN, ptr1, pin, call_back);
			}
			END_LOOP;
			PCB_PAD_LOOP((pcb_element_t *) ptr1);
			{
				PlowsPolygon(Data, PCB_TYPE_PAD, ptr1, pad, call_back);
			}
			END_LOOP;
		}
		break;
	}
	return r;
}

void RestoreToPolygon(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	if (type == PCB_TYPE_POLYGON)
		InitClip(PCB->Data, (pcb_layer_t *) ptr1, (pcb_polygon_t *) ptr2);
	else
		PlowsPolygon(Data, type, ptr1, ptr2, add_plow);
}

void ClearFromPolygon(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	if (type == PCB_TYPE_POLYGON)
		InitClip(PCB->Data, (pcb_layer_t *) ptr1, (pcb_polygon_t *) ptr2);
	else
		PlowsPolygon(Data, type, ptr1, ptr2, subtract_plow);
}

pcb_bool isects(pcb_polyarea_t * a, pcb_polygon_t *p, pcb_bool fr)
{
	pcb_polyarea_t *x;
	pcb_bool ans;
	ans = pcb_poly_touching(a, p->Clipped);
	/* argument may be register, so we must copy it */
	x = a;
	if (fr)
		poly_Free(&x);
	return ans;
}


pcb_bool IsPointInPolygon(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t r, pcb_polygon_t *p)
{
	pcb_polyarea_t *c;
	pcb_vector_t v;
	v[0] = X;
	v[1] = Y;
	if (pcb_poly_contour_inside(p->Clipped, v))
		return pcb_true;

	if (PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, p)) {
		pcb_polygon_t tmp = *p;

		/* Check all clipped-away regions that are now drawn because of full-poly */
		for (tmp.Clipped = p->Clipped->f; tmp.Clipped != p->Clipped; tmp.Clipped = tmp.Clipped->f)
			if (pcb_poly_contour_inside(tmp.Clipped, v))
				return pcb_true;
	}

	if (r < 1)
		return pcb_false;
	if (!(c = CirclePoly(X, Y, r)))
		return pcb_false;
	return isects(c, p, pcb_true);
}


pcb_bool IsPointInPolygonIgnoreHoles(pcb_coord_t X, pcb_coord_t Y, pcb_polygon_t *p)
{
	pcb_vector_t v;
	v[0] = X;
	v[1] = Y;
	return poly_InsideContour(p->Clipped->contours, v);
}

pcb_bool IsRectangleInPolygon(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_polygon_t *p)
{
	pcb_polyarea_t *s;
	if (!(s = RectPoly(min(X1, X2), max(X1, X2), min(Y1, Y2), max(Y1, Y2))))
		return pcb_false;
	return isects(s, p, pcb_true);
}

/* NB: This function will free the passed pcb_polyarea_t.
 *     It must only be passed a single pcb_polyarea_t (pa->f == pa->b == pa)
 */
static void r_NoHolesPolygonDicer(pcb_polyarea_t * pa, void (*emit) (pcb_pline_t *, void *), void *user_data)
{
	pcb_pline_t *p = pa->contours;

	if (!pa->contours->next) {		/* no holes */
		pa->contours = NULL;				/* The callback now owns the contour */
		/* Don't bother removing it from the pcb_polyarea_t's rtree
		   since we're going to free the pcb_polyarea_t below anyway */
		emit(p, user_data);
		poly_Free(&pa);
		return;
	}
	else {
		pcb_polyarea_t *poly2, *left, *right;

		/* make a rectangle of the left region slicing through the middle of the first hole */
		poly2 = RectPoly(p->xmin, (p->next->xmin + p->next->xmax) / 2, p->ymin, p->ymax);
		poly_AndSubtract_free(pa, poly2, &left, &right);
		if (left) {
			pcb_polyarea_t *cur, *next;
			cur = left;
			do {
				next = cur->f;
				cur->f = cur->b = cur;	/* Detach this polygon piece */
				r_NoHolesPolygonDicer(cur, emit, user_data);
				/* NB: The pcb_polyarea_t was freed by its use in the recursive dicer */
			}
			while ((cur = next) != left);
		}
		if (right) {
			pcb_polyarea_t *cur, *next;
			cur = right;
			do {
				next = cur->f;
				cur->f = cur->b = cur;	/* Detach this polygon piece */
				r_NoHolesPolygonDicer(cur, emit, user_data);
				/* NB: The pcb_polyarea_t was freed by its use in the recursive dicer */
			}
			while ((cur = next) != right);
		}
	}
}

void NoHolesPolygonDicer(pcb_polygon_t *p, const pcb_box_t * clip, void (*emit) (pcb_pline_t *, void *), void *user_data)
{
	pcb_polyarea_t *main_contour, *cur, *next;

	main_contour = poly_Create();
	/* copy the main poly only */
	pcb_poly_copy1(main_contour, p->Clipped);
	/* clip to the bounding box */
	if (clip) {
		pcb_polyarea_t *cbox = RectPoly(clip->X1, clip->X2, clip->Y1, clip->Y2);
		poly_Boolean_free(main_contour, cbox, &main_contour, PBO_ISECT);
	}
	if (main_contour == NULL)
		return;
	/* Now dice it up.
	 * NB: Could be more than one piece (because of the clip above)
	 */
	cur = main_contour;
	do {
		next = cur->f;
		cur->f = cur->b = cur;			/* Detach this polygon piece */
		r_NoHolesPolygonDicer(cur, emit, user_data);
		/* NB: The pcb_polyarea_t was freed by its use in the recursive dicer */
	}
	while ((cur = next) != main_contour);
}

/* make a polygon split into multiple parts into multiple polygons */
pcb_bool MorphPolygon(pcb_layer_t *layer, pcb_polygon_t *poly)
{
	pcb_polyarea_t *p, *start;
	pcb_bool many = pcb_false;
	pcb_flag_t flags;

	if (!poly->Clipped || PCB_FLAG_TEST(PCB_FLAG_LOCK, poly))
		return pcb_false;
	if (poly->Clipped->f == poly->Clipped)
		return pcb_false;
	ErasePolygon(poly);
	start = p = poly->Clipped;
	/* This is ugly. The creation of the new polygons can cause
	 * all of the polygon pointers (including the one we're called
	 * with to change if there is a realloc in pcb_poly_alloc().
	 * That will invalidate our original "poly" argument, potentially
	 * inside the loop. We need to steal the Clipped pointer and
	 * hide it from the Remove call so that it still exists while
	 * we do this dirty work.
	 */
	poly->Clipped = NULL;
	if (poly->NoHoles)
		printf("Just leaked in MorpyPolygon\n");
	poly->NoHoles = NULL;
	flags = poly->Flags;
	pcb_poly_remove(layer, poly);
	inhibit = pcb_true;
	do {
		pcb_vnode_t *v;
		pcb_polygon_t *newone;

		if (p->contours->area > PCB->IsleArea) {
			newone = pcb_poly_new(layer, flags);
			if (!newone)
				return pcb_false;
			many = pcb_true;
			v = &p->contours->head;
			pcb_poly_point_new(newone, v->point[0], v->point[1]);
			for (v = v->next; v != &p->contours->head; v = v->next)
				pcb_poly_point_new(newone, v->point[0], v->point[1]);
			newone->BoundingBox.X1 = p->contours->xmin;
			newone->BoundingBox.X2 = p->contours->xmax + 1;
			newone->BoundingBox.Y1 = p->contours->ymin;
			newone->BoundingBox.Y2 = p->contours->ymax + 1;
			AddObjectToCreateUndoList(PCB_TYPE_POLYGON, layer, newone, newone);
			newone->Clipped = p;
			p = p->f;									/* go to next pline */
			newone->Clipped->b = newone->Clipped->f = newone->Clipped;	/* unlink from others */
			r_insert_entry(layer->polygon_tree, (pcb_box_t *) newone, 0);
			DrawPolygon(layer, newone);
		}
		else {
			pcb_polyarea_t *t = p;

			p = p->f;
			pcb_poly_contour_del(&t->contours);
			free(t);
		}
	}
	while (p != start);
	inhibit = pcb_false;
	IncrementUndoSerialNumber();
	return many;
}

void debug_pline(pcb_pline_t * pl)
{
	pcb_vnode_t *v;
	pcb_fprintf(stderr, "\txmin %#mS xmax %#mS ymin %#mS ymax %#mS\n", pl->xmin, pl->xmax, pl->ymin, pl->ymax);
	v = &pl->head;
	while (v) {
		pcb_fprintf(stderr, "\t\tvnode: %#mD\n", v->point[0], v->point[1]);
		v = v->next;
		if (v == &pl->head)
			break;
	}
}

void debug_polyarea(pcb_polyarea_t * p)
{
	pcb_pline_t *pl;

	fprintf(stderr, "pcb_polyarea_t %p\n", (void *)p);
	for (pl = p->contours; pl; pl = pl->next)
		debug_pline(pl);
}

void debug_polygon(pcb_polygon_t * p)
{
	pcb_cardinal_t i;
	pcb_polyarea_t *pa;
	fprintf(stderr, "POLYGON %p  %d pts\n", (void *)p, p->PointN);
	for (i = 0; i < p->PointN; i++)
		pcb_fprintf(stderr, "\t%d: %#mD\n", i, p->Points[i].X, p->Points[i].Y);
	if (p->HoleIndexN) {
		fprintf(stderr, "%d holes, starting at indices\n", p->HoleIndexN);
		for (i = 0; i < p->HoleIndexN; i++)
			fprintf(stderr, "\t%d: %d\n", i, p->HoleIndex[i]);
	}
	else
		fprintf(stderr, "it has no holes\n");
	pa = p->Clipped;
	while (pa) {
		debug_polyarea(pa);
		pa = pa->f;
		if (pa == p->Clipped)
			break;
	}
}

/* Convert a pcb_polyarea_t (and all linked pcb_polyarea_t) to
 * raw PCB polygons on the given layer.
 */
void PolyToPolygonsOnLayer(pcb_data_t * Destination, pcb_layer_t * Layer, pcb_polyarea_t * Input, pcb_flag_t Flags)
{
	pcb_polygon_t *Polygon;
	pcb_polyarea_t *pa;
	pcb_pline_t *pline;
	pcb_vnode_t *node;
	pcb_bool outer;

	if (Input == NULL)
		return;

	pa = Input;
	do {
		Polygon = pcb_poly_new(Layer, Flags);

		pline = pa->contours;
		outer = pcb_true;

		do {
			if (!outer)
				pcb_poly_hole_new(Polygon);
			outer = pcb_false;

			node = &pline->head;
			do {
				pcb_poly_point_new(Polygon, node->point[0], node->point[1]);
			}
			while ((node = node->next) != &pline->head);

		}
		while ((pline = pline->next) != NULL);

		InitClip(Destination, Layer, Polygon);
		pcb_poly_bbox(Polygon);
		if (!Layer->polygon_tree)
			Layer->polygon_tree = r_create_tree(NULL, 0, 0);
		r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);

		DrawPolygon(Layer, Polygon);
		/* add to undo list */
		AddObjectToCreateUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	}
	while ((pa = pa->f) != Input);

	SetChangedFlag(pcb_true);
}
