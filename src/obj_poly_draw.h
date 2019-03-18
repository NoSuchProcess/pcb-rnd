/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

/*** Standard draw of polygons ***/

#include "draw.h"

/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t pcb_poly_draw_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_poly_draw_term_callback(const pcb_box_t * b, void *cl);
#endif

void pcb_poly_invalidate_erase(pcb_poly_t *Polygon);
void pcb_poly_invalidate_draw(pcb_layer_t *Layer, pcb_poly_t *Polygon);
void pcb_poly_name_invalidate_draw(pcb_poly_t *poly);
void pcb_poly_draw_label(pcb_draw_info_t *info, pcb_poly_t *poly);
void pcb_poly_draw_annotation(pcb_draw_info_t *info, pcb_poly_t *poly);


