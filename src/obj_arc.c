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
#include "compat_misc.h"

#include "obj_arc.h"
#include "obj_arc_op.h"

/* TODO: could be removed if draw.c could be split up */
#include "draw.h"
#include "obj_arc_draw.h"

static int pcb_arc_end_addr = 1;
int *pcb_arc_start_ptr = NULL, *pcb_arc_end_ptr = &pcb_arc_end_addr;

pcb_arc_t *pcb_arc_alloc(pcb_layer_t * layer)
{
	pcb_arc_t *new_obj;

	new_obj = calloc(sizeof(pcb_arc_t), 1);
	arclist_append(&layer->Arc, new_obj);

	return new_obj;
}

pcb_arc_t *pcb_element_arc_alloc(pcb_element_t *Element)
{
	pcb_arc_t *arc = calloc(sizeof(pcb_arc_t), 1);

	arclist_append(&Element->Arc, arc);
	return arc;
}



/* computes the bounding box of an arc */
static pcb_box_t pcb_arc_bbox_(const pcb_arc_t *Arc, int mini)
{
	double ca1, ca2, sa1, sa2;
	double minx, maxx, miny, maxy, delta;
	pcb_angle_t ang1, ang2;
	pcb_coord_t width;
	pcb_box_t res;

	/* first put angles into standard form:
	 *  ang1 < ang2, both angles between 0 and 720 */
	delta = PCB_CLAMP(Arc->Delta, -360, 360);

	if (delta > 0) {
		ang1 = pcb_normalize_angle(Arc->StartAngle);
		ang2 = pcb_normalize_angle(Arc->StartAngle + delta);
	}
	else {
		ang1 = pcb_normalize_angle(Arc->StartAngle + delta);
		ang2 = pcb_normalize_angle(Arc->StartAngle);
	}
	if (ang1 > ang2)
		ang2 += 360;
	/* Make sure full circles aren't treated as zero-length arcs */
	if (delta == 360 || delta == -360)
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
	res.X1 = Arc->X - Arc->Width * maxx;
	res.X2 = Arc->X - Arc->Width * minx;
	res.Y1 = Arc->Y + Arc->Height * miny;
	res.Y2 = Arc->Y + Arc->Height * maxy;

	if (!mini) {
		width = (Arc->Thickness + Arc->Clearance) / 2;
		/* Adjust for our discrete polygon approximation */
		width = (double) width *MAX(PCB_POLY_CIRC_RADIUS_ADJ, (1.0 + PCB_POLY_ARC_MAX_DEVIATION)) + 0.5;
	}
	else
		width = Arc->Thickness / 2;

	res.X1 -= width;
	res.X2 += width;
	res.Y1 -= width;
	res.Y2 += width;
	pcb_close_box(&res);
	return res;
}


void pcb_arc_bbox(pcb_arc_t *Arc)
{
	Arc->BoundingBox = pcb_arc_bbox_(Arc, 0);
}


void pcb_arc_get_end(pcb_arc_t *Arc, int which, pcb_coord_t *x, pcb_coord_t *y)
{
	if (which == 0) {
		*x = Arc->X - Arc->Width * cos(Arc->StartAngle * PCB_M180);
		*y = Arc->Y + Arc->Height * sin(Arc->StartAngle * PCB_M180);
	}
	else {
		*x = Arc->X - Arc->Width * cos((Arc->StartAngle + Arc->Delta) * PCB_M180);
		*y = Arc->Y + Arc->Height * sin((Arc->StartAngle + Arc->Delta) * PCB_M180);
	}
}

/* doesn't these belong in change.c ?? */
void pcb_arc_set_angles(pcb_layer_t *Layer, pcb_arc_t *a, pcb_angle_t new_sa, pcb_angle_t new_da)
{
	if (new_da >= 360) {
		new_da = 360;
		new_sa = 0;
	}
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, a);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_undo_add_obj_to_change_angles(PCB_TYPE_ARC, a, a, a);
	a->StartAngle = new_sa;
	a->Delta = new_da;
	pcb_arc_bbox(a);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


void pcb_arc_set_radii(pcb_layer_t *Layer, pcb_arc_t *a, pcb_coord_t new_width, pcb_coord_t new_height)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, a);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_undo_add_obj_to_change_radii(PCB_TYPE_ARC, a, a, a);
	a->Width = new_width;
	a->Height = new_height;
	pcb_arc_bbox(a);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, a);
}


/* creates a new arc on a layer */
pcb_arc_t *pcb_arc_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t width, pcb_coord_t height, pcb_angle_t sa, pcb_angle_t dir, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_arc_t *Arc;

	PCB_ARC_LOOP(Layer);
	{
		if (arc->X == X1 && arc->Y == Y1 && arc->Width == width &&
				pcb_normalize_angle(arc->StartAngle) == pcb_normalize_angle(sa) && arc->Delta == dir)
			return (NULL);						/* prevent stacked arcs */
	}
	PCB_END_LOOP;
	Arc = pcb_arc_alloc(Layer);
	if (!Arc)
		return (Arc);

	Arc->ID = pcb_create_ID_get();
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
	pcb_arc_bbox(Arc);
	if (!Layer->arc_tree)
		Layer->arc_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
}



void pcb_arc_free(pcb_arc_t * data)
{
	arclist_remove(data);
	free(data);
}


int pcb_arc_eq(const pcb_element_t *e1, const pcb_arc_t *a1, const pcb_element_t *e2, const pcb_arc_t *a2)
{
	if (pcb_field_neq(a1, a2, Thickness) || pcb_field_neq(a1, a2, Clearance)) return 0;
	if (pcb_field_neq(a1, a2, Width) || pcb_field_neq(a1, a2, Height)) return 0;
	if (pcb_element_neq_offsx(e1, a1, e2, a2, X) || pcb_element_neq_offsy(e1, a1, e2, a2, Y)) return 0;
	if (pcb_field_neq(a1, a2, StartAngle) || pcb_field_neq(a1, a2, Delta)) return 0;

	return 1;
}

unsigned int pcb_arc_hash(const pcb_element_t *e, const pcb_arc_t *a)
{
	return 
		pcb_hash_coord(a->Thickness) ^ pcb_hash_coord(a->Clearance) ^
		pcb_hash_coord(a->Width) ^ pcb_hash_coord(a->Height) ^
		pcb_hash_element_ox(e, a->X) ^ pcb_hash_element_oy(e, a->Y) ^
		pcb_hash_coord(a->StartAngle) ^ pcb_hash_coord(a->Delta);
}

pcb_coord_t pcb_arc_length(const pcb_arc_t *arc)
{
	double da = arc->Delta;
	double r = ((double)arc->Width + (double)arc->Height) / 2.0; /* TODO: lame approximation */
	if (da < 0)
		da = -da;
	while(da > 360.0)
		da = 360.0;
	return pcb_round(2.0*r*M_PI*da/360.0);
}

double pcb_arc_area(const pcb_arc_t *arc)
{
	return
		(pcb_arc_length(arc) * (double)arc->Thickness /* body */
		+ (double)arc->Thickness * (double)arc->Thickness * (double)M_PI); /* cap circles */
}

pcb_box_t pcb_arc_mini_bbox(const pcb_arc_t *arc)
{
	return pcb_arc_bbox_(arc, 1);
}

/***** operations *****/

/* copies an arc to buffer */
void *AddArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, Layer)];

	return (pcb_arc_new(layer, Arc->X, Arc->Y,
		Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta,
		Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves an arc to buffer */
void *MoveArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_arc_t * arc)
{
	pcb_layer_t *lay = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, layer)];

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_ARC, layer, arc);
	pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);

	arclist_remove(arc);
	arclist_append(&lay->Arc, arc);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, arc);

	if (!lay->arc_tree)
		lay->arc_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(lay->arc_tree, (pcb_box_t *) arc, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_ARC, lay, arc);
	return (arc);
}

/* changes the size of an arc */
void *ChangeArcSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Arc->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return (NULL);
	if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != Arc->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Thickness = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
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
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Arc->Clearance) {
		pcb_undo_add_obj_to_clear_size(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Clearance = value;
		if (Arc->Clearance == 0) {
			PCB_FLAG_CLEAR(PCB_FLAG_CLEARLINE, Arc);
			Arc->Clearance = PCB_MIL_TO_COORD(10);
		}
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
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
	value = MIN(PCB_MAX_ARCSIZE, MAX(value, PCB_MIN_ARCSIZE));
	if (value != *dst) {
		pcb_undo_add_obj_to_change_radii(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
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
		pcb_undo_add_obj_to_change_angles(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
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
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		pcb_undo_add_obj_to_clear_poly(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_false);
	}
	pcb_undo_add_obj_to_flag(PCB_TYPE_ARC, Layer, Arc, Arc);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Arc);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc)) {
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		pcb_undo_add_obj_to_clear_poly(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_true);
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

	arc = pcb_arc_new(Layer, Arc->X + ctx->copy.DeltaX,
														Arc->Y + ctx->copy.DeltaY, Arc->Width, Arc->Height, Arc->StartAngle,
														Arc->Delta, Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND));
	if (!arc)
		return (arc);
	DrawArc(Layer, arc);
	pcb_undo_add_obj_to_create(PCB_TYPE_ARC, Layer, arc, arc);
	return (arc);
}

/* moves an arc */
void *MoveArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	if (Layer->On) {
		EraseArc(Arc);
		pcb_arc_move(Arc, ctx->move.dx, ctx->move.dy);
		DrawArc(Layer, Arc);
		pcb_draw();
	}
	else {
		pcb_arc_move(Arc, ctx->move.dx, ctx->move.dy);
	}
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	return (Arc);
}

/* moves an arc between layers; lowlevel routines */
void *MoveArcToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_arc_t * arc, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->arc_tree, (pcb_box_t *) arc);

	arclist_remove(arc);
	arclist_append(&Destination->Arc, arc);

	if (!Destination->arc_tree)
		Destination->arc_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Destination->arc_tree, (pcb_box_t *) arc, 0);
	return arc;
}


/* moves an arc between layers */
void *MoveArcToLayer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_arc_t * Arc)
{
	pcb_arc_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc)) {
		pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->On) {
		DrawArc(Layer, Arc);
		pcb_draw();
	}
	if (((long int) ctx->move.dst_layer == -1) || ctx->move.dst_layer == Layer)
		return (Arc);
	pcb_undo_add_obj_to_move_to_layer(PCB_TYPE_ARC, Layer, Arc, Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	if (Layer->On)
		EraseArc(Arc);
	newone = (pcb_arc_t *) MoveArcToLayerLowLevel(ctx, Layer, Arc, ctx->move.dst_layer);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, ctx->move.dst_layer, Arc);
	if (ctx->move.dst_layer->On)
		DrawArc(ctx->move.dst_layer, newone);
	pcb_draw();
	return (newone);
}

/* destroys an arc from a layer */
void *DestroyArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);

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
	pcb_undo_move_obj_to_remove(PCB_TYPE_ARC, Layer, Arc, Arc);
	return NULL;
}

void *RemoveArcPoint_op(pcb_opctx_t *ctx, pcb_layer_t *l, pcb_arc_t *a, int *end_id)
{
	return RemoveArc_op(ctx, l, a);
}

void *pcb_arc_destroy(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveArc_op(&ctx, Layer, Arc);
}

/* rotates an arc */
void pcb_arc_rotate90(pcb_arc_t *Arc, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_coord_t save;

	/* add Number*90 degrees (i.e., Number quarter-turns) */
	Arc->StartAngle = pcb_normalize_angle(Arc->StartAngle + Number * 90);
	PCB_COORD_ROTATE90(Arc->X, Arc->Y, X, Y, Number);

	/* now change width and height */
	if (Number == 1 || Number == 3) {
		save = Arc->Width;
		Arc->Width = Arc->Height;
		Arc->Height = save;
	}
	pcb_box_rotate90(&Arc->BoundingBox, X, Y, Number);
}

void pcb_arc_rotate(pcb_layer_t *layer, pcb_arc_t *arc, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle)
{
	pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	pcb_rotate(&arc->X, &arc->Y, X, Y, cosa, sina);
	arc->StartAngle = pcb_normalize_angle(arc->StartAngle + angle);
	pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
}

void pcb_arc_mirror(pcb_layer_t *layer, pcb_arc_t *arc)
{
	pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	arc->X = PCB_SWAP_X(arc->X);
	arc->Y = PCB_SWAP_Y(arc->Y);
	arc->StartAngle = PCB_SWAP_ANGLE(arc->StartAngle);
	arc->Delta = PCB_SWAP_DELTA(arc->Delta);
	pcb_arc_bbox(arc);
	pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
}

void pcb_arc_flip_side(pcb_layer_t *layer, pcb_arc_t *arc)
{
	pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	arc->X = PCB_SWAP_X(arc->X);
	arc->Y = PCB_SWAP_Y(arc->Y);
	arc->StartAngle = PCB_SWAP_ANGLE(arc->StartAngle);
	arc->Delta = PCB_SWAP_DELTA(arc->Delta);
	pcb_arc_bbox(arc);
	pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
}

/* rotates an arc */
void *Rotate90Arc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	EraseArc(Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_arc_rotate90(Arc, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
	DrawArc(Layer, Arc);
	pcb_draw();
	return (Arc);
}

void *pcb_arc_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *arc)
{
	pcb_angle_t end_ang = arc->StartAngle + arc->Delta;
	pcb_coord_t x = pcb_crosshair.X, y = pcb_crosshair.Y;
	pcb_angle_t angle = atan2(-(y - arc->Y), (x - arc->X)) * 180.0 / M_PI + 180.0;
	pcb_arc_t *new_arc;

	if (end_ang > 360.0)
		end_ang -= 360.0;
	if (end_ang < -360.0)
		end_ang += 360.0;

	if ((arc->Delta < 0) || (arc->Delta > 180))
		new_arc = pcb_arc_new(Layer, arc->X, arc->Y, arc->Width, arc->Height, angle, end_ang - angle + 360.0, arc->Thickness, arc->Clearance, arc->Flags);
	else
		new_arc = pcb_arc_new(Layer, arc->X, arc->Y, arc->Width, arc->Height, angle, end_ang - angle, arc->Thickness, arc->Clearance, arc->Flags);

	if (new_arc != NULL) {
		PCB_FLAG_CHANGE(PCB_CHGFLG_SET, PCB_FLAG_FOUND, new_arc);
		if (arc->Delta < 0)
			pcb_arc_set_angles(Layer, arc, arc->StartAngle, angle - arc->StartAngle - 360.0);
		else
			pcb_arc_set_angles(Layer, arc, arc->StartAngle, angle - arc->StartAngle);
	}
	return new_arc;
}

/*** draw ***/
void _draw_arc(pcb_arc_t * arc)
{
	if (!arc->Thickness)
		return;

	PCB_DRAW_BBOX(arc);

	if (conf_core.editor.thin_draw)
		pcb_gui->set_line_width(Output.fgGC, 0);
	else
		pcb_gui->set_line_width(Output.fgGC, arc->Thickness);
	pcb_gui->set_line_cap(Output.fgGC, Trace_Cap);

	pcb_gui->draw_arc(Output.fgGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
}

void draw_arc(pcb_layer_t * layer, pcb_arc_t * arc)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, arc))
		color = conf_core.appearance.color.warn;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, arc)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc))
			color = layer->SelectedColor;
		else
			color = conf_core.appearance.color.connected;
	}
	else
		color = layer->Color;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, arc)) {
		assert(color != NULL);
		pcb_lighten_color(color, buf, 1.75);
		color = buf;
	}
	pcb_gui->set_color(Output.fgGC, color);
	_draw_arc(arc);
}

pcb_r_dir_t draw_arc_callback(const pcb_box_t * b, void *cl)
{
	draw_arc((pcb_layer_t *) cl, (pcb_arc_t *) b);
	return PCB_R_DIR_FOUND_CONTINUE;
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
