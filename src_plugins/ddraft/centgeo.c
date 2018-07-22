/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin: center line geometry
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

#include "config.h"
#include "centgeo.h"

#include "obj_line.h"
#include "compat_misc.h"

/* Same basic algo as in find_geo.c - see comment for the algo
   description there */
int pcb_intersect_cline_cline(pcb_line_t *Line1, pcb_line_t *Line2, pcb_box_t *ip, double offs[2])
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
				ip->X1 = pcb_round((double)Line1->Point1.X + r * line1_dx);
				ip->Y1 = pcb_round((double)Line1->Point1.Y + r * line1_dy);
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
				ip->X1 = pcb_round((double)Line1->Point1.X + r * line1_dx);
				ip->Y1 = pcb_round((double)Line1->Point1.Y + r * line1_dy);
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

void pcb_cline_offs(pcb_line_t *line, double offs, pcb_coord_t *dstx, pcb_coord_t *dsty)
{
	double line_dx, line_dy;

	line_dx = line->Point2.X - line->Point1.X;
	line_dy = line->Point2.Y - line->Point1.Y;

	*dstx = pcb_round((double)line->Point1.X + offs * line_dx);
	*dsty = pcb_round((double)line->Point1.Y + offs * line_dy);
}

double pcb_cline_pt_offs(pcb_line_t *line, pcb_coord_t px, pcb_coord_t py)
{
	double line_dx, line_dy, pt_dx, pt_dy;

	line_dx = line->Point2.X - line->Point1.X;
	line_dy = line->Point2.Y - line->Point1.Y;

	pt_dx = px - line->Point1.X;
	pt_dy = py - line->Point1.Y;

	return (line_dx * pt_dx + line_dy * pt_dy) / (line_dx*line_dx + line_dy*line_dy);
}

#if 0
pcb_bool pcb_intersect_cline_carc(pcb_line_t *Line, pcb_arc_t *Arc)
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
#endif
