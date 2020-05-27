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
#include <librnd/core/actions.h>


#include "board.h"
#include "data.h"
#include "undo.h"
#include "rotate.h"
#include "move.h"
#include "conf_core.h"
#include "draw_wireframe.h"
#include "pixmap_pcb.h"

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
static rnd_rnd_box_t pcb_gfx_bbox_(const pcb_gfx_t *gfx)
{
	rnd_rnd_box_t res = {+RND_COORD_MAX, +RND_COORD_MAX, -RND_COORD_MAX, -RND_COORD_MAX};
	int n;
	for(n = 0; n < 4; n++)
		rnd_box_bump_point(&res, gfx->corner[n].X, gfx->corner[n].Y);
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

	gfx->corner[0].X = rnd_round((double)gfx->cx + rx); gfx->corner[0].Y = rnd_round((double)gfx->cy + ry);
	gfx->corner[1].X = rnd_round((double)gfx->cx - rx); gfx->corner[1].Y = rnd_round((double)gfx->cy + ry);
	gfx->corner[2].X = rnd_round((double)gfx->cx - rx); gfx->corner[2].Y = rnd_round((double)gfx->cy - ry);
	gfx->corner[3].X = rnd_round((double)gfx->cx + rx); gfx->corner[3].Y = rnd_round((double)gfx->cy - ry);
	if (gfx->rot != 0.0) {
		a = gfx->rot / RND_RAD_TO_DEG;
		cosa = cos(a);
		sina = sin(a);
		for(n = 0; n < 4; n++)
			rnd_rotate(&gfx->corner[n].X, &gfx->corner[n].Y, gfx->cx, gfx->cy, cosa, sina);
	}
	pcb_gfx_bbox(gfx);
}

/* creates a new gfx on a layer */
pcb_gfx_t *pcb_gfx_new(pcb_layer_t *layer, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_angle_t rot, pcb_flag_t Flags)
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
	rnd_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}

static void pcb_gfx_copy_data(pcb_gfx_t *dst, pcb_gfx_t *src)
{
	dst->rot = src->rot;
	dst->xmirror = src->xmirror;
	dst->ymirror = src->ymirror;
	dst->pxm_id = src->pxm_id;
	dst->pxm_neutral = src->pxm_neutral;
	src->pxm_neutral->refco++;
	dst->pxm_xformed = src->pxm_xformed;
	src->pxm_xformed->refco++;
	pcb_gfx_update(dst);
}

pcb_gfx_t *pcb_gfx_dup_at(pcb_layer_t *dst, pcb_gfx_t *src, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_gfx_t *a = pcb_gfx_new(dst, src->cx+dx, src->cy+dy, src->sx, src->sy, src->rot, src->Flags);
	pcb_gfx_copy_meta(a, src);
	pcb_gfx_copy_data(a, src);
	return a;
}

pcb_gfx_t *pcb_gfx_dup(pcb_layer_t *dst, pcb_gfx_t *src)
{
	return pcb_gfx_dup_at(dst, src, 0, 0);
}


void pcb_add_gfx_on_layer(pcb_layer_t *Layer, pcb_gfx_t *gfx)
{
	pcb_gfx_bbox(gfx);
	if (!Layer->gfx_tree)
		Layer->gfx_tree = rnd_r_create_tree();
	rnd_r_insert_entry(Layer->gfx_tree, (rnd_rnd_box_t *)gfx);
	gfx->type = PCB_OBJ_GFX;
	PCB_SET_PARENT(gfx, layer, Layer);
}



void pcb_gfx_free(pcb_gfx_t *gfx)
{
	if ((gfx->parent.layer != NULL) && (gfx->parent.layer->gfx_tree != NULL))
		rnd_r_delete_entry(gfx->parent.layer->gfx_tree, (rnd_rnd_box_t *)gfx);
	rnd_attribute_free(&gfx->Attributes);
	pcb_gfx_unreg(gfx);
	pcb_obj_common_free((pcb_any_obj_t *)gfx);
	free(gfx);
}


int pcb_gfx_eq(const pcb_host_trans_t *tr1, const pcb_gfx_t *a1, const pcb_host_trans_t *tr2, const pcb_gfx_t *a2)
{
	double sgn1 = tr1->on_bottom ? -1 : +1;
	double sgn2 = tr2->on_bottom ? -1 : +1;

	if (a1->sx != a2->sx) return 0;
	if (a1->sy != a2->sy) return 0;
	if (pcb_neq_tr_coords(tr1, a1->cx, a1->cy, tr2, a2->cx, a2->cy)) return 0;
	if (rnd_normalize_angle(rnd_round(a1->rot * sgn1)) != rnd_normalize_angle(rnd_round(a2->rot * sgn2))) return 0;
TODO("compare pixmaps");
	return 1;
}

unsigned int pcb_gfx_hash(const pcb_host_trans_t *tr, const pcb_gfx_t *a)
{
	unsigned int content = 0, crd = 0;
	double sgn = tr->on_bottom ? -1 : +1;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, a)) {
		rnd_coord_t x, y;
		pcb_hash_tr_coords(tr, &x, &y, a->cx, a->cy);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y) ^ pcb_hash_coord(a->sx) ^ pcb_hash_coord(a->sy) ^
			pcb_hash_coord(rnd_normalize_angle(rnd_round(a->rot*sgn + tr->rot*sgn)));
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
		rnd_r_delete_entry(ly->gfx_tree, (rnd_rnd_box_t *)gfx);
}

void pcb_gfx_post(pcb_gfx_t *gfx)
{
	pcb_layer_t *ly = pcb_layer_get_real(gfx->parent.layer);
	pcb_gfx_update(gfx);
	if (ly == NULL)
		return;
	if (ly->gfx_tree != NULL)
		rnd_r_insert_entry(ly->gfx_tree, (rnd_rnd_box_t *)gfx);
}

/***** operations *****/

static const char core_gfx_cookie[] = "core-gfx";

typedef struct {
	pcb_gfx_t *gfx; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	rnd_coord_t cx, cy, sx, sy;
	rnd_angle_t rot;
} undo_gfx_geo_t;

static int undo_gfx_geo_swap(void *udata)
{
	undo_gfx_geo_t *g = udata;
	pcb_layer_t *layer = g->gfx->parent.layer;

	if (layer->gfx_tree != NULL)
		rnd_r_delete_entry(layer->gfx_tree, (rnd_rnd_box_t *)g->gfx);

	rnd_swap(rnd_coord_t, g->cx, g->gfx->cx);
	rnd_swap(rnd_coord_t, g->cy, g->gfx->cy);
	rnd_swap(rnd_coord_t, g->sx, g->gfx->sx);
	rnd_swap(rnd_coord_t, g->sy, g->gfx->sy);
	rnd_swap(rnd_angle_t, g->rot, g->gfx->rot);

	if (g->rot != g->gfx->rot)
		g->gfx->pxm_xformed = pcb_pixmap_alloc_insert_transformed(&pcb_pixmaps, g->gfx->pxm_neutral, g->gfx->rot, g->gfx->xmirror, g->gfx->ymirror);

	pcb_gfx_update(g->gfx);
	pcb_gfx_bbox(g->gfx);
	if (layer->gfx_tree != NULL)
		rnd_r_insert_entry(layer->gfx_tree, (rnd_rnd_box_t *)g->gfx);

	return 0;
}

static void undo_gfx_geo_print(void *udata, char *dst, size_t dst_len)
{
	rnd_snprintf(dst, dst_len, "gfx geo");
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
	rnd_layer_id_t lid = pcb_layer_id(ctx->buffer.src, Layer);
	pcb_layer_t *layer;

	/* the buffer may not have the specified layer, e.g. on loose subc subc-aux layer */
	if (lid == -1)
		return NULL;

	layer = &ctx->buffer.dst->Layer[lid];
	a = pcb_gfx_new(layer, gfx->cx, gfx->cy, gfx->sx, gfx->sy, gfx->rot,
		pcb_flag_mask(gfx->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));

	pcb_gfx_copy_meta(a, gfx);
	pcb_gfx_copy_data(a, gfx);
	if (ctx->buffer.keep_id) pcb_obj_change_id((pcb_any_obj_t *)a, gfx->ID);
	return a;
}

/* moves a gfx between board and buffer */
void *pcb_gfxop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_gfx_t *gfx)
{
	pcb_layer_t *srcly = gfx->parent.layer;

	assert(gfx->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) { /* auto layer in dst */
		rnd_layer_id_t lid = pcb_layer_id(ctx->buffer.src, srcly);
		if (lid < 0) {
			rnd_message(RND_MSG_ERROR, "Internal error: can't resolve source layer ID in pcb_gfxop_move_buffer\n");
			return NULL;
		}
		dstly = &ctx->buffer.dst->Layer[lid];
	}

	rnd_r_delete_entry(srcly->gfx_tree, (rnd_rnd_box_t *) gfx);

	pcb_gfx_unreg(gfx);
	pcb_gfx_reg(dstly, gfx);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, gfx);

	if (!dstly->gfx_tree)
		dstly->gfx_tree = rnd_r_create_tree();
	rnd_r_insert_entry(dstly->gfx_tree, (rnd_rnd_box_t *) gfx);

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
	pcb_gfx_copy_data(ngfx, gfx);
	pcb_gfx_invalidate_draw(Layer, ngfx);
	pcb_undo_add_obj_to_create(PCB_OBJ_GFX, Layer, ngfx, ngfx);
	return ngfx;
}


#define pcb_gfx_move(g,dx,dy)                                     \
	do {                                                            \
		rnd_coord_t __dx__ = (dx), __dy__ = (dy);                     \
		pcb_gfx_t *__g__ = (g);                                       \
		RND_MOVE_POINT((__g__)->cx, (__g__)->cy,__dx__,__dy__);          \
		RND_BOX_MOVE_LOWLEVEL(&((__g__)->BoundingBox),__dx__,__dy__); \
		RND_BOX_MOVE_LOWLEVEL(&((__g__)->bbox_naked),__dx__,__dy__); \
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
	rnd_r_delete_entry(Layer->gfx_tree, (rnd_rnd_box_t *)gfx);
	pcb_gfxop_move_noclip(ctx, Layer, gfx);
	rnd_r_insert_entry(Layer->gfx_tree, (rnd_rnd_box_t *)gfx);
	return gfx;
}

/* moves a gfx between layers; lowlevel routines */
void *pcb_gfxop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_gfx_t * gfx, pcb_layer_t * Destination)
{
	rnd_r_delete_entry(Source->gfx_tree, (rnd_rnd_box_t *)gfx);

	pcb_gfx_unreg(gfx);
	pcb_gfx_reg(Destination, gfx);

	if (!Destination->gfx_tree)
		Destination->gfx_tree = rnd_r_create_tree();
	rnd_r_insert_entry(Destination->gfx_tree, (rnd_rnd_box_t *)gfx);

	return gfx;
}


/* moves a gfx between layers */
void *pcb_gfxop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_gfx_t * gfx)
{
	pcb_gfx_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, gfx)) {
		rnd_message(RND_MSG_WARNING, "Sorry, gfx object is locked\n");
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
	rnd_r_delete_entry(Layer->gfx_tree, (rnd_rnd_box_t *) gfx);

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
void pcb_gfx_rotate90(pcb_gfx_t *gfx, rnd_coord_t X, rnd_coord_t Y, unsigned Number)
{
	gfx->rot = rnd_normalize_angle(gfx->rot + Number * 90);
	TODO("rotate content")
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);
}

void pcb_gfx_rotate(pcb_layer_t *layer, pcb_gfx_t *gfx, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina, rnd_angle_t angle)
{
	if (layer->gfx_tree != NULL)
		rnd_r_delete_entry(layer->gfx_tree, (rnd_rnd_box_t *) gfx);

	gfx->rot = rnd_normalize_angle(gfx->rot + angle);
	TODO("rotate content")
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);

	if (layer->gfx_tree != NULL)
		rnd_r_insert_entry(layer->gfx_tree, (rnd_rnd_box_t *) gfx);
}

void pcb_gfx_mirror(pcb_gfx_t *gfx, rnd_coord_t y_offs, rnd_bool undoable)
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
	rnd_r_delete_entry(layer->gfx_tree, (rnd_rnd_box_t *)gfx);
	gfx->cx = PCB_SWAP_X(gfx->cx);
	gfx->cy = PCB_SWAP_Y(gfx->cy);
	gfx->rot = RND_SWAP_ANGLE(gfx->rot);
	pcb_gfx_update(gfx);
	pcb_gfx_bbox(gfx);
	rnd_r_insert_entry(layer->gfx_tree, (rnd_rnd_box_t *)gfx);
}

void pcb_gfx_chg_geo(pcb_gfx_t *gfx, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy,  rnd_angle_t rot, rnd_bool undoable)
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

	if (sx != 1.0) gfx->sx = rnd_round((double)gfx->sx * sx);
	if (sy != 1.0) gfx->sy = rnd_round((double)gfx->sy * sy);

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
		rnd_r_delete_entry(Layer->gfx_tree, (rnd_rnd_box_t *) gfx);
	pcb_gfx_rotate90(gfx, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->gfx_tree != NULL)
		rnd_r_insert_entry(Layer->gfx_tree, (rnd_rnd_box_t *) gfx);
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

void pcb_gfx_set_pixmap_free(pcb_gfx_t *gfx, rnd_pixmap_t *pxm, rnd_bool undoable)
{
	TODO("gfx: undoable pixmap assign");
	gfx->pxm_neutral = pcb_pixmap_insert_neutral_or_free(&pcb_pixmaps, pxm);
	gfx->pxm_xformed = pcb_pixmap_alloc_insert_transformed(&pcb_pixmaps, gfx->pxm_neutral, gfx->rot, gfx->xmirror, gfx->ymirror);
	gfx->pxm_id = 0; /* no preference on the ID now */
}

void pcb_gfx_set_pixmap_dup(pcb_gfx_t *gfx, const rnd_pixmap_t *pxm, rnd_bool undoable)
{
	TODO("gfx: undoable pixmap assign");
	gfx->pxm_neutral = pcb_pixmap_insert_neutral_dup(&PCB->hidlib, &pcb_pixmaps, pxm);
	gfx->pxm_xformed = pcb_pixmap_alloc_insert_transformed(&pcb_pixmaps, gfx->pxm_neutral, gfx->rot, gfx->xmirror, gfx->ymirror);
	gfx->pxm_id = 0; /* no preference on the ID now */
}

void pcb_gfx_resize_move_corner(pcb_gfx_t *gfx, int corn_idx, rnd_coord_t dx, rnd_coord_t dy, int undoable)
{
	rnd_point_t *corn;
	double sgnx, sgny;
	undo_gfx_geo_t gtmp, *g = &gtmp;
	rnd_coord_t nsx, nsy;

	if ((corn_idx < 0) || (corn_idx > 3))
		return;
	corn = &gfx->corner[corn_idx];

	sgnx = corn->X > gfx->cx ? 1 : -1;
	sgny = corn->Y > gfx->cy ? 1 : -1;

	nsx = gfx->sx + dx * sgnx;
	nsy = gfx->sy + dy * sgny;

	if ((nsx < 128) || (nsy < 128)) {
		rnd_message(RND_MSG_ERROR, "Invalid gfx size\n");
		return;
	}

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(gfx->parent.layer->parent.data), &undo_gfx_geo, sizeof(undo_gfx_geo_t));

	g->gfx = gfx;
	g->cx = gfx->cx + rnd_round((double)dx / 2.0);;
	g->cy = gfx->cy + rnd_round((double)dy / 2.0);;
	g->sx = nsx;
	g->sy = nsy;
	g->rot = gfx->rot;

	undo_gfx_geo_swap(g);
	if (undoable) pcb_undo_inc_serial();

/*	rnd_trace("pcb_gfx_resize_move_corner pt %d %mm;%mm %.0f;%.0f!\n", corn_idx, dx, dy, sgnx, sgny); */
}

int gfx_screen2pixmap_coord(pcb_gfx_t *gfx, rnd_coord_t x, rnd_coord_t y, long *px, long *py)
{
	rnd_coord_t dx = x - gfx->cx, dy = y - gfx->cy;

	if (gfx->pxm_neutral == NULL)
		return -1;

	rnd_rotate(&dx, &dy, 0, 0, cos(-gfx->rot / RND_RAD_TO_DEG), sin(-gfx->rot / RND_RAD_TO_DEG));
	*px = floor((double)dx / (double)gfx->sx * (double)gfx->pxm_neutral->sx + (double)gfx->pxm_neutral->sx/2.0);
	*py = floor((double)dy / (double)gfx->sy * (double)gfx->pxm_neutral->sy + (double)gfx->pxm_neutral->sy/2.0);

	if ((*px < 0) || (*px >= gfx->pxm_neutral->sx) || (*py < 0) || (*py >= gfx->pxm_neutral->sy))
		return -1;

	return 0;
}

int gfx_set_resize_by_pixel_dist(pcb_gfx_t *gfx, long pdx, long pdy, rnd_coord_t len, rnd_bool allow_x, rnd_bool allow_y, rnd_bool undoable)
{
	double lenx, leny;
	rnd_coord_t nsx = 0, nsy = 0;

	if (pdx < 10) allow_x = 0;
	if (pdy < 10) allow_y = 0;

	if ((allow_x == 0) && (allow_y == 0))
		return -1;

	if (gfx->pxm_neutral == NULL)
		return -1;

/*rnd_trace("resize1: %mm %mm (%d %d)\n", gfx->sx, gfx->sy, pdx, pdy);*/
	if (allow_x) {
		double lenx = cos(-gfx->rot / RND_RAD_TO_DEG) * (double)len;
		double sc = lenx / (double)pdx;
		nsx = gfx->pxm_neutral->sx * sc;
	}

	if (allow_y) {
		double leny = sin((90-gfx->rot) / RND_RAD_TO_DEG) * (double)len;
		double sc = leny / (double)pdy;
		nsy = gfx->pxm_neutral->sy * sc;
	}

	if ((nsx > 0) || (nsy > 0)) {
		if (nsx <= 0) nsx = gfx->sx;
		if (nsy <= 0) nsy = gfx->sy;
		pcb_gfx_chg_geo(gfx, gfx->cx, gfx->cy, nsx, nsy,  gfx->rot, undoable);
	}

/*rnd_trace("resize2: %mm %mm\n", gfx->sx, gfx->sy);*/
	return 0;
}

void gfx_set_resize_gui(rnd_hidlib_t *hl, pcb_gfx_t *gfx, rnd_bool allow_x, rnd_bool allow_y)
{
	char *lens;
	rnd_coord_t x1, y1, x2, y2;
	long px1, py1, px2, py2;
	double len;
	int dirs;
	rnd_bool succ;
	static const char *msg[4] = {
		NULL,
		"horizontal distance of these two points",
		"vertical distance of these two points",
		"distance distance of these two points",
	};

	dirs = ((int)allow_y) << 1 | (int)allow_x;
	if (dirs == 0) {
		rnd_message(RND_MSG_ERROR, "Invalid direction restriction: either allow_x or allow_y should be 1\n");
		return;
	}

	if (gfx->pxm_neutral == NULL) {
		rnd_message(RND_MSG_ERROR, "Missing pixmap\n");
		return;
	}

	rnd_hid_get_coords("Click the first point", &x1, &y1, 1);
	rnd_hid_get_coords("Click the second point", &x2, &y2, 1);
	if ((gfx_screen2pixmap_coord(gfx, x1, y1, &px1, &py1) != 0) || (gfx_screen2pixmap_coord(gfx, x2, y2, &px2, &py2) != 0)) {
		rnd_message(RND_MSG_ERROR, "No pixmap available for this gfx objet or clicked outside of the pixmap\n");
		return;
	}

	lens = rnd_hid_prompt_for(hl, msg[dirs], "0", "distance");
	if (lens == NULL)
		return;

	len = rnd_get_value(lens, NULL, NULL, &succ);
	if (succ)
		gfx_set_resize_by_pixel_dist(gfx, px2-px1, py2-py1, len, allow_x, allow_y, 1);
	free(lens);
}


/*** undoable transparent color set ***/
typedef struct {
	pcb_gfx_t *gfx; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	unsigned char tr, tg, tb;
	unsigned char has_transp, transp_valid;
} undo_gfx_transp_t;

static int undo_gfx_transp_swap(void *udata)
{
	undo_gfx_transp_t *g = udata;
	pcb_gfx_t *gfx = g->gfx;

	if (gfx->pxm_neutral == NULL)
		return -1;

	rnd_swap(unsigned char,  gfx->pxm_xformed->tr, g->tr);
	rnd_swap(unsigned char,  gfx->pxm_xformed->tg, g->tg);
	rnd_swap(unsigned char,  gfx->pxm_xformed->tb, g->tb);
	rnd_swap(unsigned char,  gfx->pxm_xformed->has_transp, g->has_transp);
	rnd_swap(unsigned char,  gfx->pxm_xformed->transp_valid, g->transp_valid);

	gfx->pxm_neutral->tr = gfx->pxm_xformed->tr;
	gfx->pxm_neutral->tg = gfx->pxm_xformed->tg;
	gfx->pxm_neutral->tb = gfx->pxm_xformed->tb;
	gfx->pxm_neutral->has_transp = gfx->pxm_xformed->has_transp;
	gfx->pxm_neutral->transp_valid = gfx->pxm_xformed->transp_valid;
	rnd_pixmap_free_hid_data(gfx->pxm_xformed);

	pcb_gfx_invalidate_draw(gfx->parent.layer, gfx);
	pcb_draw();

	return 0;
}

static void undo_gfx_transp_print(void *udata, char *dst, size_t dst_len)
{
	undo_gfx_transp_t *g = udata;
	rnd_snprintf(dst, dst_len, "gfx transp: %d %d #%02x%02x%02x", g->has_transp, g->transp_valid, g->tr, g->tg, g->tb);
}

static const uundo_oper_t undo_gfx_transp = {
	core_gfx_cookie,
	NULL,
	undo_gfx_transp_swap,
	undo_gfx_transp_swap,
	undo_gfx_transp_print
};

int gfx_set_transparent(pcb_gfx_t *gfx, unsigned char tr, unsigned char tg, unsigned char tb, int undoable)
{
	undo_gfx_transp_t gtmp, *g = &gtmp;

	if (gfx->pxm_neutral == NULL)
		return -1;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(gfx->parent.layer->parent.data), &undo_gfx_transp, sizeof(undo_gfx_transp_t));

	g->gfx = gfx;
	g->tr = tr;
	g->tg = tg;
	g->tb = tb;
	g->has_transp = g->transp_valid = 1;

	undo_gfx_transp_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return 0;
}

int gfx_set_transparent_by_coord(pcb_gfx_t *gfx, long px, long py)
{
	unsigned char *t;

	if (gfx->pxm_neutral == NULL)
		return -1;

	if ((px < 0) || (py < 0) || (px >= gfx->pxm_neutral->sx) || (py >= gfx->pxm_neutral->sy))
		return -1;

	t = gfx->pxm_neutral->p + (px + py * gfx->pxm_neutral->sx) * 3;
	return gfx_set_transparent(gfx, t[0], t[1], t[2], 1);
}

void gfx_set_transparent_gui(pcb_gfx_t *gfx)
{
	rnd_coord_t x, y;
	long px, py;
	unsigned char *t;

	rnd_hid_get_coords("Click on the pixel that should be transparent", &x, &y, 1);

	if (gfx_screen2pixmap_coord(gfx, x, y, &px, &py) != 0) {
		rnd_message(RND_MSG_ERROR, "No pixmap available for this gfx objet or clicked outside of the pixmap\n");
		return;
	}

	t = gfx->pxm_neutral->p + (px + py * gfx->pxm_neutral->sx) * 3;

	gfx_set_transparent(gfx, t[0], t[1], t[2], 1);
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
		const rnd_color_t *color;
		if (layer->is_bound)
			PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
		else
			color = &conf_core.appearance.color.selected;

		rnd_render->set_color(pcb_draw_out.fgGC, color);
		rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round);
		rnd_hid_set_line_width(pcb_draw_out.fgGC, -2);
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[0].X, gfx->corner[0].Y, gfx->corner[1].X, gfx->corner[1].Y);
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[1].X, gfx->corner[1].Y, gfx->corner[2].X, gfx->corner[2].Y);
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[2].X, gfx->corner[2].Y, gfx->corner[3].X, gfx->corner[3].Y);
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[3].X, gfx->corner[3].Y, gfx->corner[0].X, gfx->corner[0].Y);
	}


	if ((gfx->pxm_neutral == NULL) || (rnd_render->draw_pixmap == NULL)) {
		rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.warn);
		rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round);
		rnd_hid_set_line_width(pcb_draw_out.fgGC, RND_MM_TO_COORD(0.1));
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[0].X, gfx->corner[0].Y, gfx->corner[2].X, gfx->corner[2].Y);
		rnd_render->draw_line(pcb_draw_out.fgGC, gfx->corner[1].X, gfx->corner[1].Y, gfx->corner[3].X, gfx->corner[3].Y);
	}
	else {
		rnd_render->draw_pixmap(rnd_render, gfx->cx, gfx->cy, gfx->sx, gfx->sy, gfx->pxm_xformed);
	}
}

void pcb_gfx_draw(pcb_draw_info_t *info, pcb_gfx_t *gfx, int allow_term_gfx)
{
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(gfx->parent.layer);

	pcb_obj_noexport(info, gfx, return);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = gfx->parent.layer;

	pcb_gfx_draw_(info, gfx, allow_term_gfx);
}

RND_INLINE rnd_r_dir_t pcb_gfx_draw_callback_(pcb_gfx_t *gfx, void *cl)
{
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)gfx, info) || pcb_partial_export((pcb_any_obj_t*)gfx, info))
		return RND_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(gfx->parent_type, &gfx->parent))
		return RND_R_DIR_NOT_FOUND;

	pcb_gfx_draw(info, gfx, 0);
	return RND_R_DIR_FOUND_CONTINUE;
}

rnd_r_dir_t pcb_gfx_draw_under_callback(const rnd_rnd_box_t *b, void *cl)
{
	pcb_gfx_t *gfx = (pcb_gfx_t *)b;
	if (!gfx->render_under)
		return RND_R_DIR_FOUND_CONTINUE;
	return pcb_gfx_draw_callback_(gfx, cl);
}

rnd_r_dir_t pcb_gfx_draw_above_callback(const rnd_rnd_box_t *b, void *cl)
{
	pcb_gfx_t *gfx = (pcb_gfx_t *)b;
	if (gfx->render_under)
		return RND_R_DIR_FOUND_CONTINUE;
	return pcb_gfx_draw_callback_(gfx, cl);
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

void pcb_gfx_draw_xor(pcb_gfx_t *gfx, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_render->draw_line(pcb_crosshair.GC, gfx->corner[0].X+dx, gfx->corner[0].Y+dy, gfx->corner[1].X+dx, gfx->corner[1].Y+dy);
	rnd_render->draw_line(pcb_crosshair.GC, gfx->corner[1].X+dx, gfx->corner[1].Y+dy, gfx->corner[2].X+dx, gfx->corner[2].Y+dy);
	rnd_render->draw_line(pcb_crosshair.GC, gfx->corner[2].X+dx, gfx->corner[2].Y+dy, gfx->corner[3].X+dx, gfx->corner[3].Y+dy);
	rnd_render->draw_line(pcb_crosshair.GC, gfx->corner[3].X+dx, gfx->corner[3].Y+dy, gfx->corner[0].X+dx, gfx->corner[0].Y+dy);
}
