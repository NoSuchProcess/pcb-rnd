/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#include "obj_poly.h"


void pcb_pline_fprint_anim(FILE *f, const pcb_pline_t *pl);
pcb_pline_t *pcb_pline_dup_offset(const pcb_pline_t *src, pcb_coord_t offs);

/* Add lines on dst tracing pline from the inner side (no line will extend
   outside of the original pline, except when the original polygon has a hair
   narrower than thickness) */
void pcb_pline_to_lines(pcb_layer_t *dst, const pcb_pline_t *src, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags);

/* Returns whether the clipped polygon is a simple rectangle (single island,
   no-hole rectangle). */
pcb_bool pcb_cpoly_is_simple_rect(const pcb_polygon_t *p);

/* Returns whether all edges of a pline are axis aligned */
pcb_bool pcb_pline_is_aligned(const pcb_pline_t *src);

typedef struct {
	pcb_box_t bbox;
	pcb_coord_t x1, y1, x2, y2;
} pcb_cpoly_edge_t;

typedef struct {
	pcb_rtree_t *edge_tree;
	pcb_box_t bbox;
	pcb_cardinal_t used, alloced;
	pcb_cpoly_edge_t edges[1];
} pcb_cpoly_edgetree_t;
