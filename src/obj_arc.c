/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "draw_wireframe.h"
#include "hid_inlines.h"

#include "obj_arc.h"
#include "obj_arc_op.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: could be removed if draw.c could be split up */
#include "draw.h"
#include "obj_arc_draw.h"


static int pcb_arc_end_addr = 1;
int *pcb_arc_start_ptr = NULL, *pcb_arc_end_ptr = &pcb_arc_end_addr;

void pcb_arc_reg(pcb_layer_t *layer, pcb_arc_t *arc)
{
	arclist_append(&layer->Arc, arc);
	PCB_SET_PARENT(arc, layer, layer);

	if (layer->parent_type == PCB_PARENT_UI)
		return;

	assert(layer->parent_type == PCB_PARENT_DATA);
	pcb_obj_id_reg(layer->parent.data, arc);
}

void pcb_arc_unreg(pcb_arc_t *arc)
{
	pcb_layer_t *layer = arc->parent.layer;
	assert(arc->parent_type == PCB_PARENT_LAYER);
	arclist_remove(arc);
	assert(layer->parent_type == PCB_PARENT_DATA);
	pcb_obj_id_del(layer->parent.data, arc);
	PCB_CLEAR_PARENT(arc);
}

pcb_arc_t *pcb_arc_alloc_id(pcb_layer_t *layer, long int id)
{
	pcb_arc_t *new_obj;

	new_obj = calloc(sizeof(pcb_arc_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_ARC;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_arc_reg(layer, new_obj);

	return new_obj;
}

pcb_arc_t *pcb_arc_alloc(pcb_layer_t *layer)
{
	return pcb_arc_alloc_id(layer, pcb_create_ID_get());
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
	Arc->bbox_naked  = pcb_arc_bbox_(Arc, 1);
}


void pcb_arc_get_end(pcb_arc_t *Arc, int which, pcb_coord_t *x, pcb_coord_t *y)
{
	if (which == 0) {
		*x = pcb_round((double)Arc->X - (double)Arc->Width * cos(Arc->StartAngle * PCB_M180));
		*y = pcb_round((double)Arc->Y + (double)Arc->Height * sin(Arc->StartAngle * PCB_M180));
	}
	else {
		*x = pcb_round((double)Arc->X - (double)Arc->Width * cos((Arc->StartAngle + Arc->Delta) * PCB_M180));
		*y = pcb_round((double)Arc->Y + (double)Arc->Height * sin((Arc->StartAngle + Arc->Delta) * PCB_M180));
	}
}

void pcb_arc_set_angles(pcb_layer_t *Layer, pcb_arc_t *a, pcb_angle_t new_sa, pcb_angle_t new_da)
{
	if (new_da >= 360) {
		new_da = 360;
		new_sa = 0;
	}
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, a);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_undo_add_obj_to_change_angles(PCB_OBJ_ARC, a, a, a);
	a->StartAngle = new_sa;
	a->Delta = new_da;
	pcb_arc_bbox(a);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, a);
}


void pcb_arc_set_radii(pcb_layer_t *Layer, pcb_arc_t *a, pcb_coord_t new_width, pcb_coord_t new_height)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, a);
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_undo_add_obj_to_change_radii(PCB_OBJ_ARC, a, a, a);
	a->Width = new_width;
	a->Height = new_height;
	pcb_arc_bbox(a);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, a);
}


/* creates a new arc on a layer */
pcb_arc_t *pcb_arc_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t width, pcb_coord_t height, pcb_angle_t sa, pcb_angle_t dir, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags, pcb_bool prevent_dups)
{
	pcb_arc_t *Arc;

	if ((prevent_dups) && (Layer->arc_tree != NULL)) {
		pcb_rtree_it_t it;
		pcb_any_obj_t *o;
		pcb_box_t b;

		b.X1 = X1 - width;
		b.Y1 = Y1 - height;
		b.X2 = X1 + width;
		b.Y2 = Y1 + height;

		for(o = pcb_rtree_first(&it, Layer->arc_tree, (pcb_rtree_box_t *)&b); o != NULL; o = pcb_rtree_next(&it)) {
			pcb_arc_t *arc = (pcb_arc_t *)o;
TODO(": this does not catch all cases; there are more ways two arcs can be the same because of the angles")
			if (arc->X == X1 && arc->Y == Y1 && arc->Width == width && pcb_normalize_angle(arc->StartAngle) == pcb_normalize_angle(sa) && arc->Delta == dir)
				return NULL; /* prevent stacked arcs */
		}
	}

	Arc = pcb_arc_alloc(Layer);
	if (!Arc)
		return Arc;

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
	return Arc;
}

static pcb_arc_t *pcb_arc_copy_meta(pcb_arc_t *dst, pcb_arc_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}


pcb_arc_t *pcb_arc_dup(pcb_layer_t *dst, pcb_arc_t *src)
{
	pcb_arc_t *a = pcb_arc_new(dst, src->X, src->Y, src->Width, src->Height, src->StartAngle, src->Delta, src->Thickness, src->Clearance, src->Flags, pcb_false);
	pcb_arc_copy_meta(a, src);
	return a;
}

pcb_arc_t *pcb_arc_dup_at(pcb_layer_t *dst, pcb_arc_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_arc_t *a = pcb_arc_new(dst, src->X+dx, src->Y+dy, src->Width, src->Height, src->StartAngle, src->Delta, src->Thickness, src->Clearance, src->Flags, pcb_false);
	pcb_arc_copy_meta(a, src);
	return a;
}


void pcb_add_arc_on_layer(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_arc_bbox(Arc);
	if (!Layer->arc_tree)
		Layer->arc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	Arc->type = PCB_OBJ_ARC;
	PCB_SET_PARENT(Arc, layer, Layer);
}



void pcb_arc_free(pcb_arc_t *arc)
{
	pcb_arc_unreg(arc);
	free(arc);
}


int pcb_arc_eq(const pcb_host_trans_t *tr1, const pcb_arc_t *a1, const pcb_host_trans_t *tr2, const pcb_arc_t *a2)
{
	double sgn1 = tr1->on_bottom ? -1 : +1;
	double sgn2 = tr2->on_bottom ? -1 : +1;

	if (pcb_field_neq(a1, a2, Thickness) || pcb_field_neq(a1, a2, Clearance)) return 0;
	if (pcb_field_neq(a1, a2, Width) || pcb_field_neq(a1, a2, Height)) return 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, a1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, a2)) {
		if (pcb_neq_tr_coords(tr1, a1->X, a1->Y, tr2, a2->X, a2->Y)) return 0;
		if (pcb_normalize_angle(pcb_round(a1->StartAngle * sgn1 + tr1->rot * sgn1)) != pcb_normalize_angle(pcb_round(a2->StartAngle * sgn2 + tr2->rot * sgn2))) return 0;
		if (pcb_round(a1->Delta * sgn1) != pcb_round(a2->Delta * sgn2)) return 0;
	}

	return 1;
}

unsigned int pcb_arc_hash(const pcb_host_trans_t *tr, const pcb_arc_t *a)
{
	unsigned int crd = 0;
	double sgn = tr->on_bottom ? -1 : +1;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, a)) {
		pcb_coord_t x, y;
		pcb_hash_tr_coords(tr, &x, &y, a->X, a->Y);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y) ^
			pcb_hash_coord(pcb_normalize_angle(pcb_round(a->StartAngle*sgn + tr->rot*sgn))) ^ pcb_hash_coord(pcb_round(a->Delta * sgn));
	}

	return
		pcb_hash_coord(a->Thickness) ^ pcb_hash_coord(a->Clearance) ^
		pcb_hash_coord(a->Width) ^ pcb_hash_coord(a->Height) ^ crd;
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

void pcb_arc_pre(pcb_arc_t *arc)
{
	pcb_layer_t *ly = pcb_layer_get_real(arc->parent.layer);
	if (ly == NULL)
		return;
	if (ly->arc_tree != NULL)
		pcb_r_delete_entry(ly->arc_tree, (pcb_box_t *)arc);
	pcb_poly_restore_to_poly(ly->parent.data, PCB_OBJ_ARC, ly, arc);
}

void pcb_arc_post(pcb_arc_t *arc)
{
	pcb_layer_t *ly = pcb_layer_get_real(arc->parent.layer);
	if (ly == NULL)
		return;
	if (ly->arc_tree != NULL)
		pcb_r_insert_entry(ly->arc_tree, (pcb_box_t *)arc);
	pcb_poly_clear_from_poly(ly->parent.data, PCB_OBJ_ARC, ly, arc);
}


/***** operations *****/

/* copies an arc to buffer */
void *pcb_arcop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, Layer)];
	pcb_arc_t *a = pcb_arc_new(layer, Arc->X, Arc->Y,
		Arc->Width, Arc->Height, Arc->StartAngle, Arc->Delta,
		Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg), pcb_false);

	pcb_arc_copy_meta(a, Arc);
	return a;
}

/* moves an arc between board and buffer */
void *pcb_arcop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_arc_t *arc)
{
	pcb_layer_t *srcly = arc->parent.layer;

	assert(arc->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) /* auto layer in dst */
		dstly = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, srcly)];

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_OBJ_ARC, srcly, arc);
	pcb_r_delete_entry(srcly->arc_tree, (pcb_box_t *) arc);

	pcb_arc_unreg(arc);
	pcb_arc_reg(dstly, arc);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, arc);

	if (!dstly->arc_tree)
		dstly->arc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dstly->arc_tree, (pcb_box_t *) arc);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_OBJ_ARC, dstly, arc);

	return arc;
}

/* changes the size of an arc */
void *pcb_arcop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Arc->Thickness + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return NULL;
	if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != Arc->Thickness) {
		pcb_undo_add_obj_to_size(PCB_OBJ_ARC, Layer, Arc, Arc);
		pcb_arc_invalidate_erase(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		Arc->Thickness = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_arc_invalidate_draw(Layer, Arc);
		return Arc;
	}
	return NULL;
}

/* changes the clearance size of an arc */
void *pcb_arcop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Arc->Clearance + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return NULL;
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (!ctx->chgsize.is_absolute && ctx->chgsize.value < 0 && value < conf_core.design.bloat * 2)
		value = 0;
	if (ctx->chgsize.value > 0 && value < conf_core.design.bloat * 2)
		value = conf_core.design.bloat * 2 + 2;
	if (value != Arc->Clearance) {
		pcb_undo_add_obj_to_clear_size(PCB_OBJ_ARC, Layer, Arc, Arc);
		pcb_arc_invalidate_erase(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		Arc->Clearance = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_arc_invalidate_draw(Layer, Arc);
		return Arc;
	}
	return NULL;
}

/* changes the radius of an arc (is_primary 0=width or 1=height or 2=both) */
void *pcb_arcop_change_radius(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_coord_t value, *dst;
	void *a0, *a1;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return NULL;

	switch(ctx->chgsize.is_primary) {
		case 0: dst = &Arc->Width; break;
		case 1: dst = &Arc->Height; break;
		case 2:
			ctx->chgsize.is_primary = 0; a0 = pcb_arcop_change_radius(ctx, Layer, Arc);
			ctx->chgsize.is_primary = 1; a1 = pcb_arcop_change_radius(ctx, Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : (*dst) + ctx->chgsize.value;
	value = MIN(PCB_MAX_ARCSIZE, MAX(value, PCB_MIN_ARCSIZE));
	if (value != *dst) {
		pcb_undo_add_obj_to_change_radii(PCB_OBJ_ARC, Layer, Arc, Arc);
		pcb_arc_invalidate_erase(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		*dst = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_arc_invalidate_draw(Layer, Arc);
		return Arc;
	}
	return NULL;
}

/* changes the angle of an arc (is_primary 0=start or 1=end) */
void *pcb_arcop_change_angle(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_angle_t value, *dst;
	void *a0, *a1;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return NULL;

	switch(ctx->chgangle.is_primary) {
		case 0: dst = &Arc->StartAngle; break;
		case 1: dst = &Arc->Delta; break;
		case 2:
			ctx->chgangle.is_primary = 0; a0 = pcb_arcop_change_angle(ctx, Layer, Arc);
			ctx->chgangle.is_primary = 1; a1 = pcb_arcop_change_angle(ctx, Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (ctx->chgangle.is_absolute) ? ctx->chgangle.value : (*dst) + ctx->chgangle.value;
	if ((value > 360.0) || (value < -360.0))
		value = fmod(value, 360.0);

	if (value != *dst) {
		pcb_undo_add_obj_to_change_angles(PCB_OBJ_ARC, Layer, Arc, Arc);
		pcb_arc_invalidate_erase(Arc);
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		*dst = value;
		pcb_arc_bbox(Arc);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_arc_invalidate_draw(Layer, Arc);
		return Arc;
	}
	return NULL;
}

/* changes the clearance flag of an arc */
void *pcb_arcop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc))
		return NULL;
	pcb_arc_invalidate_erase(Arc);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc)) {
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_ARC, Layer, Arc, Arc, pcb_false);
	}
	pcb_undo_add_obj_to_flag(Arc);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Arc);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc)) {
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_ARC, Layer, Arc, Arc, pcb_true);
	}
	pcb_arc_invalidate_draw(Layer, Arc);
	return Arc;
}

/* sets the clearance flag of an arc */
void *pcb_arcop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return NULL;
	return pcb_arcop_change_join(ctx, Layer, Arc);
}

/* clears the clearance flag of an arc */
void *pcb_arcop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Arc))
		return NULL;
	return pcb_arcop_change_join(ctx, Layer, Arc);
}

/* copies an arc */
void *pcb_arcop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_arc_t *arc;

	arc = pcb_arc_new(Layer, Arc->X + ctx->copy.DeltaX,
														Arc->Y + ctx->copy.DeltaY, Arc->Width, Arc->Height, Arc->StartAngle,
														Arc->Delta, Arc->Thickness, Arc->Clearance, pcb_flag_mask(Arc->Flags, PCB_FLAG_FOUND), pcb_true);
	if (!arc)
		return arc;
	pcb_arc_copy_meta(arc, Arc);
	pcb_arc_invalidate_draw(Layer, arc);
	pcb_undo_add_obj_to_create(PCB_OBJ_ARC, Layer, arc, arc);
	return arc;
}

/* moves an arc */
void *pcb_arcop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (Layer->meta.real.vis) {
		pcb_arc_invalidate_erase(Arc);
		pcb_arc_move(Arc, ctx->move.dx, ctx->move.dy);
		pcb_arc_invalidate_draw(Layer, Arc);
	}
	else {
		pcb_arc_move(Arc, ctx->move.dx, ctx->move.dy);
	}
	return Arc;
}

void *pcb_arcop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	pcb_arcop_move_noclip(ctx, Layer, Arc);
	pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	return Arc;
}

void *pcb_arcop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	if (ctx->clip.restore) {
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	}
	return Arc;
}

/* moves an arc between layers; lowlevel routines */
void *pcb_arcop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_arc_t * arc, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->arc_tree, (pcb_box_t *) arc);

	pcb_arc_unreg(arc);
	pcb_arc_reg(Destination, arc);

	if (!Destination->arc_tree)
		Destination->arc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Destination->arc_tree, (pcb_box_t *) arc);

	return arc;
}


/* moves an arc between layers */
void *pcb_arcop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_arc_t * Arc)
{
	pcb_arc_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Arc)) {
		pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->meta.real.vis)
		pcb_arc_invalidate_draw(Layer, Arc);
	if (ctx->move.dst_layer == Layer)
		return Arc;
	pcb_undo_add_obj_to_move_to_layer(PCB_OBJ_ARC, Layer, Arc, Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	if (Layer->meta.real.vis)
		pcb_arc_invalidate_erase(Arc);
	newone = (pcb_arc_t *) pcb_arcop_move_to_layer_low(ctx, Layer, Arc, ctx->move.dst_layer);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, ctx->move.dst_layer, Arc);
	if (ctx->move.dst_layer->meta.real.vis)
		pcb_arc_invalidate_draw(ctx->move.dst_layer, newone);
	return newone;
}

/* destroys an arc from a layer */
void *pcb_arcop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);

	pcb_arc_free(Arc);
	return NULL;
}

/* removes an arc from a layer */
void *pcb_arcop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	/* erase from screen */
	if (Layer->meta.real.vis)
		pcb_arc_invalidate_erase(Arc);
	pcb_undo_move_obj_to_remove(PCB_OBJ_ARC, Layer, Arc, Arc);
	return NULL;
}

void *pcb_arcop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *l, pcb_arc_t *a, int *end_id)
{
	return pcb_arcop_remove(ctx, l, a);
}

void *pcb_arc_destroy(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	res = pcb_arcop_remove(&ctx, Layer, Arc);
	pcb_draw();
	return res;
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

	/* can't optimize with box rotation due to closed boxes */
	pcb_arc_bbox(Arc);
}

void pcb_arc_rotate(pcb_layer_t *layer, pcb_arc_t *arc, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle)
{
	if (layer->arc_tree != NULL)
		pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	pcb_rotate(&arc->X, &arc->Y, X, Y, cosa, sina);
	arc->StartAngle = pcb_normalize_angle(arc->StartAngle + angle);
	if (layer->arc_tree != NULL)
		pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc);
}

void pcb_arc_mirror(pcb_layer_t *layer, pcb_arc_t *arc, pcb_coord_t y_offs)
{
	if (layer->arc_tree != NULL)
		pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	arc->X = PCB_SWAP_X(arc->X);
	arc->Y = PCB_SWAP_Y(arc->Y) + y_offs;
	arc->StartAngle = PCB_SWAP_ANGLE(arc->StartAngle);
	arc->Delta = PCB_SWAP_DELTA(arc->Delta);
	pcb_arc_bbox(arc);
	if (layer->arc_tree != NULL)
		pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc);
}

void pcb_arc_flip_side(pcb_layer_t *layer, pcb_arc_t *arc)
{
	pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
	arc->X = PCB_SWAP_X(arc->X);
	arc->Y = PCB_SWAP_Y(arc->Y);
	arc->StartAngle = PCB_SWAP_ANGLE(arc->StartAngle);
	arc->Delta = PCB_SWAP_DELTA(arc->Delta);
	pcb_arc_bbox(arc);
	pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc);
}

void pcb_arc_scale(pcb_arc_t *arc, double sx, double sy, double sth)
{
	int onbrd = (arc->parent.layer != NULL) && (!arc->parent.layer->is_bound);

	if (onbrd)
		pcb_arc_pre(arc);

	if (sx != 1.0) {
		arc->X = pcb_round((double)arc->X * sx);
		arc->Width = pcb_round((double)arc->Width * sx);
	}

	if (sy != 1.0) {
		arc->Y = pcb_round((double)arc->Y * sy);
		arc->Height = pcb_round((double)arc->Height * sy);
	}

	if (sth != 1.0)
		arc->Thickness = pcb_round((double)arc->Thickness * sth);

	pcb_arc_bbox(arc);
	if (onbrd)
		pcb_arc_post(arc);
}

/* rotates an arc */
void *pcb_arcop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_arc_invalidate_erase(Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	if (Layer->arc_tree != NULL)
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_arc_rotate90(Arc, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->arc_tree != NULL)
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	pcb_arc_invalidate_draw(Layer, Arc);
	return Arc;
}

void *pcb_arcop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_arc_invalidate_erase(Arc);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	pcb_arc_rotate(Layer, Arc, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina, ctx->rotate.angle);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, Layer, Arc);
	pcb_arc_invalidate_draw(Layer, Arc);
	return Arc;
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
		new_arc = pcb_arc_new(Layer, arc->X, arc->Y, arc->Width, arc->Height, angle, end_ang - angle + 360.0, arc->Thickness, arc->Clearance, arc->Flags, pcb_true);
	else
		new_arc = pcb_arc_new(Layer, arc->X, arc->Y, arc->Width, arc->Height, angle, end_ang - angle, arc->Thickness, arc->Clearance, arc->Flags, pcb_true);

	if (new_arc != NULL) {
		pcb_arc_copy_meta(new_arc, arc);
		PCB_FLAG_CHANGE(PCB_CHGFLG_SET, PCB_FLAG_FOUND, new_arc);
		if (arc->Delta < 0)
			pcb_arc_set_angles(Layer, arc, arc->StartAngle, angle - arc->StartAngle - 360.0);
		else
			pcb_arc_set_angles(Layer, arc, arc->StartAngle, angle - arc->StartAngle);
	}
	return new_arc;
}

void *pcb_arcop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	static pcb_flag_values_t pcb_arc_flags = 0;
	if (pcb_arc_flags == 0)
		pcb_arc_flags = pcb_obj_valid_flags(PCB_OBJ_ARC);

	if ((ctx->chgflag.flag & pcb_arc_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (Arc->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(Arc);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_restore_to_poly(ctx->chgflag.pcb->Data, PCB_OBJ_ARC, Arc->parent.layer, Arc);

	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Arc);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_clear_from_poly(ctx->chgflag.pcb->Data, PCB_OBJ_ARC, Arc->parent.layer, Arc);

	return Arc;
}

void *pcb_arcop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_arc_t *arc)
{
	pcb_arc_name_invalidate_draw(arc);
	return arc;
}


/*** draw ***/

static void arc_label_pos(const pcb_arc_t *arc, pcb_coord_t *x0, pcb_coord_t *y0, pcb_bool_t *vert)
{
	double da, ea, la;

	da = PCB_CLAMP(arc->Delta, -360, 360);
	ea = arc->StartAngle + da;
	while(ea < -360) ea += 360;
	while(ea > +360) ea -= 360;

	la = (arc->StartAngle+ea)/2.0;

	*x0 = pcb_round((double)arc->X - (double)arc->Width * cos(la * PCB_M180));
	*y0 = pcb_round((double)arc->Y + (double)arc->Height * sin(la * PCB_M180));
	*vert = (((la < 45) && (la > -45)) || ((la > 135) && (la < 225)));
}

void pcb_arc_middle(const pcb_arc_t *arc, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_bool_t waste;
	arc_label_pos(arc, x, y, &waste);
}

void pcb_arc_name_invalidate_draw(pcb_arc_t *arc)
{
	if (arc->term != NULL) {
		pcb_text_t text;
		pcb_coord_t x0, y0;
		pcb_bool_t vert;

		arc_label_pos(arc, &x0, &y0, &vert);
		pcb_term_label_invalidate(x0, y0, 100.0, vert, pcb_true, arc->term, arc->intconn);
	}
}

void pcb_arc_draw_label(pcb_draw_info_t *info, pcb_arc_t *arc)
{
	if (arc->term != NULL) {
		pcb_coord_t x0, y0;
		pcb_bool_t vert;

		arc_label_pos(arc, &x0, &y0, &vert);
		pcb_term_label_draw(info, x0, y0, conf_core.appearance.term_label_size, vert, pcb_true, arc->term, arc->intconn);
	}
}

void pcb_arc_draw_(pcb_draw_info_t *info, pcb_arc_t *arc, int allow_term_gfx)
{
	pcb_coord_t thickness = arc->Thickness;

	if (!arc->Thickness)
		return;

	if (delayed_terms_enabled && (arc->term != NULL)) {
		pcb_draw_delay_obj_add((pcb_any_obj_t *)arc);
		return;
	}

	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0)) {
		thickness += info->xform->bloat;
		if (thickness < 1)
			thickness = 1;
	}

	PCB_DRAW_BBOX(arc);

	if (!conf_core.editor.thin_draw && !conf_core.editor.wireframe_draw)
	{
		if ((allow_term_gfx) && pcb_draw_term_need_gfx(arc) && pcb_draw_term_hid_permission()) {
			pcb_hid_set_line_cap(pcb_draw_out.active_padGC, pcb_cap_round);
			pcb_hid_set_line_width(pcb_draw_out.active_padGC, thickness);
			pcb_gui->draw_arc(pcb_draw_out.active_padGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
			pcb_hid_set_line_width(pcb_draw_out.fgGC, PCB_DRAW_TERM_GFX_WIDTH);
		}
		else
			pcb_hid_set_line_width(pcb_draw_out.fgGC, thickness);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
		pcb_gui->draw_arc(pcb_draw_out.fgGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
	}
	else
	{
		pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);

		if(conf_core.editor.thin_draw)
			pcb_gui->draw_arc(pcb_draw_out.fgGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);

		if(conf_core.editor.wireframe_draw)
			pcb_draw_wireframe_arc(pcb_draw_out.fgGC, arc, thickness);
	}
	if (arc->term != NULL) {
		if ((pcb_draw_force_termlab) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, arc))
			pcb_draw_delay_label_add((pcb_any_obj_t *)arc);
	}
}

static void pcb_arc_draw(pcb_draw_info_t *info, pcb_arc_t *arc, int allow_term_gfx)
{
	const pcb_color_t *color;
	pcb_color_t buf;
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(arc->parent.layer);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = arc->parent.layer;

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, arc))
		color = &conf_core.appearance.color.warn;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, arc)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc)) {
			if (layer->is_bound)
				PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
			else
				color = &conf_core.appearance.color.selected;
		}
		else
			color = &conf_core.appearance.color.connected;
	}
	else if (PCB_HAS_COLOROVERRIDE(arc)) {
		color = arc->override_color;
	}
	else if (layer->is_bound)
		PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 0);
	else
		color = &layer->meta.real.color;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, arc)) {
		pcb_lighten_color(color, &buf, 1.75);
		color = &buf;
	}
	pcb_gui->set_color(pcb_draw_out.fgGC, color);
	pcb_arc_draw_(info, arc, allow_term_gfx);
}

pcb_r_dir_t pcb_arc_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *)b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(arc->parent_type, &arc->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_arc_draw(info, arc, 0);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_arc_draw_term_callback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *)b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(arc->parent_type, &arc->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_arc_draw(info, arc, 1);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* erases an arc on a layer */
void pcb_arc_invalidate_erase(pcb_arc_t *Arc)
{
	if (!Arc->Thickness)
		return;
	pcb_draw_invalidate(Arc);
	pcb_flag_erase(&Arc->Flags);
}

void pcb_arc_invalidate_draw(pcb_layer_t *Layer, pcb_arc_t *Arc)
{
	pcb_draw_invalidate(Arc);
}
