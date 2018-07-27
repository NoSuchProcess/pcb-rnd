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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "crosshair.h"
#include "obj_line.h"

typedef struct {
	double line_angle[32];
	double line_length[32];
	
	
	int line_angle_len, line_length_len;
} ddraft_cnst_t;

static ddraft_cnst_t cons;

static void cnst_line_anglen(ddraft_cnst_t *cn)
{
	double dx, dy, diff, target_ang, ang, target_len, len, best_diff;
	int n, best;
	pcb_route_object_t *line;

	if (((cn->line_angle_len == 0) && (cn->line_length_len == 0)) || (pcb_crosshair.Route.size < 1))
		return;

	line = &pcb_crosshair.Route.objects[pcb_crosshair.Route.size-1];
	if (line->type != PCB_OBJ_LINE)
		return;

	ang = atan2(-(pcb_crosshair.Y - line->point1.Y), pcb_crosshair.X - line->point1.X) * PCB_RAD_TO_DEG;
	len = pcb_distance(line->point1.X, line->point1.Y, pcb_crosshair.X, pcb_crosshair.Y);

	if (cn->line_angle_len > 0) {
		/* find the best matching constraint angle */
		best = -1;
		best_diff = 1000.0;
		for(n = 0; n < cn->line_angle_len; n++) {
			diff = fabs(ang - cn->line_angle[n]);
			if (diff < best_diff) {
				best_diff = diff;
				best = n;
			}
		}
		if (best < 0)
			return;

		target_ang = cn->line_angle[best];
	}
	else
		target_ang = ang;


	if (cn->line_length_len > 0) {
		/* find the best matching constraint length */
		best = -1;
		best_diff = COORD_MAX;
		for(n = 0; n < cn->line_length_len; n++) {
			diff = fabs(len - cn->line_length[n]);
			if (diff < best_diff) {
				best_diff = diff;
				best = n;
			}
		}
		if (best < 0)
			return;

		target_len = cn->line_length[best];
	}
	else
		target_len = len;

	target_ang /= PCB_RAD_TO_DEG;
	dx = target_len * cos(target_ang);
	dy = target_len * sin(target_ang);

	line->point2.X = line->point1.X + dx;
	line->point2.Y = line->point1.Y - dy;
}


static void cnst_line2(ddraft_cnst_t *cn)
{
	cnst_line_anglen(cn);
}

static void cnst_enforce(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND) || (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_THIRD))
		cnst_line2(&cons);
}
