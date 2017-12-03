/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/*** Standard draw of polygons ***/

/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t pcb_poly_draw_callback(const pcb_box_t * b, void *cl);
pcb_r_dir_t pcb_poly_draw_term_callback(const pcb_box_t * b, void *cl);
#endif

void pcb_poly_invalidate_erase(pcb_poly_t *Polygon);
void pcb_poly_invalidate_draw(pcb_layer_t *Layer, pcb_poly_t *Polygon);
void pcb_poly_name_invalidate_draw(pcb_poly_t *poly);
void pcb_poly_draw_label(pcb_poly_t *poly);

