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
#include "event.h"
#include "undo.h"
#include "operation.h"
#include "rotate.h"
#include "draw.h"
#include "obj_rat_draw.h"
#include "obj_line_op.h"
#include "obj_line_draw.h"
#include "plugins.h"
#include "conf_core.h"

#include "polygon.h"

typedef struct {								/* rubberband lines for element moves */
	pcb_layer_t *Layer;						/* layer that holds the line */
	pcb_line_t *Line;							/* the line itself */
	pcb_point_t *MovedPoint;			/* and finally the point */
} pcb_rubberband_t;

typedef struct {
	pcb_cardinal_t RubberbandN,  /* number of lines in array */
		RubberbandMax;
	pcb_rubberband_t *Rubberband;
} rubber_ctx_t;
static rubber_ctx_t rubber_band_state;


struct rubber_info {
	pcb_coord_t radius;
	pcb_coord_t X, Y;
	pcb_line_t *line;
	pcb_box_t box;
	pcb_layer_t *layer;
	rubber_ctx_t *rbnd;
};


static pcb_rubberband_t *pcb_rubber_band_alloc(rubber_ctx_t *rbnd);
static pcb_rubberband_t *pcb_rubber_band_create(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *MovedPoint);
static void CheckPadForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pad_t *);
static void CheckPinForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pin_t *);
static void CheckLinePointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_line_t *, pcb_point_t *, pcb_bool);
static void CheckArcPointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_arc_t *, int *, pcb_bool);
static void CheckPolygonForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_polygon_t *);
static void CheckLinePointForRat(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_point_t *);
static pcb_r_dir_t rubber_callback(const pcb_box_t *b, void *cl);





static pcb_r_dir_t rubber_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct rubber_info *i = (struct rubber_info *) cl;
	rubber_ctx_t *rbnd = i->rbnd;
	double x, y, rad, dist1, dist2;
	pcb_coord_t t;
	int n, touches = 0;

	t = line->Thickness / 2;

	for(n = 0; n < rbnd->RubberbandN; n++)
		if (rbnd->Rubberband[n].Line == line)
			return PCB_R_DIR_NOT_FOUND;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
		return PCB_R_DIR_NOT_FOUND;

	if (line == i->line)
		return PCB_R_DIR_NOT_FOUND;

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
				pcb_rubber_band_create(rbnd, i->layer, line, &line->Point1);
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
				pcb_rubber_band_create(rbnd, i->layer, line, &line->Point2);
				found++;
			}
		}
		return found ? PCB_R_DIR_FOUND_CONTINUE : PCB_R_DIR_NOT_FOUND;
	}
	/* circular search region */
	if (i->radius < 0)
		rad = 0;										/* require exact match */
	else
		rad = PCB_SQUARE(i->radius + t);

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
		return PCB_R_DIR_NOT_FOUND;

#ifdef CLOSEST_ONLY							/* keep this to remind me */
	if (dist1 < dist2)
		pcb_rubber_band_create(rbnd, i->layer, line, &line->Point1);
	else
		pcb_rubber_band_create(rbnd, i->layer, line, &line->Point2);
#else
	if (dist1 <= 0)
		pcb_rubber_band_create(rbnd, i->layer, line, &line->Point1);
	if (dist2 <= 0)
		pcb_rubber_band_create(rbnd, i->layer, line, &line->Point2);
#endif
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same layergroup as the
 * passed pad. If one of the endpoints of the line lays inside the pad,
 * the line is added to the 'rubberband' list
 */
static void CheckPadForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pad_t *Pad)
{
	pcb_coord_t half = Pad->Thickness / 2;
	pcb_layergrp_id_t i, group;
	struct rubber_info info;

	info.box.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - half;
	info.box.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - half;
	info.box.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + half;
	info.box.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + half;
	info.radius = 0;
	info.line = NULL;
	info.rbnd = rbnd;
	i = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Pad) ? pcb_solder_silk_layer : pcb_component_silk_layer;
	group = pcb_layer_get_group(i);

	/* check all visible layers in the same group */
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		info.layer = layer;
		if (info.layer->On) {
			pcb_r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
	PCB_END_LOOP;
}

struct rinfo {
	int type;
	pcb_layergrp_id_t group;
	pcb_pin_t *pin;
	pcb_pad_t *pad;
	pcb_point_t *point;
	rubber_ctx_t *rbnd;
};

static pcb_r_dir_t rat_callback(const pcb_box_t * box, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) box;
	struct rinfo *i = (struct rinfo *) cl;
	rubber_ctx_t *rbnd = i->rbnd;

	switch (i->type) {
	case PCB_TYPE_PIN:
		if (rat->Point1.X == i->pin->X && rat->Point1.Y == i->pin->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pin->X && rat->Point2.Y == i->pin->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	case PCB_TYPE_PAD:
		if (rat->Point1.X == i->pad->Point1.X && rat->Point1.Y == i->pad->Point1.Y && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pad->Point1.X && rat->Point2.Y == i->pad->Point1.Y && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point2);
		else if (rat->Point1.X == i->pad->Point2.X && rat->Point1.Y == i->pad->Point2.Y && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->Point2.X == i->pad->Point2.X && rat->Point2.Y == i->pad->Point2.Y && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point2);
		else
			if (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point1);
		else
			if (rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	case PCB_TYPE_LINE_POINT:
		if (rat->group1 == i->group && rat->Point1.X == i->point->X && rat->Point1.Y == i->point->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point1);
		else if (rat->group2 == i->group && rat->Point2.X == i->point->X && rat->Point2.Y == i->point->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, &rat->Point2);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: bad rubber-rat lookup callback\n");
	}
	return PCB_R_DIR_NOT_FOUND;
}

static void CheckPadForRat(rubber_ctx_t *rbnd, pcb_pad_t *Pad)
{
	struct rinfo info;
	pcb_cardinal_t i;

	i = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Pad) ? pcb_solder_silk_layer : pcb_component_silk_layer;
	info.group = pcb_layer_get_group(i);
	info.pad = Pad;
	info.type = PCB_TYPE_PAD;
	info.rbnd = rbnd;

	pcb_r_search(PCB->Data->rat_tree, &Pad->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckPinForRat(rubber_ctx_t *rbnd, pcb_pin_t *Pin)
{
	struct rinfo info;

	info.type = PCB_TYPE_PIN;
	info.pin = Pin;
	info.rbnd = rbnd;
	pcb_r_search(PCB->Data->rat_tree, &Pin->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckLinePointForRat(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_point_t *Point)
{
	struct rinfo info;
	info.group = pcb_layer_get_group_(Layer);
	info.point = Point;
	info.type = PCB_TYPE_LINE_POINT;
	info.rbnd = rbnd;

	pcb_r_search(PCB->Data->rat_tree, (pcb_box_t *) Point, NULL, rat_callback, &info, NULL);
}

/* ---------------------------------------------------------------------------
 * checks all visible lines. If one of the endpoints of the line lays
 * inside the pin, the line is added to the 'rubberband' list
 *
 * Square pins are handled as if they were round. Speed
 * and readability is more important then the few %
 * of failures that are immediately recognized
 */
static void CheckPinForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pin_t *Pin)
{
	struct rubber_info info;
	pcb_cardinal_t n;
	pcb_coord_t t = Pin->Thickness / 2;

	info.box.X1 = Pin->X - t;
	info.box.X2 = Pin->X + t;
	info.box.Y1 = Pin->Y - t;
	info.box.Y2 = Pin->Y + t;
	info.line = NULL;
	info.rbnd = rbnd;

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		info.radius = 0;
	else {
		info.radius = t;
		info.X = Pin->X;
		info.Y = Pin->Y;
	}

	for (n = 0; n < pcb_max_layer; n++) {
		if (pcb_layer_flags(n) & PCB_LYT_COPPER) {
			info.layer = LAYER_PTR(n);
			pcb_r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line lays * inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckLinePointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *LinePoint, pcb_bool Exact)
{
	pcb_layergrp_id_t group;
	struct rubber_info info;
	pcb_coord_t t = Line->Thickness / 2;

	/* lookup layergroup and check all visible lines in this group */
	info.radius = Exact ? -1 : MAX(Line->Thickness / 2, 1);
	info.box.X1 = LinePoint->X - t;
	info.box.X2 = LinePoint->X + t;
	info.box.Y1 = LinePoint->Y - t;
	info.box.Y2 = LinePoint->Y + t;
	info.line = Line;
	info.rbnd = rbnd;
	info.X = LinePoint->X;
	info.Y = LinePoint->Y;

	group = pcb_layer_get_group_(Layer);
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		if (layer->On) {
			info.layer = layer;
			pcb_r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed arc.
 * If one of the endpoints of the line is on the selected arc end,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckArcPointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_arc_t *Arc, int *end_pt, pcb_bool Exact)
{
	pcb_layergrp_id_t group;
	struct rubber_info info;
	pcb_coord_t t = Arc->Thickness / 2, ex, ey;

	pcb_arc_get_end(Arc, (end_pt != pcb_arc_start_ptr), &ex, &ey);

	/* lookup layergroup and check all visible lines in this group */
	info.radius = Exact ? -1 : MAX(Arc->Thickness / 2, 1);
	info.box.X1 = ex - t;
	info.box.X2 = ex + t;
	info.box.Y1 = ey - t;
	info.box.Y2 = ey + t;
	info.line = NULL; /* used only to make sure the current object is not added - we are adding lines only and the current object is an arc */
	info.rbnd = rbnd;
	info.X = ex;
	info.Y = ey;

	group = pcb_layer_get_group_(Layer);
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		if (layer->On) {
			info.layer = layer;
			pcb_r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed polygon.
 * If one of the endpoints of the line lays inside the passed polygon,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckPolygonForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_layergrp_id_t group;

	/* lookup layergroup and check all visible lines in this group */
	group = pcb_layer_get_group_(Layer);
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		if (layer->On) {
			pcb_coord_t thick;

			/* the following code just stupidly compares the endpoints
			 * of the lines
			 */
			PCB_LINE_LOOP(layer);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
					continue;
				if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line))
					continue;
				thick = (line->Thickness + 1) / 2;
				if (pcb_poly_is_point_in_p(line->Point1.X, line->Point1.Y, thick, Polygon))
					pcb_rubber_band_create(rbnd, layer, line, &line->Point1);
				if (pcb_poly_is_point_in_p(line->Point2.X, line->Point2.Y, thick, Polygon))
					pcb_rubber_band_create(rbnd, layer, line, &line->Point2);
			}
			PCB_END_LOOP;
		}
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * lookup all lines that are connected to an object and save the
 * data to 'pcb_crosshair.AttachedObject.Rubberband'
 * lookup is only done for visible layers
 */
static void pcb_rubber_band_lookup_lines(rubber_ctx_t *rbnd, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
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
			PCB_PIN_LOOP(element);
			{
				CheckPinForRubberbandConnection(rbnd, pin);
			}
			PCB_END_LOOP;
			PCB_PAD_LOOP(element);
			{
				CheckPadForRubberbandConnection(rbnd, pad);
			}
			PCB_END_LOOP;
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_layer_t *layer = (pcb_layer_t *) Ptr1;
			pcb_line_t *line = (pcb_line_t *) Ptr2;
			if (pcb_layer_flags(pcb_layer_id(PCB->Data, layer)) & PCB_LYT_COPPER) {
				CheckLinePointForRubberbandConnection(rbnd, layer, line, &line->Point1, pcb_false);
				CheckLinePointForRubberbandConnection(rbnd, layer, line, &line->Point2, pcb_false);
			}
			break;
		}

	case PCB_TYPE_LINE_POINT:
		if (pcb_layer_flags(pcb_layer_id(PCB->Data, (pcb_layer_t *) Ptr1)) & PCB_LYT_COPPER)
			CheckLinePointForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3, pcb_true);
		break;

	case PCB_TYPE_ARC_POINT:
		if (pcb_layer_flags(pcb_layer_id(PCB->Data, (pcb_layer_t *) Ptr1)) & PCB_LYT_COPPER)
			CheckArcPointForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2, (int *) Ptr3, pcb_true);
		break;

	case PCB_TYPE_VIA:
		CheckPinForRubberbandConnection(rbnd, (pcb_pin_t *) Ptr1);
		break;

	case PCB_TYPE_POLYGON:
		if (pcb_layer_flags(pcb_layer_id(PCB->Data, (pcb_layer_t *) Ptr1)) & PCB_LYT_COPPER)
			CheckPolygonForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_polygon_t *) Ptr2);
		break;
	}
}

static void pcb_rubber_band_lookup_rat_lines(rubber_ctx_t *rbnd, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_ELEMENT:
		{
			pcb_element_t *element = (pcb_element_t *) Ptr1;

			PCB_PIN_LOOP(element);
			{
				CheckPinForRat(rbnd, pin);
			}
			PCB_END_LOOP;
			PCB_PAD_LOOP(element);
			{
				CheckPadForRat(rbnd, pad);
			}
			PCB_END_LOOP;
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_layer_t *layer = (pcb_layer_t *) Ptr1;
			pcb_line_t *line = (pcb_line_t *) Ptr2;

			CheckLinePointForRat(rbnd, layer, &line->Point1);
			CheckLinePointForRat(rbnd, layer, &line->Point2);
			break;
		}

	case PCB_TYPE_LINE_POINT:
		CheckLinePointForRat(rbnd, (pcb_layer_t *) Ptr1, (pcb_point_t *) Ptr3);
		break;

	case PCB_TYPE_VIA:
		CheckPinForRat(rbnd, (pcb_pin_t *) Ptr1);
		break;
	}
}

/* ---------------------------------------------------------------------------
 * get next slot for a rubberband connection, allocates memory if necessary
 */
static pcb_rubberband_t *pcb_rubber_band_alloc(rubber_ctx_t *rbnd)
{
	pcb_rubberband_t *ptr = rbnd->Rubberband;

	/* realloc new memory if necessary and clear it */
	if (rbnd->RubberbandN >= rbnd->RubberbandMax) {
		rbnd->RubberbandMax += STEP_RUBBERBAND;
		ptr = (pcb_rubberband_t *) realloc(ptr, rbnd->RubberbandMax * sizeof(pcb_rubberband_t));
		rbnd->Rubberband = ptr;
		memset(ptr + rbnd->RubberbandN, 0, STEP_RUBBERBAND * sizeof(pcb_rubberband_t));
	}
	return (ptr + rbnd->RubberbandN++);
}

/* ---------------------------------------------------------------------------
 * adds a new line to the rubberband list of 'pcb_crosshair.AttachedObject'
 * if Layer == 0  it is a rat line
 */
static pcb_rubberband_t *pcb_rubber_band_create(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *MovedPoint)
{
	pcb_rubberband_t *ptr = pcb_rubber_band_alloc(rbnd);

	/* we toggle the PCB_FLAG_RUBBEREND of the line to determine if */
	/* both points are being moved. */
	PCB_FLAG_TOGGLE(PCB_FLAG_RUBBEREND, Line);
	ptr->Layer = Layer;
	ptr->Line = Line;
	ptr->MovedPoint = MovedPoint;
	return (ptr);
}

/*** event handlers ***/
static void rbe_reset(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	rbnd->RubberbandN = 0;
}

static void rbe_remove_element(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
	void *ptr1 = argv[1].d.p, *ptr2 = argv[2].d.p, *ptr3 = argv[3].d.p;
	int i, type = PCB_TYPE_ELEMENT;

	rbnd->RubberbandN = 0;
	pcb_rubber_band_lookup_rat_lines(rbnd, type, ptr1, ptr2, ptr3);
	ptr = rbnd->Rubberband;
	for (i = 0; i < rbnd->RubberbandN; i++) {
		if (PCB->RatOn)
			EraseRat((pcb_rat_t *) ptr->Line);
		if (PCB_FLAG_TEST(PCB_FLAG_RUBBEREND, ptr->Line))
			pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, ptr->Line, ptr->Line, ptr->Line);
		else
			PCB_FLAG_TOGGLE(PCB_FLAG_RUBBEREND, ptr->Line);	/* only remove line once */
		ptr++;
	}
}

static void rbe_move(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
	pcb_coord_t DX = argv[1].d.c, DY = argv[2].d.c;
	pcb_opctx_t *ctx = argv[3].d.p;

	if (DX == 0 && DY == 0) {
		int n;

		/* first clear any marks that we made in the line flags */
		for(n = 0, ptr = rbnd->Rubberband; n < rbnd->RubberbandN; n++, ptr++)
			PCB_FLAG_CLEAR(PCB_FLAG_RUBBEREND, ptr->Line);

		return;
	}

	/* move all the lines... and reset the counter */
	ptr = rbnd->Rubberband;
	while (rbnd->RubberbandN) {
		/* first clear any marks that we made in the line flags */
		PCB_FLAG_CLEAR(PCB_FLAG_RUBBEREND, ptr->Line);
		pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, DX, DY);
		MoveLinePoint(ctx, ptr->Layer, ptr->Line, ptr->MovedPoint);
		rbnd->RubberbandN--;
		ptr++;
	}
}

static void rbe_draw(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
	pcb_cardinal_t i;
	pcb_coord_t dx = argv[1].d.c, dy = argv[2].d.c;


	/* draw the attached rubberband lines too */
	i = rbnd->RubberbandN;
	ptr = rbnd->Rubberband;
	while (i) {
		pcb_point_t *point1, *point2;

		if (PCB_FLAG_TEST(PCB_FLAG_VIA, ptr->Line)) {
			/* this is a rat going to a polygon.  do not draw for rubberband */ ;
		}
		else if (PCB_FLAG_TEST(PCB_FLAG_RUBBEREND, ptr->Line)) {
			/* 'point1' is always the fix-point */
			if (ptr->MovedPoint == &ptr->Line->Point1) {
				point1 = &ptr->Line->Point2;
				point2 = &ptr->Line->Point1;
			}
			else {
				point1 = &ptr->Line->Point1;
				point2 = &ptr->Line->Point2;
			}
			XORDrawAttachedLine(point1->X, point1->Y, point2->X + dx, point2->Y + dy, ptr->Line->Thickness);
		}
		else if (ptr->MovedPoint == &ptr->Line->Point1)
			XORDrawAttachedLine(ptr->Line->Point1.X + dx,
													ptr->Line->Point1.Y + dy, ptr->Line->Point2.X + dx, ptr->Line->Point2.Y + dy, ptr->Line->Thickness);

		ptr++;
		i--;
	}
}

static void rbe_rotate90(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
/*	int Type = argv[1].d.i; */
/*	void *ptr1 = argv[2].d.p, *ptr2 = argv[3].d.p, *ptr3 = argv[4].d.p; */
	pcb_coord_t cx = argv[5].d.c, cy = argv[6].d.c;
	int steps = argv[7].d.i;
	int *changed = argv[8].d.p;


	/* move all the rubberband lines... and reset the counter */
	ptr = rbnd->Rubberband;
	while (rbnd->RubberbandN) {
		*changed = 1;
		PCB_FLAG_CLEAR(PCB_FLAG_RUBBEREND, ptr->Line);
		pcb_undo_add_obj_to_rotate(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, cx, cy, steps);
		EraseLine(ptr->Line);
		if (ptr->Layer) {
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			pcb_r_delete_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line);
		}
		else
			pcb_r_delete_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line);
		pcb_point_rotate90(ptr->MovedPoint, cx, cy, steps);
		pcb_line_bbox(ptr->Line);
		if (ptr->Layer) {
			pcb_r_insert_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line, 0);
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			DrawLine(ptr->Layer, ptr->Line);
		}
		else {
			pcb_r_insert_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line, 0);
			DrawRat((pcb_rat_t *) ptr->Line);
		}
		rbnd->RubberbandN--;
		ptr++;
	}
}

static void rbe_rename(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	int type = argv[1].d.i;
	void *ptr1 = argv[2].d.p, *ptr2 = argv[3].d.p, *ptr3 = argv[4].d.p;
/*	int pinnum = argv[5].d.i;*/

	if (type == PCB_TYPE_ELEMENT) {
		pcb_rubberband_t *ptr;
		int i;

		pcb_undo_restore_serial();
		rbnd->RubberbandN = 0;
		pcb_rubber_band_lookup_rat_lines(rbnd, type, ptr1, ptr2, ptr3);
		ptr = rbnd->Rubberband;
		for (i = 0; i < rbnd->RubberbandN; i++, ptr++) {
			if (PCB->RatOn)
				EraseRat((pcb_rat_t *) ptr->Line);
			pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, ptr->Line, ptr->Line, ptr->Line);
		}
		pcb_undo_inc_serial();
		pcb_draw();
	}
}

static void rbe_lookup_lines(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	int type = argv[1].d.i;
	void *ptr1 = argv[2].d.p, *ptr2 = argv[3].d.p, *ptr3 = argv[4].d.p;

	if (conf_core.editor.rubber_band_mode)
		pcb_rubber_band_lookup_lines(rbnd, type, ptr1, ptr2, ptr3);
}

static void rbe_lookup_rats(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	int type = argv[1].d.i;
	void *ptr1 = argv[2].d.p, *ptr2 = argv[3].d.p, *ptr3 = argv[4].d.p;

	pcb_rubber_band_lookup_rat_lines(rbnd, type, ptr1, ptr2, ptr3);
}


static const char *rubber_cookie = "old rubberband";

void rb_uninit(void)
{
	pcb_event_unbind_allcookie(rubber_cookie);
}

pcb_uninit_t hid_rubberband_orig_init(void)
{
	void *ctx = &rubber_band_state;
	pcb_event_bind(PCB_EVENT_RUBBER_RESET, rbe_reset, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_REMOVE_ELEMENT, rbe_remove_element, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_MOVE, rbe_move, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_MOVE_DRAW, rbe_draw, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_ROTATE90, rbe_rotate90, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_RENAME, rbe_rename, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_LOOKUP_LINES, rbe_lookup_lines, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_LOOKUP_RATS, rbe_lookup_rats, ctx, rubber_cookie);

	return rb_uninit;
}

