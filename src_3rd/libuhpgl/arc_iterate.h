/*
    libuhpgl - the micro HP-GL library
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


    This library is part of pcb-rnd: http://repo.hu/projects/pcb-rnd
*/

/* Arc approximation loops for internal use - not intended to be
   included by the library user */

#ifndef LIBUHPGL_ARC_ITERATE_H
#define LIBUHPGL_ARC_ITERATE_H

/* Arc approximation loops */

typedef struct uhpgl_arc_it_s {
	const uhpgl_arc_t *arc;
	double step;   /* stepping angle */
	double ang;
	uhpgl_point_t pt;
	int remain;
} uhpgl_arc_it_t;

/* Do not call directly. Set up iterator for angle based steps. */
static uhpgl_point_t *uhpgl_arc_it_first_ang(uhpgl_arc_it_t *it, const uhpgl_arc_t *arc, double step)
{
	double minstep = 360.0 / (arc->r*CONST_PI*2.0);

	if (step < 0)
		step = -step;
	else if (step == 0)
		step = 5;

	if (step < minstep)
		step = minstep;

	if (arc->deltaa < 0)
		step = -step;

	it->arc = arc;
	it->step = step;
	it->remain = floor(arc->deltaa / step)+2;

	it->pt = arc->startp;
	it->ang = arc->starta;
	return &it->pt;
}

/* Do not call directly. Set up iterator for radius error based steps. */
static uhpgl_point_t *uhpgl_arc_it_first_err(uhpgl_arc_it_t *it, const uhpgl_arc_t *arc, double err)
{
	double step;

	if (err > arc->r / 4)
		return uhpgl_arc_it_first_ang(it, arc, 0);

	step = RAD2DEG(acos((arc->r - err) / arc->r) * 2);
	if (step < 0)
		step = -step;

	if (step < 0.1)
		step = 0.1;
	else if (step > 15)
		step = 15;

	return uhpgl_arc_it_first_ang(it, arc, step);
}

/* Start an iteration, return the first point */
static uhpgl_point_t *uhpgl_arc_it_first(uhpgl_ctx_t *ctx, uhpgl_arc_it_t *it, const uhpgl_arc_t *arc, double resolution)
{
	if (ctx->state.ct == 0)
		return uhpgl_arc_it_first_ang(it, arc, resolution);
	return uhpgl_arc_it_first_err(it, arc, resolution);
}

/* Return the next point or NULL if there are no more points */
static uhpgl_point_t *uhpgl_arc_it_next(uhpgl_arc_it_t *it)
{
	uhpgl_coord_t x, y;
	it->remain--;
	switch(it->remain) {
		case 0: /* beyond the endpoint */
			return NULL;
		case 1: /* at the endpoint */
			last_pt:;
			/* special case: endpoint reached in the previous iterationm remaining correction path is 0 long */
			if ((it->pt.x == it->arc->endp.x) && (it->pt.y == it->arc->endp.y))
				return NULL;
			it->pt = it->arc->endp;
			return &it->pt;
		default:
			for(;;) {
				it->ang += it->step;
				x = ROUND((double)it->arc->center.x + it->arc->r * cos(DEG2RAD(it->ang)));
				y = ROUND((double)it->arc->center.y + it->arc->r * sin(DEG2RAD(it->ang)));
				if ((it->pt.x != x) || (it->pt.y != y)) {
					it->pt.x = x;
					it->pt.y = y;
					return &it->pt;
				}
				it->remain--;
				if (it->remain == 1)
					goto last_pt;
			}
	}
	return NULL;
}

#endif
