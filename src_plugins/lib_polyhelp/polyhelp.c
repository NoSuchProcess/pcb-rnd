/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <math.h>

#include "polyhelp.h"
#include "polygon.h"
#include "plugins.h"
#include "pcb-printf.h"
#include "obj_line.h"
#include "box.h"


void pcb_pline_fprint_anim(FILE *f, const pcb_pline_t *pl)
{
	const pcb_vnode_t *v, *n;
	fprintf(f, "!pline start\n");
	v = &pl->head;
	do {
		n = v->next;
		pcb_fprintf(f, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while((v = v->next) != &pl->head);
	fprintf(f, "!pline end\n");
}

static void cross(FILE *f, pcb_coord_t x, pcb_coord_t y)
{
	static pcb_coord_t cs = PCB_MM_TO_COORD(0.2);
	pcb_fprintf(f, "line %#mm %#mm %#mm %#mm\n", x - cs, y, x + cs, y);
	pcb_fprintf(f, "line %#mm %#mm %#mm %#mm\n", x, y - cs, x, y + cs);
}

static void norm(double *nx, double *ny, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double dx = x2 - x1, dy = y2 - y1, len = sqrt(dx*dx + dy*dy);
	*nx = -dy / len;
	*ny = dx / len;
}

static void ll_intersect(double *xi, double *yi, double ax1, double ay1, double ax2, double ay2, double bx1, double by1, double bx2, double by2)
{
	double ua, X1, Y1, X2, Y2, X3, Y3, X4, Y4, tmp;

	/* maths from http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/ */
	X1 = ax1;
	X2 = ax2;
	X3 = bx1;
	X4 = bx2;
	Y1 = ay1;
	Y2 = ay2;
	Y3 = by1;
	Y4 = by2;

	tmp = ((Y4 - Y3) * (X2 - X1) - (X4 - X3) * (Y2 - Y1));

	if (tmp == 0) {
		/* Corner case: parallel lines; intersect only if the endpoint of either line
		   is on the other line */
		*xi = ax2;
		*yi = ay2;
	}


	ua = ((X4 - X3) * (Y1 - Y3) - (Y4 - Y3) * (X1 - X3)) / tmp;
/*	ub = ((X2 - X1) * (Y1 - Y3) - (Y2 - Y1) * (X1 - X3)) / tmp;*/
	*xi = X1 + ua * (X2 - X1);
	*yi = Y1 + ua * (Y2 - Y1);
}


pcb_pline_t *pcb_pline_dup_offset(const pcb_pline_t *src, pcb_coord_t offs)
{
	const pcb_vnode_t *p = NULL, *v, *n;
	pcb_vector_t tmp;
	pcb_pline_t *res = NULL;
	double nx, ny, px, py;
	int num_pts, i;

fprintf(stdout, "!offs start\n");

	fprintf(stdout, "color blue\n");
	pcb_pline_fprint_anim(stdout, src);

	v = &src->head;
	num_pts = 0;
	do {
		num_pts++;
	} while((v = v->next) != &src->head);


	v = &src->head;
	i = 0;
	do {
		n = v->next;
		fprintf(stdout, "color black\n");
		cross(stdout, v->point[0], v->point[1]);
		norm(&nx, &ny, v->point[0], v->point[1], n->point[0], n->point[1]);

		fprintf(stdout, "color red\n");
		{
			static pcb_coord_t as = PCB_MM_TO_COORD(1);
			pcb_coord_t mx = (v->point[0] + n->point[0]) / 2, my = (v->point[1] + n->point[1]) / 2;
			pcb_fprintf(stdout, "line %#mm %#mm %#mm %#mm\n", mx, my, (pcb_coord_t)(mx+nx*as), (pcb_coord_t)(my+ny*as));
		}

		if (p != NULL) {
			double xi, yi, vx1, vy1, vx2, vy2, nx1, ny1, nx2, ny2;
			
			vx1 = p->point[0] - px*offs;
			vy1 = p->point[1] - py*offs;
			vx2 = v->point[0] - px*offs;
			vy2 = v->point[1] - py*offs;

			nx1 = v->point[0] - nx*offs;
			ny1 = v->point[1] - ny*offs;
			nx2 = n->point[0] - nx*offs;
			ny2 = n->point[1] - ny*offs;

pcb_fprintf(stdout, " line %#mm %#mm %#mm %#mm\n", (pcb_coord_t)vx1, (pcb_coord_t)vy1, (pcb_coord_t)vx2, (pcb_coord_t)vy2);
pcb_fprintf(stdout, " line %#mm %#mm %#mm %#mm\n", (pcb_coord_t)nx1, (pcb_coord_t)ny1, (pcb_coord_t)nx2, (pcb_coord_t)ny2);


			ll_intersect(&xi, &yi, vx1, vy1, vx2, vy2, nx1, ny1, nx2, ny2);
			cross(stdout, (pcb_coord_t)xi, (pcb_coord_t)yi);

			tmp[0] = xi;
			tmp[1] = yi;
			if (res == NULL)
				res = pcb_poly_contour_new(tmp);
			else
				pcb_poly_vertex_include(res->head.prev, pcb_poly_node_create(tmp));
		}

		p = v;
		v = v->next;
		px = nx;
		py = ny;
		i++;
	}
	while(i <= num_pts);

	fprintf(stdout, "color green\n");
	pcb_pline_fprint_anim(stdout, res);

fprintf(stdout, "!offs end\n");
	return res;
}

void pcb_pline_to_lines(pcb_layer_t *dst, const pcb_pline_t *src, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags)
{
	const pcb_vnode_t *v, *n;
	pcb_pline_t *track = pcb_pline_dup_offset(src, -((thickness/2)+1));

	v = &track->head;
	do {
		n = v->next;
		pcb_line_new(dst, v->point[0], v->point[1], n->point[0], n->point[1], thickness, clearance, flags);
	}
	while((v = v->next) != &track->head);
	pcb_poly_contour_del(&track);
}

pcb_bool pcb_pline_is_aligned(const pcb_pline_t *src)
{
	const pcb_vnode_t *v, *n;

	v = &src->head;
	do {
		n = v->next;
		if ((v->point[0] != n->point[0]) && (v->point[1] != n->point[1]))
			return pcb_false;
	}
	while((v = v->next) != &src->head);
	return pcb_true;
}


pcb_bool pcb_cpoly_is_simple_rect(const pcb_polygon_t *p)
{
	if (p->Clipped->f != p->Clipped)
		return pcb_false; /* more than one islands */
	if (p->Clipped->contours->next != NULL)
		return pcb_false; /* has holes */
	return pcb_pline_is_rectangle(p->Clipped->contours);
}

pcb_cardinal_t pcb_cpoly_num_corners(const pcb_polygon_t *src)
{
	pcb_cardinal_t res = 0;
	pcb_poly_it_t it;
	pcb_polyarea_t *pa;


	for(pa = pcb_poly_island_first(src, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_pline_t *pl;

		pl = pcb_poly_contour(&it);
		if (pl != NULL) { /* we have a contour */
			res += pl->Count;
			for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it))
				res += pl->Count;
		}
	}

	return res;
}

static void add_track_seg(pcb_cpoly_edgetree_t *dst, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_cpoly_edge_t *e = &dst->edges[dst->used++];
	pcb_box_t *b = &e->bbox;

	if (x1 <= x2) {
		b->X1 = x1;
		b->X2 = x2;
	}
	else {
		b->X1 = x2;
		b->X2 = x1;
	}
	if (y1 <= y2) {
		b->Y1 = y1;
		b->Y2 = y2;
	}
	else {
		b->Y1 = y2;
		b->Y2 = y1;
	}
	pcb_box_bump_box(&dst->bbox, b);
	pcb_r_insert_entry(dst->edge_tree, (pcb_box_t *)e, 0);
}

static void add_track(pcb_cpoly_edgetree_t *dst, pcb_poly_it_t *it, pcb_pline_t *track)
{
	int go;
	pcb_coord_t x, y, px, py;

	for(go = pcb_poly_vect_first(it, &x, &y); go; go = pcb_poly_vect_next(it, &x, &y)) {
		add_track_seg(dst, px, py, x, y);
		px = x;
		py = y;
	}

	pcb_poly_vect_first(it, &x, &y);
	add_track_seg(dst, px, py, x, y);
}


/* collect all edge lines (contour and holes) in an rtree, calculate the bbox */
pcb_cpoly_edgetree_t *pcb_cpoly_edgetree_create(const pcb_polygon_t *src, pcb_coord_t offs)
{
	pcb_poly_it_t it;
	pcb_polyarea_t *pa;
	pcb_cpoly_edgetree_t *res;
	pcb_cardinal_t alloced = pcb_cpoly_num_corners(src) * sizeof(pcb_cpoly_edge_t);

	res = malloc(sizeof(pcb_cpoly_edgetree_t) + alloced);

	res->alloced = alloced;
	res->used = 0;
	res->edge_tree = pcb_r_create_tree(NULL, 0, 0);
	res->bbox.X1 = res->bbox.Y1 = PCB_MAX_COORD;
	res->bbox.X2 = res->bbox.Y2 = -PCB_MAX_COORD;

	for(pa = pcb_poly_island_first(src, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_coord_t x, y;
		pcb_pline_t *pl, *track;

		pl = pcb_poly_contour(&it);
		if (pl != NULL) { /* we have a contour */
			track = pcb_pline_dup_offset(pl, -offs);
			add_track(res, &it, track);
			pcb_poly_contour_del(&track);

			for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it)) {
				track = pcb_pline_dup_offset(pl, +offs);
				add_track(res, &it, track);
				pcb_poly_contour_del(&track);
			}
		}
	}

	return res;
}

void pcb_cpoly_edgetree_destroy(pcb_cpoly_edgetree_t *etr)
{
	pcb_r_destroy_tree(&etr->edge_tree);
	free(etr);
}

void pcb_poly_hatch(const pcb_polygon_t *src, pcb_coord_t offs, void *ctx, void (*cb)(void *ctx, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2))
{
	pcb_cpoly_edgetree_t *etr = pcb_cpoly_edgetree_create(src, offs);

	pcb_cpoly_edgetree_destroy(etr);
}


typedef struct {
	pcb_layer_t *dst;
	pcb_coord_t thickness, clearance;
	pcb_flag_t flags;
} hatch_ctx_t;

static void hatch_cb(void *ctx, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{

}

void pcb_poly_hatch_lines(pcb_layer_t *dst, const pcb_polygon_t *src, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags)
{
	hatch_ctx_t ctx;
	ctx.dst = dst;
	ctx.thickness = thickness;
	ctx.clearance = clearance;
	ctx.flags = flags;

	pcb_poly_hatch(src, (thickness/2)+1, &ctx, hatch_cb);
}



int pplg_check_ver_lib_polyhelp(int ver_needed) { return 0; }

void pplg_uninit_lib_polyhelp(void)
{
}

int pplg_init_lib_polyhelp(void)
{
	return 0;
}
