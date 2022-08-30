/*
    libcdtr - Constrained Delaunay Triangulation using incremental algorithm

    Copyright (C) 2018, 2021 Wojciech Krutnik
    Copyright (C) 2021 Tibor 'Igor2' Palinkas

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

static void circumcircle(const triangle_t *t, pos_t *p, coord_t *r)
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

static long triangle_id(cdt_t *cdt, triangle_t *t)
{
	long tid;
	for(tid = 0; tid < cdt->triangles.used; tid++)
		if (t == cdt->triangles.array[tid])
			return tid;
	return -1;
}

static pos_t triangle_center_pos(triangle_t *t)
{
	pos_t c = {0, 0};
	int i;
	for (i = 0; i < 3; i++) {
		c.x += t->p[i]->pos.x;
		c.y += t->p[i]->pos.y;
	}
	c.x *= (1.0/3.0);
	c.y *= (1.0/3.0);
	return c;
}

void cdt_fdump_animator(FILE *f, cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations)
{
	int last_c = 0;
	int triangle_num = 0;
	double pt_r, dx = cdt->bbox_br.x - cdt->bbox_tl.x, dy = cdt->bbox_br.y - cdt->bbox_tl.y;

	/* calculate a point violation circle radius that scales with drawing size */
	pt_r = dx > dy ? dx : dy;
	pt_r = pt_r / 500.0;

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
		coord_t r;
		if (show_circles) {
			circumcircle(triangle, &pos, &r);
			fprintf(f, "circle %f %f %f 50\n", (double)pos.x, (double)pos.y, (double)r);
		}
	VTTRIANGLE_FOREACH_END();

	fprintf(f, "color black\n");
	VTTRIANGLE_FOREACH(triangle, &cdt->triangles)
		pos_t c = triangle_center_pos(triangle);
		fprintf(f, "text %f %f \"%ld\"\n", (double)c.x, (double)c.y, triangle_id(cdt, triangle));
		triangle_num++;
	VTTRIANGLE_FOREACH_END();

	fprintf(f, "color red\n");
	if (point_violations) {
		POINTLIST_FOREACH(p, point_violations)
			fprintf(f, "circle %f %f %f 10\n", (double)p->pos.x, (double)p->pos.y, pt_r);
		POINTLIST_FOREACH_END();
	}

	fprintf(f, "color darkred\n");
	if (triangle_violations) {
		TRIANGLELIST_FOREACH(t, triangle_violations)
			pos_t pos;
			coord_t r;
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

static void fdump_triangle_anim(FILE *f, cdt_t *cdt, triangle_t *t, int first)
{
	static int last_c;
	int i;
	pos_t c;

	c = triangle_center_pos(t);
	fprintf(f, "text %f %f \"%ld\"\n", (double)c.x, (double)c.y, triangle_id(cdt, t));
	for (i = 0; i < 3; i++) {
		edge_t *edge = t->e[i];
		if(last_c != edge->is_constrained) {
			fprintf(f, "color %s\n", edge->is_constrained ? "red" : "black");
			fprintf(f, "thick %s\n", edge->is_constrained ? "2" : "1");
			last_c = edge->is_constrained;
		}
		fprintf(f, "line %f %f %f %f\n", (double)edge->endp[0]->pos.x, (double)edge->endp[0]->pos.y, (double)edge->endp[1]->pos.x, (double)edge->endp[1]->pos.y);
	}
}

void cdt_fdump_adj_triangles_anim(FILE *f, cdt_t *cdt, triangle_t *t)
{
	int i;
	fprintf(f, "frame\n");
	fprintf(f, "scale 0.9\n");
	fprintf(f, "viewport %f %f - %f %f\n", (double)cdt->bbox_tl.x - 1.0, (double)cdt->bbox_tl.y - 1.0, (double)cdt->bbox_br.x + 1.0, (double)cdt->bbox_br.y  + 1.0);
	fprintf(f, "title \"%ld\"\n", triangle_id(cdt, t));

	fdump_triangle_anim(f, cdt, t, 1);
	for (i = 0; i < 3; i++) {
		if (t->adj_t[i] != NULL)
			fdump_triangle_anim(f, cdt, t->adj_t[i], 0);
	}

	fprintf(f, "flush\n");
}

int cdt_check_delaunay(cdt_t *cdt, pointlist_node_t **point_violations, trianglelist_node_t **triangle_violations)
{
	int delaunay = 1;
	VTTRIANGLE_FOREACH(triangle, &cdt->triangles)
		VTPOINT_FOREACH(point, &cdt->points)
			if (cdt_is_point_in_circumcircle(point, triangle)
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
