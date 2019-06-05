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

/* High level polygon support: pcb-specific polygon operations on complex
   polygon objects with islands and holes */

#ifndef	PCB_POLYGON_H
#define	PCB_POLYGON_H

#include "config.h"
#include "flag.h"
#include "rtree.h"
#include "math_helper.h"
#include "polyarea.h"

/* Implementation constants */

#define PCB_POLY_CIRC_SEGS 40
#define PCB_POLY_CIRC_SEGS_F ((float)PCB_POLY_CIRC_SEGS)

/* adjustment to make the segments outline the circle rather than connect
 * points on the circle: 1 - cos (\alpha / 2) < (\alpha / 2) ^ 2 / 2
 */
#define PCB_POLY_CIRC_RADIUS_ADJ (1.0 + M_PI / PCB_POLY_CIRC_SEGS_F * \
                                    M_PI / PCB_POLY_CIRC_SEGS_F / 2.0)

/* polygon diverges from modelled arc no more than MAX_ARC_DEVIATION * thick */
#define PCB_POLY_ARC_MAX_DEVIATION 0.02

/* Prototypes */

void pcb_polygon_init(void);
void pcb_polygon_uninit(void);
pcb_cardinal_t pcb_poly_point_idx(pcb_poly_t *polygon, pcb_point_t *point);
pcb_cardinal_t pcb_poly_contour_point(pcb_poly_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t pcb_poly_contour_prev_point(pcb_poly_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t pcb_poly_contour_next_point(pcb_poly_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t pcb_poly_get_lowest_distance_point(pcb_poly_t *, pcb_coord_t, pcb_coord_t);
pcb_bool pcb_poly_remove_excess_points(pcb_layer_t *, pcb_poly_t *);
void pcb_polygon_go_to_prev_point(void); /* undo */
void pcb_polygon_go_to_next_point(void); /* redo */
void pcb_polygon_close_poly(void);
void pcb_polygon_copy_attached_to_layer(void);
void pcb_polygon_close_hole(void);
void pcb_polygon_hole_create_from_attached(void);
int pcb_poly_holes(pcb_poly_t * ptr, const pcb_box_t * range, int (*callback) (pcb_pline_t *, void *user_data), void *user_data);
int pcb_poly_plows(pcb_data_t *Data, int type, void *ptr1, void *ptr2,
	pcb_r_dir_t (*cb)(pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data),
	void *user_data);
void pcb_poly_compute_no_holes(pcb_poly_t * poly);

/* helpers: create complex shaped polygons */
pcb_polyarea_t *pcb_poly_from_contour(pcb_pline_t *);
pcb_polyarea_t *pcb_poly_from_poly(pcb_poly_t *);
pcb_polyarea_t *pcb_poly_from_rect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2);
pcb_polyarea_t *pcb_poly_from_circle(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius);
pcb_polyarea_t *pcb_poly_from_octagon(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style);
pcb_polyarea_t *pcb_poly_from_pcb_line(pcb_line_t * l, pcb_coord_t thick);
pcb_polyarea_t *pcb_poly_from_pcb_arc(pcb_arc_t * l, pcb_coord_t thick);
pcb_polyarea_t *pcb_poly_from_box_bloated(pcb_box_t * box, pcb_coord_t radius);
pcb_polyarea_t *pcb_poly_clearance_construct(pcb_poly_t *subpoly); /* clearance shape for when clearpolypoly is set */


void pcb_poly_frac_circle(pcb_pline_t *, pcb_coord_t, pcb_coord_t, pcb_vector_t, int);
int pcb_poly_init_clip(pcb_data_t * d, pcb_layer_t * l, pcb_poly_t * p);
void pcb_poly_restore_to_poly(pcb_data_t *, int, void *, void *);
void pcb_poly_clear_from_poly(pcb_data_t *, int, void *, void *);

/* Same as pcb_poly_init_clip() but also call cb before each operation,
   giving the caller a chance to draw a progress bar */
int pcb_poly_init_clip_prog(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p, void (*cb)(void *ctx), void *ctx);

/* Return the number of subtractions that have to be executed by a
   pcb_poly_init_clip() on the given polygon */
pcb_cardinal_t pcb_poly_num_clears(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon);


pcb_bool pcb_poly_is_point_in_p(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_poly_t *);
pcb_bool pcb_poly_is_point_in_p_ignore_holes(pcb_coord_t, pcb_coord_t, pcb_poly_t *);
pcb_bool_t pcb_is_point_in_convex_quad(pcb_vector_t p, pcb_vector_t *q);
pcb_bool pcb_poly_is_rect_in_p(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_poly_t *);
pcb_bool pcb_poly_isects_poly(pcb_polyarea_t *, pcb_poly_t *, pcb_bool);
pcb_bool pcb_pline_isect_line(pcb_pline_t *pl, pcb_coord_t lx1, pcb_coord_t ly1, pcb_coord_t lx2, pcb_coord_t ly2, pcb_coord_t *cx, pcb_coord_t *cy);
pcb_bool pcb_pline_isect_circ(pcb_pline_t *pl, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r); /* cirlce contour crosses polyline contour */
pcb_bool pcb_pline_embraces_circ(pcb_pline_t *pl, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r); /* circle is within the polyline; caller must make sure they do not cross! */
pcb_bool pcb_pline_overlaps_circ(pcb_pline_t *pl, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r); /* circle is within or is crossing the polyline */
pcb_bool pcb_poly_morph(pcb_layer_t *, pcb_poly_t *);
void pcb_poly_no_holes_dicer(pcb_poly_t * p, const pcb_box_t * clip, void (*emit) (pcb_pline_t *, void *), void *user_data);
void pcb_poly_to_polygons_on_layer(pcb_data_t *, pcb_layer_t *, pcb_polyarea_t *, pcb_flag_t);

void pcb_poly_square_pin_factors(int style, double *xm, double *ym);

pcb_bool pcb_pline_is_rectangle(pcb_pline_t *pl);

/* clear np1 from the polygon; also free np1 if fnp is true. Returns 1 on
   success. */
int pcb_poly_subtract(pcb_polyarea_t *np1, pcb_poly_t *p, pcb_bool fnp);

/* Subtract or unsubtract obj from poly; Layer is used for looking up thermals */
int pcb_poly_sub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj);
int pcb_poly_unsub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj);


#endif
