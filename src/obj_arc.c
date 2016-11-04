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
#include "buffer.h"
#include "board.h"
#include "data.h"
#include "rtree.h"
#include "polygon.h"
#include "box.h"
#include "undo.h"
#include "obj_arc.h"

#include "create.h"

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





void RemoveFreeArc(ArcType * data)
{
	arclist_remove(data);
	free(data);
}

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

