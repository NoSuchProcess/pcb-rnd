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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* just defines common identifiers
 */
#include "config.h"

#include "board.h"
#include "data.h"
#include "data_list.h"
#include "rtree.h"
#include "list_common.h"
#include "obj_all.h"
#include "layer_it.h"
#include "operation.h"

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */
int pcb_layer_stack[PCB_MAX_LAYER];			/* determines the layer draw order */

pcb_buffer_t pcb_buffers[PCB_MAX_BUFFER]; /* my buffers */
pcb_bool pcb_bumped;                /* if the undo serial number has changed */

int pcb_added_lines;

static void pcb_loop_layer(pcb_board_t *pcb, pcb_layer_t *layer, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb)
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
	if (lacb != NULL)
		lacb(ctx, pcb, layer, 0);
}

/* callback based loops */
void pcb_loop_layers(pcb_board_t *pcb, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb)
{
	if ((lacb != NULL) || (lcb != NULL) || (acb != NULL) || (tcb != NULL) || (pocb != NULL)) {
		if (pcb->is_footprint) {
			int n;
			pcb_data_t *data = PCB_REAL_DATA(pcb);
			for(n = 0; n < data->LayerN; n++)
				pcb_loop_layer(pcb, &data->Layer[n], ctx, lacb, lcb, acb, tcb, pocb);
		}
		else {
			pcb_layer_it_t it;
			pcb_layer_id_t lid;
			for(lid = pcb_layer_first_all(&pcb->LayerGroups, &it); lid != -1; lid = pcb_layer_next(&it)) {
				pcb_layer_t *layer = pcb->Data->Layer + lid;
				pcb_loop_layer(pcb, layer, ctx, lacb, lcb, acb, tcb, pocb);
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


void pcb_loop_elements(pcb_board_t *pcb, void *ctx, pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb)
{
	if ((ecb != NULL) || (elcb != NULL) || (eacb != NULL)  || (etcb != NULL) || (epicb != NULL) || (epacb != NULL)) {
		PCB_ELEMENT_LOOP(PCB_REAL_DATA(pcb));
		{
			if (ecb != NULL)
				if (ecb(ctx, pcb, element, 1))
					continue;

			if (elcb != NULL) {
				PCB_ELEMENT_PCB_LINE_LOOP(element);
				{
					elcb(ctx, pcb, element, line);
				}
				PCB_END_LOOP;
			}

			if (eacb != NULL) {
				PCB_ELEMENT_ARC_LOOP(element);
				{
					eacb(ctx, pcb, element, arc);
				}
				PCB_END_LOOP;
			}

			if (etcb != NULL) {
				PCB_ELEMENT_PCB_TEXT_LOOP(element);
				{
					etcb(ctx, pcb, element, text);
				}
				PCB_END_LOOP;
			}

			if (epicb != NULL) {
				PCB_PIN_LOOP(element);
				{
					epicb(ctx, pcb, element, pin);
				}
				PCB_END_LOOP;
			}


			if (epacb != NULL) {
				PCB_PAD_LOOP(element);
				{
					epacb(ctx, pcb, element, pad);
				}
				PCB_END_LOOP;
			}

			if (ecb != NULL)
				ecb(ctx, pcb, element, 0);
		}
		PCB_END_LOOP;
	}
}

void pcb_loop_vias(pcb_board_t *pcb, void *ctx, pcb_via_cb_t vcb)
{
	if (vcb != NULL) {
		PCB_VIA_LOOP(PCB_REAL_DATA(pcb));
		{
			vcb(ctx, pcb, via);
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
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb,
	pcb_subc_cb_t scb,
	pcb_via_cb_t vcb, pcb_pstk_cb_t pscb
	)
{
	pcb_loop_layers(pcb, ctx, lacb, lcb, acb, tcb, pocb);
	pcb_loop_elements(pcb, ctx, ecb, elcb, eacb, etcb, epicb, epacb);
	pcb_loop_subc(pcb, ctx, scb);
	pcb_loop_vias(pcb, ctx, vcb);
	pcb_loop_pstk(pcb, ctx, pscb);
}

/* ---------------------------------------------------------------------------
 * free memory used by data struct
 */
void pcb_data_free(pcb_data_t * data)
{
	pcb_layer_t *layer;
	int i, subc;

	if (data == NULL)
		return;

	subc = (data->parent_type == PCB_PARENT_SUBC);


	PCB_VIA_LOOP(data);
	{
		free(via->Name);
	}
	PCB_END_LOOP;
	list_map0(&data->Via, pcb_pin_t, pcb_via_free);
	PCB_ELEMENT_LOOP(data);
	{
		pcb_element_destroy(element);
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(data);
	{
		pcb_subc_free(subc);
	}
	PCB_END_LOOP;

	list_map0(&data->Element, pcb_element_t, pcb_element_free);
	list_map0(&data->Rat, pcb_rat_t, pcb_rat_free);

	for (layer = data->Layer, i = 0; i < data->LayerN; layer++, i++) {
		if (!layer->is_bound)
			pcb_attribute_free(&layer->meta.real.Attributes);
		PCB_TEXT_LOOP(layer);
		{
			free(text->TextString);
		}
		PCB_END_LOOP;
		PCB_LINE_LOOP(layer);
		{
			if (line->Number)
				free(line->Number);
		}
		PCB_END_LOOP;

		list_map0(&layer->Line, pcb_line_t, pcb_line_free);
		list_map0(&layer->Arc,  pcb_arc_t,  pcb_arc_free);
		list_map0(&layer->Text, pcb_text_t, pcb_text_free);
		PCB_POLY_LOOP(layer);
		{
			pcb_poly_free_fields(polygon);
		}
		PCB_END_LOOP;
		list_map0(&layer->Polygon, pcb_poly_t, pcb_poly_free);
		if (!layer->is_bound) {
			if (layer->line_tree)
				pcb_r_destroy_tree(&layer->line_tree);
			if (layer->arc_tree)
				pcb_r_destroy_tree(&layer->arc_tree);
			if (layer->text_tree)
				pcb_r_destroy_tree(&layer->text_tree);
			if (layer->polygon_tree)
				pcb_r_destroy_tree(&layer->polygon_tree);
		}
	}

	if (!subc) {
		if (data->element_tree)
			pcb_r_destroy_tree(&data->element_tree);
		if (data->subc_tree)
			pcb_r_destroy_tree(&data->subc_tree);
		for (i = 0; i < PCB_MAX_ELEMENTNAMES; i++)
			if (data->name_tree[i])
				pcb_r_destroy_tree(&data->name_tree[i]);
		if (data->via_tree)
			pcb_r_destroy_tree(&data->via_tree);
		if (data->pin_tree)
			pcb_r_destroy_tree(&data->pin_tree);
		if (data->pad_tree)
			pcb_r_destroy_tree(&data->pad_tree);
		if (data->padstack_tree)
			pcb_r_destroy_tree(&data->padstack_tree);
		if (data->rat_tree)
			pcb_r_destroy_tree(&data->rat_tree);
	}

	for (layer = data->Layer, i = 0; i < PCB_MAX_LAYER; layer++, i++)
		free((char *)layer->name);

	/* clear struct */
	memset(data, 0, sizeof(pcb_data_t));
}

/* ---------------------------------------------------------------------------
 * returns pcb_true if data area is empty
 */
pcb_bool pcb_data_is_empty(pcb_data_t *Data)
{
	pcb_bool hasNoObjects;
	pcb_cardinal_t i;

	if (pinlist_length(&Data->Via) != 0) return pcb_false;
	if (padstacklist_length(&Data->padstack) != 0) return pcb_false;
	if (elementlist_length(&Data->Element) != 0) return pcb_false;
	for (i = 0; i < Data->LayerN; i++)
		if (!pcb_layer_is_empty_(PCB, &(Data->Layer[i])))
			return pcb_false;

	return pcb_true;
}

pcb_box_t *pcb_data_bbox(pcb_box_t *out, pcb_data_t *Data, pcb_bool ignore_floaters)
{
	/* preset identifiers with highest and lowest possible values */
	out->X1 = out->Y1 = PCB_MAX_COORD;
	out->X2 = out->Y2 = -PCB_MAX_COORD;

	/* now scan for the lowest/highest X and Y coordinate */
	PCB_VIA_LOOP(Data);
	{
		pcb_pin_bbox(via);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, via))
			pcb_box_bump_box(out, &via->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_PADSTACK_LOOP(Data);
	{
		pcb_pstk_bbox(padstack);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, padstack))
			pcb_box_bump_box(out, &padstack->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(Data);
	{
		pcb_element_bbox(Data, element, pcb_font(PCB, 0, 0));
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, element))
			pcb_box_bump_box(out, &element->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(Data);
	{
		pcb_subc_bbox(subc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, subc))
			pcb_box_bump_box(out, &subc->BoundingBox);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(Data);
	{
		pcb_line_bbox(line);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, line))
			pcb_box_bump_box(out, &line->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Data);
	{
		pcb_arc_bbox(arc);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, arc))
			pcb_box_bump_box(out, &arc->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Data);
	{
		pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
			pcb_box_bump_box(out, &text->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Data);
	{
		pcb_poly_bbox(polygon);
		if (!ignore_floaters || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, polygon))
			pcb_box_bump_box(out, &polygon->BoundingBox);
	}
	PCB_ENDALL_LOOP;
	return (pcb_data_is_empty(Data) ? NULL : out);
}

void pcb_data_set_layer_parents(pcb_data_t *data)
{
	pcb_layer_id_t n;
	for(n = 0; n < PCB_MAX_LAYER; n++)
		data->Layer[n].parent = data;
}

void pcb_data_bind_board_layers(pcb_board_t *pcb, pcb_data_t *data, int share_rtrees)
{
	pcb_layer_id_t n;
	for(n = 0; n < pcb->Data->LayerN; n++) {
		pcb_layer_real2bound(&data->Layer[n], &pcb->Data->Layer[n], share_rtrees);
		data->Layer[n].parent = data;
	}
	data->LayerN = pcb->Data->LayerN;
}

void pcb_data_make_layers_bound(pcb_board_t *pcb4layer_groups, pcb_data_t *data)
{
	pcb_layer_id_t n;
	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_type_t lyt = pcb_layergrp_flags(pcb4layer_groups, data->Layer[n].meta.real.grp);
		pcb_layer_real2bound_offs(&data->Layer[n], pcb4layer_groups, &data->Layer[n]);
		data->Layer[n].parent = data;
		data->Layer[n].meta.bound.type = lyt;
	}
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

pcb_data_t *pcb_data_new(pcb_board_t *parent)
{
	pcb_data_t *data;
	data = (pcb_data_t *) calloc(1, sizeof(pcb_data_t));
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

void pcb_data_mirror(pcb_data_t *data, pcb_coord_t y_offs, pcb_bool text_too, pcb_bool pstk_smirror)
{
	PCB_VIA_LOOP(data);
	{
		pcb_via_mirror(data, via, y_offs);
	}
	PCB_END_LOOP;
	PCB_PADSTACK_LOOP(data);
	{
		pcb_pstk_mirror(padstack, y_offs, pstk_smirror);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		pcb_subc_mirror(data, subc, y_offs, pstk_smirror);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(data);
	{
		pcb_line_mirror(layer, line, y_offs);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		pcb_arc_mirror(layer, arc, y_offs);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		pcb_poly_mirror(layer, polygon, y_offs);
	}
	PCB_ENDALL_LOOP;

	if (text_too) {
		PCB_TEXT_ALL_LOOP(data);
		{
			pcb_text_flip_side(layer, text, y_offs);
		}
		PCB_ENDALL_LOOP;
	}
}

int pcb_data_normalize_(pcb_data_t *data, pcb_box_t *data_bbox)
{
	pcb_box_t tmp;
	pcb_coord_t dx = 0, dy = 0;

	if (data_bbox == NULL) {
		data_bbox = &tmp;
		if (pcb_data_bbox(data_bbox, data, pcb_false) == NULL)
			return -1;
	}

	if (data_bbox->X1 < 0)
		dx = -data_bbox->X1;
	if (data_bbox->Y1 < 0)
		dy = -data_bbox->Y1;

	if ((dx > 0) || (dy > 0)) {
		pcb_data_move(data, dx, dy);
		return 1;
	}

	return 0;
}

int pcb_data_normalize(pcb_data_t *data)
{
	return pcb_data_normalize_(data, NULL);
}


extern pcb_opfunc_t MoveFunctions;
void pcb_data_move(pcb_data_t *data, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_opctx_t ctx;

	ctx.move.pcb = pcb_data_get_top(data);
	ctx.move.dx = dx;
	ctx.move.dy = dy;

	PCB_VIA_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_VIA, via, via, via);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_ELEMENT, element, element, element);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_SUBC, subc, subc, subc);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_LINE, layer, line, line);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_ARC, layer, arc, arc);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_TEXT, layer, text, text);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		pcb_object_operation(&MoveFunctions, &ctx, PCB_TYPE_POLY, layer, polygon, polygon);
	}
	PCB_ENDALL_LOOP;
}

void pcb_data_list_by_flag(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type, unsigned long mask)
{
	if (type & PCB_OBJ_VIA) PCB_VIA_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, via))) vtp0_append(dst, via);
	} PCB_END_LOOP;
	if (type & PCB_OBJ_PSTK) PCB_PADSTACK_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, padstack))) vtp0_append(dst, padstack);
	} PCB_END_LOOP;
	if (type & PCB_OBJ_ELEMENT) PCB_ELEMENT_LOOP(data); {
		if ((mask == 0) || (PCB_FLAG_TEST(mask, element))) vtp0_append(dst, element);
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
}

void pcb_data_list_terms(pcb_data_t *data, vtp0_t *dst, pcb_objtype_t type)
{
	if (type & PCB_OBJ_VIA) PCB_VIA_LOOP(data); {
		if (via->term != NULL) vtp0_append(dst, via);
	} PCB_END_LOOP;
#warning TODO: subc TODO
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
		pcb_r_dir_t tmp = pcb_r_search(tree, starting_region, region_in_search, rectangle_in_region, closure, num_found); \
		if (tmp == PCB_R_DIR_CANCEL) return tmp; \
		res |= tmp; \
	} while(0);

pcb_r_dir_t pcb_data_r_search(pcb_data_t *data, pcb_objtype_t types, const pcb_box_t *starting_region,
						 pcb_r_dir_t (*region_in_search) (const pcb_box_t *region, void *cl),
						 pcb_r_dir_t (*rectangle_in_region) (const pcb_box_t *box, void *cl),
						 void *closure, int *num_found, pcb_bool vis_only)
{
	pcb_layer_id_t lid;
	pcb_r_dir_t res = 0;

	if (!vis_only || PCB->ViaOn)
		if (types & PCB_OBJ_VIA)  rsearch(data->via_tree);

	if (!vis_only || PCB->RatOn)
		if (types & PCB_OBJ_RAT)  rsearch(data->rat_tree);

	if (!vis_only || PCB->PinOn) {
		if (types & PCB_OBJ_PIN)  rsearch(data->pin_tree);
		if (types & PCB_OBJ_PAD)  rsearch(data->pad_tree);
	}
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
	}

	/* safe to remove this block with elements */
	{
		int n;

		if (types & PCB_OBJ_ELEMENT) rsearch(data->element_tree);
		if (types & PCB_OBJ_ETEXT)
			for(n = 0; n < 3; n++)
				rsearch(data->name_tree[n]);
	}

	return res;
}



