/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <librnd/core/hid.h>
#include "polygon.h"
#include "obj_poly.h"
#include <librnd/core/hid_inlines.h>

static rnd_coord_t *fc_x = NULL, *fc_y = NULL;
static size_t fc_alloced = 0;


/* call this before the loop */
#define vert_opt_begin() \
	{ \
		rnd_coord_t last_x, last_y, this_x, this_y, next_x, next_y, mindist; \
		last_x = pl->head->point[0]; \
		last_y = pl->head->point[1]; \
		v = pl->head->next; \
		mindist = rnd_render->coord_per_pix * 2; \

/* call this before drawing the next vertex */
#define vert_opt_loop1(v, force, skip_stmt) \
		this_x = v->point[0]; \
		this_y = v->point[1]; \
		if ((!force) && (RND_ABS(this_x - last_x) < mindist) && (RND_ABS(this_y - last_y) < mindist)) { \
			next_x = v->next->point[0]; \
			next_y = v->next->point[1]; \
			if ((RND_ABS(this_x - next_x) < mindist) && (RND_ABS(this_y - next_y) < mindist)) \
				{ skip_stmt; } \
		} \

/* call this after drawing the next vertex */
#define vert_opt_loop2() \
		last_x = this_x; \
		last_y = this_y; \


/* call this after the loop */
#define vert_opt_end() \
	}


static void fill_contour(rnd_hid_gc_t gc, rnd_pline_t * pl)
{
	size_t n, i = 0;
	rnd_vnode_t *v;
	int first = 1;

	n = pl->Count;

	if (n > fc_alloced) {
		free(fc_x);
		free(fc_y);
		fc_x = (rnd_coord_t *) malloc(n * sizeof(rnd_coord_t));
		fc_y = (rnd_coord_t *) malloc(n * sizeof(rnd_coord_t));
		fc_alloced = n;
	}

	vert_opt_begin();
	do {
		vert_opt_loop1(v, first, continue);
		if (rnd_render->gui)
			first = 0; /* if gui, turn on optimization and start to omit vertices */
		fc_x[i] = this_x;
		fc_y[i++] = this_y;
		vert_opt_loop2();
	}
	while ((v = v->next) != pl->head->next);

	if (i < 3) {
		rnd_hid_set_line_width(gc, RND_MM_TO_COORD(0.01));
		rnd_hid_set_line_cap(gc, rnd_cap_round);
		rnd_render->draw_line(gc, last_x, last_y, this_x, this_y);
	}
	else
		rnd_render->fill_polygon(gc, i, fc_x, fc_y);

	vert_opt_end();
}


static void thindraw_contour(rnd_hid_gc_t gc, rnd_pline_t * pl)
{
	rnd_vnode_t *v;


	rnd_hid_set_line_width(gc, 0);
	rnd_hid_set_line_cap(gc, rnd_cap_round);

	/* If the contour is round, use an arc drawing routine. */
	if (pl->is_round) {
		rnd_render->draw_arc(gc, pl->cx, pl->cy, pl->radius, pl->radius, 0, 360);
		return;
	}

	/* Need at least two points in the contour */
	if (pl->head->next == NULL)
		return;

	vert_opt_begin();
	do {
		vert_opt_loop1(v, 0, continue);
		rnd_render->draw_line(gc, last_x, last_y, this_x, this_y);
		vert_opt_loop2();
	}
	while ((v = v->next) != pl->head->next);
	vert_opt_end();
}

static void fill_contour_cb(rnd_pline_t * pl, void *user_data)
{
	rnd_hid_gc_t gc = (rnd_hid_gc_t) user_data;
	rnd_pline_t *local_pl = pl;

	fill_contour(gc, pl);
	rnd_poly_contours_free(&local_pl);
}

static void fill_clipped_contour(rnd_hid_gc_t gc, rnd_pline_t * pl, const rnd_box_t * clip_box)
{
	rnd_pline_t *pl_copy;
	rnd_polyarea_t *clip_poly;
	rnd_polyarea_t *piece_poly;
	rnd_polyarea_t *clipped_pieces;
	rnd_polyarea_t *draw_piece;
	int x;

	/* Optimization: the polygon has no holes; if it is smaller the clip_box,
	   it is safe to draw directly */
	if ((clip_box->X1 <= pl->xmin) && (clip_box->X2 >= pl->xmax) && (clip_box->Y1 <= pl->ymin) && (clip_box->Y2 >= pl->ymax)) {
		fill_contour(gc, pl);
		return;
	}

	clip_poly = rnd_poly_from_rect(clip_box->X1, clip_box->X2, clip_box->Y1, clip_box->Y2);
	rnd_poly_contour_copy(&pl_copy, pl);
	piece_poly = rnd_polyarea_create();
	rnd_polyarea_contour_include(piece_poly, pl_copy);
	x = rnd_polyarea_boolean_free(piece_poly, clip_poly, &clipped_pieces, RND_PBO_ISECT);
	if (x != rnd_err_ok || clipped_pieces == NULL)
		return;

	draw_piece = clipped_pieces;
	do {
		/* NB: The polygon won't have any holes in it */
		fill_contour(gc, draw_piece->contours);
	}
	while ((draw_piece = draw_piece->f) != clipped_pieces);
	rnd_polyarea_free(&clipped_pieces);
}

/* If at least 50% of the bounding box of the polygon is on the screen,
 * lets compute the complete no-holes polygon.
 */
#define BOUNDS_INSIDE_CLIP_THRESHOLD 0.5
static int should_compute_no_holes(pcb_poly_t * poly, const rnd_box_t * clip_box)
{
	rnd_coord_t x1, x2, y1, y2;
	double poly_bounding_area;
	double clipped_poly_area;

	/* If there is no passed clip box, compute the whole thing */
	if (clip_box == NULL)
		return 1;

	x1 = MAX(poly->BoundingBox.X1, clip_box->X1);
	x2 = MIN(poly->BoundingBox.X2, clip_box->X2);
	y1 = MAX(poly->BoundingBox.Y1, clip_box->Y1);
	y2 = MIN(poly->BoundingBox.Y2, clip_box->Y2);

	/* Check if the polygon is outside the clip box */
	if ((x2 <= x1) || (y2 <= y1))
		return 0;

	poly_bounding_area = (double) (poly->BoundingBox.X2 - poly->BoundingBox.X1) *
		(double) (poly->BoundingBox.Y2 - poly->BoundingBox.Y1);

	clipped_poly_area = (double) (x2 - x1) * (double) (y2 - y1);

	if (clipped_poly_area / poly_bounding_area >= BOUNDS_INSIDE_CLIP_THRESHOLD)
		return 1;

	return 0;
}

#undef BOUNDS_INSIDE_CLIP_THRESHOLD

static void pcb_dhlp_fill_pcb_polygon(rnd_hid_gc_t gc, pcb_poly_t * poly, const rnd_box_t * clip_box)
{
	if (!poly->NoHolesValid) {
		/* If enough of the polygon is on-screen, compute the entire
		 * NoHoles version and cache it for later rendering, otherwise
		 * just compute what we need to render now.
		 */
		if (should_compute_no_holes(poly, clip_box))
			pcb_poly_compute_no_holes(poly);
		else
			pcb_poly_no_holes_dicer(poly, clip_box, fill_contour_cb, gc);
	}
	if (poly->NoHolesValid && poly->NoHoles) {
		rnd_pline_t *pl;

		for (pl = poly->NoHoles; pl != NULL; pl = pl->next) {
			if (clip_box == NULL)
				fill_contour(gc, pl);
			else
				fill_clipped_contour(gc, pl, clip_box);
		}
	}

	/* Draw other parts of the polygon if fullpoly flag is set */
	/* NB: No "NoHoles" cache for these */
	if (PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, poly)) {
		pcb_poly_t p = *poly;

		for (p.Clipped = poly->Clipped->f; p.Clipped != poly->Clipped; p.Clipped = p.Clipped->f)
			pcb_poly_no_holes_dicer(&p, clip_box, fill_contour_cb, gc);
	}
}

static int thindraw_hole_cb(rnd_pline_t * pl, void *user_data)
{
	rnd_hid_gc_t gc = (rnd_hid_gc_t) user_data;
	thindraw_contour(gc, pl);
	return 0;
}

static void pcb_dhlp_thindraw_pcb_polygon(rnd_hid_gc_t gc, pcb_poly_t * poly, const rnd_box_t * clip_box)
{
	thindraw_contour(gc, poly->Clipped->contours);
	pcb_poly_holes(poly, clip_box, thindraw_hole_cb, gc);
}

static void pcb_dhlp_uninit(void)
{
	free(fc_x);
	free(fc_y);
	fc_x = fc_y = NULL;
	fc_alloced= 0;
}
