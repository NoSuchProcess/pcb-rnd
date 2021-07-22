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

typedef struct {
	altium_tree_t tree;
	pcb_board_t *pcb;
	const char *filename;
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

		for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
			switch(field->type) {
				case altium_kw_field_layer: break;
				case altium_kw_field_x1:    x1 = conv_coord_field(field); break;
				case altium_kw_field_y1:    y1 = conv_coord_field(field); break;
				case altium_kw_field_x2:    x2 = conv_coord_field(field); break;
				case altium_kw_field_y2:    y2 = conv_coord_field(field); break;
				case altium_kw_field_width: w = conv_coord_field(field); break;
					break;
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
		TODO("create the line here");
	}

	return 0;
}

int io_altium_parse_pcbdoc_ascii(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	FILE *f;
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

printf("parse done\n");

	altium_tree_free(&rctx.tree);
	return res;
}

