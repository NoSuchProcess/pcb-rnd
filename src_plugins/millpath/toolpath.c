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

#include "config.h"

#include "toolpath.h"

#include "board.h"
#include "data.h"
#include "flag.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_ui.h"
#include "obj_line.h"
#include "obj_arc.h"
#include "obj_poly.h"
#include "polygon.h"

#include "src_plugins/lib_polyhelp/polyhelp.h"

extern const char *pcb_millpath_cookie;

static void sub_layer_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer, int centerline)
{
	pcb_line_t *line, line_tmp;
	pcb_arc_t *arc, arc_tmp;
	pcb_text_t *text;
	pcb_rtree_it_t it;

	for(line = (pcb_line_t *)pcb_r_first(layer->line_tree, &it); line != NULL; line = (pcb_line_t *)pcb_r_next(&it)) {
		memcpy(&line_tmp, line, sizeof(line_tmp));
		if (centerline) {
			line_tmp.Thickness = 1;
			line_tmp.Clearance = result->edge_clearance;
		}
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_LINE, &line_tmp);
	}
	pcb_r_end(&it);

	for(arc = (pcb_arc_t *)pcb_r_first(layer->arc_tree, &it); arc != NULL; arc = (pcb_arc_t *)pcb_r_next(&it)) {
		memcpy(&arc_tmp, arc, sizeof(arc_tmp));
		if (centerline) {
			arc_tmp.Thickness = 1;
			arc_tmp.Clearance = result->edge_clearance;
		}
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_ARC, &arc_tmp);
	}
	pcb_r_end(&it);

#warning TODO: centerline
	for(text = (pcb_text_t *)pcb_r_first(layer->text_tree, &it); text != NULL; text = (pcb_text_t *)pcb_r_next(&it))
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_ARC, text);
	pcb_r_end(&it);

#warning TODO: subs poly: not supported by core
}


static void sub_group_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp, int centerline)
{
	int n;
	for(n = 0; n < grp->len; n++) {
		pcb_layer_t *l = pcb_get_layer(grp->lid[n]);
		if (l != NULL)
			sub_layer_all(pcb, result, l, centerline);
	}
}

static void sub_global_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	pcb_pin_t *pin;
	pcb_pad_t *pad;
	pcb_rtree_it_t it;

#warning TODO: thermals: find out if any of our layers has thermal for the pin and if so, use that layer
	for(pin = (pcb_pin_t *)pcb_r_first(pcb->Data->via_tree, &it); pin != NULL; pin = (pcb_pin_t *)pcb_r_next(&it)) {
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_VIA, pin);
	}
	pcb_r_end(&it);

	for(pin = (pcb_pin_t *)pcb_r_first(pcb->Data->pin_tree, &it); pin != NULL; pin = (pcb_pin_t *)pcb_r_next(&it)) {
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_PIN, pin);
	}
	pcb_r_end(&it);

	for(pad = (pcb_pad_t *)pcb_r_first(pcb->Data->pad_tree, &it); pad != NULL; pad = (pcb_pad_t *)pcb_r_next(&it)) {
#warning TODO: only on the side we are on
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_TYPE_PAD, pad);
	}
	pcb_r_end(&it);
}

static void setup_ui_layers(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	if (result->res_ply == NULL)
		result->res_ply = pcb_uilayer_alloc(pcb_millpath_cookie, "mill remove", "#EE9922");

	if (result->res_path == NULL)
		result->res_path = pcb_uilayer_alloc(pcb_millpath_cookie, "mill toolpath", "#886611");

	if (result->fill != NULL)
		pcb_poly_remove(result->res_ply, result->fill);
}

static void setup_remove_poly(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	pcb_layergrp_id_t otl;
	int has_otl;

	result->grp = pcb_get_layergrp(pcb, layer->grp);
	has_otl = (pcb_layergrp_list(pcb, PCB_LYT_OUTLINE, &otl, 1) == 1);

	if (has_otl) { /* if there's an outline layer, the remove-poly shouldn't be bigger than that */
		pcb_line_t *line;
		pcb_arc_t *arc;
		pcb_rtree_it_t it;
		pcb_box_t otlbb;
		pcb_layergrp_t *og = pcb_get_layergrp(pcb, otl);
		int n;
		
		otlbb.X1 = otlbb.Y1 = PCB_MAX_COORD;
		otlbb.X2 = otlbb.Y2 = -PCB_MAX_COORD;

		for(n = 0; n < og->len; n++) {
			pcb_layer_t *l = pcb_get_layer(og->lid[n]);
			if (l == NULL)
				continue;

			for(line = (pcb_line_t *)pcb_r_first(l->line_tree, &it); line != NULL; line = (pcb_line_t *)pcb_r_next(&it))
				pcb_box_bump_box(&otlbb, (pcb_box_t *)line);
			pcb_r_end(&it);

			for(arc = (pcb_arc_t *)pcb_r_first(l->arc_tree, &it); arc != NULL; arc = (pcb_arc_t *)pcb_r_next(&it))
				pcb_box_bump_box(&otlbb, (pcb_box_t *)arc);
			pcb_r_end(&it);
		}
		result->fill = pcb_poly_new_from_rectangle(result->res_ply, otlbb.X1, otlbb.Y1, otlbb.X2, otlbb.Y2, pcb_flag_make(PCB_FLAG_FULLPOLY));
	}
	else
		result->fill = pcb_poly_new_from_rectangle(result->res_ply, 0, 0, pcb->MaxWidth, pcb->MaxHeight, pcb_flag_make(PCB_FLAG_FULLPOLY));

	pcb_poly_init_clip(pcb->Data, result->res_ply, result->fill);

	sub_group_all(pcb, result, result->grp, 0);
	if (has_otl)
		sub_group_all(pcb, result, pcb_get_layergrp(pcb, otl), 1);

	sub_global_all(pcb, result, layer);
}

int pcb_tlp_mill_copper_layer(pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	pcb_board_t *pcb = pcb_data_get_top(layer->parent);

	setup_ui_layers(pcb, result, layer);
	setup_remove_poly(pcb, result, layer);

	return 0;
}

