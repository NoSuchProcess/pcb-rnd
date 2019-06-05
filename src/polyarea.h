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

#ifndef	PCB_POLYAREA_H
#define	PCB_POLYAREA_H

#define PCB_PLF_DIR 1
#define PCB_PLF_INV 0
#define PCB_PLF_MARK 1

typedef pcb_coord_t pcb_vertex_t[2];				/* longing point representation of
																   coordinates */
typedef pcb_vertex_t pcb_vector_t;

#define pcb_vertex_equ(a,b) (memcmp((a),(b),sizeof(pcb_vector_t))==0)
#define pcb_vertex_cpy(a,b) memcpy((a),(b),sizeof(pcb_vector_t))


extern pcb_vector_t vect_zero;

enum {
	pcb_err_no_memory = 2,
	pcb_err_bad_parm = 3,
	pcb_err_ok = 0
};


typedef struct pcb_cvc_list_s pcb_cvc_list_t;
typedef struct pcb_vnode_s pcb_vnode_t;
struct pcb_cvc_list_s {
	double angle;
	pcb_vnode_t *parent;
	pcb_cvc_list_t *prev, *next, *head;
	char poly, side;
};
struct pcb_vnode_s {
	pcb_vnode_t *next, *prev, *shared;
	struct {
		unsigned int status:3;
		unsigned int mark:1;
		unsigned int in_hub:1;
	} Flags;
	pcb_cvc_list_t *cvc_prev;
	pcb_cvc_list_t *cvc_next;
	pcb_vector_t point;
};

typedef struct pcb_pline_s pcb_pline_t;
struct pcb_pline_s {
	pcb_coord_t xmin, ymin, xmax, ymax;
	pcb_pline_t *next;
	pcb_vnode_t head;
	unsigned int Count;
	double area;
	pcb_rtree_t *tree;
	pcb_bool is_round;
	pcb_coord_t cx, cy;
	pcb_coord_t radius;
	struct {
		unsigned int status:3;
		unsigned int orient:1;
	} Flags;
};

pcb_pline_t *pcb_poly_contour_new(pcb_vector_t v);

void pcb_poly_contour_init(pcb_pline_t * c);
void pcb_poly_contour_clear(pcb_pline_t * c);	/* clears list of vertices */
void pcb_poly_contour_del(pcb_pline_t ** c);

pcb_bool pcb_poly_contour_copy(pcb_pline_t ** dst, pcb_pline_t * src);

void pcb_poly_contour_pre(pcb_pline_t * c, pcb_bool optimize);	/* prepare contour */
void pcb_poly_contour_inv(pcb_pline_t * c);	/* invert contour */

pcb_vnode_t *pcb_poly_node_create(pcb_vector_t v);

void pcb_poly_vertex_include(pcb_vnode_t * after, pcb_vnode_t * node);
void pcb_poly_vertex_exclude(pcb_vnode_t * node);

/**********************************************************************/

struct pcb_polyarea_s {
	pcb_polyarea_t *f, *b;
	pcb_pline_t *contours;
	pcb_rtree_t *contour_tree;
};

pcb_bool pcb_polyarea_m_copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * srcfst);
void pcb_polyarea_m_include(pcb_polyarea_t ** list, pcb_polyarea_t * a);

pcb_bool pcb_polyarea_copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * src);
pcb_bool pcb_polyarea_copy1(pcb_polyarea_t * dst, const pcb_polyarea_t * src);

pcb_bool pcb_polyarea_contour_include(pcb_polyarea_t * p, pcb_pline_t * c);
pcb_bool pcb_polyarea_contour_exclude(pcb_polyarea_t * p, pcb_pline_t * c);


pcb_bool pcb_polyarea_contour_check(pcb_pline_t * a);

pcb_bool pcb_polyarea_contour_inside(pcb_polyarea_t * c, pcb_vector_t v0);
pcb_bool pcb_polyarea_touching(pcb_polyarea_t * p1, pcb_polyarea_t * p2);

/*** tools for clipping ***/

/* checks whether point lies within contour
independently of its orientation */

int pcb_poly_contour_inside(pcb_pline_t * c, pcb_vector_t v);
int pcb_poly_contour_in_contour(pcb_pline_t * poly, pcb_pline_t * inner);
pcb_polyarea_t *pcb_polyarea_create(void);

void pcb_polyarea_free(pcb_polyarea_t ** p);
void pcb_polyarea_init(pcb_polyarea_t * p);
void pcb_poly_contours_free(pcb_pline_t ** pl);
pcb_bool pcb_poly_valid(pcb_polyarea_t * p);

enum pcb_poly_bool_op_e {
	PCB_PBO_UNITE,
	PCB_PBO_ISECT,
	PCB_PBO_SUB,
	PCB_PBO_XOR
};

double pcb_vect_dist2(pcb_vector_t v1, pcb_vector_t v2);
double pcb_vect_det2(pcb_vector_t v1, pcb_vector_t v2);
double pcb_vect_len2(pcb_vector_t v1);

int pcb_vect_inters2(pcb_vector_t A, pcb_vector_t B, pcb_vector_t C, pcb_vector_t D, pcb_vector_t S1, pcb_vector_t S2);

int pcb_polyarea_boolean(const pcb_polyarea_t * a, const pcb_polyarea_t * b, pcb_polyarea_t ** res, int action);
int pcb_polyarea_boolean_free(pcb_polyarea_t * a, pcb_polyarea_t * b, pcb_polyarea_t ** res, int action);
int pcb_polyarea_and_subtract_free(pcb_polyarea_t * a, pcb_polyarea_t * b, pcb_polyarea_t ** aandb, pcb_polyarea_t ** aminusb);
int pcb_polyarea_save(pcb_polyarea_t * PA, char *fname);

/* calculate the bounding box of a pcb_polyarea_t and save result in b */
void pcb_polyarea_bbox(pcb_polyarea_t * p, pcb_box_t * b);

/* Move each point of pa1 by dx and dy */
void pcb_polyarea_move(pcb_polyarea_t *pa1, pcb_coord_t dx, pcb_coord_t dy);

/*** Tools for building polygons for common object shapes ***/


#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
double pcb_round(double x); /* from math_helper.h */

/* Calculate an endpoint of an arc and return the result in *x;*y */
PCB_INLINE void pcb_arc_get_endpt(pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t astart, pcb_angle_t adelta, int which, pcb_coord_t *x, pcb_coord_t *y)
{
	if (which == 0) {
		*x = pcb_round((double)cx - (double)width * cos(astart * (M_PI/180.0)));
		*y = pcb_round((double)cy + (double)height * sin(astart * (M_PI/180.0)));
	}
	else {
		*x = pcb_round((double)cx - (double)width * cos((astart + adelta) * (M_PI/180.0)));
		*y = pcb_round((double)cy + (double)height * sin((astart + adelta) * (M_PI/180.0)));
	}
}


#endif /* PCB_POLYAREA_H */
