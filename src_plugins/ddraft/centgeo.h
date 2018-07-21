#ifndef PCB_CENTGEO_H
#define PCB_CENTGEO_H

#include "obj_line.h"

/*** Calculate centerpoint intersections of objects (thickness ignored) ***/

/* Calculate the intersection point(s) of two lines and store them in ip if
   ip is not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = overlapping segments (overlap endpoitns are stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_cline_cline(pcb_line_t *Line1, pcb_line_t *Line2, pcb_box_t *ip);

#endif
