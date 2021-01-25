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

#ifndef CDT_DEBUG_H
#define CDT_DEBUG_H

#include <libcdtr/cdt.h>

int cdt_check_delaunay(cdt_t *cdt, pointlist_node_t **point_violations, trianglelist_node_t **triangle_violations);
void cdt_dump_animator(cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations);
void cdt_fdump_animator(FILE *f, cdt_t *cdt, int show_circles, pointlist_node_t *point_violations, trianglelist_node_t *triangle_violations);
void cdt_fdump_adj_triangles_anim(FILE *f, cdt_t *cdt, triangle_t *t);

/* Dump a cdt script in the format that's suitable for the tester to include */
void cdt_fdump(FILE *f, cdt_t *cdt);


#endif
