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
#include "obj_pinvia_therm.h"

static pcb_polyarea_t *pcb_pa_diag_line(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t l, pcb_coord_t w, pcb_bool rt)
{
	pcb_pline_t *c;
	pcb_vector_t v;
	pcb_coord_t x1, x2, y1, y2;

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
	if ((c = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	v[0] = X - x2;
	v[1] = Y - y1;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	v[0] = X - x1;
	v[1] = Y - y2;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	v[0] = X + x2;
	v[1] = Y + y1;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	return pcb_poly_from_contour(c);
}

/* ThermPoly returns a pcb_polyarea_t having all of the clearance that when
 * subtracted from the plane create the desired thermal fingers.
 * Usually this is 4 disjoint regions.
 *
 */
pcb_polyarea_t *ThermPoly_(pcb_board_t *pcb, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t thickness, pcb_coord_t clearance, pcb_cardinal_t style)
{
	pcb_arc_t a;
	pcb_polyarea_t *pa, *arc;

	/* must be circular */
	switch (style) {
	case 1:
	case 2:
		{
			pcb_polyarea_t *m;
			pcb_coord_t t = (thickness + clearance) / 2;
			pcb_coord_t w = 0.5 * pcb->ThermScale * clearance;
			pa = pcb_poly_from_circle(cx, cy, t);
			arc = pcb_poly_from_circle(cx, cy, thickness / 2);
			/* create a thin ring */
			pcb_polyarea_boolean_free(pa, arc, &m, PCB_PBO_SUB);
			/* fix me needs error checking */
			if (style == 2) {
				/* t is the theoretically required length, but we use twice that
				 * to avoid discretisation errors in our circle approximation.
				 */
				pa = pcb_poly_from_rect(cx - t * 2, cx + t * 2, cy - w, cy + w);
				pcb_polyarea_boolean_free(m, pa, &arc, PCB_PBO_SUB);
				pa = pcb_poly_from_rect(cx - w, cx + w, cy - t * 2, cy + t * 2);
			}
			else {
				/* t is the theoretically required length, but we use twice that
				 * to avoid discretisation errors in our circle approximation.
				 */
				pa = pcb_pa_diag_line(cx, cy, t * 2, w, pcb_true);
				pcb_polyarea_boolean_free(m, pa, &arc, PCB_PBO_SUB);
				pa = pcb_pa_diag_line(cx, cy, t * 2, w, pcb_false);
			}
			pcb_polyarea_boolean_free(arc, pa, &m, PCB_PBO_SUB);
			return m;
		}

	default:
		a.X = cx;
		a.Y = cy;
		a.Height = a.Width = thickness / 2 + clearance / 4;
		a.Thickness = 1;
		a.Clearance = clearance / 2;
		a.Flags = pcb_no_flags();
		a.Delta = 90 - (a.Clearance * (1. + 2. * pcb->ThermScale) * 180) / (M_PI * a.Width);
		a.StartAngle = 90 - a.Delta / 2 + (style == 4 ? 0 : 45);
		pa = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!pa)
			return NULL;
		a.StartAngle += 90;
		arc = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa->f = arc;
		arc->b = pa;
		a.StartAngle += 90;
		arc = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa->f->f = arc;
		arc->b = pa->f;
		a.StartAngle += 90;
		arc = pcb_poly_from_pcb_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa->b = arc;
		pa->f->f->f = arc;
		arc->b = pa->f->f;
		arc->f = pa;
		pa->b = arc;
		return pa;
	}
}
