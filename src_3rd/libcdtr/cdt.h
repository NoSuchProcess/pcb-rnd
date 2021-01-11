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

#ifndef CDT_H
#define CDT_H

#include "typedefs.h"
#include "point.h"
#include "edge.h"
#include "triangle.h"

typedef struct cdt_s cdt_t;

struct cdt_s {
	vtpoint_t points;
	vtedge_t edges;
	vttriangle_t triangles;

	/* optional event callbacks (may be NULL to ignore) */
	void (*ev_split_constrained_edge_pre)(cdt_t *cdt, edge_t *cedge, point_t *pt); /* called before removing a constrained edge that gets split at pt */
	void (*ev_split_constrained_edge_post)(cdt_t *cdt, edge_t *ne1, edge_t *ne2, point_t *pt); /* called after creating the two new constrained edges around pt */

	pos_t bbox_tl;
	pos_t bbox_br;
};


void cdt_init(cdt_t *cdt, coord_t bbox_x1, coord_t bbox_y1, coord_t bbox_x2, coord_t bbox_y2);
void cdt_free(cdt_t *cdt);

point_t *cdt_insert_point(cdt_t *cdt, coord_t x, coord_t y);
void cdt_delete_point(cdt_t *cdt, point_t *p);	/* any edge adjecent to this point cannot be constrained */
edge_t *cdt_insert_constrained_edge(cdt_t *cdt, point_t *p1, point_t *p2);
void cdt_delete_constrained_edge(cdt_t *cdt, edge_t *edge);

int cdt_check_delaunay(cdt_t *cdt, pointlist_node_t **point_violations, trianglelist_node_t **triangle_violations);
void cdt_dump_animator(cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations);
void cdt_fdump_animator(FILE *f, cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations);

/* Dump a cdt script in the format that's suitable for the tester to include */
void cdt_fdump(FILE *f, cdt_t *cdt);


edge_t *get_edge_from_points(point_t *p1, point_t *p2);

#define ORIENT_CCW(a, b, c) (orientation(a, b, c) < 0)
#define ORIENT_CW(a, b, c) (orientation(a, b, c) > 0)
#define ORIENT(dir, a, b, c) ((dir) ? ORIENT_CW(a, b, c) : ORIENT_CCW(a, b, c))
/* TODO: check epsilon for collinear case? */
#define ORIENT_COLLINEAR(a, b, c) (orientation(a, b, c) == 0)
#define ORIENT_CCW_CL(a, b, c) (orientation(a, b, c) <= 0)
static inline double orientation(point_t *p1, point_t *p2, point_t *p3)
{
	return ((double)p2->pos.y - (double)p1->pos.y) * ((double)p3->pos.x - (double)p2->pos.x)
				 - ((double)p2->pos.x - (double)p1->pos.x) * ((double)p3->pos.y - (double)p2->pos.y);
}

#define LINES_INTERSECT(p1, q1, p2, q2) \
	(ORIENT_CCW(p1, q1, p2) != ORIENT_CCW(p1, q1, q2) && ORIENT_CCW(p2, q2, p1) != ORIENT_CCW(p2, q2, q1))

#define EDGE_OTHER_POINT(edge, point) \
	((edge)->endp[0] != (point) ? (edge)->endp[0] : (edge)->endp[1])

#define DIST2(p, q) \
	(((double)((p)->pos.x - (q)->pos.x) * (double)((p)->pos.x - (q)->pos.x)) \
	 + ((double)((p)->pos.y - (q)->pos.y) * (double)((p)->pos.y - (q)->pos.y)))


/* Raw, low level object creation without side effects; use it only to
   reconstruct a case */
void cdt_init_(cdt_t *cdt); /* does not create the bbox in constrained edges */
edge_t *cdt_new_edge_(cdt_t *cdt, point_t *p1, point_t *p2, int constrain);
triangle_t *cdt_new_triangle_(cdt_t *cdt, point_t *p1, point_t *p2, point_t *p3);


#endif
