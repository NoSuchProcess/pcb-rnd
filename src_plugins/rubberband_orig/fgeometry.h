
#ifndef PCB_FGEOMETRY_H
#define PCB_FGEOMETRY_H

#include "obj_common.h"

typedef struct pcb_fvector_s {
	double x;
	double y;
} pcb_fvector_t;


/* flines should be created through fline_create* functions.
 * Alternatively, they can be created manually as long as
 * direction is normalized
 */
typedef struct pcb_fline_s {
	pcb_fvector_t point;
	pcb_fvector_t direction;
} pcb_fline_t;

int pcb_fvector_is_null(pcb_fvector_t v);


/* Any vector given to the following functions has to be non-null */
double pcb_fvector_dot(pcb_fvector_t v1, pcb_fvector_t v2);
void pcb_fvector_normalize(pcb_fvector_t *v);


pcb_fline_t pcb_fline_create(pcb_any_line_t *line);
pcb_fline_t pcb_fline_create_from_points(struct pcb_point_s *base_point, struct pcb_point_s *other_point);

int pcb_fline_is_valid(pcb_fline_t l);

/* l1.direction and l2.direction are expected to be normalized */
pcb_fvector_t pcb_fline_intersection(pcb_fline_t l1, pcb_fline_t l2);

#endif
