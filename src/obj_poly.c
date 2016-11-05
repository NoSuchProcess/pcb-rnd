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

/* Drawing primitive: polygons */


#include "config.h"
#include "global_objs.h"

#include "board.h"
#include "data.h"
#include "layer.h"
#include "flag.h"
#include "rtree.h"
#include "compat_nls.h"
#include "undo.h"
#include "create.h"
#include "polygon.h"
#include "move.h"
#include "box.h"
#include "rotate.h"
#include "search.h"
#include "set.h"

#include "obj_poly.h"
#include "obj_poly_op.h"
#include "obj_poly_list.h"

/* TODO: get rid of these: */
#include "draw.h"

#define	STEP_POLYGONPOINT	10
#define	STEP_POLYGONHOLEINDEX	10

/*** allocation ***/

/* get next slot for a polygon object, allocates memory if necessary */
PolygonType *GetPolygonMemory(LayerType * layer)
{
	PolygonType *new_obj;

	new_obj = calloc(sizeof(PolygonType), 1);
	polylist_append(&layer->Polygon, new_obj);

	return new_obj;
}

void RemoveFreePolygon(PolygonType * data)
{
	polylist_remove(data);
	free(data);
}

/* gets the next slot for a point in a polygon struct, allocates memory if necessary */
PointTypePtr GetPointMemoryInPolygon(PolygonTypePtr Polygon)
{
	PointTypePtr points = Polygon->Points;

	/* realloc new memory if necessary and clear it */
	if (Polygon->PointN >= Polygon->PointMax) {
		Polygon->PointMax += STEP_POLYGONPOINT;
		points = (PointTypePtr) realloc(points, Polygon->PointMax * sizeof(PointType));
		Polygon->Points = points;
		memset(points + Polygon->PointN, 0, STEP_POLYGONPOINT * sizeof(PointType));
	}
	return (points + Polygon->PointN++);
}

/* gets the next slot for a point in a polygon struct, allocates memory if necessary */
pcb_cardinal_t *GetHoleIndexMemoryInPolygon(PolygonTypePtr Polygon)
{
	pcb_cardinal_t *holeindex = Polygon->HoleIndex;

	/* realloc new memory if necessary and clear it */
	if (Polygon->HoleIndexN >= Polygon->HoleIndexMax) {
		Polygon->HoleIndexMax += STEP_POLYGONHOLEINDEX;
		holeindex = (pcb_cardinal_t *) realloc(holeindex, Polygon->HoleIndexMax * sizeof(int));
		Polygon->HoleIndex = holeindex;
		memset(holeindex + Polygon->HoleIndexN, 0, STEP_POLYGONHOLEINDEX * sizeof(int));
	}
	return (holeindex + Polygon->HoleIndexN++);
}

/* frees memory used by a polygon */
void FreePolygonMemory(PolygonType * polygon)
{
	if (polygon == NULL)
		return;

	free(polygon->Points);
	free(polygon->HoleIndex);

	if (polygon->Clipped)
		poly_Free(&polygon->Clipped);
	poly_FreeContours(&polygon->NoHoles);

	reset_obj_mem(PolygonType, polygon);
}

/*** utility ***/

/* rotates a polygon in 90 degree steps */
void RotatePolygonLowLevel(PolygonTypePtr Polygon, Coord X, Coord Y, unsigned Number)
{
	POLYGONPOINT_LOOP(Polygon);
	{
		ROTATE(point->X, point->Y, X, Y, Number);
	}
	END_LOOP;
	RotateBoxLowLevel(&Polygon->BoundingBox, X, Y, Number);
}


/* sets the bounding box of a polygons */
void SetPolygonBoundingBox(PolygonTypePtr Polygon)
{
	Polygon->BoundingBox.X1 = Polygon->BoundingBox.Y1 = MAX_COORD;
	Polygon->BoundingBox.X2 = Polygon->BoundingBox.Y2 = 0;
	POLYGONPOINT_LOOP(Polygon);
	{
		MAKEMIN(Polygon->BoundingBox.X1, point->X);
		MAKEMIN(Polygon->BoundingBox.Y1, point->Y);
		MAKEMAX(Polygon->BoundingBox.X2, point->X);
		MAKEMAX(Polygon->BoundingBox.Y2, point->Y);
	}
	/* boxes don't include the lower right corner */
	close_box(&Polygon->BoundingBox);
	END_LOOP;
}

/* creates a new polygon from the old formats rectangle data */
PolygonTypePtr CreateNewPolygonFromRectangle(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, FlagType Flags)
{
	PolygonTypePtr polygon = CreateNewPolygon(Layer, Flags);
	if (!polygon)
		return (polygon);

	CreateNewPointInPolygon(polygon, X1, Y1);
	CreateNewPointInPolygon(polygon, X2, Y1);
	CreateNewPointInPolygon(polygon, X2, Y2);
	CreateNewPointInPolygon(polygon, X1, Y2);

	pcb_add_polygon_on_layer(Layer, polygon);
	return (polygon);
}

void pcb_add_polygon_on_layer(LayerType *Layer, PolygonType *polygon)
{
	SetPolygonBoundingBox(polygon);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->polygon_tree, (BoxTypePtr) polygon, 0);
}

/* creates a new polygon on a layer */
PolygonTypePtr CreateNewPolygon(LayerTypePtr Layer, FlagType Flags)
{
	PolygonTypePtr polygon = GetPolygonMemory(Layer);

	/* copy values */
	polygon->Flags = Flags;
	polygon->ID = CreateIDGet();
	polygon->Clipped = NULL;
	polygon->NoHoles = NULL;
	polygon->NoHolesValid = 0;
	return (polygon);
}

/* creates a new point in a polygon */
PointTypePtr CreateNewPointInPolygon(PolygonTypePtr Polygon, Coord X, Coord Y)
{
	PointTypePtr point = GetPointMemoryInPolygon(Polygon);

	/* copy values */
	point->X = X;
	point->Y = Y;
	point->ID = CreateIDGet();
	return (point);
}

/* creates a new hole in a polygon */
PolygonType *CreateNewHoleInPolygon(PolygonType * Polygon)
{
	pcb_cardinal_t *holeindex = GetHoleIndexMemoryInPolygon(Polygon);
	*holeindex = Polygon->PointN;
	return Polygon;
}

/* copies data from one polygon to another; 'Dest' has to exist */
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


/*** ops ***/
/* copies a polygon to buffer */
void *AddPolygonToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	LayerTypePtr layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];
	PolygonTypePtr polygon;

	polygon = CreateNewPolygon(layer, Polygon->Flags);
	CopyPolygonLowLevel(polygon, Polygon);

	/* Update the polygon r-tree. Unlike similarly named functions for
	 * other objects, CreateNewPolygon does not do this as it creates a
	 * skeleton polygon object, which won't have correct bounds.
	 */
	if (!layer->polygon_tree)
		layer->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(layer->polygon_tree, (BoxType *) polygon, 0);

	CLEAR_FLAG(PCB_FLAG_FOUND | ctx->buffer.extraflg, polygon);
	return (polygon);
}


/* moves a polygon to buffer. Doesn't allocate memory for the points */
void *MovePolygonToBuffer(pcb_opctx_t *ctx, LayerType * layer, PolygonType * polygon)
{
	LayerType *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	r_delete_entry(layer->polygon_tree, (BoxType *) polygon);

	polylist_remove(polygon);
	polylist_append(&lay->Polygon, polygon);

	CLEAR_FLAG(PCB_FLAG_FOUND, polygon);

	if (!lay->polygon_tree)
		lay->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->polygon_tree, (BoxType *) polygon, 0);
	return (polygon);
}

/* Handle attempts to change the clearance of a polygon. */
void *ChangePolygonClearSize(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr poly)
{
	static int shown_this_message = 0;
	if (!shown_this_message) {
		gui->confirm_dialog(_("To change the clearance of objects in a polygon, "
													"change the objects, not the polygon.\n"
													"Hint: To set a minimum clearance for a group of objects, "
													"select them all then :MinClearGap(Selected,=10,mil)"), "Ok", NULL);
		shown_this_message = 1;
	}

	return (NULL);
}

/* changes the CLEARPOLY flag of a polygon */
void *ChangePolyClear(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Polygon))
		return (NULL);
	AddObjectToClearPolyUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon, pcb_true);
	AddObjectToFlagUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	TOGGLE_FLAG(PCB_FLAG_CLEARPOLY, Polygon);
	InitClip(PCB->Data, Layer, Polygon);
	DrawPolygon(Layer, Polygon);
	return (Polygon);
}

/* inserts a point into a polygon */
void *InsertPointIntoPolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	PointType save;
	pcb_cardinal_t n;
	LineType line;

	if (!ctx->insert.forcible) {
		/*
		 * first make sure adding the point is sensible
		 */
		line.Thickness = 0;
		line.Point1 = Polygon->Points[prev_contour_point(Polygon, ctx->insert.idx)];
		line.Point2 = Polygon->Points[ctx->insert.idx];
		if (IsPointOnLine((float) ctx->insert.x, (float) ctx->insert.y, 0.0, &line))
			return (NULL);
	}
	/*
	 * second, shift the points up to make room for the new point
	 */
	ErasePolygon(Polygon);
	r_delete_entry(Layer->polygon_tree, (BoxTypePtr) Polygon);
	save = *CreateNewPointInPolygon(Polygon, ctx->insert.x, ctx->insert.y);
	for (n = Polygon->PointN - 1; n > ctx->insert.idx; n--)
		Polygon->Points[n] = Polygon->Points[n - 1];

	/* Shift up indices of any holes */
	for (n = 0; n < Polygon->HoleIndexN; n++)
		if (Polygon->HoleIndex[n] > ctx->insert.idx || (ctx->insert.last && Polygon->HoleIndex[n] == ctx->insert.idx))
			Polygon->HoleIndex[n]++;

	Polygon->Points[ctx->insert.idx] = save;
	SetChangedFlag(pcb_true);
	AddObjectToInsertPointUndoList(PCB_TYPE_POLYGON_POINT, Layer, Polygon, &Polygon->Points[ctx->insert.idx]);

	SetPolygonBoundingBox(Polygon);
	r_insert_entry(Layer->polygon_tree, (BoxType *) Polygon, 0);
	InitClip(PCB->Data, Layer, Polygon);
	if (ctx->insert.forcible || !RemoveExcessPolygonPoints(Layer, Polygon)) {
		DrawPolygon(Layer, Polygon);
		Draw();
	}
	return (&Polygon->Points[ctx->insert.idx]);
}

/* low level routine to move a polygon */
void MovePolygonLowLevel(PolygonTypePtr Polygon, Coord DX, Coord DY)
{
	POLYGONPOINT_LOOP(Polygon);
	{
		MOVE(point->X, point->Y, DX, DY);
	}
	END_LOOP;
	MOVE_BOX_LOWLEVEL(&Polygon->BoundingBox, DX, DY);
}

/* moves a polygon */
void *MovePolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	if (Layer->On) {
		ErasePolygon(Polygon);
	}
	r_delete_entry(Layer->polygon_tree, (BoxType *) Polygon);
	MovePolygonLowLevel(Polygon, ctx->move.dx, ctx->move.dy);
	r_insert_entry(Layer->polygon_tree, (BoxType *) Polygon, 0);
	InitClip(PCB->Data, Layer, Polygon);
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		Draw();
	}
	return (Polygon);
}

/* moves a polygon-point */
void *MovePolygonPoint(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, PointTypePtr Point)
{
	if (Layer->On) {
		ErasePolygon(Polygon);
	}
	r_delete_entry(Layer->polygon_tree, (BoxType *) Polygon);
	MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
	SetPolygonBoundingBox(Polygon);
	r_insert_entry(Layer->polygon_tree, (BoxType *) Polygon, 0);
	RemoveExcessPolygonPoints(Layer, Polygon);
	InitClip(PCB->Data, Layer, Polygon);
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		Draw();
	}
	return (Point);
}

/* moves a polygon between layers; lowlevel routines */
void *MovePolygonToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, PolygonType * polygon, LayerType * Destination)
{
	r_delete_entry(Source->polygon_tree, (BoxType *) polygon);

	polylist_remove(polygon);
	polylist_append(&Destination->Polygon, polygon);

	if (!Destination->polygon_tree)
		Destination->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Destination->polygon_tree, (BoxType *) polygon, 0);

	return polygon;
}

struct mptlc {
	pcb_cardinal_t snum, dnum;
	int type;
	PolygonTypePtr polygon;
} mptlc;

r_dir_t mptl_pin_callback(const BoxType * b, void *cl)
{
	struct mptlc *d = (struct mptlc *) cl;
	PinTypePtr pin = (PinTypePtr) b;
	if (!TEST_THERM(d->snum, pin) || !IsPointInPolygon(pin->X, pin->Y, pin->Thickness + pin->Clearance + 2, d->polygon))
		return R_DIR_NOT_FOUND;
	if (d->type == PCB_TYPE_PIN)
		AddObjectToFlagUndoList(PCB_TYPE_PIN, pin->Element, pin, pin);
	else
		AddObjectToFlagUndoList(PCB_TYPE_VIA, pin, pin, pin);
	ASSIGN_THERM(d->dnum, GET_THERM(d->snum, pin), pin);
	CLEAR_THERM(d->snum, pin);
	return R_DIR_FOUND_CONTINUE;
}

/* moves a polygon between layers */
void *MovePolygonToLayer(pcb_opctx_t *ctx, LayerType * Layer, PolygonType * Polygon)
{
	PolygonTypePtr newone;
	struct mptlc d;

	if (TEST_FLAG(PCB_FLAG_LOCK, Polygon)) {
		Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (((long int) ctx->move.dst_layer == -1) || (Layer == ctx->move.dst_layer))
		return (Polygon);
	AddObjectToMoveToLayerUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	if (Layer->On)
		ErasePolygon(Polygon);
	/* Move all of the thermals with the polygon */
	d.snum = GetLayerNumber(PCB->Data, Layer);
	d.dnum = GetLayerNumber(PCB->Data, ctx->move.dst_layer);
	d.polygon = Polygon;
	d.type = PCB_TYPE_PIN;
	r_search(PCB->Data->pin_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d, NULL);
	d.type = PCB_TYPE_VIA;
	r_search(PCB->Data->via_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d, NULL);
	newone = (struct polygon_st *) MovePolygonToLayerLowLevel(ctx, Layer, Polygon, ctx->move.dst_layer);
	InitClip(PCB->Data, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->On) {
		DrawPolygon(ctx->move.dst_layer, newone);
		Draw();
	}
	return (newone);
}


/* destroys a polygon from a layer */
void *DestroyPolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	r_delete_entry(Layer->polygon_tree, (BoxTypePtr) Polygon);
	FreePolygonMemory(Polygon);

	RemoveFreePolygon(Polygon);

	return NULL;
}

/* removes a polygon-point from a polygon and destroys the data */
void *DestroyPolygonPoint(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, PointTypePtr Point)
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

/* removes a polygon from a layer */
void *RemovePolygon_op(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
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

/* Removes a contour from a polygon.
   If removing the outer contour, it removes the whole polygon. */
void *RemovePolygonContour(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, pcb_cardinal_t contour)
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

/* removes a polygon-point from a polygon */
void *RemovePolygonPoint(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon, PointTypePtr Point)
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

/* copies a polygon */
void *CopyPolygon(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
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
