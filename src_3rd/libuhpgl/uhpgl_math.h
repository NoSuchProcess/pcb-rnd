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

/* Math helper for internal use - not intended to be included by the
   library user */

#ifndef LIBUHPGL_MATH_H
#define LIBUHPGL_MATH_H

#include <math.h>

#define CONST_PI       3.14159265358979323846
#define RAD2DEG(r)     ((r) * 180.0 / CONST_PI)
#define DEG2RAD(d)     ((d) * CONST_PI / 180.0)
#define DDIST(dx, dy)  sqrt((double)(dx)*(double)(dx) + (double)(dy)*(double)(dy))

/* Implementation idea borrowed from an old gcc */
static double ROUND(double x)
{
	double t;

	if (x >= 0.0) {
		t = ceil(x);
		if (t - x > 0.5)
			t -= 1.0;
		return t;
	}

	t = ceil(-x);
	if ((t + x) > 0.5)
		t -= 1.0;
	return -t;
}

#endif
