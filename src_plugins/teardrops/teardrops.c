/*!
 * \file teardrops.c
 *
 * \brief Teardrops plug-in for PCB.
 *
 * \author Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
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
#include "hid_actions.h"
#include "obj_all.h"

#define MIN_LINE_LENGTH 700
#define MAX_DISTANCE 700
/* changed MAX_DISTANCE to 0.5 mm below */
 /* 0.5mm */
/* #define MAX_DISTANCE 500000 */
/* #define MAX_DISTANCE 2000000 */
 /* 1 mm */
/* #define MAX_DISTANCE 1000000 */

static pcb_pin_t *pin;
static pcb_pad_t *pad;
static int layer;
static int px, py;
static pcb_coord_t thickness;
static pcb_element_t *element;

static int new_arcs = 0;

int distance_between_points(int x1, int y1, int x2, int y2)
{
	/* int a; */
	/* int b; */
	int distance;
	/* a = (x1-x2); */
	/* b = (y1-y2); */
	distance = sqrt((pow(x1 - x2, 2)) + (pow(y1 - y2, 2)));
	return distance;
}

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
	if (distance_between_points(l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y) < MIN_LINE_LENGTH) {
		fprintf(stderr, "not within max line length\n");
		return 1;
	}

	fprintf(stderr, "......Point (%.6f, %.6f): ", PCB_COORD_TO_MM(px), PCB_COORD_TO_MM(py));

	if (distance_between_points(l->Point1.X, l->Point1.Y, px, py) < MAX_DISTANCE) {
		x1 = l->Point1.X;
		y1 = l->Point1.Y;
		x2 = l->Point2.X;
		y2 = l->Point2.Y;
	}
	else if (distance_between_points(l->Point2.X, l->Point2.Y, px, py) < MAX_DISTANCE) {
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

	/* We need one up front to determine how many segments it will take
	   to fill.  */
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
															(int) radius, (int) theta + 90 + aoffset, delta - aoffset, l->Thickness, l->Clearance, l->Flags);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_TYPE_ARC, lay, arc, arc);

		ax = lx + dy * (x + t);
		ay = ly - dx * (x + t);

		arc = pcb_arc_new(lay, (int) ax, (int) ay, (int) radius,
															(int) radius, (int) theta - 90 - aoffset, -delta + aoffset, l->Thickness, l->Clearance, l->Flags);
		if (arc)
			pcb_undo_add_obj_to_create(PCB_TYPE_ARC, lay, arc, arc);

		radius += t * 1.9;
		aoffset = acos((double) adist / radius) * 180.0 / M_PI;

		new_arcs++;
	} while (vr > radius - t);

	fprintf(stderr, "done arc'ing\n");
	return 1;
}

static void check_pin(pcb_pin_t * _pin)
{
	pcb_box_t spot;

	pin = _pin;

	px = pin->X;
	py = pin->Y;
	thickness = pin->Thickness;

	spot.X1 = px - 10;
	spot.Y1 = py - 10;
	spot.X2 = px + 10;
	spot.Y2 = py + 10;

	element = (pcb_element_t *) pin->Element;

	fprintf(stderr, "Pin %s (%s) at %.6f, %.6f (element %s, %s, %s)\n", PCB_EMPTY(pin->Number), PCB_EMPTY(pin->Name),
					/* 0.01 * pin->X, 0.01 * pin->Y, */
					PCB_COORD_TO_MM(pin->X), PCB_COORD_TO_MM(pin->Y),
					PCB_EMPTY(NAMEONPCB_NAME(element)), PCB_EMPTY(VALUE_NAME(element)), PCB_EMPTY(DESCRIPTION_NAME(element)));

	for (layer = 0; layer < pcb_max_copper_layer; layer++) {
		pcb_layer_t *l = &(PCB->Data->Layer[layer]);
		pcb_r_search(l->line_tree, &spot, NULL, check_line_callback, l, NULL);
	}
}

static void check_via(pcb_pin_t * _pin)
{
	pcb_box_t spot;

	pin = _pin;

	px = pin->X;
	py = pin->Y;

	spot.X1 = px - 10;
	spot.Y1 = py - 10;
	spot.X2 = px + 10;
	spot.Y2 = py + 10;

	fprintf(stderr, "Via at %.6f, %.6f\n", PCB_COORD_TO_MM(pin->X), PCB_COORD_TO_MM(pin->Y));

	for (layer = 0; layer < pcb_max_copper_layer; layer++) {
		pcb_layer_t *l = &(PCB->Data->Layer[layer]);
		pcb_r_search(l->line_tree, &spot, NULL, check_line_callback, l, NULL);
	}
}

/*!
 * \brief Draw teardrops for pads.
 */
static void check_pad(pcb_pad_t * _pad)
{
	pad = _pad;

	px = (pad->BoundingBox.X1 + pad->BoundingBox.X2) / 2;
	py = (pad->BoundingBox.Y1 + pad->BoundingBox.Y2) / 2;
	thickness = pad->Thickness;
	element = (pcb_element_t *) pad->Element;

	fprintf(stderr,
					"Pad %s (%s) at %.6f, %.6f (element %s, %s, %s) \n",
					PCB_EMPTY(pad->Number), PCB_EMPTY(pad->Name),
					PCB_COORD_TO_MM((pad->BoundingBox.X1 + pad->BoundingBox.X2) / 2),
					PCB_COORD_TO_MM((pad->BoundingBox.Y1 + pad->BoundingBox.Y2) / 2),
					PCB_EMPTY(NAMEONPCB_NAME(element)), PCB_EMPTY(VALUE_NAME(element)), PCB_EMPTY(DESCRIPTION_NAME(element)));

	/* fprintf(stderr, */
	/*   "Pad %s (%s) at ((%.6f, %.6f), (%.6f, %.6f)) (element %s, %s, %s) \n", */
	/*           PCB_EMPTY(pad->Number), PCB_EMPTY(pad->Name), */
	/*           PCB_COORD_TO_MM(pad->BoundingBox.X1), */
	/*           PCB_COORD_TO_MM(pad->BoundingBox.Y1), */
	/*           PCB_COORD_TO_MM(pad->BoundingBox.X2), */
	/*           PCB_COORD_TO_MM(pad->BoundingBox.Y2), */
	/*           PCB_EMPTY(NAMEONPCB_NAME (element)), */
	/*           PCB_EMPTY(VALUE_NAME (element)), */
	/*           PCB_EMPTY(DESCRIPTION_NAME (element))); */

	for (layer = 0; layer < pcb_max_copper_layer; layer++) {
		pcb_layer_t *l = &(PCB->Data->Layer[layer]);
		pcb_r_search(l->line_tree, &(pad->BoundingBox), NULL, check_line_callback, l, NULL);
	}
}

static int teardrops(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	new_arcs = 0;

	PCB_VIA_LOOP(PCB->Data);
	{
		check_via(via);
	}
	END_LOOP;

	PCB_PIN_ALL_LOOP(PCB->Data);
	{
		check_pin(pin);
	}
	ENDALL_LOOP;

	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		check_pad(pad);
	}
	ENDALL_LOOP;

	gui->invalidate_all();

	if (new_arcs)
		pcb_undo_inc_serial();

	return 0;
}

static pcb_hid_action_t teardrops_action_list[] = {
	{"Teardrops", NULL, teardrops, NULL, NULL}
};

char *teardrops_cookie = "teardrops plugin";

PCB_REGISTER_ACTIONS(teardrops_action_list, teardrops_cookie)

static void hid_teardrops_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(teardrops_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_teardrops_init()
{
	PCB_REGISTER_ACTIONS(teardrops_action_list, teardrops_cookie);
	return hid_teardrops_uninit;
}
