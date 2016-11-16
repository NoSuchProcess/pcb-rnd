/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* crosshair stuff */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "search.h"
#include "polygon.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "vtonpoint.h"

#include "obj_line_draw.h"
#include "obj_arc_draw.h"



typedef struct {
	int x, y;
} point;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void XORPolygon(pcb_polygon_t *, pcb_coord_t, pcb_coord_t, int);
static void XORDrawElement(pcb_element_t *, pcb_coord_t, pcb_coord_t);
static void XORDrawBuffer(pcb_buffer_t *);
static void XORDrawInsertPointObject(void);
static void XORDrawMoveOrpcb_copy_obj(void);
static void XORDrawAttachedLine(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t);
static void XORDrawAttachedArc(pcb_coord_t);

static void thindraw_moved_pv(pcb_pin_t * pv, pcb_coord_t x, pcb_coord_t y)
{
	/* Make a copy of the pin structure, moved to the correct position */
	pcb_pin_t moved_pv = *pv;
	moved_pv.X += x;
	moved_pv.Y += y;

	gui->thindraw_pcb_pv(Crosshair.GC, Crosshair.GC, &moved_pv, pcb_true, pcb_false);
}

static void draw_dashed_line(pcb_hid_gc_t GC, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
/* TODO: we need a real geo lib... using double here is plain wrong */
	double dx = x2-x1, dy = y2-y1;
	double len_squared = dx*dx + dy*dy;
	int n;
	const int segs = 11; /* must be odd */

	if (len_squared < 1000000) {
		/* line too short, just draw it - TODO: magic value; with a proper geo lib this would be gone anyway */
		gui->draw_line(Crosshair.GC, x1, y1, x2, y2);
		return;
	}

	/* first seg is drawn from x1, y1 with no rounding error due to n-1 == 0 */
	for(n = 1; n < segs; n+=2)
		gui->draw_line(Crosshair.GC,
			x1 + (dx * (double)(n-1) / (double)segs), y1 + (dy * (double)(n-1) / (double)segs),
			x1 + (dx * (double)n / (double)segs), y1 + (dy * (double)n / (double)segs));


	/* make sure the last segment is drawn properly to x2 and y2, don't leave
	   room for rounding errors */
	gui->draw_line(Crosshair.GC,
		x2 - (dx / (double)segs), y2 - (dy / (double)segs),
		x2, y2);
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system
 */
static void XORPolygon(pcb_polygon_t *polygon, pcb_coord_t dx, pcb_coord_t dy, int dash_last)
{
	pcb_cardinal_t i;
	for (i = 0; i < polygon->PointN; i++) {
		pcb_cardinal_t next = pcb_poly_contour_next_point(polygon, i);

		if (next == 0) { /* last line: sometimes the implicit closing line */
			if (i == 1) /* corner case: don't draw two lines on top of each other - with XOR it looks bad */
				continue;

			if (dash_last) {
				draw_dashed_line(Crosshair.GC,
									 polygon->Points[i].X + dx,
									 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy);
				break; /* skip normal line draw below */
			}
		}

		/* normal contour line */
		gui->draw_line(Crosshair.GC,
								 polygon->Points[i].X + dx,
								 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy);
	}
}

/*-----------------------------------------------------------
 * Draws the outline of an arc
 */
static void XORDrawAttachedArc(pcb_coord_t thick)
{
	pcb_arc_t arc;
	pcb_box_t *bx;
	pcb_coord_t wx, wy;
	pcb_angle_t sa, dir;
	pcb_coord_t wid = thick / 2;

	wx = Crosshair.X - Crosshair.AttachedBox.Point1.X;
	wy = Crosshair.Y - Crosshair.AttachedBox.Point1.Y;
	if (wx == 0 && wy == 0)
		return;
	arc.X = Crosshair.AttachedBox.Point1.X;
	arc.Y = Crosshair.AttachedBox.Point1.Y;
	if (PCB_XOR(Crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
		arc.X = Crosshair.AttachedBox.Point1.X + coord_abs(wy) * PCB_SGNZ(wx);
		sa = (wx >= 0) ? 0 : 180;
#ifdef ARC45
		if (coord_abs(wy) >= 2 * coord_abs(wx))
			dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 45 : -45;
		else
#endif
			dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 90 : -90;
	}
	else {
		arc.Y = Crosshair.AttachedBox.Point1.Y + coord_abs(wx) * PCB_SGNZ(wy);
		sa = (wy >= 0) ? -90 : 90;
#ifdef ARC45
		if (coord_abs(wx) >= 2 * coord_abs(wy))
			dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -45 : 45;
		else
#endif
			dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -90 : 90;
		wy = wx;
	}
	wy = coord_abs(wy);
	arc.StartAngle = sa;
	arc.Delta = dir;
	arc.Width = arc.Height = wy;
	bx = pcb_arc_get_ends(&arc);
	/*  sa = sa - 180; */
	gui->draw_arc(Crosshair.GC, arc.X, arc.Y, wy + wid, wy + wid, sa, dir);
	if (wid > pixel_slop) {
		gui->draw_arc(Crosshair.GC, arc.X, arc.Y, wy - wid, wy - wid, sa, dir);
		gui->draw_arc(Crosshair.GC, bx->X1, bx->Y1, wid, wid, sa, -180 * SGN(dir));
		gui->draw_arc(Crosshair.GC, bx->X2, bx->Y2, wid, wid, sa + dir, 180 * SGN(dir));
	}
}

/*-----------------------------------------------------------
 * Draws the outline of a line
 */
static void XORDrawAttachedLine(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thick)
{
	pcb_coord_t dx, dy, ox, oy;
	double h;

	dx = x2 - x1;
	dy = y2 - y1;
	if (dx != 0 || dy != 0)
		h = 0.5 * thick / sqrt(PCB_SQUARE(dx) + PCB_SQUARE(dy));
	else
		h = 0.0;
	ox = dy * h + 0.5 * SGN(dy);
	oy = -(dx * h + 0.5 * SGN(dx));
	gui->draw_line(Crosshair.GC, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
	if (coord_abs(ox) >= pixel_slop || coord_abs(oy) >= pixel_slop) {
		pcb_angle_t angle = atan2(dx, dy) * 57.295779;
		gui->draw_line(Crosshair.GC, x1 - ox, y1 - oy, x2 - ox, y2 - oy);
		gui->draw_arc(Crosshair.GC, x1, y1, thick / 2, thick / 2, angle - 180, 180);
		gui->draw_arc(Crosshair.GC, x2, y2, thick / 2, thick / 2, angle, 180);
	}
}

/* ---------------------------------------------------------------------------
 * draws the elements of a loaded circuit which is to be merged in
 */
static void XORDrawElement(pcb_element_t *Element, pcb_coord_t DX, pcb_coord_t DY)
{
	/* if no silkscreen, draw the bounding box */
	if (arclist_length(&Element->Arc) == 0 && linelist_length(&Element->Line) == 0) {
		gui->draw_line(Crosshair.GC,
									 DX + Element->BoundingBox.X1,
									 DY + Element->BoundingBox.Y1, DX + Element->BoundingBox.X1, DY + Element->BoundingBox.Y2);
		gui->draw_line(Crosshair.GC,
									 DX + Element->BoundingBox.X1,
									 DY + Element->BoundingBox.Y2, DX + Element->BoundingBox.X2, DY + Element->BoundingBox.Y2);
		gui->draw_line(Crosshair.GC,
									 DX + Element->BoundingBox.X2,
									 DY + Element->BoundingBox.Y2, DX + Element->BoundingBox.X2, DY + Element->BoundingBox.Y1);
		gui->draw_line(Crosshair.GC,
									 DX + Element->BoundingBox.X2,
									 DY + Element->BoundingBox.Y1, DX + Element->BoundingBox.X1, DY + Element->BoundingBox.Y1);
	}
	else {
		PCB_ELEMENT_PCB_LINE_LOOP(Element);
		{
			gui->draw_line(Crosshair.GC, DX + line->Point1.X, DY + line->Point1.Y, DX + line->Point2.X, DY + line->Point2.Y);
		}
		END_LOOP;

		/* arc coordinates and angles have to be converted to X11 notation */
		PCB_ARC_LOOP(Element);
		{
			gui->draw_arc(Crosshair.GC, DX + arc->X, DY + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
		}
		END_LOOP;
	}
	/* pin coordinates and angles have to be converted to X11 notation */
	PCB_PIN_LOOP(Element);
	{
		thindraw_moved_pv(pin, DX, DY);
	}
	END_LOOP;

	/* pads */
	PCB_PAD_LOOP(Element);
	{
		if (PCB->InvisibleObjectsOn || (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) != 0) == conf_core.editor.show_solder_side) {
			/* Make a copy of the pad structure, moved to the correct position */
			pcb_pad_t moved_pad = *pad;
			moved_pad.Point1.X += DX;
			moved_pad.Point1.Y += DY;
			moved_pad.Point2.X += DX;
			moved_pad.Point2.Y += DY;

			gui->thindraw_pcb_pad(Crosshair.GC, &moved_pad, pcb_false, pcb_false);
		}
	}
	END_LOOP;
	/* mark */
	gui->draw_line(Crosshair.GC,
								 Element->MarkX + DX - EMARK_SIZE, Element->MarkY + DY, Element->MarkX + DX, Element->MarkY + DY - EMARK_SIZE);
	gui->draw_line(Crosshair.GC,
								 Element->MarkX + DX + EMARK_SIZE, Element->MarkY + DY, Element->MarkX + DX, Element->MarkY + DY - EMARK_SIZE);
	gui->draw_line(Crosshair.GC,
								 Element->MarkX + DX - EMARK_SIZE, Element->MarkY + DY, Element->MarkX + DX, Element->MarkY + DY + EMARK_SIZE);
	gui->draw_line(Crosshair.GC,
								 Element->MarkX + DX + EMARK_SIZE, Element->MarkY + DY, Element->MarkX + DX, Element->MarkY + DY + EMARK_SIZE);
}

/* ---------------------------------------------------------------------------
 * draws all visible and attached objects of the pastebuffer
 */
static void XORDrawBuffer(pcb_buffer_t *Buffer)
{
	pcb_cardinal_t i;
	pcb_coord_t x, y;

	/* set offset */
	x = Crosshair.X - Buffer->X;
	y = Crosshair.Y - Buffer->Y;

	/* draw all visible layers */
	for (i = 0; i < max_copper_layer + 2; i++)
		if (PCB->Data->Layer[i].On) {
			pcb_layer_t *layer = &Buffer->Data->Layer[i];

			PCB_LINE_LOOP(layer);
			{
/*
				XORDrawAttachedLine(x +line->Point1.X,
					y +line->Point1.Y, x +line->Point2.X,
					y +line->Point2.Y, line->Thickness);
*/
				gui->draw_line(Crosshair.GC, x + line->Point1.X, y + line->Point1.Y, x + line->Point2.X, y + line->Point2.Y);
			}
			END_LOOP;
			PCB_ARC_LOOP(layer);
			{
				gui->draw_arc(Crosshair.GC, x + arc->X, y + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
			}
			END_LOOP;
			PCB_TEXT_LOOP(layer);
			{
				pcb_box_t *box = &text->BoundingBox;
				gui->draw_rect(Crosshair.GC, x + box->X1, y + box->Y1, x + box->X2, y + box->Y2);
			}
			END_LOOP;
			/* the tmp polygon has n+1 points because the first
			 * and the last one are set to the same coordinates
			 */
			PCB_POLY_LOOP(layer);
			{
				XORPolygon(polygon, x, y, 0);
			}
			END_LOOP;
		}

	/* draw elements if visible */
	if (PCB->PinOn && PCB->ElementOn)
		PCB_ELEMENT_LOOP(Buffer->Data);
	{
		if (PCB_FRONT(element) || PCB->InvisibleObjectsOn)
			XORDrawElement(element, x, y);
	}
	END_LOOP;

	/* and the vias */
	if (PCB->ViaOn)
		PCB_VIA_LOOP(Buffer->Data);
	{
		thindraw_moved_pv(via, x, y);
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * draws the rubberband to insert points into polygons/lines/...
 */
static void XORDrawInsertPointObject(void)
{
	pcb_line_t *line = (pcb_line_t *) Crosshair.AttachedObject.Ptr2;
	pcb_point_t *point = (pcb_point_t *) Crosshair.AttachedObject.Ptr3;

	if (Crosshair.AttachedObject.Type != PCB_TYPE_NONE) {
		gui->draw_line(Crosshair.GC, point->X, point->Y, line->Point1.X, line->Point1.Y);
		gui->draw_line(Crosshair.GC, point->X, point->Y, line->Point2.X, line->Point2.Y);
	}
}

/* ---------------------------------------------------------------------------
 * draws the attached object while in PCB_MODE_MOVE or PCB_MODE_COPY
 */
static void XORDrawMoveOrpcb_copy_obj(void)
{
	pcb_rubberband_t *ptr;
	pcb_cardinal_t i;
	pcb_coord_t dx = Crosshair.X - Crosshair.AttachedObject.X, dy = Crosshair.Y - Crosshair.AttachedObject.Y;

	switch (Crosshair.AttachedObject.Type) {
	case PCB_TYPE_VIA:
		{
			pcb_pin_t *via = (pcb_pin_t *) Crosshair.AttachedObject.Ptr1;
			thindraw_moved_pv(via, dx, dy);
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) Crosshair.AttachedObject.Ptr2;

			XORDrawAttachedLine(line->Point1.X + dx, line->Point1.Y + dy, line->Point2.X + dx, line->Point2.Y + dy, line->Thickness);
			break;
		}

	case PCB_TYPE_ARC:
		{
			pcb_arc_t *Arc = (pcb_arc_t *) Crosshair.AttachedObject.Ptr2;

			gui->draw_arc(Crosshair.GC, Arc->X + dx, Arc->Y + dy, Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta);
			break;
		}

	case PCB_TYPE_POLYGON:
		{
			pcb_polygon_t *polygon = (pcb_polygon_t *) Crosshair.AttachedObject.Ptr2;

			/* the tmp polygon has n+1 points because the first
			 * and the last one are set to the same coordinates
			 */
			XORPolygon(polygon, dx, dy, 0);
			break;
		}

	case PCB_TYPE_LINE_POINT:
		{
			pcb_line_t *line;
			pcb_point_t *point;

			line = (pcb_line_t *) Crosshair.AttachedObject.Ptr2;
			point = (pcb_point_t *) Crosshair.AttachedObject.Ptr3;
			if (point == &line->Point1)
				XORDrawAttachedLine(point->X + dx, point->Y + dy, line->Point2.X, line->Point2.Y, line->Thickness);
			else
				XORDrawAttachedLine(point->X + dx, point->Y + dy, line->Point1.X, line->Point1.Y, line->Thickness);
			break;
		}

	case PCB_TYPE_POLYGON_POINT:
		{
			pcb_polygon_t *polygon;
			pcb_point_t *point;
			pcb_cardinal_t point_idx, prev, next;

			polygon = (pcb_polygon_t *) Crosshair.AttachedObject.Ptr2;
			point = (pcb_point_t *) Crosshair.AttachedObject.Ptr3;
			point_idx = pcb_poly_point_idx(polygon, point);

			/* get previous and following point */
			prev = pcb_poly_contour_prev_point(polygon, point_idx);
			next = pcb_poly_contour_next_point(polygon, point_idx);

			/* draw the two segments */
			gui->draw_line(Crosshair.GC, polygon->Points[prev].X, polygon->Points[prev].Y, point->X + dx, point->Y + dy);
			gui->draw_line(Crosshair.GC, point->X + dx, point->Y + dy, polygon->Points[next].X, polygon->Points[next].Y);
			break;
		}

	case PCB_TYPE_ELEMENT_NAME:
		{
			/* locate the element "mark" and draw an association line from crosshair to it */
			pcb_element_t *element = (pcb_element_t *) Crosshair.AttachedObject.Ptr1;

			gui->draw_line(Crosshair.GC, element->MarkX, element->MarkY, Crosshair.X, Crosshair.Y);
			/* fall through to move the text as a box outline */
		}
	case PCB_TYPE_TEXT:
		{
			pcb_text_t *text = (pcb_text_t *) Crosshair.AttachedObject.Ptr2;
			pcb_box_t *box = &text->BoundingBox;
			gui->draw_rect(Crosshair.GC, box->X1 + dx, box->Y1 + dy, box->X2 + dx, box->Y2 + dy);
			break;
		}

		/* pin/pad movements result in moving an element */
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_ELEMENT:
		XORDrawElement((pcb_element_t *) Crosshair.AttachedObject.Ptr2, dx, dy);
		break;
	}

	/* draw the attached rubberband lines too */
	i = Crosshair.AttachedObject.RubberbandN;
	ptr = Crosshair.AttachedObject.Rubberband;
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

/* ---------------------------------------------------------------------------
 * draws additional stuff that follows the crosshair
 */
void pcb_draw_attached(void)
{
	switch (conf_core.editor.mode) {
	case PCB_MODE_VIA:
		{
			/* Make a dummy via structure to draw from */
			pcb_pin_t via;
			via.X = Crosshair.X;
			via.Y = Crosshair.Y;
			via.Thickness = conf_core.design.via_thickness;
			via.Clearance = 2 * conf_core.design.clearance;
			via.DrillingHole = conf_core.design.via_drilling_hole;
			via.Mask = 0;
			via.Flags = pcb_no_flags();

			gui->thindraw_pcb_pv(Crosshair.GC, Crosshair.GC, &via, pcb_true, pcb_false);

			if (conf_core.editor.show_drc) {
				/* XXX: Naughty cheat - use the mask to draw DRC clearance! */
				via.Mask = conf_core.design.via_thickness + PCB->Bloat * 2;
				gui->set_color(Crosshair.GC, conf_core.appearance.color.cross);
				gui->thindraw_pcb_pv(Crosshair.GC, Crosshair.GC, &via, pcb_false, pcb_true);
				gui->set_color(Crosshair.GC, conf_core.appearance.color.crosshair);
			}
			break;
		}

		/* the attached line is used by both LINEMODE, PCB_MODE_POLYGON and PCB_MODE_POLYGON_HOLE */
	case PCB_MODE_POLYGON:
	case PCB_MODE_POLYGON_HOLE:
		/* draw only if starting point is set */
		if (Crosshair.AttachedLine.State != STATE_FIRST)
			gui->draw_line(Crosshair.GC,
										 Crosshair.AttachedLine.Point1.X,
										 Crosshair.AttachedLine.Point1.Y, Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y);

		/* draw attached polygon only if in PCB_MODE_POLYGON or PCB_MODE_POLYGON_HOLE */
		if (Crosshair.AttachedPolygon.PointN > 1) {
			XORPolygon(&Crosshair.AttachedPolygon, 0, 0, 1);
		}
		break;

	case PCB_MODE_ARC:
		if (Crosshair.AttachedBox.State != STATE_FIRST) {
			XORDrawAttachedArc(conf_core.design.line_thickness);
			if (conf_core.editor.show_drc) {
				gui->set_color(Crosshair.GC, conf_core.appearance.color.cross);
				XORDrawAttachedArc(conf_core.design.line_thickness + 2 * (PCB->Bloat + 1));
				gui->set_color(Crosshair.GC, conf_core.appearance.color.crosshair);
			}

		}
		break;

	case PCB_MODE_LINE:
		/* draw only if starting point exists and the line has length */
		if (Crosshair.AttachedLine.State != STATE_FIRST && Crosshair.AttachedLine.draw) {
			XORDrawAttachedLine(Crosshair.AttachedLine.Point1.X,
													Crosshair.AttachedLine.Point1.Y,
													Crosshair.AttachedLine.Point2.X,
													Crosshair.AttachedLine.Point2.Y, PCB->RatDraw ? 10 : conf_core.design.line_thickness);
			/* draw two lines ? */
			if (conf_core.editor.line_refraction)
				XORDrawAttachedLine(Crosshair.AttachedLine.Point2.X,
														Crosshair.AttachedLine.Point2.Y,
														Crosshair.X, Crosshair.Y, PCB->RatDraw ? 10 : conf_core.design.line_thickness);
			if (conf_core.editor.show_drc) {
				gui->set_color(Crosshair.GC, conf_core.appearance.color.cross);
				XORDrawAttachedLine(Crosshair.AttachedLine.Point1.X,
														Crosshair.AttachedLine.Point1.Y,
														Crosshair.AttachedLine.Point2.X,
														Crosshair.AttachedLine.Point2.Y, PCB->RatDraw ? 10 : conf_core.design.line_thickness + 2 * (PCB->Bloat + 1));
				if (conf_core.editor.line_refraction)
					XORDrawAttachedLine(Crosshair.AttachedLine.Point2.X,
															Crosshair.AttachedLine.Point2.Y,
															Crosshair.X, Crosshair.Y, PCB->RatDraw ? 10 : conf_core.design.line_thickness + 2 * (PCB->Bloat + 1));
				gui->set_color(Crosshair.GC, conf_core.appearance.color.crosshair);
			}
		}
		break;

	case PCB_MODE_PASTE_BUFFER:
		XORDrawBuffer(PCB_PASTEBUFFER);
		break;

	case PCB_MODE_COPY:
	case PCB_MODE_MOVE:
		XORDrawMoveOrpcb_copy_obj();
		break;

	case PCB_MODE_INSERT_POINT:
		XORDrawInsertPointObject();
		break;
	}

	/* an attached box does not depend on a special mode */
	if (Crosshair.AttachedBox.State == STATE_SECOND || Crosshair.AttachedBox.State == STATE_THIRD) {
		pcb_coord_t x1, y1, x2, y2;

		x1 = Crosshair.AttachedBox.Point1.X;
		y1 = Crosshair.AttachedBox.Point1.Y;
		x2 = Crosshair.AttachedBox.Point2.X;
		y2 = Crosshair.AttachedBox.Point2.Y;
		gui->draw_rect(Crosshair.GC, x1, y1, x2, y2);
	}
}


/* --------------------------------------------------------------------------
 * draw the marker position
 */
void pcb_draw_mark(void)
{
	pcb_coord_t ms = conf_core.appearance.mark_size;

	/* Mark is not drawn when it is not set */
	if (!Marked.status)
		return;

	gui->draw_line(Crosshair.GC, Marked.X - ms, Marked.Y - ms, Marked.X + ms, Marked.Y + ms);
	gui->draw_line(Crosshair.GC, Marked.X + ms, Marked.Y - ms, Marked.X - ms, Marked.Y + ms);
}

/* ---------------------------------------------------------------------------
 * Returns the nearest grid-point to the given Coord
 */
pcb_coord_t pcb_grid_fit(pcb_coord_t x, pcb_coord_t grid_spacing, pcb_coord_t grid_offset)
{
	x -= grid_offset;
	x = grid_spacing * pcb_round((double) x / grid_spacing);
	x += grid_offset;
	return x;
}


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the crosshair is being changed.
 *
 * The argument passed is pcb_false to notify "changes are about to happen",
 * and pcb_true to notify "changes have finished".
 *
 * Each call with a 'pcb_false' parameter must be matched with a following one
 * with a 'pcb_true' parameter. Unmatched 'pcb_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'pcb_true' as parameter.
 * They should initiate a redraw of the crosshair attached objects - which may
 * (if necessary) mean repainting the whole screen if the GUI hasn't tracked the
 * location of existing attached drawing.
 */
void pcb_notify_crosshair_change(pcb_bool changes_complete)
{
	if (gui->notify_crosshair_change)
		gui->notify_crosshair_change(changes_complete);
}


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the mark is being changed.
 *
 * The argument passed is pcb_false to notify "changes are about to happen",
 * and pcb_true to notify "changes have finished".
 *
 * Each call with a 'pcb_false' parameter must be matched with a following one
 * with a 'pcb_true' parameter. Unmatched 'pcb_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'pcb_true' as parameter.
 * They should initiate a redraw of the mark - which may (if necessary) mean
 * repainting the whole screen if the GUI hasn't tracked the mark's location.
 */
void pcb_notify_mark_change(pcb_bool changes_complete)
{
	if (gui->notify_mark_change)
		gui->notify_mark_change(changes_complete);
}


/* ---------------------------------------------------------------------------
 * Convenience for plugins using the old {Hide,Restore}Crosshair API.
 * This links up to notify the GUI of the expected changes using the new APIs.
 *
 * Use of this old API is deprecated, as the names don't necessarily reflect
 * what all GUIs may do in response to the notifications. Keeping these APIs
 * is aimed at easing transition to the newer API, they will emit a harmless
 * warning at the time of their first use.
 *
 */
void pcb_crosshair_hide(void)
{
	static pcb_bool warned_old_api = pcb_false;
	if (!warned_old_api) {
		pcb_message(PCB_MSG_DEFAULT, _("WARNING: A plugin is using the deprecated API pcb_crosshair_hide().\n"
							"         This API may be removed in a future release of PCB.\n"));
		warned_old_api = pcb_true;
	}

	pcb_notify_crosshair_change(pcb_false);
	pcb_notify_mark_change(pcb_false);
}

void pcb_crosshair_restore(void)
{
	static pcb_bool warned_old_api = pcb_false;
	if (!warned_old_api) {
		pcb_message(PCB_MSG_DEFAULT, _("WARNING: A plugin is using the deprecated API pcb_crosshair_restore().\n"
							"         This API may be removed in a future release of PCB.\n"));
		warned_old_api = pcb_true;
	}

	pcb_notify_crosshair_change(pcb_true);
	pcb_notify_mark_change(pcb_true);
}

/*
 * Below is the implementation of the "highlight on endpoint" functionality.
 * This highlights lines and arcs when the crosshair is on of their (two)
 * endpoints.
 */
struct onpoint_search_info {
	pcb_crosshair_t *crosshair;
	pcb_coord_t X;
	pcb_coord_t Y;
};

static pcb_r_dir_t onpoint_line_callback(const pcb_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_line_t *line = (pcb_line_t *) box;

#ifdef DEBUG_ONPOINT
	printf("X=%ld Y=%ld    X1=%ld Y1=%ld X2=%ld Y2=%ld\n", info->X, info->Y,
				 line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
#endif
	if ((line->Point1.X == info->X && line->Point1.Y == info->Y) || (line->Point2.X == info->X && line->Point2.Y == info->Y)) {
		pcb_onpoint_t op;
		op.type = PCB_TYPE_LINE;
		op.obj.line = line;
		vtop_append(&crosshair->onpoint_objs, op);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) line);
		DrawLine(NULL, line);
		return R_DIR_FOUND_CONTINUE;
	}
	else {
		return R_DIR_NOT_FOUND;
	}
}

#define close_enough(v1, v2) (coord_abs((v1)-(v2)) < 10)

static pcb_r_dir_t onpoint_arc_callback(const pcb_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_arc_t *arc = (pcb_arc_t *) box;
	pcb_coord_t p1x, p1y, p2x, p2y;

	p1x = arc->X - arc->Width * cos(PCB_TO_RADIANS(arc->StartAngle));
	p1y = arc->Y + arc->Height * sin(PCB_TO_RADIANS(arc->StartAngle));
	p2x = arc->X - arc->Width * cos(PCB_TO_RADIANS(arc->StartAngle + arc->Delta));
	p2y = arc->Y + arc->Height * sin(PCB_TO_RADIANS(arc->StartAngle + arc->Delta));

	/* printf("p1=%ld;%ld p2=%ld;%ld info=%ld;%ld\n", p1x, p1y, p2x, p2y, info->X, info->Y); */

	if ((close_enough(p1x, info->X) && close_enough(p1y, info->Y)) || (close_enough(p2x, info->X) && close_enough(p2y, info->Y))) {
		pcb_onpoint_t op;
		op.type = PCB_TYPE_ARC;
		op.obj.arc = arc;
		vtop_append(&crosshair->onpoint_objs, op);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) arc);
		DrawArc(NULL, arc);
		return R_DIR_FOUND_CONTINUE;
	}
	else {
		return R_DIR_NOT_FOUND;
	}
}

void DrawLineOrArc(int type, void *obj)
{
	switch (type) {
	case PCB_TYPE_LINE_POINT:
		/* Attention: We can use a NULL pointer here for the layer,
		 * because it is not used in the DrawLine() function anyways.
		 * ATM DrawLine() only calls AddPart() internally, which invalidates
		 * the area specified by the line's bounding box.
		 */
		DrawLine(NULL, (pcb_line_t *) obj);
		break;
#if 0
	case ARCPOINT_TYPE:
		/* See comment above */
		DrawArc(NULL, (pcb_arc_t *) obj);
		break;
#endif
	}
}


#define op_swap(crosshair) \
do { \
	vtop_t __tmp__ = crosshair->onpoint_objs; \
	crosshair->onpoint_objs = crosshair->old_onpoint_objs; \
	crosshair->old_onpoint_objs = __tmp__; \
} while(0)

static void *onpoint_find(vtop_t *vect, void *obj_ptr)
{
	int i;

	for (i = 0; i < vect->used; i++) {
		pcb_onpoint_t *op = &(vect->array[i]);
		if (op->obj.any == obj_ptr)
			return op;
	}
	return NULL;
}

/*
 * Searches for lines or arcs which have points that are exactly
 * at the given coordinates and adds them to the crosshair's
 * object list along with their respective type.
 */
static void onpoint_work(pcb_crosshair_t * crosshair, pcb_coord_t X, pcb_coord_t Y)
{
	pcb_box_t SearchBox = pcb_point_box(X, Y);
	struct onpoint_search_info info;
	int i;
	pcb_bool redraw = pcb_false;

	op_swap(crosshair);

	/* Do not truncate to 0 because that may free the array */
	vtop_truncate(&crosshair->onpoint_objs, 1);
	crosshair->onpoint_objs.used = 0;


	info.crosshair = crosshair;
	info.X = X;
	info.Y = Y;

	for (i = 0; i < max_copper_layer; i++) {
		pcb_layer_t *layer = &PCB->Data->Layer[i];
		/* Only find points of arcs and lines on currently visible layers. */
		if (!layer->On)
			continue;
		pcb_r_search(layer->line_tree, &SearchBox, NULL, onpoint_line_callback, &info, NULL);
		pcb_r_search(layer->arc_tree, &SearchBox, NULL, onpoint_arc_callback, &info, NULL);
	}

	/* Undraw the old objects */
	for (i = 0; i < crosshair->old_onpoint_objs.used; i++) {
		pcb_onpoint_t *op = &crosshair->old_onpoint_objs.array[i];

		/* only remove and redraw those which aren't in the new list */
		if (onpoint_find(&crosshair->onpoint_objs, op->obj.any) != NULL)
			continue;

		PCB_FLAG_CLEAR(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) op->obj.any);
		DrawLineOrArc(op->type, op->obj.any);
		redraw = pcb_true;
	}

	/* draw the new objects */
	for (i = 0; i < crosshair->onpoint_objs.used; i++) {
		pcb_onpoint_t *op = &crosshair->onpoint_objs.array[i];

		/* only draw those which aren't in the old list */
		if (onpoint_find(&crosshair->old_onpoint_objs, op->obj.any) != NULL)
			continue;
		DrawLineOrArc(op->type, op->obj.any);
		redraw = pcb_true;
	}

	if (redraw) {
		pcb_redraw();
	}
}

/* ---------------------------------------------------------------------------
 * Returns the square of the given number
 */
static double square(double x)
{
	return x * x;
}

static double crosshair_sq_dist(pcb_crosshair_t * crosshair, pcb_coord_t x, pcb_coord_t y)
{
	return square(x - crosshair->X) + square(y - crosshair->Y);
}

struct snap_data {
	pcb_crosshair_t *crosshair;
	double nearest_sq_dist;
	pcb_bool nearest_is_grid;
	pcb_coord_t x, y;
};

/* Snap to a given location if it is the closest thing we found so far.
 * If "prefer_to_grid" is set, the passed location will take preference
 * over a closer grid points we already snapped to UNLESS the user is
 * pressing the SHIFT key. If the SHIFT key is pressed, the closest object
 * (including grid points), is always preferred.
 */
static void check_snap_object(struct snap_data *snap_data, pcb_coord_t x, pcb_coord_t y, pcb_bool prefer_to_grid)
{
	double sq_dist;

	sq_dist = crosshair_sq_dist(snap_data->crosshair, x, y);
	if (sq_dist < snap_data->nearest_sq_dist || (prefer_to_grid && snap_data->nearest_is_grid && !gui->shift_is_pressed())) {
		snap_data->x = x;
		snap_data->y = y;
		snap_data->nearest_sq_dist = sq_dist;
		snap_data->nearest_is_grid = pcb_false;
	}
}

static void check_snap_offgrid_line(struct snap_data *snap_data, pcb_coord_t nearest_grid_x, pcb_coord_t nearest_grid_y)
{
	void *ptr1, *ptr2, *ptr3;
	int ans;
	pcb_line_t *line;
	pcb_coord_t try_x, try_y;
	double dx, dy;
	double dist;

	if (!conf_core.editor.snap_pin)
		return;

	/* Code to snap at some sensible point along a line */
	/* Pick the nearest grid-point in the x or y direction
	 * to align with, then adjust until we hit the line
	 */
	ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_LINE, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_TYPE_NONE)
		return;

	line = (pcb_line_t *) ptr2;

	/* Allow snapping to off-grid lines when drawing new lines (on
	 * the same layer), and when moving a line end-point
	 * (but don't snap to the same line)
	 */
	if ((conf_core.editor.mode != PCB_MODE_LINE || CURRENT != ptr1) &&
			(conf_core.editor.mode != PCB_MODE_MOVE ||
			 Crosshair.AttachedObject.Ptr1 != ptr1 ||
			 Crosshair.AttachedObject.Type != PCB_TYPE_LINE_POINT || Crosshair.AttachedObject.Ptr2 == line))
		return;

	dx = line->Point2.X - line->Point1.X;
	dy = line->Point2.Y - line->Point1.Y;

	/* Try snapping along the X axis */
	if (dy != 0.) {
		/* Move in the X direction until we hit the line */
		try_x = (nearest_grid_y - line->Point1.Y) / dy * dx + line->Point1.X;
		try_y = nearest_grid_y;
		check_snap_object(snap_data, try_x, try_y, pcb_true);
	}

	/* Try snapping along the Y axis */
	if (dx != 0.) {
		try_x = nearest_grid_x;
		try_y = (nearest_grid_x - line->Point1.X) / dx * dy + line->Point1.Y;
		check_snap_object(snap_data, try_x, try_y, pcb_true);
	}

	if (dx != dy) {								/* If line not parallel with dX = dY direction.. */
		/* Try snapping diagonally towards the line in the dX = dY direction */

		if (dy == 0)
			dist = line->Point1.Y - nearest_grid_y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 - dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y + dist;

		check_snap_object(snap_data, try_x, try_y, pcb_true);
	}

	if (dx != -dy) {							/* If line not parallel with dX = -dY direction.. */
		/* Try snapping diagonally towards the line in the dX = -dY direction */

		if (dy == 0)
			dist = nearest_grid_y - line->Point1.Y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 + dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y - dist;

		check_snap_object(snap_data, try_x, try_y, pcb_true);
	}
}

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void pcb_crosshair_grid_fit(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t nearest_grid_x, nearest_grid_y;
	void *ptr1, *ptr2, *ptr3;
	struct snap_data snap_data;
	int ans;

	Crosshair.X = PCB_CLAMP(X, Crosshair.MinX, Crosshair.MaxX);
	Crosshair.Y = PCB_CLAMP(Y, Crosshair.MinY, Crosshair.MaxY);

	if (PCB->RatDraw) {
		nearest_grid_x = -PCB_MIL_TO_COORD(6);
		nearest_grid_y = -PCB_MIL_TO_COORD(6);
	}
	else {
		nearest_grid_x = pcb_grid_fit(Crosshair.X, PCB->Grid, PCB->GridOffsetX);
		nearest_grid_y = pcb_grid_fit(Crosshair.Y, PCB->Grid, PCB->GridOffsetY);

		if (Marked.status && conf_core.editor.orthogonal_moves) {
			pcb_coord_t dx = Crosshair.X - Marked.X;
			pcb_coord_t dy = Crosshair.Y - Marked.Y;
			if (PCB_ABS(dx) > PCB_ABS(dy))
				nearest_grid_y = Marked.Y;
			else
				nearest_grid_x = Marked.X;
		}

	}

	snap_data.crosshair = &Crosshair;
	snap_data.nearest_sq_dist = crosshair_sq_dist(&Crosshair, nearest_grid_x, nearest_grid_y);
	snap_data.nearest_is_grid = pcb_true;
	snap_data.x = nearest_grid_x;
	snap_data.y = nearest_grid_y;

	ans = PCB_TYPE_NONE;
	if (!PCB->RatDraw)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_ELEMENT, &ptr1, &ptr2, &ptr3);

	if (ans & PCB_TYPE_ELEMENT) {
		pcb_element_t *el = (pcb_element_t *) ptr1;
		check_snap_object(&snap_data, el->MarkX, el->MarkY, pcb_false);
	}

	ans = PCB_TYPE_NONE;
	if (PCB->RatDraw || conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_PAD, &ptr1, &ptr2, &ptr3);

	/* Avoid self-snapping when moving */
	if (ans != PCB_TYPE_NONE &&
			conf_core.editor.mode == PCB_MODE_MOVE && Crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT && ptr1 == Crosshair.AttachedObject.Ptr1)
		ans = PCB_TYPE_NONE;

	if (ans != PCB_TYPE_NONE &&
			(conf_core.editor.mode == PCB_MODE_LINE || (conf_core.editor.mode == PCB_MODE_MOVE && Crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT))) {
		pcb_pad_t *pad = (pcb_pad_t *) ptr2;
		pcb_layer_t *desired_layer;
		pcb_cardinal_t desired_group;
		pcb_cardinal_t SLayer, CLayer;
		int found_our_layer = pcb_false;

		desired_layer = CURRENT;
		if (conf_core.editor.mode == PCB_MODE_MOVE && Crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT) {
			desired_layer = (pcb_layer_t *) Crosshair.AttachedObject.Ptr1;
		}

		/* find layer groups of the component side and solder side */
		SLayer = GetLayerGroupNumberByNumber(solder_silk_layer);
		CLayer = GetLayerGroupNumberByNumber(component_silk_layer);
		desired_group = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SLayer : CLayer;

		GROUP_LOOP(PCB->Data, desired_group);
		{
			if (layer == desired_layer) {
				found_our_layer = pcb_true;
				break;
			}
		}
		END_LOOP;

		if (found_our_layer == pcb_false)
			ans = PCB_TYPE_NONE;
	}

	if (ans != PCB_TYPE_NONE) {
		pcb_pad_t *pad = (pcb_pad_t *) ptr2;
		check_snap_object(&snap_data, (pad->Point1.X + pad->Point2.X) / 2, (pad->Point1.Y + pad->Point2.Y) / 2, pcb_true);
	}

	ans = PCB_TYPE_NONE;
	if (PCB->RatDraw || conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_PIN, &ptr1, &ptr2, &ptr3);

	/* Avoid self-snapping when moving */
	if (ans != PCB_TYPE_NONE &&
			conf_core.editor.mode == PCB_MODE_MOVE && Crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT && ptr1 == Crosshair.AttachedObject.Ptr1)
		ans = PCB_TYPE_NONE;

	if (ans != PCB_TYPE_NONE) {
		pcb_pin_t *pin = (pcb_pin_t *) ptr2;
		check_snap_object(&snap_data, pin->X, pin->Y, pcb_true);
	}

	ans = PCB_TYPE_NONE;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_VIA, &ptr1, &ptr2, &ptr3);

	/* Avoid snapping vias to any other vias */
	if (conf_core.editor.mode == PCB_MODE_MOVE && Crosshair.AttachedObject.Type == PCB_TYPE_VIA && (ans & PCB_TYPEMASK_PIN))
		ans = PCB_TYPE_NONE;

	if (ans != PCB_TYPE_NONE) {
		pcb_pin_t *pin = (pcb_pin_t *) ptr2;
		check_snap_object(&snap_data, pin->X, pin->Y, pcb_true);
	}

	ans = PCB_TYPE_NONE;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_LINE_POINT, &ptr1, &ptr2, &ptr3);

	if (ans != PCB_TYPE_NONE) {
		pcb_point_t *pnt = (pcb_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, pcb_true);
	}

	/*
	 * Snap to offgrid points on lines.
	 */
	if (conf_core.editor.snap_offgrid_line)
		check_snap_offgrid_line(&snap_data, nearest_grid_x, nearest_grid_y);

	ans = PCB_TYPE_NONE;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_POLYGON_POINT, &ptr1, &ptr2, &ptr3);

	if (ans != PCB_TYPE_NONE) {
		pcb_point_t *pnt = (pcb_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, pcb_true);
	}

	if (snap_data.x >= 0 && snap_data.y >= 0) {
		Crosshair.X = snap_data.x;
		Crosshair.Y = snap_data.y;
	}

	if (conf_core.editor.highlight_on_point)
		onpoint_work(&Crosshair, Crosshair.X, Crosshair.Y);

	if (conf_core.editor.mode == PCB_MODE_ARROW) {
		ans = pcb_search_grid_slop(Crosshair.X, Crosshair.Y, PCB_TYPE_LINE_POINT, &ptr1, &ptr2, &ptr3);
		if (ans == PCB_TYPE_NONE)
			pcb_hid_action("PointCursor");
		else if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_line_t *) ptr2))
			pcb_hid_actionl("PointCursor", "True", NULL);
	}

	if (conf_core.editor.mode == PCB_MODE_LINE && Crosshair.AttachedLine.State != STATE_FIRST && conf_core.editor.auto_drc)
		EnforceLineDRC();

	gui->set_crosshair(Crosshair.X, Crosshair.Y, HID_SC_DO_NOTHING);
}

/* ---------------------------------------------------------------------------
 * move crosshair relative (has to be switched off)
 */
void pcb_crosshair_move_relative(pcb_coord_t DeltaX, pcb_coord_t DeltaY)
{
	pcb_crosshair_grid_fit(Crosshair.X + DeltaX, Crosshair.Y + DeltaY);
}

/* ---------------------------------------------------------------------------
 * move crosshair absolute
 * return pcb_true if the crosshair was moved from its existing position
 */
pcb_bool pcb_crosshair_move_absolute(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t x, y, z;
	x = Crosshair.X;
	y = Crosshair.Y;
	pcb_crosshair_grid_fit(X, Y);
	if (Crosshair.X != x || Crosshair.Y != y) {
		/* back up to old position to notify the GUI
		 * (which might want to erase the old crosshair) */
		z = Crosshair.X;
		Crosshair.X = x;
		x = z;
		z = Crosshair.Y;
		Crosshair.Y = y;
		pcb_notify_crosshair_change(pcb_false);	/* Our caller notifies when it has done */
		/* now move forward again */
		Crosshair.X = x;
		Crosshair.Y = z;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the valid range for the crosshair cursor
 */
void pcb_crosshair_set_range(pcb_coord_t MinX, pcb_coord_t MinY, pcb_coord_t MaxX, pcb_coord_t MaxY)
{
	Crosshair.MinX = MAX(0, MinX);
	Crosshair.MinY = MAX(0, MinY);
	Crosshair.MaxX = MIN(PCB->MaxWidth, MaxX);
	Crosshair.MaxY = MIN(PCB->MaxHeight, MaxY);

	/* force update of position */
	pcb_crosshair_move_relative(0, 0);
}

/* ---------------------------------------------------------------------------
 * centers the displayed PCB around the specified point (X,Y)
 */
void pcb_center_display(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t save_grid = PCB->Grid;
	PCB->Grid = 1;
	if (pcb_crosshair_move_absolute(X, Y))
		pcb_notify_crosshair_change(pcb_true);
	gui->set_crosshair(Crosshair.X, Crosshair.Y, HID_SC_WARP_POINTER);
	PCB->Grid = save_grid;
}

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct, allocates to graphical contexts
 */
void pcb_crosshair_init(void)
{
	Crosshair.GC = gui->make_gc();

	gui->set_color(Crosshair.GC, conf_core.appearance.color.crosshair);
	gui->set_draw_xor(Crosshair.GC, 1);
	gui->set_line_cap(Crosshair.GC, Trace_Cap);
	gui->set_line_width(Crosshair.GC, 1);

	/* set initial shape */
	Crosshair.shape = Basic_Crosshair_Shape;

	/* set default limits */
	Crosshair.MinX = Crosshair.MinY = 0;
	Crosshair.MaxX = PCB->MaxWidth;
	Crosshair.MaxY = PCB->MaxHeight;

	/* Initialize the onpoint data. */
	memset(&Crosshair.onpoint_objs, 0, sizeof(vtop_t));
	memset(&Crosshair.old_onpoint_objs, 0, sizeof(vtop_t));

	/* clear the mark */
	Marked.status = pcb_false;
}

/* ---------------------------------------------------------------------------
 * exits crosshair routines, release GCs
 */
void pcb_crosshair_uninit(void)
{
	pcb_poly_free_fields(&Crosshair.AttachedPolygon);
	gui->destroy_gc(Crosshair.GC);
}
