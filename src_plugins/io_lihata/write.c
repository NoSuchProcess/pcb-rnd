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
#include <liblihata/tree.h>
#include "global.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "strflags.h"
#include "compat_misc.h"
#include "macro.h"

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

static lht_node_t *build_board_meta(PCBType *pcb)
{
	lht_node_t *meta;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(meta, build_text("board_name", pcb->Name));
	return meta;
}

static lht_node_t *build_attributes(AttributeListType *lst)
{
	int n;
	lht_node_t *ln;

	ln = lht_dom_node_alloc(LHT_HASH, "attributes");

	for (n = 0; n < lst->Number; n++)
		lht_dom_hash_put(ln, build_text(lst->List[n].name, lst->List[n].value));

	return ln;
}

/* Because all the macros expect it, that's why.  */
typedef struct {
	FlagType Flags;
} flag_holder;

const char *thermal_style[] = {
	NULL,
	"diagonal-sharp",
	"horver-sharp",
	"solid",
	"diagonal-round",
	"horver-round"
};

static lht_node_t *build_flags(FlagType *f, int object_type)
{
	int n, layer;
	lht_node_t *hsh, *txt, *lst;
	flag_holder fh;

	fh.Flags = *f;

	hsh = lht_dom_node_alloc(LHT_HASH, "flags");

	/* create normal flag nodes */
	for (n = 0; n < pcb_object_flagbits_len; n++) {
		if ((pcb_object_flagbits[n].object_types & object_type) && (TEST_FLAG(pcb_object_flagbits[n].mask, &fh))) {
			lht_dom_hash_put(hsh, build_text(pcb_object_flagbits[n].name, "1"));
			CLEAR_FLAG(pcb_object_flagbits[n].mask, &fh);
		}
	}

	/* thermal flags per layer */
	lst = lht_dom_node_alloc(LHT_HASH, "thermal");
	lht_dom_hash_put(hsh, lst);

	for(layer = 0; layer < max_copper_layer; layer++) {
		if (TEST_ANY_THERMS(&fh)) {
			int t = GET_THERM(layer, &fh);
			if (t != 0) {
				txt = lht_dom_node_alloc(LHT_TEXT, PCB->Data->Layer[layer].Name);
				if (t < ENTRIES(thermal_style))
					txt->data.text.value = pcb_strdup(thermal_style[t]);
				else
					txt->data.text.value = pcb_strdup_printf("%d", t);
				lht_dom_hash_put(lst, txt);
			}
		}
	}

	if (f->q > 0)
		lht_dom_hash_put(hsh, build_textf("shape", "%d", f->q));

	if (f->int_conn_grp > 0)
		lht_dom_hash_put(hsh, build_textf("intconn", "%d", f->int_conn_grp));

	return hsh;
}

static lht_node_t *build_line(LineType *line)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "line.%ld", line->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&line->Attributes));
	lht_dom_hash_put(obj, build_flags(&line->Flags, PCB_TYPE_LINE));
	lht_dom_hash_put(obj, build_textf("thickness", "%mr", line->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", "%mr", line->Clearance));
	lht_dom_hash_put(obj, build_textf("x1", "%mr", line->Point1.X));
	lht_dom_hash_put(obj, build_textf("y1", "%mr", line->Point1.Y));
	lht_dom_hash_put(obj, build_textf("x2", "%mr", line->Point2.X));
	lht_dom_hash_put(obj, build_textf("y2", "%mr", line->Point2.Y));

	return obj;
}

static lht_node_t *build_arc(ArcType *arc)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "arc.%ld", arc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&arc->Attributes));
	lht_dom_hash_put(obj, build_flags(&arc->Flags, PCB_TYPE_ARC));
	lht_dom_hash_put(obj, build_textf("thickness", "%mr", arc->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", "%mr", arc->Clearance));
	lht_dom_hash_put(obj, build_textf("x", "%mr", arc->X));
	lht_dom_hash_put(obj, build_textf("y", "%mr", arc->Y));
	lht_dom_hash_put(obj, build_textf("width", "%mr", arc->Width));
	lht_dom_hash_put(obj, build_textf("height", "%mr", arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%ma", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%ma", arc->Delta));

	return obj;
}

static lht_node_t *build_pin(PinType *pin, int is_via)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "%s.%ld", is_via ? "via" : "pin", pin->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&pin->Attributes));
	lht_dom_hash_put(obj, build_flags(&pin->Flags, PCB_TYPE_VIA));
	lht_dom_hash_put(obj, build_textf("thickness", "%mr", pin->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", "%mr", pin->Clearance));
	lht_dom_hash_put(obj, build_textf("mask", "%mr", pin->Mask));
	lht_dom_hash_put(obj, build_textf("hole", "%mr", pin->DrillingHole));
	lht_dom_hash_put(obj, build_textf("x", "%mr", pin->X));
	lht_dom_hash_put(obj, build_textf("y", "%mr", pin->Y));
	lht_dom_hash_put(obj, build_text("name", pin->Name));
	return obj;
}

static lht_node_t *build_polygon(PolygonType *poly)
{
	char buff[128];
	lht_node_t *obj, *tbl, *geo;
	Cardinal n, hole = 0;

	sprintf(buff, "polygon.%ld", poly->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&poly->Attributes));
	lht_dom_hash_put(obj, build_flags(&poly->Flags, PCB_TYPE_VIA));

	geo = lht_dom_node_alloc(LHT_LIST, "geometry");
	lht_dom_hash_put(obj, geo);

	tbl = lht_dom_node_alloc(LHT_TABLE, "contour");
	tbl->data.table.cols = 2;
	lht_dom_list_append(geo, tbl);

	for(n = 0; n < poly->PointN; n++) {
		int row;
		if ((hole < poly->HoleIndexN) && (n == poly->HoleIndex[hole])) {
			tbl = lht_dom_node_alloc(LHT_TABLE, "hole");
			tbl->data.table.cols = 2;
			lht_dom_list_append(geo, tbl);
			hole++;
		}

		row = tbl->data.table.rows;
		lht_tree_table_ins_row(tbl, row);
		tbl->data.table.r[row][0] = build_textf(NULL, "%mr", poly->Points[n].X);
		tbl->data.table.r[row][1] = build_textf(NULL, "%mr", poly->Points[n].Y);
	}

	return obj;
}


static lht_node_t *build_data_layer(DataType *data, LayerType *layer)
{
	lht_node_t *obj, *grp;
	LineType *li;
	ArcType *ar;
	PolygonType *po;

	obj = lht_dom_node_alloc(LHT_HASH, layer->Name);

	lht_dom_hash_put(obj, build_text("visible", layer->On ? "1" : "0"));
	lht_dom_hash_put(obj, build_attributes(&layer->Attributes));

	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(obj, grp);

	for(li = linelist_first(&layer->Line); li != NULL; li = linelist_next(li))
		lht_dom_list_append(grp, build_line(li));

	for(ar = arclist_first(&layer->Arc); ar != NULL; ar = arclist_next(ar))
		lht_dom_list_append(grp, build_arc(ar));

	for(po = polylist_first(&layer->Polygon); po != NULL; po = polylist_next(po))
		lht_dom_list_append(grp, build_polygon(po));


	return obj;
}

static lht_node_t *build_data_layers(DataType *data)
{
	int n;
	lht_node_t *layers;

	layers = lht_dom_node_alloc(LHT_LIST, "layers");

	for(n = 0; n < max_copper_layer + 2; n++)
		lht_dom_list_append(layers, build_data_layer(data, data->Layer+n));

	return layers;
}

static lht_node_t *build_data(DataType *data)
{
	lht_node_t *grp, *ndt;
	PinType *pi;

	ndt = lht_dom_node_alloc(LHT_HASH, "data");

	/* build layers */
	lht_dom_hash_put(ndt, build_data_layers(data));

	/* build a list of all global objects */
	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, grp);

	for(pi = pinlist_first(&data->Via); pi != NULL; pi = pinlist_next(pi))
		lht_dom_list_append(grp, build_pin(pi, 1));

	return ndt;
}

static lht_doc_t *build_board(PCBType *pcb)
{
	lht_doc_t *brd = lht_dom_init();

	brd->root = lht_dom_node_alloc(LHT_HASH, "pcb-rnd-board-v1");
	lht_dom_hash_put(brd->root, build_board_meta(pcb));
	lht_dom_hash_put(brd->root, build_data(pcb->Data));
	lht_dom_hash_put(brd->root, build_attributes(&pcb->Attributes));
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
