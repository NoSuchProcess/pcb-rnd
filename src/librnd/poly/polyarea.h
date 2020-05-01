/*
      poly_Boolean: a polygon clip library
      Copyright (C) 1997  Alexey Nikitin, Michael Leonov
      leonov@propro.iis.nsk.su

      This library is free software; you can redistribute it and/or
      modify it under the terms of the GNU Library General Public
      License as published by the Free Software Foundation; either
      version 2 of the License, or (at your option) any later version.

      This library is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
      Library General Public License for more details.

      You should have received a copy of the GNU Library General Public
      License along with this library; if not, write to the Free
      Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

      polyarea.h
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1997 Klamer Schutte (minor patches)
*/

#ifndef RND_POLYAREA_H
#define RND_POLYAREA_H

#define RND_PLF_DIR 1
#define RND_PLF_INV 0
#define RND_PLF_MARK 1

typedef rnd_coord_t rnd_vertex_t[2]; /* longing point representation of coordinates */
typedef rnd_vertex_t rnd_vector_t;

#define rnd_vertex_equ(a,b) (memcmp((a),(b),sizeof(rnd_vector_t))==0)
#define rnd_vertex_cpy(a,b) memcpy((a),(b),sizeof(rnd_vector_t))


extern rnd_vector_t rnd_vect_zero;

enum {
	rnd_err_no_memory = 2,
	rnd_err_bad_parm = 3,
	rnd_err_ok = 0
};


typedef struct rnd_cvc_list_s rnd_cvc_list_t;
typedef struct rnd_vnode_s rnd_vnode_t;
struct rnd_cvc_list_s {
	double angle;
	rnd_vnode_t *parent;
	rnd_cvc_list_t *prev, *next, *head;
	char poly, side;
};
struct rnd_vnode_s {
	rnd_vnode_t *next, *prev, *shared;
	struct {
		unsigned int status:3;
		unsigned int mark:1;
		unsigned int in_hub:1;
	} Flags;
	rnd_cvc_list_t *cvc_prev;
	rnd_cvc_list_t *cvc_next;
	rnd_vector_t point;
};

typedef struct rnd_pline_s rnd_pline_t;
struct rnd_pline_s {
	rnd_coord_t xmin, ymin, xmax, ymax;
	rnd_pline_t *next;
	rnd_vnode_t *head;
	unsigned int Count;
	double area;
	rnd_rtree_t *tree;
	rnd_bool is_round;
	rnd_coord_t cx, cy;
	rnd_coord_t radius;
	struct {
		unsigned int status:3;
		unsigned int orient:1;
	} Flags;
};

rnd_pline_t *rnd_poly_contour_new(const rnd_vector_t v);

void rnd_poly_contour_init(rnd_pline_t *c);
void rnd_poly_contour_clear(rnd_pline_t *c); /* clears list of vertices */
void rnd_poly_contour_del(rnd_pline_t **c);

rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, const rnd_pline_t *src);

void rnd_poly_contour_pre(rnd_pline_t *c, rnd_bool optimize); /* prepare contour */
void rnd_poly_contour_inv(rnd_pline_t *c); /* invert contour */

rnd_vnode_t *rnd_poly_node_create(rnd_vector_t v);

void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node);
void rnd_poly_vertex_include_force(rnd_vnode_t *after, rnd_vnode_t *node); /* do not remove nodes even if on the same line */
void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t *node);

rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dest, rnd_vector_t po);

/**********************************************************************/

struct rnd_polyarea_s {
	rnd_polyarea_t *f, *b;
	rnd_pline_t *contours;
	rnd_rtree_t *contour_tree;
};

rnd_bool rnd_polyarea_m_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *srcfst);
void rnd_polyarea_m_include(rnd_polyarea_t **list, rnd_polyarea_t *a);

rnd_bool rnd_polyarea_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *src);
rnd_bool rnd_polyarea_copy1(rnd_polyarea_t *dst, const rnd_polyarea_t *src);

rnd_bool rnd_polyarea_contour_include(rnd_polyarea_t *p, rnd_pline_t *c);
rnd_bool rnd_polyarea_contour_exclude(rnd_polyarea_t *p, rnd_pline_t *c);


rnd_bool rnd_polyarea_contour_check(rnd_pline_t *a);

rnd_bool rnd_polyarea_contour_inside(rnd_polyarea_t *c, rnd_vector_t v0);
rnd_bool rnd_polyarea_touching(rnd_polyarea_t *p1, rnd_polyarea_t *p2);

/*** tools for clipping ***/

/* checks whether point lies within contour independently of its orientation */

int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t v);
int rnd_poly_contour_in_contour(rnd_pline_t *poly, rnd_pline_t *inner);
rnd_polyarea_t *rnd_polyarea_create(void);

void rnd_polyarea_free(rnd_polyarea_t **p);
void rnd_polyarea_init(rnd_polyarea_t *p);
void rnd_poly_contours_free(rnd_pline_t **pl);
rnd_bool rnd_poly_valid(rnd_polyarea_t *p);

enum rnd_poly_bool_op_e {
	RND_PBO_UNITE,
	RND_PBO_ISECT,
	RND_PBO_SUB,
	RND_PBO_XOR
};

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_len2(rnd_vector_t v1);

int rnd_vect_inters2(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D, rnd_vector_t S1, rnd_vector_t S2);

int rnd_polyarea_boolean(const rnd_polyarea_t *a, const rnd_polyarea_t *b, rnd_polyarea_t **res, int action);
int rnd_polyarea_boolean_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **res, int action);
int rnd_polyarea_and_subtract_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **aandb, rnd_polyarea_t **aminusb);
int rnd_polyarea_save(rnd_polyarea_t *PA, char *fname);

/* calculate the bounding box of a rnd_polyarea_t and save result in b */
void rnd_polyarea_bbox(rnd_polyarea_t *p, rnd_rnd_box_t *b);

/* Move each point of pa1 by dx and dy */
void rnd_polyarea_move(rnd_polyarea_t *pa1, rnd_coord_t dx, rnd_coord_t dy);

/*** Tools for building polygons for common object shapes ***/


#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
double rnd_round(double x); /* from math_helper.h */

/* Calculate an endpoint of an arc and return the result in *x;*y */
RND_INLINE void rnd_arc_get_endpt(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, int which, rnd_coord_t *x, rnd_coord_t *y)
{
	if (which == 0) {
		*x = rnd_round((double)cx - (double)width * cos(astart * (M_PI/180.0)));
		*y = rnd_round((double)cy + (double)height * sin(astart * (M_PI/180.0)));
	}
	else {
		*x = rnd_round((double)cx - (double)width * cos((astart + adelta) * (M_PI/180.0)));
		*y = rnd_round((double)cy + (double)height * sin((astart + adelta) * (M_PI/180.0)));
	}
}

/*** constants for the poly shape generator ***/

/* polygon diverges from modelled arc no more than MAX_ARC_DEVIATION * thick */
#define RND_POLY_ARC_MAX_DEVIATION 0.02

#define RND_POLY_CIRC_SEGS 40
#define RND_POLY_CIRC_SEGS_F ((float)RND_POLY_CIRC_SEGS)

/* adjustment to make the segments outline the circle rather than connect
   points on the circle: 1 - cos (\alpha / 2) < (\alpha / 2) ^ 2 / 2 */
#define RND_POLY_CIRC_RADIUS_ADJ \
	(1.0 + M_PI / RND_POLY_CIRC_SEGS_F * M_PI / RND_POLY_CIRC_SEGS_F / 2.0)

/*** special purpose, internal tools ***/
/* Convert a struct seg *obj extracted from a pline->tree into coords */
void rnd_polyarea_get_tree_seg(void *obj, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2);

/* create a (rnd_rtree_t *) of each seg derived from src */
void *rnd_poly_make_edge_tree(rnd_pline_t *src);


/* EPSILON^2 for endpoint matching; the bool algebra code is not
   perfect and causes tiny self intersections at the end of sharp
   spikes. Accept at most 10 nanometer of such intersection */
#define RND_POLY_ENDP_EPSILON 100

/*** generic geo ***/
int rnd_point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P);

#endif /* RND_POLYAREA_H */
