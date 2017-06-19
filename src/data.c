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

/* just defines common identifiers
 */
#include "config.h"

#include "board.h"
#include "data.h"
#include "rtree.h"
#include "list_common.h"
#include "obj_all.h"
#include "layer_it.h"

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */
int pcb_layer_stack[PCB_MAX_LAYER];			/* determines the layer draw order */

pcb_buffer_t pcb_buffers[PCB_MAX_BUFFER]; /* my buffers */
pcb_bool pcb_bumped;                /* if the undo serial number has changed */

int pcb_added_lines;


/* callback based loops */
void pcb_loop_layers(void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb)
{
	if ((lacb != NULL) || (lcb != NULL) || (acb != NULL) || (tcb != NULL) || (pocb != NULL)) {
		pcb_layer_it_t it;
		pcb_layer_id_t lid;
		for(lid = pcb_layer_first_all(&PCB->LayerGroups, &it); lid != -1; lid = pcb_layer_next(&it)) {
			pcb_layer_t *layer = PCB->Data->Layer + lid;
			if (lacb != NULL)
				if (lacb(ctx, PCB, layer, 1))
					continue;
			if (lcb != NULL) {
				PCB_LINE_LOOP(layer);
				{
					lcb(ctx, PCB, layer, line);
				}
				PCB_END_LOOP;
			}

			if (acb != NULL) {
				PCB_ARC_LOOP(layer);
				{
					acb(ctx, PCB, layer, arc);
				}
				PCB_END_LOOP;
			}

			if (tcb != NULL) {
				PCB_TEXT_LOOP(layer);
				{
					tcb(ctx, PCB, layer, text);
				}
				PCB_END_LOOP;
			}

			if (pocb != NULL) {
				PCB_POLY_LOOP(layer);
				{
					pocb(ctx, PCB, layer, polygon);
				}
				PCB_END_LOOP;
			}
			if (lacb != NULL)
				lacb(ctx, PCB, layer, 0);
		}
	}
}

void pcb_loop_elements(void *ctx, pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb)
{
	if ((ecb != NULL) || (elcb != NULL) || (eacb != NULL)  || (etcb != NULL) || (epicb != NULL) || (epacb != NULL)) {
		PCB_ELEMENT_LOOP(PCB->Data);
		{
			if (ecb != NULL)
				if (ecb(ctx, PCB, element, 1))
					continue;

			if (elcb != NULL) {
				PCB_ELEMENT_PCB_LINE_LOOP(element);
				{
					elcb(ctx, PCB, element, line);
				}
				PCB_END_LOOP;
			}

			if (eacb != NULL) {
				PCB_ELEMENT_ARC_LOOP(element);
				{
					eacb(ctx, PCB, element, arc);
				}
				PCB_END_LOOP;
			}

			if (etcb != NULL) {
				PCB_ELEMENT_PCB_TEXT_LOOP(element);
				{
					etcb(ctx, PCB, element, text);
				}
				PCB_END_LOOP;
			}

			if (epicb != NULL) {
				PCB_PIN_LOOP(element);
				{
					epicb(ctx, PCB, element, pin);
				}
				PCB_END_LOOP;
			}


			if (epacb != NULL) {
				PCB_PAD_LOOP(element);
				{
					epacb(ctx, PCB, element, pad);
				}
				PCB_END_LOOP;
			}

			if (ecb != NULL)
				ecb(ctx, PCB, element, 0);
		}
		PCB_END_LOOP;
	}
}

void pcb_loop_vias(void *ctx, pcb_via_cb_t vcb)
{
	if (vcb != NULL) {
		PCB_VIA_LOOP(PCB->Data);
		{
			vcb(ctx, PCB, via);
		}
		PCB_END_LOOP;
	}
}

void pcb_loop_all(void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb,
	pcb_via_cb_t vcb
	)
{
	pcb_loop_layers(ctx, lacb, lcb, acb, tcb, pocb);
	pcb_loop_elements(ctx, ecb, elcb, eacb, etcb, epicb, epacb);
	pcb_loop_vias(ctx, vcb);
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
		if (PCB_LAYER_IS_REAL(layer))
			pcb_attribute_free(&layer->meta.real.Attributes);
		PCB_TEXT_LOOP(layer);
		{
			free(text->TextString);
		}
		PCB_END_LOOP;
		if (PCB_LAYER_IS_REAL(layer))
			free((char*)layer->meta.real.name);
		else
			free((char*)layer->meta.bound.name);
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
		list_map0(&layer->Polygon, pcb_polygon_t, pcb_poly_free);
		if (PCB_LAYER_IS_REAL(layer)) {
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
		if (data->rat_tree)
			pcb_r_destroy_tree(&data->rat_tree);
	}

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

	hasNoObjects = (pinlist_length(&Data->Via) == 0);
	hasNoObjects &= (elementlist_length(&Data->Element) == 0);
	for (i = 0; i < Data->LayerN; i++)
		hasNoObjects = hasNoObjects && pcb_layer_is_empty_(PCB, &(Data->Layer[i]));
	return (hasNoObjects);
}

pcb_box_t *pcb_data_bbox(pcb_box_t *out, pcb_data_t *Data)
{
	/* FIX ME: use r_search to do this much faster */

	/* preset identifiers with highest and lowest possible values */
	out->X1 = out->Y1 = PCB_MAX_COORD;
	out->X2 = out->Y2 = -PCB_MAX_COORD;

	/* now scan for the lowest/highest X and Y coordinate */
	PCB_VIA_LOOP(Data);
	{
		out->X1 = MIN(out->X1, via->X - via->Thickness / 2);
		out->Y1 = MIN(out->Y1, via->Y - via->Thickness / 2);
		out->X2 = MAX(out->X2, via->X + via->Thickness / 2);
		out->Y2 = MAX(out->Y2, via->Y + via->Thickness / 2);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(Data);
	{
		out->X1 = MIN(out->X1, element->BoundingBox.X1);
		out->Y1 = MIN(out->Y1, element->BoundingBox.Y1);
		out->X2 = MAX(out->X2, element->BoundingBox.X2);
		out->Y2 = MAX(out->Y2, element->BoundingBox.Y2);
		{
			pcb_text_t *text = &PCB_ELEM_TEXT_REFDES(element);
			out->X1 = MIN(out->X1, text->BoundingBox.X1);
			out->Y1 = MIN(out->Y1, text->BoundingBox.Y1);
			out->X2 = MAX(out->X2, text->BoundingBox.X2);
			out->Y2 = MAX(out->Y2, text->BoundingBox.Y2);
		};
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(Data);
	{
		out->X1 = MIN(out->X1, subc->BoundingBox.X1);
		out->Y1 = MIN(out->Y1, subc->BoundingBox.Y1);
		out->X2 = MAX(out->X2, subc->BoundingBox.X2);
		out->Y2 = MAX(out->Y2, subc->BoundingBox.Y2);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(Data);
	{
		out->X1 = MIN(out->X1, line->Point1.X - line->Thickness / 2);
		out->Y1 = MIN(out->Y1, line->Point1.Y - line->Thickness / 2);
		out->X1 = MIN(out->X1, line->Point2.X - line->Thickness / 2);
		out->Y1 = MIN(out->Y1, line->Point2.Y - line->Thickness / 2);
		out->X2 = MAX(out->X2, line->Point1.X + line->Thickness / 2);
		out->Y2 = MAX(out->Y2, line->Point1.Y + line->Thickness / 2);
		out->X2 = MAX(out->X2, line->Point2.X + line->Thickness / 2);
		out->Y2 = MAX(out->Y2, line->Point2.Y + line->Thickness / 2);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Data);
	{
		out->X1 = MIN(out->X1, arc->BoundingBox.X1);
		out->Y1 = MIN(out->Y1, arc->BoundingBox.Y1);
		out->X2 = MAX(out->X2, arc->BoundingBox.X2);
		out->Y2 = MAX(out->Y2, arc->BoundingBox.Y2);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Data);
	{
		out->X1 = MIN(out->X1, text->BoundingBox.X1);
		out->Y1 = MIN(out->Y1, text->BoundingBox.Y1);
		out->X2 = MAX(out->X2, text->BoundingBox.X2);
		out->Y2 = MAX(out->Y2, text->BoundingBox.Y2);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Data);
	{
		out->X1 = MIN(out->X1, polygon->BoundingBox.X1);
		out->Y1 = MIN(out->Y1, polygon->BoundingBox.Y1);
		out->X2 = MAX(out->X2, polygon->BoundingBox.X2);
		out->Y2 = MAX(out->Y2, polygon->BoundingBox.Y2);
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
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_real2bound(&data->Layer[n], &pcb->Data->Layer[n], share_rtrees);
		data->Layer[n].parent = data;
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
