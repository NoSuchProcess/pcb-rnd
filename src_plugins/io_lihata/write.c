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

static void build_board_meta(PCBType *pcb, lht_doc_t *brd)
{
	lht_node_t *meta, *field;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(brd->root, meta);

	if (pcb->Name != NULL) {
		field = lht_dom_node_alloc(LHT_TEXT, "board_name");
		field->data.text.value = pcb_strdup(pcb->Name);
		lht_dom_hash_put(meta, field);
	}
}

static lht_doc_t *build_board(PCBType *pcb)
{
	lht_doc_t *brd = lht_dom_init();
	brd->root = lht_dom_node_alloc(LHT_HASH, "pcb-rnd-board-v1");
	build_board_meta(pcb, brd);
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
