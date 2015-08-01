#include <stdlib.h>
#include <math.h>
#include "coordgeo.h"
cg_obj_t *cg_objs = NULL;
int cg_objs_used = 0, cg_objs_alloced = 0;

#define DEGMPY 10000.0
#define PI 3.141592654

#define OBJ(i) cg_objs[i]
#define OBJ_VALID(i) ((i >= 0) && (i < cg_objs_used))

/* todo */
#define invalid -0xFFFF

static int oalloc()
{
	int id;
	if (cg_objs_used >= cg_objs_alloced) {
		cg_objs_alloced = cg_objs_used + 128;
		cg_objs = realloc(cg_objs, sizeof(cg_obj_t) * cg_objs_alloced);
	}
	id = cg_objs_used;
	cg_objs_used++;
	return id;
}

static void ofree(int id)
{
	/* todo */
}

int cg_new_object(cg_obj_type_t type, int x1, int y1, int x2, int y2, int r)
{
	int id;

	id = oalloc();
	OBJ(id).type = type;
	OBJ(id).x1 = x1;
	OBJ(id).y1 = y1;
	OBJ(id).x2 = x2;
	OBJ(id).y2 = y2;
	OBJ(id).r = r;
	return id;
}

static inline int cg_clone_object_(cg_obj_t *o)
{
	int id;

	id = oalloc();
	OBJ(id) = *o;
	return id;
}

int cg_clone_object(int objID)
{
	return cg_clone_object_(&OBJ(objID));
}

/* destroys an object by ID */
void cg_destroy_object(int ID)
{
	ofree(ID);
}

/* -- field accessors -- */
cg_obj_type_t cg_get_type(int objID)
{
	return OBJ(objID).type;
}

int cg_get_coord(int objID, cg_coord_t coordname)
{
	switch(coordname) {
		case CG_X1: return OBJ(objID).x1;
		case CG_Y1: return OBJ(objID).y1;
		case CG_X2: return OBJ(objID).x2;
		case CG_Y2: return OBJ(objID).y2;
		case CG_R:  return OBJ(objID).r;
	}
	return invalid;
}

void cg_set_coord(int objID, cg_coord_t coordname, int newvalue)
{
	switch(coordname) {
		case CG_X1: OBJ(objID).x1 = newvalue;
		case CG_Y1: OBJ(objID).y1 = newvalue;
		case CG_X2: OBJ(objID).x2 = newvalue;
		case CG_Y2: OBJ(objID).y2 = newvalue;
		case CG_R:  OBJ(objID).r  = newvalue;
	}
}


/* -- object operations -- */
double cg_dist_(int x1, int y1, int x2, int y2)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}

int cg_solve_quad(double a, double b, double c, double *x1, double *x2)
{
	double d;

	d = b * b - 4 * a * c;
	if (d < 0)
		return 0;
	if (d != 0)
		d = sqrt(d);

	if (x1 != NULL)
		*x1 = (-b + d) / (2 * a);

	if (x2 != NULL)
		*x2 = (-b - d) / (2 * a);

	if (d == 0)
		return 1;
	return 2;
}


/* return ID to a new object which is parallel obj in distance dist
   - for a line dist > 0 means a new line right to the original
     (looking from x1;y1 towards x2;y2)
   - for arc the same rule applies, looking from angle x2 towards y2
   - for circle dist > 0 means smaller concentric circle
   - for point and vector always returns invalid
*/
int cg_para_obj(int objID, int dist)
{
	cg_obj_t v1, v2;
	int rid;

	switch(OBJ(objID).type) {
		case CG_POINT:
		case CG_VECTOR:
			return -1;
		case CG_ARC:
			/* TODO */
			abort();
			break;
		case CG_CIRCLE:
			rid = cg_clone_object(objID);
			OBJ(rid).r -= dist;
			return rid;
		case CG_LINE:
			cg_perp_line_(&OBJ(objID), &v1, OBJ(objID).x1, OBJ(objID).y1, dist);
			cg_perp_line_(&OBJ(objID), &v2, OBJ(objID).x2, OBJ(objID).y2, dist);
			rid = oalloc();
			OBJ(rid).type = CG_LINE;
			OBJ(rid).x1 = v1.x2;
			OBJ(rid).y1 = v1.y2;
			OBJ(rid).x2 = v2.x2;
			OBJ(rid).y2 = v2.y2;
			OBJ(rid).r  = 0;
			return rid;
	}
	return -1;
}

void cg_perp_line_(const cg_obj_t *i, cg_obj_t *o, int xp, int yp, int len)
{
	double dx, dy, dl;

	dx = -(i->y2 - i->y1);
	dy = +(i->x2 - i->x1);
	dl = sqrt(dx * dx + dy * dy);
	dx = dx / dl;
	dy = dy / dl;

	o->type = CG_LINE;
	o->x1 = xp;
	o->y1 = yp;
	o->x2 = round((double)xp + dx * (double)len);
	o->y2 = round((double)yp + dy * (double)len);
	o->r  = 0;
}


/* return ID to a new line which is perpendicular to line1 and starts at
   point xp;yp and is len long */
int cg_perp_line(int line1ID, int xp, int yp, int len)
{
	int id;

	id = oalloc();
	cg_perp_line_(&OBJ(line1ID), &OBJ(id), xp, yp, len);

	return id;
}

int cg_ison_line_(cg_obj_t *l, int x, int y, int extend)
{
	int vx, vy;

	vx = l->x2 - l->x1;
	vy = l->y2 - l->y1;

	/* is the point on the extended line? */
	if ((vy * x - vx * y) != (vy * l->x1 - vy * l->y1))
		return 0;

	if ((!extend) && ((x < l->x1) || (x > l->x2)))
		return 0;
	return 1;
}

int cg_ison_arc_(cg_obj_t *a, int x, int y, int extend)
{
	double deg;

	/* is the point on the circle? */
	if (cg_dist_(a->x1, a->y1, x, y) != a->r)
		return 0;

	/* for circles (or an extended arc, which is just a circle) this means the point must be on */
	if (extend || (a->type == CG_CIRCLE))
		return 1;

	/* check if the point is between start and end angles */
	deg = atan2(y - a->y1, x - a->x1) * DEGMPY;
	if ((deg >= a->x2) && (deg >= a->y2))
		return 1;

	return 0;
}

int cg_ison_point_(cg_obj_t *p, int x, int y)
{
	return (p->x1 == x) && (p->y1 == y);
}

int cg_ison_(cg_obj_t *o, int x, int y, int extend)
{
	switch(o->type) {
		case CG_POINT:  return cg_ison_point_(o, x, y);
		case CG_VECTOR: return 1;
		case CG_LINE:   return cg_ison_line_(o, x, y, extend);
		case CG_ARC:    return cg_ison_arc_(o, x, y, extend);
		case CG_CIRCLE: return cg_ison_circle_(o, x, y);
	}

	/* can't get here */
	abort();
	return 0;
}

/* intersection of 2 lines (always returns 1) */
int cg_intersect_ll(cg_obj_t *l1, cg_obj_t *l2, cg_obj_t *o)
{
	double ua, ub, xi, yi, X1, Y1, X2, Y2, X3, Y3, X4, Y4, tmp;

	/* maths from http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/ */

	X1 = l1->x1;
	X2 = l1->x2;
	X3 = l2->x1;
	X4 = l2->x2;
	Y1 = l1->y1;
	Y2 = l1->y2;
	Y3 = l2->y1;
	Y4 = l2->y2;

	tmp = ((Y4-Y3)*(X2-X1) - (X4-X3)*(Y2-Y1));
	ua=((X4-X3)*(Y1-Y3) - (Y4-Y3)*(X1-X3)) / tmp;
	ub=((X2-X1)*(Y1-Y3) - (Y2-Y1)*(X1-X3)) / tmp;
	xi = X1 + ua * (X2 - X1);
	yi = Y1 + ua * (Y2 - Y1);

	o->type = CG_LINE;
	o->x1 = o->x2 = round(xi);
	o->y1 = o->y2 = round(yi);
	o->r = 0;
	return 1;
}

/* intersections of a line and a circle (returns number of possible intersections) */
int cg_intersect_lc(cg_obj_t *l, cg_obj_t *c, cg_obj_t *o)
{
	int x1, y1, vx, vy, a, vx2, ints;
	double A, B, C, X1, X2, Y1, Y2;

	/* first we transform the line to the coordinate system with origo in the
	   center of the circle - this will simplify the equations */
	x1 = l->x1 - c->x1;
	y1 = l->y1 - c->y1;
	vx = l->x2 - l->x1;
	vy = l->y2 - l->y1;

	/* some cache */
	a = vy * x1 - vx * y1;
	vx2 = vx * vx;

	/* this quadratic equation results in the two intersections, X1 and X2 */
	A = 1+vy*vy/vx2;
	B = - 2*a*vy / (vx2);
	C = a*a / vx2 - c->r * c->r;

	ints = cg_solve_quad(A, B, C, &X1, &X2);
	if (ints == 0)
		return 0;

	/* knowing X1 and X2 we can easily get Y1 and Y2 */
	Y1 = (a - vy * X1) / (-vx);
	Y2 = (a - vy * X2) / (-vx);

	/* transoform back to the original coordinate system */
	X1 += c->x1;
	X2 += c->x1;
	Y1 += c->y1;
	Y2 += c->y1;

	/* set up return line and return number of intersections */
	o->type = CG_LINE;
	o->x1 = X1;
	o->y1 = Y1;
	o->x2 = X2;
	o->y2 = Y2;
	o->r  = 0;
	return ints;
}

/* intersections of 2 circles (returns number of possible intersections) */
int cg_intersect_cc(cg_obj_t *c1, cg_obj_t *c2, cg_obj_t *o)
{
	double d, a, h;
	int P2x, P2y;

	d = cg_dist_(c1->x1, c1->y1, c2->x1, c2->y1);

	if (d > c1->r + c2->r)
		return 0; /* separate */
	if (d < abs(c1->r - c2->r))
		return 0; /* contained */
	if ((d == 0) && (c1->r == c2->r))
		return -1; /* they are the same circle */

	/* some temps */
	a = (double)(c1->r * c1->r - c2->r * c2->r + d * d) / (2.0 * d);
	h = sqrt(c1->r * c1->r - a * a);
	P2x = c1->x1 + a * (c2->x1 - c1->x1) / d;
	P2y = c1->y1 + a * (c2->y1 - c1->y1) / d;

	/* final coordinates */
	o->type = CG_LINE;
	o->x1 = P2x + h * (c2->y1 - c1->y1) / d;
	o->y1 = P2y - h * (c2->x1 - c1->x1) / d;
	o->x2 = P2x - h * (c2->y1 - c1->y1) / d;
	o->y2 = P2y + h * (c2->x1 - c1->x1) / d;

	if (d == c1->r + c2->r)
		return 1;
	return 2;
}


int cg_intersect_(cg_obj_t *i1, cg_obj_t *i2, cg_obj_t *o)
{
	switch(i1->type) {
		case CG_VECTOR:
			/* invalid */
			return -1;
		case CG_POINT:
			/* TODO: ison */
			break;
		case CG_LINE:
			switch(i2->type) {
				case CG_VECTOR:
					/* invalid */
					return -1;
				case CG_POINT:
					/* TODO: ison */
					break;
				case CG_LINE:
					return cg_intersect_ll(i1, i2, o);
				case CG_CIRCLE:
				case CG_ARC:
					return cg_intersect_lc(i1, i2, o);
			}
			return -1;
		case CG_CIRCLE:
		case CG_ARC:
			switch(i2->type) {
				case CG_VECTOR:
					/* invalid */
					return -1;
				case CG_POINT:
					/* TODO: ison */
					break;
				case CG_LINE:
					return cg_intersect_lc(i2, i1, o);
				case CG_CIRCLE:
				case CG_ARC:
					return cg_intersect_cc(i1, i2, o);
			}
			return -1;
	}
	return -1;
}

int cg_intersect(int obj1ID, int obj2ID, int extend)
{
	cg_obj_t res;
	int ints;

	ints = cg_intersect_(&OBJ(obj1ID), &OBJ(obj2ID), &res);

	/* if we needed to extend, we shouldn't care if the intersections
	   are on the objects. */
	if (extend)
		return cg_clone_object_(&res);


	while(1) {
		if (ints == 0)
			return -1;

		if ((!cg_ison_(&OBJ(obj1ID), res.x1, res.y1, 0)) || (!cg_ison_(&OBJ(obj2ID), res.x1, res.y1, 0))) {
			/* x1;y1 is not on the objects. make x2;y2 the new x1;y1 and test again */
			res.x1 = res.x2;
			res.y1 = res.y2;
			ints--;
		}
		else
			break;
	}

	/* x1;y1 is on the objects; check x2;y2 if we still have 2 intersections */
	if ((ints == 2) && ((!cg_ison_(&OBJ(obj1ID), res.x2, res.y2, 0)) || (!cg_ison_(&OBJ(obj2ID), res.x2, res.y2, 0)))) {
		/* x2;y2 not on the objects, kill it */
		res.x2 = res.x1;
		res.y2 = res.y1;
		ints = 1;
	}

	return cg_clone_object_(&res);
}

/* Truncate an object at x;y. Point x;y must be on the object. Optionally
   invert which part is kept. Default:
    - line: keep the part that originates from x1;y1
    - arc: keep the segment that is closer to the starting angle (x2)
    - circle: always fails
    - point: always fails
    - vector: always fails
   Returns 0 on success.
*/
int cg_truncate_(cg_obj_t *o, int x, int y, int invert_kept)
{
	double a;

	/* point is not on the object */
	if (!cg_ison_(o, x, y, 0))
		return 1;

	switch(o->type) {
		case CG_VECTOR:
		case CG_POINT:
		case CG_CIRCLE:
			return 2;
		case CG_LINE:
			if (!invert_kept) {
				o->x2 = x;
				o->y2 = y;
			}
			else {
				o->x1 = x;
				o->y1 = y;
			}
			break;
		case CG_ARC:
			a = atan2(y - o->y1, x - o->x1) * DEGMPY;
			if (!invert_kept)
				o->y2 = a;
			else
				o->x2 = a;
			break;
	}
	return 0;
}

int cg_truncate(int objID, int x, int y, int invert_kept)
{
	return cg_truncate_(&OBJ(objID), x, y, invert_kept);
}

/* cut target object with cutting edge object; optionally invert which
   part is kept of target. Default:
    - when line cuts, looking from x1;y1 towards x2;y2, right is kept
    - when circle or arc cuts, we are "walking" clock-wise, right is kept
      (in short, inner part is kept)
   Returns 0 on success.
*/
int cg_cut_(cg_obj_t *t, cg_obj_t *c, int invert_kept)
{
	cg_obj_t intersects;
	int num_intersects, ni;

	num_intersects = cg_intersect_(t, c, &intersects);
	if (num_intersects < 1)
		return 1;

	for(; num_intersects > 0; num_intersects--)
	{
		switch(c->type)  {
			case CG_POINT:
			case CG_VECTOR:
				return 1;
			case CG_LINE:
				/* TODO: leftof */
				return 1;
			case CG_CIRCLE:
			case CG_ARC:
				/* TODO: leftof */
				break;
		}
		/* shift intersection */
		intersects.x1 = intersects.x2;
		intersects.y1 = intersects.y2;
	}
	return 0;
}

int cg_cut(int targetID, int cutting_edgeID, int invert_kept);

cg_obj_t *cg_get_object(int ID)
{
	if (OBJ_VALID(ID))
		return &OBJ(ID);
	else
		return NULL;
}

int cg_simplify_object_(cg_obj_t *o)
{
	int ret = 0; /* no direct returns because CG_ARC can have more than one optimization */
	switch(o->type) {
		case CG_LINE:
			if ((o->x1 == o->x2) && (o->y1 == o->y2)) {
				o->type = CG_POINT;
				ret++;
				break;
			}
			break;
		case CG_ARC:
			if ((fabs(o->x2 - o->y2) * DEGMPY) >= 2*PI) {
				o->type = CG_CIRCLE;
				ret++;
			}
			/* intended fall trough for r==0 check */
		case CG_CIRCLE:
			if (o->r == 0) {
				o->type = CG_POINT;
				ret++;
				break;
			}
			break;
		default:
			/* no simplification possible */
			;
	}
	return ret;
}


int cg_simplify_object(int ID)
{
	if (OBJ_VALID(ID))
		return cg_simplify_object_(&OBJ(ID));
	return -1;
}