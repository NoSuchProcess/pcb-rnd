/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Drawing primitive: custom graphics (e.g. pixmap) */


#include "config.h"

#include <librnd/core/compat_misc.h>
#include <librnd/core/hid_inlines.h>


#include "board.h"
#include "data.h"
#include "undo.h"
#include "rotate.h"
#include "move.h"
#include "conf_core.h"
#include "draw_wireframe.h"

#include "obj_gfx.h"
#include "obj_gfx_op.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: could be removed if draw.c could be split up */
#include "draw.h"
#include "obj_gfx_draw.h"


void pcb_gfx_reg(pcb_layer_t *layer, pcb_gfx_t *gfx)
{
	gfxlist_append(&layer->Gfx, gfx);
	PCB_SET_PARENT(gfx, layer, layer);

	if (layer->parent_type == PCB_PARENT_UI)
		return;

	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_obj_id_reg(layer->parent.data, gfx);
}

void pcb_gfx_unreg(pcb_gfx_t *gfx)
{
	pcb_layer_t *layer = gfx->parent.layer;
	assert(gfx->parent_type == PCB_PARENT_LAYER);
	gfxlist_remove(gfx);
	if (layer->parent_type != PCB_PARENT_UI) {
		assert(layer->parent_type == PCB_PARENT_DATA);
		pcb_obj_id_del(layer->parent.data, gfx);
	}
	PCB_CLEAR_PARENT(gfx);
}

pcb_gfx_t *pcb_gfx_alloc_id(pcb_layer_t *layer, long int id)
{
	pcb_gfx_t *new_obj;

	new_obj = calloc(sizeof(pcb_gfx_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_GFX;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_gfx_reg(layer, new_obj);

	return new_obj;
}

pcb_gfx_t *pcb_gfx_alloc(pcb_layer_t *layer)
{
	return pcb_gfx_alloc_id(layer, pcb_create_ID_get());
}

/* computes the bounding box of a gfx */
static pcb_box_t pcb_gfx_bbox_(const pcb_gfx_t *gfx)
{
	pcb_box_t res = {0};
	int n;
	for(n = 0; n < 4; n++)
		pcb_box_bump_point(&res, gfx->cox[n], gfx->coy[n]);
	return res;
}


void pcb_gfx_bbox(pcb_gfx_t *gfx)
{
	gfx->bbox_naked = gfx->BoundingBox = pcb_gfx_bbox_(gfx);
}

void pcb_gfx_update(pcb_gfx_t *gfx)
{
	int n;
	double a, rx, ry, cosa, sina;

	rx = (double)gfx->sx / 2.0;
	ry = (double)gfx->sy / 2.0;

	gfx->cox[0] = pcb_round((double)gfx->cx + rx); gfx->coy[0] = pcb_round((double)gfx->cy + ry);
	gfx->cox[1] = pcb_round((double)gfx->cx - rx); gfx->coy[1] = pcb_round((double)gfx->cy + ry);
	gfx->cox[2] = pcb_round((double)gfx->cx - rx); gfx->coy[2] = pcb_round((double)gfx->cy - ry);
	gfx->cox[3] = pcb_round((double)gfx->cx + rx); gfx->coy[3] = pcb_round((double)gfx->cy - ry);
	if (gfx->rot != 0.0) {
		a = gfx->rot / PCB_RAD_TO_DEG;
		cosa = cos(a);
		sina = sin(a);
		for(n = 0; n < 4; n++)
			pcb_rotate(&gfx->cox[n], &gfx->coy[n], gfx->cx, gfx->cy, cosa, sina);
	}
}

/* creates a new gfx on a layer */
pcb_gfx_t *pcb_gfx_new(pcb_layer_t *layer, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy, pcb_angle_t rot, pcb_flag_t Flags)
{
	pcb_gfx_t *gfx;

	gfx = pcb_gfx_alloc(layer);
	if (!gfx)
		return gfx;

	gfx->Flags = Flags;
	gfx->cx = cx;
	gfx->cy = cy;
	gfx->sx = sx;
	gfx->sy = sy;
	gfx->rot = rot;
	pcb_gfx_update(gfx);
	pcb_add_gfx_on_layer(layer, gfx);
	return gfx;
}

static pcb_gfx_t *pcb_gfx_copy_meta(pcb_gfx_t *dst, pcb_gfx_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}


pcb_gfx_t *pcb_gfx_dup(pcb_layer_t *dst, pcb_gfx_t *src)
{
	pcb_gfx_t *a = pcb_gfx_new(dst, src->cx, src->cy, src->sx, src->sy, src->rot, src->Flags);
	TODO("copy the pixmap too");
	pcb_gfx_copy_meta(a, src);
	return a;
}

pcb_gfx_t *pcb_gfx_dup_at(pcb_layer_t *dst, pcb_gfx_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_gfx_t *a = pcb_gfx_new(dst, src->cx+dx, src->cy+dy, src->sx, src->sy, src->rot, src->Flags);
	TODO("copy the pixmap too");
	pcb_gfx_copy_meta(a, src);
	return a;
}


void pcb_add_gfx_on_layer(pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_bbox(gfx);
	if (!Layer->gfx_tree)
		Layer->gfx_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->gfx_tree, (pcb_box_t *)gfx);
	gfx->type = PCB_OBJ_GFX;
	PCB_SET_PARENT(gfx, layer, Layer);
}



void pcb_gfx_free(pcb_gfx_t *gfx)
{
	if ((gfx->parent.layer != NULL) && (gfx->parent.layer->gfx_tree != NULL))
		pcb_r_delete_entry(gfx->parent.layer->gfx_tree, (pcb_box_t *)gfx);
	pcb_attribute_free(&gfx->Attributes);
	pcb_gfx_unreg(gfx);
	free(gfx);
}


int pcb_gfx_eq(const pcb_host_trans_t *tr1, const pcb_gfx_t *a1, const pcb_host_trans_t *tr2, const pcb_gfx_t *a2)
{
	double sgn1 = tr1->on_bottom ? -1 : +1;
	double sgn2 = tr2->on_bottom ? -1 : +1;

	if (a1->sx != a2->sx) return 0;
	if (a1->sy != a2->sy) return 0;
	if (pcb_neq_tr_coords(tr1, a1->cx, a1->cy, tr2, a2->cx, a2->cy)) return 0;
	if (pcb_normalize_angle(pcb_round(a1->rot * sgn1)) != pcb_normalize_angle(pcb_round(a2->rot * sgn2))) return 0;
TODO("compare pixmaps");
	return 1;
}

unsigned int pcb_gfx_hash(const pcb_host_trans_t *tr, const pcb_gfx_t *a)
{
	unsigned int content = 0, crd = 0;
	double sgn = tr->on_bottom ? -1 : +1;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, a)) {
		pcb_coord_t x, y;
		pcb_hash_tr_coords(tr, &x, &y, a->cx, a->cy);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y) ^ pcb_hash_coord(a->sx) ^ pcb_hash_coord(a->sy) ^
			pcb_hash_coord(pcb_normalize_angle(pcb_round(a->rot*sgn + tr->rot*sgn)));
	}

TODO("hash pixmaps");

	return crd ^ content;
}

void pcb_gfx_pre(pcb_gfx_t *gfx)
{
	pcb_layer_t *ly = pcb_layer_get_real(gfx->parent.layer);
	if (ly == NULL)
		return;
	if (ly->gfx_tree != NULL)
		pcb_r_delete_entry(ly->gfx_tree, (pcb_box_t *)gfx);
}

void pcb_gfx_post(pcb_gfx_t *gfx)
{
	pcb_layer_t *ly = pcb_layer_get_real(gfx->parent.layer);
	pcb_gfx_update(gfx);
	if (ly == NULL)
		return;
	if (ly->gfx_tree != NULL)
		pcb_r_insert_entry(ly->gfx_tree, (pcb_box_t *)gfx);
}

/***** operations *****/

static const char core_gfx_cookie[] = "core-gfx";

typedef struct {
	pcb_gfx_t *gfx; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	pcb_coord_t cx, cy, sx, sy;
	pcb_angle_t rot;
} undo_gfx_geo_t;

static int undo_gfx_geo_swap(void *udata)
{
	undo_gfx_geo_t *g = udata;
	pcb_layer_t *layer = g->gfx->parent.layer;

	if (layer->gfx_tree != NULL)
		pcb_r_delete_entry(layer->gfx_tree, (pcb_box_t *)g->gfx);

	rnd_swap(pcb_coord_t, g->cx, g->gfx->cx);
	rnd_swap(pcb_coord_t, g->cy, g->gfx->cy);
	rnd_swap(pcb_coord_t, g->sx, g->gfx->sx);
	rnd_swap(pcb_coord_t, g->sy, g->gfx->sy);
	rnd_swap(pcb_angle_t, g->rot, g->gfx->rot);

	pcb_gfx_update(g->gfx);
	pcb_gfx_bbox(g->gfx);
	if (layer->gfx_tree != NULL)
		pcb_r_insert_entry(layer->gfx_tree, (pcb_box_t *)g->gfx);

	return 0;
}

static void undo_gfx_geo_print(void *udata, char *dst, size_t dst_len)
{
	pcb_snprintf(dst, dst_len, "gfx geo");
}

static const uundo_oper_t undo_gfx_geo = {
	core_gfx_cookie,
	NULL,
	undo_gfx_geo_swap,
	undo_gfx_geo_swap,
	undo_gfx_geo_print
};

/* copies a gfx to buffer */
void *pcb_gfxop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_t *a;
	pcb_layer_id_t lid = pcb_layer_id(ctx->buffer.src, Layer);
	pcb_layer_t *layer;

	/* the buffer may not have the specified layer, e.g. on loose subc subc-aux layer */
	if (lid == -1)
		return NULL;

	layer = &ctx->buffer.dst->Layer[lid];
	a = pcb_gfx_new(layer, gfx->cx, gfx->cy, gfx->sx, gfx->sy, gfx->rot,
		pcb_flag_mask(gfx->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));

	pcb_gfx_copy_meta(a, gfx);
	if (ctx->buffer.keep_id) pcb_obj_change_id((pcb_any_obj_t *)a, gfx->ID);
	return a;
}

/* moves a gfx between board and buffer */
void *pcb_gfxop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_gfx_t *gfx)
{
	pcb_layer_t *srcly = gfx->parent.layer;

	assert(gfx->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) { /* auto layer in dst */
		pcb_layer_id_t lid = pcb_layer_id(ctx->buffer.src, srcly);
		if (lid < 0) {
			pcb_message(PCB_MSG_ERROR, "Internal error: can't resolve source layer ID in pcb_gfxop_move_buffer\n");
			return NULL;
		}
		dstly = &ctx->buffer.dst->Layer[lid];
	}

	pcb_r_delete_entry(srcly->gfx_tree, (pcb_box_t *) gfx);

	pcb_gfx_unreg(gfx);
	pcb_gfx_reg(dstly, gfx);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, gfx);

	if (!dstly->gfx_tree)
		dstly->gfx_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dstly->gfx_tree, (pcb_box_t *) gfx);

	return gfx;
}

/* copies a gfx */
void *pcb_gfxop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_t *ngfx;

	ngfx = pcb_gfx_new(Layer, gfx->cx + ctx->copy.DeltaX, gfx->cy + ctx->copy.DeltaY,
		gfx->sx, gfx->sy, gfx->rot, pcb_flag_mask(gfx->Flags, PCB_FLAG_FOUND));
	if (ngfx == NULL)
		return NULL;
	if (ctx->copy.keep_id)
		ngfx->ID = gfx->ID;
	pcb_gfx_copy_meta(ngfx, gfx);
	pcb_gfx_invalidate_draw(Layer, ngfx);
	pcb_undo_add_obj_to_create(PCB_OBJ_GFX, Layer, ngfx, ngfx);
	return ngfx;
}


#define pcb_gfx_move(g,dx,dy)                                     \
	do {                                                            \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy);                     \
		pcb_gfx_t *__g__ = (g);                                       \
		PCB_MOVE_POINT((__g__)->cx, (__g__)->cy,__dx__,__dy__);          \
		PCB_BOX_MOVE_LOWLEVEL(&((__g__)->BoundingBox),__dx__,__dy__); \
		PCB_BOX_MOVE_LOWLEVEL(&((__g__)->bbox_naked),__dx__,__dy__); \
		pcb_gfx_update(__g__); \
	} while(0)

void *pcb_gfxop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	if (Layer->meta.real.vis) {
		pcb_gfx_invalidate_erase(gfx);
		pcb_gfx_move(gfx, ctx->move.dx, ctx->move.dy);
		pcb_gfx_invalidate_draw(Layer, gfx);
	}
	else {
		pcb_gfx_move(gfx, ctx->move.dx, ctx->move.dy);
	}
	return gfx;
}

void *pcb_gfxop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_r_delete_entry(Layer->gfx_tree, (pcb_box_t *)gfx);
	pcb_gfxop_move_noclip(ctx, Layer, gfx);
	pcb_r_insert_entry(Layer->gfx_tree, (pcb_box_t *)gfx);
	return gfx;
}

/* moves a gfx between layers; lowlevel routines */
void *pcb_gfxop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_gfx_t * gfx, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->gfx_tree, (pcb_box_t *)gfx);

	pcb_gfx_unreg(gfx);
	pcb_gfx_reg(Destination, gfx);

	if (!Destination->gfx_tree)
		Destination->gfx_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Destination->gfx_tree, (pcb_box_t *)gfx);

	return gfx;
}


/* moves a gfx between layers */
void *pcb_gfxop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_gfx_t * gfx)
{
	pcb_gfx_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, gfx)) {
		pcb_message(PCB_MSG_WARNING, "Sorry, gfx object is locked\n");
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->meta.real.vis)
		pcb_gfx_invalidate_draw(Layer, gfx);
	if (ctx->move.dst_layer == Layer)
		return gfx;
	pcb_undo_add_obj_to_move_to_layer(PCB_OBJ_GFX, Layer, gfx, gfx);
	if (Layer->meta.real.vis)
		pcb_gfx_invalidate_erase(gfx);
	newone = (pcb_gfx_t *) pcb_gfxop_move_to_layer_low(ctx, Layer, gfx, ctx->move.dst_layer);
	if (ctx->move.dst_layer->meta.real.vis)
		pcb_gfx_invalidate_draw(ctx->move.dst_layer, newone);
	return newone;
}

/* destroys a gfx from a layer */
void *pcb_gfxop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_r_delete_entry(Layer->gfx_tree, (pcb_box_t *) gfx);

	pcb_gfx_free(gfx);
	return NULL;
}

/* removes a gfx from a layer */
void *pcb_gfxop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	/* erase from screen */
	if (Layer->meta.real.vis)
		pcb_gfx_invalidate_erase(gfx);
	pcb_undo_move_obj_to_remove(PCB_OBJ_GFX, Layer, gfx, gfx);
	return NULL;
}

void *pcb_gfx_destroy(pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	res = pcb_gfxop_remove(&ctx, Layer, gfx);
	pcb_draw();
	return res;
}

/* rotates a gfx */
void pcb_gfx_rotate90(pcb_gfx_t *gfx, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	gfx->rot = pcb_normalize_angle(gfx->rot + Number * 90);
	TODO("rotate content")
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);
}

void pcb_gfx_rotate(pcb_layer_t *layer, pcb_gfx_t *gfx, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle)
{
	if (layer->gfx_tree != NULL)
		pcb_r_delete_entry(layer->gfx_tree, (pcb_box_t *) gfx);

	gfx->rot = pcb_normalize_angle(gfx->rot + angle);
	TODO("rotate content")
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);

	if (layer->gfx_tree != NULL)
		pcb_r_insert_entry(layer->gfx_tree, (pcb_box_t *) gfx);
}

void pcb_gfx_mirror(pcb_gfx_t *gfx, pcb_coord_t y_offs, pcb_bool undoable)
{
	undo_gfx_geo_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(gfx->parent.layer->parent.data), &undo_gfx_geo, sizeof(undo_gfx_geo_t));

	g->gfx = gfx;
	g->cx = gfx->cx;
	g->cy = gfx->cy;
	g->sx = gfx->sx;
	g->sy = gfx->sy;
	g->rot = gfx->rot;
TODO("implement a mirror bit")

	undo_gfx_geo_swap(g);
	if (undoable) pcb_undo_inc_serial();
}

void pcb_gfx_flip_side(pcb_layer_t *layer, pcb_gfx_t *gfx)
{
	pcb_r_delete_entry(layer->gfx_tree, (pcb_box_t *)gfx);
	gfx->cx = PCB_SWAP_X(gfx->cx);
	gfx->cy = PCB_SWAP_Y(gfx->cy);
	gfx->rot = PCB_SWAP_ANGLE(gfx->rot);
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);
	pcb_r_insert_entry(layer->gfx_tree, (pcb_box_t *)gfx);
}

void pcb_gfx_chg_geo(pcb_gfx_t *gfx, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy,  pcb_angle_t rot, pcb_bool undoable)
{
	undo_gfx_geo_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(gfx->parent.layer->parent.data), &undo_gfx_geo, sizeof(undo_gfx_geo_t));

	g->gfx = gfx;
	g->cx = cx;
	g->cy = cy;
	g->sx = sx;
	g->sy = sy;
	g->rot = rot;

	undo_gfx_geo_swap(g);
	if (undoable) pcb_undo_inc_serial();
}

void pcb_gfx_scale(pcb_gfx_t *gfx, double sx, double sy, double sth)
{
	int onbrd = (gfx->parent.layer != NULL) && (!gfx->parent.layer->is_bound);

	if (onbrd)
		pcb_gfx_pre(gfx);

	if (sx != 1.0) gfx->sx = pcb_round((double)gfx->sx * sx);
	if (sy != 1.0) gfx->sy = pcb_round((double)gfx->sy * sy);

	pcb_gfx_bbox(gfx);
	if (onbrd)
		pcb_gfx_post(gfx);
	else
		pcb_gfx_update(gfx);
}

/* rotates a gfx */
void *pcb_gfxop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_invalidate_erase(gfx);
	if (Layer->gfx_tree != NULL)
		pcb_r_delete_entry(Layer->gfx_tree, (pcb_box_t *) gfx);
	pcb_gfx_rotate90(gfx, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->gfx_tree != NULL)
		pcb_r_insert_entry(Layer->gfx_tree, (pcb_box_t *) gfx);
	pcb_gfx_invalidate_draw(Layer, gfx);
	return gfx;
}

void *pcb_gfxop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_invalidate_erase(gfx);
	pcb_gfx_rotate(Layer, gfx, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina, ctx->rotate.angle);
	pcb_gfx_invalidate_draw(Layer, gfx);
	return gfx;
}

void *pcb_gfxop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	static pcb_flag_values_t pcb_gfx_flags = 0;
	if (pcb_gfx_flags == 0)
		pcb_gfx_flags = pcb_obj_valid_flags(PCB_OBJ_GFX);

	if ((ctx->chgflag.flag & pcb_gfx_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (gfx->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(gfx);

	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, gfx);

	pcb_gfx_update(gfx);
	return gfx;
}

void *pcb_gfxop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_gfx_t *gfx)
{
	pcb_gfx_name_invalidate_draw(gfx);
	return gfx;
}


/*** draw ***/

void pcb_gfx_name_invalidate_draw(pcb_gfx_t *gfx)
{
	/* ->term: can't be a terminal, no label to draw */
}

void pcb_gfx_draw_label(pcb_draw_info_t *info, pcb_gfx_t *gfx)
{
	/* ->term: can't be a terminal, no label to draw */
	if (gfx->noexport)
		pcb_obj_noexport_mark(gfx, gfx->cx, gfx->cy);
}

void pcb_gfx_draw_(pcb_draw_info_t *info, pcb_gfx_t *gfx, int allow_term_gfx)
{
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(gfx->parent.layer);

	PCB_DRAW_BBOX(gfx);

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, gfx)) {
		const pcb_color_t *color;
		if (layer->is_bound)
			PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
		else
			color = &conf_core.appearance.color.selected;

		pcb_render->set_color(pcb_draw_out.fgGC, color);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
		pcb_hid_set_line_width(pcb_draw_out.fgGC, -2);
		pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[0], gfx->coy[0], gfx->cox[1], gfx->coy[1]);
		pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[1], gfx->coy[1], gfx->cox[2], gfx->coy[2]);
		pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[2], gfx->coy[2], gfx->cox[3], gfx->coy[3]);
		pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[3], gfx->coy[3], gfx->cox[0], gfx->coy[0]);
	}


/* temporary */
	pcb_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.warn);
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, PCB_MM_TO_COORD(0.1));
	pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[0], gfx->coy[0], gfx->cox[2], gfx->coy[2]);
	pcb_render->draw_line(pcb_draw_out.fgGC, gfx->cox[1], gfx->coy[1], gfx->cox[3], gfx->coy[3]);

	TODO("gfx draw");
}

void pcb_gfx_draw(pcb_draw_info_t *info, pcb_gfx_t *gfx, int allow_term_gfx)
{
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(gfx->parent.layer);

	pcb_obj_noexport(info, gfx, return);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = gfx->parent.layer;

	pcb_gfx_draw_(info, gfx, allow_term_gfx);
}

pcb_r_dir_t pcb_gfx_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_gfx_t *gfx = (pcb_gfx_t *)b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b, info) || pcb_partial_export((pcb_any_obj_t*)b, info))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(gfx->parent_type, &gfx->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_gfx_draw(info, gfx, 0);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* erases a gfx on a layer */
void pcb_gfx_invalidate_erase(pcb_gfx_t *gfx)
{
	pcb_draw_invalidate(gfx);
	pcb_flag_erase(&gfx->Flags);
}

void pcb_gfx_invalidate_draw(pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_draw_invalidate(gfx);
}

void pcb_gfx_draw_xor(pcb_gfx_t *gfx, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_render->draw_line(pcb_crosshair.GC, gfx->cox[0]+dx, gfx->coy[0]+dy, gfx->cox[1]+dx, gfx->coy[1]+dy);
	pcb_render->draw_line(pcb_crosshair.GC, gfx->cox[1]+dx, gfx->coy[1]+dy, gfx->cox[2]+dx, gfx->coy[2]+dy);
	pcb_render->draw_line(pcb_crosshair.GC, gfx->cox[2]+dx, gfx->coy[2]+dy, gfx->cox[3]+dx, gfx->coy[3]+dy);
	pcb_render->draw_line(pcb_crosshair.GC, gfx->cox[3]+dx, gfx->coy[3]+dy, gfx->cox[0]+dx, gfx->coy[0]+dy);
}
