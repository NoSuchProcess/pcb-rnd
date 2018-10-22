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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Dummy stroker that approximates curves with line segments */

#include "ttf_load.h"
#include "str_approx.h"

char *str_approx_comment = "!!";

static double sqr(double a)
{
	return a*a;
}

int stroke_approx_conic_to(const FT_Vector *control, const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *s = (pcb_ttf_stroke_t *)s_;
	double t;
	double nodes = 10, td = 1.0 / nodes;
	FT_Vector v;

	if (str_approx_comment != NULL) printf("%s conic to {\n", str_approx_comment);
	for(t = 0.0; t <= 1.0; t += td) {
		v.x = sqr(1.0-t) * s->x + 2*t*(1.0-t)*control->x + t*t*to->x;
		v.y = sqr(1.0-t) * s->y + 2*t*(1.0-t)*control->y + t*t*to->y;
		s->funcs.line_to(&v, s);
	}
	if (str_approx_comment != NULL) printf("%s }\n", str_approx_comment);

	s->x = to->x;
	s->y = to->y;
	return 0;
}

int stroke_approx_cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *s = (pcb_ttf_stroke_t *)s_;
	if (str_approx_comment != NULL) printf("%s cubic to {\n", str_approx_comment);
	if (str_approx_comment != NULL) printf("%s }\n", str_approx_comment);
	return 0;
}
