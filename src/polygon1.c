/*
       polygon clipping functions. harry eaton implemented the algorithm
       described in "A Closed Set of Algorithms for Performing Set
       Operations on Polygonal Regions in the Plane" which the original
       code did not do. I also modified it for integer coordinates
       and faster computation. The license for this modified copy was
       switched to the GPL per term (3) of the original LGPL license.
       Copyright (C) 2006 harry eaton
 
   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
*/

#include	<assert.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<setjmp.h>
#include	<string.h>

#include "config.h"
#include "rtree.h"
#include "math_helper.h"
#include "heap.h"
#include "compat_cc.h"
#include "pcb-printf.h"
#include "polyarea.h"
#include "obj_common.h"
#include "macro.h"


#define ROUND(a) (long)((a) > 0 ? ((a) + 0.5) : ((a) - 0.5))

#define EPSILON (1E-8)
#define IsZero(a, b) (fabs((a) - (b)) < EPSILON)

/*********************************************************************/
/*              L o n g   V e c t o r   S t u f f                    */
/*********************************************************************/

#define Vcopy(a,b) {(a)[0]=(b)[0];(a)[1]=(b)[1];}
int vect_equal(pcb_vector_t v1, pcb_vector_t v2);
void vect_init(pcb_vector_t v, double x, double y);
void vect_sub(pcb_vector_t res, pcb_vector_t v2, pcb_vector_t v3);

void vect_min(pcb_vector_t res, pcb_vector_t v2, pcb_vector_t v3);
void vect_max(pcb_vector_t res, pcb_vector_t v2, pcb_vector_t v3);

double pcb_vect_dist2(pcb_vector_t v1, pcb_vector_t v2);
double pcb_vect_det2(pcb_vector_t v1, pcb_vector_t v2);
double pcb_vect_len2(pcb_vector_t v1);

int pcb_vect_inters2(pcb_vector_t A, pcb_vector_t B, pcb_vector_t C, pcb_vector_t D, pcb_vector_t S1, pcb_vector_t S2);

/* note that a vertex v's Flags.status represents the edge defined by
 * v to v->next (i.e. the edge is forward of v)
 */
#define ISECTED 3
#define UNKNWN  0
#define INSIDE  1
#define OUTSIDE 2
#define SHARED 3
#define SHARED2 4

#define TOUCHES 99

#define NODE_LABEL(n)  ((n)->Flags.status)
#define LABEL_NODE(n,l) ((n)->Flags.status = (l))

#define error(code)  longjmp(*(e), code)

#define MemGet(ptr, type) \
  if (PCB_UNLIKELY(((ptr) = (type *)malloc(sizeof(type))) == NULL))	\
    error(err_no_memory);

#undef DEBUG_LABEL
#undef DEBUG_ALL_LABELS
#undef DEBUG_JUMP
#undef DEBUG_GATHER
#undef DEBUG_ANGLE
#undef DEBUG
#ifdef DEBUG
#include <stdarg.h>
static void DEBUGP(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	pcb_vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#else
static void DEBUGP(const char *fmt, ...) { }
#endif

/* ///////////////////////////////////////////////////////////////////////////// * /
/ *  2-Dimensional stuff
/ * ///////////////////////////////////////////////////////////////////////////// */

#define Vsub2(r,a,b)	{(r)[0] = (a)[0] - (b)[0]; (r)[1] = (a)[1] - (b)[1];}
#define Vadd2(r,a,b)	{(r)[0] = (a)[0] + (b)[0]; (r)[1] = (a)[1] + (b)[1];}
#define Vsca2(r,a,s)	{(r)[0] = (a)[0] * (s); (r)[1] = (a)[1] * (s);}
#define Vcpy2(r,a)		{(r)[0] = (a)[0]; (r)[1] = (a)[1];}
#define Vequ2(a,b)		((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define Vadds(r,a,b,s)	{(r)[0] = ((a)[0] + (b)[0]) * (s); (r)[1] = ((a)[1] + (b)[1]) * (s);}
#define Vswp2(a,b) { long t; \
	t = (a)[0], (a)[0] = (b)[0], (b)[0] = t; \
	t = (a)[1], (a)[1] = (b)[1], (b)[1] = t; \
}

#ifdef DEBUG
static char *theState(pcb_vnode_t * v);

static void pline_dump(pcb_vnode_t * v)
{
	pcb_vnode_t *s, *n;

	s = v;
	do {
		n = v->next;
		pcb_fprintf(stderr, "Line [%#mS %#mS %#mS %#mS 10 10 \"%s\"]\n",
								v->point[0], v->point[1], n->point[0], n->point[1], theState(v));
	}
	while ((v = v->next) != s);
}

static void poly_dump(pcb_polyarea_t * p)
{
	pcb_polyarea_t *f = p;
	pcb_pline_t *pl;

	do {
		pl = p->contours;
		do {
			pline_dump(&pl->head);
			fprintf(stderr, "NEXT pcb_pline_t\n");
		}
		while ((pl = pl->next) != NULL);
		fprintf(stderr, "NEXT POLY\n");
	}
	while ((p = p->f) != f);
}
#endif

/***************************************************************/
/* routines for processing intersections */

/*
node_add
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
 (C) 2006 harry eaton

 returns a bit field in new_point that indicates where the
 point was.
 1 means a new node was created and inserted
 4 means the intersection was not on the dest point
*/
static pcb_vnode_t *node_add_single(pcb_vnode_t * dest, pcb_vector_t po)
{
	pcb_vnode_t *p;

	if (vect_equal(po, dest->point))
		return dest;
	if (vect_equal(po, dest->next->point))
		return dest->next;
	p = pcb_poly_node_create(po);
	if (p == NULL)
		return NULL;
	p->cvc_prev = p->cvc_next = NULL;
	p->Flags.status = UNKNWN;
	return p;
}																/* node_add */

#define ISECT_BAD_PARAM (-1)
#define ISECT_NO_MEMORY (-2)


/*
new_descriptor
  (C) 2006 harry eaton
*/
static pcb_cvc_list_t *new_descriptor(pcb_vnode_t * a, char poly, char side)
{
	pcb_cvc_list_t *l = (pcb_cvc_list_t *) malloc(sizeof(pcb_cvc_list_t));
	pcb_vector_t v;
	register double ang, dx, dy;

	if (!l)
		return NULL;
	l->head = NULL;
	l->parent = a;
	l->poly = poly;
	l->side = side;
	l->next = l->prev = l;
	if (side == 'P')							/* previous */
		vect_sub(v, a->prev->point, a->point);
	else													/* next */
		vect_sub(v, a->next->point, a->point);
	/* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle.
	 * It still has the same monotonic sort result
	 * and is far less expensive to compute than the real angle.
	 */
	if (vect_equal(v, vect_zero)) {
		if (side == 'P') {
			if (a->prev->cvc_prev == (pcb_cvc_list_t *) - 1)
				a->prev->cvc_prev = a->prev->cvc_next = NULL;
			pcb_poly_vertex_exclude(a->prev);
			vect_sub(v, a->prev->point, a->point);
		}
		else {
			if (a->next->cvc_prev == (pcb_cvc_list_t *) - 1)
				a->next->cvc_prev = a->next->cvc_next = NULL;
			pcb_poly_vertex_exclude(a->next);
			vect_sub(v, a->next->point, a->point);
		}
	}
	assert(!vect_equal(v, vect_zero));
	dx = fabs((double) v[0]);
	dy = fabs((double) v[1]);
	ang = dy / (dy + dx);
	/* now move to the actual quadrant */
	if (v[0] < 0 && v[1] >= 0)
		ang = 2.0 - ang;						/* 2nd quadrant */
	else if (v[0] < 0 && v[1] < 0)
		ang += 2.0;									/* 3rd quadrant */
	else if (v[0] >= 0 && v[1] < 0)
		ang = 4.0 - ang;						/* 4th quadrant */
	l->angle = ang;
	assert(ang >= 0.0 && ang <= 4.0);
#ifdef DEBUG_ANGLE
	DEBUGP("node on %c at %#mD assigned angle %g on side %c\n", poly, a->point[0], a->point[1], ang, side);
#endif
	return l;
}

/*
insert_descriptor
  (C) 2006 harry eaton

   argument a is a cross-vertex node.
   argument poly is the polygon it comes from ('A' or 'B')
   argument side is the side this descriptor goes on ('P' for previous
   'N' for next.
   argument start is the head of the list of cvclists
*/
static pcb_cvc_list_t *insert_descriptor(pcb_vnode_t * a, char poly, char side, pcb_cvc_list_t * start)
{
	pcb_cvc_list_t *l, *newone, *big, *small;

	if (!(newone = new_descriptor(a, poly, side)))
		return NULL;
	/* search for the pcb_cvc_list_t for this point */
	if (!start) {
		start = newone;							/* return is also new, so we know where start is */
		start->head = newone;				/* circular list */
		return newone;
	}
	else {
		l = start;
		do {
			assert(l->head);
			if (l->parent->point[0] == a->point[0]
					&& l->parent->point[1] == a->point[1]) {	/* this pcb_cvc_list_t is at our point */
				start = l;
				newone->head = l->head;
				break;
			}
			if (l->head->parent->point[0] == start->parent->point[0]
					&& l->head->parent->point[1] == start->parent->point[1]) {
				/* this seems to be a new point */
				/* link this cvclist to the list of all cvclists */
				for (; l->head != newone; l = l->next)
					l->head = newone;
				newone->head = start;
				return newone;
			}
			l = l->head;
		}
		while (1);
	}
	assert(start);
	l = big = small = start;
	do {
		if (l->next->angle < l->angle) {	/* find start/end of list */
			small = l->next;
			big = l;
		}
		else if (newone->angle >= l->angle && newone->angle <= l->next->angle) {
			/* insert new cvc if it lies between existing points */
			newone->prev = l;
			newone->next = l->next;
			l->next = l->next->prev = newone;
			return newone;
		}
	}
	while ((l = l->next) != start);
	/* didn't find it between points, it must go on an end */
	if (big->angle <= newone->angle) {
		newone->prev = big;
		newone->next = big->next;
		big->next = big->next->prev = newone;
		return newone;
	}
	assert(small->angle >= newone->angle);
	newone->next = small;
	newone->prev = small->prev;
	small->prev = small->prev->next = newone;
	return newone;
}

/*
node_add_point
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov

 return 1 if new node in b, 2 if new node in a and 3 if new node in both
*/

static pcb_vnode_t *node_add_single_point(pcb_vnode_t * a, pcb_vector_t p)
{
	pcb_vnode_t *next_a, *new_node;

	next_a = a->next;

	new_node = node_add_single(a, p);
	assert(new_node != NULL);

	new_node->cvc_prev = new_node->cvc_next = (pcb_cvc_list_t *) - 1;

	if (new_node == a || new_node == next_a)
		return NULL;

	return new_node;
}																/* node_add_point */

/*
node_label
 (C) 2006 harry eaton
*/
static unsigned int node_label(pcb_vnode_t * pn)
{
	pcb_cvc_list_t *first_l, *l;
	char this_poly;
	int region = UNKNWN;

	assert(pn);
	assert(pn->cvc_next);
	this_poly = pn->cvc_next->poly;
	/* search counter-clockwise in the cross vertex connectivity (CVC) list
	 *
	 * check for shared edges (that could be prev or next in the list since the angles are equal)
	 * and check if this edge (pn -> pn->next) is found between the other poly's entry and exit
	 */
	if (pn->cvc_next->angle == pn->cvc_next->prev->angle)
		l = pn->cvc_next->prev;
	else
		l = pn->cvc_next->next;

	first_l = l;
	while ((l->poly == this_poly) && (l != first_l->prev))
		l = l->next;
	assert(l->poly != this_poly);

	assert(l && l->angle >= 0 && l->angle <= 4.0);
	if (l->poly != this_poly) {
		if (l->side == 'P') {
			if (l->parent->prev->point[0] == pn->next->point[0] && l->parent->prev->point[1] == pn->next->point[1]) {
				region = SHARED2;
				pn->shared = l->parent->prev;
			}
			else
				region = INSIDE;
		}
		else {
			if (l->angle == pn->cvc_next->angle) {
				assert(l->parent->next->point[0] == pn->next->point[0] && l->parent->next->point[1] == pn->next->point[1]);
				region = SHARED;
				pn->shared = l->parent;
			}
			else
				region = OUTSIDE;
		}
	}
	if (region == UNKNWN) {
		for (l = l->next; l != pn->cvc_next; l = l->next) {
			if (l->poly != this_poly) {
				if (l->side == 'P')
					region = INSIDE;
				else
					region = OUTSIDE;
				break;
			}
		}
	}
	assert(region != UNKNWN);
	assert(NODE_LABEL(pn) == UNKNWN || NODE_LABEL(pn) == region);
	LABEL_NODE(pn, region);
	if (region == SHARED || region == SHARED2)
		return UNKNWN;
	return region;
}																/* node_label */

/*
 add_descriptors
 (C) 2006 harry eaton
*/
static pcb_cvc_list_t *add_descriptors(pcb_pline_t * pl, char poly, pcb_cvc_list_t * list)
{
	pcb_vnode_t *node = &pl->head;

	do {
		if (node->cvc_prev) {
			assert(node->cvc_prev == (pcb_cvc_list_t *) - 1 && node->cvc_next == (pcb_cvc_list_t *) - 1);
			list = node->cvc_prev = insert_descriptor(node, poly, 'P', list);
			if (!node->cvc_prev)
				return NULL;
			list = node->cvc_next = insert_descriptor(node, poly, 'N', list);
			if (!node->cvc_next)
				return NULL;
		}
	}
	while ((node = node->next) != &pl->head);
	return list;
}

static inline void cntrbox_adjust(pcb_pline_t * c, pcb_vector_t p)
{
	c->xmin = min(c->xmin, p[0]);
	c->xmax = max(c->xmax, p[0] + 1);
	c->ymin = min(c->ymin, p[1]);
	c->ymax = max(c->ymax, p[1] + 1);
}

/* some structures for handling segment intersections using the rtrees */

typedef struct seg {
	pcb_box_t box;
	pcb_vnode_t *v;
	pcb_pline_t *p;
	int intersected;
} seg;

typedef struct _insert_node_task insert_node_task;

struct _insert_node_task {
	insert_node_task *next;
	seg *node_seg;
	pcb_vnode_t *new_node;
};

typedef struct info {
	double m, b;
	pcb_rtree_t *tree;
	pcb_vnode_t *v;
	struct seg *s;
	jmp_buf *env, sego, *touch;
	int need_restart;
	insert_node_task *node_insert_list;
} info;

typedef struct contour_info {
	pcb_pline_t *pa;
	jmp_buf restart;
	jmp_buf *getout;
	int need_restart;
	insert_node_task *node_insert_list;
} contour_info;


/*
 * adjust_tree()
 * (C) 2006 harry eaton
 * This replaces the segment in the tree with the two new segments after
 * a vertex has been added
 */
static int adjust_tree(pcb_rtree_t * tree, struct seg *s)
{
	struct seg *q;

	q = (seg *) malloc(sizeof(struct seg));
	if (!q)
		return 1;
	q->intersected = 0;
	q->v = s->v;
	q->p = s->p;
	q->box.X1 = min(q->v->point[0], q->v->next->point[0]);
	q->box.X2 = max(q->v->point[0], q->v->next->point[0]) + 1;
	q->box.Y1 = min(q->v->point[1], q->v->next->point[1]);
	q->box.Y2 = max(q->v->point[1], q->v->next->point[1]) + 1;
	pcb_r_insert_entry(tree, (const pcb_box_t *) q, 1);
	q = (seg *) malloc(sizeof(struct seg));
	if (!q)
		return 1;
	q->intersected = 0;
	q->v = s->v->next;
	q->p = s->p;
	q->box.X1 = min(q->v->point[0], q->v->next->point[0]);
	q->box.X2 = max(q->v->point[0], q->v->next->point[0]) + 1;
	q->box.Y1 = min(q->v->point[1], q->v->next->point[1]);
	q->box.Y2 = max(q->v->point[1], q->v->next->point[1]) + 1;
	pcb_r_insert_entry(tree, (const pcb_box_t *) q, 1);
	pcb_r_delete_entry(tree, (const pcb_box_t *) s);
	return 0;
}

/*
 * seg_in_region()
 * (C) 2006, harry eaton
 * This prunes the search for boxes that don't intersect the segment.
 */
static pcb_r_dir_t seg_in_region(const pcb_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	double y1, y2;
	/* for zero slope the search is aligned on the axis so it is already pruned */
	if (i->m == 0.)
		return R_DIR_FOUND_CONTINUE;
	y1 = i->m * b->X1 + i->b;
	y2 = i->m * b->X2 + i->b;
	if (min(y1, y2) >= b->Y2)
		return R_DIR_NOT_FOUND;
	if (max(y1, y2) < b->Y1)
		return R_DIR_NOT_FOUND;
	return R_DIR_FOUND_CONTINUE;											/* might intersect */
}

/* Prepend a deferred node-insertion task to a list */
static insert_node_task *prepend_insert_node_task(insert_node_task * list, seg * seg, pcb_vnode_t * new_node)
{
	insert_node_task *task = (insert_node_task *) malloc(sizeof(*task));
	task->node_seg = seg;
	task->new_node = new_node;
	task->next = list;
	return task;
}

/*
 * seg_in_seg()
 * (C) 2006 harry eaton
 * This routine checks if the segment in the tree intersect the search segment.
 * If it does, the plines are marked as intersected and the point is marked for
 * the cvclist. If the point is not already a vertex, a new vertex is inserted
 * and the search for intersections starts over at the beginning.
 * That is potentially a significant time penalty, but it does solve the snap rounding
 * problem. There are efficient algorithms for finding intersections with snap
 * rounding, but I don't have time to implement them right now.
 */
static pcb_r_dir_t seg_in_seg(const pcb_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	struct seg *s = (struct seg *) b;
	pcb_vector_t s1, s2;
	int cnt;
	pcb_vnode_t *new_node;

	/* When new nodes are added at the end of a pass due to an intersection
	 * the segments may be altered. If either segment we're looking at has
	 * already been intersected this pass, skip it until the next pass.
	 */
	if (s->intersected || i->s->intersected)
		return R_DIR_NOT_FOUND;

	cnt = pcb_vect_inters2(s->v->point, s->v->next->point, i->v->point, i->v->next->point, s1, s2);
	if (!cnt)
		return R_DIR_NOT_FOUND;
	if (i->touch)									/* if checking touches one find and we're done */
		longjmp(*i->touch, TOUCHES);
	i->s->p->Flags.status = ISECTED;
	s->p->Flags.status = ISECTED;
	for (; cnt; cnt--) {
		pcb_bool done_insert_on_i = pcb_false;
		new_node = node_add_single_point(i->v, cnt > 1 ? s2 : s1);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"i\" at %#mD\n", cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
			i->node_insert_list = prepend_insert_node_task(i->node_insert_list, i->s, new_node);
			i->s->intersected = 1;
			done_insert_on_i = pcb_true;
		}
		new_node = node_add_single_point(s->v, cnt > 1 ? s2 : s1);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"s\" at %#mD\n", cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
			i->node_insert_list = prepend_insert_node_task(i->node_insert_list, s, new_node);
			s->intersected = 1;
			return R_DIR_NOT_FOUND;									/* Keep looking for intersections with segment "i" */
		}
		/* Skip any remaining r_search hits against segment i, as any further
		 * intersections will be rejected until the next pass anyway.
		 */
		if (done_insert_on_i)
			longjmp(*i->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

static void *make_edge_tree(pcb_pline_t * pb)
{
	struct seg *s;
	pcb_vnode_t *bv;
	pcb_rtree_t *ans = pcb_r_create_tree(NULL, 0, 0);
	bv = &pb->head;
	do {
		s = (seg *) malloc(sizeof(struct seg));
		s->intersected = 0;
		if (bv->point[0] < bv->next->point[0]) {
			s->box.X1 = bv->point[0];
			s->box.X2 = bv->next->point[0] + 1;
		}
		else {
			s->box.X2 = bv->point[0] + 1;
			s->box.X1 = bv->next->point[0];
		}
		if (bv->point[1] < bv->next->point[1]) {
			s->box.Y1 = bv->point[1];
			s->box.Y2 = bv->next->point[1] + 1;
		}
		else {
			s->box.Y2 = bv->point[1] + 1;
			s->box.Y1 = bv->next->point[1];
		}
		s->v = bv;
		s->p = pb;
		pcb_r_insert_entry(ans, (const pcb_box_t *) s, 1);
	}
	while ((bv = bv->next) != &pb->head);
	return (void *) ans;
}

static pcb_r_dir_t get_seg(const pcb_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	struct seg *s = (struct seg *) b;
	if (i->v == s->v) {
		i->s = s;
		return R_DIR_CANCEL; /* found */
	}
	return R_DIR_NOT_FOUND;
}

/*
 * intersect() (and helpers)
 * (C) 2006, harry eaton
 * This uses an rtree to find A-B intersections. Whenever a new vertex is
 * added, the search for intersections is re-started because the rounding
 * could alter the topology otherwise. 
 * This should use a faster algorithm for snap rounding intersection finding.
 * The best algorithm is probably found in:
 *
 * "Improved output-sensitive snap rounding," John Hershberger, Proceedings
 * of the 22nd annual symposium on Computational geometry, 2006, pp 357-366.
 * http://doi.acm.org/10.1145/1137856.1137909
 *
 * Algorithms described by de Berg, or Goodrich or Halperin, or Hobby would
 * probably work as well.
 *
 */

static pcb_r_dir_t contour_bounds_touch(const pcb_box_t * b, void *cl)
{
	contour_info *c_info = (contour_info *) cl;
	pcb_pline_t *pa = c_info->pa;
	pcb_pline_t *pb = (pcb_pline_t *) b;
	pcb_pline_t *rtree_over;
	pcb_pline_t *looping_over;
	pcb_vnode_t *av;										/* node iterators */
	struct info info;
	pcb_box_t box;
	jmp_buf restart;

	/* Have seg_in_seg return to our desired location if it touches */
	info.env = &restart;
	info.touch = c_info->getout;
	info.need_restart = 0;
	info.node_insert_list = c_info->node_insert_list;

	/* Pick which contour has the fewer points, and do the loop
	 * over that. The r_tree makes hit-testing against a contour
	 * faster, so we want to do that on the bigger contour.
	 */
	if (pa->Count < pb->Count) {
		rtree_over = pb;
		looping_over = pa;
	}
	else {
		rtree_over = pa;
		looping_over = pb;
	}

	av = &looping_over->head;
	do {													/* Loop over the nodes in the smaller contour */
		pcb_r_dir_t rres;
		/* check this edge for any insertions */
		double dx;
		info.v = av;
		/* compute the slant for region trimming */
		dx = av->next->point[0] - av->point[0];
		if (dx == 0)
			info.m = 0;
		else {
			info.m = (av->next->point[1] - av->point[1]) / dx;
			info.b = av->point[1] - info.m * av->point[0];
		}
		box.X2 = (box.X1 = av->point[0]) + 1;
		box.Y2 = (box.Y1 = av->point[1]) + 1;

		/* fill in the segment in info corresponding to this node */
		rres = pcb_r_search(looping_over->tree, &box, NULL, get_seg, &info, NULL);
		assert(rres == R_DIR_CANCEL);

		/* If we're going to have another pass anyway, skip this */
		if (info.s->intersected && info.node_insert_list != NULL)
			continue;

		if (setjmp(restart))
			continue;

		/* NB: If this actually hits anything, we are teleported back to the beginning */
		info.tree = rtree_over->tree;
		if (info.tree) {
			int seen;
			pcb_r_search(info.tree, &info.s->box, seg_in_region, seg_in_seg, &info, &seen);
			if (PCB_UNLIKELY(seen))
				assert(0);							/* XXX: Memory allocation failure */
		}
	}
	while ((av = av->next) != &looping_over->head);

	c_info->node_insert_list = info.node_insert_list;
	if (info.need_restart)
		c_info->need_restart = 1;
	return R_DIR_NOT_FOUND;
}

static int intersect_impl(jmp_buf * jb, pcb_polyarea_t * b, pcb_polyarea_t * a, int add)
{
	pcb_polyarea_t *t;
	pcb_pline_t *pa;
	contour_info c_info;
	int need_restart = 0;
	insert_node_task *task;
	c_info.need_restart = 0;
	c_info.node_insert_list = NULL;

	/* Search the r-tree of the object with most contours
	 * We loop over the contours of "a". Swap if necessary.
	 */
	if (a->contour_tree->size > b->contour_tree->size) {
		t = b;
		b = a;
		a = t;
	}

	for (pa = a->contours; pa; pa = pa->next) {	/* Loop over the contours of pcb_polyarea_t "a" */
		pcb_box_t sb;
		jmp_buf out;
		int retval;

		c_info.getout = NULL;
		c_info.pa = pa;

		if (!add) {
			retval = setjmp(out);
			if (retval) {
				/* The intersection test short-circuited back here,
				 * we need to clean up, then longjmp to jb */
				longjmp(*jb, retval);
			}
			c_info.getout = &out;
		}

		sb.X1 = pa->xmin;
		sb.Y1 = pa->ymin;
		sb.X2 = pa->xmax + 1;
		sb.Y2 = pa->ymax + 1;

		pcb_r_search(b->contour_tree, &sb, NULL, contour_bounds_touch, &c_info, NULL);
		if (c_info.need_restart)
			need_restart = 1;
	}

	/* Process any deferred node insertions */
	task = c_info.node_insert_list;
	while (task != NULL) {
		insert_node_task *next = task->next;

		/* Do insertion */
		task->new_node->prev = task->node_seg->v;
		task->new_node->next = task->node_seg->v->next;
		task->node_seg->v->next->prev = task->new_node;
		task->node_seg->v->next = task->new_node;
		task->node_seg->p->Count++;

		cntrbox_adjust(task->node_seg->p, task->new_node->point);
		if (adjust_tree(task->node_seg->p->tree, task->node_seg))
			assert(0);								/* XXX: Memory allocation failure */

		need_restart = 1;						/* Any new nodes could intersect */

		free(task);
		task = next;
	}

	return need_restart;
}

static int intersect(jmp_buf * jb, pcb_polyarea_t * b, pcb_polyarea_t * a, int add)
{
	int call_count = 1;
	while (intersect_impl(jb, b, a, add))
		call_count++;
	return 0;
}

static void M_pcb_polyarea_t_intersect(jmp_buf * e, pcb_polyarea_t * afst, pcb_polyarea_t * bfst, int add)
{
	pcb_polyarea_t *a = afst, *b = bfst;
	pcb_pline_t *curcA, *curcB;
	pcb_cvc_list_t *the_list = NULL;

	if (a == NULL || b == NULL)
		error(err_bad_parm);
	do {
		do {
			if (a->contours->xmax >= b->contours->xmin &&
					a->contours->ymax >= b->contours->ymin &&
					a->contours->xmin <= b->contours->xmax && a->contours->ymin <= b->contours->ymax) {
				if (PCB_UNLIKELY(intersect(e, a, b, add)))
					error(err_no_memory);
			}
		}
		while (add && (a = a->f) != afst);
		for (curcB = b->contours; curcB != NULL; curcB = curcB->next)
			if (curcB->Flags.status == ISECTED) {
				the_list = add_descriptors(curcB, 'B', the_list);
				if (PCB_UNLIKELY(the_list == NULL))
					error(err_no_memory);
			}
	}
	while (add && (b = b->f) != bfst);
	do {
		for (curcA = a->contours; curcA != NULL; curcA = curcA->next)
			if (curcA->Flags.status == ISECTED) {
				the_list = add_descriptors(curcA, 'A', the_list);
				if (PCB_UNLIKELY(the_list == NULL))
					error(err_no_memory);
			}
	}
	while (add && (a = a->f) != afst);
}																/* M_pcb_polyarea_t_intersect */

static inline int cntrbox_inside(pcb_pline_t * c1, pcb_pline_t * c2)
{
	assert(c1 != NULL && c2 != NULL);
	return ((c1->xmin >= c2->xmin) && (c1->ymin >= c2->ymin) && (c1->xmax <= c2->xmax) && (c1->ymax <= c2->ymax));
}

/*****************************************************************/
/* Routines for making labels */

static pcb_r_dir_t count_contours_i_am_inside(const pcb_box_t * b, void *cl)
{
	pcb_pline_t *me = (pcb_pline_t *) cl;
	pcb_pline_t *check = (pcb_pline_t *) b;

	if (pcb_poly_contour_in_contour(check, me))
		return R_DIR_FOUND_CONTINUE;
	return R_DIR_NOT_FOUND;
}

/* cntr_in_M_pcb_polyarea_t
returns poly is inside outfst ? pcb_true : pcb_false */
static int cntr_in_M_pcb_polyarea_t(pcb_pline_t * poly, pcb_polyarea_t * outfst, pcb_bool test)
{
	pcb_polyarea_t *outer = outfst;
	pcb_heap_t *heap;

	assert(poly != NULL);
	assert(outer != NULL);

	heap = pcb_heap_create();
	do {
		if (cntrbox_inside(poly, outer->contours))
			pcb_heap_insert(heap, outer->contours->area, (void *) outer);
	}
	/* if checking touching, use only the first polygon */
	while (!test && (outer = outer->f) != outfst);
	/* we need only check the smallest poly container
	 * but we must loop in case the box container is not
	 * the poly container */
	do {
		int cnt;

		if (pcb_heap_is_empty(heap))
			break;
		outer = (pcb_polyarea_t *) pcb_heap_remove_smallest(heap);

		pcb_r_search(outer->contour_tree, (pcb_box_t *) poly, NULL, count_contours_i_am_inside, poly, &cnt);
		switch (cnt) {
		case 0:										/* Didn't find anything in this piece, Keep looking */
			break;
		case 1:										/* Found we are inside this piece, and not any of its holes */
			pcb_heap_destroy(&heap);
			return pcb_true;
		case 2:										/* Found inside a hole in the smallest polygon so far. No need to check the other polygons */
			pcb_heap_destroy(&heap);
			return pcb_false;
		default:
			printf("Something strange here\n");
			break;
		}
	}
	while (1);
	pcb_heap_destroy(&heap);
	return pcb_false;
}																/* cntr_in_M_pcb_polyarea_t */

#ifdef DEBUG

static char *theState(pcb_vnode_t * v)
{
	static char u[] = "UNKNOWN";
	static char i[] = "INSIDE";
	static char o[] = "OUTSIDE";
	static char s[] = "SHARED";
	static char s2[] = "SHARED2";

	switch (NODE_LABEL(v)) {
	case INSIDE:
		return i;
	case OUTSIDE:
		return o;
	case SHARED:
		return s;
	case SHARED2:
		return s2;
	default:
		return u;
	}
}

#ifdef DEBUG_ALL_LABELS
static void print_labels(pcb_pline_t * a)
{
	pcb_vnode_t *c = &a->head;

	do {
		DEBUGP("%#mD->%#mD labeled %s\n", c->point[0], c->point[1], c->next->point[0], c->next->point[1], theState(c));
	}
	while ((c = c->next) != &a->head);
}
#endif
#endif

/*
label_contour
 (C) 2006 harry eaton
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/

static pcb_bool label_contour(pcb_pline_t * a)
{
	pcb_vnode_t *cur = &a->head;
	pcb_vnode_t *first_labelled = NULL;
	int label = UNKNWN;

	do {
		if (cur->cvc_next) {				/* examine cross vertex */
			label = node_label(cur);
			if (first_labelled == NULL)
				first_labelled = cur;
			continue;
		}

		if (first_labelled == NULL)
			continue;

		/* This labels nodes which aren't cross-connected */
		assert(label == INSIDE || label == OUTSIDE);
		LABEL_NODE(cur, label);
	}
	while ((cur = cur->next) != first_labelled);
#ifdef DEBUG_ALL_LABELS
	print_labels(a);
	DEBUGP("\n\n");
#endif
	return pcb_false;
}																/* label_contour */

static pcb_bool cntr_label_pcb_polyarea_t(pcb_pline_t * poly, pcb_polyarea_t * ppl, pcb_bool test)
{
	assert(ppl != NULL && ppl->contours != NULL);
	if (poly->Flags.status == ISECTED) {
		label_contour(poly);				/* should never get here when pcb_bool is pcb_true */
	}
	else if (cntr_in_M_pcb_polyarea_t(poly, ppl, test)) {
		if (test)
			return pcb_true;
		poly->Flags.status = INSIDE;
	}
	else {
		if (test)
			return pcb_false;
		poly->Flags.status = OUTSIDE;
	}
	return pcb_false;
}																/* cntr_label_pcb_polyarea_t */

static pcb_bool M_pcb_polyarea_t_label_separated(pcb_pline_t * afst, pcb_polyarea_t * b, pcb_bool touch)
{
	pcb_pline_t *curc = afst;

	for (curc = afst; curc != NULL; curc = curc->next) {
		if (cntr_label_pcb_polyarea_t(curc, b, touch) && touch)
			return pcb_true;
	}
	return pcb_false;
}

static pcb_bool M_pcb_polyarea_t_label(pcb_polyarea_t * afst, pcb_polyarea_t * b, pcb_bool touch)
{
	pcb_polyarea_t *a = afst;
	pcb_pline_t *curc;

	assert(a != NULL);
	do {
		for (curc = a->contours; curc != NULL; curc = curc->next)
			if (cntr_label_pcb_polyarea_t(curc, b, touch)) {
				if (touch)
					return pcb_true;
			}
	}
	while (!touch && (a = a->f) != afst);
	return pcb_false;
}

/****************************************************************/

/* routines for temporary storing resulting contours */
static void InsCntr(jmp_buf * e, pcb_pline_t * c, pcb_polyarea_t ** dst)
{
	pcb_polyarea_t *newp;

	if (*dst == NULL) {
		MemGet(*dst, pcb_polyarea_t);
		(*dst)->f = (*dst)->b = *dst;
		newp = *dst;
	}
	else {
		MemGet(newp, pcb_polyarea_t);
		newp->f = *dst;
		newp->b = (*dst)->b;
		newp->f->b = newp->b->f = newp;
	}
	newp->contours = c;
	newp->contour_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(newp->contour_tree, (pcb_box_t *) c, 0);
	c->next = NULL;
}																/* InsCntr */

static void
PutContour(jmp_buf * e, pcb_pline_t * cntr, pcb_polyarea_t ** contours, pcb_pline_t ** holes,
					 pcb_polyarea_t * owner, pcb_polyarea_t * parent, pcb_pline_t * parent_contour)
{
	assert(cntr != NULL);
	assert(cntr->Count > 2);
	cntr->next = NULL;

	if (cntr->Flags.orient == PLF_DIR) {
		if (owner != NULL)
			pcb_r_delete_entry(owner->contour_tree, (pcb_box_t *) cntr);
		InsCntr(e, cntr, contours);
	}
	/* put hole into temporary list */
	else {
		/* if we know this belongs inside the parent, put it there now */
		if (parent_contour) {
			cntr->next = parent_contour->next;
			parent_contour->next = cntr;
			if (owner != parent) {
				if (owner != NULL)
					pcb_r_delete_entry(owner->contour_tree, (pcb_box_t *) cntr);
				pcb_r_insert_entry(parent->contour_tree, (pcb_box_t *) cntr, 0);
			}
		}
		else {
			cntr->next = *holes;
			*holes = cntr;						/* let cntr be 1st hole in list */
			/* We don't insert the holes into an r-tree,
			 * they just form a linked list */
			if (owner != NULL)
				pcb_r_delete_entry(owner->contour_tree, (pcb_box_t *) cntr);
		}
	}
}																/* PutContour */

static inline void remove_contour(pcb_polyarea_t * piece, pcb_pline_t * prev_contour, pcb_pline_t * contour, int remove_rtree_entry)
{
	if (piece->contours == contour)
		piece->contours = contour->next;
	else if (prev_contour != NULL) {
		assert(prev_contour->next == contour);
		prev_contour->next = contour->next;
	}

	contour->next = NULL;

	if (remove_rtree_entry)
		pcb_r_delete_entry(piece->contour_tree, (pcb_box_t *) contour);
}

struct polyarea_info {
	pcb_box_t BoundingBox;
	pcb_polyarea_t *pa;
};

static pcb_r_dir_t heap_it(const pcb_box_t * b, void *cl)
{
	pcb_heap_t *heap = (pcb_heap_t *) cl;
	struct polyarea_info *pa_info = (struct polyarea_info *) b;
	pcb_pline_t *p = pa_info->pa->contours;
	if (p->Count == 0)
		return R_DIR_NOT_FOUND;										/* how did this happen? */
	pcb_heap_insert(heap, p->area, pa_info);
	return R_DIR_FOUND_CONTINUE;
}

struct find_inside_info {
	jmp_buf jb;
	pcb_pline_t *want_inside;
	pcb_pline_t *result;
};

static pcb_r_dir_t find_inside(const pcb_box_t * b, void *cl)
{
	struct find_inside_info *info = (struct find_inside_info *) cl;
	pcb_pline_t *check = (pcb_pline_t *) b;
	/* Do test on check to see if it inside info->want_inside */
	/* If it is: */
	if (check->Flags.orient == PLF_DIR) {
		return R_DIR_NOT_FOUND;
	}
	if (pcb_poly_contour_in_contour(info->want_inside, check)) {
		info->result = check;
		longjmp(info->jb, 1);
	}
	return R_DIR_NOT_FOUND;
}

static void InsertHoles(jmp_buf * e, pcb_polyarea_t * dest, pcb_pline_t ** src)
{
	pcb_polyarea_t *curc;
	pcb_pline_t *curh, *container;
	pcb_heap_t *heap;
	pcb_rtree_t *tree;
	int i;
	int num_polyareas = 0;
	struct polyarea_info *all_pa_info, *pa_info;

	if (*src == NULL)
		return;											/* empty hole list */
	if (dest == NULL)
		error(err_bad_parm);				/* empty contour list */

	/* Count dest polyareas */
	curc = dest;
	do {
		num_polyareas++;
	}
	while ((curc = curc->f) != dest);

	/* make a polyarea info table */
	/* make an rtree of polyarea info table */
	all_pa_info = (struct polyarea_info *) malloc(sizeof(struct polyarea_info) * num_polyareas);
	tree = pcb_r_create_tree(NULL, 0, 0);
	i = 0;
	curc = dest;
	do {
		all_pa_info[i].BoundingBox.X1 = curc->contours->xmin;
		all_pa_info[i].BoundingBox.Y1 = curc->contours->ymin;
		all_pa_info[i].BoundingBox.X2 = curc->contours->xmax;
		all_pa_info[i].BoundingBox.Y2 = curc->contours->ymax;
		all_pa_info[i].pa = curc;
		pcb_r_insert_entry(tree, (const pcb_box_t *) &all_pa_info[i], 0);
		i++;
	}
	while ((curc = curc->f) != dest);

	/* loop through the holes and put them where they belong */
	while ((curh = *src) != NULL) {
		*src = curh->next;

		container = NULL;
		/* build a heap of all of the polys that the hole is inside its bounding box */
		heap = pcb_heap_create();
		pcb_r_search(tree, (pcb_box_t *) curh, NULL, heap_it, heap, NULL);
		if (pcb_heap_is_empty(heap)) {
#ifndef NDEBUG
#ifdef DEBUG
			poly_dump(dest);
#endif
#endif
			pcb_poly_contour_del(&curh);
			error(err_bad_parm);
		}
		/* Now search the heap for the container. If there was only one item
		 * in the heap, assume it is the container without the expense of
		 * proving it.
		 */
		pa_info = (struct polyarea_info *) pcb_heap_remove_smallest(heap);
		if (pcb_heap_is_empty(heap)) {	/* only one possibility it must be the right one */
			assert(pcb_poly_contour_in_contour(pa_info->pa->contours, curh));
			container = pa_info->pa->contours;
		}
		else {
			do {
				if (pcb_poly_contour_in_contour(pa_info->pa->contours, curh)) {
					container = pa_info->pa->contours;
					break;
				}
				if (pcb_heap_is_empty(heap))
					break;
				pa_info = (struct polyarea_info *) pcb_heap_remove_smallest(heap);
			}
			while (1);
		}
		pcb_heap_destroy(&heap);
		if (container == NULL) {
			/* bad input polygons were given */
#ifndef NDEBUG
#ifdef DEBUG
			poly_dump(dest);
#endif
#endif
			curh->next = NULL;
			pcb_poly_contour_del(&curh);
			error(err_bad_parm);
		}
		else {
			/* Need to check if this new hole means we need to kick out any old ones for reprocessing */
			while (1) {
				struct find_inside_info info;
				pcb_pline_t *prev;

				info.want_inside = curh;

				/* Set jump return */
				if (setjmp(info.jb)) {
					/* Returned here! */
				}
				else {
					info.result = NULL;
					/* Rtree search, calling back a routine to longjmp back with data about any hole inside the added one */
					/*   Be sure not to bother jumping back to report the main contour! */
					pcb_r_search(pa_info->pa->contour_tree, (pcb_box_t *) curh, NULL, find_inside, &info, NULL);

					/* Nothing found? */
					break;
				}

				/* We need to find the contour before it, so we can update its next pointer */
				prev = container;
				while (prev->next != info.result) {
					prev = prev->next;
				}

				/* Remove hole from the contour */
				remove_contour(pa_info->pa, prev, info.result, pcb_true);

				/* Add hole as the next on the list to be processed in this very function */
				info.result->next = *src;
				*src = info.result;
			}
			/* End check for kicked out holes */

			/* link at front of hole list */
			curh->next = container->next;
			container->next = curh;
			pcb_r_insert_entry(pa_info->pa->contour_tree, (pcb_box_t *) curh, 0);

		}
	}
	pcb_r_destroy_tree(&tree);
	free(all_pa_info);
}																/* InsertHoles */


/****************************************************************/
/* routines for collecting result */

typedef enum {
	FORW, BACKW
} DIRECTION;

/* Start Rule */
typedef int (*S_Rule) (pcb_vnode_t *, DIRECTION *);

/* Jump Rule  */
typedef int (*J_Rule) (char, pcb_vnode_t *, DIRECTION *);

static int UniteS_Rule(pcb_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == OUTSIDE) || (NODE_LABEL(cur) == SHARED);
}

static int IsectS_Rule(pcb_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == INSIDE) || (NODE_LABEL(cur) == SHARED);
}

static int SubS_Rule(pcb_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == OUTSIDE) || (NODE_LABEL(cur) == SHARED2);
}

static int XorS_Rule(pcb_vnode_t * cur, DIRECTION * initdir)
{
	if (cur->Flags.status == INSIDE) {
		*initdir = BACKW;
		return pcb_true;
	}
	if (cur->Flags.status == OUTSIDE) {
		*initdir = FORW;
		return pcb_true;
	}
	return pcb_false;
}

static int IsectJ_Rule(char p, pcb_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->Flags.status == INSIDE || v->Flags.status == SHARED);
}

static int UniteJ_Rule(char p, pcb_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->Flags.status == OUTSIDE || v->Flags.status == SHARED);
}

static int XorJ_Rule(char p, pcb_vnode_t * v, DIRECTION * cdir)
{
	if (v->Flags.status == INSIDE) {
		*cdir = BACKW;
		return pcb_true;
	}
	if (v->Flags.status == OUTSIDE) {
		*cdir = FORW;
		return pcb_true;
	}
	return pcb_false;
}

static int SubJ_Rule(char p, pcb_vnode_t * v, DIRECTION * cdir)
{
	if (p == 'B' && v->Flags.status == INSIDE) {
		*cdir = BACKW;
		return pcb_true;
	}
	if (p == 'A' && v->Flags.status == OUTSIDE) {
		*cdir = FORW;
		return pcb_true;
	}
	if (v->Flags.status == SHARED2) {
		if (p == 'A')
			*cdir = FORW;
		else
			*cdir = BACKW;
		return pcb_true;
	}
	return pcb_false;
}

/* return the edge that comes next.
 * if the direction is BACKW, then we return the next vertex
 * so that prev vertex has the flags for the edge
 *
 * returns pcb_true if an edge is found, pcb_false otherwise
 */
static int jump(pcb_vnode_t ** cur, DIRECTION * cdir, J_Rule rule)
{
	pcb_cvc_list_t *d, *start;
	pcb_vnode_t *e;
	DIRECTION newone;

	if (!(*cur)->cvc_prev) {			/* not a cross-vertex */
		if (*cdir == FORW ? (*cur)->Flags.mark : (*cur)->prev->Flags.mark)
			return pcb_false;
		return pcb_true;
	}
#ifdef DEBUG_JUMP
	DEBUGP("jump entering node at %$mD\n", (*cur)->point[0], (*cur)->point[1]);
#endif
	if (*cdir == FORW)
		d = (*cur)->cvc_prev->prev;
	else
		d = (*cur)->cvc_next->prev;
	start = d;
	do {
		e = d->parent;
		if (d->side == 'P')
			e = e->prev;
		newone = *cdir;
		if (!e->Flags.mark && rule(d->poly, e, &newone)) {
			if ((d->side == 'N' && newone == FORW) || (d->side == 'P' && newone == BACKW)) {
#ifdef DEBUG_JUMP
				if (newone == FORW)
					DEBUGP("jump leaving node at %#mD\n", e->next->point[0], e->next->point[1]);
				else
					DEBUGP("jump leaving node at %#mD\n", e->point[0], e->point[1]);
#endif
				*cur = d->parent;
				*cdir = newone;
				return pcb_true;
			}
		}
	}
	while ((d = d->prev) != start);
	return pcb_false;
}

static int Gather(pcb_vnode_t * start, pcb_pline_t ** result, J_Rule v_rule, DIRECTION initdir)
{
	pcb_vnode_t *cur = start, *newn;
	DIRECTION dir = initdir;
#ifdef DEBUG_GATHER
	DEBUGP("gather direction = %d\n", dir);
#endif
	assert(*result == NULL);
	do {
		/* see where to go next */
		if (!jump(&cur, &dir, v_rule))
			break;
		/* add edge to polygon */
		if (!*result) {
			*result = pcb_poly_contour_new(cur->point);
			if (*result == NULL)
				return err_no_memory;
		}
		else {
			if ((newn = pcb_poly_node_create(cur->point)) == NULL)
				return err_no_memory;
			pcb_poly_vertex_include((*result)->head.prev, newn);
		}
#ifdef DEBUG_GATHER
		DEBUGP("gather vertex at %#mD\n", cur->point[0], cur->point[1]);
#endif
		/* Now mark the edge as included.  */
		newn = (dir == FORW ? cur : cur->prev);
		newn->Flags.mark = 1;
		/* for SHARED edge mark both */
		if (newn->shared)
			newn->shared->Flags.mark = 1;

		/* Advance to the next edge.  */
		cur = (dir == FORW ? cur->next : cur->prev);
	}
	while (1);
	return err_ok;
}																/* Gather */

static void Collect1(jmp_buf * e, pcb_vnode_t * cur, DIRECTION dir, pcb_polyarea_t ** contours, pcb_pline_t ** holes, J_Rule j_rule)
{
	pcb_pline_t *p = NULL;							/* start making contour */
	int errc = err_ok;
	if ((errc = Gather(dir == FORW ? cur : cur->next, &p, j_rule, dir)) != err_ok) {
		if (p != NULL)
			pcb_poly_contour_del(&p);
		error(errc);
	}
	if (!p)
		return;
	pcb_poly_contour_pre(p, pcb_true);
	if (p->Count > 2) {
#ifdef DEBUG_GATHER
		DEBUGP("adding contour with %d vertices and direction %c\n", p->Count, p->Flags.orient ? 'F' : 'B');
#endif
		PutContour(e, p, contours, holes, NULL, NULL, NULL);
	}
	else {
		/* some sort of computation error ? */
#ifdef DEBUG_GATHER
		DEBUGP("Bad contour! Not enough points!!\n");
#endif
		pcb_poly_contour_del(&p);
	}
}

static void Collect(jmp_buf * e, pcb_pline_t * a, pcb_polyarea_t ** contours, pcb_pline_t ** holes, S_Rule s_rule, J_Rule j_rule)
{
	pcb_vnode_t *cur, *other;
	DIRECTION dir;

	cur = &a->head;
	do {
		if (s_rule(cur, &dir) && cur->Flags.mark == 0)
			Collect1(e, cur, dir, contours, holes, j_rule);
		other = cur;
		if ((other->cvc_prev && jump(&other, &dir, j_rule)))
			Collect1(e, other, dir, contours, holes, j_rule);
	}
	while ((cur = cur->next) != &a->head);
}																/* Collect */


static int
cntr_Collect(jmp_buf * e, pcb_pline_t ** A, pcb_polyarea_t ** contours, pcb_pline_t ** holes,
						 int action, pcb_polyarea_t * owner, pcb_polyarea_t * parent, pcb_pline_t * parent_contour)
{
	pcb_pline_t *tmprev;

	if ((*A)->Flags.status == ISECTED) {
		switch (action) {
		case PBO_UNITE:
			Collect(e, *A, contours, holes, UniteS_Rule, UniteJ_Rule);
			break;
		case PBO_ISECT:
			Collect(e, *A, contours, holes, IsectS_Rule, IsectJ_Rule);
			break;
		case PBO_XOR:
			Collect(e, *A, contours, holes, XorS_Rule, XorJ_Rule);
			break;
		case PBO_SUB:
			Collect(e, *A, contours, holes, SubS_Rule, SubJ_Rule);
			break;
		};
	}
	else {
		switch (action) {
		case PBO_ISECT:
			if ((*A)->Flags.status == INSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				PutContour(e, tmprev, contours, holes, owner, NULL, NULL);
				return pcb_true;
			}
			break;
		case PBO_XOR:
			if ((*A)->Flags.status == INSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				pcb_poly_contour_inv(tmprev);
				PutContour(e, tmprev, contours, holes, owner, NULL, NULL);
				return pcb_true;
			}
			/* break; *//* Fall through (I think this is correct! pcjc2) */
		case PBO_UNITE:
		case PBO_SUB:
			if ((*A)->Flags.status == OUTSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				PutContour(e, tmprev, contours, holes, owner, parent, parent_contour);
				return pcb_true;
			}
			break;
		}
	}
	return pcb_false;
}																/* cntr_Collect */

static void M_B_AREA_Collect(jmp_buf * e, pcb_polyarea_t * bfst, pcb_polyarea_t ** contours, pcb_pline_t ** holes, int action)
{
	pcb_polyarea_t *b = bfst;
	pcb_pline_t **cur, **next, *tmp;

	assert(b != NULL);
	do {
		for (cur = &b->contours; *cur != NULL; cur = next) {
			next = &((*cur)->next);
			if ((*cur)->Flags.status == ISECTED)
				continue;

			if ((*cur)->Flags.status == INSIDE)
				switch (action) {
				case PBO_XOR:
				case PBO_SUB:
					pcb_poly_contour_inv(*cur);
				case PBO_ISECT:
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->Flags.status = UNKNWN;
					PutContour(e, tmp, contours, holes, b, NULL, NULL);
					break;
				case PBO_UNITE:
					break;								/* nothing to do - already included */
				}
			else if ((*cur)->Flags.status == OUTSIDE)
				switch (action) {
				case PBO_XOR:
				case PBO_UNITE:
					/* include */
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->Flags.status = UNKNWN;
					PutContour(e, tmp, contours, holes, b, NULL, NULL);
					break;
				case PBO_ISECT:
				case PBO_SUB:
					break;								/* do nothing, not included */
				}
		}
	}
	while ((b = b->f) != bfst);
}


static inline int contour_is_first(pcb_polyarea_t * a, pcb_pline_t * cur)
{
	return (a->contours == cur);
}


static inline int contour_is_last(pcb_pline_t * cur)
{
	return (cur->next == NULL);
}


static inline void remove_polyarea(pcb_polyarea_t ** list, pcb_polyarea_t * piece)
{
	/* If this item was the start of the list, advance that pointer */
	if (*list == piece)
		*list = (*list)->f;

	/* But reset it to NULL if it wraps around and hits us again */
	if (*list == piece)
		*list = NULL;

	piece->b->f = piece->f;
	piece->f->b = piece->b;
	piece->f = piece->b = piece;
}

static void M_pcb_polyarea_separate_isected(jmp_buf * e, pcb_polyarea_t ** pieces, pcb_pline_t ** holes, pcb_pline_t ** isected)
{
	pcb_polyarea_t *a = *pieces;
	pcb_polyarea_t *anext;
	pcb_pline_t *curc, *next, *prev;
	int finished;

	if (a == NULL)
		return;

	/* TODO: STASH ENOUGH INFORMATION EARLIER ON, SO WE CAN REMOVE THE INTERSECTED
	   CONTOURS WITHOUT HAVING TO WALK THE FULL DATA-STRUCTURE LOOKING FOR THEM. */

	do {
		int hole_contour = 0;
		int is_outline = 1;

		anext = a->f;
		finished = (anext == *pieces);

		prev = NULL;
		for (curc = a->contours; curc != NULL; curc = next, is_outline = 0) {
			int is_first = contour_is_first(a, curc);
			int is_last = contour_is_last(curc);
			int isect_contour = (curc->Flags.status == ISECTED);

			next = curc->next;

			if (isect_contour || hole_contour) {

				/* Reset the intersection flags, since we keep these pieces */
				if (curc->Flags.status != ISECTED)
					curc->Flags.status = UNKNWN;

				remove_contour(a, prev, curc, !(is_first && is_last));

				if (isect_contour) {
					/* Link into the list of intersected contours */
					curc->next = *isected;
					*isected = curc;
				}
				else if (hole_contour) {
					/* Link into the list of holes */
					curc->next = *holes;
					*holes = curc;
				}
				else {
					assert(0);
				}

				if (is_first && is_last) {
					remove_polyarea(pieces, a);
					pcb_polyarea_free(&a);				/* NB: Sets a to NULL */
				}

			}
			else {
				/* Note the item we just didn't delete as the next
				   candidate for having its "next" pointer adjusted.
				   Saves walking the contour list when we delete one. */
				prev = curc;
			}

			/* If we move or delete an outer contour, we need to move any holes
			   we wish to keep within that contour to the holes list. */
			if (is_outline && isect_contour)
				hole_contour = 1;

		}

		/* If we deleted all the pieces of the polyarea, *pieces is NULL */
	}
	while ((a = anext), *pieces != NULL && !finished);
}


struct find_inside_m_pa_info {
	jmp_buf jb;
	pcb_polyarea_t *want_inside;
	pcb_pline_t *result;
};

static pcb_r_dir_t find_inside_m_pa(const pcb_box_t * b, void *cl)
{
	struct find_inside_m_pa_info *info = (struct find_inside_m_pa_info *) cl;
	pcb_pline_t *check = (pcb_pline_t *) b;
	/* Don't report for the main contour */
	if (check->Flags.orient == PLF_DIR)
		return R_DIR_NOT_FOUND;
	/* Don't look at contours marked as being intersected */
	if (check->Flags.status == ISECTED)
		return R_DIR_NOT_FOUND;
	if (cntr_in_M_pcb_polyarea_t(check, info->want_inside, pcb_false)) {
		info->result = check;
		longjmp(info->jb, 1);
	}
	return R_DIR_NOT_FOUND;
}


static void M_pcb_polyarea_t_update_primary(jmp_buf * e, pcb_polyarea_t ** pieces, pcb_pline_t ** holes, int action, pcb_polyarea_t * bpa)
{
	pcb_polyarea_t *a = *pieces;
	pcb_polyarea_t *b;
	pcb_polyarea_t *anext;
	pcb_pline_t *curc, *next, *prev;
	pcb_box_t box;
	/* int inv_inside = 0; */
	int del_inside = 0;
	int del_outside = 0;
	int finished;

	if (a == NULL)
		return;

	switch (action) {
	case PBO_ISECT:
		del_outside = 1;
		break;
	case PBO_UNITE:
	case PBO_SUB:
		del_inside = 1;
		break;
	case PBO_XOR:								/* NOT IMPLEMENTED OR USED */
		/* inv_inside = 1; */
		assert(0);
		break;
	}

	box = *((pcb_box_t *) bpa->contours);
	b = bpa;
	while ((b = b->f) != bpa) {
		pcb_box_t *b_box = (pcb_box_t *) b->contours;
		PCB_MAKE_MIN(box.X1, b_box->X1);
		PCB_MAKE_MIN(box.Y1, b_box->Y1);
		PCB_MAKE_MAX(box.X2, b_box->X2);
		PCB_MAKE_MAX(box.Y2, b_box->Y2);
	}

	if (del_inside) {

		do {
			anext = a->f;
			finished = (anext == *pieces);

			/* Test the outer contour first, as we may need to remove all children */

			/* We've not yet split intersected contours out, just ignore them */
			if (a->contours->Flags.status != ISECTED &&
					/* Pre-filter on bounding box */
					((a->contours->xmin >= box.X1) && (a->contours->ymin >= box.Y1)
					 && (a->contours->xmax <= box.X2)
					 && (a->contours->ymax <= box.Y2)) &&
					/* Then test properly */
					cntr_in_M_pcb_polyarea_t(a->contours, bpa, pcb_false)) {

				/* Delete this contour, all children -> holes queue */

				/* Delete the outer contour */
				curc = a->contours;
				remove_contour(a, NULL, curc, pcb_false);	/* Rtree deleted in poly_Free below */
				/* a->contours now points to the remaining holes */
				pcb_poly_contour_del(&curc);

				if (a->contours != NULL) {
					/* Find the end of the list of holes */
					curc = a->contours;
					while (curc->next != NULL)
						curc = curc->next;

					/* Take the holes and prepend to the holes queue */
					curc->next = *holes;
					*holes = a->contours;
					a->contours = NULL;
				}

				remove_polyarea(pieces, a);
				pcb_polyarea_free(&a);					/* NB: Sets a to NULL */

				continue;
			}

			/* Loop whilst we find INSIDE contours to delete */
			while (1) {
				struct find_inside_m_pa_info info;
				pcb_pline_t *prev;

				info.want_inside = bpa;

				/* Set jump return */
				if (setjmp(info.jb)) {
					/* Returned here! */
				}
				else {
					info.result = NULL;
					/* r-tree search, calling back a routine to longjmp back with
					 * data about any hole inside the B polygon.
					 * NB: Does not jump back to report the main contour!
					 */
					pcb_r_search(a->contour_tree, &box, NULL, find_inside_m_pa, &info, NULL);

					/* Nothing found? */
					break;
				}

				/* We need to find the contour before it, so we can update its next pointer */
				prev = a->contours;
				while (prev->next != info.result) {
					prev = prev->next;
				}

				/* Remove hole from the contour */
				remove_contour(a, prev, info.result, pcb_true);
				pcb_poly_contour_del(&info.result);
			}
			/* End check for deleted holes */

			/* If we deleted all the pieces of the polyarea, *pieces is NULL */
		}
		while ((a = anext), *pieces != NULL && !finished);

		return;
	}
	else {
		/* This path isn't optimised for speed */
	}

	do {
		int hole_contour = 0;
		int is_outline = 1;

		anext = a->f;
		finished = (anext == *pieces);

		prev = NULL;
		for (curc = a->contours; curc != NULL; curc = next, is_outline = 0) {
			int is_first = contour_is_first(a, curc);
			int is_last = contour_is_last(curc);
			int del_contour = 0;

			next = curc->next;

			if (del_outside)
				del_contour = curc->Flags.status != ISECTED && !cntr_in_M_pcb_polyarea_t(curc, bpa, pcb_false);

			/* Skip intersected contours */
			if (curc->Flags.status == ISECTED) {
				prev = curc;
				continue;
			}

			/* Reset the intersection flags, since we keep these pieces */
			curc->Flags.status = UNKNWN;

			if (del_contour || hole_contour) {

				remove_contour(a, prev, curc, !(is_first && is_last));

				if (del_contour) {
					/* Delete the contour */
					pcb_poly_contour_del(&curc);	/* NB: Sets curc to NULL */
				}
				else if (hole_contour) {
					/* Link into the list of holes */
					curc->next = *holes;
					*holes = curc;
				}
				else {
					assert(0);
				}

				if (is_first && is_last) {
					remove_polyarea(pieces, a);
					pcb_polyarea_free(&a);				/* NB: Sets a to NULL */
				}

			}
			else {
				/* Note the item we just didn't delete as the next
				   candidate for having its "next" pointer adjusted.
				   Saves walking the contour list when we delete one. */
				prev = curc;
			}

			/* If we move or delete an outer contour, we need to move any holes
			   we wish to keep within that contour to the holes list. */
			if (is_outline && del_contour)
				hole_contour = 1;

		}

		/* If we deleted all the pieces of the polyarea, *pieces is NULL */
	}
	while ((a = anext), *pieces != NULL && !finished);
}

static void
M_pcb_polyarea_t_Collect_separated(jmp_buf * e, pcb_pline_t * afst, pcb_polyarea_t ** contours, pcb_pline_t ** holes, int action, pcb_bool maybe)
{
	pcb_pline_t **cur, **next;

	for (cur = &afst; *cur != NULL; cur = next) {
		next = &((*cur)->next);
		/* if we disappear a contour, don't advance twice */
		if (cntr_Collect(e, cur, contours, holes, action, NULL, NULL, NULL))
			next = cur;
	}
}

static void M_pcb_polyarea_t_Collect(jmp_buf * e, pcb_polyarea_t * afst, pcb_polyarea_t ** contours, pcb_pline_t ** holes, int action, pcb_bool maybe)
{
	pcb_polyarea_t *a = afst;
	pcb_polyarea_t *parent = NULL;			/* Quiet compiler warning */
	pcb_pline_t **cur, **next, *parent_contour;

	assert(a != NULL);
	while ((a = a->f) != afst);
	/* now the non-intersect parts are collected in temp/holes */
	do {
		if (maybe && a->contours->Flags.status != ISECTED)
			parent_contour = a->contours;
		else
			parent_contour = NULL;

		/* Take care of the first contour - so we know if we
		 * can shortcut reparenting some of its children
		 */
		cur = &a->contours;
		if (*cur != NULL) {
			next = &((*cur)->next);
			/* if we disappear a contour, don't advance twice */
			if (cntr_Collect(e, cur, contours, holes, action, a, NULL, NULL)) {
				parent = (*contours)->b;	/* InsCntr inserts behind the head */
				next = cur;
			}
			else
				parent = a;
			cur = next;
		}
		for (; *cur != NULL; cur = next) {
			next = &((*cur)->next);
			/* if we disappear a contour, don't advance twice */
			if (cntr_Collect(e, cur, contours, holes, action, a, parent, parent_contour))
				next = cur;
		}
	}
	while ((a = a->f) != afst);
}

/* determine if two polygons touch or overlap */
pcb_bool pcb_polyarea_touching(pcb_polyarea_t * a, pcb_polyarea_t * b)
{
	jmp_buf e;
	int code;

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		if (!pcb_poly_valid(a))
			return -1;
		if (!pcb_poly_valid(b))
			return -1;
#endif
		M_pcb_polyarea_t_intersect(&e, a, b, pcb_false);

		if (M_pcb_polyarea_t_label(a, b, pcb_true))
			return pcb_true;
		if (M_pcb_polyarea_t_label(b, a, pcb_true))
			return pcb_true;
	}
	else if (code == TOUCHES)
		return pcb_true;
	return pcb_false;
}

/* the main clipping routines */
int pcb_polyarea_boolean(const pcb_polyarea_t * a_org, const pcb_polyarea_t * b_org, pcb_polyarea_t ** res, int action)
{
	pcb_polyarea_t *a = NULL, *b = NULL;

	if (!pcb_polyarea_m_copy0(&a, a_org) || !pcb_polyarea_m_copy0(&b, b_org))
		return err_no_memory;

	return pcb_polyarea_boolean_free(a, b, res, action);
}																/* poly_Boolean */

/* just like poly_Boolean but frees the input polys */
int pcb_polyarea_boolean_free(pcb_polyarea_t * ai, pcb_polyarea_t * bi, pcb_polyarea_t ** res, int action)
{
	pcb_polyarea_t *a = ai, *b = bi;
	pcb_pline_t *a_isected = NULL;
	pcb_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*res = NULL;

	if (!a) {
		switch (action) {
		case PBO_XOR:
		case PBO_UNITE:
			*res = bi;
			return err_ok;
		case PBO_SUB:
		case PBO_ISECT:
			if (b != NULL)
				pcb_polyarea_free(&b);
			return err_ok;
		}
	}
	if (!b) {
		switch (action) {
		case PBO_SUB:
		case PBO_XOR:
		case PBO_UNITE:
			*res = ai;
			return err_ok;
		case PBO_ISECT:
			if (a != NULL)
				pcb_polyarea_free(&a);
			return err_ok;
		}
	}

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		assert(pcb_poly_valid(a));
		assert(pcb_poly_valid(b));
#endif

		/* intersect needs to make a list of the contours in a and b which are intersected */
		M_pcb_polyarea_t_intersect(&e, a, b, pcb_true);

		/* We could speed things up a lot here if we only processed the relevant contours */
		/* NB: Relevant parts of a are labeled below */
		M_pcb_polyarea_t_label(b, a, pcb_false);

		*res = a;
		M_pcb_polyarea_t_update_primary(&e, res, &holes, action, b);
		M_pcb_polyarea_separate_isected(&e, res, &holes, &a_isected);
		M_pcb_polyarea_t_label_separated(a_isected, b, pcb_false);
		M_pcb_polyarea_t_Collect_separated(&e, a_isected, res, &holes, action, pcb_false);
		M_B_AREA_Collect(&e, b, res, &holes, action);
		pcb_polyarea_free(&b);

		/* free a_isected */
		while ((p = a_isected) != NULL) {
			a_isected = p->next;
			pcb_poly_contour_del(&p);
		}

		InsertHoles(&e, *res, &holes);
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		pcb_poly_contour_del(&p);
	}

	if (code) {
		pcb_polyarea_free(res);
		return code;
	}
	assert(!*res || pcb_poly_valid(*res));
	return code;
}																/* poly_Boolean_free */

static void clear_marks(pcb_polyarea_t * p)
{
	pcb_polyarea_t *n = p;
	pcb_pline_t *c;
	pcb_vnode_t *v;

	do {
		for (c = n->contours; c; c = c->next) {
			v = &c->head;
			do {
				v->Flags.mark = 0;
			}
			while ((v = v->next) != &c->head);
		}
	}
	while ((n = n->f) != p);
}

/* compute the intersection and subtraction (divides "a" into two pieces)
 * and frees the input polys. This assumes that bi is a single simple polygon.
 */
int pcb_polyarea_and_subtract_free(pcb_polyarea_t * ai, pcb_polyarea_t * bi, pcb_polyarea_t ** aandb, pcb_polyarea_t ** aminusb)
{
	pcb_polyarea_t *a = ai, *b = bi;
	pcb_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*aandb = NULL;
	*aminusb = NULL;

	if ((code = setjmp(e)) == 0) {

#ifdef DEBUG
		if (!pcb_poly_valid(a))
			return -1;
		if (!pcb_poly_valid(b))
			return -1;
#endif
		M_pcb_polyarea_t_intersect(&e, a, b, pcb_true);

		M_pcb_polyarea_t_label(a, b, pcb_false);
		M_pcb_polyarea_t_label(b, a, pcb_false);

		M_pcb_polyarea_t_Collect(&e, a, aandb, &holes, PBO_ISECT, pcb_false);
		InsertHoles(&e, *aandb, &holes);
		assert(pcb_poly_valid(*aandb));
		/* delete holes if any left */
		while ((p = holes) != NULL) {
			holes = p->next;
			pcb_poly_contour_del(&p);
		}
		holes = NULL;
		clear_marks(a);
		clear_marks(b);
		M_pcb_polyarea_t_Collect(&e, a, aminusb, &holes, PBO_SUB, pcb_false);
		InsertHoles(&e, *aminusb, &holes);
		pcb_polyarea_free(&a);
		pcb_polyarea_free(&b);
		assert(pcb_poly_valid(*aminusb));
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		pcb_poly_contour_del(&p);
	}


	if (code) {
		pcb_polyarea_free(aandb);
		pcb_polyarea_free(aminusb);
		return code;
	}
	assert(!*aandb || pcb_poly_valid(*aandb));
	assert(!*aminusb || pcb_poly_valid(*aminusb));
	return code;
}																/* poly_AndSubtract_free */

static inline int cntrbox_pointin(pcb_pline_t * c, pcb_vector_t p)
{
	return (p[0] >= c->xmin && p[1] >= c->ymin && p[0] <= c->xmax && p[1] <= c->ymax);

}

static inline int node_neighbours(pcb_vnode_t * a, pcb_vnode_t * b)
{
	return (a == b) || (a->next == b) || (b->next == a) || (a->next == b->next);
}

pcb_vnode_t *pcb_poly_node_create(pcb_vector_t v)
{
	pcb_vnode_t *res;
	pcb_coord_t *c;

	assert(v);
	res = (pcb_vnode_t *) calloc(1, sizeof(pcb_vnode_t));
	if (res == NULL)
		return NULL;
	/* bzero (res, sizeof (pcb_vnode_t) - sizeof(pcb_vector_t)); */
	c = res->point;
	*c++ = *v++;
	*c = *v;
	return res;
}

void pcb_poly_contour_init(pcb_pline_t * c)
{
	if (c == NULL)
		return;
	/* bzero (c, sizeof(pcb_pline_t)); */
	c->head.next = c->head.prev = &c->head;
	c->xmin = c->ymin = 0x7fffffff;
	c->xmax = c->ymax = 0x80000000;
	c->is_round = pcb_false;
	c->cx = 0;
	c->cy = 0;
	c->radius = 0;
}

pcb_pline_t *pcb_poly_contour_new(pcb_vector_t v)
{
	pcb_pline_t *res;

	res = (pcb_pline_t *) calloc(1, sizeof(pcb_pline_t));
	if (res == NULL)
		return NULL;

	pcb_poly_contour_init(res);

	if (v != NULL) {
		Vcopy(res->head.point, v);
		cntrbox_adjust(res, v);
	}

	return res;
}

void pcb_poly_contour_clear(pcb_pline_t * c)
{
	pcb_vnode_t *cur;

	assert(c != NULL);
	while ((cur = c->head.next) != &c->head) {
		pcb_poly_vertex_exclude(cur);
		free(cur);
	}
	pcb_poly_contour_init(c);
}

void pcb_poly_contour_del(pcb_pline_t ** c)
{
	pcb_vnode_t *cur, *prev;

	if (*c == NULL)
		return;
	for (cur = (*c)->head.prev; cur != &(*c)->head; cur = prev) {
		prev = cur->prev;
		if (cur->cvc_next != NULL) {
			free(cur->cvc_next);
			free(cur->cvc_prev);
		}
		free(cur);
	}
	if ((*c)->head.cvc_next != NULL) {
		free((*c)->head.cvc_next);
		free((*c)->head.cvc_prev);
	}
	/* FIXME -- strict aliasing violation.  */
	if ((*c)->tree) {
		pcb_rtree_t *r = (*c)->tree;
		pcb_r_destroy_tree(&r);
	}
	free(*c), *c = NULL;
}

void pcb_poly_contour_pre(pcb_pline_t * C, pcb_bool optimize)
{
	double area = 0;
	pcb_vnode_t *p, *c;
	pcb_vector_t p1, p2;

	assert(C != NULL);

	if (optimize) {
		for (c = (p = &C->head)->next; c != &C->head; c = (p = c)->next) {
			/* if the previous node is on the same line with this one, we should remove it */
			Vsub2(p1, c->point, p->point);
			Vsub2(p2, c->next->point, c->point);
			/* If the product below is zero then
			 * the points on either side of c 
			 * are on the same line!
			 * So, remove the point c
			 */

			if (pcb_vect_det2(p1, p2) == 0) {
				pcb_poly_vertex_exclude(c);
				free(c);
				c = p;
			}
		}
	}
	C->Count = 0;
	C->xmin = C->xmax = C->head.point[0];
	C->ymin = C->ymax = C->head.point[1];

	p = (c = &C->head)->prev;
	if (c != p) {
		do {
			/* calculate area for orientation */
			area += (double) (p->point[0] - c->point[0]) * (p->point[1] + c->point[1]);
			cntrbox_adjust(C, c->point);
			C->Count++;
		}
		while ((c = (p = c)->next) != &C->head);
	}
	C->area = PCB_ABS(area);
	if (C->Count > 2)
		C->Flags.orient = ((area < 0) ? PLF_INV : PLF_DIR);
	C->tree = (pcb_rtree_t *) make_edge_tree(C);
}																/* poly_PreContour */

static pcb_r_dir_t flip_cb(const pcb_box_t * b, void *cl)
{
	struct seg *s = (struct seg *) b;
	s->v = s->v->prev;
	return R_DIR_FOUND_CONTINUE;
}

void pcb_poly_contour_inv(pcb_pline_t * c)
{
	pcb_vnode_t *cur, *next;
	int r;

	assert(c != NULL);
	cur = &c->head;
	do {
		next = cur->next;
		cur->next = cur->prev;
		cur->prev = next;
		/* fix the segment tree */
	}
	while ((cur = next) != &c->head);
	c->Flags.orient ^= 1;
	if (c->tree) {
		pcb_r_search(c->tree, NULL, NULL, flip_cb, NULL, &r);
		assert(r == c->Count);
	}
}

void pcb_poly_vertex_exclude(pcb_vnode_t * node)
{
	assert(node != NULL);
	if (node->cvc_next) {
		free(node->cvc_next);
		free(node->cvc_prev);
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

void pcb_poly_vertex_include(pcb_vnode_t * after, pcb_vnode_t * node)
{
	double a, b;
	assert(after != NULL);
	assert(node != NULL);

	node->prev = after;
	node->next = after->next;
	after->next = after->next->prev = node;
	/* remove points on same line */
	if (node->prev->prev == node)
		return;											/* we don't have 3 points in the poly yet */
	a = (node->point[1] - node->prev->prev->point[1]);
	a *= (node->prev->point[0] - node->prev->prev->point[0]);
	b = (node->point[0] - node->prev->prev->point[0]);
	b *= (node->prev->point[1] - node->prev->prev->point[1]);
	if (fabs(a - b) < EPSILON) {
		pcb_vnode_t *t = node->prev;
		t->prev->next = node;
		node->prev = t->prev;
		free(t);
	}
}

pcb_bool pcb_poly_contour_copy(pcb_pline_t ** dst, pcb_pline_t * src)
{
	pcb_vnode_t *cur, *newnode;

	assert(src != NULL);
	*dst = NULL;
	*dst = pcb_poly_contour_new(src->head.point);
	if (*dst == NULL)
		return pcb_false;

	(*dst)->Count = src->Count;
	(*dst)->Flags.orient = src->Flags.orient;
	(*dst)->xmin = src->xmin, (*dst)->xmax = src->xmax;
	(*dst)->ymin = src->ymin, (*dst)->ymax = src->ymax;
	(*dst)->area = src->area;

	for (cur = src->head.next; cur != &src->head; cur = cur->next) {
		if ((newnode = pcb_poly_node_create(cur->point)) == NULL)
			return pcb_false;
		/* newnode->Flags = cur->Flags; */
		pcb_poly_vertex_include((*dst)->head.prev, newnode);
	}
	(*dst)->tree = (pcb_rtree_t *) make_edge_tree(*dst);
	return pcb_true;
}

/**********************************************************************/
/* polygon routines */

pcb_bool pcb_polyarea_copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * src)
{
	*dst = NULL;
	if (src != NULL)
		*dst = (pcb_polyarea_t *) calloc(1, sizeof(pcb_polyarea_t));
	if (*dst == NULL)
		return pcb_false;
	(*dst)->contour_tree = pcb_r_create_tree(NULL, 0, 0);

	return pcb_polyarea_copy1(*dst, src);
}

pcb_bool pcb_polyarea_copy1(pcb_polyarea_t * dst, const pcb_polyarea_t * src)
{
	pcb_pline_t *cur, **last = &dst->contours;

	*last = NULL;
	dst->f = dst->b = dst;

	for (cur = src->contours; cur != NULL; cur = cur->next) {
		if (!pcb_poly_contour_copy(last, cur))
			return pcb_false;
		pcb_r_insert_entry(dst->contour_tree, (pcb_box_t *) * last, 0);
		last = &(*last)->next;
	}
	return pcb_true;
}

void pcb_polyarea_m_include(pcb_polyarea_t ** list, pcb_polyarea_t * a)
{
	if (*list == NULL)
		a->f = a->b = a, *list = a;
	else {
		a->f = *list;
		a->b = (*list)->b;
		(*list)->b = (*list)->b->f = a;
	}
}

pcb_bool pcb_polyarea_m_copy0(pcb_polyarea_t ** dst, const pcb_polyarea_t * srcfst)
{
	const pcb_polyarea_t *src = srcfst;
	pcb_polyarea_t *di;

	*dst = NULL;
	if (src == NULL)
		return pcb_false;
	do {
		if ((di = pcb_polyarea_create()) == NULL || !pcb_polyarea_copy1(di, src))
			return pcb_false;
		pcb_polyarea_m_include(dst, di);
	}
	while ((src = src->f) != srcfst);
	return pcb_true;
}

pcb_bool pcb_polyarea_contour_include(pcb_polyarea_t * p, pcb_pline_t * c)
{
	pcb_pline_t *tmp;

	if ((c == NULL) || (p == NULL))
		return pcb_false;
	if (c->Flags.orient == PLF_DIR) {
		if (p->contours != NULL)
			return pcb_false;
		p->contours = c;
	}
	else {
		if (p->contours == NULL)
			return pcb_false;
		/* link at front of hole list */
		tmp = p->contours->next;
		p->contours->next = c;
		c->next = tmp;
	}
	pcb_r_insert_entry(p->contour_tree, (pcb_box_t *) c, 0);
	return pcb_true;
}

typedef struct pip {
	int f;
	pcb_vector_t p;
	jmp_buf env;
} pip;


static pcb_r_dir_t crossing(const pcb_box_t * b, void *cl)
{
	struct seg *s = (struct seg *) b;
	struct pip *p = (struct pip *) cl;

	if (s->v->point[1] <= p->p[1]) {
		if (s->v->next->point[1] > p->p[1]) {
			pcb_vector_t v1, v2;
			pcb_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (pcb_long64_t) v1[0] * v2[1] - (pcb_long64_t) v2[0] * v1[1];
			if (cross == 0) {
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross > 0)
				p->f += 1;
		}
	}
	else {
		if (s->v->next->point[1] <= p->p[1]) {
			pcb_vector_t v1, v2;
			pcb_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (pcb_long64_t) v1[0] * v2[1] - (pcb_long64_t) v2[0] * v1[1];
			if (cross == 0) {
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross < 0)
				p->f -= 1;
		}
	}
	return R_DIR_FOUND_CONTINUE;
}

int pcb_poly_contour_inside(pcb_pline_t * c, pcb_vector_t p)
{
	struct pip info;
	pcb_box_t ray;

	if (!cntrbox_pointin(c, p))
		return pcb_false;
	info.f = 0;
	info.p[0] = ray.X1 = p[0];
	info.p[1] = ray.Y1 = p[1];
	ray.X2 = 0x7fffffff;
	ray.Y2 = p[1] + 1;
	if (setjmp(info.env) == 0)
		pcb_r_search(c->tree, &ray, NULL, crossing, &info, NULL);
	return info.f;
}

pcb_bool pcb_polyarea_contour_inside(pcb_polyarea_t * p, pcb_vector_t v0)
{
	pcb_pline_t *cur;

	if ((p == NULL) || (v0 == NULL) || (p->contours == NULL))
		return pcb_false;
	cur = p->contours;
	if (pcb_poly_contour_inside(cur, v0)) {
		for (cur = cur->next; cur != NULL; cur = cur->next)
			if (pcb_poly_contour_inside(cur, v0))
				return pcb_false;
		return pcb_true;
	}
	return pcb_false;
}

pcb_bool poly_M_CheckInside(pcb_polyarea_t * p, pcb_vector_t v0)
{
	pcb_polyarea_t *cur;

	if ((p == NULL) || (v0 == NULL))
		return pcb_false;
	cur = p;
	do {
		if (pcb_polyarea_contour_inside(cur, v0))
			return pcb_true;
	}
	while ((cur = cur->f) != p);
	return pcb_false;
}

static double dot(pcb_vector_t A, pcb_vector_t B)
{
	return (double) A[0] * (double) B[0] + (double) A[1] * (double) B[1];
}

/* Compute whether point is inside a triangle formed by 3 other pints */
/* Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html */
static int point_in_triangle(pcb_vector_t A, pcb_vector_t B, pcb_vector_t C, pcb_vector_t P)
{
	pcb_vector_t v0, v1, v2;
	double dot00, dot01, dot02, dot11, dot12;
	double invDenom;
	double u, v;

	/* Compute vectors */
	v0[0] = C[0] - A[0];
	v0[1] = C[1] - A[1];
	v1[0] = B[0] - A[0];
	v1[1] = B[1] - A[1];
	v2[0] = P[0] - A[0];
	v2[1] = P[1] - A[1];

	/* Compute dot products */
	dot00 = dot(v0, v0);
	dot01 = dot(v0, v1);
	dot02 = dot(v0, v2);
	dot11 = dot(v1, v1);
	dot12 = dot(v1, v2);

	/* Compute barycentric coordinates */
	invDenom = 1. / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	/* Check if point is in triangle */
	return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}


/* Returns the dot product of pcb_vector_t A->B, and a vector
 * orthogonal to pcb_vector_t C->D. The result is not normalised, so will be
 * weighted by the magnitude of the C->D vector.
 */
static double dot_orthogonal_to_direction(pcb_vector_t A, pcb_vector_t B, pcb_vector_t C, pcb_vector_t D)
{
	pcb_vector_t l1, l2, l3;
	l1[0] = B[0] - A[0];
	l1[1] = B[1] - A[1];
	l2[0] = D[0] - C[0];
	l2[1] = D[1] - C[1];

	l3[0] = -l2[1];
	l3[1] = l2[0];

	return dot(l1, l3);
}

/* Algorithm from http://www.exaflop.org/docs/cgafaq/cga2.html
 *
 * "Given a simple polygon, find some point inside it. Here is a method based
 * on the proof that there exists an internal diagonal, in [O'Rourke, 13-14].
 * The idea is that the midpoint of a diagonal is interior to the polygon.
 *
 * 1.  Identify a convex vertex v; let its adjacent vertices be a and b.
 * 2.  For each other vertex q do:
 * 2a. If q is inside avb, compute distance to v (orthogonal to ab).
 * 2b. Save point q if distance is a new min.
 * 3.  If no point is inside, return midpoint of ab, or centroid of avb.
 * 4.  Else if some point inside, qv is internal: return its midpoint."
 *
 * [O'Rourke]: Computational Geometry in C (2nd Ed.)
 *             Joseph O'Rourke, Cambridge University Press 1998,
 *             ISBN 0-521-64010-5 Pbk, ISBN 0-521-64976-5 Hbk
 */
static void poly_ComputeInteriorPoint(pcb_pline_t * poly, pcb_vector_t v)
{
	pcb_vnode_t *pt1, *pt2, *pt3, *q;
	pcb_vnode_t *min_q = NULL;
	double dist;
	double min_dist = 0.0;
	double dir = (poly->Flags.orient == PLF_DIR) ? 1. : -1;

	/* Find a convex node on the polygon */
	pt1 = &poly->head;
	do {
		double dot_product;

		pt2 = pt1->next;
		pt3 = pt2->next;

		dot_product = dot_orthogonal_to_direction(pt1->point, pt2->point, pt3->point, pt2->point);

		if (dot_product * dir > 0.)
			break;
	}
	while ((pt1 = pt1->next) != &poly->head);

	/* Loop over remaining vertices */
	q = pt3;
	do {
		/* Is current vertex "q" outside pt1 pt2 pt3 triangle? */
		if (!point_in_triangle(pt1->point, pt2->point, pt3->point, q->point))
			continue;

		/* NO: compute distance to pt2 (v) orthogonal to pt1 - pt3) */
		/*     Record minimum */
		dist = dot_orthogonal_to_direction(q->point, pt2->point, pt1->point, pt3->point);
		if (min_q == NULL || dist < min_dist) {
			min_dist = dist;
			min_q = q;
		}
	}
	while ((q = q->next) != pt2);

	/* Were any "q" found inside pt1 pt2 pt3? */
	if (min_q == NULL) {
		/* NOT FOUND: Return midpoint of pt1 pt3 */
		v[0] = (pt1->point[0] + pt3->point[0]) / 2;
		v[1] = (pt1->point[1] + pt3->point[1]) / 2;
	}
	else {
		/* FOUND: Return mid point of min_q, pt2 */
		v[0] = (pt2->point[0] + min_q->point[0]) / 2;
		v[1] = (pt2->point[1] + min_q->point[1]) / 2;
	}
}


/* NB: This function assumes the caller _knows_ the contours do not
 *     intersect. If the contours intersect, the result is undefined.
 *     It will return the correct result if the two contours share
 *     common points between their contours. (Identical contours
 *     are treated as being inside each other).
 */
int pcb_poly_contour_in_contour(pcb_pline_t * poly, pcb_pline_t * inner)
{
	pcb_vector_t point;
	assert(poly != NULL);
	assert(inner != NULL);
	if (cntrbox_inside(inner, poly)) {
		/* We need to prove the "inner" contour is not outside
		 * "poly" contour. If it is outside, we can return.
		 */
		if (!pcb_poly_contour_inside(poly, inner->head.point))
			return 0;

		poly_ComputeInteriorPoint(inner, point);
		return pcb_poly_contour_inside(poly, point);
	}
	return 0;
}

void pcb_polyarea_init(pcb_polyarea_t * p)
{
	p->f = p->b = p;
	p->contours = NULL;
	p->contour_tree = pcb_r_create_tree(NULL, 0, 0);
}

pcb_polyarea_t *pcb_polyarea_create(void)
{
	pcb_polyarea_t *res;

	if ((res = (pcb_polyarea_t *) malloc(sizeof(pcb_polyarea_t))) != NULL)
		pcb_polyarea_init(res);
	return res;
}

void pcb_poly_contours_free(pcb_pline_t ** pline)
{
	pcb_pline_t *pl;

	while ((pl = *pline) != NULL) {
		*pline = pl->next;
		pcb_poly_contour_del(&pl);
	}
}

void pcb_polyarea_free(pcb_polyarea_t ** p)
{
	pcb_polyarea_t *cur;

	if (*p == NULL)
		return;
	for (cur = (*p)->f; cur != *p; cur = (*p)->f) {
		pcb_poly_contours_free(&cur->contours);
		pcb_r_destroy_tree(&cur->contour_tree);
		cur->f->b = cur->b;
		cur->b->f = cur->f;
		free(cur);
	}
	pcb_poly_contours_free(&cur->contours);
	pcb_r_destroy_tree(&cur->contour_tree);
	free(*p), *p = NULL;
}

static pcb_bool inside_sector(pcb_vnode_t * pn, pcb_vector_t p2)
{
	pcb_vector_t cdir, ndir, pdir;
	int p_c, n_c, p_n;

	assert(pn != NULL);
	vect_sub(cdir, p2, pn->point);
	vect_sub(pdir, pn->point, pn->prev->point);
	vect_sub(ndir, pn->next->point, pn->point);

	p_c = pcb_vect_det2(pdir, cdir) >= 0;
	n_c = pcb_vect_det2(ndir, cdir) >= 0;
	p_n = pcb_vect_det2(pdir, ndir) >= 0;

	if ((p_n && p_c && n_c) || ((!p_n) && (p_c || n_c)))
		return pcb_true;
	else
		return pcb_false;
}																/* inside_sector */

/* returns pcb_true if bad contour */
pcb_bool pcb_polyarea_contour_check(pcb_pline_t * a)
{
	pcb_vnode_t *a1, *a2, *hit1, *hit2;
	pcb_vector_t i1, i2;
	int icnt;

	assert(a != NULL);
	a1 = &a->head;
	do {
		a2 = a1;
		do {
			if (!node_neighbours(a1, a2) && (icnt = pcb_vect_inters2(a1->point, a1->next->point, a2->point, a2->next->point, i1, i2)) > 0) {
				if (icnt > 1)
					return pcb_true;

				if (pcb_vect_dist2(i1, a1->point) < EPSILON)
					hit1 = a1;
				else if (pcb_vect_dist2(i1, a1->next->point) < EPSILON)
					hit1 = a1->next;
				else
					hit1 = NULL;

				if (pcb_vect_dist2(i1, a2->point) < EPSILON)
					hit2 = a2;
				else if (pcb_vect_dist2(i1, a2->next->point) < EPSILON)
					hit2 = a2->next;
				else
					hit2 = NULL;

				if ((hit1 == NULL) && (hit2 == NULL)) {
					/* If the intersection didn't land on an end-point of either
					 * line, we know the lines cross and we return pcb_true.
					 */
					return pcb_true;
				}
				else if (hit1 == NULL) {
					/* An end-point of the second line touched somewhere along the
					   length of the first line. Check where the second line leads. */
					if (inside_sector(hit2, a1->point) != inside_sector(hit2, a1->next->point))
						return pcb_true;
				}
				else if (hit2 == NULL) {
					/* An end-point of the first line touched somewhere along the
					   length of the second line. Check where the first line leads. */
					if (inside_sector(hit1, a2->point) != inside_sector(hit1, a2->next->point))
						return pcb_true;
				}
				else {
					/* Both lines intersect at an end-point. Check where they lead. */
					if (inside_sector(hit1, hit2->prev->point) != inside_sector(hit1, hit2->next->point))
						return pcb_true;
				}
			}
		}
		while ((a2 = a2->next) != &a->head);
	}
	while ((a1 = a1->next) != &a->head);
	return pcb_false;
}

void pcb_polyarea_bbox(pcb_polyarea_t * p, pcb_box_t * b)
{
	pcb_pline_t *n;
	/*int cnt;*/

	n = p->contours;
	b->X1 = b->X2 = n->xmin;
	b->Y1 = b->Y2 = n->ymin;

	for (/*cnt = 0*/; /*cnt < 2 */ n != NULL; n = n->next) {
		if (n->xmin < b->X1)
			b->X1 = n->xmin;
		if (n->ymin < b->Y1)
			b->Y1 = n->ymin;
		if (n->xmax > b->X2)
			b->X2 = n->xmax;
		if (n->ymax > b->Y2)
			b->Y2 = n->ymax;
/*		if (n == p->contours)
			cnt++;*/
	}
}


pcb_bool pcb_poly_valid(pcb_polyarea_t * p)
{
	pcb_pline_t *c;

	if ((p == NULL) || (p->contours == NULL))
		return pcb_false;

	if (p->contours->Flags.orient == PLF_INV || pcb_polyarea_contour_check(p->contours)) {
#ifndef NDEBUG
		pcb_vnode_t *v, *n;
		DEBUGP("Invalid Outer pcb_pline_t\n");
		if (p->contours->Flags.orient == PLF_INV)
			DEBUGP("failed orient\n");
		if (pcb_polyarea_contour_check(p->contours))
			DEBUGP("failed self-intersection\n");
		v = &p->contours->head;
		do {
			n = v->next;
			pcb_fprintf(stderr, "Line [%#mS %#mS %#mS %#mS 100 100 \"\"]\n", v->point[0], v->point[1], n->point[0], n->point[1]);
		}
		while ((v = v->next) != &p->contours->head);
#endif
		return pcb_false;
	}
	for (c = p->contours->next; c != NULL; c = c->next) {
		if (c->Flags.orient == PLF_DIR || pcb_polyarea_contour_check(c) || !pcb_poly_contour_in_contour(p->contours, c)) {
#ifndef NDEBUG
			pcb_vnode_t *v, *n;
			DEBUGP("Invalid Inner pcb_pline_t orient = %d\n", c->Flags.orient);
			if (c->Flags.orient == PLF_DIR)
				DEBUGP("failed orient\n");
			if (pcb_polyarea_contour_check(c))
				DEBUGP("failed self-intersection\n");
			if (!pcb_poly_contour_in_contour(p->contours, c))
				DEBUGP("failed containment\n");
			v = &c->head;
			do {
				n = v->next;
				pcb_fprintf(stderr, "Line [%#mS %#mS %#mS %#mS 100 100 \"\"]\n", v->point[0], v->point[1], n->point[0], n->point[1]);
			}
			while ((v = v->next) != &c->head);
#endif
			return pcb_false;
		}
	}
	return pcb_true;
}


pcb_vector_t vect_zero = { (long) 0, (long) 0 };

/*********************************************************************/
/*             L o n g   V e c t o r   S t u f f                     */
/*********************************************************************/

void vect_init(pcb_vector_t v, double x, double y)
{
	v[0] = (long) x;
	v[1] = (long) y;
}																/* vect_init */

#define Vzero(a)   ((a)[0] == 0. && (a)[1] == 0.)

#define Vsub(a,b,c) {(a)[0]=(b)[0]-(c)[0];(a)[1]=(b)[1]-(c)[1];}

int vect_equal(pcb_vector_t v1, pcb_vector_t v2)
{
	return (v1[0] == v2[0] && v1[1] == v2[1]);
}																/* vect_equal */


void vect_sub(pcb_vector_t res, pcb_vector_t v1, pcb_vector_t v2)
{
Vsub(res, v1, v2)}							/* vect_sub */

void vect_min(pcb_vector_t v1, pcb_vector_t v2, pcb_vector_t v3)
{
	v1[0] = (v2[0] < v3[0]) ? v2[0] : v3[0];
	v1[1] = (v2[1] < v3[1]) ? v2[1] : v3[1];
}																/* vect_min */

void vect_max(pcb_vector_t v1, pcb_vector_t v2, pcb_vector_t v3)
{
	v1[0] = (v2[0] > v3[0]) ? v2[0] : v3[0];
	v1[1] = (v2[1] > v3[1]) ? v2[1] : v3[1];
}																/* vect_max */

double pcb_vect_len2(pcb_vector_t v)
{
	return ((double) v[0] * v[0] + (double) v[1] * v[1]);	/* why sqrt? only used for compares */
}

double pcb_vect_dist2(pcb_vector_t v1, pcb_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];

	return (dx * dx + dy * dy);		/* why sqrt */
}

/* value has sign of angle between vectors */
double pcb_vect_det2(pcb_vector_t v1, pcb_vector_t v2)
{
	return (((double) v1[0] * v2[1]) - ((double) v2[0] * v1[1]));
}

static double vect_m_dist(pcb_vector_t v1, pcb_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
	double dd = (dx * dx + dy * dy);	/* sqrt */

	if (dx > 0)
		return +dd;
	if (dx < 0)
		return -dd;
	if (dy > 0)
		return +dd;
	return -dd;
}																/* vect_m_dist */

/*
vect_inters2
 (C) 1993 Klamer Schutte
 (C) 1997 Michael Leonov, Alexey Nikitin
*/

int pcb_vect_inters2(pcb_vector_t p1, pcb_vector_t p2, pcb_vector_t q1, pcb_vector_t q2, pcb_vector_t S1, pcb_vector_t S2)
{
	double s, t, deel;
	double rpx, rpy, rqx, rqy;

	if (max(p1[0], p2[0]) < min(q1[0], q2[0]) ||
			max(q1[0], q2[0]) < min(p1[0], p2[0]) || max(p1[1], p2[1]) < min(q1[1], q2[1]) || max(q1[1], q2[1]) < min(p1[1], p2[1]))
		return 0;

	rpx = p2[0] - p1[0];
	rpy = p2[1] - p1[1];
	rqx = q2[0] - q1[0];
	rqy = q2[1] - q1[1];

	deel = rpy * rqx - rpx * rqy;	/* -vect_det(rp,rq); */

	/* coordinates are 30-bit integers so deel will be exactly zero
	 * if the lines are parallel
	 */

	if (deel == 0) {							/* parallel */
		double dc1, dc2, d1, d2, h;	/* Check to see whether p1-p2 and q1-q2 are on the same line */
		pcb_vector_t hp1, hq1, hp2, hq2, q1p1, q1q2;

		Vsub2(q1p1, q1, p1);
		Vsub2(q1q2, q1, q2);


		/* If this product is not zero then p1-p2 and q1-q2 are not on same line! */
		if (pcb_vect_det2(q1p1, q1q2) != 0)
			return 0;
		dc1 = 0;										/* m_len(p1 - p1) */

		dc2 = vect_m_dist(p1, p2);
		d1 = vect_m_dist(p1, q1);
		d2 = vect_m_dist(p1, q2);

/* Sorting the independent points from small to large */
		Vcpy2(hp1, p1);
		Vcpy2(hp2, p2);
		Vcpy2(hq1, q1);
		Vcpy2(hq2, q2);
		if (dc1 > dc2) {						/* hv and h are used as help-variable. */
			Vswp2(hp1, hp2);
			h = dc1, dc1 = dc2, dc2 = h;
		}
		if (d1 > d2) {
			Vswp2(hq1, hq2);
			h = d1, d1 = d2, d2 = h;
		}

/* Now the line-pieces are compared */

		if (dc1 < d1) {
			if (dc2 < d1)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp2);
				Vcpy2(S2, hq1);
			}
			else {
				Vcpy2(S1, hq1);
				Vcpy2(S2, hq2);
			};
		}
		else {
			if (dc1 > d2)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hp2);
			}
			else {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hq2);
			};
		}
		return (Vequ2(S1, S2) ? 1 : 2);
	}
	else {												/* not parallel */
		/*
		 * We have the lines:
		 * l1: p1 + s(p2 - p1)
		 * l2: q1 + t(q2 - q1)
		 * And we want to know the intersection point.
		 * Calculate t:
		 * p1 + s(p2-p1) = q1 + t(q2-q1)
		 * which is similar to the two equations:
		 * p1x + s * rpx = q1x + t * rqx
		 * p1y + s * rpy = q1y + t * rqy
		 * Multiplying these by rpy resp. rpx gives:
		 * rpy * p1x + s * rpx * rpy = rpy * q1x + t * rpy * rqx
		 * rpx * p1y + s * rpx * rpy = rpx * q1y + t * rpx * rqy
		 * Subtracting these gives:
		 * rpy * p1x - rpx * p1y = rpy * q1x - rpx * q1y + t * ( rpy * rqx - rpx * rqy )
		 * So t can be isolated:
		 * t = (rpy * ( p1x - q1x ) + rpx * ( - p1y + q1y )) / ( rpy * rqx - rpx * rqy )
		 * and s can be found similarly
		 * s = (rqy * (q1x - p1x) + rqx * (p1y - q1y))/( rqy * rpx - rqx * rpy)
		 */

		if (Vequ2(q1, p1) || Vequ2(q1, p2)) {
			S1[0] = q1[0];
			S1[1] = q1[1];
		}
		else if (Vequ2(q2, p1) || Vequ2(q2, p2)) {
			S1[0] = q2[0];
			S1[1] = q2[1];
		}
		else {
			s = (rqy * (p1[0] - q1[0]) + rqx * (q1[1] - p1[1])) / deel;
			if (s < 0 || s > 1.)
				return 0;
			t = (rpy * (p1[0] - q1[0]) + rpx * (q1[1] - p1[1])) / deel;
			if (t < 0 || t > 1.)
				return 0;

			S1[0] = q1[0] + ROUND(t * rqx);
			S1[1] = q1[1] + ROUND(t * rqy);
		}
		return 1;
	}
}																/* vect_inters2 */

/* how about expanding polygons so that edges can be arcs rather than
 * lines. Consider using the third coordinate to store the radius of the
 * arc. The arc would pass through the vertex points. Positive radius
 * would indicate the arc bows left (center on right of P1-P2 path)
 * negative radius would put the center on the other side. 0 radius
 * would mean the edge is a line instead of arc
 * The intersections of the two circles centered at the vertex points
 * would determine the two possible arc centers. If P2.x > P1.x then
 * the center with smaller Y is selected for positive r. If P2.y > P1.y
 * then the center with greater X is selected for positive r.
 *
 * the vec_inters2() routine would then need to handle line-line
 * line-arc and arc-arc intersections.
 *
 * perhaps reverse tracing the arc would require look-ahead to check
 * for arcs
 */
