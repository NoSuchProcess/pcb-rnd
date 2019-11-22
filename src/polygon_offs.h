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

/* Polygon contour offset calculation */

#include <genvector/vtp0.h>


/* Calculate the offset plines of src and append the resulting plines to dst.
   Yields multiple islands in some corner cases. */
void pcb_pline_dup_offsets(vtp0_t *dst, const pcb_pline_t *src, pcb_coord_t offs);

/* Same, but returns the largest island only */
pcb_pline_t *pcb_pline_dup_offset(const pcb_pline_t *src, pcb_coord_t offs);


/* low level */

typedef struct {
	double x, y, nx, ny;
} pcb_polo_t;

/* Calculate the normal vectors of a cache */
void pcb_polo_norms(pcb_polo_t *pcsh, long num_pts);

/* Calculate and return the double of the area of a cached polygon */
double pcb_polo_2area(pcb_polo_t *pcsh, long num_pts);

/* Ortho-shift all edges of a polygon. Positive offset means grow. */
void pcb_polo_offs(double offs, pcb_polo_t *pcsh, long num_pts);

/* modify dst so it is at least offs far from any point or line of src */
void pcb_pline_keepout_offs(pcb_pline_t *dst, const pcb_pline_t *src, pcb_coord_t offs);

/* Orhto-shift an edge specified by x0;y0 and x1;y1. Calculate the new
   edge points by extending/shrinking the previous and next line segment.
   Modifies the target edge's start and end coords. Requires cached normals
   Positive offset means grow. */
void pcb_polo_edge_shift(double offs,
	double *x0, double *y0, double nx, double ny,
	double *x1, double *y1,
	double prev_x, double prev_y, double prev_nx, double prev_ny,
	double next_x, double next_y, double next_nx, double next_ny);

