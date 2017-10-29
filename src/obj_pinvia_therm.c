/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* negative thermal finger polygons */

#include "config.h"

#include <stdlib.h>
#include <math.h>

#include "board.h"
#include "polygon.h"
#include "obj_pinvia_therm.h"

#warning cleanup TODO: remove this, pass on pcb as arg instead
static pcb_board_t *pcb;

struct cent {
	pcb_coord_t x, y;
	pcb_coord_t s, c;
	char style;
	pcb_polyarea_t *p;
};

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

static pcb_polyarea_t *square_therm(pcb_pin_t *pin, pcb_cardinal_t style)
{
	pcb_polyarea_t *p, *p2;
	pcb_pline_t *c;
	pcb_vector_t v;
	pcb_coord_t d, in, out;

	switch (style) {
	case 1:
		d = pcb->ThermScale * pin->Clearance * M_SQRT1_2;
		out = (pin->Thickness + pin->Clearance) / 2;
		in = pin->Thickness / 2;
		/* top (actually bottom since +y is down) */
		v[0] = pin->X - in + d;
		v[1] = pin->Y + in;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[0] = pin->X + in - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X + out - d;
		v[1] = pin->Y + out;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X - out + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		p = pcb_poly_from_contour(c);
		/* right */
		v[0] = pin->X + in;
		v[1] = pin->Y + in - d;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[1] = pin->Y - in + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X + out;
		v[1] = pin->Y - out + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[1] = pin->Y + out - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		p2 = pcb_poly_from_contour(c);
		p->f = p2;
		p2->b = p;
		/* left */
		v[0] = pin->X - in;
		v[1] = pin->Y - in + d;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[1] = pin->Y + in - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X - out;
		v[1] = pin->Y + out - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[1] = pin->Y - out + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		p2 = pcb_poly_from_contour(c);
		p->f->f = p2;
		p2->b = p->f;
		/* bottom (actually top since +y is down) */
		v[0] = pin->X + in - d;
		v[1] = pin->Y - in;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[0] = pin->X - in + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X - out + d;
		v[1] = pin->Y - out;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X + out - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		p2 = pcb_poly_from_contour(c);
		p->f->f->f = p2;
		p2->f = p;
		p2->b = p->f->f;
		p->b = p2;
		return p;
	case 4:
		{
			pcb_line_t l;
			l.Flags = pcb_no_flags();
			d = pin->Thickness / 2 - pcb->ThermScale * pin->Clearance;
			out = pin->Thickness / 2 + pin->Clearance / 4;
			in = pin->Clearance / 2;
			/* top */
			l.Point1.X = pin->X - d;
			l.Point2.Y = l.Point1.Y = pin->Y + out;
			l.Point2.X = pin->X + d;
			p = pcb_poly_from_line(&l, in);
			/* right */
			l.Point1.X = l.Point2.X = pin->X + out;
			l.Point1.Y = pin->Y - d;
			l.Point2.Y = pin->Y + d;
			p2 = pcb_poly_from_line(&l, in);
			p->f = p2;
			p2->b = p;
			/* bottom */
			l.Point1.X = pin->X - d;
			l.Point2.Y = l.Point1.Y = pin->Y - out;
			l.Point2.X = pin->X + d;
			p2 = pcb_poly_from_line(&l, in);
			p->f->f = p2;
			p2->b = p->f;
			/* left */
			l.Point1.X = l.Point2.X = pin->X - out;
			l.Point1.Y = pin->Y - d;
			l.Point2.Y = pin->Y + d;
			p2 = pcb_poly_from_line(&l, in);
			p->f->f->f = p2;
			p2->b = p->f->f;
			p->b = p2;
			p2->f = p;
			return p;
		}
	default:											/* style 2 and 5 */
		d = 0.5 * pcb->ThermScale * pin->Clearance;
		if (style == 5)
			d += d;
		out = (pin->Thickness + pin->Clearance) / 2;
		in = pin->Thickness / 2;
		/* topright */
		v[0] = pin->X + in;
		v[1] = pin->Y + in;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[1] = pin->Y + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 2) {
			v[0] = pin->X + out;
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		}
		else
			pcb_poly_frac_circle(c, v[0] + pin->Clearance / 4, v[1], v, 2);
		v[1] = pin->Y + in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		/* pivot 1/4 circle to next point */
		pcb_poly_frac_circle(c, pin->X + in, pin->Y + in, v, 4);
		v[0] = pin->X + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 2) {
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
			v[1] = pin->Y + in;
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		}
		else
			pcb_poly_frac_circle(c, v[0], v[1] - pin->Clearance / 4, v, 2);
		p = pcb_poly_from_contour(c);
		/* bottom right */
		v[0] = pin->X + in;
		v[1] = pin->Y - d;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[1] = pin->Y - in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 2) {
			v[1] = pin->Y - out;
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		}
		else
			pcb_poly_frac_circle(c, v[0], v[1] - pin->Clearance / 4, v, 2);
		v[0] = pin->X + in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		/* pivot 1/4 circle to next point */
		pcb_poly_frac_circle(c, pin->X + in, pin->Y - in, v, 4);
		v[1] = pin->Y - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 5)
			pcb_poly_frac_circle(c, v[0] - pin->Clearance / 4, v[1], v, 2);
		p2 = pcb_poly_from_contour(c);
		p->f = p2;
		p2->b = p;
		/* bottom left */
		v[0] = pin->X - d;
		v[1] = pin->Y - in;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[0] = pin->X - in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[1] = pin->Y - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 2) {
			v[0] = pin->X - out;
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		}
		else
			pcb_poly_frac_circle(c, v[0] - pin->Clearance / 4, v[1], v, 2);
		v[1] = pin->Y - in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		/* pivot 1/4 circle to next point */
		pcb_poly_frac_circle(c, pin->X - in, pin->Y - in, v, 4);
		v[0] = pin->X - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 5)
			pcb_poly_frac_circle(c, v[0], v[1] + pin->Clearance / 4, v, 2);
		p2 = pcb_poly_from_contour(c);
		p->f->f = p2;
		p2->b = p->f;
		/* top left */
		v[0] = pin->X - d;
		v[1] = pin->Y + out;
		if ((c = pcb_poly_contour_new(v)) == NULL)
			return NULL;
		v[0] = pin->X - in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		/* pivot 1/4 circle to next point (x-out, y+in) */
		pcb_poly_frac_circle(c, pin->X - in, pin->Y + in, v, 4);
		v[1] = pin->Y + d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 2) {
			v[0] = pin->X - in;
			pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		}
		else
			pcb_poly_frac_circle(c, v[0] + pin->Clearance / 4, v[1], v, 2);
		v[1] = pin->Y + in;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		v[0] = pin->X - d;
		pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
		if (style == 5)
			pcb_poly_frac_circle(c, v[0], v[1] + pin->Clearance / 4, v, 2);
		p2 = pcb_poly_from_contour(c);
		p->f->f->f = p2;
		p2->f = p;
		p2->b = p->f->f;
		p->b = p2;
		return p;
	}
}

static pcb_polyarea_t *oct_therm(pcb_pin_t *pin, pcb_cardinal_t style)
{
	pcb_polyarea_t *p, *p2, *m;
	pcb_coord_t t = 0.5 * pcb->ThermScale * pin->Clearance;
	pcb_coord_t w = pin->Thickness + pin->Clearance;
	int shape = PCB_FLAG_SQUARE_GET(pin);

	/* temporary work around corner case on square+octagon shape with large clearance */
	if (shape <= 1)
		shape = 17;

	p = pcb_poly_from_octagon(pin->X, pin->Y, w, shape);
	p2 = pcb_poly_from_octagon(pin->X, pin->Y, pin->Thickness, shape);
	/* make full clearance ring */
	pcb_polyarea_boolean_free(p, p2, &m, PCB_PBO_SUB);
	switch (style) {
	default:
	case 1:
		p = pcb_pa_diag_line(pin->X, pin->Y, w, t, pcb_true);
		pcb_polyarea_boolean_free(m, p, &p2, PCB_PBO_SUB);
		p = pcb_pa_diag_line(pin->X, pin->Y, w, t, pcb_false);
		pcb_polyarea_boolean_free(p2, p, &m, PCB_PBO_SUB);
		return m;
	case 2:
		p = pcb_poly_from_rect(pin->X - t, pin->X + t, pin->Y - w, pin->Y + w);
		pcb_polyarea_boolean_free(m, p, &p2, PCB_PBO_SUB);
		p = pcb_poly_from_rect(pin->X - w, pin->X + w, pin->Y - t, pin->Y + t);
		pcb_polyarea_boolean_free(p2, p, &m, PCB_PBO_SUB);
		return m;
		/* fix me add thermal style 4 */
	case 5:
		{
			pcb_coord_t t = pin->Thickness / 2;
			pcb_polyarea_t *q;
			/* cheat by using the square therm's rounded parts */
			p = square_therm(pin, style);
			q = pcb_poly_from_rect(pin->X - t, pin->X + t, pin->Y - t, pin->Y + t);
			pcb_polyarea_boolean_free(p, q, &p2, PCB_PBO_UNITE);
			pcb_polyarea_boolean_free(m, p2, &p, PCB_PBO_ISECT);
			return p;
		}
	}
}

/* ThermPoly returns a pcb_polyarea_t having all of the clearance that when
 * subtracted from the plane create the desired thermal fingers.
 * Usually this is 4 disjoint regions.
 *
 */
pcb_polyarea_t *ThermPoly_(pcb_board_t *p, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t thickness, pcb_coord_t clearance, pcb_cardinal_t style)
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
		pa = pcb_poly_from_arc(&a, a.Clearance);
		if (!pa)
			return NULL;
		a.StartAngle += 90;
		arc = pcb_poly_from_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa->f = arc;
		arc->b = pa;
		a.StartAngle += 90;
		arc = pcb_poly_from_arc(&a, a.Clearance);
		if (!arc)
			return NULL;
		pa->f->f = arc;
		arc->b = pa->f;
		a.StartAngle += 90;
		arc = pcb_poly_from_arc(&a, a.Clearance);
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

pcb_polyarea_t *ThermPoly(pcb_board_t *p, pcb_pin_t *pin, pcb_cardinal_t laynum)
{
	pcb_cardinal_t style = PCB_FLAG_THERM_GET(laynum, pin);
	if (style == 3)
		return NULL;								/* solid connection no clearance */
	pcb = p;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin))
		return square_therm(pin, style);
	if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin))
		return oct_therm(pin, style);

	return ThermPoly_(p, pin->X, pin->Y, pin->Thickness, pin->Clearance, style);
}
