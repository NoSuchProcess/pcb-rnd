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
#include "global_typedefs.h"
#include "flag.h"
#include <librnd/poly/rtree.h>
#include <librnd/core/math_helper.h>
#include <librnd/poly/polyarea.h>
#include <librnd/poly/rtree2_compat.h>


/* Prototypes */

void pcb_polygon_init(void);
void pcb_polygon_uninit(void);
rnd_cardinal_t pcb_poly_point_idx(pcb_poly_t *polygon, rnd_point_t *point);
rnd_cardinal_t pcb_poly_contour_point(pcb_poly_t *polygon, rnd_cardinal_t point);
rnd_cardinal_t pcb_poly_contour_prev_point(pcb_poly_t *polygon, rnd_cardinal_t point);
rnd_cardinal_t pcb_poly_contour_next_point(pcb_poly_t *polygon, rnd_cardinal_t point);
rnd_cardinal_t pcb_poly_get_lowest_distance_point(pcb_poly_t *, rnd_coord_t, rnd_coord_t);
rnd_bool pcb_poly_remove_excess_points(pcb_layer_t *, pcb_poly_t *);
void pcb_polygon_go_to_prev_point(void); /* undo */
void pcb_polygon_go_to_next_point(void); /* redo */
void pcb_polygon_close_poly(void);
void pcb_polygon_copy_attached_to_layer(void);
void pcb_polygon_close_hole(void);
void pcb_polygon_hole_create_from_attached(void);
int pcb_poly_holes(pcb_poly_t * ptr, const rnd_box_t * range, int (*callback) (rnd_pline_t *, void *user_data), void *user_data);
int pcb_poly_plows(pcb_data_t *Data, int type, void *ptr1, void *ptr2,
	rnd_r_dir_t (*cb)(pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data),
	void *user_data);
void pcb_poly_compute_no_holes(pcb_poly_t * poly);

/* helpers: create complex shaped polygons */
rnd_polyarea_t *pcb_poly_from_poly(pcb_poly_t *);
rnd_polyarea_t *pcb_poly_from_pcb_line(pcb_line_t * l, rnd_coord_t thick);
rnd_polyarea_t *pcb_poly_from_pcb_arc(pcb_arc_t * l, rnd_coord_t thick);
rnd_polyarea_t *pcb_poly_from_box_bloated(rnd_box_t * box, rnd_coord_t radius);
rnd_polyarea_t *pcb_poly_clearance_construct(pcb_poly_t *subpoly, rnd_coord_t *clr_override, pcb_poly_t *in_poly); /* clearance shape for when clearpolypoly is set */

int pcb_poly_init_clip(pcb_data_t * d, pcb_layer_t * l, pcb_poly_t * p);
int pcb_poly_init_clip_force(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p); /* bypasses clip inhibit */
void pcb_poly_restore_to_poly(pcb_data_t *, int, void *, void *);
void pcb_poly_clear_from_poly(pcb_data_t *, int, void *, void *);

/* Convert a polygon to an unclipped polyarea */
rnd_polyarea_t *pcb_poly_to_polyarea(pcb_poly_t *p, rnd_bool *need_full);

/* Same as pcb_poly_init_clip() but also call cb before each operation,
   giving the caller a chance to draw a progress bar */
int pcb_poly_init_clip_prog(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p, void (*cb)(void *ctx), void *ctx, int force);

/* Return the number of subtractions that have to be executed by a
   pcb_poly_init_clip() on the given polygon */
rnd_cardinal_t pcb_poly_num_clears(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon);


rnd_bool pcb_poly_is_point_in_p(rnd_coord_t, rnd_coord_t, rnd_coord_t, pcb_poly_t *);
rnd_bool pcb_poly_is_point_in_p_ignore_holes(rnd_coord_t, rnd_coord_t, pcb_poly_t *);
rnd_bool_t pcb_is_point_in_convex_quad(rnd_vector_t p, rnd_vector_t *q);
rnd_bool pcb_poly_is_rect_in_p(rnd_coord_t, rnd_coord_t, rnd_coord_t, rnd_coord_t, pcb_poly_t *);
rnd_bool pcb_poly_isects_poly(rnd_polyarea_t *, pcb_poly_t *, rnd_bool);
rnd_bool pcb_pline_isect_line(rnd_pline_t *pl, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t *cx, rnd_coord_t *cy);
rnd_bool pcb_pline_isect_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r); /* cirlce contour crosses polyline contour */
rnd_bool pcb_pline_embraces_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r); /* circle is within the polyline; caller must make sure they do not cross! */
rnd_bool pcb_pline_overlaps_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r); /* circle is within or is crossing the polyline */
rnd_bool pcb_poly_morph(pcb_layer_t *, pcb_poly_t *);
void pcb_poly_no_holes_dicer(pcb_poly_t * p, const rnd_box_t * clip, void (*emit) (rnd_pline_t *, void *), void *user_data);
void pcb_poly_to_polygons_on_layer(pcb_data_t *, pcb_layer_t *, rnd_polyarea_t *, pcb_flag_t);

rnd_bool pcb_pline_is_rectangle(rnd_pline_t *pl);

/* clear np1 from the polygon; also free np1 if fnp is true. Returns 1 on
   success. */
int pcb_poly_subtract(rnd_polyarea_t *np1, pcb_poly_t *p, rnd_bool fnp);

/* Subtract or unsubtract obj from poly; Layer is used for looking up thermals */
int pcb_poly_sub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj);
int pcb_poly_unsub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj);


/* Construct a polygon that represents the clearance around a text object
   (assuming a polygon with no enforce clearance). This can be a round rect
   (old method) or a complex polygon for tight_clearance */
rnd_polyarea_t *pcb_poly_construct_text_clearance(pcb_text_t *text);

#endif
