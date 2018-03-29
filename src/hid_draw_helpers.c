#include "config.h"
#include "hid.h"
#include "polygon.h"
#include "obj_poly.h"

static void fill_contour(pcb_hid_gc_t gc, pcb_pline_t * pl)
{
	pcb_coord_t *x, *y, n, i = 0;
	pcb_vnode_t *v;

	n = pl->Count;
	x = (pcb_coord_t *) malloc(n * sizeof(*x));
	y = (pcb_coord_t *) malloc(n * sizeof(*y));

	for (v = &pl->head; i < n; v = v->next) {
		x[i] = v->point[0];
		y[i++] = v->point[1];
	}

	pcb_gui->fill_polygon(gc, n, x, y);

	free(x);
	free(y);
}

static void thindraw_contour(pcb_hid_gc_t gc, pcb_pline_t * pl)
{
	pcb_vnode_t *v;
	pcb_coord_t last_x, last_y;
	pcb_coord_t this_x, this_y;

	pcb_gui->set_line_width(gc, 0);
	pcb_gui->set_line_cap(gc, Round_Cap);

	/* If the contour is round, use an arc drawing routine. */
	if (pl->is_round) {
		pcb_gui->draw_arc(gc, pl->cx, pl->cy, pl->radius, pl->radius, 0, 360);
		return;
	}

	/* Need at least two points in the contour */
	if (pl->head.next == NULL)
		return;

	last_x = pl->head.point[0];
	last_y = pl->head.point[1];
	v = pl->head.next;

	do {
		this_x = v->point[0];
		this_y = v->point[1];

		pcb_gui->draw_line(gc, last_x, last_y, this_x, this_y);
		/* pcb_gui->fill_circle (gc, this_x, this_y, 30); */

		last_x = this_x;
		last_y = this_y;
	}
	while ((v = v->next) != pl->head.next);
}

static void fill_contour_cb(pcb_pline_t * pl, void *user_data)
{
	pcb_hid_gc_t gc = (pcb_hid_gc_t) user_data;
	pcb_pline_t *local_pl = pl;

	fill_contour(gc, pl);
	pcb_poly_contours_free(&local_pl);
}

static void fill_clipped_contour(pcb_hid_gc_t gc, pcb_pline_t * pl, const pcb_box_t * clip_box)
{
	pcb_pline_t *pl_copy;
	pcb_polyarea_t *clip_poly;
	pcb_polyarea_t *piece_poly;
	pcb_polyarea_t *clipped_pieces;
	pcb_polyarea_t *draw_piece;
	int x;

	clip_poly = pcb_poly_from_rect(clip_box->X1, clip_box->X2, clip_box->Y1, clip_box->Y2);
	pcb_poly_contour_copy(&pl_copy, pl);
	piece_poly = pcb_polyarea_create();
	pcb_polyarea_contour_include(piece_poly, pl_copy);
	x = pcb_polyarea_boolean_free(piece_poly, clip_poly, &clipped_pieces, PCB_PBO_ISECT);
	if (x != pcb_err_ok || clipped_pieces == NULL)
		return;

	draw_piece = clipped_pieces;
	do {
		/* NB: The polygon won't have any holes in it */
		fill_contour(gc, draw_piece->contours);
	}
	while ((draw_piece = draw_piece->f) != clipped_pieces);
	pcb_polyarea_free(&clipped_pieces);
}

/* If at least 50% of the bounding box of the polygon is on the screen,
 * lets compute the complete no-holes polygon.
 */
#define BOUNDS_INSIDE_CLIP_THRESHOLD 0.5
static int should_compute_no_holes(pcb_poly_t * poly, const pcb_box_t * clip_box)
{
	pcb_coord_t x1, x2, y1, y2;
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

void pcb_dhlp_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t * poly, const pcb_box_t * clip_box)
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
		pcb_pline_t *pl;

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

static int thindraw_hole_cb(pcb_pline_t * pl, void *user_data)
{
	pcb_hid_gc_t gc = (pcb_hid_gc_t) user_data;
	thindraw_contour(gc, pl);
	return 0;
}

void pcb_dhlp_thindraw_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t * poly, const pcb_box_t * clip_box)
{
	thindraw_contour(gc, poly->Clipped->contours);
	pcb_poly_holes(poly, clip_box, thindraw_hole_cb, gc);
}

void pcb_dhlp_draw_helpers_init(pcb_hid_t * hid)
{
	hid->fill_pcb_polygon      = pcb_dhlp_fill_pcb_polygon;
	hid->thindraw_pcb_polygon  = pcb_dhlp_thindraw_pcb_polygon;
}
