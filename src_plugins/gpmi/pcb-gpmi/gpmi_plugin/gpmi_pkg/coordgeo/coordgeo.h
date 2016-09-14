#include <gpmi.h>
#include "src/pcb-printf.h"

#undef snprintf
#define snprintf pcb_snprintf

typedef enum cg_obj_type_e {
	CG_POINT,   /* x1;y1 */
	CG_LINE,    /* from x1;y1 to x2;y2 */
	CG_VECTOR,  /* x1;y1 (not associated with a point) */
	CG_CIRCLE,  /* center x1;y1, radius r */
	CG_ARC      /* center x1;y1, radius r, from x2 rad to y2 rad */
} cg_obj_type_t;

typedef enum cg_coord_e {
	CG_X1,
	CG_Y1,
	CG_X2,
	CG_Y2,
	CG_R
} cg_coord_t;


/* -- object administration -- */
/* returns ID to the new object */
int cg_new_object(cg_obj_type_t type, int x1, int y1, int x2, int y2, int r);

/* cloned an existing object and returns ID to the new object */
int cg_clone_object(int objID);

/* destroys an object by ID */
void cg_destroy_object(int ID);

/* -- field accessors -- */
cg_obj_type_t cg_get_type(int objID);
int cg_get_coord(int objID, cg_coord_t coordname);
void cg_set_coord(int objID, cg_coord_t coordname, int newvalue);


/* -- object operations -- */
/* return ID to a new object which is parallel obj in distance dist
   - for a line dist > 0 means a new line right to the original
     (looking from x1;y1 towards x2;y2)
   - for arc the same rule applies, looking from angle x2 towards y2
   - for circle dist > 0 means smaller concentric circle
   - for point and vector always returns invalid
*/
int cg_para_obj(int objID, int dist);

/* return ID to a new line which is perpendicular to line1 and starts at
   point xp;yp and is len long */
int cg_perp_line(int line1ID, int xp, int yp, int len);

/* returns ID to a line which marks the intersection(s) of two objects or
   returns -1 if there's no intersection. If there's only one intersection,
   the start and end point of the line is the same point. With the current
   primitives, there can not be more than 2 intersections between 2 objects,
   except when they have infinite amount of intersections:
    - overlapping lines
    - overlapping arcs
    - two circles with the same radius and center
   If extend is non-zero, the objects are extended during the calculation.
   For lines this means infinite length, for arcs this means handling them as
   circles.
*/
int cg_intersect(int obj1ID, int obj2ID, int extend);

/* Truncate an object at x;y. Point x;y must be on the object. Optionally
   invert which part is kept. Default:
    - line: keep the part that originates from x1;y1
    - arc: keep the segment that is closer to the starting angle (x2)
    - circle: always fails
    - point: always fails
    - vector: always fails
   Returns 0 on success.
*/
int cg_truncate(int objID, int x, int y, int invert_kept);

/* cut target object with cutting edge object; optionally invert which
   part is kept of target. Defaults:
    - when line cuts, looking from x1;y1 towards x2;y2, right is kept
    - when circle or arc cuts, we are "walking" clock-wise, right is kept
      (in short, inner part is kept)
    - vector and point fails
   Returns 0 on success.
*/
int cg_cut(int targetID, int cutting_edgeID, int invert_kept);

/* Convert object to simpler form, if possible:
    - if x1;y1 and x2;y2 are equal in a line, it is converted to a point
    - if radius of a circle or arc is zero, it is converted to a point
    - if an arc is a full circle, convert it to a circle
   Returns number of simplifications (0, 1 or 2) issued or -1 for
   invalid object ID.
*/
int cg_simplify_object(int ID);


/* these are for C API, scripts have no direct access */
typedef struct cg_obj_s {
	cg_obj_type_t type;
	int x1, y1, x2, y2;
	int r;
} cg_obj_t;

extern cg_obj_t *cg_objs;
extern int cg_objs_used, cg_objs_alloced;

nowrap cg_obj_t *cg_get_object(int ID);

/* create a line in o that is perpendicular to i and starts at point xp and yp */
nowrap void cg_perp_line_(const cg_obj_t *i, cg_obj_t *o, int xp, int yp, int len);

/* distance */
nowrap double cg_dist_(int x1, int y1, int x2, int y2);

/* is point on the object? Extend means the object is extended (lines become
   infinite long, arcs become circles). Vector always returns 1. */
nowrap int cg_ison_(cg_obj_t *o, int x, int y, int extend);
nowrap int cg_ison_line_(cg_obj_t *l, int x, int y, int extend);
nowrap int cg_ison_arc_(cg_obj_t *a, int x, int y, int extend);
nowrap int cg_ison_point_(cg_obj_t *p, int x, int y);
#define cg_ison_circle_(c, x, y) cg_ison_arc_(c, x, y, 1)

/* these calls are the low level versions of the ones defined for scripts above */
nowrap int cg_simplify_object_(cg_obj_t *o);

