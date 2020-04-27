#ifndef PCB_CENTGEO_H
#define PCB_CENTGEO_H

#include "obj_line.h"

/*** Calculate centerpoint intersections of objects (thickness ignored) ***/

/* Calculate the intersection point(s) of two lines and store them in ip
   and/or offs (on Line1) if they are not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = overlapping segments (overlap endpoitns are stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_cline_cline(pcb_line_t *Line1, pcb_line_t *Line2, rnd_box_t *ip, double offs[2]);

/* Calculate the intersection point(s) of a lines and an arc and store them
   in ip and/or offs if they are not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = two intersections (stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_cline_carc(pcb_line_t *Line, pcb_arc_t *Arc, rnd_box_t *ip, double offs_line[2]);
int pcb_intersect_carc_cline(pcb_arc_t *Arc, pcb_line_t *Line, rnd_box_t *ip, double offs_arc[2]);

/* Calculate the intersection point(s) of two arcs and store them in ip
   and/or offs (on Line1) if they are not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = two intersections or overlapping segments (overlap endpoints are stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_carc_carc(pcb_arc_t *Arc1, pcb_arc_t *Arc2, rnd_box_t *ip, double offs[2]);


/* Calculate the point on an object corresponding to a [0..1] offset and store
   the result in dstx;dsty. */
void pcb_cline_offs(pcb_line_t *line, double offs, rnd_coord_t *dstx, rnd_coord_t *dsty);
void pcb_carc_offs(pcb_arc_t *arc, double offs, rnd_coord_t *dstx, rnd_coord_t *dsty);

/* Project a point (px;py) onto an object and return the offset from point1 */
double pcb_cline_pt_offs(pcb_line_t *line, rnd_coord_t px, rnd_coord_t py);
double pcb_carc_pt_offs(pcb_arc_t *arc, rnd_coord_t px, rnd_coord_t py);


#endif
