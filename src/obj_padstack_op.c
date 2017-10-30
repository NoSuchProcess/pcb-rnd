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

void *pcb_padstackop_add_to_buffer(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_t *p;
	pcb_cardinal_t npid;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	if (proto == NULL)
		return NULL;

	npid = pcb_padstack_proto_insert_dup(ctx->buffer.dst, proto, 1);
	p = pcb_padstack_new(ctx->buffer.dst, npid, ps->x, ps->y, ps->Clearance, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	return pcb_padstack_copy_meta(p, ps);
}

void *pcb_padstackop_move_to_buffer(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_cardinal_t npid;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	if (proto == NULL)
		return NULL;

	npid = pcb_padstack_proto_insert_dup(ctx->buffer.dst, proto, 1);

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_PADSTACK, NULL, ps);
	pcb_r_delete_entry(ctx->buffer.src->padstack_tree, (pcb_box_t *)ps);

	padstacklist_remove(ps);
	ps->proto = npid;
	padstacklist_append(&ctx->buffer.dst->padstack, ps);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, ps);

	if (!ctx->buffer.dst->padstack_tree)
		ctx->buffer.dst->padstack_tree = pcb_r_create_tree(NULL, 0, 0);

	pcb_r_insert_entry(ctx->buffer.dst->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_PADSTACK, NULL, ps);

	PCB_SET_PARENT(ps, data, ctx->buffer.dst);
	return ps;
}

void *pcb_padstackop_copy(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_t *nps;
	pcb_data_t *data = ctx->copy.pcb->Data;
	pcb_cardinal_t npid;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);

	if (proto == NULL)
		return NULL;
	npid = pcb_padstack_proto_insert_dup(data, proto, 1);

	nps = pcb_padstack_new(data, npid, ps->x + ctx->copy.DeltaX, ps->y + ctx->copy.DeltaY, ps->Clearance, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND));
	if (nps == NULL)
		return NULL;

	pcb_padstack_copy_meta(nps, ps);
	pcb_padstack_invalidate_draw(nps);
	pcb_undo_add_obj_to_create(PCB_TYPE_PADSTACK, data, nps, nps);
	return nps;
}

void *pcb_padstackop_move_noclip(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_invalidate_erase(ps);
	pcb_padstack_move(ps, ctx->move.dx, ctx->move.dy);
	pcb_padstack_invalidate_draw(ps);
	pcb_draw();
	return ps;
}

void *pcb_padstackop_move(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);

	pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)ps);
	pcb_poly_restore_to_poly(data, PCB_TYPE_PADSTACK, NULL, ps);
	pcb_padstackop_move_noclip(ctx, ps);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(data, PCB_TYPE_PADSTACK, NULL, ps);
	return ps;
}

void *pcb_padstackop_clip(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);

	if (ctx->clip.restore) {
		pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)ps);
		pcb_poly_restore_to_poly(data, PCB_TYPE_PADSTACK, NULL, ps);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
		pcb_poly_clear_from_poly(data, PCB_TYPE_PADSTACK, NULL, ps);
	}

	return ps;
}

void *pcb_padstackop_remove(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_invalidate_erase(ps);
	pcb_undo_move_obj_to_remove(PCB_TYPE_PADSTACK, ps, ps, ps);
	PCB_CLEAR_PARENT(ps);
	return NULL;
}

void *pcb_padstackop_destroy(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_r_delete_entry(ctx->remove.destroy_target->padstack_tree, (pcb_box_t *)ps);
	pcb_padstack_free(ps);
	return NULL;
}

void *pcb_padstackop_change_thermal(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_board_t *pcb = ctx->chgtherm.pcb;
	pcb_layer_t *layer = pcb_get_layer(pcb->Data, ctx->chgtherm.lid);

	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PADSTACK, ps, ps, ps, pcb_false);
	pcb_poly_restore_to_poly(pcb->Data, PCB_TYPE_PADSTACK, layer, ps);

#warning TODO: undo
	pcb_padstack_set_thermal(ps, ctx->chgtherm.lid, ctx->chgtherm.style);

	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PADSTACK, ps, ps, ps, pcb_true);
	pcb_poly_clear_from_poly(pcb->Data, PCB_TYPE_PADSTACK, layer, ps);
	pcb_padstack_invalidate_draw(ps);
	return ps;
}

