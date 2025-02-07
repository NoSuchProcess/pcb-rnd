/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Fill up a polyarea with more or less evenly spaced points */


#include <math.h>
#include <genlist/gendlist.h>
#include <librnd/core/vtc0.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/polyarea.h>
#include <src_plugins/lib_polyhelp/triangulate.h>

typedef struct pcb_ptcloud_pt_s {
	rnd_coord_t x, y;     /* current coord */
	rnd_coord_t dx, dy;   /* temporary: push vector in this round */
	int weight;           /* 1 or more; the higher the value the more this point pushes others */
	unsigned fixed:1;     /* do not move */
	gdl_elem_t link;      /* in a grid cell */
	gdl_elem_t all;       /* in a ->points */
} pcb_ptcloud_pt_t;

typedef struct pcb_ptcloud_ctx_s {
	/*** config part, filled in by the caller before pcb_ptcloud() ***/
	rnd_pline_t *pl;
	rnd_coord_t target_dist;  /* target distance between points */

	/*** internal state/cache ***/
	/* grid; grid cells are GRID_SIZE * target_dist in x and y directions */
	long gsx, gsy;            /* grid size in number of elements in each dir */
	long glen;                /* cache: gsx*gsy */
	gdl_list_t *grid;         /* each element of this array is a grid cell with a list of pcb_ptcloud_pt_t points in it */

	rnd_coord_t closed;       /* distance for 2 points being close enough for computing gravity */
	double closed2;           /* square of ->closed */
	double target2;           /* square of ->target_dist */
	gdl_list_t points;        /* flat list of all points for easy iteration */

	/* triangulation */
	fp2t_t tri;
	void *tri_mem;
	long num_pt_edge;

	/* horizontal rays */
	rnd_coord_t ray_y;
	vtc0_t edges;             /* x coords of edges crossed */
} pcb_ptcloud_ctx_t;

void pcb_ptcloud(pcb_ptcloud_ctx_t *ctx);
void pcb_ptcloud_free(pcb_ptcloud_ctx_t *ctx);

