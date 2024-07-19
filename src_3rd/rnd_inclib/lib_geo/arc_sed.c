/*
    svg path parser for Ringdove
    Copyright (C) 2024 Tibor 'Igor2' Palinkas

    (Supported by NLnet NGI0 Entrust Fund in 2024)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <math.h>

#define DEG2RAD (M_PI/180)

#define SWAP(typ, a, b) \
do { \
	typ __tmp__ = a; \
	a = b; \
	b = __tmp__; \
} while(0)

#define GEN_OUTPUT() \
do { \
	*cx_out = cx; \
	*cy_out = cy; \
	*r_out = r; \
	*srad = atan2(sy - cy, sx - cx); \
	*erad = atan2(ey - cy, ex - cx); \
} while(0)

/* Optimized solver for 90 degree: consider the right triangle from
   cx;cy, ex;ey, sx;sy. Knowing the chord length this yields r. Consider half
   of this triangle, cutting from middle of the chord, also a right
   triangle: chord side length known, hypotenuse is r -> distance from mx;my
   to cx;cy. */
static int arc_sed_90(double sx, double sy, double ex, double ey, double deg, double *cx_out, double *cy_out, double *r_out, double *srad, double *erad)
{
	double r, cx, cy;
	double mx, my, mr;
	double chlen, chvx, chvy, chnx, chny, chl2; /* chord */

	mx = (sx + ex) / 2.0; my = (sy + ey) / 2.0;
	chvx = ex - sx; chvy = ey - sy;
	chlen = sqrt(chvx*chvx + chvy*chvy); chl2 = chlen/2.0;
	chvx /= chlen; chvy /= chlen;
	chnx = -chvy; chny = chvx;

	r  = sqrt(chlen*chl2);  /* sqrt(chlen*chlen/2) */
	mr = sqrt(r*r - chl2*chl2);
	cx = mx + chnx * mr; cy = my + chny * mr;

	GEN_OUTPUT();

	return 0;
}

/* Optimized solver for 180 deg: chord is diameter, chord middle is center */
static int arc_sed_180(double sx, double sy, double ex, double ey, double deg, double *cx_out, double *cy_out, double *r_out, double *srad, double *erad)
{
	double r, cx, cy;
	double chlen, chvx, chvy; /* chord */

	cx = (sx + ex) / 2.0; cy = (sy + ey) / 2.0;
	chvx = ex - sx; chvy = ey - sy;
	chlen = sqrt(chvx*chvx + chvy*chvy); r = chlen/2.0;

	GEN_OUTPUT();

	return 0;
}

static int arc_sed_270(double sx, double sy, double ex, double ey, double deg, double *cx_out, double *cy_out, double *r_out, double *srad, double *erad)
{
	return arc_sed_90(ex, ey, sx, sy, 90, cx_out, cy_out, r_out, srad, erad);
}

/* Generic for angle smaller than 180; draw chord between start and end, the
   middle of the cord is mx;my. Consider the right triangle mx;my
   cx;cy and ex;ey; the central angle is deg/2, the side at chord is chl2.
   This yields radius and the side from mx;my to cx;cy. */
static int arc_sed_small(double sx, double sy, double ex, double ey, double deg, double *cx_out, double *cy_out, double *r_out, double *srad, double *erad)
{
	double cx, cy, r;
	double mx, my, mr;
	double chlen, chvx, chvy, chnx, chny, chl2; /* chord */
	double delta = deg * DEG2RAD;

	/* chord middle point, length and normal vector */
	mx = (sx + ex) / 2.0; my = (sy + ey) / 2.0;
	chvx = ex - sx; chvy = ey - sy;
	chlen = sqrt(chvx*chvx + chvy*chvy); chl2 = chlen/2.0;
	chvx /= chlen; chvy /= chlen;
	chnx = -chvy; chny = chvx;

	r = chl2 / sin(delta/2.0);
	mr = sqrt(r*r - chl2*chl2);
	cx = mx + chnx * mr; cy = my + chny * mr;

	GEN_OUTPUT();
	return 0;
}

static int arc_sed_large(double sx, double sy, double ex, double ey, double deg, double *cx_out, double *cy_out, double *r_out, double *srad, double *erad)
{
	return arc_sed_small(ex, ey, sx, sy, (360.0-deg), cx_out, cy_out, r_out, srad, erad);
}

int arc_start_end_delta(double sx, double sy, double ex, double ey, double deg, double *cx, double *cy, double *r, double *srad, double *erad)
{
	int swapped = 0, res;

	/* normalize input */
	if (deg < 0) {
		SWAP(double, sx, ex);
		SWAP(double, sy, ey);
		deg = -deg;
		swapped = 1;
	}
	if (deg > 360)
		deg = fmod(deg, 360);


	/* dispatch different cases */
	if (deg == 360) return -1; /* can not solve for radius or center */
	else if (deg == 90) res = arc_sed_90(sx, sy, ex, ey, deg, cx, cy, r, srad, erad);
	else if (deg == 180) res = arc_sed_180(sx, sy, ex, ey, deg, cx, cy, r, srad, erad);
	else if (deg == 270) { res = arc_sed_270(sx, sy, ex, ey, deg, cx, cy, r, srad, erad); swapped = !swapped; }
	else if (deg < 180) res = arc_sed_small(sx, sy, ex, ey, deg, cx, cy, r, srad, erad);
	else if (deg > 180) { res = arc_sed_large(sx, sy, ex, ey, deg, cx, cy, r, srad, erad); swapped = !swapped; }
	else return -2; /* can't get here */

	if (swapped)
		SWAP(double, *srad, *erad);

	return res;
}
