/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  2d drafting plugin: center line geometry
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

#include "config.h"
#include "centgeo.h"

#include "obj_line.h"
#include "obj_arc.h"
#include <librnd/core/math_helper.h>
#include <librnd/core/compat_misc.h>
#include "search.h"

/* Note about all intersection code: same basic algo as in find_geo.c -
   see comment for the algo description there */

int pcb_intersect_cline_cline(pcb_line_t *Line1, pcb_line_t *Line2, rnd_box_t *ip, double offs[2])
{
	double s, r;
	double line1_dx, line1_dy, line2_dx, line2_dy, point1_dx, point1_dy;

	/* setup some constants */
	line1_dx = Line1->Point2.X - Line1->Point1.X;
	line1_dy = Line1->Point2.Y - Line1->Point1.Y;
	line2_dx = Line2->Point2.X - Line2->Point1.X;
	line2_dy = Line2->Point2.Y - Line2->Point1.Y;
	point1_dx = Line1->Point1.X - Line2->Point1.X;
	point1_dy = Line1->Point1.Y - Line2->Point1.Y;

	/* set s to cross product of Line1 and the line
	 *   Line1.Point1--Line2.Point1 (as vectors) */
	s = point1_dy * line1_dx - point1_dx * line1_dy;

	/* set r to cross product of both lines (as vectors) */
	r = line1_dx * line2_dy - line1_dy * line2_dx;

	/* No cross product means parallel or overlapping lines */
	if (r == 0.0) {
		if (s == 0.0) { /* overlap; r and [0] will be Line2's point offset within Line1; s and [1] will be an endpoint of line1 */

			r = pcb_cline_pt_offs(Line1, Line2->Point1.X, Line2->Point1.Y);
			if ((r < 0.0) || (r > 1.0))
				r = pcb_cline_pt_offs(Line1, Line2->Point2.X, Line2->Point2.Y);

			s = pcb_cline_pt_offs(Line2, Line1->Point1.X, Line1->Point1.Y);
			if ((s < 0.0) || (s > 1.0))
				s = 0.0;
			else
				s = 1.0;

			if (ip != NULL) {
				ip->X1 = rnd_round((double)Line1->Point1.X + r * line1_dx);
				ip->Y1 = rnd_round((double)Line1->Point1.Y + r * line1_dy);
				if (s == 1.0) {
					ip->X2 = Line1->Point2.X;
					ip->Y2 = Line1->Point2.Y;
				}
				else {
					ip->X2 = Line1->Point1.X;
					ip->Y2 = Line1->Point1.Y;
				}
			}
			if (offs != NULL) {
				offs[0] = r;
				offs[1] = s;
			}
			return 2;
		}
		return 0;
	}

	s /= r;
	r = (point1_dy * line2_dx - point1_dx * line2_dy) / r;

	/* intersection is at least on AB */
	if (r >= 0.0 && r <= 1.0) {
		if (s >= 0.0 && s <= 1.0) {
			if (ip != NULL) {
				ip->X1 = rnd_round((double)Line1->Point1.X + r * line1_dx);
				ip->Y1 = rnd_round((double)Line1->Point1.Y + r * line1_dy);
			}
			if (offs != NULL)
				offs[0] = r;
			return 1;
		}
		return 0;
	}

	/* intersection is at least on CD */
	/* [removed this case since it always returns pcb_false --asp] */
	return 0;
}

void pcb_cline_offs(pcb_line_t *line, double offs, rnd_coord_t *dstx, rnd_coord_t *dsty)
{
	double line_dx, line_dy;

	line_dx = line->Point2.X - line->Point1.X;
	line_dy = line->Point2.Y - line->Point1.Y;

	*dstx = rnd_round((double)line->Point1.X + offs * line_dx);
	*dsty = rnd_round((double)line->Point1.Y + offs * line_dy);
}

double pcb_cline_pt_offs(pcb_line_t *line, rnd_coord_t px, rnd_coord_t py)
{
	double line_dx, line_dy, pt_dx, pt_dy;

	line_dx = line->Point2.X - line->Point1.X;
	line_dy = line->Point2.Y - line->Point1.Y;

	pt_dx = px - line->Point1.X;
	pt_dy = py - line->Point1.Y;

	return (line_dx * pt_dx + line_dy * pt_dy) / (line_dx*line_dx + line_dy*line_dy);
}

static int line_ep(pcb_line_t *line, rnd_coord_t x, rnd_coord_t y)
{
	if ((line->Point1.X == x) && (line->Point1.Y == y)) return 1;
	if ((line->Point2.X == x) && (line->Point2.Y == y)) return 1;
	return 0;
}

/* Assumme there will be at two intersection points; load ofs/ix/iy in the
   next slot of the result, return from the function if both intersections
   found */
#define append(ofs, ix, iy) \
do { \
	if (ip != NULL) { \
		if (found == 0) { \
			ip->X1 = ix; \
			ip->Y1 = iy; \
		} \
		else { \
			ip->X2 = ix; \
			ip->Y2 = iy; \
		} \
	} \
	if (offs != NULL) \
		offs[found] = ofs; \
	found++; \
	if (found == 2) \
		return found; \
} while(0)

static int intersect_cline_carc(pcb_line_t *Line, pcb_arc_t *Arc, rnd_box_t *ip, double offs[2], int oline)
{
	double dx, dy, dx1, dy1, l, d, r, r2, Radius;
	rnd_coord_t ex, ey, ix, iy;
	int found = 0;

	dx = Line->Point2.X - Line->Point1.X;
	dy = Line->Point2.Y - Line->Point1.Y;
	dx1 = Line->Point1.X - Arc->X;
	dy1 = Line->Point1.Y - Arc->Y;
	l = dx * dx + dy * dy;
	d = dx * dy1 - dy * dx1;
	d *= d;

	/* use the larger diameter circle first */
	Radius = Arc->Width;
	Radius *= Radius;
	r2 = Radius * l - d;
	/* projection doesn't even intersect circle when r2 < 0 */
	if (r2 < 0)
		return 0;

	/* line ends on arc? */
	if (pcb_is_point_on_arc(Line->Point1.X, Line->Point1.Y, 0, Arc)) {
		if (oline)
			append(0, Line->Point1.X, Line->Point1.Y);
		else
			append(pcb_carc_pt_offs(Arc, Line->Point1.X, Line->Point1.Y), Line->Point1.X, Line->Point1.Y);
	}
	if (pcb_is_point_on_arc(Line->Point2.X, Line->Point2.Y, 0, Arc)) {
		if (oline)
			append(1, Line->Point2.X, Line->Point2.Y);
		else
			append(pcb_carc_pt_offs(Arc, Line->Point2.X, Line->Point2.Y), Line->Point2.X, Line->Point2.Y);
	}

	/* if line is a single point, there is no other way an intersection can happen */
	if (l == 0.0)
		return found;

	r2 = sqrt(r2);
	Radius = -(dx * dx1 + dy * dy1);
	r = (Radius + r2) / l;

	if ((r >= 0) && (r <= 1)) {
		ix = rnd_round(Line->Point1.X + r * dx);
		iy = rnd_round(Line->Point1.Y + r * dy);
		if (!line_ep(Line, ix, iy) && pcb_is_point_on_arc(ix, iy, 1, Arc)) {
			if (oline)
				append(r, ix, iy);
			else
				append(pcb_carc_pt_offs(Arc, ix, iy), ix, iy);
		}
	}

	r = (Radius - r2) / l;
	if ((r >= 0) && (r <= 1)) {
		ix = rnd_round(Line->Point1.X + r * dx);
		iy = rnd_round(Line->Point1.Y + r * dy);
		if (!line_ep(Line, ix, iy) && pcb_is_point_on_arc(ix, iy, 1, Arc)) {
			if (oline)
				append(r, ix, iy);
			else
				append(pcb_carc_pt_offs(Arc, ix, iy), ix, iy);
		}
	}

	/* check if an arc end point is on the line */
	pcb_arc_get_end(Arc, 0, &ex, &ey);
	if (pcb_is_point_in_line(ex, ey, 1, (pcb_any_line_t *) Line)) {
		if (oline) {
			r = pcb_cline_pt_offs(Line, ex, ey);
			append(r, ex, ey);
		}
		else
			append(0, ex, ey);
	}
	pcb_arc_get_end(Arc, 1, &ex, &ey);
	if (pcb_is_point_in_line(ex, ey, 1, (pcb_any_line_t *) Line)) {
		if (oline) {
			r = pcb_cline_pt_offs(Line, ex, ey);
			append(r, ex, ey);
		}
		else
			append(1, ex, ey);
	}

	return found;
}

int pcb_intersect_cline_carc(pcb_line_t *Line, pcb_arc_t *Arc, rnd_box_t *ip, double offs[2])
{
	return intersect_cline_carc(Line, Arc, ip, offs, 1);
}

int pcb_intersect_carc_cline(pcb_arc_t *Arc, pcb_line_t *Line, rnd_box_t *ip, double offs[2])
{
	return intersect_cline_carc(Line, Arc, ip, offs, 0);
}


void pcb_carc_offs(pcb_arc_t *arc, double offs, rnd_coord_t *dstx, rnd_coord_t *dsty)
{
	double ang = (arc->StartAngle + offs * arc->Delta) / PCB_RAD_TO_DEG;

	*dstx = arc->X + cos(ang) * arc->Width;
	*dsty = arc->Y - sin(ang) * arc->Height;
}

double pcb_carc_pt_offs(pcb_arc_t *arc, rnd_coord_t px, rnd_coord_t py)
{
	double dx, dy, ang, end;

	/* won't work with elliptical arc - see also pcb_is_point_on_arc */
	dy = (double)(py - arc->Y) / (double)arc->Height;
	dx = (double)(px - arc->X) / (double)arc->Width;
	ang = (-atan2(dy, dx)) * PCB_RAD_TO_DEG + 180;
	end = arc->StartAngle + arc->Delta;

	/* normalize the angle: there are multiple ways an arc can cover the same
	   angle range, e.g.  -10+30 and +20-30 - make sure the angle falls between
	   the start/end (generic) range */
	if (arc->StartAngle < end) {
		if (ang > end)
			ang -= 360;
		if (ang < arc->StartAngle)
			ang += 360;
	}
	else {
		if (ang > arc->StartAngle)
			ang -= 360;
		if (ang < end)
			ang += 360;
	}

	ang = (ang - arc->StartAngle) / arc->Delta;

	return ang;
}


static void get_arc_ends(rnd_coord_t *box, pcb_arc_t *arc)
{
	box[0] = arc->X - arc->Width * cos(PCB_M180 * arc->StartAngle);
	box[1] = arc->Y + arc->Height * sin(PCB_M180 * arc->StartAngle);
	box[2] = arc->X - arc->Width * cos(PCB_M180 * (arc->StartAngle + arc->Delta));
	box[3] = arc->Y + arc->Height * sin(PCB_M180 * (arc->StartAngle + arc->Delta));
}

/* reduce arc start angle and delta to 0..360 */
static void normalize_angles(pcb_angle_t *sa, pcb_angle_t *d)
{
	if (*d < 0) {
		*sa += *d;
		*d = -*d;
	}
	if (*d > 360) /* full circle */
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

int pcb_intersect_carc_carc(pcb_arc_t *Arc1, pcb_arc_t *Arc2, rnd_box_t *ip, double offs[2])
{
	double x, y, dx, dy, r1, r2, a, d, l, dl;
	rnd_coord_t pdx, pdy;
	rnd_coord_t box[8];
	int found = 0;

	/* try the end points first */
	get_arc_ends(&box[0], Arc1);
	get_arc_ends(&box[4], Arc2);
	if (pcb_is_point_on_arc(box[0], box[1], 1, Arc2)) append(0, box[0], box[1]);
	if (pcb_is_point_on_arc(box[2], box[3], 1, Arc2)) append(1, box[2], box[3]);
	if (pcb_is_point_on_arc(box[4], box[5], 1, Arc1)) append(pcb_carc_pt_offs(Arc1, box[4], box[5]), box[4], box[5]);
	if (pcb_is_point_on_arc(box[6], box[7], 1, Arc1)) append(pcb_carc_pt_offs(Arc1, box[6], box[7]), box[6], box[7]);

	pdx = Arc2->X - Arc1->X;
	pdy = Arc2->Y - Arc1->Y;
	dl = rnd_distance(Arc1->X, Arc1->Y, Arc2->X, Arc2->Y);

	/* the original code used to do angle checks for concentric case here but
	   those cases are already handled by the above endpoint checks. */

	r1 = Arc1->Width;
	r2 = Arc2->Width;
	/* arcs centerlines are too far or too near */
	if (dl > r1 + r2 || dl + r1 < r2 || dl + r2 < r1) {
		/* check the nearest to the other arc's center point */
		dx = pdx * r1 / dl;
		dy = pdy * r1 / dl;
		if (dl + r1 < r2) { /* Arc1 inside Arc2 */
			dx = -dx;
			dy = -dy;
		}

		if (radius_crosses_arc(Arc1->X + dx, Arc1->Y + dy, Arc1) && pcb_is_point_on_arc(Arc1->X + dx, Arc1->Y + dy, 1, Arc2))
			append(pcb_carc_pt_offs(Arc1, Arc1->X + dx, Arc1->Y + dy), Arc1->X + dx, Arc1->Y + dy);

		dx = -pdx * r2 / dl;
		dy = -pdy * r2 / dl;
		if (dl + r2 < r1) { /* Arc2 inside Arc1 */
			dx = -dx;
			dy = -dy;
		}

		if (radius_crosses_arc(Arc2->X + dx, Arc2->Y + dy, Arc2) && pcb_is_point_on_arc(Arc2->X + dx, Arc2->Y + dy, 1, Arc1))
			append(pcb_carc_pt_offs(Arc1, Arc2->X + dx, Arc2->Y + dy), Arc2->X + dx, Arc2->Y + dy);
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
	if ((radius_crosses_arc(x + dy, y - dx, Arc1) && pcb_is_point_on_arc(x + dy, y - dx, 1, Arc2)) || (radius_crosses_arc(x + dy, y - dx, Arc2) && pcb_is_point_on_arc(x + dy, y - dx, 1, Arc1)))
		append(pcb_carc_pt_offs(Arc1, x + dy, y - dx), x + dy, y - dx);

	if ((radius_crosses_arc(x - dy, y + dx, Arc1) && pcb_is_point_on_arc(x - dy, y + dx, 1, Arc2)) || (radius_crosses_arc(x - dy, y + dx, Arc2) && pcb_is_point_on_arc(x - dy, y + dx, 1, Arc1)))
		append(pcb_carc_pt_offs(Arc1, x - dy, y + dx), x - dy, y + dx);

	return found;
}

