/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

#include "config.h"

#include <time.h>

#include "board.h"
#include "data.h"
#include "data_list.h"
#include "data_it.h"
#include <librnd/poly/rtree.h>
#include "list_common.h"
#include <librnd/core/compat_misc.h>
#include "layer_it.h"
#include "operation.h"
#include "flag.h"
#include "undo_old.h"
#include "undo.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_poly.h"
#include "obj_text.h"
#include "obj_pstk.h"
#include "obj_subc.h"

int pcb_layer_stack[PCB_MAX_LAYER];			/* determines the layer draw order */

pcb_buffer_t pcb_buffers[PCB_MAX_BUFFER]; /* my buffers */
rnd_bool pcb_bumped;                /* if the undo serial number has changed */

int pcb_added_lines;

static void pcb_loop_layer(pcb_board_t *pcb, pcb_layer_t *layer, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb, pcb_gfx_cb_t gcb)
{
	if (lacb != NULL)
		if (lacb(ctx, pcb, layer, 1))
			return;

	if (lcb != NULL) {
		PCB_LINE_LOOP(layer);
		{
			lcb(ctx, pcb, layer, line);
		}
		PCB_END_LOOP;
	}

	if (acb != NULL) {
		PCB_ARC_LOOP(layer);
		{
			acb(ctx, pcb, layer, arc);
		}
		PCB_END_LOOP;
	}

	if (tcb != NULL) {
		PCB_TEXT_LOOP(layer);
		{
			tcb(ctx, pcb, layer, text);
		}
		PCB_END_LOOP;
	}

	if (pocb != NULL) {
		PCB_POLY_LOOP(layer);
		{
			pocb(ctx, pcb, layer, polygon);
		}
		PCB_END_LOOP;
	}

	if (gcb != NULL) {
		PCB_GFX_LOOP(layer);
		{
			gcb(ctx, pcb, layer, gfx);
		}
		PCB_END_LOOP;
	}

	if (lacb != NULL)
		lacb(ctx, pcb, layer, 0);
}

/* callback based loops */
void pcb_loop_layers(pcb_board_t *pcb, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb, pcb_gfx_cb_t gcb)
{
	if ((lacb != NULL) || (lcb != NULL) || (acb != NULL) || (tcb != NULL) || (pocb != NULL) || (gcb != NULL)) {
		if (pcb->is_footprint) {
			int n;
			pcb_data_t *data = PCB_REAL_DATA(pcb);
			for(n = 0; n < data->LayerN; n++)
				pcb_loop_layer(pcb, &data->Layer[n], ctx, lacb, lcb, acb, tcb, pocb, gcb);
		}
		else {
			pcb_layer_it_t it;
			rnd_layer_id_t lid;
			for(lid = pcb_layer_first_all(&pcb->LayerGroups, &it); lid != -1; lid = pcb_layer_next(&it)) {
				pcb_layer_t *layer = pcb->Data->Layer + lid;
				pcb_loop_layer(pcb, layer, ctx, lacb, lcb, acb, tcb, pocb, gcb);
			}
		}
	}
}

void pcb_loop_subc(pcb_board_t *pcb, void *ctx, pcb_subc_cb_t scb)
{
	if (scb != NULL) {
		PCB_SUBC_LOOP(PCB_REAL_DATA(pcb));
		{
			scb(ctx, pcb, subc, 1);
		}
		PCB_END_LOOP;
	}
}

void pcb_loop_pstk(pcb_board_t *pcb, void *ctx, pcb_pstk_cb_t pscb)
{
	if (pscb != NULL) {
		PCB_PADSTACK_LOOP(PCB_REAL_DATA(pcb));
		{
			pscb(ctx, pcb, padstack);
		}
		PCB_END_LOOP;
	}
}

void pcb_loop_all(pcb_board_t *pcb, void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb, pcb_gfx_cb_t gcb,
	pcb_subc_cb_t scb,
	pcb_pstk_cb_t pscb
	)
{
	pcb_loop_layers(pcb, ctx, lacb, lcb, acb, tcb, pocb, gcb);
	pcb_loop_subc(pcb, ctx, scb);
	pcb_loop_pstk(pcb, ctx, pscb);
}

void pcb_data_uninit(pcb_data_t *data)
{
	pcb_layer_t *layer;
	int i, is_subc;
	long l;

	is_subc = (data->parent_type == PCB_PARENT_SUBC);

	PCB_SUBC_LOOP(data);
	{
		pcb_subc_free(subc);
	}
	PCB_END_LOOP;

	PCB_PADSTACK_LOOP(data);
	{
		pcb_pstk_free(padstack);
	}
	PCB_END_LOOP;

	list_map0(&data->Rat, pcb_rat_t, pcb_rat_free);

	for (layer = data->Layer, i = 0; i < data->LayerN; layer++, i++)
		pcb_layer_free_fields(layer, 0);

	if (!is_subc) {
		if (data->subc_tree)
			rnd_r_destroy_tree(&data->subc_tree);
		if (data->padstack_tree)
			rnd_r_destroy_tree(&data->padstack_tree);
		if (data->rat_tree)
			rnd_r_destroy_tree(&data->rat_tree);
	}

	for (layer = data->Layer, i = 0; i < PCB_MAX_LAYER; layer++, i++)
		free((char *)layer->name);

	htip_uninit(&data->id2obj);

	for(l = 0; l < data->ps_protos.used; l++)
		pcb_pstk_proto_free_fields(&data->ps_protos.array[l]);
	pcb_vtpadstack_proto_uninit(&data->ps_protos);

	memset(data, 0, sizeof(pcb_data_t));
}

void pcb_data_free(pcb_data_t *data)
{
	if (data == NULL)
		return;

	pcb_data_uninit(data);
	free(data);
}

rnd_bool pcb_data_is_empty(pcb_data_t *Data)
{
	rnd_cardinal_t i;

	if (padstacklist_length(&Data->padstack) != 0) return rnd_false;
	if (pcb_subclist_length(&Data->subc) != 0) return rnd_false;
	for (i = 0; i < Data->LayerN; i++)
		if (!pcb_layer_is_pure_empty(&(Data->Layer[i])))
			return rnd_false;

	return rnd_true;
}

rnd_box_t *pcb_data_bbox(rnd_box_t *out, pcb_data_t *Data, rnd_bool ignore_floaters)
{
	/* preset identifiers with highest and lowest possible values */
	out->X1 = out->Y1 = RND_MAX_COORD;
	out->X2 = out->Y2 = -RND_MAX_COORD;

	/* now scan for the lowest/highest X and Y coordinate */
	PCB_PADSTACK_LOOP(Data);
	{
		pcb_pstk_bbox(padstack);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, padstack))
			rnd_box_bump_box(out, &padstack->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(Data);
	{
		pcb_subc_bbox(subc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, subc))
			rnd_box_bump_box(out, &subc->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(Data);
	{
		pcb_line_bbox(line);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, line))
			rnd_box_bump_box(out, &line->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Data);
	{
		pcb_arc_bbox(arc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, arc))
			rnd_box_bump_box(out, &arc->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Data);
	{
		pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
			rnd_box_bump_box(out, &text->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Data);
	{
		pcb_poly_bbox(polygon);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, polygon))
			rnd_box_bump_box(out, &polygon->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	return (pcb_data_is_empty(Data) ? NULL : out);
}

rnd_box_t *pcb_data_bbox_naked(rnd_box_t *out, pcb_data_t *Data, rnd_bool ignore_floaters)
{
	/* preset identifiers with highest and lowest possible values */
	out->X1 = out->Y1 = RND_MAX_COORD;
	out->X2 = out->Y2 = -RND_MAX_COORD;

	/* now scan for the lowest/highest X and Y coordinate */
	PCB_PADSTACK_LOOP(Data);
	{
		pcb_pstk_bbox(padstack);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, padstack))
			rnd_box_bump_box(out, &padstack->bbox_naked);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(Data);
	{
		pcb_subc_bbox(subc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, subc))
			rnd_box_bump_box(out, &subc->bbox_naked);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(Data);
	{
		pcb_line_bbox(line);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, line))
			rnd_box_bump_box(out, &line->bbox_naked);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Data);
	{
		pcb_arc_bbox(arc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, arc))
			rnd_box_bump_box(out, &arc->bbox_naked);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Data);
	{
		pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
			rnd_box_bump_box(out, &text->bbox_naked);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Data);
	{
		pcb_poly_bbox(polygon);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, polygon))
			rnd_box_bump_box(out, &polygon->bbox_naked);
	}
	PCB_ENDALL_LOOP;
	return (pcb_data_is_empty(Data) ? NULL : out);
}

void pcb_data_set_layer_parents(pcb_data_t *data)
{
	rnd_layer_id_t n;
	for(n = 0; n < PCB_MAX_LAYER; n++)
		pcb_layer_setup(&data->Layer[n], data);
}

void pcb_data_bind_board_layers(pcb_board_t *pcb, pcb_data_t *data, int share_rtrees)
{
	rnd_layer_id_t n;
	for(n = 0; n < pcb->Data->LayerN; n++) {
		pcb_layer_real2bound(&data->Layer[n], &pcb->Data->Layer[n], share_rtrees);
		data->Layer[n].parent.data = data;
		data->Layer[n].parent_type = PCB_PARENT_DATA;
		data->Layer[n].type = PCB_OBJ_LAYER;
	}
	data->LayerN = pcb->Data->LayerN;
}

void pcb_data_make_layers_bound(pcb_board_t *pcb4layer_groups, pcb_data_t *data)
{
	rnd_layer_id_t n;
	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_type_t lyt = pcb_layergrp_flags(pcb4layer_groups, data->Layer[n].meta.real.grp);
		memset(&data->Layer[n].meta.bound, 0, sizeof(data->Layer[n].meta.bound));
		pcb_layer_real2bound_offs(&data->Layer[n], pcb4layer_groups, &data->Layer[n]);
		data->Layer[n].parent.data = data;
		data->Layer[n].parent_type = PCB_PARENT_DATA;
		data->Layer[n].type = PCB_OBJ_LAYER;
		data->Layer[n].meta.bound.type = lyt;
	}
}

void pcb_data_unbind_layers(pcb_data_t *data)
{
	rnd_layer_id_t n;
	for(n = 0; n < data->LayerN; n++)
		data->Layer[n].meta.bound.real = NULL;
}

void pcb_data_binding_update(pcb_board_t *pcb, pcb_data_t *data)
{
	int i;
	for(i = 0; i < data->LayerN; i++) {
		pcb_layer_t *sourcelayer = &data->Layer[i];
		pcb_layer_t *destlayer = pcb_layer_resolve_binding(pcb, sourcelayer);
		sourcelayer->meta.bound.real = destlayer;
	}
}

void pcb_data_init(pcb_data_t *data)
{
	memset(data, 0, sizeof(pcb_data_t));
	htip_init(&data->id2obj, longhash, longkeyeq);
}

pcb_data_t *pcb_data_new(pcb_board_t *parent)
{
	pcb_data_t *data;
	data = malloc(sizeof(pcb_data_t));
	pcb_data_init(data);
	if (parent != NULL)
		PCB_SET_PARENT(data, board, parent);
	pcb_data_set_layer_parents(data);
	return data;
}

pcb_board_t *pcb_data_get_top(pcb_data_t *data)
{
	while((data != NULL) && (data->parent_type == PCB_PARENT_SUBC))
		data = data->parent.subc->parent.data;

	if (data == NULL)
		return NULL;

	if (data->parent_type == PCB_PARENT_BOARD)
		return data->parent.board;

	return NULL;
}

void pcb_data_mirror(pcb_data_t *data, rnd_coord_t y_offs, pcb_data_mirror_text_t mtxt, rnd_bool pstk_smirror, rnd_bool undoable)
{
	pcb_undo_freeze_serial();
	pcb_data_clip_inhibit_inc(data);
	PCB_PADSTACK_LOOP(data);
	{
		pcb_pstk_mirror(padstack, y_offs, pstk_smirror, 0, undoable);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		pcb_subc_mirror(data, subc, y_offs, pstk_smirror, undoable);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(data);
	{
		pcb_line_mirror(line, y_offs, undoable);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		pcb_arc_mirror(arc, y_offs, undoable);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		pcb_poly_mirror(polygon, y_offs, undoable);
	}
	PCB_ENDALL_LOOP;

	switch(mtxt & PCB_TXM_MASK_OP) {
		case PCB_TXM_NONE:
			break;
		case PCB_TXM_SIDE:
			PCB_TEXT_ALL_LOOP(data);
			{
				pcb_text_flip_side(layer, text, y_offs, undoable);
			}
			PCB_ENDALL_LOOP;
			break;
		case PCB_TXM_COORD:
			PCB_TEXT_ALL_LOOP(data);
			{
				pcb_text_mirror_coords(text, y_offs, undoable);
			}
			PCB_ENDALL_LOOP;
			break;
	}

	if(mtxt & PCB_TXM_ROT) {
		PCB_TEXT_ALL_LOOP(data);
		{
			if (text->rot != 0) {
				text->rot = -text->rot;
			}
		}
		PCB_ENDALL_LOOP;
	}

	pcb_data_clip_inhibit_dec(data, 0);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();
}

void pcb_data_scale(pcb_data_t *data, double sx, double sy, double sth, int recurse)
{
	int undoable = 0;

	if ((sx == 1.0) && (sy == 1.0) && (sth == 1.0))
		return;

	PCB_PADSTACK_LOOP(data);
	{
		pcb_pstk_scale(padstack, sx, sy, undoable);
	}
	PCB_END_LOOP;
	if (recurse) {
		PCB_SUBC_LOOP(data);
		{
			pcb_subc_scale(data, subc, sx, sy, sth, 1);
		}
		PCB_END_LOOP;
	}
	else {
		/* when not scaled recursively, position still needs to be scaled */
		PCB_SUBC_LOOP(data);
		{
			rnd_coord_t ox, oy, nx, ny;
			if (pcb_subc_get_origin(subc, &ox, &oy) == 0) {
				nx = rnd_round((double)ox * sx);
				ny = rnd_round((double)oy * sy);
				pcb_subc_move(subc, nx - ox, ny - oy, rnd_true);
			}
		}
		PCB_END_LOOP;
	}
	PCB_LINE_ALL_LOOP(data);
	{
		pcb_line_scale(line, sx, sy, sth);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		pcb_arc_scale(arc, sx, sy, sth);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		pcb_poly_scale(polygon, sx, sy);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(data);
	{
		pcb_text_scale(text, sx, sy, sth);
	}
	PCB_ENDALL_LOOP;
}


int pcb_data_normalize_(pcb_data_t *data, rnd_box_t *data_bbox)
{
	rnd_box_t tmp;
	rnd_coord_t dx = 0, dy = 0;

	if (data_bbox == NULL) {
		data_bbox = &tmp;
		if (pcb_data_bbox(data_bbox, data, rnd_false) == NULL)
			return -1;
	}

	if (data_bbox->X1 < 0)
		dx = -data_bbox->X1;
	if (data_bbox->Y1 < 0)
		dy = -data_bbox->Y1;

	if ((dx > 0) || (dy > 0)) {
		pcb_data_move(data, dx, dy, 0);
		return 1;
	}

	return 0;
}

int pcb_data_normalize(pcb_data_t *data)
{
	return pcb_data_normalize_(data, NULL);
}

void pcb_data_set_parent_globals(pcb_data_t *data, pcb_data_t *new_parent)
{
	pcb_pstk_t *ps;
	pcb_subc_t *sc;
	gdl_iterator_t it;

	padstacklist_foreach(&data->padstack, &it, ps) {
		ps->parent_type = PCB_PARENT_DATA;
		ps->parent.data = new_parent;
	}
	subclist_foreach(&data->subc, &it, sc) {
		sc->parent_type = PCB_PARENT_DATA;
		sc->parent.data = new_parent;
	}
}


extern pcb_opfunc_t MoveFunctions;
void pcb_data_move(pcb_data_t *data, rnd_coord_t dx, rnd_coord_t dy, int undoable)
{
	pcb_opctx_t ctx;

	ctx.move.pcb = pcb_data_get_top(data);
	ctx.move.dx = dx;
	ctx.move.dy = dy;

	PCB_PADSTACK_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_PSTK, data, padstack, padstack, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_PSTK, padstack, padstack, padstack);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, data, subc, subc, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_SUBC, subc, subc, subc);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_LINE, layer, line, line, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_LINE, layer, line, line);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_ARC, layer, arc, arc, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_ARC, layer, arc, arc);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_TEXT, layer, text, text, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_TEXT, layer, text, text);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_POLY, layer, polygon, polygon, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_POLY, layer, polygon, polygon);
	}
	PCB_ENDALL_LOOP;
	PCB_GFX_ALL_LOOP(data);
	{
		if (undoable) pcb_undo_add_obj_to_move(PCB_OBJ_GFX, layer, gfx, gfx, dx, dy);
		pcb_object_operation(&MoveFunctions, &ctx, PCB_OBJ_GFX, layer, gfx, gfx);
	}
	PCB_ENDALL_LOOP;
}

void pcb_data_list_by_flag(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type, unsigned long mask)
{
	if (type & PCB_OBJ_PSTK) PCB_PADSTACK_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, padstack))) vtp0_append(dst, padstack);
	} PCB_END_LOOP;
	if (type & PCB_OBJ_SUBC) PCB_SUBC_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, subc))) vtp0_append(dst, subc);
	} PCB_END_LOOP;
	if (type & PCB_OBJ_LINE) PCB_LINE_ALL_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, line))) vtp0_append(dst, line);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_ARC) PCB_ARC_ALL_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, arc))) vtp0_append(dst, arc);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_TEXT) PCB_TEXT_ALL_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, text))) vtp0_append(dst, text);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_POLY) PCB_POLY_ALL_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, polygon))) vtp0_append(dst, polygon);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_GFX) PCB_GFX_ALL_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, gfx))) vtp0_append(dst, gfx);
	} PCB_ENDALL_LOOP;
}

void pcb_data_list_terms(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type)
{
TODO("subc TODO: subc-in-subc")
/*	if (type & PCB_OBJ_SUBC) PCB_SUBC_LOOP(data); {
		if (subc->term != NULL) vtp0_append(dst, subc);
	} PCB_END_LOOP;*/
	if (type & PCB_OBJ_LINE) PCB_LINE_ALL_LOOP(data); {
		if ((line->term != NULL)) vtp0_append(dst, line);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_ARC) PCB_ARC_ALL_LOOP(data); {
		if (arc->term != NULL) vtp0_append(dst, arc);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_TEXT) PCB_TEXT_ALL_LOOP(data); {
		if (text->term != NULL) vtp0_append(dst, text);
	} PCB_ENDALL_LOOP;
	if (type & PCB_OBJ_POLY) PCB_POLY_ALL_LOOP(data); {
		if (polygon->term != NULL) vtp0_append(dst, polygon);
	} PCB_ENDALL_LOOP;
}

void pcb_data_clip_polys(pcb_data_t *data)
{
	PCB_POLY_ALL_LOOP(data);
	{
		pcb_poly_init_clip(data, layer, polygon);
	}
	PCB_ENDALL_LOOP;
}

#define rsearch(tree) \
	do { \
		rnd_r_dir_t tmp = rnd_r_search(tree, starting_region, region_in_search, rectangle_in_region, closure, num_found); \
		if (tmp == RND_R_DIR_CANCEL) return tmp; \
		res |= tmp; \
	} while(0);

rnd_r_dir_t pcb_data_r_search(pcb_data_t *data, pcb_objtype_t types, const rnd_box_t *starting_region,
						 rnd_r_dir_t (*region_in_search) (const rnd_box_t *region, void *cl),
						 rnd_r_dir_t (*rectangle_in_region) (const rnd_box_t *box, void *cl),
						 void *closure, int *num_found, rnd_bool vis_only)
{
	rnd_layer_id_t lid;
	rnd_r_dir_t res = 0;

	if (!vis_only || PCB->RatOn)
		if (types & PCB_OBJ_RAT)  rsearch(data->rat_tree);

	if (!vis_only || PCB->pstk_on || PCB->hole_on)
		if (types & PCB_OBJ_PSTK) rsearch(data->padstack_tree);

	if (!vis_only || PCB->SubcOn)
		if (types & PCB_OBJ_SUBC) rsearch(data->subc_tree);

	for(lid = 0; lid < data->LayerN; lid++) {
		pcb_layer_t *ly = data->Layer + lid;

		if (vis_only && (!ly->meta.real.vis))
			continue;

		if (types & PCB_OBJ_LINE) rsearch(ly->line_tree);
		if (types & PCB_OBJ_TEXT) rsearch(ly->text_tree);
		if (types & PCB_OBJ_POLY) rsearch(ly->polygon_tree);
		if (types & PCB_OBJ_ARC)  rsearch(ly->arc_tree);
		if (types & PCB_OBJ_GFX)  rsearch(ly->gfx_tree);
	}

	return res;
}

/*** poly clip inhibit mechanism ***/
void pcb_data_clip_inhibit_inc(pcb_data_t *data)
{
	int old = data->clip_inhibit;
	data->clip_inhibit++;

	if (old > data->clip_inhibit) {
		rnd_message(RND_MSG_ERROR, "Internal error: overflow on poly clip inhibit\n");
		abort();
	}
}

void pcb_data_clip_inhibit_dec(pcb_data_t *data, rnd_bool enable_progbar)
{
	if (data->clip_inhibit == 0) {
		rnd_message(RND_MSG_ERROR, "Internal error: overflow on poly clip inhibit\n");
		assert(!"clip_inhibit underflow");
		return;
	}
	data->clip_inhibit--;

	if (data->clip_inhibit == 0)
		pcb_data_clip_dirty(data, enable_progbar);
}

typedef struct {
	pcb_data_t *data;
	time_t nextt;
	int inited, force_all;
	rnd_cardinal_t at, total;
} data_clip_all_t;

static void data_clip_all_cb(void *ctx_)
{
	data_clip_all_t *ctx = ctx_;

	ctx->at++;
	if ((ctx->at % 8) == 0) {
		time_t now = time(NULL);
		if (now >= ctx->nextt) {
			ctx->nextt = now+1;
			if (!ctx->inited) {
				ctx->total = 0;
				PCB_POLY_ALL_LOOP(ctx->data); {
					if (ctx->force_all || polygon->clip_dirty)
						ctx->total += pcb_poly_num_clears(ctx->data, layer, polygon);
				} PCB_ENDALL_LOOP;
				ctx->inited = 1;
			}
			if (rnd_hid_progress(ctx->at, ctx->total, "Clipping polygons...") != 0) {
				int rv = rnd_hid_message_box(&PCB->hidlib, "warning", "Stop poly clipping", "The only way to cancel poly clipping is to quit pcb-rnd.\nAre you sure you want to quit?", "yes, quit pcb-rnd", 1, "no, continue clipping", 2, NULL);
				if (rv == 1) {
					exit(1);
				}
				else {
					/* Have to recreate the dialog next time, that's the only way to get it out from the cancel state */
					rnd_hid_progress(0, 0, NULL);
				}
			}
		}
	}
}

void pcb_data_clip_all_poly(pcb_data_t *data, rnd_bool enable_progbar, rnd_bool force_all)
{
	data_clip_all_t ctx;

	if (data->clip_inhibit != 0)
		return;

	PCB_SUBC_LOOP(data); {
		pcb_data_clip_dirty(subc->data, enable_progbar);
	} PCB_END_LOOP;

	ctx.data = data;
	ctx.force_all = force_all;
	ctx.nextt = time(NULL) + 2;
	ctx.total = ctx.at = 0;
	ctx.inited = 0;

	/* have to go in two passes, to make sure that clearing polygons are done
	   before the polygons that are potentially being cleared - this way we
	   guarantee that by the time the poly-vs-poly clearance needs to be
	   calculated, the clearing poly has a contour */
	PCB_POLY_ALL_LOOP(data); {
		if ((force_all || polygon->clip_dirty) && (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, polygon)))
			pcb_poly_init_clip_prog(data, layer, polygon, (enable_progbar ? data_clip_all_cb : NULL), &ctx, 0);
	} PCB_ENDALL_LOOP;

	PCB_POLY_ALL_LOOP(data); {
		if ((force_all || polygon->clip_dirty) && (!PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, polygon)))
			pcb_poly_init_clip_prog(data, layer, polygon, (enable_progbar ? data_clip_all_cb : NULL), &ctx, 0);
	} PCB_ENDALL_LOOP;

	if (enable_progbar)
		rnd_hid_progress(0, 0, NULL);
}

void pcb_data_clip_dirty(pcb_data_t *data, rnd_bool enable_progbar)
{
	pcb_data_clip_all_poly(data, enable_progbar, rnd_false);
}

void pcb_data_clip_all(pcb_data_t *data, rnd_bool enable_progbar)
{
	pcb_data_clip_all_poly(data, enable_progbar, rnd_true);
}


void pcb_data_flag_change(pcb_data_t *data, pcb_objtype_t mask, int how, unsigned long flags)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, mask); o != NULL; o = pcb_data_next(&it)) {
		PCB_FLAG_CHANGE(how, flags, o);
		if (o->type == PCB_OBJ_SUBC)
			pcb_data_flag_change(((pcb_subc_t *)o)->data, mask, how, flags);
	}
}

#include "obj_pstk_draw.h"
#include "obj_text_draw.h"
#include "obj_poly_draw.h"
#include "obj_arc_draw.h"
#include "obj_line_draw.h"
#include "conf_core.h"
#include "undo.h"

/* Check if object n has flag set and if so, clear it and do all administration */
#define CHK_CLEAR(n) \
do { \
	if (PCB_FLAG_TEST(flag, (pcb_any_obj_t *)n)) { \
		if (undoable) \
			pcb_undo_add_obj_to_flag((pcb_any_obj_t *)n); \
		PCB_FLAG_CLEAR(flag, (pcb_any_obj_t *)n); \
		if (redraw) \
			pcb_pstk_invalidate_draw((pcb_pstk_t *)n); \
		cnt++; \
	} \
} while(0)

unsigned long pcb_data_clear_obj_flag(pcb_data_t *data, pcb_objtype_t tmask, unsigned long flag, int redraw, int undoable)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;
	int li;
	pcb_layer_t *l;
	unsigned long cnt = 0;

	if (flag & PCB_FLAG_WARN)
		conf_core.temp.rat_warn = rnd_false;

	if (tmask & PCB_OBJ_PSTK)
		for(n = rnd_r_first(data->padstack_tree, &it); n != NULL; n = rnd_r_next(&it))
			CHK_CLEAR(n);

	if (tmask & PCB_OBJ_RAT)
		for(n = rnd_r_first(data->rat_tree, &it); n != NULL; n = rnd_r_next(&it))
			CHK_CLEAR(n);

	if (tmask & PCB_OBJ_SUBC)
		for(n = rnd_r_first(data->subc_tree, &it); n != NULL; n = rnd_r_next(&it))
			CHK_CLEAR(n);

	if ((tmask & (PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT | PCB_OBJ_GFX)) == 0)
		return cnt; /* do not run the layer loop if no layer object is requested */

	for(li = 0, l = data->Layer; li < data->LayerN; li++,l++) {
		if (tmask & PCB_OBJ_LINE)
			for(n = rnd_r_first(l->line_tree, &it); n != NULL; n = rnd_r_next(&it))
				CHK_CLEAR(n);

		if (tmask & PCB_OBJ_ARC)
			for(n = rnd_r_first(l->arc_tree, &it); n != NULL; n = rnd_r_next(&it))
				CHK_CLEAR(n);

		if (tmask & PCB_OBJ_POLY)
			for(n = rnd_r_first(l->polygon_tree, &it); n != NULL; n = rnd_r_next(&it))
				CHK_CLEAR(n);

		if (tmask & PCB_OBJ_TEXT)
			for(n = rnd_r_first(l->text_tree, &it); n != NULL; n = rnd_r_next(&it))
				CHK_CLEAR(n);

		if (tmask & PCB_OBJ_GFX)
			for(n = rnd_r_first(l->gfx_tree, &it); n != NULL; n = rnd_r_next(&it))
				CHK_CLEAR(n);
	}
	return cnt;
}
#undef CHK_CLEAR

unsigned long pcb_data_clear_flag(pcb_data_t *data, unsigned long flag, int redraw, int undoable)
{
	return pcb_data_clear_obj_flag(data, PCB_OBJ_CLASS_REAL, flag, redraw, undoable);
}


void pcb_data_dynflag_clear(pcb_data_t *data, pcb_dynf_t dynf)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;
	int li;
	pcb_layer_t *l;

	for(n = rnd_r_first(data->padstack_tree, &it); n != NULL; n = rnd_r_next(&it))
		PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

	for(n = rnd_r_first(data->subc_tree, &it); n != NULL; n = rnd_r_next(&it))
		PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

	for(n = rnd_r_first(data->rat_tree, &it); n != NULL; n = rnd_r_next(&it))
		PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

	for(li = 0, l = data->Layer; li < data->LayerN; li++,l++) {
		for(n = rnd_r_first(l->line_tree, &it); n != NULL; n = rnd_r_next(&it))
			PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

		for(n = rnd_r_first(l->arc_tree, &it); n != NULL; n = rnd_r_next(&it))
			PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

		for(n = rnd_r_first(l->polygon_tree, &it); n != NULL; n = rnd_r_next(&it))
			PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

		for(n = rnd_r_first(l->text_tree, &it); n != NULL; n = rnd_r_next(&it))
			PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);

		for(n = rnd_r_first(l->gfx_tree, &it); n != NULL; n = rnd_r_next(&it))
			PCB_DFLAG_CLR(&((pcb_any_obj_t *)n)->Flags, dynf);
	}
}

static pcb_data_t *pcb_data_by_name_(pcb_board_t *pcb, const char **name, int *addr)
{
	if (name == NULL)
		return NULL;

	if (rnd_strncasecmp(*name, "pcb", 3) == 0) {
		(*name) += 3;
		if (**name == '/')
			(*name)++;
		*addr = 1;
		return pcb->Data;
	}
	else if (rnd_strncasecmp(*name, "buffer#", 7) == 0) {
		char *end;
		long idx = strtol((*name)+7, &end, 10);
		if ((*end == '\0') && (idx >= 0) && (idx < PCB_MAX_BUFFER)) {
			*name = end;
			if (**name == '/')
				(*name)++;
			*addr = idx+2;
			return pcb_buffers[idx].Data;
		}
	}
	else if (rnd_strncasecmp(*name, "buffer", 6) == 0) {
		(*name) += 6;
		if (**name == '/')
			(*name)++;
		*addr = conf_core.editor.buffer_number+2;
		return PCB_PASTEBUFFER->Data;
	}
	*addr = 0;
	return NULL;
}

char *pcb_data_name_by_addr(int addr, char *buf)
{
	if (addr == 0) *buf = '\0';
	else if (addr == 1) strcpy(buf, "pcb");
	else rnd_snprintf(buf, 16, "buffer#%d", addr-2);
	return buf;
}

pcb_data_t *pcb_data_by_name(pcb_board_t *pcb, const char **name)
{
	int dummy;
	return pcb_data_by_name_(pcb, name, &dummy);
}

int pcb_data_addr_by_name(pcb_board_t *pcb, const char **name)
{
	int dummy;
	pcb_data_by_name_(pcb, name, &dummy);
	return dummy;
}

const char *pcb_data_to_name(pcb_board_t *pcb, pcb_data_t *data, char *buf, int buf_len)
{
	int n;

	assert(buf_len > 15);
	if (buf_len < 16)
		return NULL;

	if (data == pcb->Data)
		return "pcb";

	for(n = 0; n < PCB_MAX_BUFFER; n++) {
		if (data == pcb_buffers[n].Data) {
			rnd_sprintf(buf, "buffer#%d", n);
			return buf;
		}
	}

	return NULL;
}
