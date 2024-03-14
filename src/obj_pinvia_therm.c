/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *
 *  this file, thermal.c was written by and is
 *  (C) Copyright 2006, harry eaton
 *
 *  Adaptations for pcb-rnd:
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
 *
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
 *
 */

/* negative thermal finger polygons */

#include "config.h"

#include <stdlib.h>
#include <math.h>

#include "board.h"
#include "polygon.h"
#include <librnd/poly/polygon1_gen.h>
#include <librnd/core/error.h>
#include "obj_pinvia_therm.h"

static rnd_polyarea_t *pcb_pa_diag_line(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t l, rnd_coord_t w, rnd_bool rt)
{
	rnd_pline_t *c;
	rnd_vector_t v;
	rnd_coord_t x1, x2, y1, y2;

	if (rt) {
		x1 = (l - w) * M_SQRT1_2;
		x2 = (l + w) * M_SQRT1_2;
		y1 = x1;
		y2 = x2;
	}
	else {
		x2 = -(l - w) * M_SQRT1_2;
		x1 = -(l + w) * M_SQRT1_2;
		y1 = -x1;
		y2 = -x2;
	}

	v[0] = X + x1;
	v[1] = Y + y2;
	if ((c = rnd_poly_contour_new(v)) == NULL)
		return NULL;
	v[0] = X - x2;
	v[1] = Y - y1;
	rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	v[0] = X - x1;
	v[1] = Y - y2;
	rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	v[0] = X + x2;
	v[1] = Y + y1;
	rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	return rnd_poly_from_contour(c);
}

/* ThermPoly returns a rnd_polyarea_t having all of the clearance that when
 * subtracted from the plane create the desired thermal fingers.
 * Usually this is 4 disjoint regions.
 *
 */
rnd_polyarea_t *ThermPoly_(pcb_board_t *pcb, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t thickness, rnd_coord_t clearance, rnd_cardinal_t style)
{
	pcb_arc_t a;
	rnd_polyarea_t *pa, *pa1, *pa2, *arc;

	/* must be circular */
	switch (style) {
	case 1:
	case 2:
		{
			rnd_polyarea_t *m;
			rnd_coord_t t = (thickness + clearance) / 2;
			rnd_coord_t w = 0.5 * pcb->ThermScale * clearance;
			pa = rnd_poly_from_circle(cx, cy, t);
			arc = rnd_poly_from_circle(cx, cy, thickness / 2);


			if (w < 1000) {
				w = 1000;
				rnd_message(RND_MSG_ERROR, "Thermal finger too thin around %$mm;%$mm; expect BROKEN THERMAL! Check your \"thermal scale\" setting (should be around 0.5)\n", cx, cy);
			}

			/* create a thin ring */
			rnd_polyarea_boolean_free(pa, arc, &m, RND_PBO_SUB);
			/* fix me needs error checking */
			if (style == 2) {
				/* t is the theoretically required length, but we use twice that
				 * to avoid discretisation errors in our circle approximation.
				 */
				pa = rnd_poly_from_rect(cx - t * 2, cx + t * 2, cy - w, cy + w);
				rnd_polyarea_boolean_free(m, pa, &arc, RND_PBO_SUB);
				pa = rnd_poly_from_rect(cx - w, cx + w, cy - t * 2, cy + t * 2);
			}
			else {
				/* t is the theoretically required length, but we use twice that
				 * to avoid discretisation errors in our circle approximation.
				 */
				pa = pcb_pa_diag_line(cx, cy, t * 2, w, rnd_true);
				rnd_polyarea_boolean_free(m, pa, &arc, RND_PBO_SUB);
				pa = pcb_pa_diag_line(cx, cy, t * 2, w, rnd_false);
			}
			rnd_polyarea_boolean_free(arc, pa, &m, RND_PBO_SUB);
			return m;
		}

	default:
	{
		double therm_scale = pcb->ThermScale;

		if (therm_scale < 0.01) {
			therm_scale = 0.01;
			rnd_message(RND_MSG_ERROR, "Thermal finger too thin around %$mm;%$mm; expect BROKEN THERMAL! Check your \"thermal scale\" setting (should be around 0.5)\n", cx, cy);
		}

		a.X = cx;
		a.Y = cy;
		a.Height = a.Width = thickness / 2 + clearance / 4;
		a.Thickness = 1;
		a.Clearance = clearance / 2;
		a.Flags = pcb_no_flags();
		a.Delta = 90 - (a.Clearance * (1. + 2. * therm_scale) * 180) / (M_PI * a.Width);
		a.StartAngle = 90 - a.Delta / 2 + (style == 4 ? 0 : 45);

		/* Create union of 4 arcs, in two pairs: pairs of opposite arcs (within
		   pa1 and pa2) won't ever intersect so they can be simply linked together
		   using ->f and ->b fields of polyareas. But adjacent arcs may intersect
		   if clearance is large so the two pairs need to be combined using the
		   more expensive rnd_polyarea_boolean_free(). Test case: place a via in
		   a poly, use the standard round thermal, start increasing the clearance
		   value until the arcs get very close and merge. */
		pa1 = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!pa1)
			return NULL;
		a.StartAngle += 90;

		pa2 = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!pa2)
			return NULL;


		a.StartAngle += 90;
		arc = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa1->f = pa1->b = arc;
		arc->f = arc->b = pa1;

		a.StartAngle += 90;
		arc = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa2->f = pa2->b = arc;
		arc->f = arc->b = pa2;

		pa = NULL;
		rnd_polyarea_boolean_free(pa1, pa2, &pa, RND_PBO_UNITE);

		return pa;
	}
	}
}
