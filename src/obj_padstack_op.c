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

void *pcb_padstackop_move_to_buffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_line_t * line)
{

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
	pcb_line_invalidate_erase(ps);
	pcb_padstack_move(ps, ctx->move.dx, ctx->move.dy);
	pcb_line_invalidate_draw(ps);
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

