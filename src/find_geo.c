/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Generic geometric functions used by find.c lookup */

/*
 * Intersection of line <--> circle:
 * - calculate the signed distance from the line to the center,
 *   return false if abs(distance) > R
 * - get the distance from the line <--> distancevector intersection to
 *   (X1,Y1) in range [0,1], return true if 0 <= distance <= 1
 * - depending on (r > 1.0 or r < 0.0) check the distance of X2,Y2 or X1,Y1
 *   to X,Y
 *
 * Intersection of line <--> line:
 * - see the description of 'LineLineIntersect()'
 */

#define EXPAND_BOUNDS(p) if (Bloat > 0) {\
       (p)->BoundingBox.X1 -= Bloat; \
       (p)->BoundingBox.X2 += Bloat; \
       (p)->BoundingBox.Y1 -= Bloat; \
       (p)->BoundingBox.Y2 += Bloat;}

#define IS_PV_ON_RAT(PV, Rat) \
	(IsPointOnLineEnd((PV)->X,(PV)->Y, (Rat)))

#define IS_PV_ON_ARC(PV, Arc)	\
	(TEST_FLAG(PCB_FLAG_SQUARE, (PV)) ? \
		IsArcInRectangle( \
			(PV)->X -MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y -MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(PV)->X +MAX(((PV)->Thickness+1)/2 +Bloat,0), (PV)->Y +MAX(((PV)->Thickness+1)/2 +Bloat,0), \
			(Arc)) : \
		IsPointOnArc((PV)->X,(PV)->Y,MAX((PV)->Thickness/2.0 + Bloat,0.0), (Arc)))

#define	IS_PV_ON_PAD(PV,Pad) \
	( IsPointInPad((PV)->X, (PV)->Y, MAX((PV)->Thickness/2 +Bloat,0), (Pad)))

/* reduce arc start angle and delta to 0..360 */
static void normalize_angles(Angle * sa, Angle * d)
{
	if (*d < 0) {
		*sa += *d;
		*d = -*d;
	}
	if (*d > 360)									/* full circle */
		*d = 360;
	*sa = NormalizeAngle(*sa);
}

static int radius_crosses_arc(double x, double y, ArcTypePtr arc)
{
	double alpha = atan2(y - arc->Y, -x + arc->X) * PCB_RAD_TO_DEG;
	Angle sa = arc->StartAngle, d = arc->Delta;

	normalize_angles(&sa, &d);
	if (alpha < 0)
		alpha += 360;
	if (sa <= alpha)
		return (sa + d) >= alpha;
	return (sa + d - 360) >= alpha;
}

static void get_arc_ends(Coord * box, ArcTypePtr arc)
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
static bool ArcArcIntersect(ArcTypePtr Arc1, ArcTypePtr Arc2)
{
	double x, y, dx, dy, r1, r2, a, d, l, t, t1, t2, dl;
	Coord pdx, pdy;
	Coord box[8];

	t = 0.5 * Arc1->Thickness + Bloat;
	t2 = 0.5 * Arc2->Thickness;
	t1 = t2 + Bloat;

	/* too thin arc */
	if (t < 0 || t1 < 0)
		return false;

	/* try the end points first */
	get_arc_ends(&box[0], Arc1);
	get_arc_ends(&box[4], Arc2);
	if (IsPointOnArc(box[0], box[1], t, Arc2)
			|| IsPointOnArc(box[2], box[3], t, Arc2)
			|| IsPointOnArc(box[4], box[5], t, Arc1)
			|| IsPointOnArc(box[6], box[7], t, Arc1))
		return true;

	pdx = Arc2->X - Arc1->X;
	pdy = Arc2->Y - Arc1->Y;
	dl = Distance(Arc1->X, Arc1->Y, Arc2->X, Arc2->Y);
	/* concentric arcs, simpler intersection conditions */
	if (dl < 0.5) {
		if ((Arc1->Width - t >= Arc2->Width - t2 && Arc1->Width - t <= Arc2->Width + t2)
				|| (Arc1->Width + t >= Arc2->Width - t2 && Arc1->Width + t <= Arc2->Width + t2)) {
			Angle sa1 = Arc1->StartAngle, d1 = Arc1->Delta;
			Angle sa2 = Arc2->StartAngle, d2 = Arc2->Delta;
			/* NB the endpoints have already been checked,
			   so we just compare the angles */

			normalize_angles(&sa1, &d1);
			normalize_angles(&sa2, &d2);
			/* sa1 == sa2 was caught when checking endpoints */
			if (sa1 > sa2)
				if (sa1 < sa2 + d2 || sa1 + d1 - 360 > sa2)
					return true;
			if (sa2 > sa1)
				if (sa2 < sa1 + d1 || sa2 + d2 - 360 > sa1)
					return true;
		}
		return false;
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
				&& IsPointOnArc(Arc1->X + dx, Arc1->Y + dy, t, Arc2))
			return true;

		dx = -pdx * r2 / dl;
		dy = -pdy * r2 / dl;
		if (dl + r2 < r1) {					/* Arc2 inside Arc1 */
			dx = -dx;
			dy = -dy;
		}

		if (radius_crosses_arc(Arc2->X + dx, Arc2->Y + dy, Arc2)
				&& IsPointOnArc(Arc2->X + dx, Arc2->Y + dy, t1, Arc1))
			return true;
		return false;
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
			&& IsPointOnArc(x + dy, y - dx, t, Arc2))
		return true;
	if (radius_crosses_arc(x + dy, y - dx, Arc2)
			&& IsPointOnArc(x + dy, y - dx, t1, Arc1))
		return true;

	if (radius_crosses_arc(x - dy, y + dx, Arc1)
			&& IsPointOnArc(x - dy, y + dx, t, Arc2))
		return true;
	if (radius_crosses_arc(x - dy, y + dx, Arc2)
			&& IsPointOnArc(x - dy, y + dx, t1, Arc1))
		return true;
	return false;
}

/* ---------------------------------------------------------------------------
 * Tests if point is same as line end point
 */
static bool IsRatPointOnLineEnd(PointTypePtr Point, LineTypePtr Line)
{
	if ((Point->X == Line->Point1.X && Point->Y == Line->Point1.Y)
			|| (Point->X == Line->Point2.X && Point->Y == Line->Point2.Y))
		return (true);
	return (false);
}

static void form_slanted_rectangle(PointType p[4], LineTypePtr l)
/* writes vertices of a squared line */
{
	double dwx = 0, dwy = 0;
	if (l->Point1.Y == l->Point2.Y)
		dwx = l->Thickness / 2.0;
	else if (l->Point1.X == l->Point2.X)
		dwy = l->Thickness / 2.0;
	else {
		Coord dX = l->Point2.X - l->Point1.X;
		Coord dY = l->Point2.Y - l->Point1.Y;
		double r = Distance(l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
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
bool LineLineIntersect(LineTypePtr Line1, LineTypePtr Line2)
{
	double s, r;
	double line1_dx, line1_dy, line2_dx, line2_dy, point1_dx, point1_dy;
	if (TEST_FLAG(PCB_FLAG_SQUARE, Line1)) {	/* pretty reckless recursion */
		PointType p[4];
		form_slanted_rectangle(p, Line1);
		return IsLineInQuadrangle(p, Line2);
	}
	/* here come only round Line1 because IsLineInQuadrangle()
	   calls LineLineIntersect() with first argument rounded */
	if (TEST_FLAG(PCB_FLAG_SQUARE, Line2)) {
		PointType p[4];
		form_slanted_rectangle(p, Line2);
		return IsLineInQuadrangle(p, Line1);
	}
	/* now all lines are round */

	/* Check endpoints: this provides a quick exit, catches
	 *  cases where the "real" lines don't intersect but the
	 *  thick lines touch, and ensures that the dx/dy business
	 *  below does not cause a divide-by-zero. */
	if (IsPointInPad(Line2->Point1.X, Line2->Point1.Y, MAX(Line2->Thickness / 2 + Bloat, 0), (PadTypePtr) Line1)
			|| IsPointInPad(Line2->Point2.X, Line2->Point2.Y, MAX(Line2->Thickness / 2 + Bloat, 0), (PadTypePtr) Line1)
			|| IsPointInPad(Line1->Point1.X, Line1->Point1.Y, MAX(Line1->Thickness / 2 + Bloat, 0), (PadTypePtr) Line2)
			|| IsPointInPad(Line1->Point2.X, Line1->Point2.Y, MAX(Line1->Thickness / 2 + Bloat, 0), (PadTypePtr) Line2))
		return true;

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
		return false;

	/* set s to cross product of Line1 and the line
	 *   Line1.Point1--Line2.Point1 (as vectors) */
	s = point1_dy * line1_dx - point1_dx * line1_dy;

	/* set r to cross product of both lines (as vectors) */
	r = line1_dx * line2_dy - line1_dy * line2_dx;

	/* No cross product means parallel lines, or maybe Line2 is
	 *  zero-length. In either case, since we did a bounding-box
	 *  check before getting here, the above IsPointInPad() checks
	 *  will have caught any intersections. */
	if (r == 0.0)
		return false;

	s /= r;
	r = (point1_dy * line2_dx - point1_dx * line2_dy) / r;

	/* intersection is at least on AB */
	if (r >= 0.0 && r <= 1.0)
		return (s >= 0.0 && s <= 1.0);

	/* intersection is at least on CD */
	/* [removed this case since it always returns false --asp] */
	return false;
}

/*---------------------------------------------------
 *
 * Check for line intersection with an arc
 *
 * Mostly this is like the circle/line intersection
 * found in IsPointOnLine (search.c) see the detailed
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
bool LineArcIntersect(LineTypePtr Line, ArcTypePtr Arc)
{
	double dx, dy, dx1, dy1, l, d, r, r2, Radius;
	BoxTypePtr box;

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
		return (false);
	/* check the ends of the line in case the projected point */
	/* of intersection is beyond the line end */
	if (IsPointOnArc(Line->Point1.X, Line->Point1.Y, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return (true);
	if (IsPointOnArc(Line->Point2.X, Line->Point2.Y, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return (true);
	if (l == 0.0)
		return (false);
	r2 = sqrt(r2);
	Radius = -(dx * dx1 + dy * dy1);
	r = (Radius + r2) / l;
	if (r >= 0 && r <= 1
			&& IsPointOnArc(Line->Point1.X + r * dx, Line->Point1.Y + r * dy, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return (true);
	r = (Radius - r2) / l;
	if (r >= 0 && r <= 1
			&& IsPointOnArc(Line->Point1.X + r * dx, Line->Point1.Y + r * dy, MAX(0.5 * Line->Thickness + Bloat, 0.0), Arc))
		return (true);
	/* check arc end points */
	box = GetArcEnds(Arc);
	if (IsPointInPad(box->X1, box->Y1, Arc->Thickness * 0.5 + Bloat, (PadTypePtr) Line))
		return true;
	if (IsPointInPad(box->X2, box->Y2, Arc->Thickness * 0.5 + Bloat, (PadTypePtr) Line))
		return true;
	return false;
}

/* ---------------------------------------------------------------------------
 * checks if an arc has a connection to a polygon
 *
 * - first check if the arc can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the arc. If none of them matches
 * - check all segments of the polygon against the arc.
 */
bool IsArcInPolygon(ArcTypePtr Arc, PolygonTypePtr Polygon)
{
	BoxTypePtr Box = (BoxType *) Arc;

	/* arcs with clearance never touch polys */
	if (TEST_FLAG(PCB_FLAG_CLEARPOLY, Polygon) && TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return false;
	if (!Polygon->Clipped)
		return false;
	if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
			&& Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
			&& Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat) {
		POLYAREA *ap;

		if (!(ap = ArcPoly(Arc, Arc->Thickness + Bloat)))
			return false;							/* error */
		return isects(ap, Polygon, true);
	}
	return false;
}

/* ---------------------------------------------------------------------------
 * checks if a line has a connection to a polygon
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 */
bool IsLineInPolygon(LineTypePtr Line, PolygonTypePtr Polygon)
{
	BoxTypePtr Box = (BoxType *) Line;
	POLYAREA *lp;

	/* lines with clearance never touch polygons */
	if (TEST_FLAG(PCB_FLAG_CLEARPOLY, Polygon) && TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return false;
	if (!Polygon->Clipped)
		return false;
	if (TEST_FLAG(PCB_FLAG_SQUARE, Line) && (Line->Point1.X == Line->Point2.X || Line->Point1.Y == Line->Point2.Y)) {
		Coord wid = (Line->Thickness + Bloat + 1) / 2;
		Coord x1, x2, y1, y2;

		x1 = MIN(Line->Point1.X, Line->Point2.X) - wid;
		y1 = MIN(Line->Point1.Y, Line->Point2.Y) - wid;
		x2 = MAX(Line->Point1.X, Line->Point2.X) + wid;
		y2 = MAX(Line->Point1.Y, Line->Point2.Y) + wid;
		return IsRectangleInPolygon(x1, y1, x2, y2, Polygon);
	}
	if (Box->X1 <= Polygon->Clipped->contours->xmax + Bloat
			&& Box->X2 >= Polygon->Clipped->contours->xmin - Bloat
			&& Box->Y1 <= Polygon->Clipped->contours->ymax + Bloat && Box->Y2 >= Polygon->Clipped->contours->ymin - Bloat) {
		if (!(lp = LinePoly(Line, Line->Thickness + Bloat)))
			return FALSE;							/* error */
		return isects(lp, Polygon, true);
	}
	return false;
}

/* ---------------------------------------------------------------------------
 * checks if a pad connects to a non-clearing polygon
 *
 * The polygon is assumed to already have been proven non-clearing
 */
bool IsPadInPolygon(PadTypePtr pad, PolygonTypePtr polygon)
{
	return IsLineInPolygon((LineTypePtr) pad, polygon);
}

/* ---------------------------------------------------------------------------
 * checks if a polygon has a connection to a second one
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 against the ones of P2
 */
bool IsPolygonInPolygon(PolygonTypePtr P1, PolygonTypePtr P2)
{
	if (!P1->Clipped || !P2->Clipped)
		return false;
	assert(P1->Clipped->contours);
	assert(P2->Clipped->contours);

	/* first check if both bounding boxes intersect. If not, return quickly */
	if (P1->Clipped->contours->xmin - Bloat > P2->Clipped->contours->xmax ||
			P1->Clipped->contours->xmax + Bloat < P2->Clipped->contours->xmin ||
			P1->Clipped->contours->ymin - Bloat > P2->Clipped->contours->ymax ||
			P1->Clipped->contours->ymax + Bloat < P2->Clipped->contours->ymin)
		return false;

	/* first check un-bloated case */
	if (isects(P1->Clipped, P2, false))
		return TRUE;

	/* now the difficult case of bloated */
	if (Bloat > 0) {
		PLINE *c;
		for (c = P1->Clipped->contours; c; c = c->next) {
			LineType line;
			VNODE *v = &c->head;
			if (c->xmin - Bloat <= P2->Clipped->contours->xmax &&
					c->xmax + Bloat >= P2->Clipped->contours->xmin &&
					c->ymin - Bloat <= P2->Clipped->contours->ymax && c->ymax + Bloat >= P2->Clipped->contours->ymin) {

				line.Point1.X = v->point[0];
				line.Point1.Y = v->point[1];
				line.Thickness = 2 * Bloat;
				line.Clearance = 0;
				line.Flags = NoFlags();
				for (v = v->next; v != &c->head; v = v->next) {
					line.Point2.X = v->point[0];
					line.Point2.Y = v->point[1];
					SetLineBoundingBox(&line);
					if (IsLineInPolygon(&line, P2))
						return (true);
					line.Point1.X = line.Point2.X;
					line.Point1.Y = line.Point2.Y;
				}
			}
		}
	}

	return (false);
}

/* ---------------------------------------------------------------------------
 * some of the 'pad' routines are the same as for lines because the 'pad'
 * struct starts with a line struct. See global.h for details
 */
bool LinePadIntersect(LineTypePtr Line, PadTypePtr Pad)
{
	return LineLineIntersect((Line), (LineTypePtr) Pad);
}

bool ArcPadIntersect(ArcTypePtr Arc, PadTypePtr Pad)
{
	return LineArcIntersect((LineTypePtr) (Pad), (Arc));
}

bool BoxBoxIntersection(BoxTypePtr b1, BoxTypePtr b2)
{
	if (b2->X2 < b1->X1 || b2->X1 > b1->X2)
		return false;
	if (b2->Y2 < b1->Y1 || b2->Y1 > b1->Y2)
		return false;
	return true;
}

static bool PadPadIntersect(PadTypePtr p1, PadTypePtr p2)
{
	return LinePadIntersect((LineTypePtr) p1, p2);
}

static inline bool PV_TOUCH_PV(PinTypePtr PV1, PinTypePtr PV2)
{
	double t1, t2;
	BoxType b1, b2;
	int shape1, shape2;

	shape1 = GET_SQUARE(PV1);
	shape2 = GET_SQUARE(PV2);

	if ((shape1 > 1) || (shape2 > 1)) {
		POLYAREA *pl1, *pl2;
		int ret;

		pl1 = PinPoly(PV1, PIN_SIZE(PV1) + Bloat, 0);
		pl2 = PinPoly(PV2, PIN_SIZE(PV2) + Bloat, 0);
		ret = Touching(pl1, pl2);
		poly_Free(&pl1);
		poly_Free(&pl2);
		return ret;
	}


	t1 = MAX(PV1->Thickness / 2.0 + Bloat, 0);
	t2 = MAX(PV2->Thickness / 2.0 + Bloat, 0);
	if (IsPointOnPin(PV1->X, PV1->Y, t1, PV2)
			|| IsPointOnPin(PV2->X, PV2->Y, t2, PV1))
		return true;
	if (!TEST_FLAG(PCB_FLAG_SQUARE, PV1) || !TEST_FLAG(PCB_FLAG_SQUARE, PV2))
		return false;
	/* check for square/square overlap */
	b1.X1 = PV1->X - t1;
	b1.X2 = PV1->X + t1;
	b1.Y1 = PV1->Y - t1;
	b1.Y2 = PV1->Y + t1;
	t2 = PV2->Thickness / 2.0;
	b2.X1 = PV2->X - t2;
	b2.X2 = PV2->X + t2;
	b2.Y1 = PV2->Y - t2;
	b2.Y2 = PV2->Y + t2;
	return BoxBoxIntersection(&b1, &b2);
}

bool PinLineIntersect(PinTypePtr PV, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_SQUARE, PV)) {
		int shape = GET_SQUARE(PV);
		if (shape <= 1) {
			/* the original square case */
			/* IsLineInRectangle already has Bloat factor */
			return IsLineInRectangle(PV->X - (PIN_SIZE(PV) + 1) / 2,
															 PV->Y - (PIN_SIZE(PV) + 1) / 2,
															 PV->X + (PIN_SIZE(PV) + 1) / 2, PV->Y + (PIN_SIZE(PV) + 1) / 2, Line);
		}

		{
			/* shaped pin case */
			POLYAREA *pl, *lp;
			int ret;

			pl = PinPoly(PV, PIN_SIZE(PV), 0);
			lp = LinePoly(Line, Line->Thickness + Bloat);
			ret = Touching(lp, pl);
			poly_Free(&pl);
			poly_Free(&lp);
			return ret;
		}

	}


	/* the original round pin version */
	return IsPointInPad(PV->X, PV->Y, MAX(PIN_SIZE(PV) / 2.0 + Bloat, 0.0), (PadTypePtr) Line);
}
