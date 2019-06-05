#ifndef PCB_POLYGON1_GEN_H
#define PCB_POLYGON1_GEN_H

#include "global_typedefs.h"
#include "polyarea.h"

void pcb_poly_square_pin_factors(int style, double *xm, double *ym);

pcb_polyarea_t *pcb_poly_from_contour(pcb_pline_t *pl);
void pcb_poly_frac_circle(pcb_pline_t *, pcb_coord_t, pcb_coord_t, pcb_vector_t, int);
pcb_polyarea_t *pcb_poly_from_octagon(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style);
pcb_polyarea_t *pcb_poly_from_rect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2);
pcb_polyarea_t *pcb_poly_from_circle(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius);
pcb_polyarea_t *RoundRect(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2, pcb_coord_t t);


/* generate a polygon of a round or square cap line of a given thickness */
pcb_polyarea_t *pcb_poly_from_line(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick, pcb_bool square);

/* generate a polygon of a round cap arc of a given thickness */
pcb_polyarea_t *pcb_poly_from_arc(pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t astart, pcb_angle_t adelta, pcb_coord_t thick);


#endif
