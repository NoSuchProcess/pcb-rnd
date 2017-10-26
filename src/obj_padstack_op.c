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

void *pcb_padstackop_add_to_buffer(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_t *p = pcb_padstack_new(ctx->buffer.dst, ps->proto, ps->x, ps->y, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	return pcb_padstack_copy_meta(p, ps);
}

void *pcb_padstackop_move_to_buffer(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_PADSTACK, ps, ps);
	pcb_r_delete_entry(ctx->buffer.src->padstack_tree, (pcb_box_t *)ps);

	padstacklist_remove(ps);
	padstacklist_append(&ctx->buffer.dst->padstack, ps);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, ps);

	if (!ctx->buffer.dst->padstack_tree)
		ctx->buffer.dst->padstack_tree = pcb_r_create_tree(NULL, 0, 0);

	pcb_r_insert_entry(ctx->buffer.dst->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_PADSTACK, ps, ps);

	PCB_SET_PARENT(ps, data, ctx->buffer.dst);
	return ps;
}

void *pcb_padstackop_copy(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_padstack_t *nps;
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);
	nps = pcb_padstack_new(data, ps->proto, ps->x + ctx->copy.DeltaX, ps->y + ctx->copy.DeltaY, pcb_flag_mask(ps->Flags, PCB_FLAG_FOUND));
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
	pcb_poly_restore_to_poly(data, PCB_TYPE_PADSTACK, ps, ps);
	pcb_padstackop_move_noclip(ctx, ps);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	pcb_poly_clear_from_poly(data, PCB_TYPE_PADSTACK, ps, ps);
	return ps;
}

void *pcb_padstackop_clip(pcb_opctx_t *ctx, pcb_padstack_t *ps)
{
	pcb_data_t *data = ps->parent.data;
	assert(ps->parent_type = PCB_PARENT_DATA);

	if (ctx->clip.restore) {
		pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)ps);
		pcb_poly_restore_to_poly(data, PCB_TYPE_PADSTACK, ps, ps);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
		pcb_poly_clear_from_poly(data, PCB_TYPE_PADSTACK, ps, ps);
	}

	return ps;
}

