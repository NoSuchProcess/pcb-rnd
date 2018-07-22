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

#include "undo_old.h"

/* Move a line endpoint to a new absoltue coord in an undoable way */
static void move_lp(pcb_line_t *line, int pt_idx, pcb_coord_t x, pcb_coord_t y)
{
	pcb_point_t *pt;

	switch(pt_idx) {
		case 1: pt = &line->Point1; break;
		case 2: pt = &line->Point2; break;
		default:
			abort();
	}

	pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, line->parent.layer, line, pt, x - pt->X, y - pt->Y);
	pcb_line_pre(line);
	pt->X = x;
	pt->Y = y;
	pcb_line_post(line);
}

static int pcb_trim_line(vtp0_t *cut_edges, pcb_line_t *line, pcb_coord_t rem_x, pcb_coord_t rem_y)
{
	int p, n;
	double io[2];
	double mino = 0.0, maxo = 1.0, remo = pcb_cline_pt_offs(line, rem_x, rem_y);
	pcb_coord_t x, y;

	for(n = 0; n < vtp0_len(cut_edges); n++) {
		pcb_any_obj_t *cut_edge = (pcb_any_obj_t *)cut_edges->array[n];
		switch(cut_edge->type) {
			case PCB_OBJ_LINE:
				{
					p = pcb_intersect_cline_cline(line, (pcb_line_t *)cut_edge, NULL, io);
					switch(p) {
						case 0: continue; /* no intersection, skip to the next potential cutting edge */
						case 1:
							if (io[0] < remo) {
								if (io[0] > mino) mino = io[0];
							}
							else {
								if (io[0] < maxo) maxo = io[0];
							}
							break;
					}
				}
				break;
			default: return -1;
		}
	}

	if ((mino == 0.0) && (maxo == 1.0))
		return 0; /* nothing to be done */

	if (mino == maxo)
		return 0; /* refuse to end up with 0-length lines */

	/* mino and maxo holds the two endpoint offsets after the cuts, in respect
	   to the original line. Cut/split the line using them. */
	if ((mino > 0.0) && (maxo < 1.0)) { /* remove (shortest) middle section */
		pcb_line_t *new_line = pcb_line_dup(line->parent.layer, line);

		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, new_line->parent.layer, new_line, new_line);

		pcb_cline_offs(line, maxo, &x, &y);
		move_lp(line, 1, x, y);

		pcb_cline_offs(new_line, mino, &x, &y);
		move_lp(new_line, 2, x, y);
	}
	else if ((mino == 0.0) && (maxo < 1.0)) { /* truncate at point1 (shortest overdue) */
		pcb_cline_offs(line, maxo, &x, &y);
		move_lp(line, 1, x, y);
	}
	else if ((mino > 0.0) && (maxo == 1.0)) { /* truncate at point2 (shortest overdue) */
		pcb_cline_offs(line, mino, &x, &y);
		move_lp(line, 2, x, y);
	}
	else
		return -1;

	return 1;
}

static pcb_line_t *split_lp(pcb_line_t *line, double offs)
{
	pcb_coord_t x, y;
	pcb_line_t *new_line = pcb_line_dup(line->parent.layer, line);

	pcb_undo_add_obj_to_create(PCB_OBJ_LINE, new_line->parent.layer, new_line, new_line);

	pcb_cline_offs(line, offs, &x, &y);
	move_lp(line, 2, x, y);
	move_lp(new_line, 1, x, y);

	return new_line;
}


static int pcb_split_line(vtp0_t *cut_edges, pcb_line_t *line, pcb_coord_t rem_x, pcb_coord_t rem_y)
{
	int p, n, numsplt = 0, res;
	double io[2];
	pcb_line_t *new_line;

	for(n = 0; n < vtp0_len(cut_edges); n++) {
		pcb_any_obj_t *cut_edge = (pcb_any_obj_t *)cut_edges->array[n];
		switch(cut_edge->type) {
			case PCB_OBJ_LINE:
				{
					p = pcb_intersect_cline_cline(line, (pcb_line_t *)cut_edge, NULL, io);
					switch(p) {
						case 0: continue; /* no intersection, skip to the next potential cutting edge */
						case 2:
							if ((io[1] != 0.0) && (io[1] != 1.0)) {
								new_line = split_lp(line, io[1]);
								numsplt++;
								res = pcb_split_line(cut_edges, new_line, rem_x, rem_y);
								if (res > 0) numsplt += res;
							}
						case 1:
							if ((io[0] != 0.0) && (io[0] != 1.0)) {
								new_line = split_lp(line, io[0]);
								numsplt++;
								res = pcb_split_line(cut_edges, new_line, rem_x, rem_y);
								if (res > 0) numsplt += res;
							}
							break;
					}
				}
				break;
			default: return -1;
		}
	}

	return numsplt;
}

int pcb_trim_split(vtp0_t *cut_edges, pcb_any_obj_t *obj, pcb_coord_t rem_x, pcb_coord_t rem_y, int trim)
{
	int res = 0;
	switch(obj->type) {
		case PCB_OBJ_LINE:
			if (trim)
				res = pcb_trim_line(cut_edges, (pcb_line_t *)obj, rem_x, rem_y);
			else
				res = pcb_split_line(cut_edges, (pcb_line_t *)obj, rem_x, rem_y);
			break;
		default:
			return -1;
	}

	if (res > 0)
		pcb_undo_inc_serial();
	return res;
}

