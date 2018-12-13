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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */


TODO(": implement text intersections")
pcb_bool pcb_isc_text_line(pcb_text_t *a, pcb_line_t *b) { return pcb_false; }
pcb_bool pcb_isc_text_text(pcb_text_t *a, pcb_text_t *b) { return pcb_false; }
pcb_bool pcb_isc_text_poly(pcb_text_t *a, pcb_poly_t *b) { return pcb_false; }
pcb_bool pcb_isc_text_arc(pcb_text_t *a, pcb_arc_t *b) { return pcb_false; }
pcb_bool pcb_isc_text_pstk(pcb_text_t *a, pcb_pstk_t *b) { return pcb_false; }

pcb_bool pcb_intersect_obj_obj(pcb_any_obj_t *a, pcb_any_obj_t *b)
{
	/* produce the clopped version for polygons to compare */
	if ((a->type == PCB_OBJ_POLY) && (((pcb_poly_t *)a)->Clipped == NULL))
		pcb_poly_init_clip(a->parent.layer->parent.data, a->parent.layer, (pcb_poly_t *)a);
	if ((b->type == PCB_OBJ_POLY) && (((pcb_poly_t *)b)->Clipped == NULL))
		pcb_poly_init_clip(b->parent.layer->parent.data, b->parent.layer, (pcb_poly_t *)b);

	switch(a->type) {
		case PCB_OBJ_VOID: return pcb_false;
		case PCB_OBJ_LINE:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_isc_line_line((pcb_line_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_isc_text_line((pcb_text_t *)b, (pcb_line_t *)a);
				case PCB_OBJ_POLY: return pcb_isc_line_poly((pcb_line_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_isc_line_arc((pcb_line_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_isc_pstk_line((pcb_pstk_t *)b, (pcb_line_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_TEXT:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_isc_text_line((pcb_text_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_isc_text_text((pcb_text_t *)a, (pcb_text_t *)b);
				case PCB_OBJ_POLY: return pcb_isc_text_poly((pcb_text_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_isc_text_arc((pcb_text_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_isc_text_pstk((pcb_text_t *)a, (pcb_pstk_t *)b);
				default:;
			}
			break;

		case PCB_OBJ_POLY:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_isc_line_poly((pcb_line_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_TEXT: return pcb_isc_text_poly((pcb_text_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_POLY: return pcb_isc_poly_poly((pcb_poly_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_isc_arc_poly((pcb_arc_t *)b, (pcb_poly_t *)a);
				case PCB_OBJ_PSTK: return pcb_isc_pstk_poly((pcb_pstk_t *)b, (pcb_poly_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_ARC:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_isc_line_arc((pcb_line_t *)b, (pcb_arc_t *)a);
				case PCB_OBJ_TEXT: return pcb_isc_text_arc((pcb_text_t *)b, (pcb_arc_t *)a);
				case PCB_OBJ_POLY: return pcb_isc_arc_poly((pcb_arc_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_isc_arc_arc((pcb_arc_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_isc_pstk_arc((pcb_pstk_t *)b, (pcb_arc_t *)a);
				default:;
			}
			break;
		case PCB_OBJ_PSTK:
			switch(b->type) {
				case PCB_OBJ_VOID: return pcb_false;
				case PCB_OBJ_LINE: return pcb_isc_pstk_line((pcb_pstk_t *)a, (pcb_line_t *)b);
				case PCB_OBJ_TEXT: return pcb_isc_text_pstk((pcb_text_t *)b, (pcb_pstk_t *)a);
				case PCB_OBJ_POLY: return pcb_isc_pstk_poly((pcb_pstk_t *)a, (pcb_poly_t *)b);
				case PCB_OBJ_ARC:  return pcb_isc_pstk_arc((pcb_pstk_t *)a, (pcb_arc_t *)b);
				case PCB_OBJ_PSTK: return pcb_isc_pstk_pstk((pcb_pstk_t *)a, (pcb_pstk_t *)b);
				default:;
			}
			break;
		default:;
	}
	assert(!"Don't know how to check intersection of these objet types");
	return pcb_false;
}


