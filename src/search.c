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


/* search routines
 * some of the functions use dummy parameters
 */

#include "config.h"
#include "conf_core.h"

#include <math.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "find.h"
#include "polygon.h"
#include "search.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static double PosX, PosY;				/* search position for subroutines */
static pcb_coord_t SearchRadius;
static pcb_box_t SearchBox;
static pcb_layer_t *SearchLayer;

/* ---------------------------------------------------------------------------
 * some local prototypes.  The first parameter includes PCB_TYPE_LOCKED if we
 * want to include locked types in the search.
 */
static pcb_bool SearchLineByLocation(int, pcb_layer_t **, pcb_line_t **, pcb_line_t **);
static pcb_bool SearchArcByLocation(int, pcb_layer_t **, pcb_arc_t **, pcb_arc_t **);
static pcb_bool SearchRatLineByLocation(int, pcb_rat_t **, pcb_rat_t **, pcb_rat_t **);
static pcb_bool SearchTextByLocation(int, pcb_layer_t **, pcb_text_t **, pcb_text_t **);
static pcb_bool SearchPolygonByLocation(int, pcb_layer_t **, pcb_polygon_t **, pcb_polygon_t **);
static pcb_bool SearchPinByLocation(int, pcb_element_t **, pcb_pin_t **, pcb_pin_t **);
static pcb_bool SearchPadByLocation(int, pcb_element_t **, pcb_pad_t **, pcb_pad_t **, pcb_bool);
static pcb_bool SearchViaByLocation(int, pcb_pin_t **, pcb_pin_t **, pcb_pin_t **);
static pcb_bool SearchElementNameByLocation(int, pcb_element_t **, pcb_text_t **, pcb_text_t **, pcb_bool);
static pcb_bool SearchLinePointByLocation(int, pcb_layer_t **, pcb_line_t **, pcb_point_t **);
static pcb_bool SearchPointByLocation(int, pcb_layer_t **, pcb_polygon_t **, pcb_point_t **);
static pcb_bool SearchElementByLocation(int, pcb_element_t **, pcb_element_t **, pcb_element_t **, pcb_bool);

/* ---------------------------------------------------------------------------
 * searches a via
 */
struct ans_info {
	void **ptr1, **ptr2, **ptr3;
	pcb_bool BackToo;
	double area;
	int locked;										/* This will be zero or PCB_FLAG_LOCK */
};

static pcb_r_dir_t pinorvia_callback(const pcb_box_t * box, void *cl)
{
	struct ans_info *i = (struct ans_info *) cl;
	pcb_pin_t *pin = (pcb_pin_t *) box;
	pcb_any_obj_t *ptr1 = pin->Element ? pin->Element : pin;

	if (PCB_FLAG_TEST(i->locked, ptr1))
		return R_DIR_NOT_FOUND;

	if (!IsPointOnPin(PosX, PosY, SearchRadius, pin))
		return R_DIR_NOT_FOUND;
	*i->ptr1 = ptr1;
	*i->ptr2 = *i->ptr3 = pin;
	return R_DIR_CANCEL; /* found, stop searching */
}

static pcb_bool SearchViaByLocation(int locked, pcb_pin_t ** Via, pcb_pin_t ** Dummy1, pcb_pin_t ** Dummy2)
{
	struct ans_info info;

	/* search only if via-layer is visible */
	if (!PCB->ViaOn)
		return pcb_false;

	info.ptr1 = (void **) Via;
	info.ptr2 = (void **) Dummy1;
	info.ptr3 = (void **) Dummy2;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	if (r_search(PCB->Data->via_tree, &SearchBox, NULL, pinorvia_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * searches a pin
 * starts with the newest element
 */
static pcb_bool SearchPinByLocation(int locked, pcb_element_t ** Element, pcb_pin_t ** Pin, pcb_pin_t ** Dummy)
{
	struct ans_info info;

	/* search only if pin-layer is visible */
	if (!PCB->PinOn)
		return pcb_false;
	info.ptr1 = (void **) Element;
	info.ptr2 = (void **) Pin;
	info.ptr3 = (void **) Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	if (r_search(PCB->Data->pin_tree, &SearchBox, NULL, pinorvia_callback, &info, NULL)  != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

static pcb_r_dir_t pad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct ans_info *i = (struct ans_info *) cl;
	pcb_any_obj_t *ptr1 = pad->Element;

	if (PCB_FLAG_TEST(i->locked, ptr1))
		return R_DIR_NOT_FOUND;

	if (PCB_FRONT(pad) || i->BackToo) {
		if (IsPointInPad(PosX, PosY, SearchRadius, pad)) {
			*i->ptr1 = ptr1;
			*i->ptr2 = *i->ptr3 = pad;
			return R_DIR_CANCEL; /* found */
		}
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches a pad
 * starts with the newest element
 */
static pcb_bool SearchPadByLocation(int locked, pcb_element_t ** Element, pcb_pad_t ** Pad, pcb_pad_t ** Dummy, pcb_bool BackToo)
{
	struct ans_info info;

	/* search only if pin-layer is visible */
	if (!PCB->PinOn)
		return (pcb_false);
	info.ptr1 = (void **) Element;
	info.ptr2 = (void **) Pad;
	info.ptr3 = (void **) Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;
	info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
	if (r_search(PCB->Data->pad_tree, &SearchBox, NULL, pad_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * searches ordinary line on the SearchLayer
 */

struct line_info {
	pcb_line_t **Line;
	pcb_point_t **Point;
	double least;
	int locked;
};

static pcb_r_dir_t line_callback(const pcb_box_t * box, void *cl)
{
	struct line_info *i = (struct line_info *) cl;
	pcb_line_t *l = (pcb_line_t *) box;

	if (PCB_FLAG_TEST(i->locked, l))
		return R_DIR_NOT_FOUND;

	if (!IsPointInPad(PosX, PosY, SearchRadius, (pcb_pad_t *) l))
		return R_DIR_NOT_FOUND;
	*i->Line = l;
	*i->Point = (pcb_point_t *) l;

	return R_DIR_CANCEL; /* found what we were looking for */
}


static pcb_bool SearchLineByLocation(int locked, pcb_layer_t ** Layer, pcb_line_t ** Line, pcb_line_t ** Dummy)
{
	struct line_info info;

	info.Line = Line;
	info.Point = (pcb_point_t **) Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	*Layer = SearchLayer;
	if (r_search(SearchLayer->line_tree, &SearchBox, NULL, line_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;

	return pcb_false;
}

static pcb_r_dir_t rat_callback(const pcb_box_t * box, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	if (PCB_FLAG_TEST(i->locked, line))
		return R_DIR_NOT_FOUND;

	if (PCB_FLAG_TEST(PCB_FLAG_VIA, line) ?
			(pcb_distance(line->Point1.X, line->Point1.Y, PosX, PosY) <=
			 line->Thickness * 2 + SearchRadius) : IsPointOnLine(PosX, PosY, SearchRadius, line)) {
		*i->ptr1 = *i->ptr2 = *i->ptr3 = line;
		return R_DIR_CANCEL;
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches rat lines if they are visible
 */
static pcb_bool SearchRatLineByLocation(int locked, pcb_rat_t ** Line, pcb_rat_t ** Dummy1, pcb_rat_t ** Dummy2)
{
	struct ans_info info;

	info.ptr1 = (void **) Line;
	info.ptr2 = (void **) Dummy1;
	info.ptr3 = (void **) Dummy2;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	if (r_search(PCB->Data->rat_tree, &SearchBox, NULL, rat_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * searches arc on the SearchLayer
 */
struct arc_info {
	pcb_arc_t **Arc, **Dummy;
	int locked;
};

static pcb_r_dir_t arc_callback(const pcb_box_t * box, void *cl)
{
	struct arc_info *i = (struct arc_info *) cl;
	pcb_arc_t *a = (pcb_arc_t *) box;

	if (PCB_FLAG_TEST(i->locked, a))
		return R_DIR_NOT_FOUND;

	if (!IsPointOnArc(PosX, PosY, SearchRadius, a))
		return 0;
	*i->Arc = a;
	*i->Dummy = a;
	return R_DIR_CANCEL; /* found */
}


static pcb_bool SearchArcByLocation(int locked, pcb_layer_t ** Layer, pcb_arc_t ** Arc, pcb_arc_t ** Dummy)
{
	struct arc_info info;

	info.Arc = Arc;
	info.Dummy = Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	*Layer = SearchLayer;
	if (r_search(SearchLayer->arc_tree, &SearchBox, NULL, arc_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

static pcb_r_dir_t text_callback(const pcb_box_t * box, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	if (PCB_FLAG_TEST(i->locked, text))
		return R_DIR_NOT_FOUND;

	if (POINT_IN_BOX(PosX, PosY, &text->BoundingBox)) {
		*i->ptr2 = *i->ptr3 = text;
		return R_DIR_CANCEL; /* found */
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches text on the SearchLayer
 */
static pcb_bool SearchTextByLocation(int locked, pcb_layer_t ** Layer, pcb_text_t ** Text, pcb_text_t ** Dummy)
{
	struct ans_info info;

	*Layer = SearchLayer;
	info.ptr2 = (void **) Text;
	info.ptr3 = (void **) Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	if (r_search(SearchLayer->text_tree, &SearchBox, NULL, text_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

static pcb_r_dir_t polygon_callback(const pcb_box_t * box, void *cl)
{
	pcb_polygon_t *polygon = (pcb_polygon_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	if (PCB_FLAG_TEST(i->locked, polygon))
		return R_DIR_NOT_FOUND;

	if (IsPointInPolygon(PosX, PosY, SearchRadius, polygon)) {
		*i->ptr2 = *i->ptr3 = polygon;
		return R_DIR_CANCEL; /* found */
	}
	return R_DIR_NOT_FOUND;
}


/* ---------------------------------------------------------------------------
 * searches a polygon on the SearchLayer
 */
static pcb_bool SearchPolygonByLocation(int locked, pcb_layer_t ** Layer, pcb_polygon_t ** Polygon, pcb_polygon_t ** Dummy)
{
	struct ans_info info;

	*Layer = SearchLayer;
	info.ptr2 = (void **) Polygon;
	info.ptr3 = (void **) Dummy;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;

	if (r_search(SearchLayer->polygon_tree, &SearchBox, NULL, polygon_callback, &info, NULL) != R_DIR_NOT_FOUND)
		return pcb_true;
	return pcb_false;
}

static pcb_r_dir_t linepoint_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct line_info *i = (struct line_info *) cl;
	pcb_r_dir_t ret_val = R_DIR_NOT_FOUND;
	double d;

	if (PCB_FLAG_TEST(i->locked, line))
		return R_DIR_NOT_FOUND;

	/* some stupid code to check both points */
	d = pcb_distance(PosX, PosY, line->Point1.X, line->Point1.Y);
	if (d < i->least) {
		i->least = d;
		*i->Line = line;
		*i->Point = &line->Point1;
		ret_val = R_DIR_FOUND_CONTINUE;
	}

	d = pcb_distance(PosX, PosY, line->Point2.X, line->Point2.Y);
	if (d < i->least) {
		i->least = d;
		*i->Line = line;
		*i->Point = &line->Point2;
		ret_val = R_DIR_FOUND_CONTINUE;
	}
	return ret_val;
}

/* ---------------------------------------------------------------------------
 * searches a line-point on all the search layer
 */
static pcb_bool SearchLinePointByLocation(int locked, pcb_layer_t ** Layer, pcb_line_t ** Line, pcb_point_t ** Point)
{
	struct line_info info;
	*Layer = SearchLayer;
	info.Line = Line;
	info.Point = Point;
	*Point = NULL;
	info.least = MAX_LINE_POINT_DISTANCE + SearchRadius;
	info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;
	if (r_search(SearchLayer->line_tree, &SearchBox, NULL, linepoint_callback, &info, NULL))
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * searches a polygon-point on all layers that are switched on
 * in layerstack order
 */
static pcb_bool SearchPointByLocation(int locked, pcb_layer_t ** Layer, pcb_polygon_t ** Polygon, pcb_point_t ** Point)
{
	double d, least;
	pcb_bool found = pcb_false;

	least = SearchRadius + MAX_POLYGON_POINT_DISTANCE;
	*Layer = SearchLayer;
	POLYGON_LOOP(*Layer);
	{
		POLYGONPOINT_LOOP(polygon);
		{
			d = pcb_distance(point->X, point->Y, PosX, PosY);
			if (d < least) {
				least = d;
				*Polygon = polygon;
				*Point = point;
				found = pcb_true;
			}
		}
		END_LOOP;
	}
	END_LOOP;
	if (found)
		return (pcb_true);
	return (pcb_false);
}

static pcb_r_dir_t name_callback(const pcb_box_t * box, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) box;
	struct ans_info *i = (struct ans_info *) cl;
	pcb_element_t *element = (pcb_element_t *) text->Element;
	double newarea;

	if (PCB_FLAG_TEST(i->locked, text))
		return R_DIR_NOT_FOUND;

	if ((PCB_FRONT(element) || i->BackToo) && !PCB_FLAG_TEST(PCB_FLAG_HIDENAME, element) && POINT_IN_BOX(PosX, PosY, &text->BoundingBox)) {
		/* use the text with the smallest bounding box */
		newarea = (text->BoundingBox.X2 - text->BoundingBox.X1) * (double) (text->BoundingBox.Y2 - text->BoundingBox.Y1);
		if (newarea < i->area) {
			i->area = newarea;
			*i->ptr1 = element;
			*i->ptr2 = *i->ptr3 = text;
		}
		return R_DIR_FOUND_CONTINUE;
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches the name of an element
 * the search starts with the last element and goes back to the beginning
 */
static pcb_bool
SearchElementNameByLocation(int locked, pcb_element_t ** Element, pcb_text_t ** Text, pcb_text_t ** Dummy, pcb_bool BackToo)
{
	struct ans_info info;

	/* package layer have to be switched on */
	if (PCB->ElementOn) {
		info.ptr1 = (void **) Element;
		info.ptr2 = (void **) Text;
		info.ptr3 = (void **) Dummy;
		info.area = PCB_SQUARE(MAX_COORD);
		info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
		info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;
		if (r_search(PCB->Data->name_tree[NAME_INDEX()], &SearchBox, NULL, name_callback, &info, NULL))
			return pcb_true;
	}
	return (pcb_false);
}

static pcb_r_dir_t element_callback(const pcb_box_t * box, void *cl)
{
	pcb_element_t *element = (pcb_element_t *) box;
	struct ans_info *i = (struct ans_info *) cl;
	double newarea;

	if (PCB_FLAG_TEST(i->locked, element))
		return R_DIR_NOT_FOUND;

	if ((PCB_FRONT(element) || i->BackToo) && POINT_IN_BOX(PosX, PosY, &element->VBox)) {
		/* use the element with the smallest bounding box */
		newarea = (element->VBox.X2 - element->VBox.X1) * (double) (element->VBox.Y2 - element->VBox.Y1);
		if (newarea < i->area) {
			i->area = newarea;
			*i->ptr1 = *i->ptr2 = *i->ptr3 = element;
			return R_DIR_FOUND_CONTINUE;
		}
	}
	return R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches an element
 * the search starts with the last element and goes back to the beginning
 * if more than one element matches, the smallest one is taken
 */
static pcb_bool
SearchElementByLocation(int locked, pcb_element_t ** Element, pcb_element_t ** Dummy1, pcb_element_t ** Dummy2, pcb_bool BackToo)
{
	struct ans_info info;

	/* Both package layers have to be switched on */
	if (PCB->ElementOn && PCB->PinOn) {
		info.ptr1 = (void **) Element;
		info.ptr2 = (void **) Dummy1;
		info.ptr3 = (void **) Dummy2;
		info.area = PCB_SQUARE(MAX_COORD);
		info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
		info.locked = (locked & PCB_TYPE_LOCKED) ? 0 : PCB_FLAG_LOCK;
		if (r_search(PCB->Data->element_tree, &SearchBox, NULL, element_callback, &info, NULL))
			return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * checks if a point is on a pin
 */
pcb_bool IsPointOnPin(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_pin_t *pin)
{
	pcb_coord_t t = PIN_SIZE(pin) / 2;
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
		pcb_box_t b;

		b.X1 = pin->X - t;
		b.X2 = pin->X + t;
		b.Y1 = pin->Y - t;
		b.Y2 = pin->Y + t;
		if (IsPointInBox(X, Y, &b, Radius))
			return pcb_true;
	}
	else if (pcb_distance(pin->X, pin->Y, X, Y) <= Radius + t)
		return pcb_true;
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * checks if a rat-line end is on a PV
 */
pcb_bool IsPointOnLineEnd(pcb_coord_t X, pcb_coord_t Y, pcb_rat_t *Line)
{
	if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) || ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
		return (pcb_true);
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * checks if a line intersects with a PV
 *
 * let the point be (X,Y) and the line (X1,Y1)(X2,Y2)
 * the length of the line is
 *
 *   L = ((X2-X1)^2 + (Y2-Y1)^2)^0.5
 *
 * let Q be the point of perpendicular projection of (X,Y) onto the line
 *
 *   QX = X1 + D1*(X2-X1) / L
 *   QY = Y1 + D1*(Y2-Y1) / L
 *
 * with (from vector geometry)
 *
 *        (Y1-Y)(Y1-Y2)+(X1-X)(X1-X2)
 *   D1 = ---------------------------
 *                     L
 *
 *   D1 < 0   Q is on backward extension of the line
 *   D1 > L   Q is on forward extension of the line
 *   else     Q is on the line
 *
 * the signed distance from (X,Y) to Q is
 *
 *        (Y2-Y1)(X-X1)-(X2-X1)(Y-Y1)
 *   D2 = ----------------------------
 *                     L
 *
 * Finally, D1 and D2 are orthogonal, so we can sum them easily
 * by Pythagorean theorem.
 */
pcb_bool IsPointOnLine(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_line_t *Line)
{
	double D1, D2, L;

	/* Get length of segment */
	L = pcb_distance(Line->Point1.X, Line->Point1.Y, Line->Point2.X, Line->Point2.Y);
	if (L < 0.1)
		return pcb_distance(X, Y, Line->Point1.X, Line->Point1.Y) < Radius + Line->Thickness / 2;

	/* Get distance from (X1, Y1) to Q (on the line) */
	D1 = ((double) (Y - Line->Point1.Y) * (Line->Point2.Y - Line->Point1.Y)
				+ (double) (X - Line->Point1.X) * (Line->Point2.X - Line->Point1.X)) / L;
	/* Translate this into distance to Q from segment */
	if (D1 < 0)
		D1 = -D1;
	else if (D1 > L)
		D1 -= L;
	else
		D1 = 0;
	/* Get distance from (X, Y) to Q */
	D2 = ((double) (X - Line->Point1.X) * (Line->Point2.Y - Line->Point1.Y)
				- (double) (Y - Line->Point1.Y) * (Line->Point2.X - Line->Point1.X)) / L;
	/* Total distance is then the Pythagorean sum of these */
	return sqrt(D1 * D1 + D2 * D2) <= Radius + Line->Thickness / 2;
}

static int is_point_on_line(pcb_coord_t px, pcb_coord_t py, pcb_coord_t lx1, pcb_coord_t ly1, pcb_coord_t lx2, pcb_coord_t ly2)
{
	/* ohh well... let's hope the optimizer does something clever with inlining... */
	pcb_line_t l;
	l.Point1.X = lx1;
	l.Point1.Y = ly1;
	l.Point2.X = lx2;
	l.Point2.Y = ly2;
	l.Thickness = 1;
	return IsPointOnLine(px, py, 1, &l);
}

/* ---------------------------------------------------------------------------
 * checks if a line crosses a rectangle
 */
pcb_bool IsLineInRectangle(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_line_t *Line)
{
	pcb_line_t line;

	/* first, see if point 1 is inside the rectangle */
	/* in case the whole line is inside the rectangle */
	if (X1 < Line->Point1.X && X2 > Line->Point1.X && Y1 < Line->Point1.Y && Y2 > Line->Point1.Y)
		return (pcb_true);
	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.Y = line.Point2.Y = Y1;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* upper-right to lower-right corner */
	line.Point1.X = X2;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* lower-right to lower-left corner */
	line.Point1.Y = Y2;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* lower-left to upper-left corner */
	line.Point2.X = X1;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	return (pcb_false);
}

static int /*checks if a point (of null radius) is in a slanted rectangle */ IsPointInQuadrangle(pcb_point_t p[4], pcb_point_t *l)
{
	pcb_coord_t dx, dy, x, y;
	double prod0, prod1;

	dx = p[1].X - p[0].X;
	dy = p[1].Y - p[0].Y;
	x = l->X - p[0].X;
	y = l->Y - p[0].Y;
	prod0 = (double) x *dx + (double) y *dy;
	x = l->X - p[1].X;
	y = l->Y - p[1].Y;
	prod1 = (double) x *dx + (double) y *dy;
	if (prod0 * prod1 <= 0) {
		dx = p[1].X - p[2].X;
		dy = p[1].Y - p[2].Y;
		prod0 = (double) x *dx + (double) y *dy;
		x = l->X - p[2].X;
		y = l->Y - p[2].Y;
		prod1 = (double) x *dx + (double) y *dy;
		if (prod0 * prod1 <= 0)
			return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * checks if a line crosses a quadrangle: almost copied from IsLineInRectangle()
 * Note: actually this quadrangle is a slanted rectangle
 */
pcb_bool IsLineInQuadrangle(pcb_point_t p[4], pcb_line_t *Line)
{
	pcb_line_t line;

	/* first, see if point 1 is inside the rectangle */
	/* in case the whole line is inside the rectangle */
	if (IsPointInQuadrangle(p, &(Line->Point1)))
		return pcb_true;
	if (IsPointInQuadrangle(p, &(Line->Point2)))
		return pcb_true;
	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.X = p[0].X;
	line.Point1.Y = p[0].Y;
	line.Point2.X = p[1].X;
	line.Point2.Y = p[1].Y;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* upper-right to lower-right corner */
	line.Point1.X = p[2].X;
	line.Point1.Y = p[2].Y;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* lower-right to lower-left corner */
	line.Point2.X = p[3].X;
	line.Point2.Y = p[3].Y;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	/* lower-left to upper-left corner */
	line.Point1.X = p[0].X;
	line.Point1.Y = p[0].Y;
	if (pcb_intersect_line_line(&line, Line))
		return (pcb_true);

	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * checks if an arc crosses a square
 */
pcb_bool IsArcInRectangle(pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_arc_t *Arc)
{
	pcb_line_t line;

	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.Y = line.Point2.Y = Y1;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_intersect_line_arc(&line, Arc))
		return (pcb_true);

	/* upper-right to lower-right corner */
	line.Point1.X = line.Point2.X = X2;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_intersect_line_arc(&line, Arc))
		return (pcb_true);

	/* lower-right to lower-left corner */
	line.Point1.Y = line.Point2.Y = Y2;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_intersect_line_arc(&line, Arc))
		return (pcb_true);

	/* lower-left to upper-left corner */
	line.Point1.X = line.Point2.X = X1;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_intersect_line_arc(&line, Arc))
		return (pcb_true);

	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * Check if a circle of Radius with center at (X, Y) intersects a Pad.
 * Written to enable arbitrary pad directions; for rounded pads, too.
 */
pcb_bool IsPointInPad(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_pad_t *Pad)
{
	double r, Sin, Cos;
	pcb_coord_t x;

	/* Also used from line_callback with line type smaller than pad type;
	   use the smallest common subset; ->Thickness is still ok. */
	pcb_coord_t t2 = (Pad->Thickness + 1) / 2, range;
	pcb_any_line_t pad = *(pcb_any_line_t *) Pad;


	/* series of transforms saving range */
	/* move Point1 to the origin */
	X -= pad.Point1.X;
	Y -= pad.Point1.Y;

	pad.Point2.X -= pad.Point1.X;
	pad.Point2.Y -= pad.Point1.Y;
	/* so, pad.Point1.X = pad.Point1.Y = 0; */

	/* rotate round (0, 0) so that Point2 coordinates be (r, 0) */
	r = pcb_distance(0, 0, pad.Point2.X, pad.Point2.Y);
	if (r < .1) {
		Cos = 1;
		Sin = 0;
	}
	else {
		Sin = pad.Point2.Y / r;
		Cos = pad.Point2.X / r;
	}
	x = X;
	X = X * Cos + Y * Sin;
	Y = Y * Cos - x * Sin;
	/* now pad.Point2.X = r; pad.Point2.Y = 0; */

	/* take into account the ends */
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad)) {
		r += Pad->Thickness;
		X += t2;
	}
	if (Y < 0)
		Y = -Y;											/* range value is evident now */

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad)) {
		if (X <= 0) {
			if (Y <= t2)
				range = -X;
			else
				return Radius > pcb_distance(0, t2, X, Y);
		}
		else if (X >= r) {
			if (Y <= t2)
				range = X - r;
			else
				return Radius > pcb_distance(r, t2, X, Y);
		}
		else
			range = Y - t2;
	}
	else {												/*Rounded pad: even more simple */

		if (X <= 0)
			return (Radius + t2) > pcb_distance(0, 0, X, Y);
		else if (X >= r)
			return (Radius + t2) > pcb_distance(r, 0, X, Y);
		else
			range = Y - t2;
	}
	return range < Radius;
}

pcb_bool IsPointInBox(pcb_coord_t X, pcb_coord_t Y, pcb_box_t *box, pcb_coord_t Radius)
{
	pcb_coord_t width, height, range;

	/* NB: Assumes box has point1 with numerically lower X and Y coordinates */

	/* Compute coordinates relative to Point1 */
	X -= box->X1;
	Y -= box->Y1;

	width = box->X2 - box->X1;
	height = box->Y2 - box->Y1;

	if (X <= 0) {
		if (Y < 0)
			return Radius > pcb_distance(0, 0, X, Y);
		else if (Y > height)
			return Radius > pcb_distance(0, height, X, Y);
		else
			range = -X;
	}
	else if (X >= width) {
		if (Y < 0)
			return Radius > pcb_distance(width, 0, X, Y);
		else if (Y > height)
			return Radius > pcb_distance(width, height, X, Y);
		else
			range = X - width;
	}
	else {
		if (Y < 0)
			range = -Y;
		else if (Y > height)
			range = Y - height;
		else
			return pcb_true;
	}

	return range < Radius;
}

/* TODO: this code is BROKEN in the case of non-circular arcs,
 *       and in the case that the arc thickness is greater than
 *       the radius.
 */
pcb_bool IsPointOnArc(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius, pcb_arc_t *Arc)
{
	/* Calculate angle of point from arc center */
	double p_dist = pcb_distance(X, Y, Arc->X, Arc->Y);
	double p_cos = (X - Arc->X) / p_dist;
	pcb_angle_t p_ang = acos(p_cos) * PCB_RAD_TO_DEG;
	pcb_angle_t ang1, ang2;

	/* Convert StartAngle, Delta into bounding angles in [0, 720) */
	if (Arc->Delta > 0) {
		ang1 = NormalizeAngle(Arc->StartAngle);
		ang2 = NormalizeAngle(Arc->StartAngle + Arc->Delta);
	}
	else {
		ang1 = NormalizeAngle(Arc->StartAngle + Arc->Delta);
		ang2 = NormalizeAngle(Arc->StartAngle);
	}
	if (ang1 > ang2)
		ang2 += 360;
	/* Make sure full circles aren't treated as zero-length arcs */
	if (Arc->Delta == 360 || Arc->Delta == -360)
		ang2 = ang1 + 360;

	if (Y > Arc->Y)
		p_ang = -p_ang;
	p_ang += 180;

	/* Check point is outside arc range, check distance from endpoints */
	if (ang1 >= p_ang || ang2 <= p_ang) {
		pcb_coord_t ArcX, ArcY;

		ArcX = Arc->X + Arc->Width * cos((Arc->StartAngle + 180) / PCB_RAD_TO_DEG);
		ArcY = Arc->Y - Arc->Width * sin((Arc->StartAngle + 180) / PCB_RAD_TO_DEG);
		if (pcb_distance(X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
			return pcb_true;

		ArcX = Arc->X + Arc->Width * cos((Arc->StartAngle + Arc->Delta + 180) / PCB_RAD_TO_DEG);
		ArcY = Arc->Y - Arc->Width * sin((Arc->StartAngle + Arc->Delta + 180) / PCB_RAD_TO_DEG);
		if (pcb_distance(X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
			return pcb_true;
		return pcb_false;
	}
	/* If point is inside the arc range, just compare it to the arc */
	return fabs(pcb_distance(X, Y, Arc->X, Arc->Y) - Arc->Width) < Radius + Arc->Thickness / 2;
}

/* ---------------------------------------------------------------------------
 * searches for any kind of object or for a set of object types
 * the calling routine passes two pointers to allocated memory for storing
 * the results.
 * A type value is returned too which is PCB_TYPE_NONE if no objects has been found.
 * A set of object types is passed in.
 * The object is located by it's position.
 *
 * The layout is checked in the following order:
 *   polygon-point, pin, via, line, text, elementname, polygon, element
 *
 * Note that if Type includes PCB_TYPE_LOCKED, then the search includes
 * locked items.  Otherwise, locked items are ignored.
 */
int SearchObjectByLocation(unsigned Type, void **Result1, void **Result2, void **Result3, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Radius)
{
	void *r1, *r2, *r3;
	void **pr1 = &r1, **pr2 = &r2, **pr3 = &r3;
	int i;
	double HigherBound = 0;
	int HigherAvail = PCB_TYPE_NONE;
	int locked = Type & PCB_TYPE_LOCKED;
	/* setup variables used by local functions */
	PosX = X;
	PosY = Y;
	SearchRadius = Radius;
	if (Radius) {
		SearchBox.X1 = X - Radius;
		SearchBox.Y1 = Y - Radius;
		SearchBox.X2 = X + Radius;
		SearchBox.Y2 = Y + Radius;
	}
	else {
		SearchBox = pcb_point_box(X, Y);
	}

	if (conf_core.editor.lock_names) {
		Type &= ~(PCB_TYPE_ELEMENT_NAME | PCB_TYPE_TEXT);
	}
	if (conf_core.editor.hide_names) {
		Type &= ~PCB_TYPE_ELEMENT_NAME;
	}
	if (conf_core.editor.only_names) {
		Type &= (PCB_TYPE_ELEMENT_NAME | PCB_TYPE_TEXT);
	}
	if (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly) {
		Type &= ~PCB_TYPE_POLYGON;
	}

	if (Type & PCB_TYPE_RATLINE && PCB->RatOn &&
			SearchRatLineByLocation(locked, (pcb_rat_t **) Result1, (pcb_rat_t **) Result2, (pcb_rat_t **) Result3))
		return (PCB_TYPE_RATLINE);

	if (Type & PCB_TYPE_VIA && SearchViaByLocation(locked, (pcb_pin_t **) Result1, (pcb_pin_t **) Result2, (pcb_pin_t **) Result3))
		return (PCB_TYPE_VIA);

	if (Type & PCB_TYPE_PIN && SearchPinByLocation(locked, (pcb_element_t **) pr1, (pcb_pin_t **) pr2, (pcb_pin_t **) pr3))
		HigherAvail = PCB_TYPE_PIN;

	if (!HigherAvail && Type & PCB_TYPE_PAD &&
			SearchPadByLocation(locked, (pcb_element_t **) pr1, (pcb_pad_t **) pr2, (pcb_pad_t **) pr3, pcb_false))
		HigherAvail = PCB_TYPE_PAD;

	if (!HigherAvail && Type & PCB_TYPE_ELEMENT_NAME &&
			SearchElementNameByLocation(locked, (pcb_element_t **) pr1, (pcb_text_t **) pr2, (pcb_text_t **) pr3, pcb_false)) {
		pcb_box_t *box = &((pcb_text_t *) r2)->BoundingBox;
		HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
		HigherAvail = PCB_TYPE_ELEMENT_NAME;
	}

	if (!HigherAvail && Type & PCB_TYPE_ELEMENT &&
			SearchElementByLocation(locked, (pcb_element_t **) pr1, (pcb_element_t **) pr2, (pcb_element_t **) pr3, pcb_false)) {
		pcb_box_t *box = &((pcb_element_t *) r1)->BoundingBox;
		HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
		HigherAvail = PCB_TYPE_ELEMENT;
	}

	for (i = -1; i < max_copper_layer + 1; i++) {
		if (i < 0)
			SearchLayer = &PCB->Data->SILKLAYER;
		else if (i < max_copper_layer)
			SearchLayer = LAYER_ON_STACK(i);
		else {
			SearchLayer = &PCB->Data->BACKSILKLAYER;
			if (!PCB->InvisibleObjectsOn)
				continue;
		}
		if (SearchLayer->On) {
			if ((HigherAvail & (PCB_TYPE_PIN | PCB_TYPE_PAD)) == 0 &&
					Type & PCB_TYPE_POLYGON_POINT &&
					SearchPointByLocation(locked, (pcb_layer_t **) Result1, (pcb_polygon_t **) Result2, (pcb_point_t **) Result3))
				return (PCB_TYPE_POLYGON_POINT);

			if ((HigherAvail & (PCB_TYPE_PIN | PCB_TYPE_PAD)) == 0 &&
					Type & PCB_TYPE_LINE_POINT &&
					SearchLinePointByLocation(locked, (pcb_layer_t **) Result1, (pcb_line_t **) Result2, (pcb_point_t **) Result3))
				return (PCB_TYPE_LINE_POINT);

			if ((HigherAvail & (PCB_TYPE_PIN | PCB_TYPE_PAD)) == 0 && Type & PCB_TYPE_LINE
					&& SearchLineByLocation(locked, (pcb_layer_t **) Result1, (pcb_line_t **) Result2, (pcb_line_t **) Result3))
				return (PCB_TYPE_LINE);

			if ((HigherAvail & (PCB_TYPE_PIN | PCB_TYPE_PAD)) == 0 && Type & PCB_TYPE_ARC &&
					SearchArcByLocation(locked, (pcb_layer_t **) Result1, (pcb_arc_t **) Result2, (pcb_arc_t **) Result3))
				return (PCB_TYPE_ARC);

			if ((HigherAvail & (PCB_TYPE_PIN | PCB_TYPE_PAD)) == 0 && Type & PCB_TYPE_TEXT
					&& SearchTextByLocation(locked, (pcb_layer_t **) Result1, (pcb_text_t **) Result2, (pcb_text_t **) Result3))
				return (PCB_TYPE_TEXT);

			if (Type & PCB_TYPE_POLYGON &&
					SearchPolygonByLocation(locked, (pcb_layer_t **) Result1, (pcb_polygon_t **) Result2, (pcb_polygon_t **) Result3)) {
				if (HigherAvail) {
					pcb_box_t *box = &(*(pcb_polygon_t **) Result2)->BoundingBox;
					double area = (double) (box->X2 - box->X1) * (double) (box->X2 - box->X1);
					if (HigherBound < area)
						break;
					else
						return (PCB_TYPE_POLYGON);
				}
				else
					return (PCB_TYPE_POLYGON);
			}
		}
	}
	/* return any previously found objects */
	if (HigherAvail & PCB_TYPE_PIN) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return (PCB_TYPE_PIN);
	}

	if (HigherAvail & PCB_TYPE_PAD) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return (PCB_TYPE_PAD);
	}

	if (HigherAvail & PCB_TYPE_ELEMENT_NAME) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return (PCB_TYPE_ELEMENT_NAME);
	}

	if (HigherAvail & PCB_TYPE_ELEMENT) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return (PCB_TYPE_ELEMENT);
	}

	/* search the 'invisible objects' last */
	if (!PCB->InvisibleObjectsOn)
		return (PCB_TYPE_NONE);

	if (Type & PCB_TYPE_PAD &&
			SearchPadByLocation(locked, (pcb_element_t **) Result1, (pcb_pad_t **) Result2, (pcb_pad_t **) Result3, pcb_true))
		return (PCB_TYPE_PAD);

	if (Type & PCB_TYPE_ELEMENT_NAME &&
			SearchElementNameByLocation(locked, (pcb_element_t **) Result1, (pcb_text_t **) Result2, (pcb_text_t **) Result3, pcb_true))
		return (PCB_TYPE_ELEMENT_NAME);

	if (Type & PCB_TYPE_ELEMENT &&
			SearchElementByLocation(locked, (pcb_element_t **) Result1, (pcb_element_t **) Result2, (pcb_element_t **) Result3, pcb_true))
		return (PCB_TYPE_ELEMENT);

	return (PCB_TYPE_NONE);
}

/* ---------------------------------------------------------------------------
 * searches for a object by it's unique ID. It doesn't matter if
 * the object is visible or not. The search is performed on a PCB, a
 * buffer or on the remove list.
 * The calling routine passes two pointers to allocated memory for storing
 * the results.
 * A type value is returned too which is PCB_TYPE_NONE if no objects has been found.
 */
int SearchObjectByID(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type)
{
	if (type == PCB_TYPE_LINE || type == PCB_TYPE_LINE_POINT) {
		ALLLINE_LOOP(Base);
		{
			if (line->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) line;
				return (PCB_TYPE_LINE);
			}
			if (line->Point1.ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point1;
				return (PCB_TYPE_LINE_POINT);
			}
			if (line->Point2.ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point2;
				return (PCB_TYPE_LINE_POINT);
			}
		}
		ENDALL_LOOP;
	}
	if (type == PCB_TYPE_ARC) {
		PCB_ARC_ALL_LOOP(Base);
		{
			if (arc->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) arc;
				return (PCB_TYPE_ARC);
			}
		}
		ENDALL_LOOP;
	}

	if (type == PCB_TYPE_TEXT) {
		ALLTEXT_LOOP(Base);
		{
			if (text->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) text;
				return (PCB_TYPE_TEXT);
			}
		}
		ENDALL_LOOP;
	}

	if (type == PCB_TYPE_POLYGON || type == PCB_TYPE_POLYGON_POINT) {
		ALLPOLYGON_LOOP(Base);
		{
			if (polygon->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) polygon;
				return (PCB_TYPE_POLYGON);
			}
			if (type == PCB_TYPE_POLYGON_POINT)
				POLYGONPOINT_LOOP(polygon);
			{
				if (point->ID == ID) {
					*Result1 = (void *) layer;
					*Result2 = (void *) polygon;
					*Result3 = (void *) point;
					return (PCB_TYPE_POLYGON_POINT);
				}
			}
			END_LOOP;
		}
		ENDALL_LOOP;
	}
	if (type == PCB_TYPE_VIA) {
		VIA_LOOP(Base);
		{
			if (via->ID == ID) {
				*Result1 = *Result2 = *Result3 = (void *) via;
				return (PCB_TYPE_VIA);
			}
		}
		END_LOOP;
	}

	if (type == PCB_TYPE_RATLINE || type == PCB_TYPE_LINE_POINT) {
		RAT_LOOP(Base);
		{
			if (line->ID == ID) {
				*Result1 = *Result2 = *Result3 = (void *) line;
				return (PCB_TYPE_RATLINE);
			}
			if (line->Point1.ID == ID) {
				*Result1 = (void *) NULL;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point1;
				return (PCB_TYPE_LINE_POINT);
			}
			if (line->Point2.ID == ID) {
				*Result1 = (void *) NULL;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point2;
				return (PCB_TYPE_LINE_POINT);
			}
		}
		END_LOOP;
	}

	if (type == PCB_TYPE_ELEMENT || type == PCB_TYPE_PAD || type == PCB_TYPE_PIN
			|| type == PCB_TYPE_ELEMENT_LINE || type == PCB_TYPE_ELEMENT_NAME || type == PCB_TYPE_ELEMENT_ARC)
		/* check pins and elementnames too */
		ELEMENT_LOOP(Base);
	{
		if (element->ID == ID) {
			*Result1 = *Result2 = *Result3 = (void *) element;
			return (PCB_TYPE_ELEMENT);
		}
		if (type == PCB_TYPE_ELEMENT_LINE)
			ELEMENTLINE_LOOP(element);
		{
			if (line->ID == ID) {
				*Result1 = (void *) element;
				*Result2 = *Result3 = (void *) line;
				return (PCB_TYPE_ELEMENT_LINE);
			}
		}
		END_LOOP;
		if (type == PCB_TYPE_ELEMENT_ARC)
			PCB_ARC_LOOP(element);
		{
			if (arc->ID == ID) {
				*Result1 = (void *) element;
				*Result2 = *Result3 = (void *) arc;
				return (PCB_TYPE_ELEMENT_ARC);
			}
		}
		END_LOOP;
		if (type == PCB_TYPE_ELEMENT_NAME)
			ELEMENTTEXT_LOOP(element);
		{
			if (text->ID == ID) {
				*Result1 = (void *) element;
				*Result2 = *Result3 = (void *) text;
				return (PCB_TYPE_ELEMENT_NAME);
			}
		}
		END_LOOP;
		if (type == PCB_TYPE_PIN)
			PIN_LOOP(element);
		{
			if (pin->ID == ID) {
				*Result1 = (void *) element;
				*Result2 = *Result3 = (void *) pin;
				return (PCB_TYPE_PIN);
			}
		}
		END_LOOP;
		if (type == PCB_TYPE_PAD)
			PAD_LOOP(element);
		{
			if (pad->ID == ID) {
				*Result1 = (void *) element;
				*Result2 = *Result3 = (void *) pad;
				return (PCB_TYPE_PAD);
			}
		}
		END_LOOP;
	}
	END_LOOP;

	pcb_message(PCB_MSG_DEFAULT, "hace: Internal error, search for ID %d failed\n", ID);
	return (PCB_TYPE_NONE);
}

/* ---------------------------------------------------------------------------
 * searches for an element by its board name.
 * The function returns a pointer to the element, NULL if not found
 */
pcb_element_t *SearchElementByName(pcb_data_t *Base, const char *Name)
{
	pcb_element_t *result = NULL;

	ELEMENT_LOOP(Base);
	{
		if (element->Name[1].TextString && PCB_NSTRCMP(element->Name[1].TextString, Name) == 0) {
			result = element;
			return (result);
		}
	}
	END_LOOP;
	return result;
}

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type
 */
int SearchScreen(pcb_coord_t X, pcb_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	int ans;

	ans = SearchObjectByLocation(Type, Result1, Result2, Result3, X, Y, SLOP * pixel_slop);
	return (ans);
}

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type
 */
int SearchScreenGridSlop(pcb_coord_t X, pcb_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	int ans;

	ans = SearchObjectByLocation(Type, Result1, Result2, Result3, X, Y, PCB->Grid / 2);
	return (ans);
}

int lines_intersect(pcb_coord_t ax1, pcb_coord_t ay1, pcb_coord_t ax2, pcb_coord_t ay2, pcb_coord_t bx1, pcb_coord_t by1, pcb_coord_t bx2, pcb_coord_t by2)
{
/* TODO: this should be long double if pcb_coord_t is 64 bits */
	double ua, xi, yi, X1, Y1, X2, Y2, X3, Y3, X4, Y4, tmp;
	int is_a_pt, is_b_pt;

	/* degenerate cases: a line is actually a point */
	is_a_pt = (ax1 == ax2) && (ay1 == ay2);
	is_b_pt = (bx1 == bx2) && (by1 == by2);

	if (is_a_pt && is_b_pt)
		return (ax1 == bx1) && (ay1 == by1);

	if (is_a_pt)
		return is_point_on_line(ax1, ay1, bx1, by1, bx2, by2);
	if (is_b_pt)
		return is_point_on_line(bx1, by1, ax1, ay1, ax2, ay2);

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
		return
			is_point_on_line(ax1, ay1,  bx1, by1, bx2, by2) ||
			is_point_on_line(ax2, ay2,  bx1, by1, bx2, by2) ||
			is_point_on_line(bx1, by1,  ax1, ay1, ax2, ay2) ||
			is_point_on_line(bx2, by2,  ax1, ay1, ax2, ay2);
	}


	ua = ((X4 - X3) * (Y1 - Y3) - (Y4 - Y3) * (X1 - X3)) / tmp;
/*	ub = ((X2 - X1) * (Y1 - Y3) - (Y2 - Y1) * (X1 - X3)) / tmp;*/
	xi = X1 + ua * (X2 - X1);
	yi = Y1 + ua * (Y2 - Y1);

#define check(e1, e2, i) \
	if (e1 < e2) { \
		if ((i < e1) || (i > e2)) \
			return 0; \
	} \
	else { \
		if ((i > e1) || (i < e2)) \
			return 0; \
	}

	check(ax1, ax2, xi);
	check(bx1, bx2, xi);
	check(ay1, ay2, yi);
	check(by1, by2, yi);
	return 1;
}
