/*
 *  Copyright (C) 2018 Wojciech Krutnik
 *
 *  Constrained Delaunay Triangulation using incremental algorithm, as described in:
 *    Yizhi Lu and Wayne Wei-Ming Dai, "A numerical stable algorithm for constructing
 *    constrained Delaunay triangulation and application to multichip module layout,"
 *    China., 1991 International Conference on Circuits and Systems, Shenzhen, 1991,
 *    pp. 644-647 vol.2.
 */

#ifndef CDT_H
#define CDT_H

#include "typedefs.h"
#include "point.h"
#include "edge.h"
#include "triangle.h"


typedef struct {
	vtpoint_t points;
	vtedge_t edges;
	vttriangle_t triangles;

	pos_t bbox_tl;
	pos_t bbox_br;
} cdt_t;


void cdt_init(cdt_t *cdt, coord_t bbox_x1, coord_t bbox_y1, coord_t bbox_x2, coord_t bbox_y2);

point_t *cdt_insert_point(cdt_t *cdt, coord_t x, coord_t y);
void cdt_delete_point(cdt_t *cdt, point_t *p);	/* any edge adjecent to this point cannot be constrained */
edge_t *cdt_insert_constrained_edge(cdt_t *cdt, point_t *p1, point_t *p2);
void cdt_delete_constrained_edge(cdt_t *cdt, edge_t *edge);

int cdt_check_delaunay(cdt_t *cdt, pointlist_node_t **point_violations, trianglelist_node_t **triangle_violations);
void cdt_dump_animator(cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations);

#endif
