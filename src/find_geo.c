/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* Generic geometric functions used by find.c lookup */

/*
 * Intersection of line <--> circle:
 * - calculate the signed distance from the line to the center,
 *   return false if abs(distance) > R
 * - get the distance from the line <--> distancevector intersection to
 *   (X1,Y1) in range [0,1], return pcb_true if 0 <= distance <= 1
 * - depending on (r > 1.0 or r < 0.0) check the distance of X2,Y2 or X1,Y1
 *   to X,Y
 *
 * Intersection of line <--> line:
 * - see the description of 'pcb_isc_line_line()'
 */

#include "macro.h"
#include "obj_arc_ui.h"
#include "obj_pstk_inlines.h"

#define EXPAND_BOUNDS(p) if (Bloat > 0) {\
       (p)->BoundingBox.X1 -= Bloat; \
       (p)->BoundingBox.X2 += Bloat; \
       (p)->BoundingBox.Y1 -= Bloat; \
       (p)->BoundingBox.Y2 += Bloat;}

/* reduce arc start angle and delta to 0..360 */
static void normalize_angles(pcb_angle_t * sa, pcb_angle_t * d)
{
	if (*d < 0) {
		*sa += *d;
		*d = -*d;
	}
	if (*d > 360)									/* full circle */
		*d = 360;
	*sa = pcb_normalize_angle(*sa);
}

static int radius_crosses_arc(double x, double y, pcb_arc_t *arc)
{
	double alpha = atan2(y - arc->Y, -x + arc->X) * PCB_RAD_TO_DEG;
	pcb_angle_t sa = arc->StartAngle, d = arc->Delta;

	normalize_angles(&sa, &d);
	if (alpha < 0)
		alpha += 360;
	if (sa <= alpha)
		return (sa + d) >= alpha;
	return (sa + d - 360) >= alpha;
}

static void get_arc_ends(pcb_coord_t * box, pcb_arc_t *arc)
{
	box[0] = arc->X - arc->Width * cos(PCB_M180 * arc->StartAngle);
	box[1] = arc->Y + arc->Height * sin(PCB_M180 * arc->StartAngle);
	box[2] = arc->X - arc->Width * cos(PCB_M180 * (arc->StartAngle + arc->Delta));
	box[3] = arc->Y + arc->Height * sin(PCB_M180 * (arc->StartAngle + arc->Delta));
}

/* ---------------------------------------------------------------------------
 * check if two arcs intersect
 * first we check for circle intersections,
 * then find the actual points of intersection
 * and test them to see if they are on arcs
 *
 * consider a, the distance from the center of arc 1
 * to the point perpendicular to the intersecting points.
 *
 *  a = (r1^2 - r2^2 + l^2)/(2l)
 *
 * the perpendicular distance to the point of intersection
 * is then
 *
 * d = sqrt(r1^2 - a^2)
 *
 * the points of intersection would then be
 *
 * x = X1 + a/l dx +- d/l dy
 * y = Y1 + a/l dy -+ d/l dx
 *
 * where dx = X2 - X1 and dy = Y2 - Y1
 *
 *
 */
static pcb_bool pcb_isc_arc_arc(pcb_arc_t *Arc1, pcb_arc_t *Arc2)
{
	double x, y, dx, dy, r1, r2, a, d, l, t, t1, t2, dl;
	pcb_coord_t pdx, pdy;
	pcb_coord_t box[8];

	t = 0.5 * Arc1->Thickness + Bloat;
	t2 = 0.5 * Arc2->Thickness;
	t1 = t2 + Bloat;

	/* too thin arc */
	if (t < 0 || t1 < 0)
		return pcb_false;

	/* try the end points first */
	get_arc_ends(&box[0], Arc1);
	get_arc_ends(&box[4], Arc2);
	if (pcb_is_point_on_arc(box[0], box[1], t, Arc2)
			|| pcb_is_point_on_arc(box[2], box[3], t, Arc2)
			|| pcb_is_point_on_arc(box[4], box[5], t, Arc1)
			|| pcb_is_point_on_arc(box[6], box[7], t, Arc1))
		return pcb_true;

	pdx = Arc2->X - Arc1->X;
	pdy = Arc2->Y - Arc1->Y;
	dl = pcb_distance(Arc1->X, Arc1->Y, Arc2->X, Arc2->Y);
	/* concentric arcs, simpler intersection conditions */
	if (dl < 0.5) {
		if ((Arc1->Width - t >= Arc2->Width - t2 && Arc1->Width - t <= Arc2->Width + t2)
				|| (Arc1->Width + t >= Arc2->Width - t2 && Arc1->Width + t <= Arc2->Width + t2)) {
			pcb_angle_t sa1 = Arc1->StartAngle, d1 = Arc1->Delta;
			pcb_angle_t sa2 = Arc2->StartAngle, d2 = Arc2->Delta;
			/* NB the endpoints have already been checked,
			   so we just compare the angles */

			normalize_angles(&sa1, &d1);
			normalize_angles(&sa2, &d2);
			/* sa1 == sa2 was caught when checking endpoints */
			if (sa1 > sa2)
				if (sa1 < sa2 + d2 || sa1 + d1 - 360 > sa2)
					return pcb_true;
			if (sa2 > sa1)
				if (sa2 < sa1 + d1 || sa2 + d2 - 360 > sa1)
					return pcb_true;
		}
		return pcb_false;
	}
	r1 = Arc1->Width;
	r2 = Arc2->Width;
	/* arcs centerlines are too far or too near */
	if (dl > r1 + r2 || dl + r1 < r2 || dl + r2 < r1) {
		/* check the nearest to the other arc's center point */
		dx = pdx * r1 / dl;
		dy = pdy * r1 / dl;
		if (dl + r1 < r2) {					/* Arc1 inside Arc2 */
			dx = -dx;
			dy = -dy;
		}

		if (radius_crosses_arc(Arc1->X + dx, Arc1->Y + dy, Arc1)
				&& pcb_is_point_on_arc(Arc1->X + dx, Arc1->Y + dy, t, Arc2))
			return pcb_true;

		dx = -pdx * r2 / dl;
		dy = -pdy * r2 / dl;
		if (dl + r2 < r1) {					/* Arc2 inside Arc1 */
			dx = -dx;
			dy = -dy;
		}

		if (radius_crosses_arc(Arc2->X + dx, Arc2->Y + dy, Arc2)
				&& pcb_is_point_on_arc(Arc2->X + dx, Arc2->Y + dy, t1, Arc1))
			return pcb_true;
		return pcb_false;
	}

	l = dl * dl;
	r1 *= r1;
	r2 *= r2;
	a = 0.5 * (r1 - r2 + l) / l;
	r1 = r1 / l;
	d = r1 - a * a;
	/* the circles are too far apart to touch or probably just touch:
	   check the nearest point */
	if (d < 0)
		d = 0;
	else
		d = sqrt(d);
	x = Arc1->X + a * pdx;
	y = Arc1->Y + a * pdy;
	dx = d * pdx;
	dy = d * pdy;
	if (radius_crosses_arc(x + dy, y - dx, Arc1)
			&& pcb_is_point_on_arc(x + dy, y - dx, t, Arc2))
		return pcb_true;
	if (radius_crosses_arc(x + dy, y - dx, Arc2)
			&& pcb_is_point_on_arc(x + dy, y - dx, t1, Arc1))
		return pcb_true;

	if (radius_crosses_arc(x - dy, y + dx, Arc1)
			&& pcb_is_point_on_arc(x - dy, y + dx, t, Arc2))
		return pcb_true;
	if (radius_crosses_arc(x - dy, y + dx, Arc2)
			&& pcb_is_point_on_arc(x - dy, y + dx, t1, Arc1))
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * Tests if point is same as line end point or center point
 */
static pcb_bool pcb_isc_rat_line(pcb_point_t *Point, pcb_line_t *Line)
{
	pcb_coord_t cx, cy;

	/* either end */
	if ((Point->X == Line->Point1.X && Point->Y == Line->Point1.Y)
			|| (Point->X == Line->Point2.X && Point->Y == Line->Point2.Y))
		return pcb_true;

	/* middle point */
	pcb_obj_center((pcb_any_obj_t *)Line, &cx, &cy);
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * Tests if point is same as arc end point or center point
 */
static pcb_bool pcb_isc_rat_arc(pcb_point_t *Point, pcb_arc_t *arc)
{
	pcb_coord_t cx, cy;

	/* either end */
	pcb_arc_get_end(arc, 0, &cx, &cy);
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	pcb_arc_get_end(arc, 1, &cx, &cy);
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	/* middle point */
	pcb_arc_middle(arc, &cx, &cy);
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * Tests if rat line point is connected to a polygon
 */
static pcb_bool pcb_isc_rat_poly(pcb_point_t *Point, pcb_poly_t *polygon)
{
	pcb_coord_t cx, cy;

	/* canonical point */
	cx = polygon->Clipped->contours->head.point[0];
	cy = polygon->Clipped->contours->head.point[1];
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	/* middle point */
	pcb_obj_center((pcb_any_obj_t *)polygon, &cx, &cy);
	if ((Point->X == cx) && (Point->Y == cy))
		return pcb_true;

	return pcb_false;
}

static void form_slanted_rectangle(pcb_point_t p[4], pcb_line_t *l)
/* writes vertices of a squared line */
{
	double dwx = 0, dwy = 0;
	if (l->Point1.Y == l->Point2.Y)
		dwx = l->Thickness / 2.0;
	else if (l->Point1.X == l->Point2.X)
		dwy = l->Thickness / 2.0;
	else {
		pcb_coord_t dX = l->Point2.X - l->Point1.X;
		pcb_coord_t dY = l->Point2.Y - l->Point1.Y;
		double r = pcb_distance(l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
		dwx = l->Thickness / 2.0 / r * dX;
		dwy = l->Thickness / 2.0 / r * dY;
	}
	p[0].X = l->Point1.X - dwx + dwy;
	p[0].Y = l->Point1.Y - dwy - dwx;
	p[1].X = l->Point1.X - dwx - dwy;
	p[1].Y = l->Point1.Y - dwy + dwx;
	p[2].X = l->Point2.X + dwx - dwy;
	p[2].Y = l->Point2.Y + dwy + dwx;
	p[3].X = l->Point2.X + dwx + dwy;
	p[3].Y = l->Point2.Y + dwy - dwx;
}

/* ---------------------------------------------------------------------------
 * checks if two lines intersect
 * from news FAQ:
 *
 *  Let A,B,C,D be 2-space position vectors.  Then the directed line
 *  segments AB & CD are given by:
 *
 *      AB=A+r(B-A), r in [0,1]
 *      CD=C+s(D-C), s in [0,1]
 *
 *  If AB & CD intersect, then
 *
 *      A+r(B-A)=C+s(D-C), or
 *
 *      XA+r(XB-XA)=XC+s(XD-XC)
 *      YA+r(YB-YA)=YC+s(YD-YC)  for some r,s in [0,1]
 *
 *  Solving the above for r and s yields
 *
 *          (YA-YC)(XD-XC)-(XA-XC)(YD-YC)
 *      r = -----------------------------  (eqn 1)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *          (YA-YC)(XB-XA)-(XA-XC)(YB-YA)
 *      s = -----------------------------  (eqn 2)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *  Let I be the position vector of the intersection point, then
 *
 *      I=A+r(B-A) or
 *
 *      XI=XA+r(XB-XA)
 *      YI=YA+r(YB-YA)
 *
 *  By examining the values of r & s, you can also determine some
 *  other limiting conditions:
 *
 *      If 0<=r<=1 & 0<=s<=1, intersection exists
 *          r<0 or r>1 or s<0 or s>1 line segments do not intersect
 *
 *      If the denominator in eqn 1 is zero, AB & CD are parallel
 *      If the numerator in eqn 1 is also zero, AB & CD are coincident
 *
 *  If the intersection point of the 2 lines are needed (lines in this
 *  context mean infinite lines) regardless whether the two line
 *  segments intersect, then
 *
 *      If r>1, I is located on extension of AB
 *      If r<0, I is located on extension of BA
 *      If s>1, I is located on extension of CD
 *      If s<0, I is located on extension of DC
 *
 *  Also note that the denominators of eqn 1 & 2 are identical.
 *
 */
pcb_bool pcb_isc_line_line(pcb_line_t *Line1, pcb_line_t *Line2)
{
	double s, r;
	double line1_dx, line1_dy, line2_dx, line2_dy, point1_dx, point1_dy;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Line1)) {	/* pretty reckless recursion */
		pcb_point_t p[4];
		form_slanted_rectangle(p, Line1);
		return pcb_is_line_in_quadrangle(p, Line2);
	}
	/* here come only round Line1 because pcb_is_line_in_quadrangle()
	   calls pcb_isc_line_line() with first argument rounded */
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Line2)) {
		pcb_point_t p[4];
		form_slanted_rectangle(p, Line2);
		return pcb_is_line_in_quadrangle(p, Line1);
	}
	/* now all lines are round */

	/* Check endpoints: this provides a quick exit, catches
	 *  cases where the "real" lines don't intersect but the
	 *  thick lines touch, and ensures that the dx/dy business
	 *  below does not cause a divide-by-zero. */
	if (pcb_is_point_in_line(Line2->Point1.X, Line2->Point1.Y, MAX(Line2->Thickness / 2 + Bloat, 0), (pcb_any_line_t *) Line1)
			|| pcb_is_point_in_line(Line2->Point2.X, Line2->Point2.Y, MAX(Line2->Thickness / 2 + Bloat, 0), (pcb_any_line_t *) Line1)
			|| pcb_is_point_in_line(Line1->Point1.X, Line1->Point1.Y, MAX(Line1->Thickness / 2 + Bloat, 0), (pcb_any_line_t *) Line2)
			|| pcb_is_point_in_line(Line1->Point2.X, Line1->Point2.Y, MAX(Line1->Thickness / 2 + Bloat, 0), (pcb_any_line_t *) Line2))
		return pcb_true;

	/* setup some constants */
	line1_dx = Line1->Point2.X - Line1->Point1.X;
	line1_dy = Line1->Point2.Y - Line1->Point1.Y;
	line2_dx = Line2->Point2.X - Line2->Point1.X;
	line2_dy = Line2->Point2.Y - Line2->Point1.Y;
	point1_dx = Line1->Point1.X - Line2->Point1.X;
	point1_dy = Line1->Point1.Y - Line2->Point1.Y;

	/* If either line is a point, we have failed already, since the
	 *   endpoint check above will have caught an "intersection". */
	if ((line1_dx == 0 && line1_dy == 0)
			|| (line2_dx == 0 && line2_dy == 0))
		return pcb_false;

	/* set s to cross product of Line1 and the line
	 *   Line1.Point1--Line2.Point1 (as vectors) */
	s = point1_dy * line1_dx - point1_dx * line1_dy;

	/* set r to cross product of both lines (as vectors) */
	r = line1_dx * line2_dy - line1_dy * line2_dx;

	/* No cross product means parallel lines, or maybe Line2 is
	 *  zero-length. In either case, since we did a bounding-box
	 *  check before getting here, the above pcb_is_point_in_line() checks
	 *  will have caught any intersections. */
	if (r == 0.0)
		return pcb_false;

	s /= r;
	r = (point1_dy * line2_dx - point1_dx * line2_dy) / r;

	/* intersection is at least on AB */
	if (r >= 0.0 && r <= 1.0)
		return (s >= 0.0 && s <= 1.0);

	/* intersection is at least on CD */
	/* [removed this case since it always returns pcb_false --asp] */
	return pcb_false;
}

/*---------------------------------------------------
 *
 * Check for line intersection with an arc
 *
 * Mostly this is like the circle/line intersection
 * found in pcb_is_point_on_line(search.c) see the detailed
 * discussion for the basics there.
 *
 * Since this is only an arc, not a full circle we need
 * to find the actual points of intersection with the
 * circle, and see if they are on the arc.
 *
 * To do this, we translate along the line from the point Q
 * plus or minus a distance delta = sqrt(Radius^2 - d^2)
 * but it's handy to normalize with respect to l, the line
 * length so a single projection is done (e.g. we don't first
 * find the point Q
 *
 * The projection is now of the form
 *
 *      Px = X1 + (r +- r2)(X2 - X1)
 *      Py = Y1 + (r +- r2)(Y2 - Y1)
 *
 * Where r2 sqrt(Radius^2 l^2 - d^2)/l^2
 * note that this is the variable d, not the symbol d described in IsPointOnLine
 * (variable d = symbol d * l)
 *
 * The end points are hell so they are checked individually
 */
pcb_bool pcb_isc_line_arc(pcb_line_t *Line, pcb_arc_t *Arc)
{
	double dx, dy, dx1, dy1, l, d, r, r2, Radius;
	pcb_coord_t ex, ey;

	dx = Line->Point2.X - Line->Point1.X;
	dy = Line->Point2.Y - Line->Point1.Y;
	dx1 = Line->Point1.X - Arc->X;
	dy1 = Line->Point1.Y - Arc->Y;
	l = dx * dx + dy * dy;
	d = dx * dy1 - dy * dx1;
	d *= d;

	/* use the larger diameter circle first */
	Radius = Arc->Width + MAX(0.5 * (Arc->Thickness + Line->Thickness) + Bloat, 0.0);
	Radius *= Radius;
	r2 = Radius * l - d;
	/* projection doesn't even intersect circle when r2 < 0 */
	if (r2 < 0)
		return pcb_false;
	/* check the ends of the line in case the projected point */
	/* of intersection is beyond the line end */
	if (pcb_is_point_on_arc(Line->Point1.X, Line->Point1.Y, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return pcb_true;
	if (pcb_is_point_on_arc(Line->Point2.X, Line->Point2.Y, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return pcb_true;
	if (l == 0.0)
		return pcb_false;
	r2 = sqrt(r2);
	Radius = -(dx * dx1 + dy * dy1);
	r = (Radius + r2) / l;
	if (r >= 0 && r <= 1
			&& pcb_is_point_on_arc(Line->Point1.X + r * dx, Line->Point1.Y + r * dy, MAX(0.5 * Line->Thickness + Bloat, 0.0) + 1, Arc))
		return pcb_true;
	r = (Radius - r2) / l;
	if (r >= 0 && r <= 1
			&& pcb_is_point_on_arc(Line->Point1.X + r * dx, Line->Point1.Y + r * dy, MAX(0.5 * Line->Thickness + Bloat, 0.0) + 1, Arc))
		return pcb_true;

	/* check arc end points */
	pcb_arc_get_end(Arc, 0, &ex, &ey);
	if (pcb_is_point_in_line(ex, ey, Arc->Thickness * 0.5 + Bloat, (pcb_any_line_t *) Line))
		return pcb_true;

	pcb_arc_get_end(Arc, 1, &ex, &ey);
	if (pcb_is_point_in_line(ex, ey, Arc->Thickness * 0.5 + Bloat, (pcb_any_line_t *) Line))
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * checks if an arc has a connection to a polygon
 *
 * - first check if the arc can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the arc. If none of them matches
 * - check all segments of the polygon against the arc.
 */
pcb_bool pcb_isc_arc_poly(pcb_arc_t *Arc, pcb_poly_t *Polygon)
{
	pcb_box_t *Box = (pcb_box_t *) Arc;

	/* arcs with clearance never touch polys */
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, Polygon) && PCB_OBJ_HAS_CLEARANCE(Arc))
		return pcb_false;
	if (!Polygon->Clipped)
		return pcb_false;
	if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
			&& Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
			&& Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat) {
		pcb_polyarea_t *ap;

		if (!(ap = pcb_poly_from_arc(Arc, Arc->Thickness + Bloat)))
			return pcb_false;							/* error */
		return pcb_poly_isects_poly(ap, Polygon, pcb_true);
	}
	return pcb_false;
}

pcb_bool pcb_isc_arc_polyarea(pcb_arc_t *Arc, pcb_polyarea_t *pa)
{
	pcb_box_t *Box = (pcb_box_t *) Arc;
	pcb_bool res = pcb_false;

	/* arcs with clearance never touch polys */
	if ((Box->X1 <= pa->contours->xmax + Bloat) && (Box->X2 >= pa->contours->xmin - Bloat)
			&& (Box->Y1 <= pa->contours->ymax + Bloat) && (Box->Y2 >= pa->contours->ymin - Bloat)) {
		pcb_polyarea_t *arcp;

		if (!(arcp = pcb_poly_from_arc(Arc, Arc->Thickness + Bloat)))
			return pcb_false; /* error */
		res = pcb_polyarea_touching(arcp, pa);
		pcb_polyarea_free(&arcp);
	}
	return res;
}


/* ---------------------------------------------------------------------------
 * checks if a line has a connection to a polygon
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 */
pcb_bool pcb_is_line_in_poly(pcb_line_t *Line, pcb_poly_t *Polygon)
{
	pcb_box_t *Box = (pcb_box_t *) Line;
	pcb_polyarea_t *lp;

	/* lines with clearance never touch polygons */
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, Polygon) && PCB_OBJ_HAS_CLEARANCE(Line))
		return pcb_false;
	if (!Polygon->Clipped)
		return pcb_false;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Line) && (Line->Point1.X == Line->Point2.X || Line->Point1.Y == Line->Point2.Y)) {
		pcb_coord_t wid = (Line->Thickness + Bloat + 1) / 2;
		pcb_coord_t x1, x2, y1, y2;

		x1 = MIN(Line->Point1.X, Line->Point2.X) - wid;
		y1 = MIN(Line->Point1.Y, Line->Point2.Y) - wid;
		x2 = MAX(Line->Point1.X, Line->Point2.X) + wid;
		y2 = MAX(Line->Point1.Y, Line->Point2.Y) + wid;
		return pcb_poly_is_rect_in_p(x1, y1, x2, y2, Polygon);
	}
	if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
			&& Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
			&& Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat) {
		if (!(lp = pcb_poly_from_line(Line, Line->Thickness + Bloat)))
			return pcb_false;							/* error */
		return pcb_poly_isects_poly(lp, Polygon, pcb_true);
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * checks if a polygon has a connection to a second one
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 against the ones of P2
 */
pcb_bool pcb_is_poly_in_poly(pcb_poly_t *P1, pcb_poly_t *P2)
{
	int pcp_cnt = 0;
	pcb_coord_t pcp_gap;

	if (!P1->Clipped || !P2->Clipped)
		return pcb_false;
	assert(P1->Clipped->contours);
	assert(P2->Clipped->contours);


	/* first check if both bounding boxes intersect. If not, return quickly */
	if (P1->Clipped->contours->xmin - Bloat > P2->Clipped->contours->xmax ||
			P1->Clipped->contours->xmax + Bloat < P2->Clipped->contours->xmin ||
			P1->Clipped->contours->ymin - Bloat > P2->Clipped->contours->ymax ||
			P1->Clipped->contours->ymax + Bloat < P2->Clipped->contours->ymin)
		return pcb_false;

	/* cheat: poly-clear-poly means we did generate the clearance; this
	   shall happen only if there's exactly one poly that is clearing the other */
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, P1)) {
		pcp_cnt++;
		pcp_gap = P1->Clearance / 2.0;
	}
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, P2)) {
		pcp_cnt++;
		pcp_gap = P2->Clearance / 2.0;
	}
	if (pcp_cnt == 1) {
		if (pcp_gap >= Bloat)
			return pcb_false;
		return pcb_true;
	}

	/* first check un-bloated case */
	if (pcb_poly_isects_poly(P1->Clipped, P2, pcb_false))
		return pcb_true;

	/* now the difficult case of bloated */
	if (Bloat > 0) {
		pcb_pline_t *c;
		for (c = P1->Clipped->contours; c; c = c->next) {
			pcb_line_t line;
			pcb_vnode_t *v = &c->head;
			if (c->xmin - Bloat <= P2->Clipped->contours->xmax &&
					c->xmax + Bloat >= P2->Clipped->contours->xmin &&
					c->ymin - Bloat <= P2->Clipped->contours->ymax && c->ymax + Bloat >= P2->Clipped->contours->ymin) {

				line.Point1.X = v->point[0];
				line.Point1.Y = v->point[1];
				line.Thickness = 2 * Bloat;
				line.Clearance = 0;
				line.Flags = pcb_no_flags();
				for (v = v->next; v != &c->head; v = v->next) {
					line.Point2.X = v->point[0];
					line.Point2.Y = v->point[1];
					pcb_line_bbox(&line);
					if (pcb_is_line_in_poly(&line, P2))
						return pcb_true;
					line.Point1.X = line.Point2.X;
					line.Point1.Y = line.Point2.Y;
				}
			}
		}
	}

	return pcb_false;
}

pcb_bool BoxBoxIntersection(pcb_box_t *b1, pcb_box_t *b2)
{
	if (b2->X2 < b1->X1 || b2->X1 > b1->X2)
		return pcb_false;
	if (b2->Y2 < b1->Y1 || b2->Y1 > b1->Y2)
		return pcb_false;
	return pcb_true;
}

/* returns whether a round-cap pcb line touches a polygon; assumes bounding
   boxes do touch */
PCB_INLINE pcb_bool_t pcb_intersect_line_polyline(pcb_pline_t *pl, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick)
{
	pcb_coord_t ox, oy, vx, vy;
	double dx, dy, h, l;

	thick += Bloat*2;
	if (thick < 0) thick = 0;

	/* single-point line - check only one circle*/
	if ((x1 == x2) && (y1 == y2))
		return pcb_pline_overlaps_circ(pl, x1, y1, thick/2);

	/* long line - check ends */
	if (pcb_pline_overlaps_circ(pl, x1, y1, thick/2)) return pcb_true;
	if (pcb_pline_overlaps_circ(pl, x2, y2, thick/2)) return pcb_true;

	dx = x2 - x1;
	dy = y2 - y1;
	l = sqrt(PCB_SQUARE(dx) + PCB_SQUARE(dy));
	h = 0.5 * thick / l;
	ox = dy * h + 0.5 * SGN(dy);
	oy = -(dx * h + 0.5 * SGN(dx));
	vx = (double)dx / -l * ((double)Bloat/2.0);
	vy = (double)dy / -l * ((double)Bloat/2.0);

	/* long line - consider edge intersection */
	if (pcb_pline_isect_line(pl, x1 + ox + vx, y1 + oy + vy, x2 + ox - vx, y2 + oy - vy, NULL, NULL)) return pcb_true;
	if (pcb_pline_isect_line(pl, x1 - ox + vx, y1 - oy + vy, x2 - ox - vx, y2 - oy - vy, NULL, NULL)) return pcb_true;

	/* A corner case is when the polyline is fully within the line. By now we
	   are sure there's no contour intersection, so if any of the polyline points
	   is in, the whole polyline is in. */
	{
		pcb_vector_t q[4];

		q[0][0] = x1 + ox; q[0][1] = y1 + oy;
		q[1][0] = x2 + ox; q[1][1] = y2 + oy;
		q[2][0] = x1 - ox; q[2][1] = y1 - oy;
		q[3][0] = x2 - ox; q[3][1] = y2 - oy;

		return pcb_is_point_in_convex_quad(pl->head.point, q);
	}
}

#define shape_line_to_pcb_line(ps, shape_line, pcb_line) \
	do { \
		pcb_line.Point1.X = shape_line.x1 + ps->x; \
		pcb_line.Point1.Y = shape_line.y1 + ps->y; \
		pcb_line.Point2.X = shape_line.x2 + ps->x; \
		pcb_line.Point2.Y = shape_line.y2 + ps->y; \
		pcb_line.Clearance = 0; \
		pcb_line.Thickness = shape_line.thickness; \
		pcb_line.Flags = shape_line.square ? pcb_flag_make(PCB_FLAG_SQUARE) : pcb_no_flags(); \
	} while(0)

pcb_bool_t pcb_pstk_intersect_line(pcb_pstk_t *ps, pcb_line_t *line)
{
	pcb_pstk_shape_t *shape = pcb_pstk_shape_at(PCB, ps, line->parent.layer);
	if (shape == NULL) return pcb_false;
	switch(shape->shape) {
		case PCB_PSSH_POLY:
			if (shape->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shape->data.poly);
			return pcb_intersect_line_polyline(shape->data.poly.pa->contours, line->Point1.X - ps->x, line->Point1.Y - ps->y, line->Point2.X - ps->x, line->Point2.Y - ps->y, line->Thickness);
		case PCB_PSSH_LINE:
		{
			pcb_line_t tmp;
			shape_line_to_pcb_line(ps, shape->data.line, tmp);
			return pcb_isc_line_line(line, &tmp);
		}
		case PCB_PSSH_CIRC:
		{
			pcb_any_line_t tmp;
			tmp.Point1.X = line->Point1.X;
			tmp.Point1.Y = line->Point1.Y;
			tmp.Point2.X = line->Point2.X;
			tmp.Point2.Y = line->Point2.Y;
			tmp.Thickness = line->Thickness;
			tmp.Flags = pcb_no_flags();
			return pcb_is_point_in_line(shape->data.circ.x + ps->x, shape->data.circ.y + ps->y, shape->data.circ.dia/2, &tmp);
		}
		case PCB_PSSH_HSHADOW:
			return 0; /* non-object won't intersect */
	}
	return pcb_false;
}


PCB_INLINE pcb_bool_t pcb_pstk_intersect_arc(pcb_pstk_t *ps, pcb_arc_t *arc)
{
	pcb_pstk_shape_t *shape = pcb_pstk_shape_at(PCB, ps, arc->parent.layer);
	if (shape == NULL) return pcb_false;
	switch(shape->shape) {
		case PCB_PSSH_POLY:
			{
				pcb_arc_t tmp;

				/* transform the arc back to 0;0 of the padstack */
				tmp = *arc;
				tmp.X -= ps->x;
				tmp.Y -= ps->y;
				tmp.BoundingBox.X1 -= ps->x;
				tmp.BoundingBox.Y1 -= ps->y;
				tmp.BoundingBox.X2 -= ps->x;
				tmp.BoundingBox.Y2 -= ps->y;

				if (shape->data.poly.pa == NULL)
					pcb_pstk_shape_update_pa(&shape->data.poly);

				return pcb_isc_arc_polyarea(&tmp, shape->data.poly.pa);
			}

		case PCB_PSSH_LINE:
		{
			pcb_line_t tmp;
			shape_line_to_pcb_line(ps, shape->data.line, tmp);
			return pcb_isc_line_arc(&tmp, arc);
		}
		case PCB_PSSH_CIRC:
			return pcb_is_point_on_arc(shape->data.circ.x + ps->x, shape->data.circ.y + ps->y, shape->data.circ.dia/2, arc);
			break;
		case PCB_PSSH_HSHADOW:
			return 0; /* non-object won't intersect */
	}
	return pcb_false;
}

PCB_INLINE pcb_polyarea_t *pcb_pstk_shp_poly2area(pcb_pstk_t *ps, pcb_pstk_shape_t *shape)
{
	int n;
	pcb_pline_t *pl;
	pcb_vector_t v;
	pcb_polyarea_t *shp = pcb_polyarea_create();

	v[0] = shape->data.poly.x[0] + ps->x; v[1] = shape->data.poly.y[0] + ps->y;
	pl = pcb_poly_contour_new(v);
	for(n = 1; n < shape->data.poly.len; n++) {
		v[0] = shape->data.poly.x[n] + ps->x; v[1] = shape->data.poly.y[n] + ps->y;
		pcb_poly_vertex_include(pl->head.prev, pcb_poly_node_create(v));
	}
	pcb_poly_contour_pre(pl, 1);
	pcb_polyarea_contour_include(shp, pl);

	if (!pcb_poly_valid(shp)) {
/*		pcb_polyarea_free(&shp); shp = pcb_polyarea_create();*/
		pcb_poly_contour_inv(pl);
		pcb_polyarea_contour_include(shp, pl);
	}

	return shp;
}

PCB_INLINE pcb_bool_t pcb_pstk_intersect_poly(pcb_pstk_t *ps, pcb_poly_t *poly)
{
	pcb_pstk_shape_t *shape = pcb_pstk_shape_at(PCB, ps, poly->parent.layer);
	if (shape == NULL) return pcb_false;

	switch(shape->shape) {
		case PCB_PSSH_POLY:
		{
			/* convert the shape poly to a new poly so that it can be intersected */
			pcb_bool res;
			pcb_polyarea_t *shp = pcb_pstk_shp_poly2area(ps, shape);
			res = pcb_polyarea_touching(shp, poly->Clipped);
			pcb_polyarea_free(&shp);
			return res;
		}
		case PCB_PSSH_LINE:
		{
			pcb_line_t tmp;
			shape_line_to_pcb_line(ps, shape->data.line, tmp);
			pcb_line_bbox(&tmp);
			return pcb_is_line_in_poly(&tmp, poly);
		}
		case PCB_PSSH_CIRC:
		{
			pcb_line_t tmp;
			tmp.Point1.X = tmp.Point2.X = shape->data.circ.x + ps->x;
			tmp.Point1.Y = tmp.Point2.Y = shape->data.circ.y + ps->y;
			tmp.Clearance = 0;
			tmp.Thickness = shape->data.circ.dia;
			tmp.Flags = pcb_no_flags();
			pcb_line_bbox(&tmp);
			return pcb_is_line_in_poly(&tmp, poly);
		}
		case PCB_PSSH_HSHADOW:
			return 0; /* non-object won't intersect */
	}
	return pcb_false;
}

PCB_INLINE pcb_bool_t pstk_shape_int_circ_poly(pcb_pstk_t *p, pcb_pstk_shape_t *sp, pcb_pstk_t *c, pcb_pstk_shape_t *sc)
{
	pcb_coord_t px = sc->data.circ.x + c->x - p->x;
	pcb_coord_t py = sc->data.circ.y + c->y - p->y;
	return pcb_intersect_line_polyline(sp->data.poly.pa->contours,  px, py, px, py, sc->data.circ.dia);
}

PCB_INLINE pcb_bool_t pstk_shape_int_circ_line(pcb_pstk_t *l, pcb_pstk_shape_t *sl, pcb_pstk_t *c, pcb_pstk_shape_t *sc)
{
	pcb_any_line_t tmp;
	tmp.Point1.X = sl->data.line.x1 + l->x;
	tmp.Point1.Y = sl->data.line.y1 + l->y;
	tmp.Point2.X = sl->data.line.x2 + l->x;
	tmp.Point2.Y = sl->data.line.y2 + l->y;
	tmp.Thickness = sl->data.line.thickness;
	tmp.Flags = pcb_no_flags();
	return pcb_is_point_in_line(sc->data.circ.x + c->x, sc->data.circ.y + c->y, sc->data.circ.dia/2, &tmp);
}

PCB_INLINE pcb_bool_t pcb_pstk_shape_intersect(pcb_pstk_t *ps1, pcb_pstk_shape_t *shape1, pcb_pstk_t *ps2, pcb_pstk_shape_t *shape2)
{
	if ((shape1->shape == PCB_PSSH_POLY) && (shape1->data.poly.pa == NULL))
		pcb_pstk_shape_update_pa(&shape1->data.poly);
	if ((shape2->shape == PCB_PSSH_POLY) && (shape2->data.poly.pa == NULL))
		pcb_pstk_shape_update_pa(&shape2->data.poly);

	switch(shape1->shape) {
		case PCB_PSSH_POLY:
			switch(shape2->shape) {
				case PCB_PSSH_POLY:
				{
					pcb_bool res;
					pcb_polyarea_t *shp1 = pcb_pstk_shp_poly2area(ps1, shape1);
					pcb_polyarea_t *shp2 = pcb_pstk_shp_poly2area(ps2, shape2);
					res = pcb_polyarea_touching(shp1, shp2);
					pcb_polyarea_free(&shp1);
					pcb_polyarea_free(&shp2);
					return res;
				}
				case PCB_PSSH_LINE:
					return pcb_intersect_line_polyline(shape1->data.poly.pa->contours, shape2->data.line.x1 + ps2->x - ps1->x, shape2->data.line.y1 + ps2->y - ps1->y, shape2->data.line.x2 + ps2->x - ps1->x, shape2->data.line.y2 + ps2->y - ps1->y, shape2->data.line.thickness);
				case PCB_PSSH_CIRC:
					return pstk_shape_int_circ_poly(ps1, shape1, ps2, shape2);
				case PCB_PSSH_HSHADOW:
					return 0; /* non-object won't intersect */
			}
			break;

		case PCB_PSSH_LINE:
			switch(shape2->shape) {
				case PCB_PSSH_POLY:
					return pcb_intersect_line_polyline(shape2->data.poly.pa->contours, shape1->data.line.x1 + ps1->x - ps2->x, shape1->data.line.y1 + ps1->y - ps2->y, shape1->data.line.x2 + ps1->x - ps2->x, shape1->data.line.y2 + ps1->y - ps2->y, shape1->data.line.thickness);
				case PCB_PSSH_LINE:
				{
					pcb_line_t tmp1, tmp2;
					shape_line_to_pcb_line(ps1, shape1->data.line, tmp1);
					shape_line_to_pcb_line(ps2, shape2->data.line, tmp2);
					return pcb_isc_line_line(&tmp1, &tmp2);
				}
				case PCB_PSSH_CIRC:
					return pstk_shape_int_circ_line(ps1, shape1, ps2, shape2);
				case PCB_PSSH_HSHADOW:
					return 0; /* non-object won't intersect */
			}
			break;

		case PCB_PSSH_CIRC:
			switch(shape2->shape) {
				case PCB_PSSH_POLY:
					return pstk_shape_int_circ_poly(ps2, shape2, ps1, shape1);
				case PCB_PSSH_LINE:
					return pstk_shape_int_circ_line(ps2, shape2, ps1, shape1);
				case PCB_PSSH_CIRC:
					{
						double cdist2 = pcb_distance2(ps1->x + shape1->data.circ.x, ps1->y + shape1->data.circ.y, ps2->x + shape2->data.circ.x, ps2->y + shape2->data.circ.y);
						double dia = ((double)shape1->data.circ.dia + (double)shape2->data.circ.dia)/2.0;
						return cdist2 <= dia*dia;
					}
				case PCB_PSSH_HSHADOW:
					return 0; /* non-object won't intersect */
			}
			break;

		case PCB_PSSH_HSHADOW:
			return 0; /* non-object won't intersect */
	}
	return pcb_false;
}

PCB_INLINE pcb_bool_t pcb_pstk_intersect_pstk(pcb_pstk_t *ps1, pcb_pstk_t *ps2)
{
	pcb_layer_t *ly;
	int n;

	for(n = 0, ly = PCB->Data->Layer; n < PCB->Data->LayerN; n++,ly++) {
		pcb_pstk_shape_t *shape1;
		pcb_pstk_shape_t *shape2;
		pcb_layer_type_t lyt = pcb_layer_flags_(ly);

		if (!(lyt & PCB_LYT_COPPER)) /* consider only copper for connections */
			continue;

		shape1 = pcb_pstk_shape_at(PCB, ps1, ly);
		if (shape1 == NULL) continue;
		shape2 = pcb_pstk_shape_at(PCB, ps2, ly);
		if (shape2 == NULL) continue;

		if (pcb_pstk_shape_intersect(ps1, shape1, ps2, shape2)) return pcb_true;
	}
	return pcb_false;
}


PCB_INLINE pcb_bool_t pcb_pstk_intersect_rat(pcb_pstk_t *ps, pcb_rat_t *rat)
{
	pcb_board_t *pcb = PCB;

	if ((rat->Point1.X == ps->x) && (rat->Point1.Y == ps->y)) {
		pcb_layer_t *layer = pcb_get_layer(pcb->Data, pcb->LayerGroups.grp[rat->group1].lid[0]);
		if ((layer != NULL) && (pcb_pstk_shape_at(pcb, ps, layer) != NULL))
			return pcb_true;
	}

	if ((rat->Point2.X == ps->x) && (rat->Point2.Y == ps->y)) {
		pcb_layer_t *layer = pcb_get_layer(pcb->Data, pcb->LayerGroups.grp[rat->group2].lid[0]);
		if ((layer != NULL) && (pcb_pstk_shape_at(pcb, ps, layer) != NULL))
			return pcb_true;
	}

	return pcb_false;
}
