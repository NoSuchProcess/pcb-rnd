/*
    libcdtr - Constrained Delaunay Triangulation using incremental algorithm

    Copyright (C) 2018 Wojciech Krutnik

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Constrained Delaunay Triangulation using incremental algorithm, as described in:
      Yizhi Lu and Wayne Wei-Ming Dai, "A numerical stable algorithm for constructing
      constrained Delaunay triangulation and application to multichip module layout,"
      China., 1991 International Conference on Circuits and Systems, Shenzhen, 1991,
      pp. 644-647 vol.2.
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "cdt.h"

#define DEBUG

#define LEFTPOINT(p1, p2) ((p1)->pos.x < (p2)->pos.x || ((p1)->pos.x == (p2)->pos.x && (p1)->pos.y > (p2)->pos.y))


static point_t *new_point(cdt_t *cdt, pos_t pos)
{
	point_t *p = *vtpoint_alloc_append(&cdt->points, 1);
	p->pos = pos;
	return p;
}

static edge_t *new_edge(cdt_t *cdt, point_t *p1, point_t *p2, int constrain)
{
	edge_t *e;
	assert(p1->pos.x != p2->pos.x || p1->pos.y != p2->pos.y);
	e = *vtedge_alloc_append(&cdt->edges, 1);
	/* always orient the edge to the right (or down when x1==x2; requires epsilon check?) */
	if (LEFTPOINT(p1, p2)) {
		e->endp[0] = p1;
		e->endp[1] = p2;
	}
	else {
		e->endp[0] = p2;
		e->endp[1] = p1;
	}
	e->is_constrained = constrain;
	p1->adj_edges = edgelist_prepend(p1->adj_edges, &e);
	p2->adj_edges = edgelist_prepend(p2->adj_edges, &e);
	return e;
}

edge_t *cdt_new_edge_(cdt_t *cdt, point_t *p1, point_t *p2, int constrain)
{
	return new_edge(cdt, p1, p2, constrain);
}

edge_t *get_edge_from_points(point_t *p1, point_t *p2)
{
	EDGELIST_FOREACH(e1, p1->adj_edges)
		EDGELIST_FOREACH(e2, p2->adj_edges)
			if (e1 == e2)
				return e1;
		POINTLIST_FOREACH_END();
	EDGELIST_FOREACH_END();
	return NULL;
}

static void order_triangle_points_ccw(point_t **p1, point_t **p2, point_t **p3)
{
	pointlist_node_t *points = NULL;
	points = pointlist_prepend(points, p1);
	points = pointlist_prepend(points, p2);
	points = pointlist_prepend(points, p3);

	/* first point */
	*p1 = LEFTPOINT(*p1, *p2) ? *p1 : *p2;
	*p1 = LEFTPOINT(*p1, *p3) ? *p1 : *p3;
	points = pointlist_remove_item(points, p1);

	/* two other points */
	if (ORIENT_CCW(*p1, points->item, points->next->item)) {
		*p2 = points->item;
		*p3 = points->next->item;
	} else {
		*p2 = points->next->item;
		*p3 = points->item;
	}
	points = pointlist_remove_front(points);
	points = pointlist_remove_front(points);
}

static triangle_t *new_triangle(cdt_t *cdt, point_t *p1, point_t *p2, point_t *p3)
{
	edge_t *e1, *e2, *e3;
	triangle_t *t = *vttriangle_alloc_append(&cdt->triangles, 1);

	assert(!ORIENT_COLLINEAR(p1, p2, p3));	/* points cannot be colinear */

	order_triangle_points_ccw(&p1, &p2, &p3);
	t->p[0] = p1;
	t->p[1] = p2;
	t->p[2] = p3;
	p1->adj_triangles = trianglelist_prepend(p1->adj_triangles, &t);
	p2->adj_triangles = trianglelist_prepend(p2->adj_triangles, &t);
	p3->adj_triangles = trianglelist_prepend(p3->adj_triangles, &t);

	e1 = get_edge_from_points(p1, p2);
	e2 = get_edge_from_points(p2, p3);
	e3 = get_edge_from_points(p3, p1);
	assert(e1 != NULL && e2 != NULL && e3 != NULL);
	t->e[0] = e1;
	t->e[1] = e2;
	t->e[2] = e3;

#define connect_adjacent_triangle(_t_, _edge_num_, _e_) \
	if ((_e_)->adj_t[0] == NULL && (_e_)->adj_t[1] == NULL) \
		(_e_)->adj_t[0] = (_t_); \
	else { \
		int i = (_e_)->adj_t[0] == NULL ? 0 : 1; \
		(_e_)->adj_t[i] = (_t_); \
		(_t_)->adj_t[(_edge_num_)] = (_e_)->adj_t[i^1]; \
		if ((_t_)->adj_t[(_edge_num_)] != NULL) { \
			for (i = 0; i < 3; i++) { \
				if ((_t_)->adj_t[(_edge_num_)]->e[i] == (_e_)) \
					(_t_)->adj_t[(_edge_num_)]->adj_t[i] = (_t_); \
			} \
		} \
	}

	connect_adjacent_triangle(t, 0, e1);
	connect_adjacent_triangle(t, 1, e2);
	connect_adjacent_triangle(t, 2, e3);
#undef connect_adjacent_triangle

	return t;
}

triangle_t *cdt_new_triangle_(cdt_t *cdt, point_t *p1, point_t *p2, point_t *p3)
{
	return new_triangle(cdt, p1, p2, p3);
}


static void remove_triangle(cdt_t *cdt, triangle_t *t)
{
	int i, j;

	for (i = 0; i < 3; i++) {
		/* disconnect adjacent points */
		t->p[i]->adj_triangles = trianglelist_remove_item(t->p[i]->adj_triangles, &t);
		/* disconnect adjacent edges */
		for (j = 0; j < 2; j++)
			if (t->e[i]->adj_t[j] == t)
				t->e[i]->adj_t[j] = NULL;
		/* disconnect adjacent triangles */
		for (j = 0; j < 3; j++) {
			triangle_t *adj_t = t->adj_t[i];
			if (adj_t != NULL && adj_t->adj_t[j] == t)
				adj_t->adj_t[j] = NULL;
		}
	}

	/* remove triangle */
	for(i = 0; i < vttriangle_len(&cdt->triangles); i++) {
		if (cdt->triangles.array[i] == t) {
			vttriangle_remove(&cdt->triangles, i, 1);
			break;
		}
	}
}

static void remove_edge(cdt_t *cdt, edge_t *e)
{
	int i;

	assert(e != NULL);

	/* disconnect adjacent triangles */
	for (i = 0; i < 2; i++)
		if (e->adj_t[i])
			remove_triangle(cdt, e->adj_t[i]);
	/* disconnect endpoints */
	for (i = 0; i < 2; i++)
		e->endp[i]->adj_edges = edgelist_remove_item(e->endp[i]->adj_edges, &e);

	/* remove edge */
	for(i = 0; i < vtedge_len(&cdt->edges); i++) {
		if (cdt->edges.array[i] == e) {
			vtedge_remove(&cdt->edges, i, 1);
			break;
		}
	}
}

static int is_point_in_circumcircle(point_t *p, triangle_t *t)
{
	double x1 = t->p[0]->pos.x, x2 = t->p[1]->pos.x, x3 = t->p[2]->pos.x;
	double y1 = t->p[0]->pos.y, y2 = t->p[1]->pos.y, y3 = t->p[2]->pos.y;
	double d1, d2, d3, d4;
	double n1, n2, n3;
	double px = p->pos.x, py = p->pos.y;

	n1 = (x1 * x1) + (y1 * y1);
	n2 = (x2 * x2) + (y2 * y2);
	n3 = (x3 * x3) + (y3 * y3);

	d1 = (x1 * y2) + (x2 * y3) + (x3 * y1) - (x3 * y2) - (x1 * y3) - (x2 * y1);
	d2 = (n1 * y2) + (n2 * y3) + (n3 * y1) - (n3 * y2) - (n1 * y3) - (n2 * y1);
	d3 = (n1 * x2) + (n2 * x3) + (n3 * x1) - (n3 * x2) - (n1 * x3) - (n2 * x1);
	d4 = (n1 * x2 * y3) + (n2 * x3 * y1) + (n3 * x1 * y2) - (n3 * x2 * y1) - (n1 * x3 * y2) - (n2 * x1 * y3);

	return d1 * ( (px * (px*d1 - d2)) + (py * (py*d1 + d3)) - d4 ) < 0 ? 1 : 0;
}

static void init_bbox(cdt_t *cdt, coord_t bbox_x1, coord_t bbox_y1, coord_t bbox_x2, coord_t bbox_y2)
{
	pos_t bbox;
	point_t *p_tl, *p_tr, *p_bl, *p_br;

	bbox.x = bbox_x1;
	bbox.y = bbox_y1;
	cdt->bbox_tl = bbox;
	p_bl = new_point(cdt, bbox);
	bbox.x = bbox_x2;
	bbox.y = bbox_y1;
	p_br = new_point(cdt, bbox);
	bbox.x = bbox_x1;
	bbox.y = bbox_y2;
	p_tl = new_point(cdt, bbox);
	bbox.x = bbox_x2;
	bbox.y = bbox_y2;
	p_tr = new_point(cdt, bbox);
	cdt->bbox_br = bbox;

	new_edge(cdt, p_tl, p_bl, 1);
	new_edge(cdt, p_bl, p_br, 1);
	new_edge(cdt, p_br, p_tr, 1);
	new_edge(cdt, p_tr, p_tl, 1);
	new_edge(cdt, p_bl, p_tr, 0);	/* diagonal */

	new_triangle(cdt, p_bl, p_br, p_tr);
	new_triangle(cdt, p_tl, p_bl, p_tr);
}

void cdt_init_(cdt_t *cdt)
{
	cdt->points.elem_constructor = vtpoint_constructor;
	cdt->points.elem_destructor = vtpoint_destructor;
	cdt->points.elem_copy = NULL;
	vtpoint_init(&cdt->points);
	cdt->edges.elem_constructor = vtedge_constructor;
	cdt->edges.elem_destructor = vtedge_destructor;
	cdt->edges.elem_copy = NULL;
	vtedge_init(&cdt->edges);
	cdt->triangles.elem_constructor = vttriangle_constructor;
	cdt->triangles.elem_destructor = vttriangle_destructor;
	cdt->triangles.elem_copy = NULL;
	vttriangle_init(&cdt->triangles);
}

void cdt_init(cdt_t *cdt, coord_t bbox_x1, coord_t bbox_y1, coord_t bbox_x2, coord_t bbox_y2)
{
	cdt_init_(cdt);
	init_bbox(cdt, bbox_x1, bbox_y1, bbox_x2, bbox_y2);
}


void cdt_free(cdt_t *cdt)
{
	vttriangle_uninit(&cdt->triangles);
	vtedge_uninit(&cdt->edges);
	VTPOINT_FOREACH(p, &cdt->points)
		trianglelist_free(p->adj_triangles);
		edgelist_free(p->adj_edges);
	VTPOINT_FOREACH_END();
	vtpoint_uninit(&cdt->points);
}

static int is_point_in_triangle(point_t *p, triangle_t *t)
{
	return ORIENT_CCW_CL(t->p[0], t->p[1], p) && ORIENT_CCW_CL(t->p[1], t->p[2], p) && ORIENT_CCW_CL(t->p[2], t->p[0], p);
}

typedef struct {
	edgelist_node_t *border_edges;
	edgelist_node_t *edges_to_remove;
} retriangulation_region_t;

static void check_adjacent_triangle(triangle_t *adj_t, edge_t *adj_e, point_t *new_p, retriangulation_region_t *region)
{
	int i;

	assert(adj_t != NULL && adj_e != NULL);

	if (is_point_in_circumcircle(new_p, adj_t)) {
		region->edges_to_remove = edgelist_prepend(region->edges_to_remove, &adj_e);
		region->border_edges = edgelist_remove_item(region->border_edges, &adj_e);
		for (i = 0; i < 3; i++) {
			if (adj_t->e[i] == adj_e)
				continue;
			else {
				edge_t *next_adj_e = adj_t->e[i];
				triangle_t *next_adj_t = adj_t->adj_t[i];

				region->border_edges = edgelist_prepend(region->border_edges, &next_adj_e);
				if (!next_adj_e->is_constrained)
					check_adjacent_triangle(next_adj_t, next_adj_e, new_p, region); /* recursive call */
			}
		}
	}
}

#ifdef DEBUG
void dump_pointlist(pointlist_node_t *list)
{
	POINTLIST_FOREACH(p, list)
		fprintf(stderr, "\t{pos=(%f, %f) adj_t=(", (double)p->pos.x, (double)p->pos.y);
		TRIANGLELIST_FOREACH(t, p->adj_triangles)
			fprintf(stderr, "%p", (void*)t);
			if(_node_->next != NULL)
				fprintf(stderr, ", ");
		TRIANGLELIST_FOREACH_END();
		fprintf(stderr, ") adj_e=(");
		EDGELIST_FOREACH(t, p->adj_edges)
			fprintf(stderr, "%p", (void*)t);
			if(_node_->next != NULL)
				fprintf(stderr, ", ");
		EDGELIST_FOREACH_END();
		fprintf(stderr, ")}\n");
	POINTLIST_FOREACH_END();
}

void dump_edgelist(edgelist_node_t *list)
{
	EDGELIST_FOREACH(e, list)
		fprintf(stderr, "\t{endp1=(%p (pos=(%f, %f))) endp2=(%p (pos=(%f, %f))) adj_t=(",
						(void*)e->endp[0], (double)e->endp[0]->pos.x, (double)e->endp[0]->pos.y, (void*)e->endp[1], (double)e->endp[1]->pos.x, (double)e->endp[1]->pos.y);
		fprintf(stderr, "%p, %p)", (void*)e->adj_t[0], (void*)e->adj_t[1]);
		fprintf(stderr, " %s}\n", e->is_constrained ? "constrained" : "");
	EDGELIST_FOREACH_END();
}

void dump_trianglelist(trianglelist_node_t *list)
{
	TRIANGLELIST_FOREACH(t, list)
		fprintf(stderr, "\t{p1=(%f, %f) p2=(%f, %f) p3=(%f, %f)",
						(double)t->p[0]->pos.x, (double)t->p[0]->pos.y, (double)t->p[1]->pos.x, (double)t->p[1]->pos.y, (double)t->p[2]->pos.x, (double)t->p[2]->pos.y);
		fprintf(stderr, " adj_e=(%p, %p, %p)", (void*)t->e[0], (void*)t->e[1], (void*)t->e[2]);
		fprintf(stderr, " adj_t=(%p, %p, %p)}\n", (void*)t->adj_t[0], (void*)t->adj_t[1], (void*)t->adj_t[2]);
	TRIANGLELIST_FOREACH_END();
}
#endif

static pointlist_node_t *order_edges_adjacently(edgelist_node_t *edges)
{
	pointlist_node_t *plist_ordered = NULL;
	edge_t *e1 = edges->item;
	point_t *p = e1->endp[0];
	int i = 1;

	plist_ordered = pointlist_prepend(plist_ordered, &p);
	edges = edgelist_remove_front(edges);

	while (edges != NULL) {
		p = e1->endp[i];
		EDGELIST_FOREACH(e2, edges)
			if (e2->endp[0] == p || e2->endp[1] == p) {
				plist_ordered = pointlist_prepend(plist_ordered, &p);
				edges = edgelist_remove(edges, _node_);
				i = e2->endp[0] == p ? 1 : 0;
				e1 = e2;
				break;
			}
		EDGELIST_FOREACH_END();
	}

	return plist_ordered;
}

static void triangulate_polygon(cdt_t *cdt, pointlist_node_t *polygon)
{
	pointlist_node_t *current_point_node;
	point_t *p[3];
	int i;

	assert(pointlist_length(polygon) >= 3);

	current_point_node = polygon;
	while (pointlist_length(polygon) > 3) {
		triangle_t candidate_t;
		edge_t candidate_e;
		pointlist_node_t *pnode;

		candidate_t.p[0] = current_point_node->item;
		candidate_e.endp[0] = candidate_t.p[0];
		if (current_point_node->next == NULL)
			current_point_node = polygon; /* wrap around */
		else
			current_point_node = current_point_node->next;
		candidate_t.p[1] = current_point_node->item;
		if (current_point_node->next == NULL)
			candidate_t.p[2] = polygon->item; /* wrap around */
		else
			candidate_t.p[2] = current_point_node->next->item;
		candidate_e.endp[1] = candidate_t.p[2];

		if (ORIENT_COLLINEAR(candidate_t.p[0], candidate_t.p[1], candidate_t.p[2]))
			goto skip;

		/* case 1: another point of the polygon violates the circle criterion */
		POINTLIST_FOREACH(p, polygon)
			if (p != candidate_t.p[0] && p != candidate_t.p[1] && p != candidate_t.p[2])
				if (is_point_in_circumcircle(p, &candidate_t))
					goto skip;
		POINTLIST_FOREACH_END();

		/* case 2: edge to be added already exists */
		EDGELIST_FOREACH(e1, candidate_e.endp[0]->adj_edges)
			EDGELIST_FOREACH(e2, candidate_e.endp[1]->adj_edges)
				if (e1 == e2)
					goto skip;
			EDGELIST_FOREACH_END();
		EDGELIST_FOREACH_END();

		/* case 3: edge to be added intersects an existing edge */
		/* case 4: a point adjacent to the current_point is in the candidate triangle */
		EDGELIST_FOREACH(e, candidate_t.p[1]->adj_edges)
			if (e != get_edge_from_points(candidate_t.p[0], candidate_t.p[1])
					&& e != get_edge_from_points(candidate_t.p[1], candidate_t.p[2])) {
				triangle_t ord_t;
				if (LINES_INTERSECT(candidate_e.endp[0], candidate_e.endp[1], e->endp[0], e->endp[1]))
					goto skip;
				ord_t.p[0] = candidate_t.p[0];
				ord_t.p[1] = candidate_t.p[1];
				ord_t.p[2] = candidate_t.p[2];
				order_triangle_points_ccw(&ord_t.p[0], &ord_t.p[1], &ord_t.p[2]);
				if (is_point_in_triangle(e->endp[0] != candidate_t.p[1] ? e->endp[0] : e->endp[1], &ord_t))
					goto skip;
			}
		EDGELIST_FOREACH_END();

		new_edge(cdt, candidate_e.endp[0], candidate_e.endp[1], 0);
		new_triangle(cdt, candidate_t.p[0], candidate_t.p[1], candidate_t.p[2]);

		/* update polygon: remove the point now covered by the new edge */
		if (current_point_node->next == NULL)
			pnode = polygon; /* wrap around */
		else
			pnode = current_point_node->next;
		polygon = pointlist_remove(polygon, current_point_node);
		current_point_node = pnode;

skip:
		continue;
	}

	/* create triangle from the remaining edges */
	for (i = 0; i < 3; i++) {
		p[i] = polygon->item;
		polygon = pointlist_remove_front(polygon);
	}
	new_triangle(cdt, p[0], p[1], p[2]);
}

static int insert_point_(cdt_t *cdt, point_t *new_p)
{
	triangle_t *enclosing_triangle = NULL;
	retriangulation_region_t region = {NULL, NULL};
	pointlist_node_t *points_to_attach, *prev_point_node;
	edge_t *crossing_edge = NULL;
	point_t *split_edge_endp[2] = {NULL, NULL};
	void *split_edge_user_data;
	int i, j;

	/* find enclosing triangle */
	VTTRIANGLE_FOREACH(t, &cdt->triangles)
		if (is_point_in_triangle(new_p, t)) {
			enclosing_triangle = t;
			break;
		}
	VTTRIANGLE_FOREACH_END();
	if (enclosing_triangle == NULL)
		return -1;

	/* check if the new point is on a constrained edge and unconstrain it
	 * (it will be removed, and then created as 2 segments connecting the new point)
	 */
	for (i = 0; i < 3; i++) {
		edge_t *e = enclosing_triangle->e[i];
		if (ORIENT_COLLINEAR(e->endp[0], e->endp[1], new_p)) {
			crossing_edge = e;
			if (crossing_edge->is_constrained) {
				for (j = 0; j < 2; j++)
					split_edge_endp[j] = enclosing_triangle->e[i]->endp[j];
				if (cdt->ev_split_constrained_edge_pre != NULL)
					cdt->ev_split_constrained_edge_pre(cdt, crossing_edge, new_p);
				split_edge_user_data = crossing_edge->data;
				crossing_edge->is_constrained = 0;
			}
			break;
		}
	}

	/* remove invalid edges and attach enclosing points */
	for (i = 0; i < 3; i++) {
		edge_t *adj_e = enclosing_triangle->e[i];
		triangle_t *adj_t = enclosing_triangle->adj_t[i];

		region.border_edges = edgelist_prepend(region.border_edges, &adj_e);
		if (!adj_e->is_constrained && adj_t != NULL)
			check_adjacent_triangle(adj_t, adj_e, new_p, &region);
	}
	remove_triangle(cdt, enclosing_triangle);
	EDGELIST_FOREACH(e, region.edges_to_remove)
		remove_edge(cdt, e);
	EDGELIST_FOREACH_END();

	points_to_attach = order_edges_adjacently(region.border_edges);
	prev_point_node = points_to_attach;
	new_edge(cdt, points_to_attach->item, new_p, 0);
	POINTLIST_FOREACH(p, points_to_attach->next)
		point_t *prev_p = prev_point_node->item;
		new_edge(cdt, p, new_p, 0);
		if (!ORIENT_COLLINEAR(prev_p, p, new_p))
			new_triangle(cdt, prev_p, p, new_p);
		prev_point_node = _node_;
	POINTLIST_FOREACH_END();
	if (!ORIENT_COLLINEAR(prev_point_node->item, points_to_attach->item, new_p))
		new_triangle(cdt, prev_point_node->item, points_to_attach->item, new_p);
	pointlist_free(points_to_attach);

	/* delete the crossing edge if it wasn't before (point on a boundary edge case) */
	if (crossing_edge != NULL && edgelist_find(region.edges_to_remove, &crossing_edge) == NULL)
		remove_edge(cdt, crossing_edge);
	edgelist_free(region.edges_to_remove);

	/* recreate, now splitted, constrained edge */
	if (split_edge_endp[0] != NULL) {
		edge_t *e1 = cdt_insert_constrained_edge(cdt, split_edge_endp[0], new_p);
		edge_t *e2 = cdt_insert_constrained_edge(cdt, new_p, split_edge_endp[1]);
		e1->data = e2->data = split_edge_user_data;
		if (cdt->ev_split_constrained_edge_post != NULL)
			cdt->ev_split_constrained_edge_post(cdt, e1, e2, new_p);
	}
	return 0;
}

point_t *cdt_insert_point(cdt_t *cdt, coord_t x, coord_t y)
{
	pos_t pos;
	point_t *new_p;

	pos.x = x;
	pos.y = y;
	VTPOINT_FOREACH(p, &cdt->points)
		if (p->pos.x == pos.x && p->pos.y == pos.y)	/* point in the same pos already exists */
			return p;
	VTPOINT_FOREACH_END();

	new_p = new_point(cdt, pos);
	if (insert_point_(cdt, new_p) != 0) {
		cdt->points.used--; /* new_point() allocated one slot at the end */
		return NULL;
	}

	return new_p;
}

void cdt_delete_point(cdt_t *cdt, point_t *p)
{
	edgelist_node_t *polygon_edges = NULL;
	pointlist_node_t *polygon_points;
	int i;

	/* find opposite edges of adjacent triangles and add them to the polygon */
	TRIANGLELIST_FOREACH(t, p->adj_triangles)
		i = 0;
		while(i < 3) {
			edge_t *edge_of_triangle = t->e[i];
			EDGELIST_FOREACH(edge_of_point, p->adj_edges)
				if (edge_of_point == edge_of_triangle)
					goto next_i;
			EDGELIST_FOREACH_END();
			polygon_edges = edgelist_prepend(polygon_edges, &edge_of_triangle);
			break;
next_i:
			i++;
		}
	TRIANGLELIST_FOREACH_END();

	/* remove adjacent edges */
	EDGELIST_FOREACH(e, p->adj_edges)
		assert(e->is_constrained == 0);
		_node_ = _node_->next;
		remove_edge(cdt, e);
		continue;
	EDGELIST_FOREACH_END();

	/* remove point */
	for(i = 0; i < vtpoint_len(&cdt->points); i++) {
		if (cdt->points.array[i] == p) {
			vtpoint_remove(&cdt->points, i, 1);
			break;
		}
	}

	polygon_points = order_edges_adjacently(polygon_edges);
	triangulate_polygon(cdt, polygon_points);
}

static trianglelist_node_t *triangles_intersecting_line(point_t *p1, point_t *p2)
{
	trianglelist_node_t *triangles = NULL;
	triangle_t *next_t = NULL;
	edge_t *adj_e;
	int i;

	/* find first triangle */
	TRIANGLELIST_FOREACH(t, p1->adj_triangles)
		for (i = 0; i < 3; i++)
			if (t->e[i]->endp[0] != p1 && t->e[i]->endp[1] != p1) /* opposite edge */
				if (LINES_INTERSECT(p1, p2, t->e[i]->endp[0], t->e[i]->endp[1])) {
					adj_e = t->e[i];
					next_t = adj_e->adj_t[0] != t ? adj_e->adj_t[0] : adj_e->adj_t[1];
					triangles = trianglelist_prepend(triangles, &t);
					goto first_triangle_found;
				}
	TRIANGLELIST_FOREACH_END();

first_triangle_found:
	assert(next_t != NULL);

	/* follow the path */
	while (next_t->p[0] != p2 && next_t->p[1] != p2 && next_t->p[2] != p2) {
		triangle_t *t = next_t;
		for (i = 0; i < 3; i++)
			if (t->e[i] != adj_e && LINES_INTERSECT(p1, p2, t->e[i]->endp[0], t->e[i]->endp[1])) {
				adj_e = t->e[i];
				next_t = adj_e->adj_t[0] != t ? adj_e->adj_t[0] : adj_e->adj_t[1];
				triangles = trianglelist_prepend(triangles, &t);
				break;
			}
	}

	triangles = trianglelist_prepend(triangles, &next_t);
	return triangles;
}

static void insert_constrained_edge_(cdt_t *cdt, point_t *p1, point_t *p2, pointlist_node_t **left_poly, pointlist_node_t **right_poly)
{
	edge_t *e;
	triangle_t *t;
	trianglelist_node_t *triangles;
	pointlist_node_t *left_polygon = NULL, *right_polygon = NULL;
	int i;

	/* find intersecting edges and remove them */
	triangles = triangles_intersecting_line(p1, p2);
	t = triangles->item;
	triangles = trianglelist_remove_front(triangles);
	left_polygon = pointlist_prepend(left_polygon, &p2);	/* triangle list begins from p2 */
	right_polygon = pointlist_prepend(right_polygon, &p2);
	for (i = 0; i < 3; i++)
		if (t->p[i] != p2) {
			if (ORIENT_CCW(p1, p2, t->p[i]))
				left_polygon = pointlist_prepend(left_polygon, &t->p[i]);
			else
				right_polygon = pointlist_prepend(right_polygon, &t->p[i]);
		}

	while (triangles->next != NULL) {
		t = triangles->item;
		triangles = trianglelist_remove_front(triangles);
		e = get_edge_from_points(left_polygon->item, right_polygon->item);
		for (i = 0; i < 3; i++)
			if (t->p[i] != left_polygon->item && t->p[i] != right_polygon->item) {
				if (ORIENT_CCW(p1, p2, t->p[i]))
					left_polygon = pointlist_prepend(left_polygon, &t->p[i]);
				else
					right_polygon = pointlist_prepend(right_polygon, &t->p[i]);
				break;
			}
		remove_edge(cdt, e);
	}
	triangles = trianglelist_remove_front(triangles);
	remove_edge(cdt, get_edge_from_points(left_polygon->item, right_polygon->item));
	left_polygon = pointlist_prepend(left_polygon, &p1);
	right_polygon = pointlist_prepend(right_polygon, &p1);
	*left_poly = left_polygon;
	*right_poly = right_polygon;
}

edge_t *cdt_insert_constrained_edge(cdt_t *cdt, point_t *p1, point_t *p2)
{
	edge_t *e;
	pointlist_node_t *left_polygon, *right_polygon;

	/* edge already exists - just constrain it */
	e = get_edge_from_points(p1, p2);
	if (e != NULL) {
		e->is_constrained = 1;
		return e;
	}

	insert_constrained_edge_(cdt, p1, p2, &left_polygon, &right_polygon);

	/* add new edge */
	e = new_edge(cdt, p1, p2, 1);

	/* triangulate the created polygons */
	triangulate_polygon(cdt, left_polygon);
	triangulate_polygon(cdt, right_polygon);

	return e;
}

void cdt_delete_constrained_edge(cdt_t *cdt, edge_t *edge)
{
	pointlist_node_t *polygon = NULL;
	edgelist_node_t *border_edges = NULL;
	edgelist_node_t *constrained_edges_within_scope = NULL;
	pointlist_node_t *isolated_points = NULL;
	int i, j;

	assert(edge->is_constrained);

	/* initial polygon */
	polygon = pointlist_prepend(polygon, &edge->endp[0]);
	polygon = pointlist_prepend(polygon, &edge->endp[1]);
	for (i = 0; i < 2; i++) {
		triangle_t *t = edge->adj_t[i];
		for(j = 0; j < 3; j++) {
			if (t->p[j] != edge->endp[0] && t->p[j] != edge->endp[1])
				polygon = pointlist_prepend(polygon, &edge->adj_t[i]->p[j]);
			if (t->e[j] != edge)
				border_edges = edgelist_prepend(border_edges, &t->e[j]);
		}
	}
	remove_edge(cdt, edge);

	/* find invalid edges and remove them */
	EDGELIST_FOREACH(e, border_edges)
		triangle_t *t = e->adj_t[0] != NULL ? e->adj_t[0] : e->adj_t[1];
		edgelist_node_t *e_node = _node_;
		int invalid_edge = 0;
		if (t != NULL) {
			POINTLIST_FOREACH(p, polygon)
				if (p != t->p[0] && p != t->p[1] && p != t->p[2] && is_point_in_circumcircle(p, t)) {
					for (i = 0; i < 3; i++) {
						if (t->p[i] != e->endp[0] && t->p[i] != e->endp[1])
							polygon = pointlist_prepend(polygon, &t->p[i]);
						if (t->e[i] != e)
							border_edges = edgelist_prepend(border_edges, &t->e[i]);
					}
					if (e->is_constrained) {	/* don't remove constrained edges - only detach them */
						constrained_edges_within_scope = edgelist_prepend(constrained_edges_within_scope, &e);
						for (i = 0; i < 2; i++)
							e->endp[i]->adj_edges = edgelist_remove_item(e->endp[i]->adj_edges, &e);
						remove_triangle(cdt, t);
					}
					else
						remove_edge(cdt, e);
					border_edges = edgelist_remove(border_edges, e_node);
					invalid_edge = 1;
					break;
				}
			POINTLIST_FOREACH_END();
		}
		else { /* triangle beyond a border edge was removed earlier - the edge should be removed too */
			point_t *endp[2];
			endp[0] = e->endp[0];
			endp[1] = e->endp[1];
			for (i = 0; i < 2; i++) /* check if the edge is not bbox of the triangulation */
				if ((e->endp[i]->pos.x == cdt->bbox_tl.x && e->endp[i]->pos.y == cdt->bbox_tl.y)
						|| (e->endp[i]->pos.x == cdt->bbox_br.x && e->endp[i]->pos.y == cdt->bbox_br.y))
					goto next_edge;
			border_edges = edgelist_remove(border_edges, e_node);
			border_edges = edgelist_remove_item(border_edges, &e);
			invalid_edge = 1; /* TODO: the loop doesn't have to start from the beginning, but border removal should preserve current node */
			if (e->is_constrained) {
				constrained_edges_within_scope = edgelist_prepend(constrained_edges_within_scope, &e);
				for (i = 0; i < 2; i++)
					endp[i]->adj_edges = edgelist_remove_item(endp[i]->adj_edges, &e); /* detach the edge */
			}
			else
				remove_edge(cdt, e);
			for (i = 0; i < 2; i++) {
				if (endp[i]->adj_edges == NULL) { /* isolated point */
					polygon = pointlist_remove_item(polygon, &endp[i]);
					isolated_points = pointlist_prepend(isolated_points, &endp[i]);
				}
			}
		}
		if (invalid_edge) {
			_node_ = border_edges;	/* start from the beginning (new point to check was added) */
			continue;
		}
next_edge:
	EDGELIST_FOREACH_END();
	pointlist_free(polygon);

	/* triangulate the resultant polygon */
	polygon = order_edges_adjacently(border_edges);
	triangulate_polygon(cdt, polygon);

	/* reattach isolated points and constrained edges */
	POINTLIST_FOREACH(p, isolated_points)
		insert_point_(cdt, p);
	POINTLIST_FOREACH_END();
	pointlist_free(isolated_points);
	EDGELIST_FOREACH(e, constrained_edges_within_scope)
		pointlist_node_t *left_polygon, *right_polygon;
		insert_constrained_edge_(cdt, e->endp[0], e->endp[1], &left_polygon, &right_polygon);
		for (i = 0; i < 2; i++)
			e->endp[i]->adj_edges = edgelist_prepend(e->endp[i]->adj_edges, &e); /* reattach the edge to the endpoints */
		triangulate_polygon(cdt, left_polygon);
		triangulate_polygon(cdt, right_polygon);
	EDGELIST_FOREACH_END();
	edgelist_free(constrained_edges_within_scope);
}

static void circumcircle(const triangle_t *t, pos_t *p, int *r)
{
	double x1 = t->p[0]->pos.x, x2 = t->p[1]->pos.x, x3 = t->p[2]->pos.x;
	double y1 = t->p[0]->pos.y, y2 = t->p[1]->pos.y, y3 = t->p[2]->pos.y;
	double d1, d2, d3, d4;
	double n1, n2, n3;

	n1 = (x1 * x1) + (y1 * y1);
	n2 = (x2 * x2) + (y2 * y2);
	n3 = (x3 * x3) + (y3 * y3);

	d1 = (x1 * y2) + (x2 * y3) + (x3 * y1) - (x3 * y2) - (x1 * y3) - (x2 * y1);
	d2 = (n1 * y2) + (n2 * y3) + (n3 * y1) - (n3 * y2) - (n1 * y3) - (n2 * y1);
	d3 = (n1 * x2) + (n2 * x3) + (n3 * x1) - (n3 * x2) - (n1 * x3) - (n2 * x1);
	d4 = (n1 * x2 * y3) + (n2 * x3 * y1) + (n3 * x1 * y2) - (n3 * x2 * y1) - (n1 * x3 * y2) - (n2 * x1 * y3);

	p->x = d2/(2*d1);
	p->y = -d3/(2*d1);
	*r = sqrt((d2*d2)/(d1*d1) + (d3*d3)/(d1*d1) + 4*d4/d1)/2;
}

void cdt_fdump_animator(FILE *f, cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations)
{
	int last_c = 0;
	int triangle_num = 0;
	fprintf(f, "frame\n");
	fprintf(f, "scale 0.9\n");
	fprintf(f, "viewport %f %f - %f %f\n", (double)cdt->bbox_tl.x - 1.0, (double)cdt->bbox_tl.y - 1.0, (double)cdt->bbox_br.x + 1.0, (double)cdt->bbox_br.y  + 1.0);

	VTEDGE_FOREACH(edge, &cdt->edges)
		if(last_c != edge->is_constrained) {
			fprintf(f, "color %s\n", edge->is_constrained ? "red" : "black");
			fprintf(f, "thick %s\n", edge->is_constrained ? "2" : "1");
			last_c = edge->is_constrained;
		}
		fprintf(f, "line %f %f %f %f\n", (double)edge->endp[0]->pos.x, (double)edge->endp[0]->pos.y, (double)edge->endp[1]->pos.x, (double)edge->endp[1]->pos.y);
	VTEDGE_FOREACH_END();

	fprintf(f, "color green\n");
	VTTRIANGLE_FOREACH(triangle, &cdt->triangles)
		pos_t pos;
		int r;
		if (show_circles) {
			circumcircle(triangle, &pos, &r);
			fprintf(f, "circle %f %f %f 50\n", (double)pos.x, (double)pos.y, (double)r);
		}
		triangle_num++;
	VTTRIANGLE_FOREACH_END();

	fprintf(f, "color red\n");
	if (point_violations) {
		POINTLIST_FOREACH(p, point_violations)
			fprintf(f, "circle %f %f 50 10\n", (double)p->pos.x, (double)p->pos.y);
		POINTLIST_FOREACH_END();
	}

	fprintf(f, "color darkred\n");
	if (triangle_violations) {
		TRIANGLELIST_FOREACH(t, triangle_violations)
			pos_t pos;
			int r;
			circumcircle(t, &pos, &r);
			fprintf(f, "circle %f %f %f 50\n", (double)pos.x, (double)pos.y, (double)r);
		TRIANGLELIST_FOREACH_END();
	}

	fprintf(f, "flush\n");
	fprintf(f, "! triangle num: %d\n", triangle_num);
}

void cdt_dump_animator(cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations)
{
	cdt_fdump_animator(stdout, cdt, show_circles, point_violations, triangle_violations);
}

static long point_id(cdt_t *cdt, point_t *p)
{
	long pid;
	for(pid = 0; pid < cdt->points.used; pid++)
		if (p == cdt->points.array[pid])
			return pid;
	return -1;
}


void cdt_fdump(FILE *f, cdt_t *cdt)
{
	long n;

	fprintf(f, "raw_init\n");

	for(n = 0; n < cdt->points.used; n++) {
		point_t *p = cdt->points.array[n];
		fprintf(f, "raw_point %.16f %.16f\n", (double)p->pos.x, (double)p->pos.y);
	}

	for(n = 0; n < cdt->edges.used; n++) {
		edge_t *e = cdt->edges.array[n];
		fprintf(f, "raw_edge P%ld P%ld %d\n", point_id(cdt, e->endp[0]), point_id(cdt, e->endp[1]), e->is_constrained);
	}

	for(n = 0; n < cdt->triangles.used; n++) {
		triangle_t *t = cdt->triangles.array[n];
		fprintf(f, "raw_triangle P%ld P%ld P%ld\n", point_id(cdt, t->p[0]), point_id(cdt, t->p[1]), point_id(cdt, t->p[2]));
	}
}


int cdt_check_delaunay(cdt_t *cdt, pointlist_node_t **point_violations, trianglelist_node_t **triangle_violations)
{
	int delaunay = 1;
	VTTRIANGLE_FOREACH(triangle, &cdt->triangles)
		VTPOINT_FOREACH(point, &cdt->points)
			if (is_point_in_circumcircle(point, triangle)
					&& point != triangle->p[0]
					&& point != triangle->p[1]
					&& point != triangle->p[2]) {
				delaunay = 0;
				if (point_violations != NULL)
					*point_violations = pointlist_prepend(*point_violations, &point);
				if (triangle_violations != NULL)
					*triangle_violations = trianglelist_prepend(*triangle_violations, &triangle);
			}
		VTPOINT_FOREACH_END();
	VTTRIANGLE_FOREACH_END();
	return delaunay;
}
