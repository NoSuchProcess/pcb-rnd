/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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

/* Build an in-memory lihata document that represents the board then save it.
   A document is built for the merge-save. */

#include <stdio.h>
#include <stdarg.h>
#include <liblihata/tree.h>
#include <libminuid/libminuid.h>
#include "config.h"
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "flag_str.h"
#include "compat_misc.h"
#include "rats_patch.h"
#include "actions.h"
#include "misc_util.h"
#include "macro.h"
#include "layer.h"
#include "common.h"
#include "write_style.h"
#include "io_lihata.h"
#include "paths.h"
#include "obj_subc_list.h"
#include "pcb_minuid.h"
#include "safe_fs.h"
#include "thermal.h"
#include "funchash_core.h"
#include "netlist.h"
#include "hid_dad.h"

#include "src_plugins/lib_compat_help/pstk_compat.h"

/*#define CFMT "%[9]"*/
#define CFMT "%.08$$mH"
/*#define CFMT "%$$mn"*/

/* Note: this works because of using str_flag compat_types */
#define PCB_OBJ_VIA PCB_OBJ_PSTK
#define PCB_OBJ_PIN PCB_OBJ_PSTK
#define PCB_OBJ_PAD PCB_OBJ_PSTK
#define PCB_OBJ_ELEMENT PCB_OBJ_SUBC

static int io_lihata_full_tree = 0;
static int wrver;

static lht_node_t *build_data(pcb_data_t *data);

#define lht_dom_list_append_safe(lst, item) \
do { \
	lht_node_t *__append_safe__ = item; \
	if (__append_safe__ != NULL) \
		lht_dom_list_append(lst, __append_safe__); \
} while(0)

/* An invalid node will kill any existing node on an overwrite-save-merge */
static lht_node_t *dummy_node(const char *name)
{
	lht_node_t *n;
	n = lht_dom_node_alloc(LHT_TEXT, name);
	n->type = LHT_INVALID_TYPE;
	return n;
}

static lht_node_t *dummy_text_node(const char *name)
{
	if (io_lihata_full_tree) {
		lht_node_t *n = lht_dom_node_alloc(LHT_TEXT, name);
		n->data.text.value = pcb_strdup("");
		return n;
	}
	return dummy_node(name);
}

static lht_node_t *build_text(const char *key, const char *value)
{
	lht_node_t *field;

	if (value == NULL)
		return dummy_text_node(key);

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

static lht_node_t *build_minuid(const char *key, minuid_bin_t val)
{
	minuid_str_t tmp;
	minuid_bin2str(tmp, val);
	return build_text(key, tmp);
}

static lht_node_t *build_board_meta(pcb_board_t *pcb)
{
	lht_node_t *meta, *grp;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(meta, build_text("board_name", pcb->hidlib.name));

	grp = lht_dom_node_alloc(LHT_HASH, "grid");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("offs_x", CFMT, pcb->hidlib.grid_ox));
	lht_dom_hash_put(grp, build_textf("offs_y", CFMT, pcb->hidlib.grid_oy));
	lht_dom_hash_put(grp, build_textf("spacing", CFMT, pcb->hidlib.grid));

	grp = lht_dom_node_alloc(LHT_HASH, "size");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("x", CFMT, pcb->hidlib.size_x));
	lht_dom_hash_put(grp, build_textf("y", CFMT, pcb->hidlib.size_y));
	if (wrver < 5) {
		lht_dom_hash_put(grp, build_textf("isle_area_nm2", "%f", conf_core.design.poly_isle_area));
	}
	lht_dom_hash_put(grp, build_textf("thermal_scale", "%f", pcb->ThermScale));

	if (wrver < 5) {
		grp = lht_dom_node_alloc(LHT_HASH, "drc");
		lht_dom_hash_put(meta, grp);
		lht_dom_hash_put(grp, build_textf("bloat",     CFMT, conf_core.design.bloat));
		lht_dom_hash_put(grp, build_textf("shrink",    CFMT, conf_core.design.shrink));
		lht_dom_hash_put(grp, build_textf("min_width", CFMT, conf_core.design.min_wid));
		lht_dom_hash_put(grp, build_textf("min_silk",  CFMT, conf_core.design.min_slk));
		lht_dom_hash_put(grp, build_textf("min_drill", CFMT, conf_core.design.min_drill));
		lht_dom_hash_put(grp, build_textf("min_ring",  CFMT, conf_core.design.min_ring));
	}

	if (wrver < 5) {
		grp = lht_dom_node_alloc(LHT_HASH, "cursor");
		lht_dom_hash_put(meta, grp);
		lht_dom_hash_put(grp, build_textf("x", CFMT, 0));
		lht_dom_hash_put(grp, build_textf("y", CFMT, 0));
		lht_dom_hash_put(grp, build_textf("zoom", "%.6f", 1000));
	}

	return meta;
}

static lht_node_t *build_attributes(pcb_attribute_list_t *lst)
{
	int n;
	lht_node_t *ln;

	if ((lst->Number == 0) && (!io_lihata_full_tree))
		return dummy_node("attributes");
	ln = lht_dom_node_alloc(LHT_HASH, "attributes");

	for (n = 0; n < lst->Number; n++) {
		if ((wrver < 3) && (strcmp(lst->List[n].name, "intconn") == 0))
			continue; /* do not write intconn as attribute for v1 and v2, we used a flag for those */
		lht_dom_hash_put(ln, build_text(lst->List[n].name, lst->List[n].value));
	}

	return ln;
}

static lht_node_t *build_flags(pcb_flag_t *f, int object_type, int intconn)
{
	int n, layer, added = 0, thrm = 0;
	lht_node_t *hsh, *txt, *lst;
	io_lihata_flag_holder fh;


	fh.Flags = *f;

	hsh = lht_dom_node_alloc(LHT_HASH, "flags");

	/* create normal flag nodes */
	for (n = 0; n < pcb_object_flagbits_len; n++) {
		long my_types = pcb_object_flagbits[n].object_types | ((wrver <= 4) ? pcb_object_flagbits[n].compat_types : 0);
		if ((my_types & object_type) && (PCB_FLAG_TEST(pcb_object_flagbits[n].mask, &fh))) {
			lht_dom_hash_put(hsh, build_text(pcb_object_flagbits[n].name, "1"));
			PCB_FLAG_CLEAR(pcb_object_flagbits[n].mask, &fh);
			added++;
		}
	}

	/* thermal flags per layer */
	lst = lht_dom_node_alloc(LHT_HASH, "thermal");

	for(layer = 0; layer < pcb_max_layer; layer++) {
		if (PCB_FLAG_THERM_TEST_ANY(&fh)) {
			int t = PCB_FLAG_THERM_GET(layer, &fh);
			if (t != 0) {
				const char *name;
				txt = lht_dom_node_alloc(LHT_TEXT, PCB->Data->Layer[layer].name);
				name = io_lihata_thermal_style_old(t);
				if (name != NULL)
					txt->data.text.value = pcb_strdup(name);
				else
					txt->data.text.value = pcb_strdup_printf("%d", t);
				lht_dom_hash_put(lst, txt);
				added++;
				thrm++;
			}
		}
	}

	if (thrm > 0)
		lht_dom_hash_put(hsh, lst);
	else
		lht_dom_node_free(lst);

	if (f->q > 0) {
		lht_dom_hash_put(hsh, build_textf("shape", "%d", f->q));
		added++;
	}

	if ((intconn > 0) && (wrver < 3)) {
		lht_dom_hash_put(hsh, build_textf("intconn", "%d", intconn));
		added++;
	}

	if ((added == 0) && (!io_lihata_full_tree)) {
		lht_dom_node_free(hsh);
		return dummy_node("flags");
	}

	return hsh;
}

static void obj_attr_flag_warn(pcb_any_obj_t *obj)
{
	int warned = 0;

	if (wrver < 5) {
		if (pcb_attribute_get(&obj->Attributes, "intnoconn") != NULL) {
			warned = 1;
			pcb_message(PCB_MSG_WARNING, "pcb-rnd versions only reading file older than lihata v5 may ignore the intnoconn flag\n");
		}
	}
	if (wrver < 3) {
		if (pcb_attribute_get(&obj->Attributes, "intconn") != NULL) {
			warned = 1;
			pcb_message(PCB_MSG_WARNING, "pcb-rnd versions only reading file older than lihata v3 may ignore the intconn flag\n");
		}
	}

	if (warned)
		pcb_message(PCB_MSG_WARNING, "^^^ in %s #%ld\n", pcb_obj_type_name(obj->type), obj->ID);
}

/* Write the thermal list of heavy terminals; put the resulting "thermal"
   node in ha:dst */
void build_thermal_heavy(lht_node_t *dst, pcb_any_obj_t *o)
{
	lht_node_t *th;
	pcb_thermal_t tin = o->thermal;
	const char *bit;

	assert(dst->type == LHT_HASH);

	if (o->thermal == 0) {
		lht_dom_hash_put(dst, dummy_node("thermal"));
		return;
	}

	if (wrver < 4)
		pcb_io_incompat_save(NULL, o, "thermal", "lihata boards before version v4 did not support heavy terminal vias\n", "Either save in lihata v4+ or do not use heavy terminal thermals");

	th = lht_dom_node_alloc(LHT_LIST, "thermal");
	lht_dom_hash_put(dst, th);
	while((bit = pcb_thermal_bits2str(&tin)) != NULL)
		lht_dom_list_append(th, build_textf(NULL, bit));
}


static lht_node_t *build_line(pcb_line_t *line, int local_id, pcb_coord_t dx, pcb_coord_t dy, int simple)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "line.%ld", (local_id >= 0 ? local_id : line->ID));
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	if (!simple) {
		obj_attr_flag_warn((pcb_any_obj_t *)line);
		lht_dom_hash_put(obj, build_attributes(&line->Attributes));
		lht_dom_hash_put(obj, build_flags(&line->Flags, PCB_OBJ_LINE, line->intconn));
		lht_dom_hash_put(obj, build_textf("clearance", CFMT, line->Clearance));
	}

	lht_dom_hash_put(obj, build_textf("thickness", CFMT, line->Thickness));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, line->Point1.X+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, line->Point1.Y+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, line->Point2.X+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, line->Point2.Y+dy));

	build_thermal_heavy(obj, (pcb_any_obj_t*)line);

	return obj;
}

static lht_node_t *build_simplearc(pcb_arc_t *arc, int local_id)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "simplearc.%ld", (local_id >= 0 ? local_id : arc->ID));
	obj = lht_dom_node_alloc(LHT_HASH, buff);
	lht_dom_hash_put(obj, build_textf("x", CFMT, arc->X));
	lht_dom_hash_put(obj, build_textf("y", CFMT, arc->Y));
	lht_dom_hash_put(obj, build_textf("r", CFMT, arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%f", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%f", arc->Delta));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, arc->Thickness));

	return obj;
}


static lht_node_t *build_simplepoly(pcb_poly_t *poly, int local_id)
{
	char buff[128];
	lht_node_t *obj;
	pcb_point_t *pnt;
	int n;

	sprintf(buff, "simplepoly.%ld", (local_id >= 0 ? local_id : poly->ID));
	obj = lht_dom_node_alloc(LHT_LIST, buff);

	for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
		lht_dom_list_append(obj, build_textf(NULL, CFMT, pnt->X));
		lht_dom_list_append(obj, build_textf(NULL, CFMT, pnt->Y));
	}
	return obj;
}


static lht_node_t *build_rat(pcb_rat_t *rat)
{
	char buff[128];
	lht_node_t *obj;
	sprintf(buff, "rat.%ld", rat->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)rat);

	lht_dom_hash_put(obj, build_attributes(&rat->Attributes));
	lht_dom_hash_put(obj, build_flags(&rat->Flags, PCB_OBJ_LINE, rat->intconn));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, rat->Point1.X));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, rat->Point1.Y));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, rat->Point2.X));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, rat->Point2.Y));
	lht_dom_hash_put(obj, build_textf("lgrp1", "%d", rat->group1));
	lht_dom_hash_put(obj, build_textf("lgrp2", "%d", rat->group2));

	return obj;
}

static lht_node_t *build_arc(pcb_arc_t *arc, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "arc.%ld", arc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)arc);

	lht_dom_hash_put(obj, build_attributes(&arc->Attributes));
	lht_dom_hash_put(obj, build_flags(&arc->Flags, PCB_OBJ_ARC, arc->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, arc->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, arc->Clearance));
	lht_dom_hash_put(obj, build_textf("x", CFMT, arc->X+dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, arc->Y+dy));
	lht_dom_hash_put(obj, build_textf("width", CFMT, arc->Width));
	lht_dom_hash_put(obj, build_textf("height", CFMT, arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%ma", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%ma", arc->Delta));

	build_thermal_heavy(obj, (pcb_any_obj_t*)arc);

	return obj;
}

/* attempt to convert a padstack to an old-style via for v1, v2 and v3 */
static lht_node_t *build_pstk_pinvia(pcb_data_t *data, pcb_pstk_t *ps, pcb_bool is_via, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;
	pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask;
	pcb_pstk_compshape_t cshape;
	pcb_bool plated;
	pcb_flag_t flg;
	char *name = pcb_attribute_get(&ps->Attributes, "name");


	if (!pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
		pcb_io_incompat_save(data, (pcb_any_obj_t *)ps, "padstack-old", "Failed to convert to old-style via", "Old via format is very much restricted; try to use a simpler, uniform shape padstack");
		return NULL;
	}

	sprintf(buff, "%s.%ld", is_via ? "via" : "pin", ps->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	flg = pcb_pstk_compat_pinvia_flag(ps, cshape);

	obj_attr_flag_warn((pcb_any_obj_t *)ps);

	lht_dom_hash_put(obj, build_attributes(&ps->Attributes));
	lht_dom_hash_put(obj, build_flags(&flg, PCB_OBJ_VIA, ps->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, pad_dia));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, mask));
	lht_dom_hash_put(obj, build_textf("hole", CFMT, drill_dia));
	lht_dom_hash_put(obj, build_textf("x", CFMT, x + dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, y + dy));
	lht_dom_hash_put(obj, build_text("name", name));
	lht_dom_hash_put(obj, build_text("number", ps->term));
	return obj;
}

/* attempt to convert a padstack to an old-style pad for v1, v2 and v3 */
static lht_node_t *build_pstk_pad(pcb_data_t *data, pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;
	pcb_coord_t x1, y1, x2, y2, thickness, clearance, mask;
	pcb_bool square, nopaste;
	char *name = pcb_attribute_get(&ps->Attributes, "name");
	pcb_flag_t flg;

	if (!pcb_pstk_export_compat_pad(ps, &x1, &y1, &x2, &y2, &thickness, &clearance, &mask, &square, &nopaste)) {
		pcb_io_incompat_save(data, (pcb_any_obj_t *)ps, "padstack-old", "Failed to convert to old-style pad", "Old pad format is very much restricted; try to use a simpler, uniform shape padstack, square or round-cap line based");
		return NULL;
	}

	sprintf(buff, "pad.%ld", ps->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	flg = ps->Flags;
	if (square)
		flg.f |= PCB_FLAG_SQUARE;
	if (nopaste)
		flg.f |= PCB_FLAG_NOPASTE;

	obj_attr_flag_warn((pcb_any_obj_t *)ps);

	lht_dom_hash_put(obj, build_attributes(&ps->Attributes));
	lht_dom_hash_put(obj, build_flags(&flg, PCB_OBJ_PAD, ps->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, mask));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, x1+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, y1+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, x2+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, y2+dy));
	lht_dom_hash_put(obj, build_text("name", name));
	lht_dom_hash_put(obj, build_text("number", ps->term));
	return obj;
}

static lht_node_t *build_polygon(pcb_poly_t *poly)
{
	char buff[128];
	lht_node_t *obj, *tbl, *geo;
	pcb_cardinal_t n, hole = 0;

	sprintf(buff, "polygon.%ld", poly->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)poly);

	lht_dom_hash_put(obj, build_attributes(&poly->Attributes));
	lht_dom_hash_put(obj, build_flags(&poly->Flags, PCB_OBJ_POLY, poly->intconn));

	if ((wrver >= 3) && (poly->Clearance > 0))
		lht_dom_hash_put(obj, build_textf("clearance", CFMT, poly->Clearance));
	else
		lht_dom_hash_put(obj, dummy_node("clearance"));

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

		lht_dom_node_free(tbl->data.table.r[row][0]);
		lht_dom_node_free(tbl->data.table.r[row][1]);
		tbl->data.table.r[row][0] = build_textf(NULL, CFMT, poly->Points[n].X);
		tbl->data.table.r[row][1] = build_textf(NULL, CFMT, poly->Points[n].Y);
	}

	build_thermal_heavy(obj, (pcb_any_obj_t*)poly);

	return obj;
}

static lht_node_t *build_pcb_text(const char *role, pcb_text_t *text)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "text.%ld", text->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)text);

	lht_dom_hash_put(obj, build_attributes(&text->Attributes));
	lht_dom_hash_put(obj, build_flags(&text->Flags, PCB_OBJ_TEXT, text->intconn));
	lht_dom_hash_put(obj, build_text("string", text->TextString));
	lht_dom_hash_put(obj, build_textf("fid", "%ld", text->fid));
	lht_dom_hash_put(obj, build_textf("scale", "%d", text->Scale));
	lht_dom_hash_put(obj, build_textf("x", CFMT, text->X));
	lht_dom_hash_put(obj, build_textf("y", CFMT, text->Y));
	if (wrver >= 6) {
		lht_dom_hash_put(obj, build_textf("rot", "%f", text->rot));
		if (text->thickness > 0)
			lht_dom_hash_put(obj, build_textf("thickness", CFMT, text->thickness));
		else
			lht_dom_hash_put(obj, dummy_node("thickness"));
	}
	else {
		int dir;
		if (!pcb_text_old_direction(&dir, text->rot))
			pcb_io_incompat_save(NULL, (pcb_any_obj_t *)text, "text-rot", "versions below lihata board v6 do not support arbitrary text rotation - rounding to 90 degree rotation", "Use only 90 degree rotations through the direction field");

		lht_dom_hash_put(obj, build_textf("direction", "%d", dir));

		if (text->thickness > 0)
			pcb_io_incompat_save(NULL, (pcb_any_obj_t *)text, "text-width", "versions below lihata board v6 do not support arbitrary text width - will render with default width", "Leave the thickness field empty (zero) and depend on the automatism");
	}

	if (role != NULL)
		lht_dom_hash_put(obj, build_text("role", role));

	return obj;
}

static lht_node_t *build_subc_element(pcb_subc_t *subc)
{
	char buff[128];
	lht_node_t *obj, *lst;
	pcb_coord_t ox, oy;
	gdl_iterator_t it;
	int l, seen_refdes = 0;
	pcb_pstk_t *ps;

	if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
		assert(subc->parent_type == PCB_PARENT_DATA);
		pcb_io_incompat_save(subc->parent.data, (pcb_any_obj_t *)subc, "subc-elem", "Failed to convert subc to old-style element: missing origin", "make sure the subcircuit has the vectors on its subc-aux layer");
		return NULL;
	}

	sprintf(buff, "element.%ld", subc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)subc);

	lht_dom_hash_put(obj, build_attributes(&subc->Attributes));
	lht_dom_hash_put(obj, build_flags(&subc->Flags, PCB_OBJ_ELEMENT, 0));

	/* build drawing primitives */
	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(obj, lst);


	lht_dom_hash_put(obj, build_textf("x", CFMT, ox));
	lht_dom_hash_put(obj, build_textf("y", CFMT, oy));

	/* export layer objects: silk lines and arcs */
	for(l = 0; l < subc->data->LayerN; l++) {
		pcb_layer_t *ly = &subc->data->Layer[l];
		pcb_line_t *line;
		pcb_arc_t *arc;
		pcb_text_t *text;

		if ((ly->meta.bound.type & PCB_LYT_SILK) && (ly->meta.bound.type & PCB_LYT_TOP)) {
			linelist_foreach(&ly->Line, &it, line)
				lht_dom_list_append(lst, build_line(line, -1, -ox, -oy, 0));
			arclist_foreach(&ly->Arc, &it, arc) 
				lht_dom_list_append(lst, build_arc(arc, -ox, -oy));
			textlist_foreach(&ly->Text, &it, text) {
				if (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text)) {
					if (!seen_refdes) {
						pcb_text_t tmp;
						memcpy(&tmp, text, sizeof(tmp));
						tmp.TextString = pcb_attribute_get(&subc->Attributes, "footprint");
						lht_dom_list_append(lst, build_pcb_text("desc", &tmp));
						tmp.TextString = pcb_attribute_get(&subc->Attributes, "refdes");
						lht_dom_list_append(lst, build_pcb_text("name", &tmp));
						tmp.TextString = pcb_attribute_get(&subc->Attributes, "value");
						lht_dom_list_append(lst, build_pcb_text("value", &tmp));
						seen_refdes = 1;
					}
				}
				else {
					pcb_io_incompat_save(subc->parent.data, (pcb_any_obj_t *)text, "subc-text", "can't export custom silk text object", "the only text old pcb elements support is the refdes/value/description text");
				}
			}
			if (polylist_length(&ly->Polygon) > 0) {
				char *desc = pcb_strdup_printf("Polygons on layer %s can not be exported in an element", ly->name);
				pcb_io_incompat_save(subc->data, NULL, desc, "subc-objs", "only lines and arcs are exported");
				free(desc);
			}
			if (textlist_length(&ly->Text) > 1) {
				char *desc = pcb_strdup_printf("Text on layer %s can not be exported in an element", ly->name);
				pcb_io_incompat_save(subc->data, NULL, desc, "subc-objs", "only lines and arcs are exported");
				free(desc);
			}
			continue;
		}

		if (!(ly->meta.bound.type & PCB_LYT_VIRTUAL) && (!pcb_layer_is_pure_empty(ly))) {
			char *desc = pcb_strdup_printf("Objects on layer %s can not be exported in an element", ly->name);
			pcb_io_incompat_save(subc->data, NULL, desc, "subc-layer", "only top silk lines and arcs are exported; heavy terminals are not supported, use padstacks only");
			free(desc);
		}
	}

	for(ps = padstacklist_first(&subc->data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
		lht_node_t *nps;

		nps = build_pstk_pinvia(subc->data, ps, pcb_false, -ox, -oy);
		if (nps == NULL)
			nps = build_pstk_pad(subc->data, ps, -ox, -oy);
		if (nps != NULL)
			lht_dom_list_append(lst, nps);
		else
			pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)ps, "padstack-old", "Padstack can not be exported as pin or pad", "use simpler padstack; for pins, all copper layers must have the same shape and there must be no paste; for pads, use a line or a rectangle; paste and mask must match the copper shape");
	}

	if (!seen_refdes)
		pcb_io_incompat_save(subc->parent.data, (pcb_any_obj_t *)subc, "subc-refdes", "can't export subcircuit without refdes text on silk", "old pcb elements require refdes text on silk");

	return obj;
}

static lht_node_t *build_subc(pcb_subc_t *sc)
{
	char buff[128];
	lht_node_t *obj;

	if (wrver < 3)
		return build_subc_element(sc);

	sprintf(buff, "subc.%ld", sc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	obj_attr_flag_warn((pcb_any_obj_t *)sc);

	lht_dom_hash_put(obj, build_attributes(&sc->Attributes));
	lht_dom_hash_put(obj, build_flags(&sc->Flags, PCB_OBJ_SUBC, 0));
	lht_dom_hash_put(obj, build_data(sc->data));
	lht_dom_hash_put(obj, build_minuid("uid", sc->uid));

	return obj;
}

static void build_layer_stack_flag(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name)
{
	lht_node_t *dst = ctx;
	lht_dom_hash_put(dst, build_text(name, "1"));
}

static void build_data_layer_comb(void *ctx, pcb_layer_combining_t bit, const char *name)
{
	lht_node_t *comb = ctx;
	lht_dom_hash_put(comb, build_text(name, "1"));
}


static lht_node_t *build_pstk_protos(pcb_data_t *data, pcb_vtpadstack_proto_t *pp)
{
	lht_node_t *lst, *nproto, *nmask, *nshape, *nshapelst, *ncomb, *nshapeo;
	pcb_cardinal_t n, sn, pn;
	pcb_pstk_tshape_t *ts;
	char tmp[64];
	pcb_layer_type_t lyt_permit = PCB_LYT_ANYWHERE | PCB_LYT_COPPER | PCB_LYT_SILK | PCB_LYT_MASK | PCB_LYT_PASTE;

	if (wrver >= 6)
		lyt_permit |= PCB_LYT_MECH;

	lst = lht_dom_node_alloc(LHT_LIST, "padstack_prototypes");
	for(n = 0; n < pcb_vtpadstack_proto_len(pp); n++) {
		pcb_pstk_proto_t *proto = pp->array+n;

		if (!proto->in_use) {
			lht_dom_list_append(lst, build_text("unused", "1"));
			continue;
		}

		if (wrver < 6)
			sprintf(tmp, "ps_proto_v4.%ld", (long)n);
		else
			sprintf(tmp, "ps_proto_v6.%ld", (long)n);

		lht_dom_list_append(lst, nproto = lht_dom_node_alloc(LHT_HASH, tmp));

		lht_dom_hash_put(nproto, build_textf("hdia", CFMT, proto->hdia));
		lht_dom_hash_put(nproto, build_textf("htop", "%d", proto->htop));
		lht_dom_hash_put(nproto, build_textf("hbottom", "%d", proto->hbottom));
		lht_dom_hash_put(nproto, build_textf("hplated", "%d", proto->hplated));
		if (proto->name != NULL) {
			if (wrver >= 5)
				lht_dom_hash_put(nproto, build_text("name", proto->name));
			else
				pcb_io_incompat_save(NULL, NULL, "padstack-name", "versions below lihata board v5 do not support padstack prototype names\n", "Be aware that padstack proto names are lost in save or use lihata board v5 or higher");
		}
		else
			lht_dom_hash_put(nproto, dummy_node("name"));

		/* save each shape */
		lht_dom_hash_put(nproto, nshapelst = lht_dom_node_alloc(LHT_LIST, "shape"));
		ts = &proto->tr.array[0]; /* save the canonical shape only, the transformation cache is generated runtime */
		if (ts != NULL)
		for(sn = 0; sn < ts->len; sn++) {
			pcb_pstk_shape_t *shape = ts->shape + sn;
			pcb_layer_type_t save_mask;

			save_mask = shape->layer_mask & lyt_permit;
			if (save_mask != shape->layer_mask) {
				pcb_io_incompat_save(data, NULL, "padstack-layer", "Can not save padstack prototype properly because it uses a layer type not supported by this version of lihata padstack.", "Either save in the latest lihata - or accept that some shapes are omitted");
				continue;
			}

			lht_dom_list_append(nshapelst, nshape = lht_dom_node_alloc(LHT_HASH, "ps_shape_v4"));

			lht_dom_hash_put(nshape, nmask = lht_dom_node_alloc(LHT_HASH, "layer_mask"));
			pcb_layer_type_map(save_mask, nmask, build_layer_stack_flag);

			lht_dom_hash_put(nshape, ncomb = lht_dom_node_alloc(LHT_HASH, "combining"));
			pcb_layer_comb_map(shape->comb, ncomb, build_data_layer_comb);

			lht_dom_hash_put(nshape, build_textf("clearance", CFMT, shape->clearance));

			switch(shape->shape) {
				case PCB_PSSH_POLY:
					nshapeo = lht_dom_node_alloc(LHT_LIST, "ps_poly");
					for(pn = 0; pn < shape->data.poly.len; pn++) {
						lht_dom_list_append(nshapeo, build_textf(NULL, CFMT, shape->data.poly.x[pn]));
						lht_dom_list_append(nshapeo, build_textf(NULL, CFMT, shape->data.poly.y[pn]));
					}
					break;
				case PCB_PSSH_LINE:
					nshapeo = lht_dom_node_alloc(LHT_HASH, "ps_line");
					lht_dom_hash_put(nshapeo, build_textf("x1", CFMT, shape->data.line.x1));
					lht_dom_hash_put(nshapeo, build_textf("y1", CFMT, shape->data.line.y1));
					lht_dom_hash_put(nshapeo, build_textf("x2", CFMT, shape->data.line.x2));
					lht_dom_hash_put(nshapeo, build_textf("y2", CFMT, shape->data.line.y2));
					lht_dom_hash_put(nshapeo, build_textf("thickness", CFMT, shape->data.line.thickness));
					lht_dom_hash_put(nshapeo, build_textf("square", "%d", shape->data.line.square));
					break;
				case PCB_PSSH_CIRC:
					nshapeo = lht_dom_node_alloc(LHT_HASH, "ps_circ");
					lht_dom_hash_put(nshapeo, build_textf("x", CFMT, shape->data.circ.x));
					lht_dom_hash_put(nshapeo, build_textf("y", CFMT, shape->data.circ.y));
					lht_dom_hash_put(nshapeo, build_textf("dia", CFMT, shape->data.circ.dia));
					break;
				case PCB_PSSH_HSHADOW:
					if (wrver < 6) {
						pcb_io_incompat_save(data, NULL, "padstack-shape", "Can not save padstack prototype shape \"hshadow\" in lihata formats below version 6.", "Either save in lihata v6 - or accept that the padstack will connect more layers than it should.");
						nshapeo = lht_dom_node_alloc(LHT_HASH, "ps_circ");
						lht_dom_hash_put(nshapeo, build_textf("dia", CFMT, 1));
					}
					else
						nshapeo = build_text("ps_hshadow", "");
					break;
				default:
					pcb_message(PCB_MSG_ERROR, "Internal error: unimplemented pad stack shape %d\n", shape->shape);
					abort();
			}
			lht_dom_hash_put(nshape, nshapeo);
		}
	}

	return lst;
}

static lht_node_t *build_pstk(pcb_pstk_t *ps)
{
	char buff[128];
	lht_node_t *obj, *thr, *thr_lst;
	unsigned long n;

	sprintf(buff, "padstack_ref.%ld", ps->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&ps->Attributes));
	lht_dom_hash_put(obj, build_flags(&ps->Flags, PCB_OBJ_PSTK, ps->intconn));

	lht_dom_hash_put(obj, build_textf("proto", "%ld", (long int)ps->proto));
	lht_dom_hash_put(obj, build_textf("x", CFMT, ps->x));
	lht_dom_hash_put(obj, build_textf("y", CFMT, ps->y));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, ps->Clearance));
	lht_dom_hash_put(obj, build_textf("rot", "%f", ps->rot));
	lht_dom_hash_put(obj, build_textf("xmirror", "%d", ps->xmirror));
	lht_dom_hash_put(obj, build_textf("smirror", "%d", ps->smirror));

	lht_dom_hash_put(obj, thr = lht_dom_node_alloc(LHT_LIST, "thermal"));
	for(n = 0; n < ps->thermals.used; n++) {
		pcb_thermal_t ts = ps->thermals.shape[n];
		if (ts != 0) {
			char tmp[64];
			const char *b;
			sprintf(tmp, "%ld", n);
			thr_lst = lht_dom_node_alloc(LHT_LIST, tmp);
			while((b = pcb_thermal_bits2str(&ts)) != NULL)
				lht_dom_list_append(thr_lst, build_text(NULL, b));
			if ((wrver >= 6) && ((ts & 3) == PCB_THERMAL_NOSHAPE))
				lht_dom_list_append(thr_lst, build_text(NULL, "noshape"));
			lht_dom_list_append(thr, thr_lst);
		}

	}

	return obj;
}

static lht_node_t *build_layer_stack(pcb_board_t *pcb)
{
	lht_node_t *lstk, *grps, *grp, *layers, *flags;
	int n, i, outlines = 0;

	lstk = lht_dom_node_alloc(LHT_HASH, "layer_stack");
	lht_dom_hash_put(lstk, grps = lht_dom_node_alloc(LHT_LIST, "groups"));

	for(n = 0; n < pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[n];
		pcb_layer_type_t lyt;
		char tmp[32];
		sprintf(tmp, "%d", n);
		lht_dom_list_append(grps, grp = lht_dom_node_alloc(LHT_HASH, tmp));

		if (wrver >= 5)
			lht_dom_hash_put(grp, build_attributes(&g->Attributes));
		else if (g->Attributes.Number > 0)
			pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)g, "group-meta", "Can not save layer group attributes in lihata formats below version 5.", "Either save in lihata v5 - or accept that attributes are not saved");

		lht_dom_hash_put(grp, build_text("name", g->name));
		lht_dom_hash_put(grp, layers = lht_dom_node_alloc(LHT_LIST, "layers"));
		for(i = 0; i < g->len; i++)
			lht_dom_list_append(layers, build_textf("", "%ld", g->lid[i]));

		lyt = g->ltype;
		if (wrver < 6) {
			int is_outline = (PCB_LAYER_IS_OUTLINE(g->ltype, g->purpi));

			if ((!is_outline) && (g->purpose != NULL))
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)g, "group-meta", "Can not save layer group purpose in lihata formats below version 6.", "Either save in lihata v6 - or accept that these layers will change type in the file");

			if (is_outline) {
				lht_dom_hash_put(grp, flags = lht_dom_node_alloc(LHT_HASH, "type"));
				pcb_layer_type_map(lyt & (~PCB_LYT_ANYTHING), flags, build_layer_stack_flag);
				lht_dom_hash_put(flags, build_text("outline", "1"));
				outlines++;
				if (outlines > 1) {
					pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)g, "group", "Can not save multiple outline layer groups in lihata board version below v6", "Save in lihata board v6");
					return NULL;
				}
			}
			else if ((lyt & PCB_LYT_DOC) || (lyt & PCB_LYT_MECH) || (lyt & PCB_LYT_BOUNDARY)) {
				lyt &= ~(PCB_LYT_DOC | PCB_LYT_MECH | PCB_LYT_BOUNDARY);
				pcb_io_incompat_save(pcb->Data, (pcb_any_obj_t *)g, "group", "Can not save layer group type DOC or MECH in lihata formats below version 6, saving as MISC.", "Either save in lihata v6 - or accept that these layers will change type in the file");
			}
		}
		else {
			if (g->purpose != NULL)
				lht_dom_hash_put(grp, build_text("purpose", g->purpose));
			else
				lht_dom_hash_put(grp, dummy_node("purpose"));
		}
		lht_dom_hash_put(grp, flags = lht_dom_node_alloc(LHT_HASH, "type"));
		pcb_layer_type_map(lyt, flags, build_layer_stack_flag);
	}

	return lstk;
}


static lht_node_t *build_data_layer(pcb_data_t *data, pcb_layer_t *layer, pcb_layergrp_id_t layer_group, pcb_layer_id_t lid)
{
	lht_node_t *obj, *grp, *comb;
	pcb_line_t *li;
	pcb_arc_t *ar;
	pcb_poly_t *po;
	pcb_text_t *tx;
	int added = 0;

	obj = lht_dom_node_alloc(LHT_HASH, layer->name);

	if (!layer->is_bound) {
		if (wrver < 6)
			lht_dom_hash_put(obj, build_text("visible", layer->meta.real.vis ? "1" : "0"));
		lht_dom_hash_put(obj, build_textf("group", "%ld", layer_group));
		if (wrver >= 5)
			lht_dom_hash_put(obj, build_text("color", layer->meta.real.color.str));
	}
	else {
		if (wrver >= 3) {
			lht_node_t *type;

			if (layer->meta.bound.stack_offs != 0)
				lht_dom_hash_put(obj, build_textf("stack_offs", "%d", layer->meta.bound.stack_offs));

			lht_dom_hash_put(obj, type = lht_dom_node_alloc(LHT_HASH, "type"));
			pcb_layer_type_map(layer->meta.bound.type, type, build_layer_stack_flag);
			if (wrver >= 6) {
				if (layer->meta.bound.purpose != NULL)
					lht_dom_hash_put(obj, build_text("purpose", layer->meta.bound.purpose));
				else
					lht_dom_hash_put(obj, dummy_node("purpose"));
			}
			else if (layer->meta.bound.purpose != NULL)
				pcb_message(PCB_MSG_WARNING, "io_lihata: attempting to save bound layer with a purpose string - not supported in lihata board below v6. Layer binding might be broken after load.\n");
		}
		else
			pcb_message(PCB_MSG_WARNING, "io_lihata: attempting to save bound layers in lihata version lower than 3; feature not supported by the format.\n");
	}

	if (!layer->is_bound)
		lht_dom_hash_put(obj, build_attributes(&layer->Attributes));

	if (wrver >= 2) {
		lht_dom_hash_put(obj, build_textf("lid", "%ld", lid));
		lht_dom_hash_put(obj, comb = lht_dom_node_alloc(LHT_HASH, "combining"));
		pcb_layer_comb_map(layer->comb, comb, build_data_layer_comb);
	}

	grp = lht_dom_node_alloc(LHT_LIST, "objects");

	for(li = linelist_first(&layer->Line); li != NULL; li = linelist_next(li)) {
		lht_dom_list_append(grp, build_line(li, -1, 0, 0, 0));
		added++;
	}

	for(ar = arclist_first(&layer->Arc); ar != NULL; ar = arclist_next(ar)) {
		lht_dom_list_append(grp, build_arc(ar, 0, 0));
		added++;
	}

	for(po = polylist_first(&layer->Polygon); po != NULL; po = polylist_next(po)) {
		lht_dom_list_append(grp, build_polygon(po));
		added++;
	}

	for(tx = textlist_first(&layer->Text); tx != NULL; tx = textlist_next(tx)) {
		lht_dom_list_append(grp, build_pcb_text(NULL, tx));
		added++;
	}

	lht_dom_hash_put(obj, grp);

	return obj;
}

#define LAYER_GID_FIX_V1() \
	do { \
		if (wrver == 1) { \
			if (gid >= 0) { \
				if (gid < sizeof(grp) / sizeof(grp[0])) \
					gid = grp[gid]; \
				else \
					gid = -1; \
			} \
		} \
	} while(0)


static lht_node_t *build_data_layers(pcb_data_t *data)
{
	long int n;
	lht_node_t *layers;
	pcb_layergrp_t *g;

	layers = lht_dom_node_alloc(LHT_LIST, "layers");

	if (wrver == 1) { /* produce an old layer group assignment from top to bottom (needed for v1, good for other versions too) */
		pcb_layergrp_id_t gm, grp[PCB_MAX_LAYERGRP], gtop = -1, gbottom = -1;

		gm = 0;
		for(n = 0; n < pcb_max_group(PCB); n++) {
			unsigned int gflg = pcb_layergrp_flags(PCB, n);
			if (gflg & PCB_LYT_COPPER) {
				if (gflg & PCB_LYT_TOP)
					gtop = gm;
				if (gflg & PCB_LYT_BOTTOM)
					gbottom = gm;
				grp[n] = gm;
/*				pcb_trace("build data layers: %d -> %ld {%ld %ld}\n", n, gm, gtop, gbottom); */
				gm++;
			}
			else
				grp[n] = -1;
		}

		g = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_SILK);
		if (g == NULL) {
			pcb_io_incompat_save(NULL, NULL, "layer", "lihata board v1 did not support a layer stackup without bottom silk\n", "Either create the top silk layer or save in at least v2\nNote: only pcb-rnd above 2.1.0 will load boards without silk; best use v6 or higher.");
			return NULL;
		}
		grp[g - PCB->LayerGroups.grp] = gbottom;

		g = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_TOP, PCB_LYT_SILK);
		if (g == NULL) {
			pcb_io_incompat_save(NULL, NULL, "layer", "lihata board v1 did not support a layer stackup without top silk\n", "Either create the top silk layer or save in at least v2\nNote: only pcb-rnd above 2.1.0 will load boards without silk; best use v6 or higher.");
			return NULL;
		}
		grp[g - PCB->LayerGroups.grp] = gtop;

		/* v1 needs to have silk at the end of the list */
		for(n = 0; n < pcb_max_layer; n++) {
			if ((pcb_layer_flags(PCB, n) & PCB_LYT_SILK) == 0) {
				pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, n);
				LAYER_GID_FIX_V1();
				lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
			}
		}
		for(n = 0; n < pcb_max_layer; n++) {
			if (pcb_layer_flags(PCB, n) & PCB_LYT_SILK) {
				pcb_layer_id_t gid = pcb_layer_get_group(PCB, n);
				LAYER_GID_FIX_V1();
				lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
			}
		}
	}
	else {
		/* keep the original order from v2, to minimize diffs */
		for(n = 0; n < data->LayerN; n++) {
			pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, n);
			lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
		}
	}

	return layers;
}


static lht_node_t *build_data(pcb_data_t *data)
{
	lht_node_t *grp, *ndt, *dlr;
	pcb_pstk_t *ps;
	pcb_subc_t *sc;
	gdl_iterator_t it;
	pcb_rat_t *line;

	dlr = build_data_layers(data);
	if (dlr == NULL)
		return NULL;

	ndt = lht_dom_node_alloc(LHT_HASH, "data");

	/* build layers */
	lht_dom_hash_put(ndt, dlr);

	/* build a list of all global objects */
	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, grp);

	if (wrver >= 4) {
		lht_dom_hash_put(ndt, build_pstk_protos(data, &data->ps_protos));
		for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps))
			lht_dom_list_append(grp, build_pstk(ps));
	}
	else {
		for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
			lht_node_t *p = build_pstk_pinvia(data, ps, pcb_true, 0, 0);
			if (p != NULL)
				lht_dom_list_append(grp, p);
		}
	}

	for(sc = pcb_subclist_first(&data->subc); sc != NULL; sc = pcb_subclist_next(sc))
		lht_dom_list_append_safe(grp, build_subc(sc));

	ratlist_foreach(&data->Rat, &it, line)
		lht_dom_list_append(grp, build_rat(line));

	return ndt;
}

static lht_node_t *build_symbol(pcb_symbol_t *sym, const char *name)
{
	lht_node_t *lst, *ndt;
	pcb_line_t *li;
	pcb_poly_t *poly;
	pcb_arc_t *arc;
	int n;

	ndt = lht_dom_node_alloc(LHT_HASH, name);
	lht_dom_hash_put(ndt, build_textf("width", CFMT, sym->Width));
	lht_dom_hash_put(ndt, build_textf("height", CFMT, sym->Height));
	lht_dom_hash_put(ndt, build_textf("delta", CFMT, sym->Delta));

	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, lst);
	for(n = 0, li = sym->Line; n < sym->LineN; n++, li++)
		lht_dom_list_append(lst, build_line(li, n, 0, 0, 1));

	for(arc = arclist_first(&sym->arcs); arc != NULL; arc = arclist_next(arc), n++)
		lht_dom_list_append(lst, build_simplearc(arc, n));

	for(poly = polylist_first(&sym->polys); poly != NULL; poly = polylist_next(poly), n++)
		lht_dom_list_append(lst, build_simplepoly(poly, n));

	return ndt;
}

static lht_node_t *build_font(pcb_font_t *font)
{
	lht_node_t *syms, *ndt;
	char *sid, sidbuff[64];
	int n;

	if (font->id > 0) {
		sprintf(sidbuff, "%ld", font->id);
		sid = sidbuff;
	}
	else
		sid = "geda_pcb"; /* special case for comaptibility: font 0 is the geda_pcb font */

	ndt = lht_dom_node_alloc(LHT_HASH, sid);

	lht_dom_hash_put(ndt, build_textf("cell_height", CFMT, font->MaxHeight));
	lht_dom_hash_put(ndt, build_textf("cell_width", CFMT, font->MaxWidth));
	lht_dom_hash_put(ndt, build_textf("id", "%ld", font->id));
	if (font->name != NULL)
		lht_dom_hash_put(ndt, build_text("name", font->name));
/*	lht_dom_hash_put(ndt, build_symbol(&font->DefaultSymbol)); */


	syms = lht_dom_node_alloc(LHT_HASH, "symbols");
	lht_dom_hash_put(ndt, syms);
	for(n = 0; n < PCB_MAX_FONTPOSITION + 1; n++) {
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
	return ndt;
}

static lht_node_t *build_fontkit(pcb_fontkit_t *fk)
{
	lht_node_t *ndt, *frt;

	if (conf_io_lihata.plugins.io_lihata.omit_font)
		return dummy_node("font");

	frt = lht_dom_node_alloc(LHT_HASH, "font");

	/* write the default font first - it's always there */
	ndt = build_font(&fk->dflt);
	lht_dom_hash_put(frt, ndt);

	/* write extra fonts, if there are */
	if (fk->hash_inited) {
		htip_entry_t *e;
		for(e = htip_first(&fk->fonts); e; e = htip_next(&fk->fonts, e)) {
			ndt = build_font(e->value);
			lht_dom_hash_put(frt, ndt);
		}
	}


	return frt;
}


static lht_node_t *build_styles(vtroutestyle_t *styles)
{
	lht_node_t *stl, *sn;
	int n;

	if (conf_io_lihata.plugins.io_lihata.omit_styles)
		return dummy_node("styles");

	stl = lht_dom_node_alloc(LHT_LIST, "styles");
	for(n = 0; n < vtroutestyle_len(styles); n++) {
		pcb_route_style_t *s = styles->array + n;
		sn = lht_dom_node_alloc(LHT_HASH, s->name);
		lht_dom_list_append(stl, sn);
		lht_dom_hash_put(sn, build_textf("thickness", CFMT, s->Thick));
		lht_dom_hash_put(sn, build_textf("diameter", CFMT, s->Diameter));
		lht_dom_hash_put(sn, build_textf("hole", CFMT, s->Hole));
		lht_dom_hash_put(sn, build_textf("clearance", CFMT, s->Clearance));

		if (wrver >= 5) {
			if (s->via_proto_set)
				lht_dom_hash_put(sn, build_textf("via_proto", "%ld", (long int)s->via_proto));
			else
			lht_dom_hash_put(sn, dummy_text_node("via_proto"));
		}
		else
			pcb_io_incompat_save(NULL, NULL, "route-style", "lihata boards before version v5 did not support padstack prototype in route style\n", "Either save in lihata v5+ or be aware of losing this information");

		if (wrver >= 6)
			lht_dom_hash_put(sn, build_textf("text_thick", CFMT, s->textt));
		else if (s->textt > 0)
			pcb_io_incompat_save(NULL, NULL, "route-style", "lihata boards before version v6 did not support text thickness in route style\n", "Either save in lihata v6+ or be aware of losing this information");

		if (wrver >= 6)
			lht_dom_hash_put(sn, build_textf("text_scale", "%d", s->texts));
		else if (s->texts > 0)
			pcb_io_incompat_save(NULL, NULL, "route-style", "lihata boards before version v6 did not support text scale in route style\n", "Either save in lihata v6+ or be aware of losing this information");

		lht_dom_hash_put(sn, build_attributes(&s->attr));
	}
	return stl;
}

/* Build a plain old netlist */
static lht_node_t *build_netlist(pcb_netlist_t *netlist, const char *name, int *nonempty)
{
	lht_node_t *nl, *pl, *pn, *nnet;

	if (netlist->used < 1)
		return dummy_node(name);

	(*nonempty)++;

	nl = lht_dom_node_alloc(LHT_LIST, name);

	{
		htsp_entry_t *e;
		for(e = htsp_first(netlist); e != NULL; e = htsp_next(netlist, e)) {
			pcb_net_t *net = e->value;
			pcb_net_term_t *t;
			const char *netname = net->name;
			const char *style = pcb_attribute_get(&net->Attributes, "style");

			/* create the net hash */
			nnet = lht_dom_node_alloc(LHT_HASH, netname);

			if (wrver >= 5)
				lht_dom_hash_put(nnet, build_attributes(&net->Attributes));
			else if (net->Attributes.Number > 0)
				pcb_io_incompat_save(NULL, (pcb_any_obj_t *)net, "net-attr", "Can not save netlist attributes in lihata formats below version 5.", "Either save in lihata v5 - or accept that attributes are not saved");

			pl = lht_dom_node_alloc(LHT_LIST, "conn");
			lht_dom_hash_put(nnet, pl);
			if ((style != NULL) && (*style == '\0')) style = NULL;
			lht_dom_hash_put(nnet, build_text("style", style));

			/* grow the connection list */
			for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
				pn = lht_dom_node_alloc(LHT_TEXT, "");
				pn->data.text.value = pcb_strdup_printf("%s-%s", t->refdes, t->term);
				lht_dom_list_append(pl, pn);
			}
			lht_dom_list_append(nl, nnet);
		}
	}

	return nl;
}

typedef struct {
	lht_node_t *patch, *info;
} build_net_patch_t;

static void build_net_patch_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	build_net_patch_t *ctx = ctx_;
	lht_node_t *n;
	switch(ev) {
		case PCB_RPE_INFO_BEGIN:
			ctx->info = lht_dom_node_alloc(LHT_LIST, "net_info");
			lht_dom_list_append(ctx->info, build_text("net", netn));
			break;
		case PCB_RPE_INFO_TERMINAL:
			lht_dom_list_append(ctx->info, build_text("term", val));
			break;
		case PCB_RPE_INFO_END:
			lht_dom_list_append(ctx->patch, ctx->info);
			ctx->info = NULL;
			break;
		case PCB_RPE_CONN_ADD:
			n = lht_dom_node_alloc(LHT_HASH, "add_conn");
			goto append_term;

		case PCB_RPE_CONN_DEL:
			n = lht_dom_node_alloc(LHT_HASH, "del_conn");
			append_term:;
			lht_dom_hash_put(n, build_text("net", netn));
			lht_dom_hash_put(n, build_text("term", val));
			lht_dom_list_append(ctx->patch, n);
			break;

		case PCB_RPE_ATTR_CHG:
			n = lht_dom_node_alloc(LHT_HASH, "change_attrib");
			lht_dom_hash_put(n, build_text("net", netn));
			lht_dom_hash_put(n, build_text("key", key));
			lht_dom_hash_put(n, build_text("val", val));
			lht_dom_list_append(ctx->patch, n);
			break;
	}
}

/* Build a netlist patch so that we don't need to export a complete new set of "as built" netlist */
static lht_node_t *build_net_patch(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, int *nonempty)
{
	lht_node_t *pn;
	build_net_patch_t ctx;

	pn = lht_dom_node_alloc(LHT_LIST, "netlist_patch");

	ctx.patch = pn;
	pcb_rats_patch_export(pcb, pat, pcb_false, build_net_patch_cb, &ctx);

	if (pn->data.list.first == NULL) {
		lht_dom_node_free(pn);
		return dummy_node("netlist_patch");
	}

	(*nonempty)++;
	return pn;
}


static lht_node_t *build_netlists(pcb_board_t *pcb, pcb_netlist_t *netlists, pcb_ratspatch_line_t *pat, int num_netlists)
{
	lht_node_t *nls;
	int n, nonempty = 0;

	if (num_netlists > PCB_NUM_NETLISTS)
		return dummy_node("netlists");

	nls = lht_dom_node_alloc(LHT_HASH, "netlists");

	for(n = 0; n < num_netlists; n++) {
		lht_node_t *nl;
		if (n == PCB_NETLIST_EDITED)
			nl = build_net_patch(pcb, pat, &nonempty);
		else
			nl = build_netlist(netlists+n, pcb_netlist_names[n], &nonempty);
		lht_dom_hash_put(nls, nl);
	}

	if (!nonempty) {
		lht_dom_node_free(nls);
		return dummy_node("netlists");
	}

	return nls;
}

extern lht_doc_t *pcb_conf_main_root[CFR_max_alloc];
static lht_node_t *build_conf()
{
	const char **s, *del_paths[] = { "editor/mode", NULL };
	lht_node_t *res, *n;

	if (conf_io_lihata.plugins.io_lihata.omit_config)
		return dummy_node("pcb-rnd-conf-v1");

	if ((pcb_conf_main_root[CFR_DESIGN] == NULL) || (pcb_conf_main_root[CFR_DESIGN]->root == NULL) || (pcb_conf_main_root[CFR_DESIGN]->root->type != LHT_LIST))
		return lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");

	res = lht_dom_duptree(pcb_conf_main_root[CFR_DESIGN]->root);

	for(n = res->data.list.first; n != NULL; n = n->next) {
		for(s = del_paths; *s != NULL; s++) {
			lht_node_t *sub = lht_tree_path_(n->doc, n, *s, 0, 0, NULL);
			if (sub != NULL) {
				lht_tree_del(sub);
			}
		}
	}

	return res;
}

static lht_doc_t *build_board(pcb_board_t *pcb)
{
	char vers[32];
	lht_doc_t *brd = lht_dom_init();
	lht_node_t *ntmp;

	sprintf(vers, "pcb-rnd-board-v%d", wrver);
	brd->root = lht_dom_node_alloc(LHT_HASH, vers);
	lht_dom_hash_put(brd->root, build_board_meta(pcb));
	if (wrver >= 2) {
		lht_node_t *stack = build_layer_stack(pcb);
		if (stack == NULL)
			goto error;
		lht_dom_hash_put(brd->root, stack);
	}

	ntmp = build_data(pcb->Data);
	if (ntmp == NULL)
		goto error;

	lht_dom_hash_put(brd->root, ntmp);
	lht_dom_hash_put(brd->root, build_attributes(&pcb->Attributes));
	lht_dom_hash_put(brd->root, build_fontkit(&pcb->fontkit));
	lht_dom_hash_put(brd->root, build_styles(&pcb->RouteStyle));
	lht_dom_hash_put(brd->root, build_netlists(pcb, pcb->netlist, pcb->NetlistPatches, PCB_NUM_NETLISTS));
	lht_dom_hash_put(brd->root, build_conf());
	return brd;

	error:;
	lht_dom_uninit(brd);
	return NULL;
}

static lhtpers_ev_res_t check_text(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_node, const char *ondisk_value)
{
	/* for coords, preserve formatting as long as values match */
	if (lhtpers_rule_find(io_lihata_out_coords, inmem_node) != NULL) {
		pcb_coord_t v1, v2;
		pcb_bool success1, success2;

/*		fprintf(stderr, "SMART d='%s' m='%s'\n", ondisk_value, inmem_node->data.text.value);*/

		v1 = pcb_get_value_ex(ondisk_value, NULL, NULL, NULL, NULL, &success1);
		v2 = pcb_get_value_ex(inmem_node->data.text.value, NULL, NULL, NULL, NULL, &success2);
/*		pcb_fprintf(stderr, " %d %d | %mm %mm\n", success1, success2, v1, v2);*/
		if (success1 && success2) {
			/* smart: if values are the same, keep the on-disk version */
			if (v1 == v2)
				return LHTPERS_DISK;
			else
				return LHTPERS_MEM;
		}
		/* else fall back to the string compare below */
	}

	if (inmem_node->data.text.value == NULL)
		return LHTPERS_INHIBIT;

	/* classic method: string mismatch => overwrite from mem */
	if (strcmp(inmem_node->data.text.value, ondisk_value) != 0)
		return LHTPERS_MEM;

	return LHTPERS_DISK;
}

static void clean_invalid(lht_node_t *node)
{
	lht_node_t *n;
	lht_dom_iterator_t it;
	for(n = lht_dom_first(&it, node); n != NULL; n = lht_dom_next(&it)) {
		if (n->type == LHT_INVALID_TYPE)
			lht_tree_del(n);
		else
			clean_invalid(n);
	}
}

static int io_lihata_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency, int ver)
{
	int res;
	lht_doc_t *brd;

	wrver = ver;
	brd = build_board(PCB);

	if (brd == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to build the board at that version - nothing is written\n");
		return -1;
	}

#if IO_LIHATA_SAVE_BACKUP_IN_PCB
	{
		const char *fnpat = conf_io_lihata.plugins.io_lihata.aux_pcb_pattern;

		/* Backup in .pcb before saving in .lht so if the save code crashes,
		   the board is preserved. WARNING: since .pcb can't represent a lot
		   of details, it may degrade the board _before_ we save it in lht.
		   Thus this feature should be disabled */
		if ((fnpat != NULL) && (*fnpat != '\0')) {
			char *orig_fn, *end;
			char *pcb_fn = pcb_strdup_subst(fnpat, pcb_build_fn_cb, &PCB->hidlib, PCB_SUBST_ALL);
			
			orig_fn = PCB->hidlib.filename;
			PCB->hidlib.filename = NULL;

			/* avoid .lht.lht.pcb */
			end = pcb_fn + strlen(pcb_fn) - 1;
			if ((end-11 > pcb_fn) && (strcmp(end-11, ".lht.lht.pcb") == 0))
				strcpy(end-11, ".lht.pcb");

			fprintf(stderr, "NOTE: io_lihata_write_pcb will save to '%s' but first saves in '%s': res=%d (expected: 0)\n", new_filename, pcb_fn, pcb_save_pcb(pcb_fn, "pcb"));
			free(pcb_fn);
			
			/* restore these because SaveTo() has changed them */
			free(PCB->hidlib.filename);
			PCB->hidlib.filename = orig_fn;
			PCB->Data->loader = ctx;
		}
	}
#endif

	if ((emergency) || ((old_filename == NULL) && (new_filename == NULL))) {
		/* emergency or pipe save: use the canonical form */
		clean_invalid(brd->root); /* remove invalid nodes placed for persistency */
		res = lht_dom_export(brd->root, FP, "");
	}
	else {
		FILE *inf = NULL;
		char *errmsg;
		lhtpers_ev_t events;

		if (old_filename != NULL)
			inf = pcb_fopen(&PCB->hidlib, old_filename, "r");

		memset(&events, 0, sizeof(events));
		events.text = check_text;
/*		events.list_empty = check_list_empty;
		events.list_elem_removed = check_list_elem_removed;*/
		events.output_rules = io_lihata_out_rules;

		res = lhtpers_fsave_as(&events, brd, inf, FP, old_filename, &errmsg);
		if ((res == LHTPERS_ERR_ROOT_MISMATCH) || (res == LHTPERS_ERR_ROOT_MISSING)) {
			/* target is a different lht file or not even an lht file; the user requested the save, including overwrite, so do that */
			rewind(FP);
			res = lhtpers_fsave_as(&events, brd, NULL, FP, old_filename, &errmsg);
		}
		if (res != 0) {
			FILE *fe;
			char *fe_name = pcb_concat(old_filename, ".mem.lht", NULL);
			fe = pcb_fopen(&PCB->hidlib, fe_name, "w");
			if (fe != NULL) {
				clean_invalid(brd->root); /* remove invalid nodes placed for persistency */
				res = lht_dom_export(brd->root, fe, "");
				fclose(fe);
			}
			pcb_message(PCB_MSG_ERROR, "lhtpers_fsave_as() failed. Please include files %s and %s and %s in your bugreport\n", inf, old_filename, fe_name);
			pcb_message(PCB_MSG_ERROR, "in case this broke your file %s, please use the emergency save %s instead.\n", old_filename, fe_name);
		}
		fflush(FP);
		if (inf != NULL)
			fclose(inf);
	}

	lht_dom_uninit(brd);
	return res;
}

int io_lihata_write_pcb_v1(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 1);
}

int io_lihata_write_pcb_v2(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 2);
}

int io_lihata_write_pcb_v3(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 3);
}

int io_lihata_write_pcb_v4(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 4);
}

int io_lihata_write_pcb_v5(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 5);
}

int io_lihata_write_pcb_v6(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 6);
}

int io_lihata_write_font(pcb_plug_io_t *ctx, pcb_font_t *font, const char *Filename)
{
	FILE *f;
	int res;
	lht_doc_t *doc;


	f = pcb_fopen_askovr(&PCB->hidlib, Filename, "w", NULL);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to open font file %s for write\n", Filename);
		return -1;
	}

	/* create the doc */
	io_lihata_full_tree = 1;
	doc = lht_dom_init();
	doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-font-v1");
	lht_dom_list_append(doc->root, build_font(font));

	clean_invalid(doc->root);
	res = lht_dom_export(doc->root, f, "");

	fclose(f);
	lht_dom_uninit(doc);
	io_lihata_full_tree = 0;
	return res;
}

static int plug2ver(const pcb_plug_io_t *ctx)
{
	if (ctx == &plug_io_lihata_v1) return 1;
	if (ctx == &plug_io_lihata_v2) return 2;
	if (ctx == &plug_io_lihata_v3) return 3;
	if (ctx == &plug_io_lihata_v4) return 4;
	if (ctx == &plug_io_lihata_v5) return 5;
	if (ctx == &plug_io_lihata_v6) return 6;
	return 0;
}

static int io_lihata_dump_subc(pcb_plug_io_t *ctx, FILE *f, pcb_subc_t *sc)
{
	int res;
	lht_doc_t *doc;

	/* create the doc */
	io_lihata_full_tree = 1;
	doc = lht_dom_init();

	/* determine version */
	wrver = plug2ver(ctx);
	if (wrver < 3)
		wrver = 1;

	/* bump version if features require */
TODO("subc: for subc-in-subc this should be recursive")
	if (padstacklist_first(&sc->data->padstack) != NULL) {
		if (wrver < 4) {
			pcb_message(PCB_MSG_WARNING, "Had to bump lihata subc version to 4 because the subcircuit contains padstacks.\n");
			wrver = 4;
		}
	}

	if (wrver == 3)
		doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-subcircuit-v3");
	else if ((wrver >= 4) && (wrver < 6))
		doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-subcircuit-v4");
	else if (wrver >= 6)
		doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-subcircuit-v6");
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid lihata subc version to write: %d\n", wrver);
		return -1;
	}

	lht_dom_list_append(doc->root, build_subc(sc));

	clean_invalid(doc->root); /* remove invalid nodes placed for persistency */
	res = lht_dom_export(doc->root, f, "");

	lht_dom_uninit(doc);
	io_lihata_full_tree = 0;
	return res;
}


static int io_lihata_dump_nth_subc(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *data, int enforce1, long subc_idx)
{
	pcb_subc_t *sc;

	if ((subc_idx < 0) && (enforce1) && (pcb_subclist_length(&data->subc)) > 1) {
		pcb_message(PCB_MSG_ERROR, "Can't save more than one subcircuit from a buffer\n");
		return -1;
	}
	if (pcb_subclist_length(&data->subc) < 1) {
		pcb_message(PCB_MSG_ERROR, "there's no subcircuit in the buffer\n");
		return -1;
	}

	if (subc_idx == -1)
		sc = pcb_subclist_first(&data->subc);
	else
		sc = pcb_subclist_nth(&data->subc, subc_idx);

	if (sc == NULL)
		return -1;

	return io_lihata_dump_subc(ctx, f, sc);
}

int io_lihata_write_subcs_head(pcb_plug_io_t *ctx, void **udata, FILE *f, int lib, long num_subcs)
{
	if ((lib) || (num_subcs != 1)) {
		pcb_message(PCB_MSG_ERROR, "Only one subcircuit per footprint file can be written in lihata\n");
		return -1;
	}
	return 0;
}

int io_lihata_write_subcs_subc(pcb_plug_io_t *ctx, void **udata, FILE *f, pcb_subc_t *subc)
{
	return io_lihata_dump_subc(ctx, f, subc);
}

int io_lihata_write_subcs_tail(pcb_plug_io_t *ctx, void **udata, FILE *f)
{
	return 0;
}


int io_lihata_write_buffer(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff)
{
	int res;
	lht_doc_t *doc;

	wrver = plug2ver(ctx);
	if (wrver < 6)
		wrver = 6;

	io_lihata_full_tree = 1;
	doc = lht_dom_init();
	doc->root = lht_dom_node_alloc(LHT_HASH, "pcb-rnd-buffer-v6");

	lht_dom_hash_put(doc->root, build_data(buff->Data));
	lht_dom_hash_put(doc->root, build_textf("x", CFMT, buff->X));
	lht_dom_hash_put(doc->root, build_textf("y", CFMT, buff->Y));

	clean_invalid(doc->root); /* remove invalid nodes placed for persistency */
	res = lht_dom_export(doc->root, f, "");

	lht_dom_uninit(doc);
	io_lihata_full_tree = 0;
	return res;

}

int io_lihata_write_element(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *dt, long subc_idx)
{
	return io_lihata_dump_nth_subc(ctx, f, dt, 1, subc_idx);
}

typedef struct {
	int womit_font, womit_config, womit_styles;
	int ver;
} io_lihata_save_t;

void *io_lihata_save_as_subd_init(const pcb_plug_io_t *ctx, pcb_hid_dad_subdialog_t *sub, pcb_plug_iot_t type)
{
	io_lihata_save_t *save = calloc(sizeof(io_lihata_save_t), 1);

	if (type == PCB_IOT_PCB) {
		PCB_DAD_BEGIN_HBOX(sub->dlg);
			PCB_DAD_LABEL(sub->dlg, "Omit font");
				PCB_DAD_HELP(sub->dlg, "Do not save the font subtree\nWARNING: this will make the board depend on\nthe default font available on systems\nwhere it is loaded; if multiple fonts\nare used, all will be displayed using the default");
			PCB_DAD_BOOL(sub->dlg, "");
				PCB_DAD_DEFAULT_NUM(sub->dlg, !!conf_io_lihata.plugins.io_lihata.omit_font);
				save->womit_font = PCB_DAD_CURRENT(sub->dlg);
		PCB_DAD_END(sub->dlg);
		PCB_DAD_BEGIN_HBOX(sub->dlg);
			PCB_DAD_LABEL(sub->dlg, "Omit config");
				PCB_DAD_HELP(sub->dlg, "Do not save the config subtree\nWARNING: this will lose all DESIGN role\nconfig setting in the resulting file");
			PCB_DAD_BOOL(sub->dlg, "");
				PCB_DAD_DEFAULT_NUM(sub->dlg, !!conf_io_lihata.plugins.io_lihata.omit_config);
				save->womit_config = PCB_DAD_CURRENT(sub->dlg);
		PCB_DAD_END(sub->dlg);
		PCB_DAD_BEGIN_HBOX(sub->dlg);
			PCB_DAD_LABEL(sub->dlg, "Omit styles");
				PCB_DAD_HELP(sub->dlg, "Do not save the routing style subtree\nThe resulting file will have no\nrouting styles");
			PCB_DAD_BOOL(sub->dlg, "");
				PCB_DAD_DEFAULT_NUM(sub->dlg, !!conf_io_lihata.plugins.io_lihata.omit_styles);
				save->womit_styles = PCB_DAD_CURRENT(sub->dlg);
		PCB_DAD_END(sub->dlg);
	}
	return save;
}

void io_lihata_save_as_subd_uninit(const pcb_plug_io_t *ctx, void *plg_ctx, pcb_hid_dad_subdialog_t *sub, pcb_bool apply)
{
	io_lihata_save_t *save = plg_ctx;

	if (apply) {
		int omit_font = !!sub->dlg[save->womit_font].val.lng;
		int omit_config = !!sub->dlg[save->womit_config].val.lng;
		int omit_styles = !!sub->dlg[save->womit_styles].val.lng;

		if (omit_font != !!conf_io_lihata.plugins.io_lihata.omit_font)
			pcb_conf_setf(CFR_CLI, "plugins/io_lihata/omit_font", 0, "%d", omit_font);

		if (omit_config != !!conf_io_lihata.plugins.io_lihata.omit_config)
			pcb_conf_setf(CFR_CLI, "plugins/io_lihata/omit_config", 0, "%d", omit_config);

		if (omit_styles != !!conf_io_lihata.plugins.io_lihata.omit_styles)
			pcb_conf_setf(CFR_CLI, "plugins/io_lihata/omit_styles", 0, "%d", omit_styles);
	}

	free(save);
}

void io_lihata_save_as_fmt_changed(const pcb_plug_io_t *ctx, void *plg_ctx, pcb_hid_dad_subdialog_t *sub)
{
	io_lihata_save_t *save = plg_ctx;
	save->ver = plug2ver(ctx);
}

