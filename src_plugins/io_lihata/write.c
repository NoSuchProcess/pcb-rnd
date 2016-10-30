/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

/* Build an in-memory lihata document that represents the board then save it.
   A document is built for the merge-save. */

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
#include "layer.h"
#include "common.h"
#include "write_style.h"

/*#define CFMT "%[9]"*/
/*#define CFMT "%.08mH"*/
#define CFMT "%$$mn"

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
	lht_node_t *meta, *grp;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(meta, build_text("board_name", pcb->Name));

	grp = lht_dom_node_alloc(LHT_HASH, "grid");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("offs_x", CFMT, pcb->GridOffsetX));
	lht_dom_hash_put(grp, build_textf("offs_y", CFMT, pcb->GridOffsetY));
	lht_dom_hash_put(grp, build_textf("spacing", CFMT, pcb->Grid));

	grp = lht_dom_node_alloc(LHT_HASH, "size");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("x", CFMT, pcb->MaxWidth));
	lht_dom_hash_put(grp, build_textf("y", CFMT, pcb->MaxHeight));

	grp = lht_dom_node_alloc(LHT_HASH, "drc");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("bloat",     CFMT, pcb->Bloat));
	lht_dom_hash_put(grp, build_textf("shrink",    CFMT, pcb->Shrink));
	lht_dom_hash_put(grp, build_textf("min_width", CFMT, pcb->minWid));
	lht_dom_hash_put(grp, build_textf("min_silk",  CFMT, pcb->minSlk));
	lht_dom_hash_put(grp, build_textf("min_drill", CFMT, pcb->minDrill));
	lht_dom_hash_put(grp, build_textf("min_ring",  CFMT, pcb->minRing));

	grp = lht_dom_node_alloc(LHT_HASH, "cursor");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("x", CFMT, pcb->CursorX));
	lht_dom_hash_put(grp, build_textf("y", CFMT, pcb->CursorY));

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

static lht_node_t *build_flags(FlagType *f, int object_type)
{
	int n, layer;
	lht_node_t *hsh, *txt, *lst;
	io_lihata_flag_holder fh;

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
				const char *name;
				txt = lht_dom_node_alloc(LHT_TEXT, PCB->Data->Layer[layer].Name);
				name = io_lihata_thermal_style(t);
				if (name != NULL)
					txt->data.text.value = pcb_strdup(name);
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

static lht_node_t *build_line(LineType *line, int local_id, Coord dx, Coord dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "line.%ld", (local_id >= 0 ? local_id : line->ID));
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&line->Attributes));
	lht_dom_hash_put(obj, build_flags(&line->Flags, PCB_TYPE_LINE));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, line->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, line->Clearance));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, line->Point1.X+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, line->Point1.Y+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, line->Point2.X+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, line->Point2.Y+dy));

	return obj;
}

static lht_node_t *build_arc(ArcType *arc, Coord dx, Coord dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "arc.%ld", arc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&arc->Attributes));
	lht_dom_hash_put(obj, build_flags(&arc->Flags, PCB_TYPE_ARC));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, arc->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, arc->Clearance));
	lht_dom_hash_put(obj, build_textf("x", CFMT, arc->X+dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, arc->Y+dy));
	lht_dom_hash_put(obj, build_textf("width", CFMT, arc->Width));
	lht_dom_hash_put(obj, build_textf("height", CFMT, arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%ma", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%ma", arc->Delta));

	return obj;
}

static lht_node_t *build_pin(PinType *pin, int is_via, Coord dx, Coord dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "%s.%ld", is_via ? "via" : "pin", pin->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&pin->Attributes));
	lht_dom_hash_put(obj, build_flags(&pin->Flags, PCB_TYPE_VIA));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, pin->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, pin->Clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, pin->Mask));
	lht_dom_hash_put(obj, build_textf("hole", CFMT, pin->DrillingHole));
	lht_dom_hash_put(obj, build_textf("x", CFMT, pin->X+dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, pin->Y+dy));
	lht_dom_hash_put(obj, build_text("name", pin->Name));
	lht_dom_hash_put(obj, build_text("number", pin->Number));
	return obj;
}

static lht_node_t *build_pad(PadType *pad, Coord dx, Coord dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "pad.%ld", pad->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&pad->Attributes));
	lht_dom_hash_put(obj, build_flags(&pad->Flags, PCB_TYPE_PAD));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, pad->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, pad->Clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, pad->Mask));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, pad->Point1.X+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, pad->Point1.Y+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, pad->Point2.X+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, pad->Point2.Y+dy));
	lht_dom_hash_put(obj, build_text("name", pad->Name));
	lht_dom_hash_put(obj, build_text("number", pad->Number));
	return obj;
}

static lht_node_t *build_polygon(PolygonType *poly)
{
	char buff[128];
	lht_node_t *obj, *tbl, *geo;
	pcb_cardinal_t n, hole = 0;

	sprintf(buff, "polygon.%ld", poly->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&poly->Attributes));
	lht_dom_hash_put(obj, build_flags(&poly->Flags, PCB_TYPE_POLYGON));

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
		tbl->data.table.r[row][0] = build_textf(NULL, CFMT, poly->Points[n].X);
		tbl->data.table.r[row][1] = build_textf(NULL, CFMT, poly->Points[n].Y);
	}

	return obj;
}

static lht_node_t *build_pcb_text(const char *role, TextType *text)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "text.%ld", text->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&text->Attributes));
	lht_dom_hash_put(obj, build_text("string", text->TextString));
	lht_dom_hash_put(obj, build_textf("scale", "%d", text->Scale));
	lht_dom_hash_put(obj, build_textf("direction", "%d", text->Direction));
	lht_dom_hash_put(obj, build_textf("x", CFMT, text->X));
	lht_dom_hash_put(obj, build_textf("y", CFMT, text->Y));

	if (role != NULL)
		lht_dom_hash_put(obj, build_text("role", role));

	return obj;
}

static lht_node_t *build_element(ElementType *elem)
{
	char buff[128];
	LineType *li;
	ArcType *ar;
	PinType *pi;
	PadType *pa;
	lht_node_t *obj, *lst;

	sprintf(buff, "element.%ld", elem->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&elem->Attributes));
	lht_dom_hash_put(obj, build_flags(&elem->Flags, PCB_TYPE_ELEMENT));


	/* build drawing primitives */
	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(obj, lst);

	lht_dom_list_append(lst, build_pcb_text("desc", &elem->Name[DESCRIPTION_INDEX]));
	lht_dom_list_append(lst, build_pcb_text("name", &elem->Name[NAMEONPCB_INDEX]));
	lht_dom_list_append(lst, build_pcb_text("value", &elem->Name[VALUE_INDEX]));
	lht_dom_hash_put(obj, build_textf("x", CFMT, elem->MarkX));
	lht_dom_hash_put(obj, build_textf("y", CFMT, elem->MarkY));


	for(li = linelist_first(&elem->Line); li != NULL; li = linelist_next(li))
		lht_dom_list_append(lst, build_line(li, -1, -elem->MarkX, -elem->MarkY));

	for(ar = arclist_first(&elem->Arc); ar != NULL; ar = arclist_next(ar))
		lht_dom_list_append(lst, build_arc(ar, -elem->MarkX, -elem->MarkY));

	for(pi = pinlist_first(&elem->Pin); pi != NULL; pi = pinlist_next(pi))
		lht_dom_list_append(lst, build_pin(pi, 0, -elem->MarkX, -elem->MarkY));

	for(pa = padlist_first(&elem->Pad); pa != NULL; pa = padlist_next(pa))
		lht_dom_list_append(lst, build_pad(pa, -elem->MarkX, -elem->MarkY));

	return obj;
}


static lht_node_t *build_data_layer(DataType *data, LayerType *layer, int layer_group)
{
	lht_node_t *obj, *grp;
	LineType *li;
	ArcType *ar;
	PolygonType *po;
	TextType *tx;
	char tmp[16];

	obj = lht_dom_node_alloc(LHT_HASH, layer->Name);

	lht_dom_hash_put(obj, build_text("visible", layer->On ? "1" : "0"));
	lht_dom_hash_put(obj, build_attributes(&layer->Attributes));
	sprintf(tmp, "%d", layer_group);
	lht_dom_hash_put(obj, build_text("group", tmp));

	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(obj, grp);

	for(li = linelist_first(&layer->Line); li != NULL; li = linelist_next(li))
		lht_dom_list_append(grp, build_line(li, -1, 0, 0));

	for(ar = arclist_first(&layer->Arc); ar != NULL; ar = arclist_next(ar))
		lht_dom_list_append(grp, build_arc(ar, 0, 0));

	for(po = polylist_first(&layer->Polygon); po != NULL; po = polylist_next(po))
		lht_dom_list_append(grp, build_polygon(po));

	for(tx = textlist_first(&layer->Text); tx != NULL; tx = textlist_next(tx))
		lht_dom_list_append(grp, build_pcb_text(NULL, tx));


	return obj;
}

static lht_node_t *build_data_layers(DataType *data)
{
	int n;
	lht_node_t *layers;

	layers = lht_dom_node_alloc(LHT_LIST, "layers");

	for(n = 0; n < max_copper_layer + 2; n++)
		lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, pcb_layer_lookup_group(n)));

	return layers;
}


static lht_node_t *build_data(DataType *data)
{
	lht_node_t *grp, *ndt;
	PinType *pi;
	ElementType *el;

	ndt = lht_dom_node_alloc(LHT_HASH, "data");

	/* build layers */
	lht_dom_hash_put(ndt, build_data_layers(data));

	/* build a list of all global objects */
	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, grp);

	for(pi = pinlist_first(&data->Via); pi != NULL; pi = pinlist_next(pi))
		lht_dom_list_append(grp, build_pin(pi, 1, 0, 0));

	for(el = elementlist_first(&data->Element); el != NULL; el = elementlist_next(el))
		lht_dom_list_append(grp, build_element(el));

	return ndt;
}

static lht_node_t *build_symbol(SymbolType *sym, const char *name)
{
	lht_node_t *lst, *ndt;
	LineType *li;
	int n;

	ndt = lht_dom_node_alloc(LHT_HASH, name);
	lht_dom_hash_put(ndt, build_textf("width", CFMT, sym->Width));
	lht_dom_hash_put(ndt, build_textf("height", CFMT, sym->Height));
	lht_dom_hash_put(ndt, build_textf("delta", CFMT, sym->Delta));

	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, lst);
	for(n = 0, li = sym->Line; n < sym->LineN; n++, li++)
		lht_dom_list_append(lst, build_line(li, n, 0, 0));


	return ndt;
}

static lht_node_t *build_font(FontType *font)
{
	lht_node_t *syms, *ndt, *frt;
	int n;

	frt = lht_dom_node_alloc(LHT_HASH, "font");

	/* TODO: support only one, hard-wired font for now but make room for extensions */
	ndt = lht_dom_node_alloc(LHT_HASH, "geda_pcb");
	lht_dom_hash_put(frt, ndt);

	lht_dom_hash_put(ndt, build_textf("cell_height", CFMT, font->MaxHeight));
	lht_dom_hash_put(ndt, build_textf("cell_width", CFMT, font->MaxWidth));
/*	lht_dom_hash_put(ndt, build_symbol(&font->DefaultSymbol)); */


	syms = lht_dom_node_alloc(LHT_HASH, "symbols");
	lht_dom_hash_put(ndt, syms);
	for(n = 0; n < MAX_FONTPOSITION + 1; n++) {
		char sname[32];
		if (!font->Symbol[n].Valid)
			continue;
		if ((n <= 32) || (n > 126) || (n == '&') || (n == '#') || (n == '{') || (n == '}') || (n == '/') || (n == ':') || (n == ';') || (n == '=') || (n == '\\') || (n == ':')) {
			sprintf(sname, "&%02x", n);
		}
		else {
			sname[0] = n;
			sname[1] = '\0';
		}
		lht_dom_hash_put(syms, build_symbol(&font->Symbol[n], sname));
	}
	return frt;
}

static lht_node_t *build_styles(vtroutestyle_t *styles)
{
	lht_node_t *stl, *sn;
	int n;

	stl = lht_dom_node_alloc(LHT_LIST, "styles");
	for(n = 0; n < vtroutestyle_len(styles); n++) {
		RouteStyleType *s = styles->array + n;
		sn = lht_dom_node_alloc(LHT_HASH, s->name);
		lht_dom_list_append(stl, sn);
		lht_dom_hash_put(sn, build_textf("thickness", CFMT, s->Thick));
		lht_dom_hash_put(sn, build_textf("diameter", CFMT, s->Diameter));
		lht_dom_hash_put(sn, build_textf("hole", CFMT, s->Hole));
		lht_dom_hash_put(sn, build_textf("clearance", CFMT, s->Clearance));
	}
	return stl;
}

static lht_doc_t *build_board(PCBType *pcb)
{
	lht_doc_t *brd = lht_dom_init();

	brd->root = lht_dom_node_alloc(LHT_HASH, "pcb-rnd-board-v1");
	lht_dom_hash_put(brd->root, build_board_meta(pcb));
	lht_dom_hash_put(brd->root, build_data(pcb->Data));
	lht_dom_hash_put(brd->root, build_attributes(&pcb->Attributes));
	lht_dom_hash_put(brd->root, build_font(&pcb->Font));
	lht_dom_hash_put(brd->root, build_styles(&pcb->RouteStyle));
	return brd;
}

int io_lihata_write_pcb(plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	int res;
	lht_doc_t *brd = build_board(PCB);
	if ((emergency) || ((old_filename == NULL) && (new_filename == NULL))) {
		/* emergency or pipe save: use the canonical form */
		res = lht_dom_export(brd->root, FP, "");
	}
	else {
		FILE *inf = NULL;
		char *errmsg;
		lhtpers_ev_t events;

		if (old_filename != NULL)
			inf = fopen(old_filename, "r");

		memset(&events, 0, sizeof(events));
/*		events.text = check_text;
		events.list_empty = check_list_empty;
		events.list_elem_removed = check_list_elem_removed;*/
		events.output_rules = io_lihata_out_rules;

		res = lhtpers_fsave_as(&events, brd, inf, FP, old_filename, &errmsg);
		if (inf != NULL)
			fclose(inf);
	}

	lht_dom_uninit(brd);
	return res;
}
