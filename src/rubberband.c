/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */
/* functions used by 'rubberband moves' */

#define	STEP_RUBBERBAND		100

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "polygon.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void CheckPadForRubberbandConnection(pcb_pad_t *);
static void CheckPinForRubberbandConnection(pcb_pin_t *);
static void CheckLinePointForRubberbandConnection(pcb_layer_t *, pcb_line_t *, pcb_point_t *, pcb_bool);
static void CheckPolygonForRubberbandConnection(pcb_layer_t *, pcb_polygon_t *);
static void CheckLinePointForRat(pcb_layer_t *, pcb_point_t *);
static r_dir_t rubber_callback(const pcb_box_t * b, void *cl);

struct rubber_info {
	Coord radius;
	Coord X, Y;
	pcb_line_t *line;
	pcb_box_t box;
	pcb_layer_t *layer;
};

static r_dir_t rubber_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct rubber_info *i = (struct rubber_info *) cl;
	double x, y, rad, dist1, dist2;
	Coord t;
	int touches = 0;

	t = line->Thickness / 2;

	if (TEST_FLAG(PCB_FLAG_LOCK, line))
		return R_DIR_NOT_FOUND;
	if (line == i->line)
		return R_DIR_NOT_FOUND;
	/*
	 * Check to see if the line touches a rectangular region.
	 * To do this we need to look for the intersection of a circular
	 * region and a rectangular region.
	 */
	if (i->radius == 0) {
		int found = 0;

		if (line->Point1.X + t >= i->box.X1 && line->Point1.X - t <= i->box.X2
				&& line->Point1.Y + t >= i->box.Y1 && line->Point1.Y - t <= i->box.Y2) {
			if (((i->box.X1 <= line->Point1.X) &&
					 (line->Point1.X <= i->box.X2)) || ((i->box.Y1 <= line->Point1.Y) && (line->Point1.Y <= i->box.Y2))) {
				/*
				 * The circle is positioned such that the closest point
				 * on the rectangular region boundary is not at a corner
				 * of the rectangle.  i.e. the shortest line from circle
				 * center to rectangle intersects the rectangle at 90
				 * degrees.  In this case our first test is sufficient
				 */
				touches = 1;
			}
			else {
				/*
				 * Now we must check the distance from the center of the
				 * circle to the corners of the rectangle since the
				 * closest part of the rectangular region is the corner.
				 */
				x = MIN(coord_abs(i->box.X1 - line->Point1.X), coord_abs(i->box.X2 - line->Point1.X));
				x *= x;
				y = MIN(coord_abs(i->box.Y1 - line->Point1.Y), coord_abs(i->box.Y2 - line->Point1.Y));
				y *= y;
				x = x + y - (t * t);

				if (x <= 0)
					touches = 1;
			}
			if (touches) {
				CreateNewRubberbandEntry(i->layer, line, &line->Point1);
				found++;
			}
		}
		if (line->Point2.X + t >= i->box.X1 && line->Point2.X - t <= i->box.X2
				&& line->Point2.Y + t >= i->box.Y1 && line->Point2.Y - t <= i->box.Y2) {
			if (((i->box.X1 <= line->Point2.X) &&
					 (line->Point2.X <= i->box.X2)) || ((i->box.Y1 <= line->Point2.Y) && (line->Point2.Y <= i->box.Y2))) {
				touches = 1;
			}
			else {
				x = MIN(coord_abs(i->box.X1 - line->Point2.X), coord_abs(i->box.X2 - line->Point2.X));
				x *= x;
				y = MIN(coord_abs(i->box.Y1 - line->Point2.Y), coord_abs(i->box.Y2 - line->Point2.Y));
				y *= y;
				x = x + y - (t * t);

				if (x <= 0)
					touches = 1;
			}
			if (touches) {
				CreateNewRubberbandEntry(i->layer, line, &line->Point2);
				found++;
			}
		}
		return found ? R_DIR_FOUND_CONTINUE : R_DIR_NOT_FOUND;
	}
	/* circular search region */
	if (i->radius < 0)
		rad = 0;										/* require exact match */
	else
		rad = SQUARE(i->radius + t);

	x = (i->X - line->Point1.X);
	x *= x;
	y = (i->Y - line->Point1.Y);
	y *= y;
	dist1 = x + y - rad;

	x = (i->X - line->Point2.X);
	x *= x;
	y = (i->Y - line->Point2.Y);
	y *= y;
	dist2 = x + y - rad;

	if (dist1 > 0 && dist2 > 0)
		return R_DIR_NOT_FOUND;

#ifdef CLOSEST_ONLY							/* keep this to remind me */
	if (dist1 < dist2)
		CreateNewRubberbandEntry(i->layer, line, &line->Point1);
	else
		CreateNewRubberbandEntry(i->layer, line, &line->Point2);
#else
	if (dist1 <= 0)
		CreateNewRubberbandEntry(i->layer, line, &line->Point1);
	if (dist2 <= 0)
		CreateNewRubberbandEntry(i->layer, line, &line->Point2);
#endif
	return R_DIR_FOUND_CONTINUE;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same layergroup as the
 * passed pad. If one of the endpoints of the line lays inside the pad,
 * the line is added to the 'rubberband' list
 */
static void CheckPadForRubberbandConnection(pcb_pad_t *Pad)
{
	Coord half = Pad->Thickness / 2;
	pcb_cardinal_t i, group;
	struct rubber_info info;

	info.box.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - half;
	info.box.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - half;
	info.box.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + half;
	info.box.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + half;
	info.radius = 0;
	info.line = NULL;
	i = TEST_FLAG(PCB_FLAG_ONSOLDER, Pad) ? solder_silk_layer : component_silk_layer;
	group = GetLayerGroupNumberByNumber(i);

	/* check all visible layers in the same group */
	GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		info.layer = layer;
		if (info.layer->On) {
			r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
	END_LOOP;
}

struct rinfo {
	int type;
	pcb_cardinal_t group;
	pcb_pin_t *pin;
	pcb_pad_t *pad;
	pcb_point_t *point;
};

static r_dir_t rat_callback(const pcb_box_t * box, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) box;
	struct rinfo *i = (struct rinfo *) cl;

	switch (i->type) {
	case PCB_TYPE_PIN:
		if (rat->Point1.X == i->pin->X && rat->Point1.Y == i->pin->Y)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pin->X && rat->Point2.Y == i->pin->Y)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	case PCB_TYPE_PAD:
		if (rat->Point1.X == i->pad->Point1.X && rat->Point1.Y == i->pad->Point1.Y && rat->group1 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pad->Point1.X && rat->Point2.Y == i->pad->Point1.Y && rat->group2 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point2);
		else if (rat->Point1.X == i->pad->Point2.X && rat->Point1.Y == i->pad->Point2.Y && rat->group1 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pad->Point2.X && rat->Point2.Y == i->pad->Point2.Y && rat->group2 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point2);
		else
			if (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group1 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point1);
		else
			if (rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group2 == i->group)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	case PCB_TYPE_LINE_POINT:
		if (rat->group1 == i->group && rat->Point1.X == i->point->X && rat->Point1.Y == i->point->Y)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->group2 == i->group && rat->Point2.X == i->point->X && rat->Point2.Y == i->point->Y)
			CreateNewRubberbandEntry(NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	default:
		Message(PCB_MSG_DEFAULT, "hace: bad rubber-rat lookup callback\n");
	}
	return R_DIR_NOT_FOUND;
}

static void CheckPadForRat(pcb_pad_t *Pad)
{
	struct rinfo info;
	pcb_cardinal_t i;

	i = TEST_FLAG(PCB_FLAG_ONSOLDER, Pad) ? solder_silk_layer : component_silk_layer;
	info.group = GetLayerGroupNumberByNumber(i);
	info.pad = Pad;
	info.type = PCB_TYPE_PAD;

	r_search(PCB->Data->rat_tree, &Pad->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckPinForRat(pcb_pin_t *Pin)
{
	struct rinfo info;

	info.type = PCB_TYPE_PIN;
	info.pin = Pin;
	r_search(PCB->Data->rat_tree, &Pin->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckLinePointForRat(pcb_layer_t *Layer, pcb_point_t *Point)
{
	struct rinfo info;
	info.group = GetLayerGroupNumberByPointer(Layer);
	info.point = Point;
	info.type = PCB_TYPE_LINE_POINT;

	r_search(PCB->Data->rat_tree, (pcb_box_t *) Point, NULL, rat_callback, &info, NULL);
}

/* ---------------------------------------------------------------------------
 * checks all visible lines. If one of the endpoints of the line lays
 * inside the pin, the line is added to the 'rubberband' list
 *
 * Square pins are handled as if they were round. Speed
 * and readability is more important then the few %
 * of failures that are immediately recognized
 */
static void CheckPinForRubberbandConnection(pcb_pin_t *Pin)
{
	struct rubber_info info;
	pcb_cardinal_t n;
	Coord t = Pin->Thickness / 2;

	info.box.X1 = Pin->X - t;
	info.box.X2 = Pin->X + t;
	info.box.Y1 = Pin->Y - t;
	info.box.Y2 = Pin->Y + t;
	info.line = NULL;
	if (TEST_FLAG(PCB_FLAG_SQUARE, Pin))
		info.radius = 0;
	else {
		info.radius = t;
		info.X = Pin->X;
		info.Y = Pin->Y;
	}

	for (n = 0; n < max_copper_layer; n++) {
		info.layer = LAYER_PTR(n);
		r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line lays * inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckLinePointForRubberbandConnection(pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *LinePoint, pcb_bool Exact)
{
	pcb_cardinal_t group;
	struct rubber_info info;
	Coord t = Line->Thickness / 2;

	/* lookup layergroup and check all visible lines in this group */
	info.radius = Exact ? -1 : MAX(Line->Thickness / 2, 1);
	info.box.X1 = LinePoint->X - t;
	info.box.X2 = LinePoint->X + t;
	info.box.Y1 = LinePoint->Y - t;
	info.box.Y2 = LinePoint->Y + t;
	info.line = Line;
	info.X = LinePoint->X;
	info.Y = LinePoint->Y;
	group = GetLayerGroupNumberByPointer(Layer);
	GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		if (layer->On) {
			info.layer = layer;
			r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed polygon.
 * If one of the endpoints of the line lays inside the passed polygon,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckPolygonForRubberbandConnection(pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_cardinal_t group;

	/* lookup layergroup and check all visible lines in this group */
	group = GetLayerGroupNumberByPointer(Layer);
	GROUP_LOOP(PCB->Data, group);
	{
		if (layer->On) {
			Coord thick;

			/* the following code just stupidly compares the endpoints
			 * of the lines
			 */
			LINE_LOOP(layer);
			{
				if (TEST_FLAG(PCB_FLAG_LOCK, line))
					continue;
				if (TEST_FLAG(PCB_FLAG_CLEARLINE, line))
					continue;
				thick = (line->Thickness + 1) / 2;
				if (IsPointInPolygon(line->Point1.X, line->Point1.Y, thick, Polygon))
					CreateNewRubberbandEntry(layer, line, &line->Point1);
				if (IsPointInPolygon(line->Point2.X, line->Point2.Y, thick, Polygon))
					CreateNewRubberbandEntry(layer, line, &line->Point2);
			}
			END_LOOP;
		}
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * lookup all lines that are connected to an object and save the
 * data to 'Crosshair.AttachedObject.Rubberband'
 * lookup is only done for visible layers
 */
void LookupRubberbandLines(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{

	/* the function is only supported for some types
	 * check all visible lines;
	 * it is only necessary to check if one of the endpoints
	 * is connected
	 */
	switch (Type) {
	case PCB_TYPE_ELEMENT:
		{
			pcb_element_t *element = (pcb_element_t *) Ptr1;

			/* square pins are handled as if they are round. Speed
			 * and readability is more important then the few %
			 * of failures that are immediately recognized
			 */
			PIN_LOOP(element);
			{
				CheckPinForRubberbandConnection(pin);
			}
			END_LOOP;
			PAD_LOOP(element);
			{
				CheckPadForRubberbandConnection(pad);
			}
			END_LOOP;
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_layer_t *layer = (pcb_layer_t *) Ptr1;
			pcb_line_t *line = (pcb_line_t *) Ptr2;
			if (GetLayerNumber(PCB->Data, layer) < max_copper_layer) {
				CheckLinePointForRubberbandConnection(layer, line, &line->Point1, pcb_false);
				CheckLinePointForRubberbandConnection(layer, line, &line->Point2, pcb_false);
			}
			break;
		}

	case PCB_TYPE_LINE_POINT:
		if (GetLayerNumber(PCB->Data, (pcb_layer_t *) Ptr1) < max_copper_layer)
			CheckLinePointForRubberbandConnection((pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3, pcb_true);
		break;

	case PCB_TYPE_VIA:
		CheckPinForRubberbandConnection((pcb_pin_t *) Ptr1);
		break;

	case PCB_TYPE_POLYGON:
		if (GetLayerNumber(PCB->Data, (pcb_layer_t *) Ptr1) < max_copper_layer)
			CheckPolygonForRubberbandConnection((pcb_layer_t *) Ptr1, (pcb_polygon_t *) Ptr2);
		break;
	}
}

void LookupRatLines(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_ELEMENT:
		{
			pcb_element_t *element = (pcb_element_t *) Ptr1;

			PIN_LOOP(element);
			{
				CheckPinForRat(pin);
			}
			END_LOOP;
			PAD_LOOP(element);
			{
				CheckPadForRat(pad);
			}
			END_LOOP;
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_layer_t *layer = (pcb_layer_t *) Ptr1;
			pcb_line_t *line = (pcb_line_t *) Ptr2;

			CheckLinePointForRat(layer, &line->Point1);
			CheckLinePointForRat(layer, &line->Point2);
			break;
		}

	case PCB_TYPE_LINE_POINT:
		CheckLinePointForRat((pcb_layer_t *) Ptr1, (pcb_point_t *) Ptr3);
		break;

	case PCB_TYPE_VIA:
		CheckPinForRat((pcb_pin_t *) Ptr1);
		break;
	}
}

/* ---------------------------------------------------------------------------
 * get next slot for a rubberband connection, allocates memory if necessary
 */
RubberbandTypePtr GetRubberbandMemory(void)
{
	RubberbandTypePtr ptr = Crosshair.AttachedObject.Rubberband;

	/* realloc new memory if necessary and clear it */
	if (Crosshair.AttachedObject.RubberbandN >= Crosshair.AttachedObject.RubberbandMax) {
		Crosshair.AttachedObject.RubberbandMax += STEP_RUBBERBAND;
		ptr = (RubberbandTypePtr) realloc(ptr, Crosshair.AttachedObject.RubberbandMax * sizeof(RubberbandType));
		Crosshair.AttachedObject.Rubberband = ptr;
		memset(ptr + Crosshair.AttachedObject.RubberbandN, 0, STEP_RUBBERBAND * sizeof(RubberbandType));
	}
	return (ptr + Crosshair.AttachedObject.RubberbandN++);
}

/* ---------------------------------------------------------------------------
 * adds a new line to the rubberband list of 'Crosshair.AttachedObject'
 * if Layer == 0  it is a rat line
 */
RubberbandTypePtr CreateNewRubberbandEntry(pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *MovedPoint)
{
	RubberbandTypePtr ptr = GetRubberbandMemory();

	/* we toggle the PCB_FLAG_RUBBEREND of the line to determine if */
	/* both points are being moved. */
	TOGGLE_FLAG(PCB_FLAG_RUBBEREND, Line);
	ptr->Layer = Layer;
	ptr->Line = Line;
	ptr->MovedPoint = MovedPoint;
	return (ptr);
}
