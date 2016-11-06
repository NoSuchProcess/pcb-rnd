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

/* Drawing primitive: line segment */

#include "config.h"
#include <setjmp.h>

#include "undo.h"
#include "board.h"
#include "data.h"
#include "search.h"
#include "polygon.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "rotate.h"

#include "obj_line.h"
#include "obj_line_op.h"

/* TODO: maybe remove this and move lines from draw here? */
#include "draw.h"


/**** allocation ****/

/* get next slot for a line, allocates memory if necessary */
LineType *GetLineMemory(LayerType * layer)
{
	LineType *new_obj;

	new_obj = calloc(sizeof(LineType), 1);
	linelist_append(&layer->Line, new_obj);

	return new_obj;
}

void RemoveFreeLine(LineType * data)
{
	linelist_remove(data);
	free(data);
}


/**** utility ****/
struct line_info {
	Coord X1, X2, Y1, Y2;
	Coord Thickness;
	FlagType Flags;
	LineType test, *ans;
	jmp_buf env;
};

static r_dir_t line_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct line_info *i = (struct line_info *) cl;

	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (LineTypePtr) (-1);
		longjmp(i->env, 1);
	}
	/* check the other point order */
	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (LineTypePtr) (-1);
		longjmp(i->env, 1);
	}
	if (line->Point2.X == i->X1 && line->Point1.X == i->X2 && line->Point2.Y == i->Y1 && line->Point1.Y == i->Y2) {
		i->ans = (LineTypePtr) - 1;
		longjmp(i->env, 1);
	}
	/* remove unnecessary line points */
	if (line->Thickness == i->Thickness
			/* don't merge lines if the clear flags differ  */
			&& TEST_FLAG(PCB_FLAG_CLEARLINE, line) == TEST_FLAG(PCB_FLAG_CLEARLINE, i)) {
		if (line->Point1.X == i->X1 && line->Point1.Y == i->Y1) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (IsPointOnLine(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X1 && line->Point2.Y == i->Y1) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (IsPointOnLine(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point1.X == i->X2 && line->Point1.Y == i->Y2) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (IsPointOnLine(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X2 && line->Point2.Y == i->Y2) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (IsPointOnLine(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
	}
	return R_DIR_NOT_FOUND;
}


/* creates a new line on a layer and checks for overlap and extension */
LineTypePtr CreateDrawnLineOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags)
{
	struct line_info info;
	BoxType search;

	search.X1 = MIN(X1, X2);
	search.X2 = MAX(X1, X2);
	search.Y1 = MIN(Y1, Y2);
	search.Y2 = MAX(Y1, Y2);
	if (search.Y2 == search.Y1)
		search.Y2++;
	if (search.X2 == search.X1)
		search.X2++;
	info.X1 = X1;
	info.X2 = X2;
	info.Y1 = Y1;
	info.Y2 = Y2;
	info.Thickness = Thickness;
	info.Flags = Flags;
	info.test.Thickness = 0;
	info.test.Flags = NoFlags();
	info.ans = NULL;
	/* prevent stacking of duplicate lines
	 * and remove needless intermediate points
	 * verify that the layer is on the board first!
	 */
	if (setjmp(info.env) == 0) {
		r_search(Layer->line_tree, &search, NULL, line_callback, &info, NULL);
		return CreateNewLineOnLayer(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
	}

	if ((void *) info.ans == (void *) (-1))
		return NULL;								/* stacked line */
	/* remove unnecessary points */
	if (info.ans) {
		/* must do this BEFORE getting new line memory */
		MoveObjectToRemoveUndoList(PCB_TYPE_LINE, Layer, info.ans, info.ans);
		X1 = info.test.Point1.X;
		X2 = info.test.Point2.X;
		Y1 = info.test.Point1.Y;
		Y2 = info.test.Point2.Y;
	}
	return CreateNewLineOnLayer(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
}

LineTypePtr CreateNewLineOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags)
{
	LineTypePtr Line;

	Line = GetLineMemory(Layer);
	if (!Line)
		return (Line);
	Line->ID = CreateIDGet();
	Line->Flags = Flags;
	CLEAR_FLAG(PCB_FLAG_RAT, Line);
	Line->Thickness = Thickness;
	Line->Clearance = Clearance;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = CreateIDGet();
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = CreateIDGet();
	pcb_add_line_on_layer(Layer, Line);
	return (Line);
}

void pcb_add_line_on_layer(LayerType *Layer, LineType *Line)
{
	SetLineBoundingBox(Line);
	if (!Layer->line_tree)
		Layer->line_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
}

/* sets the bounding box of a line */
void SetLineBoundingBox(LineTypePtr Line)
{
	Coord width = (Line->Thickness + Line->Clearance + 1) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *POLY_CIRC_RADIUS_ADJ + 0.5;

	Line->BoundingBox.X1 = MIN(Line->Point1.X, Line->Point2.X) - width;
	Line->BoundingBox.X2 = MAX(Line->Point1.X, Line->Point2.X) + width;
	Line->BoundingBox.Y1 = MIN(Line->Point1.Y, Line->Point2.Y) - width;
	Line->BoundingBox.Y2 = MAX(Line->Point1.Y, Line->Point2.Y) + width;
	close_box(&Line->BoundingBox);
	SetPointBoundingBox(&Line->Point1);
	SetPointBoundingBox(&Line->Point2);
}




/*** ops ***/

/* copies a line to buffer */
void *AddLineToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	LineTypePtr line;
	LayerTypePtr layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];

	line = CreateNewLineOnLayer(layer, Line->Point1.X, Line->Point1.Y,
															Line->Point2.X, Line->Point2.Y,
															Line->Thickness, Line->Clearance, MaskFlags(Line->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	if (line && Line->Number)
		line->Number = pcb_strdup(Line->Number);
	return (line);
}

/* moves a line to buffer */
void *MoveLineToBuffer(pcb_opctx_t *ctx, LayerType * layer, LineType * line)
{
	LayerTypePtr lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_LINE, layer, line);
	r_delete_entry(layer->line_tree, (BoxType *) line);

	linelist_remove(line);
	linelist_append(&(lay->Line), line);

	CLEAR_FLAG(PCB_FLAG_FOUND, line);

	if (!lay->line_tree)
		lay->line_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->line_tree, (BoxType *) line, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_LINE, lay, line);
	return (line);
}

/* changes the size of a line */
void *ChangeLineSize(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Line->Thickness + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Line))
		return (NULL);
	if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != Line->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_LINE, Layer, Line, Line);
		EraseLine(Line);
		r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		Line->Thickness = value;
		SetLineBoundingBox(Line);
		r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		DrawLine(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/*changes the clearance size of a line */
void *ChangeLineClearSize(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Line->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	value = MIN(MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Line->Clearance) {
		AddObjectToClearSizeUndoList(PCB_TYPE_LINE, Layer, Line, Line);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		EraseLine(Line);
		r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
		Line->Clearance = value;
		if (Line->Clearance == 0) {
			CLEAR_FLAG(PCB_FLAG_CLEARLINE, Line);
			Line->Clearance = PCB_MIL_TO_COORD(10);
		}
		SetLineBoundingBox(Line);
		r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		DrawLine(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/* changes the name of a line */
void *ChangeLineName(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	char *old = Line->Number;

	Layer = Layer;
	Line->Number = ctx->chgname.new_name;
	return (old);
}

/* changes the clearance flag of a line */
void *ChangeLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line))
		return (NULL);
	EraseLine(Line);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Line)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_LINE, Layer, Line, Line, pcb_false);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	AddObjectToFlagUndoList(PCB_TYPE_LINE, Layer, Line, Line);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Line);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Line)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_LINE, Layer, Line, Line, pcb_true);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	DrawLine(Layer, Line);
	return (Line);
}

/* sets the clearance flag of a line */
void *SetLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return ChangeLineJoin(ctx, Layer, Line);
}

/* clears the clearance flag of a line */
void *ClrLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return ChangeLineJoin(ctx, Layer, Line);
}

/* copies a line */
void *CopyLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
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

/* moves a line */
void *MoveLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	if (Layer->On)
		EraseLine(Line);
	RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	r_delete_entry(Layer->line_tree, (BoxType *) Line);
	MOVE_LINE_LOWLEVEL(Line, ctx->move.dx, ctx->move.dy);
	r_insert_entry(Layer->line_tree, (BoxType *) Line, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	if (Layer->On) {
		DrawLine(Layer, Line);
		Draw();
	}
	return (Line);
}

/* moves one end of a line */
void *MoveLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
	if (Layer) {
		if (Layer->On)
			EraseLine(Line);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		r_delete_entry(Layer->line_tree, &Line->BoundingBox);
		MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		SetLineBoundingBox(Line);
		r_insert_entry(Layer->line_tree, &Line->BoundingBox, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		if (Layer->On) {
			DrawLine(Layer, Line);
			Draw();
		}
		return (Line);
	}
	else {												/* must be a rat */

		if (PCB->RatOn)
			EraseRat((RatTypePtr) Line);
		r_delete_entry(PCB->Data->rat_tree, &Line->BoundingBox);
		MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		SetLineBoundingBox(Line);
		r_insert_entry(PCB->Data->rat_tree, &Line->BoundingBox, 0);
		if (PCB->RatOn) {
			DrawRat((RatTypePtr) Line);
			Draw();
		}
		return (Line);
	}
}

/* moves a line between layers; lowlevel routines */
void *MoveLineToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, LineType * line, LayerType * Destination)
{
	r_delete_entry(Source->line_tree, (BoxType *) line);

	linelist_remove(line);
	linelist_append(&(Destination->Line), line);

	if (!Destination->line_tree)
		Destination->line_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Destination->line_tree, (BoxType *) line, 0);
	return line;
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
struct via_info {
	Coord X, Y;
	jmp_buf env;
};

static r_dir_t moveline_callback(const BoxType * b, void *cl)
{
	struct via_info *i = (struct via_info *) cl;
	PinTypePtr via;

	if ((via =
			 CreateNewVia(PCB->Data, i->X, i->Y,
										conf_core.design.via_thickness, 2 * conf_core.design.clearance, PCB_FLAG_NO, conf_core.design.via_drilling_hole, NULL, NoFlags())) != NULL) {
		AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
		DrawVia(via);
	}
	longjmp(i->env, 1);
}

void *MoveLineToLayer(pcb_opctx_t *ctx, LayerType * Layer, LineType * Line)
{
	struct via_info info;
	BoxType sb;
	LineTypePtr newone;
	void *ptr1, *ptr2, *ptr3;

	if (TEST_FLAG(PCB_FLAG_LOCK, Line)) {
		Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->On) {
		DrawLine(Layer, Line);
		Draw();
	}
	if (((long int) ctx->move.dst_layer == -1) || ctx->move.dst_layer == Layer)
		return (Line);

	AddObjectToMoveToLayerUndoList(PCB_TYPE_LINE, Layer, Line, Line);
	if (Layer->On)
		EraseLine(Line);
	RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	newone = (LineTypePtr) MoveLineToLayerLowLevel(ctx, Layer, Line, ctx->move.dst_layer);
	Line = NULL;
	ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->On)
		DrawLine(ctx->move.dst_layer, newone);
	Draw();
	if (!PCB->ViaOn || ctx->move.more_to_come ||
			GetLayerGroupNumberByPointer(Layer) ==
			GetLayerGroupNumberByPointer(ctx->move.dst_layer) || TEST_SILK_LAYER(Layer) || TEST_SILK_LAYER(ctx->move.dst_layer))
		return (newone);
	/* consider via at Point1 */
	sb.X1 = newone->Point1.X - newone->Thickness / 2;
	sb.X2 = newone->Point1.X + newone->Thickness / 2;
	sb.Y1 = newone->Point1.Y - newone->Thickness / 2;
	sb.Y2 = newone->Point1.Y + newone->Thickness / 2;
	if ((SearchObjectByLocation(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point1.X, newone->Point1.Y, conf_core.design.via_thickness / 2) == PCB_TYPE_NONE)) {
		info.X = newone->Point1.X;
		info.Y = newone->Point1.Y;
		if (setjmp(info.env) == 0)
			r_search(Layer->line_tree, &sb, NULL, moveline_callback, &info, NULL);
	}
	/* consider via at Point2 */
	sb.X1 = newone->Point2.X - newone->Thickness / 2;
	sb.X2 = newone->Point2.X + newone->Thickness / 2;
	sb.Y1 = newone->Point2.Y - newone->Thickness / 2;
	sb.Y2 = newone->Point2.Y + newone->Thickness / 2;
	if ((SearchObjectByLocation(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point2.X, newone->Point2.Y, conf_core.design.via_thickness / 2) == PCB_TYPE_NONE)) {
		info.X = newone->Point2.X;
		info.Y = newone->Point2.Y;
		if (setjmp(info.env) == 0)
			r_search(Layer->line_tree, &sb, NULL, moveline_callback, &info, NULL);
	}
	Draw();
	return (newone);
}

/* destroys a line from a layer */
void *DestroyLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
	free(Line->Number);

	RemoveFreeLine(Line);
	return NULL;
}

/* remove point... */
struct rlp_info {
	jmp_buf env;
	LineTypePtr line;
	PointTypePtr point;
};
static r_dir_t remove_point(const BoxType * b, void *cl)
{
	LineType *line = (LineType *) b;
	struct rlp_info *info = (struct rlp_info *) cl;
	if (line == info->line)
		return R_DIR_NOT_FOUND;
	if ((line->Point1.X == info->point->X)
			&& (line->Point1.Y == info->point->Y)) {
		info->line = line;
		info->point = &line->Point1;
		longjmp(info->env, 1);
	}
	else if ((line->Point2.X == info->point->X)
					 && (line->Point2.Y == info->point->Y)) {
		info->line = line;
		info->point = &line->Point2;
		longjmp(info->env, 1);
	}
	return R_DIR_NOT_FOUND;
}

/* removes a line point, or a line if the selected point is the end */
void *RemoveLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
	PointType other;
	struct rlp_info info;
	if (&Line->Point1 == Point)
		other = Line->Point2;
	else
		other = Line->Point1;
	info.line = Line;
	info.point = Point;
	if (setjmp(info.env) == 0) {
		r_search(Layer->line_tree, (const BoxType *) Point, NULL, remove_point, &info, NULL);
		return RemoveLine_op(ctx, Layer, Line);
	}
	MoveObject(PCB_TYPE_LINE_POINT, Layer, info.line, info.point, other.X - Point->X, other.Y - Point->Y);
	return (RemoveLine_op(ctx, Layer, Line));
}

/* removes a line from a layer */
void *RemoveLine_op(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	/* erase from screen */
	if (Layer->On) {
		EraseLine(Line);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_LINE, Layer, Line, Line);
	return NULL;
}

void *RemoveLine(LayerTypePtr Layer, LineTypePtr Line)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveLine_op(&ctx, Layer, Line);
}

/* rotates a line in 90 degree steps */
void RotateLineLowLevel(LineTypePtr Line, Coord X, Coord Y, unsigned Number)
{
	ROTATE(Line->Point1.X, Line->Point1.Y, X, Y, Number);
	ROTATE(Line->Point2.X, Line->Point2.Y, X, Y, Number);
	/* keep horizontal, vertical Point2 > Point1 */
	if (Line->Point1.X == Line->Point2.X) {
		if (Line->Point1.Y > Line->Point2.Y) {
			Coord t;
			t = Line->Point1.Y;
			Line->Point1.Y = Line->Point2.Y;
			Line->Point2.Y = t;
		}
	}
	else if (Line->Point1.Y == Line->Point2.Y) {
		if (Line->Point1.X > Line->Point2.X) {
			Coord t;
			t = Line->Point1.X;
			Line->Point1.X = Line->Point2.X;
			Line->Point2.X = t;
		}
	}
	/* instead of rotating the bounding box, the call updates both end points too */
	SetLineBoundingBox(Line);
}


/* rotates a line's point */
void *RotateLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point)
{
	EraseLine(Line);
	if (Layer) {
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
	}
	else
		r_delete_entry(PCB->Data->rat_tree, (BoxTypePtr) Line);
	RotatePointLowLevel(Point, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	SetLineBoundingBox(Line);
	if (Layer) {
		r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		DrawLine(Layer, Line);
	}
	else {
		r_insert_entry(PCB->Data->rat_tree, (BoxTypePtr) Line, 0);
		DrawRat((RatTypePtr) Line);
	}
	Draw();
	return (Line);
}

/* inserts a point into a line */
void *InsertPointIntoLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line)
{
	LineTypePtr line;
	Coord X, Y;

	if (((Line->Point1.X == ctx->insert.x) && (Line->Point1.Y == ctx->insert.y)) ||
			((Line->Point2.X == ctx->insert.x) && (Line->Point2.Y == ctx->insert.y)))
		return (NULL);
	X = Line->Point2.X;
	Y = Line->Point2.Y;
	AddObjectToMoveUndoList(PCB_TYPE_LINE_POINT, Layer, Line, &Line->Point2, ctx->insert.x - X, ctx->insert.y - Y);
	EraseLine(Line);
	r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
	RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	Line->Point2.X = ctx->insert.x;
	Line->Point2.Y = ctx->insert.y;
	SetLineBoundingBox(Line);
	r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	DrawLine(Layer, Line);
	/* we must create after playing with Line since creation may
	 * invalidate the line pointer
	 */
	if ((line = CreateDrawnLineOnLayer(Layer, ctx->insert.x, ctx->insert.y, X, Y, Line->Thickness, Line->Clearance, Line->Flags))) {
		AddObjectToCreateUndoList(PCB_TYPE_LINE, Layer, line, line);
		DrawLine(Layer, line);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, line);
		/* creation call adds it to the rtree */
	}
	Draw();
	return (line);
}

/*** draw ***/
void _draw_line(LineType * line)
{
	gui->set_line_cap(Output.fgGC, Trace_Cap);
	if (conf_core.editor.thin_draw)
		gui->set_line_width(Output.fgGC, 0);
	else
		gui->set_line_width(Output.fgGC, line->Thickness);

	gui->draw_line(Output.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
}

void draw_line(LayerType * layer, LineType * line)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (TEST_FLAG(PCB_FLAG_WARN, line))
		color = PCB->WarnColor;
	else if (TEST_FLAG(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, line)) {
		if (TEST_FLAG(PCB_FLAG_SELECTED, line))
			color = layer->SelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else
		color = layer->Color;

	if (TEST_FLAG(PCB_FLAG_ONPOINT, line)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}

	gui->set_color(Output.fgGC, color);
	_draw_line(line);
}

r_dir_t draw_line_callback(const BoxType * b, void *cl)
{
	draw_line((LayerType *) cl, (LineType *) b);
	return R_DIR_FOUND_CONTINUE;
}

/* erases a line on a layer */
void EraseLine(LineTypePtr Line)
{
	pcb_draw_invalidate(Line);
	EraseFlags(&Line->Flags);
}

void DrawLine(LayerTypePtr Layer, LineTypePtr Line)
{
	pcb_draw_invalidate(Line);
}

