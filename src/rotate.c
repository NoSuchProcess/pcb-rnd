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


/* functions used to rotate pins, elements ...
 */

#include "config.h"

#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "misc.h"
#include "polygon.h"
#include "rotate.h"
#include "rtree.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "obj_all_op.h"
#include "obj_line.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *RotateElement(pcb_opctx_t *ctx, ElementTypePtr);
static void *RotateElementName(pcb_opctx_t *ctx, ElementTypePtr);

/* ----------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t RotateFunctions = {
	NULL,
	RotateText,
	NULL,
	NULL,
	RotateElement,
	RotateElementName,
	NULL,
	NULL,
	RotateLinePoint,
	NULL,
	RotateArc,
	NULL
};

/* ---------------------------------------------------------------------------
 * rotates a point in 90 degree steps
 */
void RotatePointLowLevel(PointTypePtr Point, Coord X, Coord Y, unsigned Number)
{
	ROTATE(Point->X, Point->Y, X, Y, Number);
}

/* ---------------------------------------------------------------------------
 * rotates a polygon in 90 degree steps
 */
void RotatePolygonLowLevel(PolygonTypePtr Polygon, Coord X, Coord Y, unsigned Number)
{
	POLYGONPOINT_LOOP(Polygon);
	{
		ROTATE(point->X, point->Y, X, Y, Number);
	}
	END_LOOP;
	RotateBoxLowLevel(&Polygon->BoundingBox, X, Y, Number);
}

/* ---------------------------------------------------------------------------
 * rotate an element in 90 degree steps
 */
void RotateElementLowLevel(DataTypePtr Data, ElementTypePtr Element, Coord X, Coord Y, unsigned Number)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (BoxType *) text);
		RotateTextLowLevel(text, X, Y, Number);
	}
	END_LOOP;
	ELEMENTLINE_LOOP(Element);
	{
		RotateLineLowLevel(line, X, Y, Number);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		/* pre-delete the pins from the pin-tree before their coordinates change */
		if (Data)
			r_delete_entry(Data->pin_tree, (BoxType *) pin);
		RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		ROTATE_PIN_LOWLEVEL(pin, X, Y, Number);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			r_delete_entry(Data->pad_tree, (BoxType *) pad);
		RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		ROTATE_PAD_LOWLEVEL(pad, X, Y, Number);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		RotateArcLowLevel(arc, X, Y, Number);
	}
	END_LOOP;
	ROTATE(Element->MarkX, Element->MarkY, X, Y, Number);
	/* SetElementBoundingBox reenters the rtree data */
	SetElementBoundingBox(Data, Element, &PCB->Font);
	ClearFromPolygon(Data, PCB_TYPE_ELEMENT, Element, Element);
}

/* ---------------------------------------------------------------------------
 * rotates an element
 */
static void *RotateElement(pcb_opctx_t *ctx, ElementTypePtr Element)
{
	EraseElement(Element);
	RotateElementLowLevel(PCB->Data, Element, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	DrawElement(Element);
	Draw();
	return (Element);
}

/* ----------------------------------------------------------------------
 * rotates the name of an element
 */
static void *RotateElementName(pcb_opctx_t *ctx, ElementTypePtr Element)
{
	EraseElementName(Element);
	ELEMENTTEXT_LOOP(Element);
	{
		r_delete_entry(PCB->Data->name_tree[n], (BoxType *) text);
		RotateTextLowLevel(text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
		r_insert_entry(PCB->Data->name_tree[n], (BoxType *) text, 0);
	}
	END_LOOP;
	DrawElementName(Element);
	Draw();
	return (Element);
}

/* ---------------------------------------------------------------------------
 * rotates a box in 90 degree steps
 */
void RotateBoxLowLevel(BoxTypePtr Box, Coord X, Coord Y, unsigned Number)
{
	Coord x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

	ROTATE(x1, y1, X, Y, Number);
	ROTATE(x2, y2, X, Y, Number);
	Box->X1 = MIN(x1, x2);
	Box->Y1 = MIN(y1, y2);
	Box->X2 = MAX(x1, x2);
	Box->Y2 = MAX(y1, y2);
}

/* ----------------------------------------------------------------------
 * rotates an objects at the cursor position as identified by its ID
 * the center of rotation is determined by the current cursor location
 */
void *RotateObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord X, Coord Y, unsigned Steps)
{
	RubberbandTypePtr ptr;
	void *ptr2;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;

	/* setup default  global identifiers */
	ctx.rotate.pcb = PCB;
	ctx.rotate.number = Steps;
	ctx.rotate.center_x = X;
	ctx.rotate.center_y = Y;

	/* move all the rubberband lines... and reset the counter */
	ptr = Crosshair.AttachedObject.Rubberband;
	while (Crosshair.AttachedObject.RubberbandN) {
		changed = pcb_true;
		CLEAR_FLAG(PCB_FLAG_RUBBEREND, ptr->Line);
		AddObjectToRotateUndoList(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, ctx.rotate.center_x, ctx.rotate.center_y, Steps);
		EraseLine(ptr->Line);
		if (ptr->Layer) {
			RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			r_delete_entry(ptr->Layer->line_tree, (BoxType *) ptr->Line);
		}
		else
			r_delete_entry(PCB->Data->rat_tree, (BoxType *) ptr->Line);
		RotatePointLowLevel(ptr->MovedPoint, ctx.rotate.center_x, ctx.rotate.center_y, Steps);
		SetLineBoundingBox(ptr->Line);
		if (ptr->Layer) {
			r_insert_entry(ptr->Layer->line_tree, (BoxType *) ptr->Line, 0);
			ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			DrawLine(ptr->Layer, ptr->Line);
		}
		else {
			r_insert_entry(PCB->Data->rat_tree, (BoxType *) ptr->Line, 0);
			DrawRat((RatTypePtr) ptr->Line);
		}
		Crosshair.AttachedObject.RubberbandN--;
		ptr++;
	}
	AddObjectToRotateUndoList(Type, Ptr1, Ptr2, Ptr3, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number);
	ptr2 = ObjectOperation(&RotateFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	changed |= (ptr2 != NULL);
	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (ptr2);
}

void RotateScreenObject(Coord X, Coord Y, unsigned Steps)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	if ((type = SearchScreen(X, Y, ROTATE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
		if (TEST_FLAG(PCB_FLAG_LOCK, (ArcTypePtr) ptr2)) {
			Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
			return;
		}
		Crosshair.AttachedObject.RubberbandN = 0;
		if (conf_core.editor.rubber_band_mode)
			LookupRubberbandLines(type, ptr1, ptr2, ptr3);
		if (type == PCB_TYPE_ELEMENT)
			LookupRatLines(type, ptr1, ptr2, ptr3);
		RotateObject(type, ptr1, ptr2, ptr3, X, Y, Steps);
		SetChangedFlag(pcb_true);
	}
}
