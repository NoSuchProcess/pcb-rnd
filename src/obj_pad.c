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

/* Drawing primitive: smd pads */

#include "config.h"
#include "global_objs.h"

#include "board.h"
#include "data.h"
#include "create.h"
#include "move.h"
#include "undo.h"
#include "math_helper.h"
#include "box.h"
#include "polygon.h"
#include "compat_misc.h"
#include "misc.h"

#include "obj_pad.h"
#include "obj_pad_list.h"
#include "obj_pad_op.h"


/* TODO: remove this if draw.[ch] pads are moved */
#include "draw.h"

/*** allocation ***/
/* get next slot for a pad, allocates memory if necessary */
PadType *GetPadMemory(ElementType * element)
{
	PadType *new_obj;

	new_obj = calloc(sizeof(PadType), 1);
	padlist_append(&element->Pad, new_obj);

	return new_obj;
}

void RemoveFreePad(PadType * data)
{
	padlist_remove(data);
	free(data);
}


/*** utility ***/
/* creates a new pad in an element */
PadTypePtr CreateNewPad(ElementTypePtr Element, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, Coord Mask, char *Name, char *Number, FlagType Flags)
{
	PadTypePtr pad = GetPadMemory(Element);

	/* copy values */
	if (X1 > X2 || (X1 == X2 && Y1 > Y2)) {
		pad->Point1.X = X2;
		pad->Point1.Y = Y2;
		pad->Point2.X = X1;
		pad->Point2.Y = Y1;
	}
	else {
		pad->Point1.X = X1;
		pad->Point1.Y = Y1;
		pad->Point2.X = X2;
		pad->Point2.Y = Y2;
	}
	pad->Thickness = Thickness;
	pad->Clearance = Clearance;
	pad->Mask = Mask;
	pad->Name = pcb_strdup_null(Name);
	pad->Number = pcb_strdup_null(Number);
	pad->Flags = Flags;
	CLEAR_FLAG(PCB_FLAG_WARN, pad);
	pad->ID = CreateIDGet();
	pad->Element = Element;
	return (pad);
}

/* sets the bounding box of a pad */
void SetPadBoundingBox(PadTypePtr Pad)
{
	Coord width;
	Coord deltax;
	Coord deltay;

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = (Pad->Thickness + Pad->Clearance + 1) / 2;
	width = MAX(width, (Pad->Mask + 1) / 2);
	deltax = Pad->Point2.X - Pad->Point1.X;
	deltay = Pad->Point2.Y - Pad->Point1.Y;

	if (TEST_FLAG(PCB_FLAG_SQUARE, Pad) && deltax != 0 && deltay != 0) {
		/* slanted square pad */
		double theta;
		Coord btx, bty;

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		theta = atan2(deltay, deltax);
		btx = width * cos(theta + M_PI / 4) * sqrt(2.0);
		bty = width * sin(theta + M_PI / 4) * sqrt(2.0);


		Pad->BoundingBox.X1 = MIN(MIN(Pad->Point1.X - btx, Pad->Point1.X - bty), MIN(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.X2 = MAX(MAX(Pad->Point1.X - btx, Pad->Point1.X - bty), MAX(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.Y1 = MIN(MIN(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MIN(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
		Pad->BoundingBox.Y2 = MAX(MAX(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MAX(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
	}
	else {
		/* Adjust for our discrete polygon approximation */
		width = (double) width *POLY_CIRC_RADIUS_ADJ + 0.5;

		Pad->BoundingBox.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - width;
		Pad->BoundingBox.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + width;
		Pad->BoundingBox.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - width;
		Pad->BoundingBox.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + width;
	}
	close_box(&Pad->BoundingBox);
}



/*** ops ***/

/* changes the size of a pad */
void *ChangePadSize(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Thickness + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	if (value <= MAX_PADSIZE && value >= MIN_PADSIZE && value != Pad->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		AddObjectToMaskSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		ErasePad(Pad);
		r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask += value - Pad->Thickness;
		Pad->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		DrawPad(Pad);
		return (Pad);
	}
	return (NULL);
}

/* changes the clearance size of a pad */
void *ChangePadClearSize(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (value == Pad->Clearance)
		return NULL;
	AddObjectToClearSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	ErasePad(Pad);
	r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
	Pad->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	DrawPad(Pad);
	return Pad;
}

/* changes the name of a pad */
void *ChangePadName(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	char *old = Pad->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Name = ctx->chgname.new_name;
		DrawPadName(Pad);
	}
	else
		Pad->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pad */
void *ChangePadNum(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	char *old = Pad->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Number = ctx->chgname.new_name;
		DrawPadName(Pad);
	}
	else
		Pad->Number = ctx->chgname.new_name;
	return (old);
}

/* changes the square flag of a pad */
void *ChangePadSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	ErasePad(Pad);
	AddObjectToClearPolyUndoList(PCB_TYPE_PAD, Element, Pad, Pad, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	AddObjectToFlagUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
	TOGGLE_FLAG(PCB_FLAG_SQUARE, Pad);
	AddObjectToClearPolyUndoList(PCB_TYPE_PAD, Element, Pad, Pad, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	DrawPad(Pad);
	return (Pad);
}

/* sets the square flag of a pad */
void *SetPadSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(ctx, Element, Pad));
}


/* clears the square flag of a pad */
void *ClrPadSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || !TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(ctx, Element, Pad));
}

/* changes the mask size of a pad */
void *ChangePadMaskSize(pcb_opctx_t *ctx, ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pad->Mask && ctx->chgsize.absolute == 0)
		value = Pad->Thickness;
	if (value != Pad->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		ErasePad(Pad);
		r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask = value;
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		DrawPad(Pad);
		return (Pad);
	}
	return (NULL);
}

