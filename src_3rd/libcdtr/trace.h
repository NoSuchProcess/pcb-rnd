/*
    libcdtr - Constrained Delaunay Triangulation using incremental algorithm

    Copyright (C) 2018 Wojciech Krutnik
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
 */

#define cdt_init(cdt, x1, y1, x2, y2) \
do { \
	int n; \
	fprintf(stderr, "cdt:%p init %.16f %.16f %.16f %.16f\n", cdt, x1, y1, x2, y2); \
	cdt_init(cdt, x1, y1, x2, y2); \
	for(n = 0; n < cdt->points.used; n++) { \
		point_t *p = cdt->points.array[n]; \
		fprintf(stderr, "cdt:%p init_point %.16f %.16f %p\n", cdt, p->pos.x, p->pos.y, p); \
	} \
} while(0)

#define cdt_free(cdt) \
do { \
	fprintf(stderr, "cdt:%p free\n", cdt); \
	cdt_free(cdt); \
} while(0)

static point_t *cdtrace_insert_point(cdt_t *cdt, coord_t x, coord_t y)
{
	point_t *p;
	fprintf(stderr, "cdt:%p ins_point %.16f %.16f ", cdt, x, y);
	fflush(stderr);
	p = cdt_insert_point(cdt, x, y);
	fprintf(stderr, "%p\n", p);
	return p;
}
#define cdt_insert_point(cdt, x, y) cdtrace_insert_point(cdt, x, y)

static edge_t *cdtrace_insert_constrained_edge(cdt_t *cdt, point_t *p1, point_t *p2)
{
	edge_t *e;
	fprintf(stderr, "cdt:%p ins_cedge %p %p ", cdt, p1, p2);
	e = cdt_insert_constrained_edge(cdt, p1, p2);
	fflush(stderr);
	fprintf(stderr, " %p\n", e);
	return e;
}
#define cdt_insert_constrained_edge(cdt, p1, p2) cdtrace_insert_constrained_edge(cdt, p1, p2)


