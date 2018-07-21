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

static int pcb_trim_with_line(pcb_line_t *cut_edge, pcb_any_obj_t *obj, pcb_coord_t rem_x, pcb_coord_t rem_y)
{
	int p;
	double d1, d2;
	pcb_box_t ip;

	switch(obj->type) {
		case PCB_OBJ_LINE:
			{
				pcb_line_t *l = (pcb_line_t *)obj;
				p = pcb_intersect_cline_cline(cut_edge, l, &ip);
				switch(p) {
					case 0: return 0;
					case 1:
						/* do not trim if the result would be a zero-length line */
						if ((ip.X1 == l->Point1.X) && (ip.Y1 == l->Point1.Y)) return 0;
						if ((ip.X1 == l->Point2.X) && (ip.Y1 == l->Point2.Y)) return 0;

						/* determine which endpoint to move (the one closer to the rem point) */
						d1 = pcb_distance2(rem_x, rem_y, l->Point1.X, l->Point1.Y);
						d2 = pcb_distance2(rem_x, rem_y, l->Point2.X, l->Point2.Y);
						pcb_line_pre(l);
						if (d1 < d2) {
							l->Point1.X = ip.X1;
							l->Point1.Y = ip.Y1;
						}
						else {
							l->Point2.X = ip.X1;
							l->Point2.Y = ip.Y1;
						}
						pcb_line_post(l);
				}
			}
			break;
		default: return -1;
	}
	return 0;
}

int pcb_trim(pcb_any_obj_t *cut_edge, pcb_any_obj_t *obj, pcb_coord_t rem_x, pcb_coord_t rem_y)
{
pcb_trace("trim: %p %p %mm %mm\n", cut_edge, obj, rem_x, rem_y);
	switch(cut_edge->type) {
		case PCB_OBJ_LINE: return pcb_trim_with_line((pcb_line_t *)cut_edge, obj, rem_x, rem_y);
		default:
			return -1;
	}
}

