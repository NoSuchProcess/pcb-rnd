/*
 * Teardrops plug-in for PCB.
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 * Copyright (C) 2016, 2018, 2021 Tibor 'Igor2' Palinkas

 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 * Updated to new data model by Tibor 'Igor2' Palinkas in 2018.
 * Upgraded for extended objets by Tibor 'Igor2' Palinkas in 2021.
 *
 * Original source: http://www.delorie.com/pcb/teardrops/
*/

#include <stdio.h>
#include <math.h>

#include "config.h"
#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include <librnd/hid/hid.h>
#include <librnd/poly/rtree.h>
#include "undo.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "obj_pstk_inlines.h"

#define MIN_LINE_LENGTH 700
#define MAX_DISTANCE 700

#define MIN_LINE_LENGTH2 ((double)MIN_LINE_LENGTH*(double)MIN_LINE_LENGTH)
#define MAX_DISTANCE2 ((double)MAX_DISTANCE*(double)MAX_DISTANCE)

int teardrop_trace = 0;
#define trprintf \
	if (teardrop_trace) rnd_trace

typedef struct {
	pcb_board_t *pcb;
	pcb_pstk_t *pstk; /* for the search only, not really used in the arc calculations */
	pcb_line_t *line; /* for the search only, not really used in the arc calculations */
	pcb_layer_t *layer;
	rnd_coord_t px, py;
	rnd_coord_t thickness;
	long new_arcs;
	pcb_flag_t flags;
	rnd_coord_t jx, jy; /* output: junction point x;y where all the arcs come together on the line */
} teardrop_t;

#define RCRD(c) ((rnd_coord_t)(rnd_round(c)))

static int teardrop_line(teardrop_t *tr, pcb_line_t *l)
{
	pcb_layer_t *lay = tr->layer;
	int x1, x2, y1, y2;
	double a, b, c, x, r, t;
	double dx, dy, len;
	double ax, ay, lx, ly, theta;
	double ldist, adist, radius;
	double vx, vy, vr, vl;
	int delta, aoffset, count;
	pcb_arc_t *arc;

	trprintf("...Line ((%mm, %mm), (%mm, %mm)): ", l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);

	/* if our line is to short ignore it */
	if (rnd_distance2(l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y) < MIN_LINE_LENGTH2) {
		trprintf("not within max line length\n");
		return 1;
	}

	trprintf("......Point (%mm, %mm): ", tr->px, tr->py);

	if (rnd_distance2(l->Point1.X, l->Point1.Y, tr->px, tr->py) < MAX_DISTANCE2) {
		x1 = l->Point1.X;
		y1 = l->Point1.Y;
		x2 = l->Point2.X;
		y2 = l->Point2.Y;
	}
	else if (rnd_distance(l->Point2.X, l->Point2.Y, tr->px, tr->py) < MAX_DISTANCE2) {
		x1 = l->Point2.X;
		y1 = l->Point2.Y;
		x2 = l->Point1.X;
		y2 = l->Point1.Y;
	}
	else {
		trprintf("not within max distance\n");
		return 1;
	}

	/* r = pin->Thickness / 2.0; */
	r = tr->thickness / 2.0;
	t = l->Thickness / 2.0;

	if (t > r) {
		trprintf("t > r: t = %mm, r = %mm\n", t, r);
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
			trprintf("(radius < r || radius < t): radius = %mm, r = %mm, t = %mm\n", radius, r, t);
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

	lx = tr->px + dx * ldist;
	ly = tr->py + dy * ldist;

	tr->jx = RCRD(lx);
	tr->jy = RCRD(ly);

	/* We need one up front to determine how many segments it will take to fill.  */
	ax = lx - dy * adist;
	ay = ly + dx * adist;
	vl = sqrt(r * r - t * t);
	vx = tr->px + dx * vl;
	vy = tr->py + dy * vl;
	vx -= dy * t;
	vy += dx * t;
	vr = sqrt((ax - vx) * (ax - vx) + (ay - vy) * (ay - vy));

	aoffset = 0;
	count = 0;
	do {
		if (++count > 5) {
			trprintf("......a %mm,%mm v %mm,%mm adist %g radius %g vr %mm\n", RCRD(ax), RCRD(ay), RCRD(vx), RCRD(vy), RCRD(adist), RCRD(radius), RCRD(vr));
			return 1;
		}

		ax = lx - dy * adist;
		ay = ly + dx * adist;

		arc = pcb_arc_new(lay, RCRD(ax), RCRD(ay), RCRD(radius),
															RCRD(radius), theta + 90 + aoffset, delta - aoffset, l->Thickness, l->Clearance, tr->flags, rnd_true);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_OBJ_ARC, lay, arc, arc);

		ax = lx + dy * (x + t);
		ay = ly - dx * (x + t);

		arc = pcb_arc_new(lay, RCRD(ax), RCRD(ay), RCRD(radius),
															RCRD(radius), theta - 90 - aoffset, -delta + aoffset, l->Thickness, l->Clearance, tr->flags, rnd_true);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_OBJ_ARC, lay, arc, arc);

		radius += t * 1.9;
		aoffset = acos((double) adist / radius) * 180.0 / M_PI;

		tr->new_arcs++;
	} while (vr > radius - t);

	trprintf("done arc'ing\n");
	return 0;
}

static int teardrops_init_pstk(teardrop_t *tr, pcb_pstk_t *ps, pcb_layer_t *l)
{
	pcb_pstk_shape_t *shp, tmpshp;
	int n;
	double mindist;

	shp = pcb_pstk_shape_at(tr->pcb, ps, l);
	if (shp == NULL)
		return -1;

	tr->layer = l;

	retry:;
	switch(shp->shape) {
		case PCB_PSSH_POLY:
			/* Simplistic approach on polygons; works only on the simplest cases
			   How this could be handled better: list the 1 or 2 polygon edges
			   that cross the incoming line's sides and do the teardrops there;
			   but there are corner cases lurking: what if the next edges out
			   from the 1..2 edges are curving back? */
			tr->px = tr->py = 0;
			for(n = 0; n < shp->data.poly.len; n++) {
				tr->px += shp->data.poly.x[n];
				tr->py += shp->data.poly.y[n];
			}
			tr->px /= shp->data.poly.len;
			tr->py /= shp->data.poly.len;

			mindist = RND_MM_TO_COORD(8);
			mindist *= mindist;
			for(n = 0; n < shp->data.poly.len; n++) {
				double dist = rnd_distance2(tr->px, tr->py, shp->data.poly.x[n], shp->data.poly.y[n]);
				if (dist < mindist)
					mindist = dist;
			}
			tr->thickness = sqrt(mindist)*1.4;
			tr->px += ps->x;
			tr->py += ps->y;
			break;

		case PCB_PSSH_LINE:
			tr->thickness = shp->data.line.thickness;
			tr->px = ps->x + (shp->data.line.x1 + shp->data.line.x2)/2;
			tr->py = ps->y + (shp->data.line.y1 + shp->data.line.y2)/2;
			break;

		case PCB_PSSH_CIRC:
			tr->thickness = shp->data.circ.dia;
			tr->px = ps->x + shp->data.circ.x;
			tr->py = ps->y + shp->data.circ.y;
			break;

		case PCB_PSSH_HSHADOW:
			shp = pcb_pstk_hshadow_shape(ps, shp, &tmpshp);
			if (shp != NULL)
				goto retry;
			return -1;
	}
	return 0;
}

static rnd_rtree_dir_t check_line_callback(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	teardrop_t *tr = cl;
	pcb_line_t *line = (pcb_line_t *)obj;

	tr->flags = line->Flags;

	teardrop_line(tr, line);
	return rnd_RTREE_DIR_FOUND_CONT;
}

static long check_pstk(pcb_board_t *pcb, pcb_pstk_t *ps)
{
	teardrop_t t;
	rnd_layer_id_t lid;

	t.pcb = pcb;
	t.new_arcs = 0;
	t.pstk = ps;

	for(lid = 0; lid < pcb_max_layer(pcb); lid++) {
		pcb_layer_t *l = &(pcb->Data->Layer[lid]);
		rnd_box_t spot;

		if (!(pcb_layer_flags(pcb, lid) & PCB_LYT_COPPER))
			continue;

		if (teardrops_init_pstk(&t, ps, l) != 0)
			continue;

		spot.X1 = t.px - 10;
		spot.Y1 = t.py - 10;
		spot.X2 = t.px + 10;
		spot.Y2 = t.py + 10;

		rnd_rtree_search_any(l->line_tree, (rnd_rtree_box_t *)&spot, NULL, check_line_callback, &t, NULL);
	}
	return t.new_arcs;
}

static fgw_error_t pcb_act_teardrops(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_box_t *b;
	rnd_rtree_it_t it;
	long new_arcs = 0;
	pcb_board_t *pcb = PCB_ACT_BOARD;

	if (pcb->Data->padstack_tree != NULL)
		for(b = rnd_rtree_all_first(&it, pcb->Data->padstack_tree); b != NULL; b = rnd_rtree_all_next(&it))
			new_arcs += check_pstk(pcb, (pcb_pstk_t *)b);

	rnd_gui->invalidate_all(rnd_gui);

	if (new_arcs)
		pcb_undo_inc_serial();

	RND_ACT_IRES(0);
	return 0;
}

#include "exto_trdp.c"

static rnd_action_t teardrops_action_list[] = {
	{"Teardrops", pcb_act_teardrops, NULL, NULL}
};

char *teardrops_cookie = "teardrops plugin";

int pplg_check_ver_teardrops(int ver_needed) { return 0; }

void pplg_uninit_teardrops(void)
{
	rnd_remove_actions_by_cookie(teardrops_cookie);
	pcb_extobj_unreg(&pcb_trdp);
}

int pplg_init_teardrops(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(teardrops_action_list, teardrops_cookie);
	pcb_extobj_reg(&pcb_trdp);
	return 0;
}
