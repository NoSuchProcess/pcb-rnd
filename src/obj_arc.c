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

/* Drawing primitive: (elliptical) arc */

#include "config.h"
#include "global_objs.h"
#include "global_element.h"
#include "compat_nls.h"
#include "buffer.h"
#include "board.h"
#include "data.h"
#include "rtree.h"
#include "polygon.h"
#include "box.h"
#include "undo.h"
#include "rotate.h"
#include "move.h"
#include "obj_arc.h"
#include "create.h"

/* TODO: could be removed if draw.c could be split up */
#include "draw.h"


ArcTypePtr GetArcMemory(LayerType * layer)
{
	ArcType *new_obj;

	new_obj = calloc(sizeof(ArcType), 1);
	arclist_append(&layer->Arc, new_obj);

	return new_obj;
}

ArcType *GetElementArcMemory(ElementType *Element)
{
	ArcType *arc = calloc(sizeof(ArcType), 1);

	arclist_append(&Element->Arc, arc);
	return arc;
}



/* computes the bounding box of an arc */
void SetArcBoundingBox(ArcTypePtr Arc)
{
	double ca1, ca2, sa1, sa2;
	double minx, maxx, miny, maxy;
	Angle ang1, ang2;
	Coord width;

	/* first put angles into standard form:
	 *  ang1 < ang2, both angles between 0 and 720 */
	Arc->Delta = PCB_CLAMP(Arc->Delta, -360, 360);

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

	/* calculate sines, cosines */
	sa1 = sin(PCB_M180 * ang1);
	ca1 = cos(PCB_M180 * ang1);
	sa2 = sin(PCB_M180 * ang2);
	ca2 = cos(PCB_M180 * ang2);

	minx = MIN(ca1, ca2);
	maxx = MAX(ca1, ca2);
	miny = MIN(sa1, sa2);
	maxy = MAX(sa1, sa2);

	/* Check for extreme angles */
	if ((ang1 <= 0 && ang2 >= 0) || (ang1 <= 360 && ang2 >= 360))
		maxx = 1;
	if ((ang1 <= 90 && ang2 >= 90) || (ang1 <= 450 && ang2 >= 450))
		maxy = 1;
	if ((ang1 <= 180 && ang2 >= 180) || (ang1 <= 540 && ang2 >= 540))
		minx = -1;
	if ((ang1 <= 270 && ang2 >= 270) || (ang1 <= 630 && ang2 >= 630))
		miny = -1;

	/* Finally, calculate bounds, converting sane geometry into pcb geometry */
	Arc->BoundingBox.X1 = Arc->X - Arc->Width * maxx;
	Arc->BoundingBox.X2 = Arc->X - Arc->Width * minx;
	Arc->BoundingBox.Y1 = Arc->Y + Arc->Height * miny;
	Arc->BoundingBox.Y2 = Arc->Y + Arc->Height * maxy;

	width = (Arc->Thickness + Arc->Clearance) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *MAX(POLY_CIRC_RADIUS_ADJ, (1.0 + POLY_ARC_MAX_DEVIATION)) + 0.5;

	Arc->BoundingBox.X1 -= width;
	Arc->BoundingBox.X2 += width;
	Arc->BoundingBox.Y1 -= width;
	Arc->BoundingBox.Y2 += width;
	close_box(&Arc->BoundingBox);
}

BoxTypePtr GetArcEnds(ArcTypePtr Arc)
{
	static BoxType box;
	box.X1 = Arc->X - Arc->Width * cos(Arc->StartAngle * PCB_M180);
	box.Y1 = Arc->Y + Arc->Height * sin(Arc->StartAngle * PCB_M180);
	box.X2 = Arc->X - Arc->Width * cos((Arc->StartAngle + Arc->Delta) * PCB_M180);
	box.Y2 = Arc->Y + Arc->Height * sin((Arc->StartAngle + Arc->Delta) * PCB_M180);
	return &box;
}

/* doesn't these belong in change.c ?? */
void ChangeArcAngles(LayerTypePtr Layer, ArcTypePtr a, Angle new_sa, Angle new_da)
{
	if (new_da >= 360) {
		new_da = 360;
		new_sa = 0;
	}
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
	r_delete_entry(Layer->arc_tree, (BoxTypePtr) a);
	AddObjectToChangeAnglesUndoList(PCB_TYPE_ARC, a, a, a);
	a->StartAngle = new_sa;
	a->Delta = new_da;
	SetArcBoundingBox(a);
	r_insert_entry(Layer->arc_tree, (BoxTypePtr) a, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


void ChangeArcRadii(LayerTypePtr Layer, ArcTypePtr a, Coord new_width, Coord new_height)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
	r_delete_entry(Layer->arc_tree, (BoxTypePtr) a);
	AddObjectToChangeRadiiUndoList(PCB_TYPE_ARC, a, a, a);
	a->Width = new_width;
	a->Height = new_height;
	SetArcBoundingBox(a);
	r_insert_entry(Layer->arc_tree, (BoxTypePtr) a, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


/* creates a new arc on a layer */
ArcType *CreateNewArcOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord width, Coord height, Angle sa, Angle dir, Coord Thickness, Coord Clearance, FlagType Flags)
{
	ArcTypePtr Arc;

	ARC_LOOP(Layer);
	{
		if (arc->X == X1 && arc->Y == Y1 && arc->Width == width &&
				NormalizeAngle(arc->StartAngle) == NormalizeAngle(sa) && arc->Delta == dir)
			return (NULL);						/* prevent stacked arcs */
	}
	END_LOOP;
	Arc = GetArcMemory(Layer);
	if (!Arc)
		return (Arc);

	Arc->ID = CreateIDGet();
	Arc->Flags = Flags;
	Arc->Thickness = Thickness;
	Arc->Clearance = Clearance;
	Arc->X = X1;
	Arc->Y = Y1;
	Arc->Width = width;
	Arc->Height = height;
	Arc->StartAngle = sa;
	Arc->Delta = dir;
	pcb_add_arc_on_layer(Layer, Arc);
	return (Arc);
}

void pcb_add_arc_on_layer(LayerType *Layer, ArcType *Arc)
{
	SetArcBoundingBox(Arc);
	if (!Layer->arc_tree)
		Layer->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
}



void RemoveFreeArc(ArcType * data)
{
	arclist_remove(data);
	free(data);
}


/***** operations *****/

/* copies an arc to buffer */
void *AddArcToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	LayerTypePtr layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];

	return (CreateNewArcOnLayer(layer, Arc->X, Arc->Y,
		Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta,
		Arc->Thickness, Arc->Clearance, MaskFlags(Arc->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves an arc to buffer */
void *MoveArcToBuffer(pcb_opctx_t *ctx, LayerType * layer, ArcType * arc)
{
	LayerType *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_ARC, layer, arc);
	r_delete_entry(layer->arc_tree, (BoxType *) arc);

	arclist_remove(arc);
	arclist_append(&lay->Arc, arc);

	CLEAR_FLAG(PCB_FLAG_FOUND, arc);

	if (!lay->arc_tree)
		lay->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->arc_tree, (BoxType *) arc, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_ARC, lay, arc);
	return (arc);
}

/* changes the size of an arc */
void *ChangeArcSize(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Arc->Thickness + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);
	if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != Arc->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Thickness = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the clearance size of an arc */
void *ChangeArcClearSize(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Arc->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	value = MIN(MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Arc->Clearance) {
		AddObjectToClearSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Clearance = value;
		if (Arc->Clearance == 0) {
			CLEAR_FLAG(PCB_FLAG_CLEARLINE, Arc);
			Arc->Clearance = PCB_MIL_TO_COORD(10);
		}
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the radius of an arc (is_primary 0=width or 1=height or 2=both) */
void *ChangeArcRadius(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value, *dst;
	void *a0, *a1;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);

	switch(ctx->chgsize.is_primary) {
		case 0: dst = &Arc->Width; break;
		case 1: dst = &Arc->Height; break;
		case 2:
			ctx->chgsize.is_primary = 0; a0 = ChangeArcRadius(ctx, Layer, Arc);
			ctx->chgsize.is_primary = 1; a1 = ChangeArcRadius(ctx, Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : (*dst) + ctx->chgsize.delta;
	value = MIN(MAX_ARCSIZE, MAX(value, MIN_ARCSIZE));
	if (value != *dst) {
		AddObjectToChangeRadiiUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the angle of an arc (is_primary 0=start or 1=end) */
void *ChangeArcAngle(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	Angle value, *dst;
	void *a0, *a1;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);

	switch(ctx->chgangle.is_primary) {
		case 0: dst = &Arc->StartAngle; break;
		case 1: dst = &Arc->Delta; break;
		case 2:
			ctx->chgangle.is_primary = 0; a0 = ChangeArcAngle(ctx, Layer, Arc);
			ctx->chgangle.is_primary = 1; a1 = ChangeArcAngle(ctx, Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (ctx->chgangle.absolute) ? ctx->chgangle.absolute : (*dst) + ctx->chgangle.delta;
	value = fmod(value, 360.0);
	if (value < 0)
		value += 360;

	if (value != *dst) {
		AddObjectToChangeAnglesUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the clearance flag of an arc */
void *ChangeArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);
	EraseArc(Arc);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Arc)) {
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_false);
	}
	AddObjectToFlagUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Arc);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Arc)) {
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_true);
	}
	DrawArc(Layer, Arc);
	return (Arc);
}

/* sets the clearance flag of an arc */
void *SetArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(ctx, Layer, Arc);
}

/* clears the clearance flag of an arc */
void *ClrArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(ctx, Layer, Arc);
}

/* copies an arc */
void *CopyArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	ArcTypePtr arc;

	arc = CreateNewArcOnLayer(Layer, Arc->X + ctx->copy.DeltaX,
														Arc->Y + ctx->copy.DeltaY, Arc->Width, Arc->Height, Arc->StartAngle,
														Arc->Delta, Arc->Thickness, Arc->Clearance, MaskFlags(Arc->Flags, PCB_FLAG_FOUND));
	if (!arc)
		return (arc);
	DrawArc(Layer, arc);
	AddObjectToCreateUndoList(PCB_TYPE_ARC, Layer, arc, arc);
	return (arc);
}

/* moves an arc */
void *MoveArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	r_delete_entry(Layer->arc_tree, (BoxType *) Arc);
	if (Layer->On) {
		EraseArc(Arc);
		MOVE_ARC_LOWLEVEL(Arc, ctx->move.dx, ctx->move.dy);
		DrawArc(Layer, Arc);
		Draw();
	}
	else {
		MOVE_ARC_LOWLEVEL(Arc, ctx->move.dx, ctx->move.dy);
	}
	r_insert_entry(Layer->arc_tree, (BoxType *) Arc, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	return (Arc);
}

/* moves an arc between layers; lowlevel routines */
void *MoveArcToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, ArcType * arc, LayerType * Destination)
{
	r_delete_entry(Source->arc_tree, (BoxType *) arc);

	arclist_remove(arc);
	arclist_append(&Destination->Arc, arc);

	if (!Destination->arc_tree)
		Destination->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Destination->arc_tree, (BoxType *) arc, 0);
	return arc;
}


/* moves an arc between layers */
void *MoveArcToLayer(pcb_opctx_t *ctx, LayerType * Layer, ArcType * Arc)
{
	ArcTypePtr newone;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc)) {
		Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->On) {
		DrawArc(Layer, Arc);
		Draw();
	}
	if (((long int) ctx->move.dst_layer == -1) || ctx->move.dst_layer == Layer)
		return (Arc);
	AddObjectToMoveToLayerUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	if (Layer->On)
		EraseArc(Arc);
	newone = (ArcTypePtr) MoveArcToLayerLowLevel(ctx, Layer, Arc, ctx->move.dst_layer);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, ctx->move.dst_layer, Arc);
	if (ctx->move.dst_layer->On)
		DrawArc(ctx->move.dst_layer, newone);
	Draw();
	return (newone);
}

/* destroys an arc from a layer */
void *DestroyArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);

	RemoveFreeArc(Arc);

	return NULL;
}

/* removes an arc from a layer */
void *RemoveArc_op(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	/* erase from screen */
	if (Layer->On) {
		EraseArc(Arc);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	return NULL;
}

void *RemoveArc(LayerTypePtr Layer, ArcTypePtr Arc)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveArc_op(&ctx, Layer, Arc);
}

/* rotates an arc */
void RotateArcLowLevel(ArcTypePtr Arc, Coord X, Coord Y, unsigned Number)
{
	Coord save;

	/* add Number*90 degrees (i.e., Number quarter-turns) */
	Arc->StartAngle = NormalizeAngle(Arc->StartAngle + Number * 90);
	ROTATE(Arc->X, Arc->Y, X, Y, Number);

	/* now change width and height */
	if (Number == 1 || Number == 3) {
		save = Arc->Width;
		Arc->Width = Arc->Height;
		Arc->Height = save;
	}
	RotateBoxLowLevel(&Arc->BoundingBox, X, Y, Number);
}

/* rotates an arc */
void *RotateArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc)
{
	EraseArc(Arc);
	r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
	RotateArcLowLevel(Arc, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
	DrawArc(Layer, Arc);
	Draw();
	return (Arc);
}
