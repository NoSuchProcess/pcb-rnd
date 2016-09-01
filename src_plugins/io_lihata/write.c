/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include "global.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "compat_misc.h"

static lht_node_t *build_text(const char *key, const char *value)
{
	lht_node_t *field;

	field = lht_dom_node_alloc(LHT_TEXT, key);
	if (value != NULL)
		field->data.text.value = pcb_strdup(value);
	return field;
}

static void build_board_meta(PCBType *pcb, lht_doc_t *brd)
{
	lht_node_t *meta, *f;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(brd->root, meta);

	lht_dom_hash_put(meta, build_text("board_name", pcb->Name));
}

static void build_data_layer(DataType *data, lht_node_t *parent, LayerType *layer)
{
	lht_node_t *meta;

	meta = lht_dom_node_alloc(LHT_HASH, layer->Name);
	lht_dom_list_append(parent, meta);
}

static void build_data_layers(DataType *data, lht_doc_t *brd)
{
	int n;
	lht_node_t *layers;

	layers = lht_dom_node_alloc(LHT_LIST, "layers");
	lht_dom_hash_put(brd->root, layers);

	for(n = 0; n < max_copper_layer + 2; n++)
		build_data_layer(data, layers, data->Layer+n);
}

static lht_doc_t *build_board(PCBType *pcb)
{
	lht_doc_t *brd = lht_dom_init();
	brd->root = lht_dom_node_alloc(LHT_HASH, "pcb-rnd-board-v1");
	build_board_meta(pcb, brd);
	build_data_layers(pcb->Data, brd);
	return brd;
}

int io_lihata_write_pcb(plug_io_t *ctx, FILE * FP)
{
	int res;
	lht_doc_t *brd = build_board(PCB);
	res = lht_dom_export(brd->root, FP, "");
	lht_dom_uninit(brd);
	return res;
}
