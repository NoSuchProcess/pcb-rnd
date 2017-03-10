
#ifndef PCB_FGEOMETRY_H
#define PCB_FGEOMETRY_H

#include "obj_common.h"

typedef struct
{
	double x;
	double y;
} fvector;


/* flines should be created through fline_create* functions.
 * Alternatively, they can be created manually as long as
 * direction is normalized
 */
typedef struct
{
	fvector point;
	fvector direction;
} fline;

int fvector_is_null(fvector v);


/* Any vector given to the following functions has to be non-null */   
double fvector_dot (fvector v1, fvector v2);
void fvector_normalize (fvector *v);


fline fline_create(pcb_any_line_t *line);
fline fline_create_from_points(struct pcb_point_s *base_point, struct pcb_point_s *other_point);

int fline_is_valid(fline l);

/* l1.direction and l2.direction are expected to be normalized */
fvector fline_intersection(fline l1, fline l2);

#endif

