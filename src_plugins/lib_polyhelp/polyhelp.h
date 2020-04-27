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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "obj_poly.h"


void pcb_pline_fprint_anim(FILE *f, const pcb_pline_t *pl);

/* Add lines on dst tracing pline from the inner side (no line will extend
   outside of the original pline, except when the original polygon has a hair
   narrower than thickness). Returns number of lines created */
pcb_cardinal_t pcb_pline_to_lines(pcb_layer_t *dst, const pcb_pline_t *src, rnd_coord_t thickness, rnd_coord_t clearance, pcb_flag_t flags);

/* Returns whether the clipped polygon is a simple rectangle (single island,
   no-hole rectangle). */
rnd_bool pcb_cpoly_is_simple_rect(const pcb_poly_t *p);

/* Returns whether all edges of a pline are axis aligned */
rnd_bool pcb_pline_is_aligned(const pcb_pline_t *src);

/*** Generate an rtree of all edges if a polygon */

typedef struct {
	rnd_box_t bbox;
	rnd_coord_t x1, y1, x2, y2;
} pcb_cpoly_edge_t;

typedef struct {
	pcb_rtree_t *edge_tree;
	rnd_box_t bbox;
	pcb_cardinal_t used, alloced;
	pcb_cpoly_edge_t edges[1];
} pcb_cpoly_edgetree_t;

pcb_cpoly_edgetree_t *pcb_cpoly_edgetree_create(const pcb_poly_t *src, rnd_coord_t offs);
void pcb_cpoly_edgetree_destroy(pcb_cpoly_edgetree_t *etr);


/*** hatching ***/

/* bitfield to request horizontal and/or vertical hatching or striping */
typedef enum {
	PCB_CPOLY_HATCH_HORIZONTAL = 1,
	PCB_CPOLY_HATCH_VERTICAL = 2
} pcb_cpoly_hatchdir_t;

/* hatch a polygon with horizontal and/or vertical lines drawn on dst,
   one line per period */
void pcb_cpoly_hatch_lines(pcb_layer_t *dst, const pcb_poly_t *src, pcb_cpoly_hatchdir_t dir, rnd_coord_t period, rnd_coord_t thickness, rnd_coord_t clearance, pcb_flag_t flags);

/* Generic hor-ver hatch with a callback */
void pcb_cpoly_hatch(const pcb_poly_t *src, pcb_cpoly_hatchdir_t dir, rnd_coord_t offs, rnd_coord_t period, void *ctx, void (*cb)(void *ctx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2));

