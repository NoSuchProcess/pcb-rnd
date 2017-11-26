/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#include "polygon.h"
#include "rotate.h"

void *pcb_pstkop_add_to_buffer(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_t *p;
	pcb_cardinal_t npid;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	if (proto == NULL)
		return NULL;

	npid = pcb_pstk_proto_insert_dup(ctx->buffer.dst, proto, 1);
	p = pcb_pstk_new(ctx->buffer.dst, npid, ps->x, ps->y, ps->Clearance, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	return pcb_pstk_copy_meta(p, ps);
}

void *pcb_pstkop_move_to_buffer(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_cardinal_t npid;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	if (proto == NULL)
		return NULL;

	npid = pcb_pstk_proto_insert_dup(ctx->buffer.dst, proto, 1);

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_PSTK, NULL, ps);
	pcb_r_delete_entry(ctx->buffer.src->padstack_tree, (pcb_box_t *)ps);

	padstacklist_remove(ps);
	ps->proto = npid;
	ps->protoi = -1; /* only the canonical trshape got copied, not the transofrmed ones */
	padstacklist_append(&ctx->buffer.dst->padstack, ps);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, ps);

	if (!ctx->buffer.dst->padstack_tree)
		ctx->buffer.dst->padstack_tree = pcb_r_create_tree(NULL, 0, 0);

	pcb_r_insert_entry(ctx->buffer.dst->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_PSTK, NULL, ps);

	PCB_SET_PARENT(ps, data, ctx->buffer.dst);
	return ps;
}

void *pcb_pstkop_copy(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_t *nps;
	pcb_data_t *data = ctx->copy.pcb->Data;
	pcb_cardinal_t npid;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	if (proto == NULL)
		return NULL;
	npid = pcb_pstk_proto_insert_dup(data, proto, 1);

	nps = pcb_pstk_new(data, npid, ps->x + ctx->copy.DeltaX, ps->y + ctx->copy.DeltaY, ps->Clearance, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND));
	if (nps == NULL)
		return NULL;

	pcb_pstk_copy_meta(nps, ps);
	pcb_pstk_invalidate_draw(nps);
	pcb_undo_add_obj_to_create(PCB_TYPE_PSTK, data, nps, nps);
	return nps;
}

void *pcb_pstkop_move_noclip(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_invalidate_erase(ps);
	pcb_pstk_move(ps, ctx->move.dx, ctx->move.dy);
	pcb_pstk_invalidate_draw(ps);
	pcb_draw();
	return ps;
}

void *pcb_pstkop_move(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);

	pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)ps);
	pcb_poly_restore_to_poly(data, PCB_TYPE_PSTK, NULL, ps);
	pcb_pstkop_move_noclip(ctx, ps);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(data, PCB_TYPE_PSTK, NULL, ps);
	return ps;
}

void *pcb_pstkop_clip(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);

	if (ctx->clip.restore) {
		if (data->padstack_tree != NULL)
			pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)ps);
		pcb_poly_restore_to_poly(data, PCB_TYPE_PSTK, NULL, ps);
	}
	if (ctx->clip.clear) {
		if (data->padstack_tree != NULL)
			pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
		pcb_poly_clear_from_poly(data, PCB_TYPE_PSTK, NULL, ps);
	}

	return ps;
}

void *pcb_pstkop_remove(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_invalidate_erase(ps);
	pcb_undo_move_obj_to_remove(PCB_TYPE_PSTK, ps, ps, ps);
	return NULL;
}

void *pcb_pstkop_destroy(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_r_delete_entry(ctx->remove.destroy_target->padstack_tree, (pcb_box_t *)ps);
	pcb_pstk_free(ps);
	return NULL;
}

void *pcb_pstkop_change_thermal(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_board_t *pcb = ctx->chgtherm.pcb;
	pcb_layer_t *layer = pcb_get_layer(pcb->Data, ctx->chgtherm.lid);

	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PSTK, ps, ps, ps, pcb_false);
	pcb_poly_restore_to_poly(pcb->Data, PCB_TYPE_PSTK, layer, ps);

#warning TODO: undo
	pcb_pstk_set_thermal(ps, ctx->chgtherm.lid, ctx->chgtherm.style);

	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PSTK, ps, ps, ps, pcb_true);
	pcb_poly_clear_from_poly(pcb->Data, PCB_TYPE_PSTK, layer, ps);
	pcb_pstk_invalidate_draw(ps);
	return ps;
}

void *pcb_pstkop_change_flag(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	static pcb_flag_values_t pcb_pstk_flags = 0;
	if (pcb_pstk_flags == 0)
		pcb_pstk_flags = pcb_obj_valid_flags(PCB_TYPE_PSTK);

	if ((ctx->chgflag.flag & pcb_pstk_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (ps->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(ps);
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, ps);
	return ps;
}


void *pcb_pstkop_rotate(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	double rot = ps->rot;

	rot += (double)ctx->rotate.angle;

	if ((rot > 360.0) || (rot < -360.0))
		rot = fmod(rot, 360.0);

	if ((rot == 360.0) || (rot == -360.0))
		rot = 0;

	if (pcb_pstk_change_instance(ps, NULL, NULL, &rot, NULL) == 0) {
		pcb_coord_t nx = ps->x, ny = ps->y;


		pcb_poly_restore_to_poly(ps->parent.data, PCB_TYPE_PSTK, NULL, ps);
		pcb_pstk_invalidate_erase(ps);
		pcb_r_delete_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps);

		pcb_rotate(&nx, &ny, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);
		if ((nx != ps->x) || (ny != ps->y))
			pcb_pstk_move(ps, nx - ps->x, ny - ps->y);

		pcb_pstk_bbox(ps);
		pcb_r_insert_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps, 0);
		pcb_poly_clear_from_poly(ps->parent.data, PCB_TYPE_PSTK, NULL, ps);
		pcb_pstk_invalidate_draw(ps);

		return ps;
	}
	return NULL;
}

void *pcb_pstkop_rotate90(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	ctx->rotate.angle = (double)ctx->rotate.number * 90.0;

	switch(ctx->rotate.number) {
		case 0:
			ctx->rotate.sina = 0;
			ctx->rotate.cosa = 1;
			break;
		case 1:
			ctx->rotate.sina = 1;
			ctx->rotate.cosa = 0;
			break;
		case 2:
			ctx->rotate.sina = 0;
			ctx->rotate.cosa = -1;
			break;
		case 3:
			ctx->rotate.sina = -1;
			ctx->rotate.cosa = 0;
			break;
	}
	return pcb_pstkop_rotate(ctx, ps);
}

void *pcb_pstkop_change_size(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t proto;
	pcb_cardinal_t nproto;

	/* create the new prototype and insert it */
	pcb_pstk_proto_copy(&proto, pcb_pstk_get_proto(ps));
	pcb_pstk_proto_grow(&proto, ctx->chgsize.is_absolute, ctx->chgsize.value);
	nproto = pcb_pstk_proto_insert_dup(ps->parent.data, &proto, 1);
	pcb_pstk_proto_free_fields(&proto);

	if (nproto == PCB_PADSTACK_INVALID)
		return NULL;

	if (pcb_pstk_change_instance(ps, &nproto, NULL, NULL, NULL) == 0)
		return ps;

	return NULL;
}


void *pcb_pstkop_change_2nd_size(pcb_opctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t proto;
	pcb_cardinal_t nproto;

	/* create the new prototype and insert it */
	pcb_pstk_proto_copy(&proto, pcb_pstk_get_proto(ps));
	if (!ctx->chgsize.is_absolute) {
		proto.hdia += ctx->chgsize.value;
		if (proto.hdia < ctx->chgsize.pcb->minDrill)
			proto.hdia = ctx->chgsize.pcb->minDrill;
	}
	else
		proto.hdia = ctx->chgsize.value;
	nproto = pcb_pstk_proto_insert_dup(ps->parent.data, &proto, 1);
	pcb_pstk_proto_free_fields(&proto);

	if (nproto == PCB_PADSTACK_INVALID)
		return NULL;

	if (pcb_pstk_change_instance(ps, &nproto, NULL, NULL, NULL) == 0)
		return ps;

	return NULL;
}

