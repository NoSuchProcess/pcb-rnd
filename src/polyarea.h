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
      Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

      polyarea.h
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1997 Klamer Schutte (minor patches)
*/

#ifndef	PCB_POLYAREA_H
#define	PCB_POLYAREA_H

#define PLF_DIR 1
#define PLF_INV 0
#define PLF_MARK 1

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif


typedef Coord pcb_vertex_t[2];				/* longing point representation of
																   coordinates */
typedef pcb_vertex_t pcb_vector_t;

#define VertexEqu(a,b) (memcmp((a),(b),sizeof(pcb_vector_t))==0)
#define VertexCpy(a,b) memcpy((a),(b),sizeof(pcb_vector_t))


extern pcb_vector_t vect_zero;

enum {
	err_no_memory = 2,
	err_bad_parm = 3,
	err_ok = 0
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
	} Flags;
	pcb_cvc_list_t *cvc_prev;
	pcb_cvc_list_t *cvc_next;
	pcb_vector_t point;
};

typedef struct pcb_pline_s pcb_pline_t;
struct pcb_pline_s {
	Coord xmin, ymin, xmax, ymax;
	pcb_pline_t *next;
	pcb_vnode_t head;
	unsigned int Count;
	double area;
	pcb_rtree_t *tree;
	pcb_bool is_round;
	Coord cx, cy;
	Coord radius;
	struct {
		unsigned int status:3;
		unsigned int orient:1;
	} Flags;
};

pcb_pline_t *poly_NewContour(pcb_vector_t v);

void poly_IniContour(pcb_pline_t * c);
void poly_ClrContour(pcb_pline_t * c);	/* clears list of vertices */
void poly_DelContour(pcb_pline_t ** c);

pcb_bool poly_CopyContour(pcb_pline_t ** dst, pcb_pline_t * src);

void poly_PreContour(pcb_pline_t * c, pcb_bool optimize);	/* prepare contour */
void poly_InvContour(pcb_pline_t * c);	/* invert contour */

pcb_vnode_t *poly_CreateNode(pcb_vector_t v);

void poly_InclVertex(pcb_vnode_t * after, pcb_vnode_t * node);
void poly_ExclVertex(pcb_vnode_t * node);

/**********************************************************************/

typedef struct pcb_polyarea_s pcb_polyarea_t;
struct pcb_polyarea_s {
	pcb_polyarea_t *f, *b;
	pcb_pline_t *contours;
	pcb_rtree_t *contour_tree;
};

pcb_bool poly_M_Copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * srcfst);
void poly_M_Incl(pcb_polyarea_t ** list, pcb_polyarea_t * a);

pcb_bool poly_Copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * src);
pcb_bool poly_Copy1(pcb_polyarea_t * dst, const pcb_polyarea_t * src);

pcb_bool poly_InclContour(pcb_polyarea_t * p, pcb_pline_t * c);
pcb_bool poly_ExclContour(pcb_polyarea_t * p, pcb_pline_t * c);


pcb_bool poly_ChkContour(pcb_pline_t * a);

pcb_bool poly_CheckInside(pcb_polyarea_t * c, pcb_vector_t v0);
pcb_bool Touching(pcb_polyarea_t * p1, pcb_polyarea_t * p2);

/**********************************************************************/

/* tools for clipping */

/* checks whether point lies within contour
independently of its orientation */

int poly_InsideContour(pcb_pline_t * c, pcb_vector_t v);
int poly_ContourInContour(pcb_pline_t * poly, pcb_pline_t * inner);
pcb_polyarea_t *poly_Create(void);

void poly_Free(pcb_polyarea_t ** p);
void poly_Init(pcb_polyarea_t * p);
void poly_FreeContours(pcb_pline_t ** pl);
pcb_bool poly_Valid(pcb_polyarea_t * p);

enum pcb_poly_bool_op_e {
	PBO_UNITE,
	PBO_ISECT,
	PBO_SUB,
	PBO_XOR
};

double vect_dist2(pcb_vector_t v1, pcb_vector_t v2);
double vect_det2(pcb_vector_t v1, pcb_vector_t v2);
double vect_len2(pcb_vector_t v1);

int vect_inters2(pcb_vector_t A, pcb_vector_t B, pcb_vector_t C, pcb_vector_t D, pcb_vector_t S1, pcb_vector_t S2);

int poly_Boolean(const pcb_polyarea_t * a, const pcb_polyarea_t * b, pcb_polyarea_t ** res, int action);
int poly_Boolean_free(pcb_polyarea_t * a, pcb_polyarea_t * b, pcb_polyarea_t ** res, int action);
int poly_AndSubtract_free(pcb_polyarea_t * a, pcb_polyarea_t * b, pcb_polyarea_t ** aandb, pcb_polyarea_t ** aminusb);
int Savepcb_polyarea_t(pcb_polyarea_t * PA, char *fname);

/* calculate the bounding box of a pcb_polyarea_t and save result in b */
void poly_bbox(pcb_polyarea_t * p, pcb_box_t * b);

#endif /* PCB_POLYAREA_H */
