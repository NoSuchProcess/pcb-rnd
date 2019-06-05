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

/* pcb-rnd specific polygon editing routines
   see doc/developer/polygon.html for more info */

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
#include "thermal.h"
#include "undo.h"
#include "layer.h"
#include "obj_poly_draw.h"
#include "polygon_selfi.h"
#include "event.h"

#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

#define UNSUBTRACT_BLOAT 10
#define SUBTRACT_PIN_VIA_BATCH_SIZE 100
#define SUBTRACT_PADSTACK_BATCH_SIZE 50
#define SUBTRACT_LINE_BATCH_SIZE 20

#define sqr(x) ((x)*(x))

#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

static double rotate_circle_seg[4];

static int Unsubtract(pcb_polyarea_t * np1, pcb_poly_t * p);

static const char *polygon_cookie = "core polygon";


void pcb_poly_layers_chg(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_layer_t *ly;
	pcb_data_t *data;

	if ((argc < 2) || (argv[1].type != PCB_EVARG_PTR))
		return;
	ly = argv[1].d.p;
	if ((ly->is_bound) || (ly->parent_type != PCB_PARENT_DATA))
		return;

	data = ly->parent.data;
	pcb_data_clip_inhibit_inc(data);
	PCB_POLY_LOOP(ly); {
		polygon->clip_dirty = 1;
	}
	PCB_END_LOOP;
	pcb_data_clip_inhibit_dec(data, 1);
}

void pcb_polygon_init(void)
{
	double cos_ang = cos(2.0 * M_PI / PCB_POLY_CIRC_SEGS_F);
	double sin_ang = sin(2.0 * M_PI / PCB_POLY_CIRC_SEGS_F);

	rotate_circle_seg[0] = cos_ang;
	rotate_circle_seg[1] = -sin_ang;
	rotate_circle_seg[2] = sin_ang;
	rotate_circle_seg[3] = cos_ang;

	pcb_event_bind(PCB_EVENT_LAYER_CHANGED_GRP, pcb_poly_layers_chg, NULL, polygon_cookie);
}

void pcb_polygon_uninit(void)
{
	pcb_event_unbind_allcookie(polygon_cookie);
}

pcb_cardinal_t pcb_poly_point_idx(pcb_poly_t *polygon, pcb_point_t *point)
{
	assert(point >= polygon->Points);
	assert(point <= polygon->Points + polygon->PointN);
	return ((char *) point - (char *) polygon->Points) / sizeof(pcb_point_t);
}

/* Find contour number: 0 for outer, 1 for first hole etc.. */
pcb_cardinal_t pcb_poly_contour_point(pcb_poly_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t i;
	pcb_cardinal_t contour = 0;

	for (i = 0; i < polygon->HoleIndexN; i++)
		if (point >= polygon->HoleIndex[i])
			contour = i + 1;
	return contour;
}

pcb_cardinal_t pcb_poly_contour_next_point(pcb_poly_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t contour;
	pcb_cardinal_t this_contour_start;
	pcb_cardinal_t next_contour_start;

	contour = pcb_poly_contour_point(polygon, point);

	this_contour_start = (contour == 0) ? 0 : polygon->HoleIndex[contour - 1];
	next_contour_start = (contour == polygon->HoleIndexN) ? polygon->PointN : polygon->HoleIndex[contour];

	/* Wrap back to the start of the contour we're in if we pass the end */
	if (++point == next_contour_start)
		point = this_contour_start;

	return point;
}

pcb_cardinal_t pcb_poly_contour_prev_point(pcb_poly_t *polygon, pcb_cardinal_t point)
{
	pcb_cardinal_t contour;
	pcb_cardinal_t prev_contour_end;
	pcb_cardinal_t this_contour_end;

	contour = pcb_poly_contour_point(polygon, point);

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
	pcb_poly_t *poly = (pcb_poly_t *) user_data;

	/* Prepend the pline into the NoHoles linked list */
	pline->next = poly->NoHoles;
	poly->NoHoles = pline;
}

void pcb_poly_compute_no_holes(pcb_poly_t * poly)
{
	pcb_poly_contours_free(&poly->NoHoles);
	if (poly->Clipped)
		pcb_poly_no_holes_dicer(poly, NULL, add_noholes_polyarea, poly);
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
		if (n->contours->area < conf_core.design.poly_isle_area) {
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

/* Convert a polygon to an unclipped polyarea */
static pcb_polyarea_t *original_poly(pcb_poly_t *p, pcb_bool *need_full)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np1 = NULL, *np = NULL;
	pcb_cardinal_t n;
	pcb_vector_t v;
	int hole = 0;

	*need_full = pcb_false;

	np1 = np = pcb_polyarea_create();
	if (np == NULL)
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
			if (contour->Flags.orient != (hole ? PCB_PLF_INV : PCB_PLF_DIR))
				pcb_poly_contour_inv(contour);
			assert(contour->Flags.orient == (hole ? PCB_PLF_INV : PCB_PLF_DIR));

			/* attempt to auto-correct simple self intersecting cases */
			if (0 && pcb_pline_is_selfint(contour)) {
				vtp0_t islands;
				int n;

				/* self intersecting polygon - attempt to fix it by splitting up into multiple polyareas */
				vtp0_init(&islands);
				pcb_pline_split_selfint(contour, &islands);
				for(n = 0; n < vtp0_len(&islands); n++) {
					pcb_pline_t *pl = *vtp0_get(&islands, n, 0);
					if (n > 0) {
						pcb_polyarea_t *newpa = pcb_polyarea_create();
						newpa->b = np;
						newpa->f = np->f;
						np->f->b = newpa;
						np->f = newpa;
						np = newpa;
						*need_full = pcb_true;
					}
					pcb_poly_contour_pre(pl, pcb_true);
					if (pl->Flags.orient != (hole ? PCB_PLF_INV : PCB_PLF_DIR))
						pcb_poly_contour_inv(pl);
					assert(pl->Flags.orient == (hole ? PCB_PLF_INV : PCB_PLF_DIR));
					pcb_polyarea_contour_include(np, pl);
				}
				vtp0_uninit(&islands);
				free(contour);
			}
			else
				pcb_polyarea_contour_include(np, contour);
			contour = NULL;
			assert(pcb_poly_valid(np));

			hole++;
		}
	}
	return biggest(np1);
}

pcb_polyarea_t *pcb_poly_from_poly(pcb_poly_t * p)
{
	pcb_bool tmp;
	return original_poly(p, &tmp);
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

#define ARC_ANGLE 5
static pcb_polyarea_t *ArcPolyNoIntersect(pcb_arc_t * a, pcb_coord_t thick, int end_caps)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np = NULL;
	pcb_vector_t v, v2;
	pcb_box_t ends;
	int i, segs;
	double ang, da, rx, ry;
	long half;
	double radius_adj;
	pcb_coord_t edx, edy;

	if (thick <= 0)
		return NULL;
	if (a->Delta < 0) {
		a->StartAngle += a->Delta;
		a->Delta = -a->Delta;
	}
	half = (thick + 1) / 2;

	pcb_arc_get_end(a, 0, &ends.X1, &ends.Y1);
	pcb_arc_get_end(a, 1, &ends.X2, &ends.Y2);

	/* start with inner radius */
	rx = MAX(a->Width - half, 0);
	ry = MAX(a->Height - half, 0);
	segs = 1;
	if (thick > 0)
		segs = MAX(segs, a->Delta * M_PI / 360 *
							 sqrt(sqrt((double) rx * rx + (double) ry * ry) / PCB_POLY_ARC_MAX_DEVIATION / 2 / thick));
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
	if (end_caps)
		pcb_poly_frac_circle(contour, ends.X2, ends.Y2, v, 2);

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

	/* explicitly draw the last point if the manhattan-distance is large enough */
	ang = a->StartAngle;
	v2[0] = a->X - rx * cos(ang * PCB_M180) * (1 - radius_adj);
	v2[1] = a->Y + ry * sin(ang * PCB_M180) * (1 - radius_adj);
	edx = (v[0] - v2[0]);
	edy = (v[1] - v2[1]);
	if (edx < 0) edx = -edx;
	if (edy < 0) edy = -edy;
	if (edx+edy > PCB_MM_TO_COORD(0.001))
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v2));


	/* now add other round cap */
	if (end_caps)
		pcb_poly_frac_circle(contour, ends.X1, ends.Y1, v2, 2);

	/* now we have the whole contour */
	if (!(np = pcb_poly_from_contour(contour)))
		return NULL;
	return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
pcb_polyarea_t *pcb_poly_from_pcb_arc(pcb_arc_t * a, pcb_coord_t thick)
{
	double delta;
	pcb_arc_t seg1, seg2;
	pcb_coord_t half;

	delta = (a->Delta < 0) ? -a->Delta : a->Delta;

	half = (thick + 1) / 2;

	/* corner case: can't even calculate the end cap properly because radius
	   is so small that there's no inner arc of the clearance */
	if ((a->Width - half <= 0) || (a->Height - half <= 0)) {
		pcb_line_t lin = {0};
		pcb_polyarea_t *tmp_arc, *tmp1, *tmp2, *res, *ends;

		tmp_arc = ArcPolyNoIntersect(a, thick, 0);

		pcb_arc_get_end(a, 0, &lin.Point1.X, &lin.Point1.Y);
		lin.Point2.X = lin.Point1.X;
		lin.Point2.Y = lin.Point1.Y;
		tmp1 = pcb_poly_from_pcb_line(&lin, thick);

		pcb_arc_get_end(a, 1, &lin.Point1.X, &lin.Point1.Y);
		lin.Point2.X = lin.Point1.X;
		lin.Point2.Y = lin.Point1.Y;
		tmp2 = pcb_poly_from_pcb_line(&lin, thick);

		pcb_polyarea_boolean_free(tmp1, tmp2, &ends, PCB_PBO_UNITE);
		pcb_polyarea_boolean_free(ends, tmp_arc, &res, PCB_PBO_UNITE);
		return res;
	}

	/* If the arc segment would self-intersect, we need to construct it as the union of
	   two non-intersecting segments */
	if (2 * M_PI * a->Width * (1. - (double) delta / 360.) - thick < MIN_CLEARANCE_BEFORE_BISECT) {
		pcb_polyarea_t *tmp1, *tmp2, *res;
		int half_delta = a->Delta / 2;

		seg1 = seg2 = *a;
		seg1.Delta = half_delta;
		seg2.Delta -= half_delta;
		seg2.StartAngle += half_delta;

		tmp1 = ArcPolyNoIntersect(&seg1, thick, 1);
		tmp2 = ArcPolyNoIntersect(&seg2, thick, 1);
		pcb_polyarea_boolean_free(tmp1, tmp2, &res, PCB_PBO_UNITE);
		return res;
	}

	return ArcPolyNoIntersect(a, thick, 1);
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

pcb_polyarea_t *pcb_poly_from_pcb_line(pcb_line_t *L, pcb_coord_t thick)
{
	return pcb_poly_from_line(L->Point1.X, L->Point1.Y, L->Point2.X, L->Point2.Y, thick, PCB_FLAG_TEST(PCB_FLAG_SQUARE, L));
}


/* clear np1 from the polygon - should be inline with -O3 */
static int Subtract(pcb_polyarea_t * np1, pcb_poly_t * p, pcb_bool fnp)
{
	pcb_polyarea_t *merged = NULL, *np = np1;
	int x;
	assert(np);
	assert(p);
	if (!p->Clipped) {
		if (fnp)
			pcb_polyarea_free(&np);
		return 1;
	}
	assert(pcb_poly_valid(p->Clipped));
	assert(pcb_poly_valid(np));
	if (fnp)
		x = pcb_polyarea_boolean_free(p->Clipped, np, &merged, PCB_PBO_SUB);
	else {
		x = pcb_polyarea_boolean(p->Clipped, np, &merged, PCB_PBO_SUB);
		pcb_polyarea_free(&p->Clipped);
	}
	assert(!merged || pcb_poly_valid(merged));
	if (x != pcb_err_ok) {
		fprintf(stderr, "Error while clipping PCB_PBO_SUB: %d\n", x);
		pcb_polyarea_free(&merged);
		p->Clipped = NULL;
		if (p->NoHoles)
			printf("Just leaked in Subtract\n");
		p->NoHoles = NULL;
		return -1;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || pcb_poly_valid(p->Clipped));
	if (!p->Clipped)
		pcb_message(PCB_MSG_WARNING, "Polygon cleared out of existence near (%$mm, %$mm)\n",
						(p->BoundingBox.X1 + p->BoundingBox.X2) / 2, (p->BoundingBox.Y1 + p->BoundingBox.Y2) / 2);
	return 1;
}

int pcb_poly_subtract(pcb_polyarea_t *np1, pcb_poly_t *p, pcb_bool fnp)
{
	return Subtract(np1, p, fnp);
}

pcb_polyarea_t *pcb_poly_from_box_bloated(pcb_box_t * box, pcb_coord_t bloat)
{
	return pcb_poly_from_rect(box->X1 - bloat, box->X2 + bloat, box->Y1 - bloat, box->Y2 + bloat);
}

/* remove the padstack clearance from the polygon */
static int SubtractPadstack(pcb_data_t *d, pcb_pstk_t *ps, pcb_layer_t *l, pcb_poly_t *p)
{
	pcb_polyarea_t *np;
	pcb_layer_id_t i;

	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return 0;
	i = pcb_layer_id(d, l);
	np = pcb_thermal_area_pstk(pcb_data_get_top(d), ps, i);
	if (np == 0)
		return 0;

	return Subtract(np, p, pcb_true);
}

/* return the clearance polygon for a line */
static pcb_polyarea_t *line_clearance_poly(pcb_cardinal_t layernum, pcb_board_t *pcb, pcb_line_t *line)
{
	if (line->thermal & PCB_THERMAL_ON)
		return pcb_thermal_area_line(pcb, line, layernum);
	return pcb_poly_from_pcb_line(line, line->Thickness + line->Clearance);
}

static int SubtractLine(pcb_line_t * line, pcb_poly_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return 0;
	if (!(np = line_clearance_poly(-1, NULL, line)))
		return -1;
	return Subtract(np, p, pcb_true);
}

static int SubtractArc(pcb_arc_t * arc, pcb_poly_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return 0;
	if (!(np = pcb_poly_from_pcb_arc(arc, arc->Thickness + arc->Clearance)))
		return -1;
	return Subtract(np, p, pcb_true);
}

static int SubtractText(pcb_text_t * text, pcb_poly_t * p)
{
	pcb_polyarea_t *np;
	const pcb_box_t *b = &text->BoundingBox;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;
	if (!(np = RoundRect(b->X1 + conf_core.design.bloat, b->X2 - conf_core.design.bloat, b->Y1 + conf_core.design.bloat, b->Y2 - conf_core.design.bloat, conf_core.design.bloat)))
		return -1;
	return Subtract(np, p, pcb_true);
}

struct cpInfo {
	const pcb_box_t *other;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_poly_t *polygon;
	pcb_bool solder;
	pcb_polyarea_t *accumulate;
	int batch_size;
	jmp_buf env;
};

static void subtract_accumulated(struct cpInfo *info, pcb_poly_t *polygon)
{
	if (info->accumulate == NULL)
		return;
	Subtract(info->accumulate, polygon, pcb_true);
	info->accumulate = NULL;
	info->batch_size = 0;
}

static int pcb_poly_clip_noop = 0;
static void *pcb_poly_clip_prog_ctx;
static void (*pcb_poly_clip_prog)(void *ctx) = NULL;

/* call the progress report callback and return if no-op is set */
#define POLY_CLIP_PROG() \
do { \
	if (pcb_poly_clip_prog != NULL) \
		pcb_poly_clip_prog(pcb_poly_clip_prog_ctx); \
	if (pcb_poly_clip_noop) \
		return PCB_R_DIR_FOUND_CONTINUE; \
} while(0)

static pcb_r_dir_t padstack_sub_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	struct cpInfo *info = (struct cpInfo *)cl;
	pcb_poly_t *polygon;
	pcb_polyarea_t *np;
	pcb_polyarea_t *merged;
	pcb_layer_id_t i;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return PCB_R_DIR_NOT_FOUND;
	polygon = info->polygon;

	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return PCB_R_DIR_NOT_FOUND;

	i = pcb_layer_id(info->data, info->layer);

	np = pcb_thermal_area_pstk(pcb_data_get_top(info->data), ps, i);
	if (np == 0)
			return PCB_R_DIR_FOUND_CONTINUE;

	info->batch_size++;
	POLY_CLIP_PROG();

	pcb_polyarea_boolean_free(info->accumulate, np, &merged, PCB_PBO_UNITE);
	info->accumulate = merged;

	if (info->batch_size == SUBTRACT_PADSTACK_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t arc_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return PCB_R_DIR_NOT_FOUND;
	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return PCB_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	polygon = info->polygon;

	if (SubtractArc(arc, polygon) < 0)
		longjmp(info->env, 1);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* quick create a polygon area from a line, knowing the coords and width */
static pcb_polyarea_t *poly_sub_callback_line(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t width)
{
	static pcb_line_t lin;
	static int inited = 0;

	if (!inited) {
		lin.type = PCB_OBJ_LINE;
		lin.Flags = pcb_no_flags();
		PCB_FLAG_SET(PCB_FLAG_CLEARLINE, &lin);
		lin.Thickness = 0;
		inited = 1;
	}

	lin.Point1.X = x1;
	lin.Point1.Y = y1;
	lin.Point2.X = x2;
	lin.Point2.Y = y2;
	lin.Clearance = width;

	return pcb_poly_from_pcb_line(&lin, width);
}

#define pa_append(src) \
	do { \
		pcb_polyarea_boolean(*dst, src, &tmp, PCB_PBO_UNITE); \
		pcb_polyarea_free(dst); \
		*dst = tmp; \
	} while(0)

void pcb_poly_pa_clearance_construct(pcb_polyarea_t **dst, pcb_poly_it_t *it, pcb_coord_t clearance)
{
	pcb_polyarea_t *tmp, *lin;
	pcb_coord_t x, y, px, py, x0, y0;
	pcb_pline_t *pl;
	int go;
	pcb_cardinal_t cnt;

	if (*dst != NULL)
		pa_append(it->pa);
	else
		pcb_polyarea_copy0(dst, it->pa);

	clearance *= 2;

	/* care about the outer contours only */
	pl = pcb_poly_contour(it);
	if (pl != NULL) {
		/* iterate over the vectors of the contour */
		for(go = pcb_poly_vect_first(it, &x, &y), cnt = 0; go; go = pcb_poly_vect_next(it, &x, &y), cnt++) {
			if (cnt > 0) {
				lin = poly_sub_callback_line(px, py, x, y, clearance);
				pa_append(lin);
				pcb_polyarea_free(&lin);
			}
			else {
				x0 = x;
				y0 = y;
			}
			px = x;
			py = y;
		}
		if (cnt > 0) {
			lin = poly_sub_callback_line(px, py, x0, y0, clearance);
			pa_append(lin);
			pcb_polyarea_free(&lin);
		}
	}
}
#undef pa_append

/* Construct a poly area that represents the enlarged subpoly - so it can be
   subtracted from the parent poly to form the clearance for subpoly */
pcb_polyarea_t *pcb_poly_clearance_construct(pcb_poly_t *subpoly)
{
	pcb_polyarea_t *ret = NULL, *pa;
	pcb_poly_it_t it;

	/* iterate over all islands of a polygon */
	for(pa = pcb_poly_island_first(subpoly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
		pcb_poly_pa_clearance_construct(&ret, &it, subpoly->Clearance/2);

	return ret;
}

/* return the clearance polygon for a line */
static pcb_polyarea_t *poly_clearance_poly(pcb_cardinal_t layernum, pcb_board_t *pcb, pcb_poly_t *subpoly)
{
	if (subpoly->thermal & PCB_THERMAL_ON)
		return pcb_thermal_area_poly(pcb, subpoly, layernum);
	return pcb_poly_clearance_construct(subpoly);
}


static int SubtractPolyPoly(pcb_poly_t *subpoly, pcb_poly_t *frompoly)
{
	pcb_polyarea_t *pa;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, frompoly)) /* two clearing polys won't interact */
		return 0; /* but it's not an error, that'd kill other clearances in the same poly */

	pa = poly_clearance_poly(-1, NULL, subpoly);
	if (pa == NULL)
		return -1;

	Subtract(pa, frompoly, pcb_true);
	frompoly->NoHolesValid = 0;
	return 0;
}


static int UnsubtractPolyPoly(pcb_poly_t *subpoly, pcb_poly_t *frompoly)
{
	pcb_polyarea_t *pa;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, frompoly)) /* two clearing polys won't interact */
		return 0; /* but it's not an error, that'd kill other clearances in the same poly */

	pa = poly_clearance_poly(-1, NULL, subpoly);
	if (pa == NULL)
		return -1;

	if (!Unsubtract(pa, frompoly))
		return -1;
	frompoly->NoHolesValid = 0;
	return 0;
}

static pcb_r_dir_t poly_sub_callback(const pcb_box_t *b, void *cl)
{
	pcb_poly_t *subpoly = (pcb_poly_t *)b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	polygon = info->polygon;

	/* don't do clearance in itself */
	if (subpoly == polygon)
		return PCB_R_DIR_NOT_FOUND;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return PCB_R_DIR_NOT_FOUND;
	if (!PCB_POLY_HAS_CLEARANCE(subpoly))
		return PCB_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	if (SubtractPolyPoly(subpoly, polygon) < 0)
		longjmp(info->env, 1);

	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t line_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;
	pcb_polyarea_t *np;
	pcb_polyarea_t *merged;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return PCB_R_DIR_NOT_FOUND;
	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return PCB_R_DIR_NOT_FOUND;
	polygon = info->polygon;

	POLY_CLIP_PROG();

	np = line_clearance_poly(-1, NULL, line);
	if (!np)
		longjmp(info->env, 1);

	pcb_polyarea_boolean_free(info->accumulate, np, &merged, PCB_PBO_UNITE);
	info->accumulate = merged;
	info->batch_size++;

	if (info->batch_size == SUBTRACT_LINE_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t text_sub_callback(const pcb_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return PCB_R_DIR_NOT_FOUND;
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return PCB_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	polygon = info->polygon;
	if (SubtractText(text, polygon) < 0)
		longjmp(info->env, 1);
	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_cardinal_t clearPoly(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *polygon, const pcb_box_t *here, pcb_coord_t expand, int noop)
{
	pcb_cardinal_t r = 0;
	int seen;
	pcb_box_t region;
	struct cpInfo info;
	pcb_layergrp_id_t group;
	unsigned int gflg;
	pcb_layer_type_t lf;
	int old_noop;

	lf = pcb_layer_flags_(Layer);
	if (!(lf & PCB_LYT_COPPER)) { /* also handles lf == 0 */
		polygon->NoHolesValid = 0;
		return 0;
	}

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon))
		return 0;

	old_noop = pcb_poly_clip_noop;
	pcb_poly_clip_noop = noop;

	group = pcb_layer_get_group_(Layer);
	gflg = pcb_layergrp_flags(PCB, group);
	info.solder = (gflg & PCB_LYT_BOTTOM);
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
		PCB_COPPER_GROUP_LOOP(Data, group);
		{
			pcb_r_search(layer->line_tree, &region, NULL, line_sub_callback, &info, &seen);
			r += seen;
			subtract_accumulated(&info, polygon);
			pcb_r_search(layer->arc_tree, &region, NULL, arc_sub_callback, &info, &seen);
			r += seen;
			pcb_r_search(layer->text_tree, &region, NULL, text_sub_callback, &info, &seen);
			r += seen;
			pcb_r_search(layer->polygon_tree, &region, NULL, poly_sub_callback, &info, &seen);
			r += seen;
		}
		PCB_END_LOOP;
		pcb_r_search(Data->padstack_tree, &region, NULL, padstack_sub_callback, &info, &seen);
		r += seen;
		subtract_accumulated(&info, polygon);
	}
	if (!noop)
		polygon->NoHolesValid = 0;
	pcb_poly_clip_noop = old_noop;
	return r;
}

static int Unsubtract(pcb_polyarea_t * np1, pcb_poly_t * p)
{
	pcb_polyarea_t *merged = NULL, *np = np1;
	pcb_polyarea_t *orig_poly, *clipped_np;
	int x;
	pcb_bool need_full;
	assert(np);
	assert(p); /* NOTE: p->clipped might be NULL if a poly is "cleared out of existence" and is now coming back */

	orig_poly = original_poly(p, &need_full);

	x = pcb_polyarea_boolean_free(np, orig_poly, &clipped_np, PCB_PBO_ISECT);
	if (x != pcb_err_ok) {
		fprintf(stderr, "Error while clipping PCB_PBO_ISECT: %d\n", x);
		pcb_polyarea_free(&clipped_np);
		goto fail;
	}

	x = pcb_polyarea_boolean_free(p->Clipped, clipped_np, &merged, PCB_PBO_UNITE);
	if (x != pcb_err_ok) {
		fprintf(stderr, "Error while clipping PCB_PBO_UNITE: %d\n", x);
		pcb_polyarea_free(&merged);
		goto fail;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || pcb_poly_valid(p->Clipped));
	return 1;

fail:
	p->Clipped = NULL;
	if (p->NoHoles)
		printf("Just leaked in Unsubtract\n");
	p->NoHoles = NULL;
	return 0;
}

static int UnsubtractPadstack(pcb_data_t *data, pcb_pstk_t *ps, pcb_layer_t *l, pcb_poly_t *p)
{
	pcb_polyarea_t *np;

	/* overlap a bit to prevent gaps from rounding errors */
	np = pcb_poly_from_box_bloated(&ps->BoundingBox, UNSUBTRACT_BLOAT * 400000);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;

	clearPoly(PCB->Data, l, p, (const pcb_box_t *)ps, 2 * UNSUBTRACT_BLOAT * 400000, 0);
	return 1;
}



static int UnsubtractArc(pcb_arc_t * arc, pcb_layer_t * l, pcb_poly_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return 0;

	/* overlap a bit to prevent gaps from rounding errors */
	np = pcb_poly_from_box_bloated(&arc->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) arc, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static int UnsubtractLine(pcb_line_t * line, pcb_layer_t * l, pcb_poly_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = pcb_poly_from_box_bloated(&line->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) line, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static int UnsubtractText(pcb_text_t * text, pcb_layer_t * l, pcb_poly_t * p)
{
	pcb_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = pcb_poly_from_box_bloated(&text->BoundingBox, UNSUBTRACT_BLOAT);

	if (!np)
		return -1;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const pcb_box_t *) text, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static pcb_bool inhibit = pcb_false;

int pcb_poly_init_clip_prog(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p, void (*cb)(void *ctx), void *ctx)
{
	pcb_board_t *pcb;
	pcb_bool need_full;
	void (*old_cb)(void *ctx);
	void *old_ctx;

	if (inhibit)
		return 0;

	if (Data->clip_inhibit > 0) {
		p->clip_dirty = 1;
		return 0;
	}

	if (layer->is_bound)
		layer = layer->meta.bound.real;

	if (p->Clipped)
		pcb_polyarea_free(&p->Clipped);
	p->Clipped = original_poly(p, &need_full);
	if (need_full && !PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, p)) {
		pcb_message(PCB_MSG_WARNING, "Polygon #%ld was self intersecting; it had to be split up and\nthe full poly flag set.\n", (long)p->ID);
		PCB_FLAG_SET(PCB_FLAG_FULLPOLY, p);
	}
	pcb_poly_contours_free(&p->NoHoles);

	if (layer == NULL)
		return 0;

	if (!p->Clipped)
		return 0;
	assert(pcb_poly_valid(p->Clipped));

	/* calculate clipping for the real data (in case of subcircuits) */
	pcb = pcb_data_get_top(Data);
	if (pcb != NULL)
		Data = pcb->Data;

	old_cb = pcb_poly_clip_prog;
	old_ctx = pcb_poly_clip_prog_ctx;
	pcb_poly_clip_prog = cb;
	pcb_poly_clip_prog_ctx = ctx;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, p))
		clearPoly(Data, layer, p, NULL, 0, 0);
	else
		p->NoHolesValid = 0;

	pcb_poly_clip_prog = old_cb;
	pcb_poly_clip_prog_ctx = old_ctx;
	return 1;
}

int pcb_poly_init_clip(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p)
{
	return pcb_poly_init_clip_prog(Data, layer, p, NULL, NULL);
}

pcb_cardinal_t pcb_poly_num_clears(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon)
{
	pcb_cardinal_t res;
	void (*old_cb)(void *ctx);

	if (layer->is_bound)
		layer = layer->meta.bound.real;

	old_cb = pcb_poly_clip_prog;
	pcb_poly_clip_prog = NULL;

	res = clearPoly(data, layer, polygon, NULL, 0, 1);

	pcb_poly_clip_prog = old_cb;
	return res;
}

/* --------------------------------------------------------------------------
 * remove redundant polygon points. Any point that lies on the straight
 * line between the points on either side of it is redundant.
 * returns pcb_true if any points are removed
 */
pcb_bool pcb_poly_remove_excess_points(pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_point_t *p;
	pcb_cardinal_t n, prev, next;
	pcb_line_t line;
	pcb_bool changed = pcb_false;

	if (pcb_undoing())
		return pcb_false;

	for (n = 0; n < Polygon->PointN; n++) {
		prev = pcb_poly_contour_prev_point(Polygon, n);
		next = pcb_poly_contour_next_point(Polygon, n);
		p = &Polygon->Points[n];

		line.Point1 = Polygon->Points[prev];
		line.Point2 = Polygon->Points[next];
		line.Thickness = 0;
		if (pcb_is_point_on_line(p->X, p->Y, 0.0, &line)) {
			pcb_remove_object(PCB_OBJ_POLY_POINT, Layer, Polygon, p);
			changed = pcb_true;
		}
	}
	return changed;
}

/* ---------------------------------------------------------------------------
 * returns the index of the polygon point which is the end
 * point of the segment with the lowest distance to the passed
 * coordinates
 */
pcb_cardinal_t pcb_poly_get_lowest_distance_point(pcb_poly_t *Polygon, pcb_coord_t X, pcb_coord_t Y)
{
	double mindistance = (double) PCB_MAX_COORD * PCB_MAX_COORD;
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
		ptr1 = &Polygon->Points[pcb_poly_contour_prev_point(Polygon, n)];
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
	return result;
}

/* ---------------------------------------------------------------------------
 * go back to the previous point of the polygon (undo)
 */
void pcb_polygon_go_to_prev_point(void)
{
	switch (pcb_crosshair.AttachedPolygon.PointN) {
		/* do nothing if mode has just been entered */
	case 0:
		break;

		/* reset number of points and 'PCB_MODE_LINE' state */
	case 1:
		pcb_crosshair.AttachedPolygon.PointN = 0;
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
		break;

		/* back-up one point */
	default:
		{
			pcb_point_t *points = pcb_crosshair.AttachedPolygon.Points;
			pcb_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN - 2;

			pcb_crosshair.AttachedPolygon.PointN--;
			pcb_crosshair.AttachedLine.Point1.X = points[n].X;
			pcb_crosshair.AttachedLine.Point1.Y = points[n].Y;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * go forward to the next point of the polygon (redo)
 */
void pcb_polygon_go_to_next_point(void)
{
	if ((pcb_crosshair.AttachedPolygon.PointN > 0) && (pcb_crosshair.AttachedPolygon.PointN < pcb_crosshair.AttachedPolygon_pts)) {
		pcb_point_t *points = pcb_crosshair.AttachedPolygon.Points;
		pcb_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

		pcb_crosshair.AttachedPolygon.PointN++;
		pcb_crosshair.AttachedLine.Point1.X = points[n].X;
		pcb_crosshair.AttachedLine.Point1.Y = points[n].Y;
	}
}

/* ---------------------------------------------------------------------------
 * close polygon if possible
 */
void pcb_polygon_close_poly(void)
{
	pcb_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

	/* check number of points */
	if (n >= 3) {
		/* if 45 degree lines are what we want do a quick check
		 * if closing the polygon makes sense
		 */
		if (!conf_core.editor.all_direction_lines) {
			pcb_coord_t dx, dy;

			dx = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].X - pcb_crosshair.AttachedPolygon.Points[0].X);
			dy = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].Y - pcb_crosshair.AttachedPolygon.Points[0].Y);
			if (!(dx == 0 || dy == 0 || dx == dy)) {
				pcb_message(PCB_MSG_ERROR, "Cannot close polygon because 45 degree lines are requested.\n");
				return;
			}
		}
		pcb_polygon_copy_attached_to_layer();
		pcb_draw();
	}
	else
		pcb_message(PCB_MSG_ERROR, "A polygon has to have at least 3 points\n");
}

static void poly_copy_data(pcb_poly_t *dst, pcb_poly_t *src)
{
	pcb_poly_t p;
	void *old_parent = dst->parent.any;
	pcb_parenttype_t old_pt = dst->parent_type;

	memcpy(dst, src, ((char *)&p.link - (char *)&p));
	dst->type = PCB_OBJ_POLY;
	dst->parent.any = old_parent;
	dst->parent_type = old_pt;
}

/* ---------------------------------------------------------------------------
 * moves the data of the attached (new) polygon to the current layer
 */
void pcb_polygon_copy_attached_to_layer(void)
{
	pcb_layer_t *layer = pcb_loose_subc_layer(PCB, CURRENT, pcb_true);
	pcb_poly_t *polygon;
	int saveID;

	/* move data to layer and clear attached struct */
	polygon = pcb_poly_new(layer, 0, pcb_no_flags());
	saveID = polygon->ID;
	poly_copy_data(polygon, &pcb_crosshair.AttachedPolygon);
	polygon->Clearance = 2 * conf_core.design.clearance;
	polygon->ID = saveID;
	PCB_FLAG_SET(PCB_FLAG_CLEARPOLY, polygon);
	if (conf_core.editor.full_poly)
		PCB_FLAG_SET(PCB_FLAG_FULLPOLY, polygon);
	if (conf_core.editor.clear_polypoly)
		PCB_FLAG_SET(PCB_FLAG_CLEARPOLYPOLY, polygon);

	memset(&pcb_crosshair.AttachedPolygon, 0, sizeof(pcb_poly_t));

	pcb_add_poly_on_layer(layer, polygon);

	pcb_poly_init_clip(PCB->Data, layer, polygon);
	pcb_poly_invalidate_draw(layer, polygon);
	pcb_board_set_changed_flag(pcb_true);

	/* reset state of attached line */
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;

	/* add to undo list */
	pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, polygon, polygon);
	pcb_undo_inc_serial();
}

/* ---------------------------------------------------------------------------
 * close polygon hole if possible
 */
void pcb_polygon_close_hole(void)
{
	pcb_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

	/* check number of points */
	if (n >= 3) {
		/* if 45 degree lines are what we want do a quick check
		 * if closing the polygon makes sense
		 */
		if (!conf_core.editor.all_direction_lines) {
			pcb_coord_t dx, dy;

			dx = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].X - pcb_crosshair.AttachedPolygon.Points[0].X);
			dy = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].Y - pcb_crosshair.AttachedPolygon.Points[0].Y);
			if (!(dx == 0 || dy == 0 || dx == dy)) {
				pcb_message(PCB_MSG_ERROR, "Cannot close polygon hole because 45 degree lines are requested.\n");
				return;
			}
		}
		pcb_polygon_hole_create_from_attached();
		pcb_draw();
	}
	else
		pcb_message(PCB_MSG_ERROR, "A polygon hole has to have at least 3 points\n");
}

/* ---------------------------------------------------------------------------
 * creates a hole of attached polygon shape in the underneath polygon
 */
void pcb_polygon_hole_create_from_attached(void)
{
	pcb_polyarea_t *original, *new_hole, *result;
	pcb_flag_t Flags;
	
	/* Create pcb_polyarea_ts from the original polygon
	 * and the new hole polygon */
	original = pcb_poly_from_poly((pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2);
	new_hole = pcb_poly_from_poly(&pcb_crosshair.AttachedPolygon);

	/* Subtract the hole from the original polygon shape */
	pcb_polyarea_boolean_free(original, new_hole, &result, PCB_PBO_SUB);

	/* Convert the resulting polygon(s) into a new set of nodes
	 * and place them on the page. Delete the original polygon.
	 */
	pcb_undo_save_serial();
	Flags = ((pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2)->Flags;
	pcb_poly_to_polygons_on_layer(PCB->Data, (pcb_layer_t *) pcb_crosshair.AttachedObject.Ptr1, result, Flags);
	pcb_remove_object(PCB_OBJ_POLY,
							 pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();

	/* reset state of attached line */
	memset(&pcb_crosshair.AttachedPolygon, 0, sizeof(pcb_poly_t));
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
}

/* find polygon holes in range, then call the callback function for
 * each hole. If the callback returns non-zero, stop
 * the search.
 */
int
pcb_poly_holes(pcb_poly_t * polygon, const pcb_box_t * range, int (*callback) (pcb_pline_t * contour, void *user_data), void *user_data)
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
	void *user_data;
	pcb_r_dir_t (*callback)(pcb_data_t *, pcb_layer_t *, pcb_poly_t *, int, void *, void *, void *user_data);
};

static pcb_r_dir_t subtract_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	if (!Polygon->Clipped)
		return 0;
	switch (type) {
	case PCB_OBJ_PSTK:
		SubtractPadstack(Data, (pcb_pstk_t *) ptr2, Layer, Polygon);
		Polygon->NoHolesValid = 0;
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_LINE:
		SubtractLine((pcb_line_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_ARC:
		SubtractArc((pcb_arc_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_POLY:
		if (ptr2 != Polygon) {
			SubtractPolyPoly((pcb_poly_t *) ptr2, Polygon);
			Polygon->NoHolesValid = 0;
			return PCB_R_DIR_FOUND_CONTINUE;
		}
		break;
	case PCB_OBJ_TEXT:
		SubtractText((pcb_text_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	return PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t add_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	switch (type) {
	case PCB_OBJ_PSTK:
		UnsubtractPadstack(Data, (pcb_pstk_t *) ptr2, Layer, Polygon);
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_LINE:
		UnsubtractLine((pcb_line_t *) ptr2, Layer, Polygon);
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_ARC:
		UnsubtractArc((pcb_arc_t *) ptr2, Layer, Polygon);
		return PCB_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_POLY:
		if (ptr2 != Polygon) {
			UnsubtractPolyPoly((pcb_poly_t *) ptr2, Polygon);
			return PCB_R_DIR_FOUND_CONTINUE;
		}
		break;
	case PCB_OBJ_TEXT:
		UnsubtractText((pcb_text_t *) ptr2, Layer, Polygon);
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	return PCB_R_DIR_NOT_FOUND;
}

int pcb_poly_sub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj)
{
	if (subtract_plow(Data, Layer, Polygon, type, NULL, obj, NULL) == PCB_R_DIR_FOUND_CONTINUE)
		return 0;
	return -1;
}

int pcb_poly_unsub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj)
{
	if (add_plow(Data, Layer, Polygon, type, NULL, obj, NULL) == PCB_R_DIR_FOUND_CONTINUE)
		return 0;
	return -1;
}


static pcb_r_dir_t plow_callback(const pcb_box_t * b, void *cl)
{
	struct plow_info *plow = (struct plow_info *) cl;
	pcb_poly_t *polygon = (pcb_poly_t *) b;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon))
		return plow->callback(plow->data, plow->layer, polygon, plow->type, plow->ptr1, plow->ptr2, plow->user_data);
	return PCB_R_DIR_NOT_FOUND;
}

/* poly plows while clipping inhibit is on: mark any potentail polygon dirty */
static pcb_r_dir_t poly_plows_inhibited_cb(pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data)
{
	poly->clip_dirty = 1;
	return PCB_R_DIR_FOUND_CONTINUE;
}


int pcb_poly_plows(pcb_data_t *Data, int type, void *ptr1, void *ptr2,
	pcb_r_dir_t (*call_back) (pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data),
	void *user_data)
{
	pcb_box_t sb = ((pcb_any_obj_t *) ptr2)->BoundingBox;
	int r = 0, seen;
	struct plow_info info;

	if (Data->clip_inhibit > 0)
		call_back = poly_plows_inhibited_cb;

	info.type = type;
	info.ptr1 = ptr1;
	info.ptr2 = ptr2;
	info.data = Data;
	info.user_data = user_data;
	info.callback = call_back;
	switch (type) {
	case PCB_OBJ_PSTK:
		if (Data->parent_type != PCB_PARENT_BOARD)
			return 0;
		if (ptr1 == NULL) { /* no layer specified: run on all layers */
			LAYER_LOOP(Data, pcb_max_layer);
			{
				if (!(pcb_layer_flags_(layer) & PCB_LYT_COPPER))
					continue;
				r += pcb_poly_plows(Data, type, layer, ptr2, call_back, NULL);
			}
			PCB_END_LOOP;
			return r;
		}

		/* run on the specified layer (ptr1), if there's any need for clearing */
	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
		if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (pcb_pstk_t *)ptr2))
			return 0;
		goto doit;
	case PCB_OBJ_POLY:
		if (!PCB_POLY_HAS_CLEARANCE((pcb_poly_t *) ptr2))
			return 0;
		goto doit;

	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
		/* the cast works equally well for lines and arcs */
		if (!PCB_NONPOLY_HAS_CLEARANCE((pcb_line_t *) ptr2))
			return 0;
		goto doit;

	case PCB_OBJ_TEXT:
		/* text has no clearance property */
		if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (pcb_line_t *) ptr2))
			return 0;
		/* non-copper (e.g. silk, outline) doesn't plow */
TODO(": use pcb_layer_flags_ here - but what PCB?")
		if (!(pcb_layer_flags(PCB, pcb_layer_id(Data, (pcb_layer_t *) ptr1) & PCB_LYT_COPPER)))
			return 0;
		doit:;
		PCB_COPPER_GROUP_LOOP(Data, pcb_layer_get_group(PCB, pcb_layer_id(Data, ((pcb_layer_t *) ptr1))));
		{
			info.layer = layer;
			pcb_r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
			r += seen;
		}
		PCB_END_LOOP;
		break;
	}
	return r;
}

void pcb_poly_restore_to_poly(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	pcb_data_t *dt = Data;
	while((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC))
		dt = dt->parent.subc->parent.data;
	if ((dt == NULL) || (dt->parent_type != PCB_PARENT_BOARD)) /* clear/restore only on boards */
		return;
	if (type == PCB_OBJ_POLY)
		pcb_poly_init_clip(dt, (pcb_layer_t *) ptr1, (pcb_poly_t *) ptr2);
	pcb_poly_plows(dt, type, ptr1, ptr2, add_plow, NULL);
}

void pcb_poly_clear_from_poly(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	pcb_data_t *dt = Data;
	while((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC))
		dt = dt->parent.subc->parent.data;
	if ((dt == NULL) || (dt->parent_type != PCB_PARENT_BOARD)) /* clear/restore only on boards */
		return;
	if (type == PCB_OBJ_POLY)
		pcb_poly_init_clip(dt, (pcb_layer_t *) ptr1, (pcb_poly_t *) ptr2);
	pcb_poly_plows(dt, type, ptr1, ptr2, subtract_plow, NULL);
}

pcb_bool pcb_poly_isects_poly(pcb_polyarea_t * a, pcb_poly_t *p, pcb_bool fr)
{
	pcb_polyarea_t *x;
	pcb_bool ans;
	ans = pcb_polyarea_touching(a, p->Clipped);
	/* argument may be register, so we must copy it */
	x = a;
	if (fr)
		pcb_polyarea_free(&x);
	return ans;
}


pcb_bool pcb_poly_is_point_in_p(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t r, pcb_poly_t *p)
{
	pcb_polyarea_t *c;
	pcb_vector_t v;
	v[0] = X;
	v[1] = Y;
	if (pcb_polyarea_contour_inside(p->Clipped, v))
		return pcb_true;

	if (PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, p)) {
		pcb_poly_t tmp = *p;

		/* Check all clipped-away regions that are now drawn because of full-poly */
		for (tmp.Clipped = p->Clipped->f; tmp.Clipped != p->Clipped; tmp.Clipped = tmp.Clipped->f)
			if (pcb_polyarea_contour_inside(tmp.Clipped, v))
				return pcb_true;
	}

	if (r < 1)
		return pcb_false;
	if (!(c = pcb_poly_from_circle(X, Y, r)))
		return pcb_false;
	return pcb_poly_isects_poly(c, p, pcb_true);
}


pcb_bool pcb_poly_is_point_in_p_ignore_holes(pcb_coord_t X, pcb_coord_t Y, pcb_poly_t *p)
{
	pcb_vector_t v;
	v[0] = X;
	v[1] = Y;
	return pcb_poly_contour_inside(p->Clipped->contours, v);
}

pcb_bool pcb_poly_is_rect_in_p(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_poly_t *p)
{
	pcb_polyarea_t *s;
	if (!(s = pcb_poly_from_rect(min(X1, X2), max(X1, X2), min(Y1, Y2), max(Y1, Y2))))
		return pcb_false;
	return pcb_poly_isects_poly(s, p, pcb_true);
}

TODO(": this should be in polygon1.c")
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
		pcb_polyarea_free(&pa);
		return;
	}
	else {
		pcb_polyarea_t *poly2, *left, *right;

		/* make a rectangle of the left region slicing through the middle of the first hole */
		poly2 = pcb_poly_from_rect(p->xmin, (p->next->xmin + p->next->xmax) / 2, p->ymin, p->ymax);
		pcb_polyarea_and_subtract_free(pa, poly2, &left, &right);
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

void pcb_poly_no_holes_dicer(pcb_poly_t *p, const pcb_box_t * clip, void (*emit) (pcb_pline_t *, void *), void *user_data)
{
	pcb_polyarea_t *main_contour, *cur, *next;

	main_contour = pcb_polyarea_create();
	/* copy the main poly only */
	pcb_polyarea_copy1(main_contour, p->Clipped);
	/* clip to the bounding box */
	if (clip) {
		pcb_polyarea_t *cbox = pcb_poly_from_rect(clip->X1, clip->X2, clip->Y1, clip->Y2);
		pcb_polyarea_boolean_free(main_contour, cbox, &main_contour, PCB_PBO_ISECT);
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
pcb_bool pcb_poly_morph(pcb_layer_t *layer, pcb_poly_t *poly)
{
	pcb_polyarea_t *p, *start;
	pcb_bool many = pcb_false;
	pcb_flag_t flags;

	if (!poly->Clipped || PCB_FLAG_TEST(PCB_FLAG_LOCK, poly))
		return pcb_false;
	if (poly->Clipped->f == poly->Clipped)
		return pcb_false;
	pcb_poly_invalidate_erase(poly);
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
		pcb_poly_t *newone;

		if (p->contours->area > conf_core.design.poly_isle_area) {
			newone = pcb_poly_new(layer, poly->Clearance, flags);
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
			pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, newone, newone);
			newone->Clipped = p;
			p = p->f;									/* go to next pline */
			newone->Clipped->b = newone->Clipped->f = newone->Clipped;	/* unlink from others */
			pcb_r_insert_entry(layer->polygon_tree, (pcb_box_t *) newone);
			pcb_poly_invalidate_draw(layer, newone);
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
	pcb_undo_inc_serial();
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

void debug_polygon(pcb_poly_t * p)
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
void pcb_poly_to_polygons_on_layer(pcb_data_t * Destination, pcb_layer_t * Layer, pcb_polyarea_t * Input, pcb_flag_t Flags)
{
	pcb_poly_t *Polygon;
	pcb_polyarea_t *pa;
	pcb_pline_t *pline;
	pcb_vnode_t *node;
	pcb_bool outer;

	if (Input == NULL)
		return;

	pa = Input;
	do {
		Polygon = pcb_poly_new(Layer, 2 * conf_core.design.clearance, Flags);

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

		pcb_poly_init_clip(Destination, Layer, Polygon);
		pcb_poly_bbox(Polygon);
		if (!Layer->polygon_tree)
			Layer->polygon_tree = pcb_r_create_tree();
		pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);

		pcb_poly_invalidate_draw(Layer, Polygon);
		/* add to undo list */
		pcb_undo_add_obj_to_create(PCB_OBJ_POLY, Layer, Polygon, Polygon);
	}
	while ((pa = pa->f) != Input);

	pcb_board_set_changed_flag(pcb_true);
}

pcb_bool pcb_pline_is_rectangle(pcb_pline_t *pl)
{
	int n;
	pcb_coord_t x[4], y[4];
	pcb_vnode_t *v;

	v = pl->head.next;
	n = 0;
	do {
		x[n] = v->point[0];
		y[n] = v->point[1];
		n++;
		v = v->next;
	} while((n < 4) && (v != pl->head.next));
	
	if (n != 4)
		return pcb_false;

	if (sqr(x[0] - x[2]) + sqr(y[0] - y[2]) == sqr(x[1] - x[3]) + sqr(y[1] - y[3]))
		return pcb_true;

	return pcb_false;
}

#include "polygon_offs.c"
