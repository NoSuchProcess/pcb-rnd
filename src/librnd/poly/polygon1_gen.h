#ifndef PCB_POLYGON1_GEN_H
#define PCB_POLYGON1_GEN_H

#include <librnd/core/global_typedefs.h>
#include <librnd/poly/polyarea.h>

void pcb_poly_square_pin_factors(int style, double *xm, double *ym);

rnd_polyarea_t *pcb_poly_from_contour(rnd_pline_t *pl);
rnd_polyarea_t *pcb_poly_from_contour_autoinv(rnd_pline_t *pl);

rnd_polyarea_t *pcb_poly_from_circle(rnd_coord_t x, rnd_coord_t y, rnd_coord_t radius);
rnd_polyarea_t *pcb_poly_from_octagon(rnd_coord_t x, rnd_coord_t y, rnd_coord_t radius, int style);
rnd_polyarea_t *pcb_poly_from_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2);
rnd_polyarea_t *RoundRect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2, rnd_coord_t t);


/* generate a polygon of a round or square cap line of a given thickness */
rnd_polyarea_t *pcb_poly_from_line(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick, rnd_bool square);

/* generate a polygon of a round cap arc of a given thickness */
rnd_polyarea_t *pcb_poly_from_arc(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, rnd_coord_t thick);

/* Slice up a polyarea-with-holes into a set of polygon islands with no
   holes, within the clip area. If the clip area is all-zero, do not clip.
   Free's main_contour. */
void pcb_polyarea_no_holes_dicer(rnd_polyarea_t *main_contour, rnd_coord_t clipX1, rnd_coord_t clipY1, rnd_coord_t clipX2, rnd_coord_t clipY2, void (*emit)(rnd_pline_t *, void *), void *user_data);

/* Add vertices in a fractional-circle starting from v centered at X, Y and
   going counter-clockwise. Does not include the first point. Last argument is:
   1 for a full circle
   2 for a half circle
   4 for a quarter circle */
void pcb_poly_frac_circle(rnd_pline_t * c, rnd_coord_t X, rnd_coord_t Y, rnd_vector_t v, int range);

/* same but adds the last vertex */
void pcb_poly_frac_circle_end(rnd_pline_t * c, rnd_coord_t X, rnd_coord_t Y, rnd_vector_t v, int range);

#endif
