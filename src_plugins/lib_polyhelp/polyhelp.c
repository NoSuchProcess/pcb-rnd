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
#include "plugins.h"
#include "pcb-printf.h"



void pcb_pline_fprint_anim(FILE *f, pcb_pline_t *pl)
{
	pcb_vnode_t *v, *n;
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


pcb_pline_t *pcb_pline_dup_offset(pcb_pline_t *src, pcb_coord_t offs)
{
	pcb_vnode_t *p = NULL, *v, *n;
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
			
			vx1 = p->point[0] + px*offs;
			vy1 = p->point[1] + py*offs;
			vx2 = v->point[0] + px*offs;
			vy2 = v->point[1] + py*offs;

			nx1 = v->point[0] + nx*offs;
			ny1 = v->point[1] + ny*offs;
			nx2 = n->point[0] + nx*offs;
			ny2 = n->point[1] + ny*offs;

pcb_fprintf(stdout, " line %#mm %#mm %#mm %#mm\n", (pcb_coord_t)vx1, (pcb_coord_t)vy1, (pcb_coord_t)vx2, (pcb_coord_t)vy2);
pcb_fprintf(stdout, " line %#mm %#mm %#mm %#mm\n", (pcb_coord_t)nx1, (pcb_coord_t)ny1, (pcb_coord_t)nx2, (pcb_coord_t)ny2);

			
			ll_intersect(&xi, &yi, vx1, vy1, vx2, vy2, nx1, ny1, nx2, ny2);
			cross(stdout, (pcb_coord_t)xi, (pcb_coord_t)yi);
		}

		p = v;
		v = v->next;
		px = nx;
		py = ny;
		i++;
	}
	while(i <= num_pts);
fprintf(stdout, "!offs end\n");
	return NULL;
}


int pplg_check_ver_lib_polyhelp(int ver_needed) { return 0; }

void pplg_uninit_lib_polyhelp(void)
{
}

int pplg_init_lib_polyhelp(void)
{
	return 0;
}
