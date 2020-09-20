/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf stroker
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* Dummy stroker that approximates curves with line segments */

#include "ttf_load.h"
#include "str_approx.h"
#include <librnd/core/error.h>

char *str_approx_comment = "!!";

static double sqr(double a)
{
	return a*a;
}

static double cub(double a)
{
	return a*a*a;
}

int stroke_approx_conic_to(const FT_Vector *control, const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *s = (pcb_ttf_stroke_t *)s_;
	double t;
	double nodes = 10, td = 1.0 / nodes;
	FT_Vector v;

	if (str_approx_comment != NULL) rnd_trace("%s conic to {\n", str_approx_comment);
	for(t = 0.0; t <= 1.0; t += td) {
		v.x = sqr(1.0-t) * s->x + 2.0*t*(1.0-t)*(double)control->x + t*t*(double)to->x;
		v.y = sqr(1.0-t) * s->y + 2.0*t*(1.0-t)*(double)control->y + t*t*(double)to->y;
		s->funcs.line_to(&v, s);
	}
	if (str_approx_comment != NULL) rnd_trace("%s }\n", str_approx_comment);

	s->funcs.line_to(to, s);
	return 0;
}

int stroke_approx_cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *s = (pcb_ttf_stroke_t *)s_;
	double t;
	double nodes = 10, td = 1.0 / nodes;
	FT_Vector v;

	if (str_approx_comment != NULL) rnd_trace("%s cubic to {\n", str_approx_comment);
	for(t = 0.0; t <= 1.0; t += td) {
		v.x = cub(1.0-t)*s->x + 3.0*t*sqr(1.0-t)*(double)control1->x + 3.0*sqr(t)*(1.0-t)*(double)control2->x + cub(t)*(double)to->x;
		v.y = cub(1.0-t)*s->y + 3.0*t*sqr(1.0-t)*(double)control1->y + 3.0*sqr(t)*(1.0-t)*(double)control2->y + cub(t)*(double)to->y;
		s->funcs.line_to(&v, s);
	}
	if (str_approx_comment != NULL) rnd_trace("%s }\n", str_approx_comment);
	s->funcs.line_to(to, s);
	return 0;
}
