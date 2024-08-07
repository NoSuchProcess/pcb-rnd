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

#ifndef SVGPATH_H
#define SVGPATH_H

/* Rendering callback; uctx is user context passed to the rendering call */
typedef struct svgpath_cfg_s {

	/*** config ***/

	/* max length of a line segment used in curve/arc approximation; if 0
	   or negative fall back to hardwired default */
	double curve_approx_seglen;


	/*** mandatory - may be NULL only if no rendering is to be done (syntax check) ***/

	/* draw a straight line segment betwen x1;y1 and x2;y2 */
	void (*line)(void *uctx, double x1, double y1, double x2, double y2);


	/*** optional - when NULL, emulated using lines instead ***/

	/* draw a cubic bezier between sx;sy and ex;ey using control points cx*;cy* */
	void (*bezier_cubic)(void *uctx, double sx, double sy, double cx1, double cy1, double cx2, double cy2, double ex, double ey);

	/* draw a quadratic bezier between sx;sy and ex;ey using control point cx;cy */
	void (*bezier_quadratic)(void *uctx, double sx, double sy, double cx, double cy, double ex, double ey);

	/* draw a circular arc; center is cx;cy with radius rx;ry, from
	   start angle sa (in radian) to delta angle da (in radian). If NULL,
	   earc is used instead. */
	void (*carc)(void *uctx, double cx, double cy, double r, double sa, double da);

	/* draw a rotated elliptical arc; center is cx;cy with radius rx;ry, from
	   start angle sa (in radian) to delta angle da (in radian). Radii and angles
	   are interpreted in an ellipse-local coord system that is then rotated
	   by rot radians. If NULL, line approximated. */
	void (*earc)(void *uctx, double cx, double cy, double rx, double ry, double sa, double da, double rot);

	/*** optional - silently skipped when NULL ***/
	/* report an error in path; offs is the byte offset within the path string */
	void (*error)(void *uctx, const char *errmsg, long offs);

} svgpath_cfg_t;

/* Render a path using callbacks from cb. Returns 0 on success. */
int svgpath_render(const svgpath_cfg_t *cfg, void *uctx, const char *path);

/* Curve approximation with lines; apl2 is curve_approx_seglen^2 */
void svgpath_approx_bezier_cubic(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx1, double cy1, double cx2, double cy2, double ex, double ey, double apl2);
void svgpath_approx_bezier_quadratic(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx, double cy, double ex, double ey, double apl2);
void svgpath_approx_earc(const svgpath_cfg_t *cfg, void *uctx, double sx, double sy, double cx, double cy, double rx, double ry, double sa, double da, double rot, double ex, double ey, double apl2);

#endif
