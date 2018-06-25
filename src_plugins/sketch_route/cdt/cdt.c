/*
 *  Copyright (C) 2018 Wojciech Krutnik
 *
 *  Constrained Delaunay Triangulation using incremental algorithm, as described in:
 *    Yizhi Lu and Wayne Wei-Ming Dai, "A numerical stable algorithm for constructing
 *    constrained Delaunay triangulation and application to multichip module layout,"
 *    China., 1991 International Conference on Circuits and Systems, Shenzhen, 1991,
 *    pp. 644-647 vol.2.
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "cdt.h"

#define DEBUG

#define LEFTPOINT(p1, p2) ((p1)->pos.x < (p2)->pos.x || ((p1)->pos.x == (p2)->pos.x && (p1)->pos.y > (p2)->pos.y))

#define ORIENT_CCW(a, b, c) (orientation(a, b, c) < 0)
#define ORIENT_CW(a, b, c) (orientation(a, b, c) > 0)
/* TODO: check epsilon for collinear case? */
#define ORIENT_COLLINEAR(a, b, c) (orientation(a, b, c) == 0)
#define ORIENT_CCW_CL(a, b, c) (orientation(a, b, c) <= 0)
static double orientation(point_t *p1, point_t *p2, point_t *p3)
{
	return ((double)p2->pos.y - (double)p1->pos.y) * ((double)p3->pos.x - (double)p2->pos.x)
				 - ((double)p2->pos.x - (double)p1->pos.x) * ((double)p3->pos.y - (double)p2->pos.y);
}

#define LINES_INTERSECT(p1, q1, p2, q2) \
	(ORIENT_CCW(p1, q1, p2) != ORIENT_CCW(p1, q1, q2) && ORIENT_CCW(p2, q2, p1) != ORIENT_CCW(p2, q2, q1))


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

static edge_t *get_edge_from_points(point_t *p1, point_t *p2)
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

void cdt_init(cdt_t *cdt, coord_t bbox_x1, coord_t bbox_y1, coord_t bbox_x2, coord_t bbox_y2)
{
	vtpoint_init(&cdt->points);
	cdt->points.elem_constructor = vtpoint_constructor;
	cdt->points.elem_destructor = vtpoint_destructor;
	vtedge_init(&cdt->edges);
	cdt->edges.elem_constructor = vtedge_constructor;
	cdt->edges.elem_destructor = vtedge_destructor;
	vttriangle_init(&cdt->triangles);
	cdt->triangles.elem_constructor = vttriangle_constructor;
	cdt->triangles.elem_destructor = vttriangle_destructor;

	init_bbox(cdt, bbox_x1, bbox_y1, bbox_x2, bbox_y2);
}

static int is_point_in_triangle(point_t *p, triangle_t *t)
{
	return ORIENT_CCW_CL(t->p[0], t->p[1], p) && ORIENT_CCW_CL(t->p[1], t->p[2], p) && ORIENT_CCW_CL(t->p[2], t->p[0], p);
}

typedef struct {
	edgelist_node_t *border_edges;
	edgelist_node_t *edges_to_remove;
} point_insert_region_t;

static void check_adjacent_triangle(triangle_t *adj_t, edge_t *adj_e, point_t *new_p, point_insert_region_t *region)
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
		fprintf(stderr, "\t{pos=(%d, %d) adj_t=(", p->pos.x, p->pos.y);
		TRIANGLELIST_FOREACH(t, p->adj_triangles)
			fprintf(stderr, "%p", t);
			if(_node_->next != NULL)
				fprintf(stderr, ", ");
		TRIANGLELIST_FOREACH_END();
		fprintf(stderr, ") adj_e=(");
		EDGELIST_FOREACH(t, p->adj_edges)
			fprintf(stderr, "%p", t);
			if(_node_->next != NULL)
				fprintf(stderr, ", ");
		EDGELIST_FOREACH_END();
		fprintf(stderr, ")}\n");
	POINTLIST_FOREACH_END();
}

void dump_edgelist(edgelist_node_t *list)
{
	EDGELIST_FOREACH(e, list)
		fprintf(stderr, "\t{endp1=(%p (pos=(%d, %d))) endp2=(%p (pos=(%d, %d))) adj_t=(",
						e->endp[0], e->endp[0]->pos.x, e->endp[0]->pos.y, e->endp[1], e->endp[1]->pos.x, e->endp[1]->pos.y);
		fprintf(stderr, "%p, %p)", e->adj_t[0], e->adj_t[1]);
		fprintf(stderr, " %s}\n", e->is_constrained ? "constrained" : "");
	EDGELIST_FOREACH_END();
}

void dump_trianglelist(trianglelist_node_t *list)
{
	TRIANGLELIST_FOREACH(t, list)
		fprintf(stderr, "\t{p1=(%d, %d) p2=(%d, %d) p3=(%d, %d)",
						t->p[0]->pos.x, t->p[0]->pos.y, t->p[1]->pos.x, t->p[1]->pos.y, t->p[2]->pos.x, t->p[2]->pos.y);
		fprintf(stderr, " adj_e=(%p, %p, %p)", t->e[0], t->e[1], t->e[2]);
		fprintf(stderr, " adj_t=(%p, %p, %p)}\n", t->adj_t[0], t->adj_t[1], t->adj_t[2]);
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
				triangle_t ord_t = { {candidate_t.p[0], candidate_t.p[1], candidate_t.p[2]} };
				if (LINES_INTERSECT(candidate_e.endp[0], candidate_e.endp[1], e->endp[0], e->endp[1]))
					goto skip;
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

point_t *cdt_insert_point(cdt_t *cdt, coord_t x, coord_t y)
{
	pos_t pos = {x, y};
	point_t *new_p;
	triangle_t *enclosing_triangle = NULL;
	point_insert_region_t region = {NULL, NULL};
	pointlist_node_t *points_to_attach, *prev_point_node;
	int i;

	VTPOINT_FOREACH(p, &cdt->points)
		if (p->pos.x == pos.x && p->pos.y == pos.y)	/* point in the same pos already exists */
			return p;
	VTPOINT_FOREACH_END();

	new_p = new_point(cdt, pos);

	/* find enclosing triangle */
	VTTRIANGLE_FOREACH(t, &cdt->triangles)
		if (is_point_in_triangle(new_p, t)) {
			enclosing_triangle = t;
			break;
		}
	VTTRIANGLE_FOREACH_END();
	assert(enclosing_triangle != NULL);

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
		new_triangle(cdt, prev_p, p, new_p);
		prev_point_node = _node_;
	POINTLIST_FOREACH_END();
	new_triangle(cdt, prev_point_node->item, points_to_attach->item, new_p);

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
		remove_edge(cdt, e);
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

edge_t *cdt_insert_constrained_edge(cdt_t *cdt, point_t *p1, point_t *p2)
{
	edge_t *e;
	triangle_t *t;
	trianglelist_node_t *triangles;
	pointlist_node_t *left_polygon = NULL, *right_polygon = NULL;
	int i;

	/* edge already exists - just constrain it */
	e = get_edge_from_points(p1, p2);
	if (e != NULL) {
		e->is_constrained = 1;
		return e;
	}

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
	edgelist_node_t *border_edges
	int i, j;

	assert(edge->is_constrained);

	/* initial polygon */
	polygon = pointlist_prepend(polygon, &edge->endp[0]);
	polygon = pointlist_prepend(polygon, &edge->endp[1]);
	for (i = 0; i < 2; i++)
		for(j = 0; j < 3; j++)
			if (edge->adj_t[i]->p[j] != edge->endp[0] && edge->adj_t[i]->p[j] != edge->endp[1]) {
				polygon = pointlist_prepend(polygon, &edge->adj_t[i]->p[j]);
				break;
			}

	/* find invalid edges */

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

void cdt_dump_animator(cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations)
{
	int last_c = 0;
	int triangle_num = 0;
	printf("frame\n");
	printf("scale 0.9\n");
	printf("viewport %f %f - %f %f\n", (double)cdt->bbox_tl.x - 1.0, (double)cdt->bbox_tl.y - 1.0, (double)cdt->bbox_br.x + 1.0, (double)cdt->bbox_br.y  + 1.0);

	VTEDGE_FOREACH(edge, &cdt->edges)
		if(last_c != edge->is_constrained) {
			printf("color %s\n", edge->is_constrained ? "red" : "black");
			printf("thick %s\n", edge->is_constrained ? "2" : "1");
			last_c = edge->is_constrained;
		}
		printf("line %d %d %d %d\n", edge->endp[0]->pos.x, edge->endp[0]->pos.y, edge->endp[1]->pos.x, edge->endp[1]->pos.y);
	VTEDGE_FOREACH_END();

	printf("color green\n");
	VTTRIANGLE_FOREACH(triangle, &cdt->triangles)
		pos_t pos;
		int r;
		if (show_circles) {
			circumcircle(triangle, &pos, &r);
			printf("circle %d %d %d 50\n", pos.x, pos.y, r);
		}
		triangle_num++;
	VTTRIANGLE_FOREACH_END();

	printf("color red\n");
	if (point_violations) {
		POINTLIST_FOREACH(p, point_violations)
			printf("circle %d %d 50 10\n", p->pos.x, p->pos.y);
		POINTLIST_FOREACH_END();
	}

	printf("color darkred\n");
	if (triangle_violations) {
		TRIANGLELIST_FOREACH(t, triangle_violations)
			pos_t pos;
			int r;
			circumcircle(t, &pos, &r);
			printf("circle %d %d %d 50\n", pos.x, pos.y, r);
		TRIANGLELIST_FOREACH_END();
	}

	printf("flush\n");
	fprintf(stderr, "triangle num: %d\n", triangle_num);
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
