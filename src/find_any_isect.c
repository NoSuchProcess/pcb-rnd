/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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


#warning TODO: implement text intersections
pcb_bool pcb_intersect_text_line(pcb_text_t *a, pcb_line_t *b) { return pcb_false; }
pcb_bool pcb_intersect_text_text(pcb_text_t *a, pcb_text_t *b) { return pcb_false; }
pcb_bool pcb_intersect_text_poly(pcb_text_t *a, pcb_poly_t *b) { return pcb_false; }
pcb_bool pcb_intersect_text_arc(pcb_text_t *a, pcb_arc_t *b) { return pcb_false; }
pcb_bool pcb_intersect_text_pstk(pcb_text_t *a, pcb_pstk_t *b) { return pcb_false; }

pcb_bool pcb_intersect_obj_obj(pcb_any_obj_t *a, pcb_any_obj_t *b)
{
	switch(a->type) {
		case PCB_OBJ_VOID: return pcb_false;
		case PCB_OBJ_LINE:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_intersect_line_line((pcb_line_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_intersect_text_line((pcb_text_t *)b, (pcb_line_t *)a);
				case PCB_OBJ_POLY: return pcb_is_line_in_poly((pcb_line_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_intersect_line_arc((pcb_line_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_pstk_intersect_line((pcb_pstk_t *)b, (pcb_line_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_TEXT:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_intersect_text_line((pcb_text_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_intersect_text_text((pcb_text_t *)a, (pcb_text_t *)b);
				case PCB_OBJ_POLY: return pcb_intersect_text_poly((pcb_text_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_intersect_text_arc((pcb_text_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_intersect_text_pstk((pcb_text_t *)a, (pcb_pstk_t *)b);
				default:;
			}
			break;

		case PCB_OBJ_POLY:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_is_line_in_poly((pcb_line_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_TEXT: return pcb_intersect_text_poly((pcb_text_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_POLY: return pcb_is_poly_in_poly((pcb_poly_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_is_arc_in_poly((pcb_arc_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_PSTK: return pcb_pstk_intersect_poly((pcb_pstk_t *)b, (pcb_poly_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_ARC:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_intersect_line_arc((pcb_line_t *)b, (pcb_arc_t *)a);
				case PCB_OBJ_TEXT: return pcb_intersect_text_arc((pcb_text_t *)b, (pcb_arc_t *)a);
				case PCB_OBJ_POLY: return pcb_is_arc_in_poly((pcb_arc_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return ArcArcIntersect((pcb_arc_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_pstk_intersect_arc((pcb_pstk_t *)b, (pcb_arc_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_PSTK:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_pstk_intersect_line((pcb_pstk_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_intersect_text_pstk((pcb_text_t *)b, (pcb_pstk_t *)a);
				case PCB_OBJ_POLY: return pcb_pstk_intersect_poly((pcb_pstk_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_pstk_intersect_arc((pcb_pstk_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_pstk_intersect_pstk((pcb_pstk_t *)a, (pcb_pstk_t *)b);
				default:;
			}
			break;
		default:;
	}
	assert(!"Don't know how to check intersection of these objet types");
	return pcb_false;
}


