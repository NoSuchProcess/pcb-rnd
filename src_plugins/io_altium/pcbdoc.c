/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  altium pcbdoc ASCII plugin - high level read: convert tree to pcb-rnd
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include "board.h"

#include "pcbdoc_ascii.h"

#define LY_CACHE_MAX 64

typedef struct {
	altium_tree_t tree;
	pcb_board_t *pcb;
	const char *filename;

	/* caches */
	pcb_layer_t *lych[LY_CACHE_MAX];
} rctx_t;

static rnd_coord_t conv_coord_field(altium_field_t *field)
{
	double res;
	rnd_bool succ;

	res = rnd_get_value(field->val, NULL, NULL, &succ);
	if (!succ) {
		rnd_message(RND_MSG_ERROR, "failed to convert coord value '%s'\n", field->val);
		return 0;
	}
	return res;
}

static rnd_coord_t conv_coordx_field(rctx_t *rctx, altium_field_t *field)
{
	return conv_coord_field(field);
}

static rnd_coord_t conv_coordy_field(rctx_t *rctx, altium_field_t *field)
{
	return rctx->pcb->hidlib.size_y - conv_coord_field(field);
}


static pcb_layer_t *conv_layer_(rctx_t *rctx, int cache_idx, pcb_layer_type_t lyt, const char *purpose)
{
	rnd_layer_id_t lid;
	pcb_layer_t *ly;

	if ((cache_idx >= 0) && (cache_idx < LY_CACHE_MAX) && (rctx->lych[cache_idx] != NULL))
		return rctx->lych[cache_idx];
	
	if (pcb_layer_listp(rctx->pcb, lyt, &lid, 1, -1, purpose) != 1)
		return NULL;

	ly = pcb_get_layer(rctx->pcb->Data, lid);
	if (ly == NULL)
		return NULL;

	if ((cache_idx >= 0) && (cache_idx < LY_CACHE_MAX))
		rctx->lych[cache_idx] = ly;

	return ly;
}

static pcb_layer_t *conv_layer_field(rctx_t *rctx, altium_field_t *field)
{
	int kw = altium_kw_sphash(field->val);

	switch(kw) {
		case altium_kw_layers_top:           return conv_layer_(rctx, 0,  PCB_LYT_COPPER | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottom:        return conv_layer_(rctx, 1,  PCB_LYT_COPPER | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_topoverlay:    return conv_layer_(rctx, 2,  PCB_LYT_SILK | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottomoverlay: return conv_layer_(rctx, 3,  PCB_LYT_SILK | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_toppaste:      return conv_layer_(rctx, 4,  PCB_LYT_PASTE | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottompaste:   return conv_layer_(rctx, 5,  PCB_LYT_PASTE | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_topsolder:     return conv_layer_(rctx, 6,  PCB_LYT_MASK | PCB_LYT_TOP, NULL);
		case altium_kw_layers_bottomsolder:  return conv_layer_(rctx, 7,  PCB_LYT_MASK | PCB_LYT_BOTTOM, NULL);
		case altium_kw_layers_drillguide:    return conv_layer_(rctx, 8,  PCB_LYT_DOC, "drill_guide");
		case altium_kw_layers_keepout:       return conv_layer_(rctx, 9,  PCB_LYT_DOC, "keepout");
		case altium_kw_layers_drilldrawing:  return conv_layer_(rctx, 10, PCB_LYT_DOC, "drill");
		case altium_kw_layers_multilayer:    return conv_layer_(rctx, 11, PCB_LYT_DOC, "multilayer");
	}
TODO("MID1...MID16: look up or create new intern copper; use cache index from 15+mid");
TODO("PLANE1...PLANE16: look up or create new intern copper; use cache index from 15+16+plane");
TODO("MECHANICAL1...MECHANICAL16: look up or create new doc?; use cache index from 15+16+16+mechanical");
	rnd_message(RND_MSG_ERROR, "Layer not found: '%s'\n", field->val);
	return NULL;
}

static int altium_parse_board(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_board]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_board], rec)) {
		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_sheetheight: rctx->pcb->hidlib.size_x = conv_coord_field(field); break;
				case altium_kw_field_sheetwidth:  rctx->pcb->hidlib.size_y = conv_coord_field(field); break;
					break;
				default: break;
			}
		}
	}

	return 0;
}

static int altium_parse_track(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_track]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_track], rec)) {
		pcb_layer_t *ly = NULL;
		rnd_coord_t x1 = RND_COORD_MAX, y1 = RND_COORD_MAX, x2 = RND_COORD_MAX, y2 = RND_COORD_MAX, w = RND_COORD_MAX;
		rnd_coord_t cl = 0;
		TODO("figure clearance for cl");

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_layer: ly = conv_layer_field(rctx, field); break;
				case altium_kw_field_x1:    x1 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y1:    y1 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_x2:    x2 = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y2:    y2 = conv_coordy_field(rctx, field); break;
				case altium_kw_field_width: w = conv_coord_field(field); break;
				default: break;
			}
		}
		if ((x1 == RND_COORD_MAX) || (y1 == RND_COORD_MAX) || (x2 == RND_COORD_MAX) || (y2 == RND_COORD_MAX) || (w == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid track object: missing coordinate or width (line not created)\n");
			continue;
		}
		if (ly == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid track object: no valid layer (line not created)\n");
			continue;
		}
		pcb_line_new(ly, x1, y1, x2, y2, w, cl, pcb_flag_make(PCB_FLAG_CLEARLINE));
	}

	return 0;
}

static int altium_parse_via(rctx_t *rctx)
{
	altium_record_t *rec;
	altium_field_t *field;

	for(rec = gdl_first(&rctx->tree.rec[altium_kw_record_via]); rec != NULL; rec = gdl_next(&rctx->tree.rec[altium_kw_record_via], rec)) {
		pcb_layer_t *ly = NULL;
		rnd_coord_t x = RND_COORD_MAX, y = RND_COORD_MAX, dia = RND_COORD_MAX, hole = RND_COORD_MAX;
		rnd_coord_t cl = 0;
		TODO("figure clearance for cl");

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_x:        x = conv_coordx_field(rctx, field); break;
				case altium_kw_field_y:        y = conv_coordy_field(rctx, field); break;
				case altium_kw_field_diameter: dia = conv_coordx_field(rctx, field); break;
				case altium_kw_field_holesize: hole = conv_coordy_field(rctx, field); break;
TODO("TENTINGTOP and TENTINGBOTTOM");
TODO("STARTLAYER and ENDLAYER (for bbvias)");
				default: break;
			}
		}
		if ((x == RND_COORD_MAX) || (y == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid via object: missing coordinate (via not created)\n");
			continue;
		}
		if ((dia == RND_COORD_MAX) || (hole == RND_COORD_MAX)) {
			rnd_message(RND_MSG_ERROR, "Invalid via object: missing geometry (via not created)\n");
			continue;
		}
		TODO("create poly here");
	}

	return 0;
}

int io_altium_parse_pcbdoc_ascii(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	rctx_t rctx = {0};
	int res = 0;

	rctx.pcb = pcb;
	rctx.filename = filename;

	if (pcbdoc_ascii_parse_file(&pcb->hidlib, &rctx.tree, filename) != 0) {
		altium_tree_free(&rctx.tree);
		rnd_message(RND_MSG_ERROR, "failed to parse '%s'\n", filename);
		return -1;
	}

	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap);
	pcb_layergrp_upgrade_by_map(pcb, pcb_dflgmap_doc);

	res |= altium_parse_board(&rctx);
	res |= altium_parse_track(&rctx);
	res |= altium_parse_via(&rctx);

	altium_tree_free(&rctx.tree);
	return res;
}

