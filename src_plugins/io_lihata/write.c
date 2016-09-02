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
#include <stdarg.h>
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

static lht_node_t *build_textf(const char *key, const char *fmt, ...)
{
	lht_node_t *field;
	va_list ap;

	field = lht_dom_node_alloc(LHT_TEXT, key);
	va_start(ap, fmt);
	field->data.text.value = pcb_strdup_vprintf(fmt, ap);
	va_end(ap);
	return field;
}

static void build_board_meta(PCBType *pcb, lht_doc_t *brd)
{
	lht_node_t *meta;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(brd->root, meta);

	lht_dom_hash_put(meta, build_text("board_name", pcb->Name));
}

static void build_attributes(lht_node_t *parent, AttributeListType *lst)
{
	int n;
	lht_node_t *ln;

	ln = lht_dom_node_alloc(LHT_HASH, "attributes");
	lht_dom_hash_put(parent, ln);

	for (n = 0; n < lst->Number; n++)
		lht_dom_hash_put(ln, build_text(lst->List[n].name, lst->List[n].value));
}

static void build_line(lht_node_t *parent, LineType *line)
{
	char buff[128];
	lht_node_t *ln;

	sprintf(buff, "line.%d", line->ID);
	ln = lht_dom_node_alloc(LHT_HASH, buff);
	lht_dom_hash_put(parent, ln);

#warning TODO: Flags
	build_attributes(ln, &line->Attributes);
	lht_dom_hash_put(ln, build_textf("thickness", "%mr", line->Thickness));
	lht_dom_hash_put(ln, build_textf("clearance", "%mr", line->Clearance));
	lht_dom_hash_put(ln, build_textf("x1", "%mr", line->Point1.X));
	lht_dom_hash_put(ln, build_textf("y1", "%mr", line->Point1.Y));
	lht_dom_hash_put(ln, build_textf("x2", "%mr", line->Point2.X));
	lht_dom_hash_put(ln, build_textf("y2", "%mr", line->Point2.Y));
}

static void build_arc(lht_node_t *parent, ArcType *arc)
{
	char buff[128];
	lht_node_t *ln;

	sprintf(buff, "arc.%d", arc->ID);
	ln = lht_dom_node_alloc(LHT_HASH, buff);
	lht_dom_hash_put(parent, ln);

#warning TODO: Flags
	build_attributes(ln, &arc->Attributes);
	lht_dom_hash_put(ln, build_textf("thickness", "%mr", arc->Thickness));
	lht_dom_hash_put(ln, build_textf("clearance", "%mr", arc->Clearance));
	lht_dom_hash_put(ln, build_textf("x", "%mr", arc->X));
	lht_dom_hash_put(ln, build_textf("y", "%mr", arc->Y));
	lht_dom_hash_put(ln, build_textf("width", "%mr", arc->Width));
	lht_dom_hash_put(ln, build_textf("height", "%mr", arc->Height));
	lht_dom_hash_put(ln, build_textf("astart", "%ma", arc->StartAngle));
	lht_dom_hash_put(ln, build_textf("adelta", "%ma", arc->Delta));
}

static void build_data_layer(DataType *data, lht_node_t *parent, LayerType *layer)
{
	lht_node_t *ln, *grp;
	LineType *li;
	ArcType *ar;
	int n;

	ln = lht_dom_node_alloc(LHT_HASH, layer->Name);
	lht_dom_list_append(parent, ln);

	lht_dom_hash_put(ln, build_text("visible", layer->On ? "1" : "0"));
	build_attributes(ln, &layer->Attributes);

	grp = lht_dom_node_alloc(LHT_HASH, "objects");
	lht_dom_hash_put(ln, grp);

	for(li = linelist_first(&layer->Line); li != NULL; li = linelist_next(li))
		build_line(grp, li);

	for(ar = arclist_first(&layer->Arc); ar != NULL; ar = arclist_next(li))
		build_arc(grp, ar);
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
