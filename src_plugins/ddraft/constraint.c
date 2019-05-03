/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin: trim objects
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "crosshair.h"
#include "obj_line.h"
#include "tool.h"

typedef struct {
	double line_angle[32], move_angle[32];
	pcb_coord_t line_length[32], move_length[32];
	double line_angle_mod, move_angle_mod;
	pcb_coord_t line_length_mod, move_length_mod;
	
	int line_angle_len, line_length_len, move_angle_len, move_length_len;
} ddraft_cnst_t;

static ddraft_cnst_t cons;

static int find_best_angle(double *out_ang, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, double *angles, int anglen, double angle_mod)
{
	double diff, ang, best_diff, target_ang;
	int n, best;

	ang = atan2(y2 - y1, x2 - x1) * PCB_RAD_TO_DEG;

	if (anglen > 0) {
		/* find the best matching constraint angle */
		best = -1;
		best_diff = 1000.0;
		for(n = 0; n < anglen; n++) {
			if (angles[n] < 180)
				diff = fabs(ang - angles[n]);
			else
				diff = fabs(ang + (angles[n]-180));
			if (diff < best_diff) {
				best_diff = diff;
				best = n;
			}
		}
		if (best < 0)
			return -1;

		target_ang = angles[best];
	}
	else
		target_ang = ang;

	if (angle_mod > 0)
		target_ang = floor(target_ang / angle_mod) * angle_mod;

	target_ang /= PCB_RAD_TO_DEG;
	*out_ang = target_ang;
	return 0;
}

static int find_best_length(double *out_len, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t *lengths, int lengthlen, pcb_coord_t length_mod)
{
	double len, best_diff, diff, target_len;
	int n, best;

	len = pcb_distance(x1, y1, x2, y2);

	if (lengthlen > 0) {
		/* find the best matching constraint length */
		best = -1;
		best_diff = COORD_MAX;
		for(n = 0; n < lengthlen; n++) {
			diff = fabs(len - lengths[n]);
			if (diff < best_diff) {
				best_diff = diff;
				best = n;
			}
		}
		if (best < 0)
			return -1;

		target_len = lengths[best];
	}
	else
		target_len = len;

	if (length_mod > 0)
		target_len = floor(target_len / (double)length_mod) * (double)length_mod;

	*out_len = target_len;
	return 0;
}

static void cnst_line_anglen(ddraft_cnst_t *cn)
{
	double target_ang, dx, dy, target_len;
	pcb_route_object_t *line;
	int res;

	if (((cn->line_angle_len == 0) && (cn->line_length_len == 0) && (cn->line_angle_mod <= 0) && (cn->line_length_mod <= 0)) || (pcb_crosshair.Route.size < 1))
		return;

	line = &pcb_crosshair.Route.objects[pcb_crosshair.Route.size-1];
	if (line->type != PCB_OBJ_LINE)
		return;

	res = find_best_angle(&target_ang,
		line->point1.X, line->point1.Y, pcb_crosshair.X, pcb_crosshair.Y,
		cn->line_angle, cn->line_angle_len, cn->line_angle_mod);
	if (res < 0) return;

	res = find_best_length(&target_len,
		line->point1.X, line->point1.Y, pcb_crosshair.X, pcb_crosshair.Y,
		cn->line_length, cn->line_length_len, cn->line_length_mod);
	if (res < 0) return;

	dx = target_len * cos(target_ang);
	dy = target_len * sin(target_ang);

	line->point2.X = line->point1.X + dx;
	line->point2.Y = line->point1.Y + dy;
}

static void cnst_line2(ddraft_cnst_t *cn)
{
	cnst_line_anglen(cn);
}

static void cnst_move(ddraft_cnst_t *cn)
{
	double target_ang, dx, dy, target_len;
	int res;

	if (((cn->move_angle_len == 0) && (cn->move_length_len == 0) && (cn->move_angle_mod <= 0) && (cn->move_length_mod <= 0)))
		return;

	res = find_best_angle(&target_ang,
		pcb_crosshair.AttachedObject.X, pcb_crosshair.AttachedObject.Y, pcb_crosshair.X, pcb_crosshair.Y,
		cn->move_angle, cn->move_angle_len, cn->move_angle_mod);
	if (res < 0) return;

	res = find_best_length(&target_len,
		pcb_crosshair.AttachedObject.X, pcb_crosshair.AttachedObject.Y, pcb_crosshair.X, pcb_crosshair.Y,
		cn->move_length, cn->move_length_len, cn->move_length_mod);
	if (res < 0) return;

	dx = target_len * cos(target_ang);
	dy = target_len * sin(target_ang);

	pcb_crosshair.AttachedObject.tx = pcb_crosshair.AttachedObject.X + dx;
	pcb_crosshair.AttachedObject.ty = pcb_crosshair.AttachedObject.Y + dy;
}

static void cnst_enforce(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND) || (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_THIRD))
		cnst_line2(&cons);
	else if (pcb_crosshair.AttachedObject.State == PCB_CH_STATE_SECOND) /* normal d&d move or copy */
		cnst_move(&cons);
	else if (pcb_tool_note.Moving) /* selected copy (buffer mode) */
		cnst_move(&cons);

}
