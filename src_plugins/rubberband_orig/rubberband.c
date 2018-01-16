/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "data_it.h"
#include "data_list.h"
#include "error.h"
#include "event.h"
#include "undo.h"
#include "operation.h"
#include "rotate.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "crosshair.h"
#include "obj_rat_draw.h"
#include "obj_line_op.h"
#include "obj_line_draw.h"
#include "obj_pstk_inlines.h"
#include "plugins.h"
#include "conf_core.h"
#include "layer_grp.h"
#include "fgeometry.h"
#include "search.h"

#include "polygon.h"

#include "rubberband_conf.h"
conf_rubberband_orig_t conf_rbo;


typedef struct {								/* rubberband lines for element moves */
	pcb_layer_t *Layer;						/* layer that holds the line */
	pcb_line_t *Line;							/* the line itself */
/*	pcb_point_t *MovedPoint;	*/		/* and finally the point */
/*
	pcb_coord_t DX;
	pcb_coord_t DY;
	int delta_is_valid; */

	int delta_index[2];           /* The index into the delta array for the two line points */
} pcb_rubberband_t;

typedef struct {
	pcb_layer_t * Layer;					/* Layer that holds the arc */
	pcb_arc_t 	* Arc;						/* The arc itself */
	int						Ends;						/* 1 = first end, 2 = second end, 3 = both ends */
	int           delta_index;    /* The index of the delta passed to rubber move events */
} pcb_rubberband_arc_t;


typedef struct {
	pcb_cardinal_t RubberbandN;  /* number of lines in array */
	pcb_cardinal_t RubberbandMax;
	pcb_rubberband_t *Rubberband;
	pcb_cardinal_t RubberbandArcN;  /* number of lines in array */
	pcb_cardinal_t RubberbandArcMax;
	pcb_rubberband_arc_t *RubberbandArcs;
} rubber_ctx_t;

static rubber_ctx_t rubber_band_state;


struct rubber_info {
	pcb_coord_t radius;
	pcb_coord_t X, Y;
	pcb_line_t *line;
	pcb_box_t box;
	pcb_layer_t *layer;
	rubber_ctx_t *rbnd;
	int delta_index;
};


static pcb_rubberband_t *pcb_rubber_band_alloc(rubber_ctx_t *rbnd);
static pcb_rubberband_arc_t *pcb_rubber_band_arc_alloc(rubber_ctx_t *rbnd);
static pcb_rubberband_t *pcb_rubber_band_create(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, int point_number,int delta_index);
static pcb_rubberband_arc_t *pcb_rubber_band_create_arc(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_arc_t *Line, int end);
static void CheckPadForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pad_t *);
static void CheckPinForRubberbandConnection(rubber_ctx_t *rbnd, pcb_pin_t *);
static void CheckLinePointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_line_t *, pcb_point_t *,int delta_index);
static void CheckArcPointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_arc_t *, int *, pcb_bool);
static void CheckArcForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_arc_t *, pcb_bool);
static void CheckPolygonForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_poly_t *);
static void CheckLinePointForRat(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_point_t *);
static void CheckLineForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_line_t *);

static void	calculate_route_rubber_arc_point_move(pcb_rubberband_arc_t * arcptr,pcb_coord_t group_dx,pcb_coord_t group_dy,pcb_route_t * route);

static void CheckLinePointForRubberbandArcConnection(rubber_ctx_t *rbnd, pcb_layer_t *, pcb_line_t *, pcb_point_t *, pcb_bool);

static pcb_r_dir_t rubber_callback(const pcb_box_t *b, void *cl);
static pcb_r_dir_t rubber_callback_arc(const pcb_box_t * b, void *cl);

static pcb_r_dir_t rubber_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	pcb_rubberband_t *have_line = NULL;
	struct rubber_info *i = (struct rubber_info *) cl;
	rubber_ctx_t *rbnd = i->rbnd;
	double x, y, rad, dist1, dist2;
	pcb_coord_t t;
	int n, touches1 = 0, touches2 = 0;
	int have_point1 = 0;
	int have_point2 = 0;

	t = line->Thickness / 2;

	/* Don't add the line if both ends of it are already in the list. */
	for(n = 0; (n < rbnd->RubberbandN) && (have_line == NULL); n++)
		if (rbnd->Rubberband[n].Line == line) {
			have_line = &rbnd->Rubberband[n];
			if(rbnd->Rubberband[n].delta_index[0] >= 0)
				have_point1 = 1;
			if(rbnd->Rubberband[n].delta_index[1] >= 0)
				have_point2 = 1;
		}

	if(have_point1 && have_point2)
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

		if (!have_point1
				&& line->Point1.X + t >= i->box.X1 && line->Point1.X - t <= i->box.X2
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
				touches1 = 1;
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
					touches1 = 1;
			}
		}
		if (!have_point2
				&& line->Point2.X + t >= i->box.X1 && line->Point2.X - t <= i->box.X2
				&& line->Point2.Y + t >= i->box.Y1 && line->Point2.Y - t <= i->box.Y2) {
			if (((i->box.X1 <= line->Point2.X) &&
					 (line->Point2.X <= i->box.X2)) || ((i->box.Y1 <= line->Point2.Y) && (line->Point2.Y <= i->box.Y2))) {
				touches2 = 1;
			}
			else {
				x = MIN(coord_abs(i->box.X1 - line->Point2.X), coord_abs(i->box.X2 - line->Point2.X));
				x *= x;
				y = MIN(coord_abs(i->box.Y1 - line->Point2.Y), coord_abs(i->box.Y2 - line->Point2.Y));
				y *= y;
				x = x + y - (t * t);

				if (x <= 0)
					touches2 = 1;
			}
		}
	}
	else
	{
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
#if 0
#ifdef CLOSEST_ONLY							/* keep this to remind me */
		if ((dist1 < dist2) && !have_point1)
			touches1 = 1;
		else if(!have_point2)
			touches2 = 1;
#else
		if ((dist1 <= 0) && !have_point1)
			touches1 = 1;
		if ((dist2 <= 0) && !have_point2)
			touches2 = 1;
#endif
#endif
		if ((dist1 <= 0) && !have_point1)
			touches1 = 1;
		if ((dist2 <= 0) && !have_point2)
			touches2 = 1;
	}

	if (touches1) {
		if(have_line)
			have_line->delta_index[0] = i->delta_index;
		else
			have_line = pcb_rubber_band_create(rbnd, i->layer, line, 0,i->delta_index);
	}

	if (touches2) {
		if(have_line)
			have_line->delta_index[1] = i->delta_index;
		else
			pcb_rubber_band_create(rbnd, i->layer, line, 1,i->delta_index);
	}

	return touches1 || touches2 ? PCB_R_DIR_FOUND_CONTINUE : PCB_R_DIR_NOT_FOUND;
}

static pcb_r_dir_t rubber_callback_arc(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct rubber_info *i = (struct rubber_info *) cl;
	rubber_ctx_t *rbnd = i->rbnd;
	double x, y, rad, dist1, dist2;
	pcb_coord_t t;
	pcb_coord_t ex1, ey1;
	pcb_coord_t ex2, ey2;
	int n = 0;
	int have_point1 = 0;
	int have_point2 = 0;

	/* If the arc is locked then don't add it to the rubberband list. */
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, arc))
		return PCB_R_DIR_NOT_FOUND;

	/* Don't add the arc if both ends of it are already in the list. */
	for(n = 0; n < rbnd->RubberbandArcN; n++)
		if (rbnd->RubberbandArcs[n].Arc == arc) {
			have_point1 = rbnd->RubberbandArcs[n].Ends & 1;
			have_point2 = rbnd->RubberbandArcs[n].Ends & 2;
		}

	if(have_point1 && have_point2)
			return PCB_R_DIR_NOT_FOUND;

	/* Calculate the arc end points */	
	pcb_arc_get_end(arc,0,&ex1, &ey1);
	pcb_arc_get_end(arc,1,&ex2, &ey2);

	/* circular search region */
	t = arc->Thickness / 2;

	if (i->radius < 0)
		rad = 0;										/* require exact match */
	else
		rad = PCB_SQUARE(i->radius + t);

	x = (i->X - ex1);
	x *= x;
	y = (i->Y - ey1);
	y *= y;
	dist1 = x + y - rad;

	x = (i->X - ex2);
	x *= x;
	y = (i->Y - ey2);
	y *= y;
	dist2 = x + y - rad;

	if (dist1 > 0 && dist2 > 0)
		return PCB_R_DIR_NOT_FOUND;

	/* The Arc end-point is touching so create an entry in the rubberband arc list */

#ifdef CLOSEST_ONLY							/* keep this to remind me */
	if ((dist1 < dist2) && !have_point1)
		pcb_rubber_band_create_arc(rbnd, i->layer, arc, 1);
	else if(!have_point2)
		pcb_rubber_band_create_arc(rbnd, i->layer, arc, 2);
#else
	if ((dist1 <= 0) && !have_point1)
		pcb_rubber_band_create_arc(rbnd, i->layer, arc, 1);
	if ((dist2 <= 0) && !have_point2)
		pcb_rubber_band_create_arc(rbnd, i->layer, arc, 2);
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
	pcb_layergrp_id_t group;
	struct rubber_info info;
	unsigned int flg;

	info.box.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - half;
	info.box.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - half;
	info.box.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + half;
	info.box.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + half;
	info.radius = 0;
	info.line = NULL;
	info.rbnd = rbnd;
	info.delta_index = 0;
	flg = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Pad) ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	if (pcb_layergrp_list(PCB, flg | PCB_LYT_COPPER, &group, 1) < 1)
		return;

	/* check all visible layers in the same group */
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		/* check all visible lines of the group member */
		info.layer = layer;
		if (info.layer->meta.real.vis) {
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
	int delta_index;
};

static pcb_r_dir_t rat_callback(const pcb_box_t * box, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) box;
	struct rinfo *i = (struct rinfo *) cl;
	rubber_ctx_t *rbnd = i->rbnd;

	switch (i->type) {
	case PCB_TYPE_PIN:
		if (rat->Point1.X == i->pin->X && rat->Point1.Y == i->pin->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 0,i->delta_index);
		else if (rat->Point2.X == i->pin->X && rat->Point2.Y == i->pin->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 1,i->delta_index);
		break;
	case PCB_TYPE_PAD:
		if (rat->Point1.X == i->pad->Point1.X && rat->Point1.Y == i->pad->Point1.Y && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 0,i->delta_index);
		else if (rat->Point2.X == i->pad->Point1.X && rat->Point2.Y == i->pad->Point1.Y && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 1,i->delta_index);
		else if (rat->Point1.X == i->pad->Point2.X && rat->Point1.Y == i->pad->Point2.Y && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 0,i->delta_index);
		else if (rat->Point2.X == i->pad->Point2.X && rat->Point2.Y == i->pad->Point2.Y && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 1,i->delta_index);
		else
			if (rat->Point1.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point1.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group1 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 0,i->delta_index);
		else
			if (rat->Point2.X == (i->pad->Point1.X + i->pad->Point2.X) / 2 &&
					rat->Point2.Y == (i->pad->Point1.Y + i->pad->Point2.Y) / 2 && rat->group2 == i->group)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 1,i->delta_index);
		break;
	case PCB_TYPE_LINE_POINT:
		if (rat->group1 == i->group && rat->Point1.X == i->point->X && rat->Point1.Y == i->point->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 0,i->delta_index);
		else if (rat->group2 == i->group && rat->Point2.X == i->point->X && rat->Point2.Y == i->point->Y)
			pcb_rubber_band_create(rbnd, NULL, (pcb_line_t *) rat, 1,i->delta_index);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: bad rubber-rat lookup callback\n");
	}
	return PCB_R_DIR_NOT_FOUND;
}

static void CheckPadForRat(rubber_ctx_t *rbnd, pcb_pad_t *Pad)
{
	struct rinfo info;
	unsigned int flg;
	pcb_layergrp_id_t group;

	flg = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Pad) ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	if (pcb_layergrp_list(PCB, flg | PCB_LYT_COPPER, &group, 1) < 1)
		return;

	info.group = group;
	info.pad = Pad;
	info.type = PCB_TYPE_PAD;
	info.rbnd = rbnd;
	info.delta_index = 0;
	pcb_r_search(PCB->Data->rat_tree, &Pad->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckPinForRat(rubber_ctx_t *rbnd, pcb_pin_t *Pin)
{
	struct rinfo info;

	info.type = PCB_TYPE_PIN;
	info.pin = Pin;
	info.rbnd = rbnd;
	info.delta_index = 0;
	pcb_r_search(PCB->Data->rat_tree, &Pin->BoundingBox, NULL, rat_callback, &info, NULL);
}

static void CheckLinePointForRat(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_point_t *Point)
{
	struct rinfo info;
	info.group = pcb_layer_get_group_(Layer);
	info.point = Point;
	info.type = PCB_TYPE_LINE_POINT;
	info.rbnd = rbnd;
	info.delta_index = 0;

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
	info.delta_index = 0;

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		info.radius = 0;
	else {
		info.radius = t;
		info.X = Pin->X;
		info.Y = Pin->Y;
	}

	for (n = 0; n < pcb_max_layer; n++) {
		if (pcb_layer_flags(PCB, n) & PCB_LYT_COPPER) {
			info.layer = LAYER_PTR(n);
			pcb_r_search(info.layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed line.
 * If one of the endpoints of the line is connected to an endpoint of the 
 * passed line, the scanned line is added to the 'rubberband' list
 */
static void CheckLinePointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *LinePoint,int delta_index)
{
	const pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);
	pcb_board_t * board = pcb_data_get_top(PCB->Data);

	if(board == NULL)
		board = PCB;

	if(group >= 0) {
		pcb_cardinal_t length = board->LayerGroups.grp[group].len;
		pcb_cardinal_t entry;
		const pcb_coord_t t = Line->Thickness / 2;
		const int comb = Layer->comb & PCB_LYC_SUB;
		struct rubber_info info;

		info.radius = -1;
		info.box.X1 = LinePoint->X - t;
		info.box.X2 = LinePoint->X + t;
		info.box.Y1 = LinePoint->Y - t;
		info.box.Y2 = LinePoint->Y + t;
		info.line = Line;
		info.rbnd = rbnd;
		info.X = LinePoint->X;
		info.Y = LinePoint->Y;
		info.delta_index = delta_index;

		for(entry = 0;entry < length;++entry)	{
			const pcb_layer_id_t layer_id = board->LayerGroups.grp[group].lid[entry];
			pcb_layer_t * layer = &PCB->Data->Layer[layer_id];

			if(layer->meta.real.vis && ((layer->comb & PCB_LYC_SUB) == comb))	{
				info.layer = layer;
				pcb_r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible arcs which belong to the same group as the passed line.
 * If one of the endpoints of the arc lays * inside the passed line,
 * the scanned arc is added to the 'rubberband' list
 */
static void CheckLinePointForRubberbandArcConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *LinePoint, pcb_bool Exact)
{
	const pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);
	pcb_board_t * board = pcb_data_get_top(PCB->Data);

	if(board == NULL)
		board = PCB;

	if(group >= 0) {
		pcb_cardinal_t length = board->LayerGroups.grp[group].len;
		pcb_cardinal_t entry;
		const pcb_coord_t t = Line->Thickness / 2;
		const int comb = Layer->comb & PCB_LYC_SUB;
		struct rubber_info info;

		info.radius = Exact ? -1 : MAX(Line->Thickness / 2, 1);
		info.box.X1 = LinePoint->X - t;
		info.box.X2 = LinePoint->X + t;
		info.box.Y1 = LinePoint->Y - t;
		info.box.Y2 = LinePoint->Y + t;
		info.line = Line;
		info.rbnd = rbnd;
		info.X = LinePoint->X;
		info.Y = LinePoint->Y;
		info.delta_index = 0;

		for(entry = 0;entry < length;++entry)	{
			const pcb_layer_id_t layer_id = board->LayerGroups.grp[group].lid[entry];
			pcb_layer_t * layer = &PCB->Data->Layer[layer_id];

			if(layer->meta.real.vis && ((layer->comb & PCB_LYC_SUB) == comb))	{
				info.layer = layer;
				pcb_r_search(layer->arc_tree, &info.box, NULL, rubber_callback_arc, &info, NULL);
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed arc.
 * If one of the endpoints of the line is on the selected arc end,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckArcPointForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_arc_t *Arc, int *end_pt, pcb_bool Exact)
{
	pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);
	struct rubber_info info;
	pcb_coord_t t = Arc->Thickness / 2, ex, ey;
	int end; /* = end_pt == pcb_arc_start_ptr ? 0 : 1;*/

	for(end=0;end<=1;++end)
	{
		pcb_arc_get_end(Arc, end, &ex, &ey);

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
		info.delta_index = end;

		PCB_COPPER_GROUP_LOOP(PCB->Data, group);
		{
			/* check all visible lines of the group member */
			if (layer->meta.real.vis) {
				info.layer = layer;
				pcb_r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
			}
		}
		PCB_END_LOOP;
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed arc.
 * If one of the endpoints of the line is on either of the selected arcs ends,
 * the scanned line is added to the 'rubberband' list.
 */
static void CheckArcForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_arc_t *Arc, pcb_bool Exact)
{
	struct rubber_info info;
	int which;
	pcb_coord_t t = Arc->Thickness / 2, ex, ey;
	const pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);
	pcb_board_t * board = pcb_data_get_top(PCB->Data);

	if(board == NULL)
		board = PCB;

	if(group >= 0) {
		pcb_cardinal_t length = board->LayerGroups.grp[group].len;
		pcb_cardinal_t entry;
		const int comb = Layer->comb & PCB_LYC_SUB;

		for(which=0; which<=1; ++which) {
			pcb_arc_get_end(Arc,which,&ex, &ey);

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
			info.delta_index = which;

			for(entry = 0;entry < length;++entry) {
				const pcb_layer_id_t layer_id = board->LayerGroups.grp[group].lid[entry];
				pcb_layer_t * layer = &PCB->Data->Layer[layer_id];

				if(layer->meta.real.vis && ((layer->comb & PCB_LYC_SUB) == comb)) {
					/* check all visible lines of the group member */
					info.layer = layer;
					pcb_r_search(layer->line_tree, &info.box, NULL, rubber_callback, &info, NULL);
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed Arc.
 * If either of the endpoints of the line lays anywhere inside the passed Arc,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckEntireArcForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t * Layer, pcb_arc_t * Arc)
{
	/* lookup layergroup and check all visible lines in this group */
	pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);

	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		if (layer->meta.real.vis) {
			pcb_coord_t thick;

			/* the following code just stupidly compares the endpoints of the lines */
			PCB_LINE_LOOP(layer);
			{
				pcb_rubberband_t *have_line = NULL;
				pcb_bool touches1 = pcb_false;
				pcb_bool touches2 = pcb_false;
				int l;

				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
					continue;

				/* Check whether the line is already in the rubberband list. */
				for(l = 0; (l < rbnd->RubberbandN) && (have_line == NULL); l++)
					if (rbnd->Rubberband[l].Line == line) 
						have_line = &rbnd->Rubberband[l];

				/* Check whether any of the scanned line points touch the passed line */
				thick = (line->Thickness + 1) / 2;
				touches1 = pcb_is_point_on_arc(line->Point1.X, line->Point1.Y, thick, Arc);
				touches2 = pcb_is_point_on_arc(line->Point2.X, line->Point2.Y, thick, Arc);

				if(touches1) {
					if(have_line)
						have_line->delta_index[0] = 0;
					else 
						have_line =	pcb_rubber_band_create(rbnd, layer, line, 0,0);
				}

				if(touches2) {
					if(have_line)
						have_line->delta_index[1] = 0;
					else 
						have_line =	pcb_rubber_band_create(rbnd, layer, line, 1,0);
				}
			}
			PCB_END_LOOP;
		}
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed polygon.
 * If one of the endpoints of the line lays inside the passed polygon,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckPolygonForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_layergrp_id_t group;
	const pcb_bool clearpoly = PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY,Polygon);

	/* lookup layergroup and check all visible lines in this group */
	group = pcb_layer_get_group_(Layer);
	PCB_COPPER_GROUP_LOOP(PCB->Data, group);
	{
		if (layer->meta.real.vis) {
			pcb_coord_t thick;

			/* the following code just stupidly compares the endpoints
			 * of the lines
			 */
			PCB_LINE_LOOP(layer);
			{
				pcb_rubberband_t *have_line = NULL;
				int touches1 = 0, touches2 = 0;
				int l;

				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
					continue;
				if (clearpoly && PCB_OBJ_HAS_CLEARANCE(line))
					continue;

				/* Check whether the line is already in the rubberband list. */
				for(l = 0; (l < rbnd->RubberbandN) && (have_line == NULL); l++)
					if (rbnd->Rubberband[l].Line == line) 
						have_line = &rbnd->Rubberband[l];

				/* Check whether any of the line points touch the polygon */
				thick = (line->Thickness + 1) / 2;
				touches1 = (pcb_poly_is_point_in_p(line->Point1.X, line->Point1.Y, thick, Polygon));
				touches2 = (pcb_poly_is_point_in_p(line->Point2.X, line->Point2.Y, thick, Polygon));

				if(touches1) {
					if(have_line)
						have_line->delta_index[0] = 0;
					else 
						have_line =	pcb_rubber_band_create(rbnd, layer, line, 0,0);
				}

				if(touches2) {
					if(have_line)
						have_line->delta_index[1] = 0;
					else 
						have_line =	pcb_rubber_band_create(rbnd, layer, line, 1,0);
				}
			}
			PCB_END_LOOP;
		}
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * checks all visible lines which belong to the same group as the passed Line.
 * If either of the endpoints of the line lays anywhere inside the passed line,
 * the scanned line is added to the 'rubberband' list
 */
static void CheckLineForRubberbandConnection(rubber_ctx_t *rbnd, pcb_layer_t * Layer, pcb_line_t * Line)
{
	pcb_layergrp_id_t group = pcb_layer_get_group_(Layer);
	pcb_board_t * board = pcb_data_get_top(PCB->Data);

	if(board == NULL)
		board = PCB;

	if(group >= 0) {
		pcb_cardinal_t length = board->LayerGroups.grp[group].len;
		pcb_cardinal_t entry;
		const int comb = Layer->comb & PCB_LYC_SUB;

		for(entry = 0;entry < length;++entry) {
			const pcb_layer_id_t layer_id = board->LayerGroups.grp[group].lid[entry];
			pcb_layer_t * layer = &PCB->Data->Layer[layer_id];

			if (layer->meta.real.vis && ((layer->comb & PCB_LYC_SUB) == comb)) {
				pcb_coord_t thick;

				/* the following code just stupidly compares the endpoints of the lines */
				PCB_LINE_LOOP(layer);
				{
					pcb_rubberband_t *have_line = NULL;
					pcb_bool touches1 = pcb_false;
					pcb_bool touches2 = pcb_false;
					int l;

					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
						continue;

					/* Check whether the line is already in the rubberband list. */
					for(l = 0; (l < rbnd->RubberbandN) && (have_line == NULL); l++)
						if (rbnd->Rubberband[l].Line == line) 
							have_line = &rbnd->Rubberband[l];

					/* Check whether any of the scanned line points touch the passed line */
					thick = (line->Thickness + 1) / 2;
					touches1 = pcb_is_point_on_line(line->Point1.X, line->Point1.Y, thick, Line);
					touches2 = pcb_is_point_on_line(line->Point2.X, line->Point2.Y, thick, Line);

					if(touches1) {
						if(have_line)
							have_line->delta_index[0] = 0;
						else 
							have_line =	pcb_rubber_band_create(rbnd, layer, line, 0,0);
					}

					if(touches2) {
						if(have_line)
							have_line->delta_index[1] = 0;
						else 
							have_line =	pcb_rubber_band_create(rbnd, layer, line, 1,0);
					}
				}
				PCB_END_LOOP;
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * checks all visible lines.  If either of the endpoints of the line lays 
 * anywhere inside the passed padstack, the scanned line is added to the 
 * 'rubberband' list.
 */
static void CheckPadStackForRubberbandConnection(rubber_ctx_t *rbnd,pcb_pstk_t * pstk)
{
	pcb_layergrp_id_t top;	
	pcb_layergrp_id_t bottom;
	pcb_bb_type_t bb_type;

	bb_type = pcb_pstk_bbspan(PCB,pstk,&top,&bottom,NULL);

	if(bb_type != PCB_BB_INVALID)
	{
		for(;top<=bottom;++top)
		{
			PCB_COPPER_GROUP_LOOP(PCB->Data, top);
			{
				if (layer->meta.real.vis) {
					pcb_coord_t thick;

					/* the following code just stupidly compares the endpoints of the lines */
					PCB_LINE_LOOP(layer);
					{
						pcb_rubberband_t *have_line = NULL;
						pcb_bool touches1 = pcb_false;
						pcb_bool touches2 = pcb_false;
						int l;

						if (PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
							continue;

						/* Check whether the line is already in the rubberband list. */
						for(l = 0; (l < rbnd->RubberbandN) && (have_line == NULL); l++)
							if (rbnd->Rubberband[l].Line == line) 
								have_line = &rbnd->Rubberband[l];

						/* Check whether any of the scanned line points touch the passed padstack */
						thick = (line->Thickness + 1) / 2;
						touches1 = pcb_is_point_in_pstk(line->Point1.X, line->Point1.Y, thick, pstk, layer);
						touches2 = pcb_is_point_in_pstk(line->Point2.X, line->Point2.Y, thick, pstk, layer);

						if(touches1) {
							if(have_line)
								have_line->delta_index[0] = 0;
							else 
								have_line =	pcb_rubber_band_create(rbnd, layer, line, 0,0);
						}

						if(touches2) {
							if(have_line)
								have_line->delta_index[1] = 0;
							else 
								have_line =	pcb_rubber_band_create(rbnd, layer, line, 1,0);
						}
					}
					PCB_END_LOOP;
				}
			}
			PCB_END_LOOP;
		}
	}
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
	case PCB_TYPE_PSTK:
		CheckPadStackForRubberbandConnection(rbnd,(pcb_pstk_t *)Ptr1);
		break;

	case PCB_TYPE_SUBC:
		{
			pcb_subc_t * subc = (pcb_subc_t *)Ptr1;
			pcb_data_it_t	it;
			pcb_any_obj_t * p_obj;
		 
			p_obj	= pcb_data_first(&it,subc->data,PCB_OBJ_CLASS_REAL);

			while(p_obj)
			{
				pcb_layer_t * layer = p_obj->parent.layer;
				switch(p_obj->type)
				{
					case PCB_OBJ_LINE :	if(p_obj->term) CheckLineForRubberbandConnection(rbnd, layer, (pcb_line_t *) p_obj); break;
					case PCB_OBJ_POLY :	if(p_obj->term) CheckPolygonForRubberbandConnection(rbnd, layer, (pcb_poly_t *) p_obj); break;
					case PCB_OBJ_ARC :	if(p_obj->term) CheckEntireArcForRubberbandConnection(rbnd, layer, (pcb_arc_t *) p_obj); break;
					case PCB_OBJ_PSTK :	CheckPadStackForRubberbandConnection(rbnd, (pcb_pstk_t *) p_obj); break;
					default : 					break;
				}
				p_obj = pcb_data_next(&it);
			}
			break;
		}

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
			CheckLinePointForRubberbandConnection(rbnd, layer, line, &line->Point1,0 );
			CheckLinePointForRubberbandConnection(rbnd, layer, line, &line->Point2,1 );
			break;
		}

	case PCB_TYPE_LINE_POINT:
		CheckLinePointForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3,0);
		if(conf_rbo.plugins.rubberband_orig.enable_rubberband_arcs != 0)
			CheckLinePointForRubberbandArcConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3, pcb_true);
		break;

	case PCB_TYPE_ARC_POINT:
		if (pcb_layer_flags_((pcb_layer_t *)Ptr1) & PCB_LYT_COPPER)
			CheckArcPointForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2, (int *) Ptr3, pcb_true);
		break;

	case PCB_TYPE_ARC:
		CheckArcForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2, pcb_true);
		break;

	case PCB_TYPE_VIA:
		CheckPinForRubberbandConnection(rbnd, (pcb_pin_t *) Ptr1);
		break;

	case PCB_TYPE_POLY:
		if (pcb_layer_flags_((pcb_layer_t *)Ptr1) & PCB_LYT_COPPER)
			CheckPolygonForRubberbandConnection(rbnd, (pcb_layer_t *) Ptr1, (pcb_poly_t *) Ptr2);
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

static pcb_rubberband_arc_t *pcb_rubber_band_arc_alloc(rubber_ctx_t *rbnd)
{
	pcb_rubberband_arc_t *ptr = rbnd->RubberbandArcs;

	/* realloc new memory if necessary and clear it */
	if (rbnd->RubberbandArcN >= rbnd->RubberbandArcMax) {
		rbnd->RubberbandArcMax += STEP_RUBBERBAND;
		ptr = (pcb_rubberband_arc_t *) realloc(ptr, rbnd->RubberbandArcMax * sizeof(pcb_rubberband_arc_t));
		rbnd->RubberbandArcs = ptr;
		memset(ptr + rbnd->RubberbandArcN, 0, STEP_RUBBERBAND * sizeof(pcb_rubberband_arc_t));
	}
	return (ptr + rbnd->RubberbandArcN++);
}

/* ---------------------------------------------------------------------------
 * adds a new line to the rubberband list of 'pcb_crosshair.AttachedObject'
 * if Layer == 0  it is a rat line
 */
static pcb_rubberband_t *pcb_rubber_band_create(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_line_t *Line, int point_number, int delta_index )
{
	pcb_rubberband_t *ptr = pcb_rubber_band_alloc(rbnd);

	point_number &= 1;
	ptr->Layer = Layer;
	ptr->Line = Line;
	ptr->delta_index[point_number] = delta_index;
	ptr->delta_index[point_number^1] = -1;

	return ptr;
}

static pcb_rubberband_arc_t *pcb_rubber_band_create_arc(rubber_ctx_t *rbnd, pcb_layer_t *Layer, pcb_arc_t *Arc, int end)
{
	pcb_rubberband_arc_t *ptr = NULL;
	int n;

	for(n = 0; (n < rbnd->RubberbandArcN) && (ptr == NULL); n++) 
		if (rbnd->RubberbandArcs[n].Arc == Arc) 
			ptr = &rbnd->RubberbandArcs[n];

	if(ptr == NULL) {
		ptr = pcb_rubber_band_arc_alloc(rbnd);
		ptr->Layer = Layer;
		ptr->Arc = Arc;
		ptr->delta_index = (end == 2 ? 1 : 0);
	}
	
	ptr->Ends |= end;
	
	return ptr;
}

/*** event handlers ***/
static void rbe_reset(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	rbnd->RubberbandN = 0;
	rbnd->RubberbandArcN = 0;
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
			pcb_rat_invalidate_erase((pcb_rat_t *) ptr->Line);

		pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, ptr->Line, ptr->Line, ptr->Line);
		ptr++;
	}
}

static void rbe_move(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr = rbnd->Rubberband;
	int direct = argv[1].d.i;


	while (rbnd->RubberbandN) {
		const int dindex1 = ptr->delta_index[0];
		const int dindex2 = ptr->delta_index[1];

		if((dindex1 >= 0) && (dindex2 >= 0) && !direct)	{
			/* Move both ends with route. */
			const int argi1 = (dindex1*2)+2;
			const int argi2 = (dindex2*2)+2;
			pcb_point_t point1 = ptr->Line->Point1;
			pcb_point_t point2 = ptr->Line->Point2;
			pcb_route_t route;

			point1.X += argv[argi1].d.i;
			point1.Y += argv[argi1+1].d.i;
			point2.X += argv[argi2].d.i;
			point2.Y += argv[argi2+1].d.i;

			pcb_route_init(&route);
			pcb_route_calculate(	PCB,
														&route,
														&point1,
														&point2,
														pcb_layer_id(PCB->Data,ptr->Layer),
														ptr->Line->Thickness,
														ptr->Line->Clearance,
														ptr->Line->Flags,
														pcb_gui->shift_is_pressed(),
														pcb_gui->control_is_pressed() );
			pcb_route_apply_to_line(&route,ptr->Layer,ptr->Line);
			pcb_route_destroy(&route);
		}
		else {
			if(dindex1 >= 0) {
				const int argi = (dindex1*2)+2;
				pcb_opctx_t ctx;
				ctx.move.pcb = PCB;
				ctx.move.dx = argv[argi].d.i;
				ctx.move.dy = argv[argi+1].d.i;
	
				if(direct) {
					pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, &ptr->Line->Point1, ctx.move.dx, ctx.move.dy);			
					pcb_lineop_move_point(&ctx, ptr->Layer, ptr->Line, &ptr->Line->Point1);
				}
				else
					pcb_lineop_move_point_with_route(&ctx, ptr->Layer, ptr->Line, &ptr->Line->Point1);
			}

			if(dindex2 >= 0) {
				const int argi = (dindex2*2)+2;
				pcb_opctx_t ctx;
				ctx.move.pcb = PCB;
				ctx.move.dx = argv[argi].d.i;
				ctx.move.dy = argv[argi+1].d.i;
	
				if(direct) {
					pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, &ptr->Line->Point2, ctx.move.dx, ctx.move.dy);			
					pcb_lineop_move_point(&ctx, ptr->Layer, ptr->Line, &ptr->Line->Point2);
				}
				else
					pcb_lineop_move_point_with_route(&ctx, ptr->Layer, ptr->Line, &ptr->Line->Point2);
			}
		}

		rbnd->RubberbandN--;
		ptr++;
	}

	/* TODO: Move rubberband arcs. */
}

static void rbe_draw(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
	pcb_cardinal_t i;
	int direct = argv[1].d.i;

	/* draw the attached rubberband lines too */
	i = rbnd->RubberbandN;
	ptr = rbnd->Rubberband;
	while (i) {

		if (PCB_FLAG_TEST(PCB_FLAG_VIA, ptr->Line)) {
			/* this is a rat going to a polygon.  do not draw for rubberband */ ;
		}
		else {
			pcb_coord_t x[2];
			pcb_coord_t y[2];
			int p;

			x[0] = ptr->Line->Point1.X;
			y[0] = ptr->Line->Point1.Y;
			x[1] = ptr->Line->Point2.X;
			y[1] = ptr->Line->Point2.Y;

			for(p=0;p<2;++p)
			{
				const int dindex = ptr->delta_index[p];

				if((dindex >= 0) ) /*&& (dindex<((argc-2)/2)))*/
				{
					const int argi = (dindex*2)+2;
					x[p] += argv[argi].d.i;
					y[p] += argv[argi+1].d.i;
				}
			}

			if (PCB_FLAG_TEST(PCB_FLAG_RAT, ptr->Line)) {
				pcb_gui->set_color(pcb_crosshair.GC, conf_core.appearance.color.rat);
				pcb_draw_wireframe_line(pcb_crosshair.GC,x[0],y[0],x[1],y[1], ptr->Line->Thickness, 0);
			}
			else if(direct || (conf_core.editor.move_linepoint_uses_route == 0)) {
				pcb_gui->set_color(pcb_crosshair.GC,ptr->Layer->meta.real.color);
				pcb_draw_wireframe_line(pcb_crosshair.GC,x[0],y[0],x[1],y[1], ptr->Line->Thickness, 0);
				/* Draw the DRC outline if it is enabled */
				if (conf_core.editor.show_drc) {
					pcb_gui->set_color(pcb_crosshair.GC, conf_core.appearance.color.cross);
					pcb_draw_wireframe_line(pcb_crosshair.GC,x[0],y[0],x[1],y[1],ptr->Line->Thickness + 2 * (PCB->Bloat + 1), 0);
				}
			}
			else {
				pcb_point_t point1;
				pcb_point_t point2;
				pcb_route_t route;

				point1.X = x[0];
				point1.Y = y[0];
				point2.X = x[1];
				point2.Y = y[1];

				pcb_route_init(&route);
				pcb_route_calculate(	PCB,
															&route,
															ptr->delta_index[0] < 0 ? &point1 : &point2,
															ptr->delta_index[0] < 0 ? &point2 : &point1,
															pcb_layer_id(PCB->Data,ptr->Layer),
															ptr->Line->Thickness,
															ptr->Line->Clearance,
															ptr->Line->Flags,
															pcb_gui->shift_is_pressed(),
															pcb_gui->control_is_pressed() );
				pcb_route_draw(&route,pcb_crosshair.GC);
				if (conf_core.editor.show_drc)
					pcb_route_draw_drc(&route,pcb_crosshair.GC);
				pcb_route_destroy(&route);
			}
			pcb_gui->set_color(pcb_crosshair.GC, conf_core.appearance.color.crosshair);
		}

		ptr++;
		i--;
	}

	/* TODO: 
	 * draw the attached rubberband arcs */
	/*
	if(conf_rbo.plugins.rubberband_orig.enable_rubberband_arcs != 0) {
		pcb_route_t route;
		pcb_route_init(&route);
		i = rbnd->RubberbandArcN;
		arcptr = rbnd->RubberbandArcs;
	
		while (i) {
			calculate_route_rubber_arc_point_move(arcptr,group_dx,group_dy,&route);
			pcb_route_draw(&route,pcb_crosshair.GC);
			pcb_route_reset(&route);
			arcptr++;
			i--;
		}
		pcb_route_destroy(&route);
	}
	*/
}

static void rbe_rotate90(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_rubberband_t *ptr;
	pcb_coord_t cx = argv[5].d.c, cy = argv[6].d.c;
	int steps = argv[7].d.i;
	int *changed = argv[8].d.p;

	/* move all the rubberband lines... and reset the counter */
	ptr = rbnd->Rubberband;
	while (rbnd->RubberbandN) {
		const int dindex1 = ptr->delta_index[0];
		const int dindex2 = ptr->delta_index[1];
		
		*changed = 1;
		
		if(dindex1 >= 0) 
			pcb_undo_add_obj_to_rotate90(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, &ptr->Line->Point1, cx, cy, steps);
		if(dindex2 >= 0) 
			pcb_undo_add_obj_to_rotate90(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, &ptr->Line->Point2, cx, cy, steps);

		pcb_line_invalidate_erase(ptr->Line);
		if (ptr->Layer) {
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			pcb_r_delete_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line);
		}
		else
			pcb_r_delete_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line);

		if(dindex1 >= 0) 
			pcb_point_rotate90(&ptr->Line->Point1, cx, cy, steps);
		
		if(dindex2 >= 0) 
			pcb_point_rotate90(&ptr->Line->Point2, cx, cy, steps);
	
		pcb_line_bbox(ptr->Line);
		if (ptr->Layer) {
			pcb_r_insert_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line);
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			pcb_line_invalidate_draw(ptr->Layer, ptr->Line);
		}
		else {
			pcb_r_insert_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line);
			pcb_rat_invalidate_draw((pcb_rat_t *) ptr->Line);
		}

		rbnd->RubberbandN--;
		ptr++;
	}
}

static void rbe_rotate(void *user_data, int argc, pcb_event_arg_t argv[])
{
#warning TODO
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
				pcb_rat_invalidate_erase((pcb_rat_t *) ptr->Line);
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

static void rbe_constrain_main_line(void *user_data, int argc, pcb_event_arg_t argv[])
{
	rubber_ctx_t *rbnd = user_data;
	pcb_line_t *line = argv[1].d.p;
	int * constrained = argv[2].d.p;
	int * dx1 = argv[3].d.p;	/* in/out */
	int * dy1 = argv[4].d.p;  /* in/out */
	int * dx2 = argv[5].d.p;	/* out */
	int * dy2 = argv[6].d.p;	/* out */
	pcb_line_t *rub1, *rub2;
	int rub1end, rub2end;
	pcb_fline_t fmain, frub1, frub2;
	
	*constrained = 0;

	if (rbnd->RubberbandN != 2)
		return;
			
	rub1 = rbnd->Rubberband[0].Line;
	rub2 = rbnd->Rubberband[1].Line;
	rub1end = rbnd->Rubberband[0].delta_index[0] >= 0 ? 1 : 2;
	rub2end =	rbnd->Rubberband[1].delta_index[0] >= 0 ? 1 : 2;

	/* Create float point-vector representations of the lines */
	fmain = pcb_fline_create_from_points(&line->Point1, &line->Point2);

	if (rub1end == 1)
		frub1 = pcb_fline_create_from_points(&rub1->Point1, &rub1->Point2);
	else
		frub1 = pcb_fline_create_from_points(&rub1->Point2, &rub1->Point1);

	if (rub2end == 1)
		frub2 = pcb_fline_create_from_points(&rub2->Point1, &rub2->Point2);
	else
		frub2 = pcb_fline_create_from_points(&rub2->Point2, &rub2->Point1);

	/* If either of the lines are parallel to the main line then the main line cannot be constrained */
	if( (fabs(pcb_fvector_dot(fmain.direction,frub1.direction)) > 0.990) ||
			(fabs(pcb_fvector_dot(fmain.direction,frub2.direction)) > 0.990) ) 
		return;

	*constrained = 1;

	/* If they are valid (non-null directions) we carry on */
	if (pcb_fline_is_valid(fmain) && pcb_fline_is_valid(frub1) && pcb_fline_is_valid(frub2)) {
		pcb_fvector_t fmove;

			fmove.x = *dx1;
			fmove.y = *dy1;

		if (!pcb_fvector_is_null(fmove)) {
			pcb_fvector_t fnormal;
			double rub1_move, rub2_move;

			fnormal.x = fmain.direction.y;
			fnormal.y = -fmain.direction.x;
			if (pcb_fvector_dot(fnormal, fmove) < 0) {
				fnormal.x = -fnormal.x;
				fnormal.y = -fnormal.y;
			}
			rub1_move = pcb_fvector_dot(fmove, fnormal) / pcb_fvector_dot(frub1.direction, fnormal);
			*dx1 = rub1_move*frub1.direction.x;
			*dy1 = rub1_move*frub1.direction.y;

			rub2_move = pcb_fvector_dot(fmove, fnormal) / pcb_fvector_dot(frub2.direction, fnormal);
			*dx2 = rub2_move*frub2.direction.x;
			*dy2 = rub2_move*frub2.direction.y;
		}
	}
}

static const char *rubber_cookie = "old rubberband";

void rb_uninit(void)
{
	pcb_event_unbind_allcookie(rubber_cookie);
}

int pplg_check_ver_rubberband_orig(int ver_needed) { return 0; }

void pplg_uninit_rubberband_orig(void)
{
	pcb_event_unbind_allcookie(rubber_cookie);
	conf_unreg_fields("plugins/rubberband_orig/");
}

int pplg_init_rubberband_orig(void)
{
	void *ctx = &rubber_band_state;
	pcb_event_bind(PCB_EVENT_RUBBER_RESET, rbe_reset, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_REMOVE_ELEMENT, rbe_remove_element, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_MOVE, rbe_move, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_MOVE_DRAW, rbe_draw, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_ROTATE90, rbe_rotate90, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_ROTATE, rbe_rotate, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_RENAME, rbe_rename, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_LOOKUP_LINES, rbe_lookup_lines, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_LOOKUP_RATS, rbe_lookup_rats, ctx, rubber_cookie);
	pcb_event_bind(PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, rbe_constrain_main_line, ctx, rubber_cookie);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_rbo, field,isarray,type_name,cpath,cname,desc,flags);
#include "rubberband_conf_fields.h"

	return 0;
}

static void	
calculate_route_rubber_arc_point_move(pcb_rubberband_arc_t * arcptr,pcb_coord_t group_dx,pcb_coord_t group_dy,pcb_route_t * route)
{
	/* TODO: Calculate a new route that represents the arc if the endpoint was moved by dx,dy. */
}

