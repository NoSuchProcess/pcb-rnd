#ifndef PCB_POLYGON1_GEN_H
#define PCB_POLYGON1_GEN_H

#include <librnd/core/global_typedefs.h>
#include <librnd/poly/polyarea.h>

void pcb_poly_square_pin_factors(int style, double *xm, double *ym);

pcb_polyarea_t *pcb_poly_from_contour(pcb_pline_t *pl);
pcb_polyarea_t *pcb_poly_from_contour_autoinv(pcb_pline_t *pl);

pcb_polyarea_t *pcb_poly_from_circle(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius);
pcb_polyarea_t *pcb_poly_from_octagon(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style);
pcb_polyarea_t *pcb_poly_from_rect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2);
pcb_polyarea_t *RoundRect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2, pcb_coord_t t);


/* generate a polygon of a round or square cap line of a given thickness */
pcb_polyarea_t *pcb_poly_from_line(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick, pcb_bool square);

/* generate a polygon of a round cap arc of a given thickness */
pcb_polyarea_t *pcb_poly_from_arc(pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t astart, pcb_angle_t adelta, pcb_coord_t thick);

/* Slice up a polyarea-with-holes into a set of polygon islands with no
   holes, within the clip area. If the clip area is all-zero, do not clip.
   Free's main_contour. */
void pcb_polyarea_no_holes_dicer(pcb_polyarea_t *main_contour, pcb_coord_t clipX1, pcb_coord_t clipY1, pcb_coord_t clipX2, pcb_coord_t clipY2, void (*emit)(pcb_pline_t *, void *), void *user_data);

/* Add vertices in a fractional-circle starting from v centered at X, Y and
   going counter-clockwise. Does not include the first point. Last argument is:
   1 for a full circle
   2 for a half circle
   4 for a quarter circle */
void pcb_poly_frac_circle(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range);

/* same but adds the last vertex */
void pcb_poly_frac_circle_end(pcb_pline_t * c, pcb_coord_t X, pcb_coord_t Y, pcb_vector_t v, int range);

#endif
