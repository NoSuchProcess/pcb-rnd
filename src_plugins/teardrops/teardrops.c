/*
 * Teardrops plug-in for PCB.
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 * Updated to new data model by Tibor 'Igor2' Palinkas in 2018.
 *
 * Original source: http://www.delorie.com/pcb/teardrops/
*/

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "plugins.h"
#include "actions.h"
#include "obj_pstk_inlines.h"

#define MIN_LINE_LENGTH 700
#define MAX_DISTANCE 700

#define MIN_LINE_LENGTH2 ((double)MIN_LINE_LENGTH*(double)MIN_LINE_LENGTH)
#define MAX_DISTANCE2 ((double)MAX_DISTANCE*(double)MAX_DISTANCE)

static pcb_pstk_t *pstk;
static int layer;
static pcb_coord_t px, py;
static pcb_coord_t thickness;

static int new_arcs = 0;

static pcb_r_dir_t check_line_callback(const pcb_box_t * box, void *cl)
{
	pcb_layer_t *lay = &PCB->Data->Layer[layer];
	pcb_line_t *l = (pcb_line_t *) box;
	int x1, x2, y1, y2;
	double a, b, c, x, r, t;
	double dx, dy, len;
	double ax, ay, lx, ly, theta;
	double ldist, adist, radius;
	double vx, vy, vr, vl;
	int delta, aoffset, count;
	pcb_arc_t *arc;

	fprintf(stderr, "...Line ((%.6f, %.6f), (%.6f, %.6f)): ",
					PCB_COORD_TO_MM(l->Point1.X), PCB_COORD_TO_MM(l->Point1.Y), PCB_COORD_TO_MM(l->Point2.X), PCB_COORD_TO_MM(l->Point2.Y));

	/* if our line is to short ignore it */
	if (pcb_distance2(l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y) < MIN_LINE_LENGTH2) {
		fprintf(stderr, "not within max line length\n");
		return 1;
	}

	fprintf(stderr, "......Point (%.6f, %.6f): ", PCB_COORD_TO_MM(px), PCB_COORD_TO_MM(py));

	if (pcb_distance2(l->Point1.X, l->Point1.Y, px, py) < MAX_DISTANCE2) {
		x1 = l->Point1.X;
		y1 = l->Point1.Y;
		x2 = l->Point2.X;
		y2 = l->Point2.Y;
	}
	else if (pcb_distance(l->Point2.X, l->Point2.Y, px, py) < MAX_DISTANCE2) {
		x1 = l->Point2.X;
		y1 = l->Point2.Y;
		x2 = l->Point1.X;
		y2 = l->Point1.Y;
	}
	else {
		fprintf(stderr, "not within max distance\n");
		return 1;
	}

	/* r = pin->Thickness / 2.0; */
	r = thickness / 2.0;
	t = l->Thickness / 2.0;

	if (t > r) {
		fprintf(stderr, "t > r: t = %3.6f, r = %3.6f\n", PCB_COORD_TO_MM(t), PCB_COORD_TO_MM(r));
		return 1;
	}

	a = 1;
	b = 4 * t - 2 * r;
	c = 2 * t * t - r * r;

	x = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);

	len = sqrt(((double) x2 - x1) * (x2 - x1) + ((double) y2 - y1) * (y2 - y1));

	if (len > (x + t)) {
		adist = ldist = x + t;
		radius = x + t;
		delta = 45;

		if (radius < r || radius < t) {
			fprintf(stderr,
							"(radius < r || radius < t): radius = %3.6f, r = %3.6f, t = %3.6f\n",
							PCB_COORD_TO_MM(radius), PCB_COORD_TO_MM(r), PCB_COORD_TO_MM(t));
			return 1;
		}
	}
	else if (len > r + t) {
		/* special "short teardrop" code */

		x = (len * len - r * r + t * t) / (2 * (r - t));
		ldist = len;
		adist = x + t;
		radius = x + t;
		delta = atan2(len, x + t) * 180.0 / M_PI;
	}
	else
		return 1;

	dx = ((double) x2 - x1) / len;
	dy = ((double) y2 - y1) / len;
	theta = atan2(y2 - y1, x1 - x2) * 180.0 / M_PI;

	lx = px + dx * ldist;
	ly = py + dy * ldist;

	/* We need one up front to determine how many segments it will take to fill.  */
	ax = lx - dy * adist;
	ay = ly + dx * adist;
	vl = sqrt(r * r - t * t);
	vx = px + dx * vl;
	vy = py + dy * vl;
	vx -= dy * t;
	vy += dx * t;
	vr = sqrt((ax - vx) * (ax - vx) + (ay - vy) * (ay - vy));

	aoffset = 0;
	count = 0;
	do {
		if (++count > 5) {
			fprintf(stderr, "......a %d,%d v %d,%d adist %g radius %g vr %g\n",
							(int) ax, (int) ay, (int) vx, (int) vy, adist, radius, vr);
			printf("a %d,%d v %d,%d adist %g radius %g vr %g\n", (int) ax, (int) ay, (int) vx, (int) vy, adist, radius, vr);
			return 1;
		}

		ax = lx - dy * adist;
		ay = ly + dx * adist;

		arc = pcb_arc_new(lay, (int) ax, (int) ay, (int) radius,
															(int) radius, (int) theta + 90 + aoffset, delta - aoffset, l->Thickness, l->Clearance, l->Flags, pcb_true);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_OBJ_ARC, lay, arc, arc);

		ax = lx + dy * (x + t);
		ay = ly - dx * (x + t);

		arc = pcb_arc_new(lay, (int) ax, (int) ay, (int) radius,
															(int) radius, (int) theta - 90 - aoffset, -delta + aoffset, l->Thickness, l->Clearance, l->Flags, pcb_true);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_OBJ_ARC, lay, arc, arc);

		radius += t * 1.9;
		aoffset = acos((double) adist / radius) * 180.0 / M_PI;

		new_arcs++;
	} while (vr > radius - t);

	fprintf(stderr, "done arc'ing\n");
	return 1;
}

static void check_pstk(pcb_pstk_t *ps)
{
	pstk = ps;

	for (layer = 0; layer < pcb_max_layer; layer++) {
		pcb_layer_t *l = &(PCB->Data->Layer[layer]);
		pcb_pstk_shape_t *shp, tmpshp;
		pcb_box_t spot;
		int n;
		double mindist;

		if (!(pcb_layer_flags(PCB, layer) & PCB_LYT_COPPER))
			continue;

		shp = pcb_pstk_shape_at(PCB, ps, l);
		if (shp == NULL)
			continue;

		retry:;
		switch(shp->shape) {
			case PCB_PSSH_POLY:
				/* Simplistic approach on polygons; works only on the simplest cases
				   How this could be handled better: list the 1 or 2 polygon edges
				   that cross the incoming line's sides and do the teardrops there;
				   but there are corner cases lurking: what if the next edges out
				   from the 1..2 edges are curving back? */
				px = py = 0;
				for(n = 0; n < shp->data.poly.len; n++) {
					px += shp->data.poly.x[n];
					py += shp->data.poly.y[n];
				}
				px /= shp->data.poly.len;
				py /= shp->data.poly.len;

				mindist = PCB_MM_TO_COORD(8);
				mindist *= mindist;
				for(n = 0; n < shp->data.poly.len; n++) {
					double dist = pcb_distance2(px, py, shp->data.poly.x[n], shp->data.poly.y[n]);
					if (dist < mindist)
						mindist = dist;
				}
				thickness = sqrt(mindist)*1.4;
				px += ps->x;
				py += ps->y;
				break;

			case PCB_PSSH_LINE:
				thickness = shp->data.line.thickness;
				px = ps->x + (shp->data.line.x1 + shp->data.line.x2)/2;
				py = ps->y + (shp->data.line.y1 + shp->data.line.y2)/2;
				break;

			case PCB_PSSH_CIRC:
				thickness = shp->data.circ.dia;
				px = ps->x + shp->data.circ.x;
				py = ps->y + shp->data.circ.y;
				break;

			case PCB_PSSH_HSHADOW:
				shp = pcb_pstk_hshadow_shape(ps, shp, &tmpshp);
				if (shp != NULL)
					goto retry;
				continue;
		}

		spot.X1 = px - 10;
		spot.Y1 = py - 10;
		spot.X2 = px + 10;
		spot.Y2 = py + 10;

		pcb_r_search(l->line_tree, &spot, NULL, check_line_callback, l, NULL);
	}
}

static fgw_error_t pcb_act_teardrops(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_box_t *b;
	pcb_rtree_it_t it;
	new_arcs = 0;

	for(b = pcb_r_first(PCB->Data->padstack_tree, &it); b != NULL; b = pcb_r_next(&it))
		check_pstk((pcb_pstk_t *)b);
	pcb_r_end(&it);

	pcb_gui->invalidate_all(&PCB->hidlib);

	if (new_arcs)
		pcb_undo_inc_serial();

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t teardrops_action_list[] = {
	{"Teardrops", pcb_act_teardrops, NULL, NULL}
};

char *teardrops_cookie = "teardrops plugin";

PCB_REGISTER_ACTIONS(teardrops_action_list, teardrops_cookie)

int pplg_check_ver_teardrops(int ver_needed) { return 0; }

void pplg_uninit_teardrops(void)
{
	pcb_remove_actions_by_cookie(teardrops_cookie);
}

#include "dolists.h"
int pplg_init_teardrops(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(teardrops_action_list, teardrops_cookie);
	return 0;
}
