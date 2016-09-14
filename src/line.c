/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include "conf_core.h"

#include <math.h>
#include <setjmp.h>
#include <stdlib.h>

#include "data.h"
#include "crosshair.h"
#include "find.h"
#include "line.h"
#include "misc.h"
#include "rtree.h"
#include "layer.h"

static double drc_lines(PointTypePtr end, bool way);

/* ---------------------------------------------------------------------------
 * Adjust the attached line to 45 degrees if necessary
 */
void AdjustAttachedLine(void)
{
	AttachedLineTypePtr line = &Crosshair.AttachedLine;

	/* I need at least one point */
	if (line->State == STATE_FIRST)
		return;
	/* don't draw outline when ctrl key is pressed */
	if (conf_core.editor.mode == PCB_MODE_LINE && gui->control_is_pressed()) {
		line->draw = false;
		return;
	}
	else
		line->draw = true;
	/* no 45 degree lines required */
	if (PCB->RatDraw || conf_core.editor.all_direction_lines) {
		line->Point2.X = Crosshair.X;
		line->Point2.Y = Crosshair.Y;
		return;
	}
	FortyFiveLine(line);
}

/* ---------------------------------------------------------------------------
 * makes the attached line fit into a 45 degree direction
 *
 * directions:
 *
 *           0
 *          7 1
 *         6   2
 *          5 3
 *           4
 */
void FortyFiveLine(AttachedLineTypePtr Line)
{
	Coord dx, dy, min;
	unsigned direction = 0;
	double m;

	/* first calculate direction of line */
	dx = Crosshair.X - Line->Point1.X;
	dy = Crosshair.Y - Line->Point1.Y;

	if (!dx) {
		if (!dy)
			/* zero length line, don't draw anything */
			return;
		else
			direction = dy > 0 ? 0 : 4;
	}
	else {
		m = (double) dy / dx;
		direction = 2;
		if (m > PCB_TAN_30_DEGREE)
			direction = m > PCB_TAN_60_DEGREE ? 0 : 1;
		else if (m < -PCB_TAN_30_DEGREE)
			direction = m < -PCB_TAN_60_DEGREE ? 0 : 3;
	}
	if (dx < 0)
		direction += 4;

	dx = coord_abs(dx);
	dy = coord_abs(dy);
	min = MIN(dx, dy);

	/* now set up the second pair of coordinates */
	switch (direction) {
	case 0:
	case 4:
		Line->Point2.X = Line->Point1.X;
		Line->Point2.Y = Crosshair.Y;
		break;

	case 2:
	case 6:
		Line->Point2.X = Crosshair.X;
		Line->Point2.Y = Line->Point1.Y;
		break;

	case 1:
		Line->Point2.X = Line->Point1.X + min;
		Line->Point2.Y = Line->Point1.Y + min;
		break;

	case 3:
		Line->Point2.X = Line->Point1.X + min;
		Line->Point2.Y = Line->Point1.Y - min;
		break;

	case 5:
		Line->Point2.X = Line->Point1.X - min;
		Line->Point2.Y = Line->Point1.Y - min;
		break;

	case 7:
		Line->Point2.X = Line->Point1.X - min;
		Line->Point2.Y = Line->Point1.Y + min;
		break;
	}
}

/* ---------------------------------------------------------------------------
 *  adjusts the insert lines to make them 45 degrees as necessary
 */
void AdjustTwoLine(bool way)
{
	Coord dx, dy;
	AttachedLineTypePtr line = &Crosshair.AttachedLine;

	if (Crosshair.AttachedLine.State == STATE_FIRST)
		return;
	/* don't draw outline when ctrl key is pressed */
	if (gui->control_is_pressed()) {
		line->draw = false;
		return;
	}
	else
		line->draw = true;
	if (conf_core.editor.all_direction_lines) {
		line->Point2.X = Crosshair.X;
		line->Point2.Y = Crosshair.Y;
		return;
	}
	/* swap the modes if shift is held down */
	if (gui->shift_is_pressed())
		way = !way;
	dx = Crosshair.X - line->Point1.X;
	dy = Crosshair.Y - line->Point1.Y;
	if (!way) {
		if (coord_abs(dx) > coord_abs(dy)) {
			line->Point2.X = Crosshair.X - SGN(dx) * coord_abs(dy);
			line->Point2.Y = line->Point1.Y;
		}
		else {
			line->Point2.X = line->Point1.X;
			line->Point2.Y = Crosshair.Y - SGN(dy) * coord_abs(dx);
		}
	}
	else {
		if (coord_abs(dx) > coord_abs(dy)) {
			line->Point2.X = line->Point1.X + SGN(dx) * coord_abs(dy);
			line->Point2.Y = Crosshair.Y;
		}
		else {
			line->Point2.X = Crosshair.X;
			line->Point2.Y = line->Point1.Y + SGN(dy) * coord_abs(dx);;
		}
	}
}

struct drc_info {
	LineTypePtr line;
	bool solder;
	jmp_buf env;
};

static r_dir_t drcVia_callback(const BoxType * b, void *cl)
{
	PinTypePtr via = (PinTypePtr) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (!TEST_FLAG(PCB_FLAG_FOUND, via) && PinLineIntersect(via, i->line))
		longjmp(i->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t drcPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (TEST_FLAG(PCB_FLAG_ONSOLDER, pad) == i->solder && !TEST_FLAG(PCB_FLAG_FOUND, pad) && LinePadIntersect(i->line, pad))
		longjmp(i->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t drcLine_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (!TEST_FLAG(PCB_FLAG_FOUND, line) && LineLineIntersect(line, i->line))
		longjmp(i->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t drcArc_callback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (!TEST_FLAG(PCB_FLAG_FOUND, arc) && LineArcIntersect(i->line, arc))
		longjmp(i->env, 1);
	return R_DIR_FOUND_CONTINUE;
}

/* drc_lines() checks for intersectors against two lines and
 * adjusts the end point until there is no intersection or
 * it winds up back at the start. If way is false it checks
 * an ortho start line with one 45 refraction to reach the endpoint,
 * otherwise it checks a 45 start, with a ortho refraction to reach endpoint
 *
 * It returns the straight-line length of the best answer, and
 * changes the position of the input end point to the best answer.
 */

static double drc_lines(PointTypePtr end, bool way)
{
	double f, s, f2, s2, len, best;
	Coord dx, dy, temp, last, length;
	Coord temp2, last2, length2;
	LineType line1, line2;
	Cardinal group, comp;
	struct drc_info info;
	bool two_lines, x_is_long, blocker;
	PointType ans;

	f = 1.0;
	s = 0.5;
	last = -1;
	line1.Flags = line2.Flags = NoFlags();
	line1.Thickness = conf_core.design.line_thickness + 2 * (PCB->Bloat + 1);
	line2.Thickness = line1.Thickness;
	line1.Clearance = line2.Clearance = 0;
	line1.Point1.X = Crosshair.AttachedLine.Point1.X;
	line1.Point1.Y = Crosshair.AttachedLine.Point1.Y;
	dy = end->Y - line1.Point1.Y;
	dx = end->X - line1.Point1.X;
	if (coord_abs(dx) > coord_abs(dy)) {
		x_is_long = true;
		length = coord_abs(dx);
	}
	else {
		x_is_long = false;
		length = coord_abs(dy);
	}
	group = GetGroupOfLayer(INDEXOFCURRENT);
	comp = max_group + 10;				/* this out-of-range group might save a call */
	if (GetLayerGroupNumberByNumber(solder_silk_layer) == group)
		info.solder = true;
	else {
		info.solder = false;
		comp = GetLayerGroupNumberByNumber(component_silk_layer);
	}
	temp = length;
	/* assume the worst */
	best = 0.0;
	ans.X = line1.Point1.X;
	ans.Y = line1.Point1.Y;
	while (length != last) {
		last = length;
		if (x_is_long) {
			dx = SGN(dx) * length;
			dy = end->Y - line1.Point1.Y;
			length2 = coord_abs(dy);
		}
		else {
			dy = SGN(dy) * length;
			dx = end->X - line1.Point1.X;
			length2 = coord_abs(dx);
		}
		temp2 = length2;
		f2 = 1.0;
		s2 = 0.5;
		last2 = -1;
		blocker = true;
		while (length2 != last2) {
			if (x_is_long)
				dy = SGN(dy) * length2;
			else
				dx = SGN(dx) * length2;
			two_lines = true;
			if (coord_abs(dx) > coord_abs(dy) && x_is_long) {
				line1.Point2.X = line1.Point1.X + (way ? SGN(dx) * coord_abs(dy) : dx - SGN(dx) * coord_abs(dy));
				line1.Point2.Y = line1.Point1.Y + (way ? dy : 0);
			}
			else if (coord_abs(dy) >= coord_abs(dx) && !x_is_long) {
				line1.Point2.X = line1.Point1.X + (way ? dx : 0);
				line1.Point2.Y = line1.Point1.Y + (way ? SGN(dy) * coord_abs(dx) : dy - SGN(dy) * coord_abs(dx));
			}
			else if (x_is_long) {
				/* we've changed which axis is long, so only do one line */
				line1.Point2.X = line1.Point1.X + dx;
				line1.Point2.Y = line1.Point1.Y + (way ? SGN(dy) * coord_abs(dx) : 0);
				two_lines = false;
			}
			else {
				/* we've changed which axis is long, so only do one line */
				line1.Point2.Y = line1.Point1.Y + dy;
				line1.Point2.X = line1.Point1.X + (way ? SGN(dx) * coord_abs(dy) : 0);
				two_lines = false;
			}
			line2.Point1.X = line1.Point2.X;
			line2.Point1.Y = line1.Point2.Y;
			if (!two_lines) {
				line2.Point2.Y = line1.Point2.Y;
				line2.Point2.X = line1.Point2.X;
			}
			else {
				line2.Point2.X = line1.Point1.X + dx;
				line2.Point2.Y = line1.Point1.Y + dy;
			}
			SetLineBoundingBox(&line1);
			SetLineBoundingBox(&line2);
			last2 = length2;
			if (setjmp(info.env) == 0) {
				info.line = &line1;
				r_search(PCB->Data->via_tree, &line1.BoundingBox, NULL, drcVia_callback, &info, NULL);
				r_search(PCB->Data->pin_tree, &line1.BoundingBox, NULL, drcVia_callback, &info, NULL);
				if (info.solder || comp == group)
					r_search(PCB->Data->pad_tree, &line1.BoundingBox, NULL, drcPad_callback, &info, NULL);
				if (two_lines) {
					info.line = &line2;
					r_search(PCB->Data->via_tree, &line2.BoundingBox, NULL, drcVia_callback, &info, NULL);
					r_search(PCB->Data->pin_tree, &line2.BoundingBox, NULL, drcVia_callback, &info, NULL);
					if (info.solder || comp == group)
						r_search(PCB->Data->pad_tree, &line2.BoundingBox, NULL, drcPad_callback, &info, NULL);
				}
				GROUP_LOOP(PCB->Data, group);
				{
					info.line = &line1;
					r_search(layer->line_tree, &line1.BoundingBox, NULL, drcLine_callback, &info, NULL);
					r_search(layer->arc_tree, &line1.BoundingBox, NULL, drcArc_callback, &info, NULL);
					if (two_lines) {
						info.line = &line2;
						r_search(layer->line_tree, &line2.BoundingBox, NULL, drcLine_callback, &info, NULL);
						r_search(layer->arc_tree, &line2.BoundingBox, NULL, drcArc_callback, &info, NULL);
					}
				}
				END_LOOP;
				/* no intersector! */
				blocker = false;
				f2 += s2;
				len = (line2.Point2.X - line1.Point1.X);
				len *= len;
				len += (double) (line2.Point2.Y - line1.Point1.Y) * (line2.Point2.Y - line1.Point1.Y);
				if (len > best) {
					best = len;
					ans.X = line2.Point2.X;
					ans.Y = line2.Point2.Y;
				}
#if 0
				if (f2 > 1.0)
					f2 = 0.5;
#endif
			}
			else {
				/* bumped into something, back off */
				f2 -= s2;
			}
			s2 *= 0.5;
			length2 = MIN(f2 * temp2, temp2);
		}
		if (!blocker && ((x_is_long && line2.Point2.X - line1.Point1.X == dx)
										 || (!x_is_long && line2.Point2.Y - line1.Point1.Y == dy)))
			f += s;
		else
			f -= s;
		s *= 0.5;
		length = MIN(f * temp, temp);
	}

	end->X = ans.X;
	end->Y = ans.Y;
	return best;
}

void EnforceLineDRC(void)
{
	PointType r45, rs;
	bool shift;
	double r1, r2;
	int refraction = conf_core.editor.line_refraction; 

	/* Silence a bogus compiler warning by storing this in a variable */
	int layer_idx = INDEXOFCURRENT;

	if (gui->mod1_is_pressed() || gui->control_is_pressed() || PCB->RatDraw || layer_idx >= max_copper_layer)
		return;

	rs.X = r45.X = Crosshair.X;
	rs.Y = r45.Y = Crosshair.Y;
	/* first try starting straight */
	r1 = drc_lines(&rs, false);
	/* then try starting at 45 */
	r2 = drc_lines(&r45, true);
	/* shift<Key> forces the line lookahead path to refract the alternate way */
	shift = gui->shift_is_pressed();
	if (XOR(r1 > r2, shift)) {
		if (conf_core.editor.line_refraction != 0) {
			if (shift) {
				if (conf_core.editor.line_refraction==1) {
					conf_setf(CFR_DESIGN, "editor/line_refraction", -1, "%d", 2);
					printf("line_refraction is set to: %d\n", conf_core.editor.line_refraction);
					}
			}
			else{
				conf_setf(CFR_DESIGN, "editor/line_refraction", -1, "%d", 1);
				printf("line_refraction is set to: %d\n", conf_core.editor.line_refraction);
			}
		}
		Crosshair.X = rs.X;
		Crosshair.Y = rs.Y;
	}
	else {
		if (conf_core.editor.line_refraction !=0) {
			if (shift) {
				if (conf_core.editor.line_refraction==2) {
				conf_setf(CFR_DESIGN, "editor/line_refraction", -1, "%d", 1);
				printf("line_refraction is set to: %d\n", conf_core.editor.line_refraction);
				}
			}
			else{
				conf_setf(CFR_DESIGN, "editor/line_refraction", -1, "%d", 2);
				printf("line_refraction is set to: %d\n", conf_core.editor.line_refraction);
			}
		}
		Crosshair.X = r45.X;
		Crosshair.Y = r45.Y;
	}
}
