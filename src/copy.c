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

/* functions used to copy pins, elements ...
 * it's necessary to copy data by calling create... since the base pointer
 * may change cause of dynamic memory allocation
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "create.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "select.h"
#include "undo.h"
#include "compat_misc.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *CopyVia(pcb_opctx_t *ctx, PinTypePtr);
static void *CopyLine(pcb_opctx_t *ctx, LayerTypePtr, LineTypePtr);
static void *CopyText(pcb_opctx_t *ctx, LayerTypePtr, TextTypePtr);
static void *CopyPolygon(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr);
static void *CopyElement(pcb_opctx_t *ctx, ElementTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t CopyFunctions = {
	CopyLine,
	CopyText,
	CopyPolygon,
	CopyVia,
	CopyElement,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	CopyArc,
	NULL
};

/* ---------------------------------------------------------------------------
 * copies data from one polygon to another
 * 'Dest' has to exist
 */
PolygonTypePtr CopyPolygonLowLevel(PolygonTypePtr Dest, PolygonTypePtr Src)
{
	pcb_cardinal_t hole = 0;
	pcb_cardinal_t n;

	for (n = 0; n < Src->PointN; n++) {
		if (hole < Src->HoleIndexN && n == Src->HoleIndex[hole]) {
			CreateNewHoleInPolygon(Dest);
			hole++;
		}
		CreateNewPointInPolygon(Dest, Src->Points[n].X, Src->Points[n].Y);
	}
	SetPolygonBoundingBox(Dest);
	Dest->Flags = Src->Flags;
	CLEAR_FLAG(PCB_FLAG_FOUND, Dest);
	return (Dest);
}

/* ---------------------------------------------------------------------------
 * copies data from one element to another and creates the destination
 * if necessary
 */
ElementTypePtr
CopyElementLowLevel(DataTypePtr Data, ElementTypePtr Dest, ElementTypePtr Src, pcb_bool uniqueName, Coord dx, Coord dy)
{
	int i;
	/* release old memory if necessary */
	if (Dest)
		FreeElementMemory(Dest);

	/* both coordinates and flags are the same */
	Dest = CreateNewElement(Data, Dest, &PCB->Font,
													MaskFlags(Src->Flags, PCB_FLAG_FOUND),
													DESCRIPTION_NAME(Src), NAMEONPCB_NAME(Src),
													VALUE_NAME(Src), DESCRIPTION_TEXT(Src).X + dx,
													DESCRIPTION_TEXT(Src).Y + dy,
													DESCRIPTION_TEXT(Src).Direction,
													DESCRIPTION_TEXT(Src).Scale, MaskFlags(DESCRIPTION_TEXT(Src).Flags, PCB_FLAG_FOUND), uniqueName);

	/* abort on error */
	if (!Dest)
		return (Dest);

	ELEMENTLINE_LOOP(Src);
	{
		CreateNewLineInElement(Dest, line->Point1.X + dx,
													 line->Point1.Y + dy, line->Point2.X + dx, line->Point2.Y + dy, line->Thickness);
	}
	END_LOOP;
	PIN_LOOP(Src);
	{
		CreateNewPin(Dest, pin->X + dx, pin->Y + dy, pin->Thickness,
								 pin->Clearance, pin->Mask, pin->DrillingHole, pin->Name, pin->Number, MaskFlags(pin->Flags, PCB_FLAG_FOUND));
	}
	END_LOOP;
	PAD_LOOP(Src);
	{
		CreateNewPad(Dest, pad->Point1.X + dx, pad->Point1.Y + dy,
								 pad->Point2.X + dx, pad->Point2.Y + dy, pad->Thickness,
								 pad->Clearance, pad->Mask, pad->Name, pad->Number, MaskFlags(pad->Flags, PCB_FLAG_FOUND));
	}
	END_LOOP;
	ARC_LOOP(Src);
	{
		CreateNewArcInElement(Dest, arc->X + dx, arc->Y + dy, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
	}
	END_LOOP;

	for (i = 0; i < Src->Attributes.Number; i++)
		CreateNewAttribute(&Dest->Attributes, Src->Attributes.List[i].name, Src->Attributes.List[i].value);

	Dest->MarkX = Src->MarkX + dx;
	Dest->MarkY = Src->MarkY + dy;

	SetElementBoundingBox(Data, Dest, &PCB->Font);
	return (Dest);
}

/* ---------------------------------------------------------------------------
 * copies a via
 */
static void *CopyVia(pcb_opctx_t *ctx, PinTypePtr Via)
{
	PinTypePtr via;

	via = CreateNewVia(PCB->Data, Via->X + ctx->copy.DeltaX, Via->Y + ctx->copy.DeltaY,
										 Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, MaskFlags(Via->Flags, PCB_FLAG_FOUND));
	if (!via)
		return (via);
	DrawVia(via);
	AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
	return (via);
}

/* ---------------------------------------------------------------------------
 * copies a line
 */
static void *CopyLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	LineTypePtr line;

	line = CreateDrawnLineOnLayer(Layer, Line->Point1.X + ctx->copy.DeltaX,
																Line->Point1.Y + ctx->copy.DeltaY,
																Line->Point2.X + ctx->copy.DeltaX,
																Line->Point2.Y + ctx->copy.DeltaY, Line->Thickness, Line->Clearance, MaskFlags(Line->Flags, PCB_FLAG_FOUND));
	if (!line)
		return (line);
	if (Line->Number)
		line->Number = pcb_strdup(Line->Number);
	DrawLine(Layer, line);
	AddObjectToCreateUndoList(PCB_TYPE_LINE, Layer, line, line);
	return (line);
}

/* ---------------------------------------------------------------------------
 * copies a text
 */
static void *CopyText(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	TextTypePtr text;

	text = CreateNewText(Layer, &PCB->Font, Text->X + ctx->copy.DeltaX,
											 Text->Y + ctx->copy.DeltaY, Text->Direction, Text->Scale, Text->TextString, MaskFlags(Text->Flags, PCB_FLAG_FOUND));
	DrawText(Layer, text);
	AddObjectToCreateUndoList(PCB_TYPE_TEXT, Layer, text, text);
	return (text);
}

/* ---------------------------------------------------------------------------
 * copies a polygon
 */
static void *CopyPolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	PolygonTypePtr polygon;

	polygon = CreateNewPolygon(Layer, NoFlags());
	CopyPolygonLowLevel(polygon, Polygon);
	MovePolygonLowLevel(polygon, ctx->copy.DeltaX, ctx->copy.DeltaY);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->polygon_tree, (BoxTypePtr) polygon, 0);
	InitClip(PCB->Data, Layer, polygon);
	DrawPolygon(Layer, polygon);
	AddObjectToCreateUndoList(PCB_TYPE_POLYGON, Layer, polygon, polygon);
	return (polygon);
}

/* ---------------------------------------------------------------------------
 * copies an element onto the PCB.  Then does a draw.
 */
static void *CopyElement(pcb_opctx_t *ctx, ElementTypePtr Element)
{

#ifdef DEBUG
	printf("Entered CopyElement, trying to copy element %s\n", Element->Name[1].TextString);
#endif

	ElementTypePtr element = CopyElementLowLevel(PCB->Data,
																							 NULL, Element,
																							 conf_core.editor.unique_names, ctx->copy.DeltaX,
																							 ctx->copy.DeltaY);

	/* this call clears the polygons */
	AddObjectToCreateUndoList(PCB_TYPE_ELEMENT, element, element, element);
	if (PCB->ElementOn && (FRONT(element) || PCB->InvisibleObjectsOn)) {
		DrawElementName(element);
		DrawElementPackage(element);
	}
	if (PCB->PinOn) {
		DrawElementPinsAndPads(element);
	}
#ifdef DEBUG
	printf(" ... Leaving CopyElement.\n");
#endif
	return (element);
}

/* ---------------------------------------------------------------------------
 * pastes the contents of the buffer to the layout. Only visible objects
 * are handled by the routine.
 */
pcb_bool CopyPastebufferToLayout(Coord X, Coord Y)
{
	pcb_cardinal_t i;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;

#ifdef DEBUG
	printf("Entering CopyPastebufferToLayout.....\n");
#endif

	/* set movement vector */
	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = X - PASTEBUFFER->X;
	ctx.copy.DeltaY = Y - PASTEBUFFER->Y;

	/* paste all layers */
	for (i = 0; i < max_copper_layer + 2; i++) {
		LayerTypePtr sourcelayer = &PASTEBUFFER->Data->Layer[i], destlayer = LAYER_PTR(i);

		if (destlayer->On) {
			changed = changed || (!LAYER_IS_EMPTY(sourcelayer));
			LINE_LOOP(sourcelayer);
			{
				CopyLine(&ctx, destlayer, line);
			}
			END_LOOP;
			ARC_LOOP(sourcelayer);
			{
				CopyArc(&ctx, destlayer, arc);
			}
			END_LOOP;
			TEXT_LOOP(sourcelayer);
			{
				CopyText(&ctx, destlayer, text);
			}
			END_LOOP;
			POLYGON_LOOP(sourcelayer);
			{
				CopyPolygon(&ctx, destlayer, polygon);
			}
			END_LOOP;
		}
	}

	/* paste elements */
	if (PCB->PinOn && PCB->ElementOn) {
		ELEMENT_LOOP(PASTEBUFFER->Data);
		{
#ifdef DEBUG
			printf("In CopyPastebufferToLayout, pasting element %s\n", element->Name[1].TextString);
#endif
			if (FRONT(element) || PCB->InvisibleObjectsOn) {
				CopyElement(&ctx, element);
				changed = pcb_true;
			}
		}
		END_LOOP;
	}

	/* finally the vias */
	if (PCB->ViaOn) {
		changed |= (pinlist_length(&(PASTEBUFFER->Data->Via)) != 0);
		VIA_LOOP(PASTEBUFFER->Data);
		{
			CopyVia(&ctx, via);
		}
		END_LOOP;
	}

	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}

#ifdef DEBUG
	printf("  .... Leaving CopyPastebufferToLayout.\n");
#endif

	return (changed);
}

/* ---------------------------------------------------------------------------
 * copies the object identified by its data pointers and the type
 * the new objects is moved by DX,DY
 * I assume that the appropriate layer ... is switched on
 */
void *CopyObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord DX, Coord DY)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = DX;
	ctx.copy.DeltaY = DY;

	/* the subroutines add the objects to the undo-list */
	ptr = ObjectOperation(&CopyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();
	return (ptr);
}
