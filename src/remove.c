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

/* functions used to remove vias, pins ... */

#include "config.h"
#include "conf_core.h"

#include <setjmp.h>
#include <memory.h>

#include "board.h"
#include "data.h"
#include "rats.h"
#include "draw.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "select.h"
#include "undo.h"
#include "obj_all_op.h"
#include "obj_pinvia.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *DestroyRat(pcb_opctx_t *ctx, RatTypePtr);
static void *DestroyPolygon(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr);
static void *DestroyElement(pcb_opctx_t *ctx, ElementTypePtr);
static void *RemoveRat(pcb_opctx_t *ctx, RatTypePtr);
static void *DestroyPolygonPoint(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr, PointTypePtr);
static void *RemovePolygonContour(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr, pcb_cardinal_t);
static void *RemovePolygonPoint(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr, PointTypePtr);

static void *RemoveElement_op(pcb_opctx_t *ctx, ElementTypePtr Element);
static void *RemovePolygon_op(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon);

/* ---------------------------------------------------------------------------
 * some local types
 */
static pcb_opfunc_t RemoveFunctions = {
	RemoveLine_op,
	RemoveText_op,
	RemovePolygon_op,
	RemoveVia,
	RemoveElement_op,
	NULL,
	NULL,
	NULL,
	RemoveLinePoint,
	RemovePolygonPoint,
	RemoveArc_op,
	RemoveRat
};

static pcb_opfunc_t DestroyFunctions = {
	DestroyLine,
	DestroyText,
	DestroyPolygon,
	DestroyVia,
	DestroyElement,
	NULL,
	NULL,
	NULL,
	NULL,
	DestroyPolygonPoint,
	DestroyArc,
	DestroyRat
};

/* ---------------------------------------------------------------------------
 * remove PCB
 */
void RemovePCB(PCBTypePtr Ptr)
{
	ClearUndoList(pcb_true);
	FreePCBMemory(Ptr);
	free(Ptr);
}

/* ---------------------------------------------------------------------------
 * destroys a polygon from a layer
 */
static void *DestroyPolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	r_delete_entry(Layer->polygon_tree, (BoxTypePtr) Polygon);
	FreePolygonMemory(Polygon);

	RemoveFreePolygon(Polygon);

	return NULL;
}

/* ---------------------------------------------------------------------------
 * removes a polygon-point from a polygon and destroys the data
 */
static void *DestroyPolygonPoint(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, PointTypePtr Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	point_idx = polygon_point_idx(Polygon, Point);
	contour = polygon_point_contour(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return RemovePolygonContour(ctx, Layer, Polygon, contour);

	r_delete_entry(Layer->polygon_tree, (BoxType *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	SetPolygonBoundingBox(Polygon);
	r_insert_entry(Layer->polygon_tree, (BoxType *) Polygon, 0);
	InitClip(PCB->Data, Layer, Polygon);
	return (Polygon);
}

/* ---------------------------------------------------------------------------
 * destroys a element
 */
static void *DestroyElement(pcb_opctx_t *ctx, ElementTypePtr Element)
{
	if (ctx->remove.destroy_target->element_tree)
		r_delete_entry(ctx->remove.destroy_target->element_tree, (BoxType *) Element);
	if (ctx->remove.destroy_target->pin_tree) {
		PIN_LOOP(Element);
		{
			r_delete_entry(ctx->remove.destroy_target->pin_tree, (BoxType *) pin);
		}
		END_LOOP;
	}
	if (ctx->remove.destroy_target->pad_tree) {
		PAD_LOOP(Element);
		{
			r_delete_entry(ctx->remove.destroy_target->pad_tree, (BoxType *) pad);
		}
		END_LOOP;
	}
	ELEMENTTEXT_LOOP(Element);
	{
		if (ctx->remove.destroy_target->name_tree[n])
			r_delete_entry(ctx->remove.destroy_target->name_tree[n], (BoxType *) text);
	}
	END_LOOP;
	FreeElementMemory(Element);

	RemoveFreeElement(Element);

	return NULL;
}

/* ---------------------------------------------------------------------------
 * destroys a rat
 */
static void *DestroyRat(pcb_opctx_t *ctx, RatTypePtr Rat)
{
	if (ctx->remove.destroy_target->rat_tree)
		r_delete_entry(ctx->remove.destroy_target->rat_tree, &Rat->BoundingBox);

	RemoveFreeRat(Rat);
	return NULL;
}

/* ---------------------------------------------------------------------------
 * removes a rat
 */
static void *RemoveRat(pcb_opctx_t *ctx, RatTypePtr Rat)
{
	/* erase from screen and memory */
	if (PCB->RatOn) {
		EraseRat(Rat);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_RATLINE, Rat, Rat, Rat);
	return NULL;
}

/* ---------------------------------------------------------------------------
 * removes a polygon from a layer
 */
static void *RemovePolygon_op(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	/* erase from screen */
	if (Layer->On) {
		ErasePolygon(Polygon);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	return NULL;
}

void *RemovePolygon(LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemovePolygon_op(&ctx, Layer, Polygon);
}

/* ---------------------------------------------------------------------------
 * removes a contour from a polygon.
 * If removing the outer contour, it removes the whole polygon.
 */
static void *RemovePolygonContour(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, pcb_cardinal_t contour)
{
	pcb_cardinal_t contour_start, contour_end, contour_points;
	pcb_cardinal_t i;

	if (contour == 0)
		return RemovePolygon(Layer, Polygon);

	if (Layer->On) {
		ErasePolygon(Polygon);
		if (!ctx->remove.bulk)
			Draw();
	}

	/* Copy the polygon to the undo list */
	AddObjectToRemoveContourUndoList(PCB_TYPE_POLYGON, Layer, Polygon);

	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	/* remove points from list, keep point order */
	for (i = contour_start; i < Polygon->PointN - contour_points; i++)
		Polygon->Points[i] = Polygon->Points[i + contour_points];
	Polygon->PointN -= contour_points;

	/* remove hole from list and shift down remaining indices */
	for (i = contour; i < Polygon->HoleIndexN; i++)
		Polygon->HoleIndex[i - 1] = Polygon->HoleIndex[i] - contour_points;
	Polygon->HoleIndexN--;

	InitClip(PCB->Data, Layer, Polygon);
	/* redraw polygon if necessary */
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		if (!ctx->remove.bulk)
			Draw();
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 * removes a polygon-point from a polygon
 */
static void *RemovePolygonPoint(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, PointTypePtr Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	point_idx = polygon_point_idx(Polygon, Point);
	contour = polygon_point_contour(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return RemovePolygonContour(ctx, Layer, Polygon, contour);

	if (Layer->On)
		ErasePolygon(Polygon);

	/* insert the polygon-point into the undo list */
	AddObjectToRemovePointUndoList(PCB_TYPE_POLYGON_POINT, Layer, Polygon, point_idx);
	r_delete_entry(Layer->polygon_tree, (BoxType *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	SetPolygonBoundingBox(Polygon);
	r_insert_entry(Layer->polygon_tree, (BoxType *) Polygon, 0);
	RemoveExcessPolygonPoints(Layer, Polygon);
	InitClip(PCB->Data, Layer, Polygon);

	/* redraw polygon if necessary */
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		if (!ctx->remove.bulk)
			Draw();
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 * removes an element
 */
static void *RemoveElement_op(pcb_opctx_t *ctx, ElementTypePtr Element)
{
	/* erase from screen */
	if ((PCB->ElementOn || PCB->PinOn) && (FRONT(Element) || PCB->InvisibleObjectsOn)) {
		EraseElement(Element);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
}

void *RemoveElement(ElementTypePtr Element)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveElement_op(&ctx, Element);
}

/* ----------------------------------------------------------------------
 * removes all selected and visible objects
 * returns pcb_true if any objects have been removed
 */
pcb_bool RemoveSelected(void)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_true;
	ctx.remove.destroy_target = NULL;

	if (SelectedOperation(&RemoveFunctions, &ctx, pcb_false, PCB_TYPEMASK_ALL)) {
		IncrementUndoSerialNumber();
		Draw();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type,
 * allocated memory is passed to the 'remove undo' list
 */
void *RemoveObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;
	void *ptr;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	ptr = ObjectOperation(&RemoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return (ptr);
}

/* ---------------------------------------------------------------------------
 * DeleteRats - deletes rat lines only
 * can delete all rat lines, or only selected one
 */

pcb_bool DeleteRats(pcb_bool selected)
{
	pcb_opctx_t ctx;
	pcb_bool changed = pcb_false;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_true;
	ctx.remove.destroy_target = NULL;

	RAT_LOOP(PCB->Data);
	{
		if ((!selected) || TEST_FLAG(PCB_FLAG_SELECTED, line)) {
			changed = pcb_true;
			RemoveRat(&ctx, line);
		}
	}
	END_LOOP;
	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (changed);
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type
 * allocated memory is destroyed assumed to already be erased
 */
void *DestroyObject(DataTypePtr Target, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = Target;

	return (ObjectOperation(&DestroyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}
