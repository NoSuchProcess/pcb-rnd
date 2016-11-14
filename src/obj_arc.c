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
#include "compat_nls.h"
#include "board.h"
#include "data.h"
#include "polygon.h"
#include "undo.h"
#include "rotate.h"
#include "move.h"
#include "conf_core.h"

#include "obj_arc.h"
#include "obj_arc_op.h"

/* TODO: could be removed if draw.c could be split up */
#include "draw.h"
#include "obj_arc_draw.h"


pcb_arc_t *pcb_arc_new(pcb_layer_t * layer)
{
	pcb_arc_t *new_obj;

	new_obj = calloc(sizeof(pcb_arc_t), 1);
	arclist_append(&layer->Arc, new_obj);

	return new_obj;
}

pcb_arc_t *pcb_element_arc_new(pcb_element_t *Element)
{
	pcb_arc_t *arc = calloc(sizeof(pcb_arc_t), 1);

	arclist_append(&Element->Arc, arc);
	return arc;
}



/* computes the bounding box of an arc */
void SetArcBoundingBox(pcb_arc_t *Arc)
{
	double ca1, ca2, sa1, sa2;
	double minx, maxx, miny, maxy;
	pcb_angle_t ang1, ang2;
	pcb_coord_t width;

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
	pcb_close_box(&Arc->BoundingBox);
}

pcb_box_t *GetArcEnds(pcb_arc_t *Arc)
{
	static pcb_box_t box;
	box.X1 = Arc->X - Arc->Width * cos(Arc->StartAngle * PCB_M180);
	box.Y1 = Arc->Y + Arc->Height * sin(Arc->StartAngle * PCB_M180);
	box.X2 = Arc->X - Arc->Width * cos((Arc->StartAngle + Arc->Delta) * PCB_M180);
	box.Y2 = Arc->Y + Arc->Height * sin((Arc->StartAngle + Arc->Delta) * PCB_M180);
	return &box;
}

/* doesn't these belong in change.c ?? */
void ChangeArcAngles(pcb_layer_t *Layer, pcb_arc_t *a, pcb_angle_t new_sa, pcb_angle_t new_da)
{
	if (new_da >= 360) {
		new_da = 360;
		new_sa = 0;
	}
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
	r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	AddObjectToChangeAnglesUndoList(PCB_TYPE_ARC, a, a, a);
	a->StartAngle = new_sa;
	a->Delta = new_da;
	SetArcBoundingBox(a);
	r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


void ChangeArcRadii(pcb_layer_t *Layer, pcb_arc_t *a, pcb_coord_t new_width, pcb_coord_t new_height)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
	r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	AddObjectToChangeRadiiUndoList(PCB_TYPE_ARC, a, a, a);
	a->Width = new_width;
	a->Height = new_height;
	SetArcBoundingBox(a);
	r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


/* creates a new arc on a layer */
pcb_arc_t *CreateNewArcOnLayer(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t width, pcb_coord_t height, pcb_angle_t sa, pcb_angle_t dir, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_arc_t *Arc;

	ARC_LOOP(Layer);
	{
		if (arc->X == X1 && arc->Y == Y1 && arc->Width == width &&
				NormalizeAngle(arc->StartAngle) == NormalizeAngle(sa) && arc->Delta == dir)
			return (NULL);						/* prevent stacked arcs */
	}
	END_LOOP;
	Arc = pcb_arc_new(Layer);
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

void pcb_add_arc_on_layer(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	SetArcBoundingBox(Arc);
	if (!Layer->arc_tree)
		Layer->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
}



void pcb_arc_free(pcb_arc_t * data)
{
	arclist_remove(data);
	free(data);
}


/***** operations *****/

/* copies an arc to buffer */
void *AddArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];

	return (CreateNewArcOnLayer(layer, Arc->X, Arc->Y,
		Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta,
		Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves an arc to buffer */
void *MoveArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_arc_t * arc)
{
	pcb_layer_t *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_ARC, layer, arc);
	r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);

	arclist_remove(arc);
	arclist_append(&lay->Arc, arc);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, arc);

	if (!lay->arc_tree)
		lay->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->arc_tree, (pcb_box_t *) arc, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_ARC, lay, arc);
	return (arc);
}

/* changes the size of an arc */
void *ChangeArcSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Arc->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return (NULL);
	if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != Arc->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Thickness = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the clearance size of an arc */
void *ChangeArcClearSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Arc->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	value = MIN(MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Arc->Clearance) {
		AddObjectToClearSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Clearance = value;
		if (Arc->Clearance == 0) {
			PCB_FLAG_CLEAR(PCB_FLAG_CLEARLINE, Arc);
			Arc->Clearance = PCB_MIL_TO_COORD(10);
		}
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the radius of an arc (is_primary 0=width or 1=height or 2=both) */
void *ChangeArcRadius(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value, *dst;
	void *a0, *a1;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
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
		r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the angle of an arc (is_primary 0=start or 1=end) */
void *ChangeArcAngle(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_angle_t value, *dst;
	void *a0, *a1;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
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
		r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* changes the clearance flag of an arc */
void *ChangeArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return (NULL);
	EraseArc(Arc);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc)) {
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_false);
	}
	AddObjectToFlagUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Arc);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc)) {
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_true);
	}
	DrawArc(Layer, Arc);
	return (Arc);
}

/* sets the clearance flag of an arc */
void *SetArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(ctx, Layer, Arc);
}

/* clears the clearance flag of an arc */
void *ClrArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(ctx, Layer, Arc);
}

/* copies an arc */
void *CopyArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_arc_t *arc;

	arc = CreateNewArcOnLayer(Layer, Arc->X + ctx->copy.DeltaX,
														Arc->Y + ctx->copy.DeltaY, Arc->Width, Arc->Height, Arc->StartAngle,
														Arc->Delta, Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND));
	if (!arc)
		return (arc);
	DrawArc(Layer, arc);
	AddObjectToCreateUndoList(PCB_TYPE_ARC, Layer, arc, arc);
	return (arc);
}

/* moves an arc */
void *MoveArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	if (Layer->On) {
		EraseArc(Arc);
		MOVE_ARC_LOWLEVEL(Arc, ctx->move.dx, ctx->move.dy);
		DrawArc(Layer, Arc);
		pcb_draw();
	}
	else {
		MOVE_ARC_LOWLEVEL(Arc, ctx->move.dx, ctx->move.dy);
	}
	r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	return (Arc);
}

/* moves an arc between layers; lowlevel routines */
void *MoveArcToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_arc_t * arc, pcb_layer_t * Destination)
{
	r_delete_entry(Source->arc_tree, (pcb_box_t *) arc);

	arclist_remove(arc);
	arclist_append(&Destination->Arc, arc);

	if (!Destination->arc_tree)
		Destination->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Destination->arc_tree, (pcb_box_t *) arc, 0);
	return arc;
}


/* moves an arc between layers */
void *MoveArcToLayer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_arc_t * Arc)
{
	pcb_arc_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc)) {
		pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->On) {
		DrawArc(Layer, Arc);
		pcb_draw();
	}
	if (((long int) ctx->move.dst_layer == -1) || ctx->move.dst_layer == Layer)
		return (Arc);
	AddObjectToMoveToLayerUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	if (Layer->On)
		EraseArc(Arc);
	newone = (pcb_arc_t *) MoveArcToLayerLowLevel(ctx, Layer, Arc, ctx->move.dst_layer);
	ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, ctx->move.dst_layer, Arc);
	if (ctx->move.dst_layer->On)
		DrawArc(ctx->move.dst_layer, newone);
	pcb_draw();
	return (newone);
}

/* destroys an arc from a layer */
void *DestroyArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);

	pcb_arc_free(Arc);

	return NULL;
}

/* removes an arc from a layer */
void *RemoveArc_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	/* erase from screen */
	if (Layer->On) {
		EraseArc(Arc);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	return NULL;
}

void *RemoveArc(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveArc_op(&ctx, Layer, Arc);
}

/* rotates an arc */
void RotateArcLowLevel(pcb_arc_t *Arc, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_coord_t save;

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
void *RotateArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	EraseArc(Arc);
	r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	RotateArcLowLevel(Arc, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
	DrawArc(Layer, Arc);
	pcb_draw();
	return (Arc);
}

/*** draw ***/
void _draw_arc(pcb_arc_t * arc)
{
	if (!arc->Thickness)
		return;

	if (conf_core.editor.thin_draw)
		gui->set_line_width(Output.fgGC, 0);
	else
		gui->set_line_width(Output.fgGC, arc->Thickness);
	gui->set_line_cap(Output.fgGC, Trace_Cap);

	gui->draw_arc(Output.fgGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
}

void draw_arc(pcb_layer_t * layer, pcb_arc_t * arc)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, arc))
		color = PCB->WarnColor;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, arc)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc))
			color = layer->SelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else
		color = layer->Color;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, arc)) {
		assert(color != NULL);
		pcb_lighten_color(color, buf, 1.75);
		color = buf;
	}
	gui->set_color(Output.fgGC, color);
	_draw_arc(arc);
}

pcb_r_dir_t draw_arc_callback(const pcb_box_t * b, void *cl)
{
	draw_arc((pcb_layer_t *) cl, (pcb_arc_t *) b);
	return R_DIR_FOUND_CONTINUE;
}

/* erases an arc on a layer */
void EraseArc(pcb_arc_t *Arc)
{
	if (!Arc->Thickness)
		return;
	pcb_draw_invalidate(Arc);
	pcb_flag_erase(&Arc->Flags);
}

void DrawArc(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_draw_invalidate(Arc);
}
