/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include "conf_core.h"
#include <librnd/core/math_helper.h>

#include <setjmp.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "find.h"
#include <librnd/poly/rtree.h>

void pcb_line_adjust_attached(void)
{
	pcb_attached_line_t *line = &pcb_crosshair.AttachedLine;
	int flags = conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0;

	if (conf_core.editor.auto_drc && (pcb_layer_flags_(PCB_CURRLAYER(PCB)) & PCB_LYT_COPPER))
		flags |= PCB_FLAG_FOUND;

	/* I need at least one point */
	if (line->State == PCB_CH_STATE_FIRST)
		return;

	line->Point2.X = pcb_crosshair.X;
	line->Point2.Y = pcb_crosshair.Y;

	pcb_route_calculate(PCB,
		&pcb_crosshair.Route, &line->Point1, &line->Point2,
		pcb_layer_id(PCB->Data, PCB_CURRLAYER(PCB)),
		conf_core.design.line_thickness, conf_core.design.clearance * 2,
		pcb_flag_make(flags), rnd_gui->shift_is_pressed(rnd_gui),
		rnd_gui->control_is_pressed(rnd_gui));
}

/* directions:
 *
 *           0
 *          7 1
 *         6   2
 *          5 3
 *           4
 */
void pcb_line_45(pcb_attached_line_t *Line)
{
	rnd_coord_t dx, dy, min;
	unsigned direction = 0;
	double m;

	/* first calculate direction of line */
	dx = pcb_crosshair.X - Line->Point1.X;
	dy = pcb_crosshair.Y - Line->Point1.Y;

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
		if (m > RND_TAN_30_DEGREE)
			direction = m > RND_TAN_60_DEGREE ? 0 : 1;
		else if (m < -RND_TAN_30_DEGREE)
			direction = m < -RND_TAN_60_DEGREE ? 0 : 3;
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
			Line->Point2.Y = pcb_crosshair.Y;
			break;

		case 2:
		case 6:
			Line->Point2.X = pcb_crosshair.X;
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

void pcb_line_adjust_attached_2lines(rnd_bool way)
{
	rnd_coord_t dx, dy;
	pcb_attached_line_t *line = &pcb_crosshair.AttachedLine;

	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_FIRST)
		return;

	/* don't draw outline when ctrl key is pressed */
	if (rnd_gui->control_is_pressed(rnd_gui)) {
		line->draw = rnd_false;
		return;
	}
	else
		line->draw = rnd_true;

	if (conf_core.editor.all_direction_lines) {
		line->Point2.X = pcb_crosshair.X;
		line->Point2.Y = pcb_crosshair.Y;
		return;
	}

	/* swap the modes if shift is held down */
	if (rnd_gui->shift_is_pressed(rnd_gui))
		way = !way;

	dx = pcb_crosshair.X - line->Point1.X;
	dy = pcb_crosshair.Y - line->Point1.Y;

	if (!way) {
		if (coord_abs(dx) > coord_abs(dy)) {
			line->Point2.X = pcb_crosshair.X - SGN(dx) * coord_abs(dy);
			line->Point2.Y = line->Point1.Y;
		}
		else {
			line->Point2.X = line->Point1.X;
			line->Point2.Y = pcb_crosshair.Y - SGN(dy) * coord_abs(dx);
		}
	}
	else {
		if (coord_abs(dx) > coord_abs(dy)) {
			line->Point2.X = line->Point1.X + SGN(dx) * coord_abs(dy);
			line->Point2.Y = pcb_crosshair.Y;
		}
		else {
			line->Point2.X = pcb_crosshair.X;
			line->Point2.Y = line->Point1.Y + SGN(dy) * coord_abs(dx);
		}
	}
}

struct drc_info {
	pcb_line_t *line;
	rnd_bool solder;
	jmp_buf env;
};

static rnd_r_dir_t drcPstk_callback(const rnd_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	struct drc_info *i = (struct drc_info *)cl;

	if (!PCB_FLAG_TEST(PCB_FLAG_FOUND, ps) && pcb_isc_pstk_line(pcb_find0, ps, i->line, rnd_false))
		longjmp(i->env, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_r_dir_t drcLine_callback(const rnd_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (!PCB_FLAG_TEST(PCB_FLAG_FOUND, line) && pcb_isc_line_line(pcb_find0, line, i->line))
		longjmp(i->env, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_r_dir_t drcArc_callback(const rnd_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct drc_info *i = (struct drc_info *) cl;

	if (!PCB_FLAG_TEST(PCB_FLAG_FOUND, arc) && pcb_isc_line_arc(pcb_find0, i->line, arc))
		longjmp(i->env, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

double pcb_drc_lines(pcb_board_t *pcb, const rnd_point_t *start, rnd_point_t *end, rnd_point_t *mid_out, rnd_bool way, rnd_bool optimize, rnd_coord_t tot_thick)
{
	double f, s, f2, s2, len, best;
	rnd_coord_t dx, dy, temp, last, length;
	rnd_coord_t temp2, last2, length2;
	pcb_line_t line1, line2;
	rnd_layergrp_id_t group, comp;
	struct drc_info info;
	rnd_bool two_lines, x_is_long, blocker;
	rnd_point_t ans;

	f = 1.0;
	s = 0.5;
	last = -1;
	line1.type = line2.type = PCB_OBJ_LINE;
	line1.Flags = line2.Flags = pcb_no_flags();
	line1.parent_type = line2.parent_type = PCB_PARENT_LAYER;
	line1.parent.layer = line2.parent.layer = PCB_CURRLAYER(PCB);
	line1.Thickness = tot_thick;
	line2.Thickness = line1.Thickness;
	line1.Clearance = line2.Clearance = 0;
	line1.Point1.X = start->X;
	line1.Point1.Y = start->Y;
	dy = end->Y - line1.Point1.Y;
	dx = end->X - line1.Point1.X;

	if (coord_abs(dx) > coord_abs(dy)) {
		x_is_long = rnd_true;
		length = coord_abs(dx);
	}
	else {
		x_is_long = rnd_false;
		length = coord_abs(dy);
	}

	group = pcb_layer_get_group(PCB, PCB_CURRLID(pcb));
	comp = PCB->LayerGroups.len + 10; /* this out-of-range group might save a call */

	if (pcb_layergrp_flags(PCB, group) & PCB_LYT_BOTTOM)
		info.solder = rnd_true;
	else {
		info.solder = rnd_false;
		pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, &comp, 1);
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
		blocker = rnd_true;
		while (length2 != last2) {
			if (x_is_long)
				dy = SGN(dy) * length2;
			else
				dx = SGN(dx) * length2;
			two_lines = rnd_true;
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
				two_lines = rnd_false;
			}
			else {
				/* we've changed which axis is long, so only do one line */
				line1.Point2.Y = line1.Point1.Y + dy;
				line1.Point2.X = line1.Point1.X + (way ? SGN(dx) * coord_abs(dy) : 0);
				two_lines = rnd_false;
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
			pcb_line_bbox(&line1);
			pcb_line_bbox(&line2);
			last2 = length2;
			if (setjmp(info.env) == 0) {
				info.line = &line1;
				rnd_r_search(PCB->Data->padstack_tree, &line1.BoundingBox, NULL, drcPstk_callback, &info, NULL);
				if (two_lines) {
					info.line = &line2;
					rnd_r_search(PCB->Data->padstack_tree, &line2.BoundingBox, NULL, drcPstk_callback, &info, NULL);
				}
				PCB_COPPER_GROUP_LOOP(PCB->Data, group);
				{
					info.line = &line1;
					rnd_r_search(layer->line_tree, &line1.BoundingBox, NULL, drcLine_callback, &info, NULL);
					rnd_r_search(layer->arc_tree, &line1.BoundingBox, NULL, drcArc_callback, &info, NULL);
					if (two_lines) {
						info.line = &line2;
						rnd_r_search(layer->line_tree, &line2.BoundingBox, NULL, drcLine_callback, &info, NULL);
						rnd_r_search(layer->arc_tree, &line2.BoundingBox, NULL, drcArc_callback, &info, NULL);
					}
				}
				PCB_END_LOOP;
				/* no intersector! */
				blocker = rnd_false;
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
		if (!optimize) {
			if (blocker)
				best = -1;
			else
				best = length;
			break;
		}
	}

	end->X = ans.X;
	end->Y = ans.Y;
	if (mid_out != NULL) {
		mid_out->X = line1.Point2.X;
		mid_out->Y = line1.Point2.Y;
	}
	return best;
}

static void drc_line(rnd_point_t *end)
{
	struct drc_info info;
	rnd_layergrp_id_t group, comp;
	pcb_line_t line;
	pcb_attached_line_t aline;
	static rnd_point_t last_good; /* internal state of last good endpoint - we can do this cheat, because... */

	/* ... we hardwire the assumption on how a line is drawn: it starts out as a 0 long segment, which is valid: */
	if ((pcb_crosshair.AttachedLine.Point1.X == pcb_crosshair.X) && (pcb_crosshair.AttachedLine.Point1.Y == pcb_crosshair.Y)) {
		line.Point1 = line.Point2 = pcb_crosshair.AttachedLine.Point1;
		goto auto_good;
	}

	memset(&line, 0, sizeof(line));

	/* check where the line wants to end */
	aline.Point1.X = pcb_crosshair.AttachedLine.Point1.X;
	aline.Point1.Y = pcb_crosshair.AttachedLine.Point1.Y;
	pcb_line_45(&aline);
	line.parent_type = PCB_PARENT_LAYER;
	line.parent.layer = PCB_CURRLAYER(PCB);
	line.type = PCB_OBJ_LINE;
	line.Point1 = aline.Point1;
	line.Point2 = aline.Point2;

	/* prepare for the intersection search */
	group = pcb_layer_get_group(PCB, PCB_CURRLID(pcb));
	comp = PCB->LayerGroups.len + 10;  /* this out-of-range group might save a call */
	if (pcb_layergrp_flags(PCB, group) & PCB_LYT_BOTTOM)
		info.solder = rnd_true;
	else {
		info.solder = rnd_false;
		pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, &comp, 1);
	}

	/* search for intersection */
	pcb_line_bbox(&line);
	if (setjmp(info.env) == 0) {
		info.line = &line;
		rnd_r_search(PCB->Data->padstack_tree, &line.BoundingBox, NULL, drcPstk_callback, &info, NULL);
		PCB_COPPER_GROUP_LOOP(PCB->Data, group);
		{
			info.line = &line;
			rnd_r_search(layer->line_tree, &line.BoundingBox, NULL, drcLine_callback, &info, NULL);
			rnd_r_search(layer->arc_tree, &line.BoundingBox, NULL, drcArc_callback, &info, NULL);
		}
		PCB_END_LOOP;
		/* no intersector! */
		auto_good:;
		last_good.X = end->X = line.Point2.X;
		last_good.Y = end->Y = line.Point2.Y;
		return;
	}

	/* bumped into ans */
	end->X = last_good.X;
	end->Y = last_good.Y;
}

void pcb_line_enforce_drc(pcb_board_t *pcb, rnd_coord_t tot_thick)
{
	rnd_point_t r45, rs, start;
	rnd_bool shift;
	double r1, r2;

	/* Silence a bogus compiler warning by storing this in a variable */
	rnd_layer_id_t layer_idx = PCB_CURRLID(pcb);

	if (rnd_gui->mod1_is_pressed(rnd_gui) || rnd_gui->control_is_pressed(rnd_gui) || PCB->RatDraw)
		return;

	if (!(pcb_layer_flags(PCB, layer_idx) & PCB_LYT_COPPER))
		return;

	start.X = pcb_crosshair.AttachedLine.Point1.X;
	start.Y = pcb_crosshair.AttachedLine.Point1.Y;
	rs.X = r45.X = pcb_crosshair.X;
	rs.Y = r45.Y = pcb_crosshair.Y;

	if (conf_core.editor.line_refraction != 0) {
		/* first try starting straight */
		r1 = pcb_drc_lines(pcb, &start, &rs, NULL, rnd_false, rnd_true, tot_thick);
		/* then try starting at 45 */
		r2 = pcb_drc_lines(pcb, &start, &r45, NULL, rnd_true, rnd_true, tot_thick);
	}
	else {
		drc_line(&rs);
		r45 = rs;
#define sqr(a) ((a) * (a))
		r1 = r2 = sqrt(sqr(rs.X - pcb_crosshair.AttachedLine.Point1.X) + sqr(rs.Y - pcb_crosshair.AttachedLine.Point1.Y));
#undef sqr
	}
	/* shift<Key> forces the line lookahead path to refract the alternate way */
	shift = rnd_gui->shift_is_pressed(rnd_gui);

	if (RND_XOR(r1 > r2, shift)) {
		if (conf_core.editor.line_refraction != 0) {
			if (shift) {
				if (conf_core.editor.line_refraction !=2)
					rnd_conf_setf(RND_CFR_DESIGN, "editor/line_refraction", -1, "%d", 2);
			}
			else{
				if (conf_core.editor.line_refraction != 1)
				rnd_conf_setf(RND_CFR_DESIGN, "editor/line_refraction", -1, "%d", 1);
			}
		}
		pcb->hidlib.ch_x = pcb_crosshair.X = rs.X;
		pcb->hidlib.ch_y = pcb_crosshair.Y = rs.Y;
	}
	else {
		if (conf_core.editor.line_refraction !=0) {
			if (shift) {
				if (conf_core.editor.line_refraction != 1)
					rnd_conf_setf(RND_CFR_DESIGN, "editor/line_refraction", -1, "%d", 1);
			}
			else {
				if (conf_core.editor.line_refraction != 2)
					rnd_conf_setf(RND_CFR_DESIGN, "editor/line_refraction", -1, "%d", 2);
			}
		}
		pcb->hidlib.ch_x = pcb_crosshair.X = r45.X;
		pcb->hidlib.ch_y = pcb_crosshair.Y = r45.Y;
	}
}
