/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017, 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Load a lihata document in-memory and walk the tree and build pcb native
   structs. A full dom load is used instead of the event parser so that
   symlinks and tree merges can be supported later. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <liblihata/tree.h>
#include <libminuid/libminuid.h>
#include "config.h"
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "flag_str.h"
#include "compat_misc.h"
#include "macro.h"
#include "error.h"
#include "misc_util.h"
#include "layer.h"
#include <genvector/vtp0.h>
#include "common.h"
#include "polygon.h"
#include "conf_core.h"
#include "obj_subc.h"
#include "pcb_minuid.h"
#include "thermal.h"
#include "io_lihata.h"
#include "safe_fs.h"
#include "plug_footprint.h"
#include "vtpadstack.h"
#include "obj_pstk_inlines.h"

#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/elem_rot.h"

#warning cleanup TODO: put these in a gloal load-context-struct
vtp0_t post_ids, post_thermal_old, post_thermal_heavy;
static int rdver;
unsigned long warned;

#warning padstack TODO #22: flags: old pins/pads had more flags (e.g. square)
#define PCB_OBJ_VIA PCB_OBJ_PSTK
#define PCB_OBJ_PIN PCB_OBJ_PSTK
#define PCB_OBJ_PAD PCB_OBJ_PSTK
#define PCB_OBJ_ELEMENT PCB_OBJ_SUBC

static pcb_data_t *parse_data(pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *nd, pcb_data_t *subc_parent);

static int iolht_error(lht_node_t *nd, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	gds_append_str(&str, "io_lihata parse error");
	if (nd != NULL)
		pcb_append_printf(&str, "at %s:%d.%d: ", nd->file_name, nd->line, nd->col);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

static void iolht_warn(lht_node_t *nd, int wbit, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	if (wbit >= 0) {
		unsigned long bv = 1ul << wbit;
		if (warned & bv)
			return;
		warned |= bv;
	}

	gds_init(&str);
	gds_append_str(&str, "io_lihata parse warning");
	if (nd != NULL)
		pcb_append_printf(&str, "at %s:%d.%d: ", nd->file_name, nd->line, nd->col);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	pcb_message(PCB_MSG_WARNING, "%s", str.array);

	gds_uninit(&str);
}

/* Collect objects that has unknown ID on a list. Once all objects with
   known-IDs are allocated, the unknonw-ID objects are allocated a fresh
   ID. This makes sure they don't occupy IDs that would be used by known-ID
   objects during the load. */
#define post_id_req(obj) vtp0_append(&post_ids, &((obj)->ID))

static int parse_attributes(pcb_attribute_list_t *list, lht_node_t *nd)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (nd == NULL)
		return 0;

	if (nd->type != LHT_HASH)
		return -1;

	for(n = lht_dom_first(&it, nd); n != NULL; n = lht_dom_next(&it)) {
		if (n->type == LHT_TEXT)
			pcb_attribute_put(list, n->name, n->data.text.value);
	}

	return 0;
}

/* Load the (duplicated) string value of a text node into res. Return 0 on success */
static int parse_text(char **res, lht_node_t *nd)
{
	if (nd == NULL)
		return -1;
	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "expected a text node\n");
	*res = pcb_strdup(nd->data.text.value);
	return 0;
}

/* Load a minuid from a text node into res. Return 0 on success */
static int parse_minuid(minuid_bin_t res, lht_node_t *nd)
{
	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return iolht_error(nd, "expected a text node for minuid\n");
	if (strlen(nd->data.text.value) != sizeof(minuid_str_t)-1)
		return iolht_error(nd, "invalid minuid: '%s'\n", nd->data.text.value);
	minuid_str2bin(res, (char *)nd->data.text.value);
	return 0;
}

/* Load the pcb_coord_t value of a text node into res. Return 0 on success */
static int parse_coord(pcb_coord_t *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;

	tmp = pcb_get_value_ex(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return iolht_error(nd, "Invalid coord value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the Angle value of a text node into res. Return 0 on success */
static int parse_angle(pcb_angle_t *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return iolht_error(nd, "Invalid angle type: '%d'\n", nd->type);

	tmp = pcb_get_value_ex(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return iolht_error(nd, "Invalid angle value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the integer value of a text node into res. Return 0 on success */
static int parse_int(int *res, lht_node_t *nd)
{
	long int tmp;
	int base = 10;
	char *end;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid integer node type (int): '%d'\n", nd->type);

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtol(nd->data.text.value, &end, base);
	if (*end != '\0')
		return iolht_error(nd, "Invalid integer value (not an integer number): '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}


/* Load the unsigned long value of a text node into res. Return 0 on success */
static int parse_ulong(unsigned long *res, lht_node_t *nd)
{
	unsigned long int tmp;
	int base = 10;
	char *end;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid integer node type (ulong): '%d'\n", nd->type);

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtoul(nd->data.text.value, &end, base);
	if (*end != '\0')
		return iolht_error(nd, "Invalid integer value (not an unsigned long integer): '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the duble value of a text node into res. Return 0 on success */
static int parse_double(double *res, lht_node_t *nd)
{
	double tmp;
	char *end;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT)
		return iolht_error(nd, "Invalid floating point number type: '%d'\n", nd->type);

	tmp = strtod(nd->data.text.value, &end);

	if (*end != '\0')
		return iolht_error(nd, "Invalid floating point value: '%s'\n", nd->data.text.value);

	*res = tmp;
	return 0;
}

/* Load the id name of a text node (with a prefixed name) into res.
   Return 0 on success */
static int parse_id(long int *res, lht_node_t *nd, int prefix_len)
{
	long int tmp;
	char *end;

	if (nd == NULL)
		return -1;

	tmp = strtol(nd->name + prefix_len, &end, 10);
	if (*end != '\0')
		return iolht_error(nd, "Invalid id value (must be a positive integer): '%s'\n", nd->data.text.value);

	pcb_create_ID_bump(tmp+1);

	*res = tmp;
	return 0;
}

/* Load the boolean value of a text node into res.
   Return 0 on success */
static int parse_bool(pcb_bool *res, lht_node_t *nd)
{
	char val[8], *end;

	if (nd == NULL)
		return -1;

	strncpy(val, nd->data.text.value, sizeof(val)-1);
	val[sizeof(val)-1] = '\0';
	end = strpbrk(val, " \t\r\n");
	if (end != NULL)
		*end = '\0';


	if ((strcmp(val, "1") == 0) || (pcb_strcasecmp(val, "on") == 0) ||
	    (pcb_strcasecmp(val, "true") == 0) || (pcb_strcasecmp(val, "yes") == 0)) {
		*res = 1;
		return 0;
	}

	if ((strcmp(val, "0") == 0) || (pcb_strcasecmp(val, "off") == 0) ||
	    (pcb_strcasecmp(val, "false") == 0) || (pcb_strcasecmp(val, "no") == 0)) {
		*res = 0;
		return 0;
	}

	return iolht_error(nd, "Invalid bool value: '%s'\n", nd->data.text.value);
}

static int parse_meta(pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *grp;

	if (nd->type != LHT_HASH)
		return iolht_error(nd, "board meta must be a hash\n");

	parse_text(&pcb->Name, lht_dom_hash_get(nd, "meta"));

	parse_text(&pcb->Name, lht_dom_hash_get(nd, "board_name"));

	grp = lht_dom_hash_get(nd, "grid");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		parse_coord(&pcb->GridOffsetX, lht_dom_hash_get(grp, "offs_x"));
		parse_coord(&pcb->GridOffsetY, lht_dom_hash_get(grp, "offs_y"));
		parse_coord(&pcb->Grid, lht_dom_hash_get(grp, "spacing"));
	}

	grp = lht_dom_hash_get(nd, "size");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		parse_coord(&pcb->MaxWidth, lht_dom_hash_get(grp, "x"));
		parse_coord(&pcb->MaxHeight, lht_dom_hash_get(grp, "y"));
		parse_double(&pcb->IsleArea, lht_dom_hash_get(grp, "isle_area_nm2"));
		parse_double(&pcb->ThermScale, lht_dom_hash_get(grp, "thermal_scale"));
	}

	grp = lht_dom_hash_get(nd, "drc");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		parse_coord(&pcb->Bloat, lht_dom_hash_get(grp, "bloat"));
		parse_coord(&pcb->Shrink, lht_dom_hash_get(grp, "shrink"));
		parse_coord(&pcb->minWid, lht_dom_hash_get(grp, "min_width"));
		parse_coord(&pcb->minSlk, lht_dom_hash_get(grp, "min_silk"));
		parse_coord(&pcb->minDrill, lht_dom_hash_get(grp, "min_drill"));
		parse_coord(&pcb->minRing, lht_dom_hash_get(grp, "min_ring"));
	}

	grp = lht_dom_hash_get(nd, "cursor");
	if ((grp != NULL) && (grp->type == LHT_HASH)) {
		parse_coord(&pcb->CursorX, lht_dom_hash_get(grp, "x"));
		parse_coord(&pcb->CursorY, lht_dom_hash_get(grp, "y"));
		parse_double(&pcb->Zoom, lht_dom_hash_get(grp, "zoom"));
	}

	return 0;
}

static int parse_thermal(unsigned char *dst, lht_node_t *src)
{
	if (src == NULL)
		return 0;

	if (src->type != LHT_LIST)
		return iolht_error(src, "thermals must be a list\n");

	*dst = 0;
	for(src = src->data.list.first; src != NULL; src = src->next)
		if (src->type == LHT_TEXT)
			*dst |= pcb_thermal_str2bits(src->data.text.value);

	return 0;
}

/* pt is a list of lihata node pointers to thermal nodes; each has user
   data set to the object. Look up layer info and build the thermal. This
   needs to be done in a separate pass at the end of parsing because
   vias may precede layers in the lihata input file. */
static int post_thermal_assign(pcb_board_t *pcb, vtp0_t *old, vtp0_t *heavy)
{
	int i;
	lht_node_t *n;
	lht_dom_iterator_t it;

	/* pin/via before lihata v4: thermal is part of the object flag*/
	for(i = 0; i < vtp0_len(old); i++) {
		lht_node_t *thr = old->array[i];
		pcb_pstk_t *ps = thr->user_data;

		assert(ps->type == PCB_OBJ_PSTK);

		ps->thermals.used = 0;

		for(n = lht_dom_first(&it, thr); n != NULL; n = lht_dom_next(&it)) {
			if (n->type == LHT_TEXT) {
				int layer = pcb_layer_by_name(pcb->Data, n->name) + 1;
				if (layer > ps->thermals.used)
					ps->thermals.used = layer;
			}
		}

		if (ps->thermals.used > 0) {
			ps->thermals.shape = calloc(sizeof(ps->thermals.shape[0]), ps->thermals.used);
			for(n = lht_dom_first(&it, thr); n != NULL; n = lht_dom_next(&it)) {
				if (n->type == LHT_TEXT) {
					int layer = pcb_layer_by_name(pcb->Data, n->name);
					if (layer < 0)
						return iolht_error(n, "Invalid layer name in thermal: '%s'\n", n->name);
					ps->thermals.shape[layer] = io_lihata_resolve_thermal_style_old(n->data.text.value);
				}
			}
		}
		else
			ps->thermals.shape = NULL;
	}
	vtp0_uninit(old);

	/* from lihata v4 up: thermal is an object property (on heavy terminal layer objects) */
	for(i = 0; i < vtp0_len(heavy); i++) {
		lht_node_t *thr = heavy->array[i];
		pcb_any_obj_t *obj = thr->user_data;

		/* single character thermal: only on the layer the object is on */
		parse_thermal(&obj->thermal, thr);
	}
	vtp0_uninit(heavy);

	return 0;
}

static int parse_flags(pcb_flag_t *f, lht_node_t *fn, int object_type, unsigned char *intconn, int can_have_thermal)
{
	io_lihata_flag_holder fh;

	memset(&fh, 0, sizeof(fh));

	if (fn != NULL) {
		int n;
		for (n = 0; n < pcb_object_flagbits_len; n++) {
			if (pcb_object_flagbits[n].object_types & object_type) {
				pcb_bool b;
				if ((parse_bool(&b, lht_dom_hash_get(fn, pcb_object_flagbits[n].name)) == 0) && b)
					PCB_FLAG_SET(pcb_object_flagbits[n].mask, &fh);
			}
		}

		if ((!can_have_thermal) && (lht_dom_hash_get(fn, "thermal") != NULL))
			iolht_error(fn, "Invalid flag thermal: object type can not have a thermal (ignored)\n");


		if (parse_int(&n, lht_dom_hash_get(fn, "shape")) == 0)
			fh.Flags.q = n;

		if ((intconn != NULL) && (rdver < 3))
			if (parse_int(&n, lht_dom_hash_get(fn, "intconn")) == 0)
				*intconn = n;
	}

	*f = fh.Flags;
	return 0;
}

static void parse_thermal_old(pcb_any_obj_t *obj, lht_node_t *fn)
{
	lht_node_t *thr;

	if (fn == NULL)
		return;

	thr = lht_dom_hash_get(fn, "thermal");
	if (thr == NULL)
		return;

	thr->user_data = obj;
	vtp0_append(&post_thermal_old, thr);
}

/* heavy terminal thermal: save for later processing */
static int parse_thermal_heavy(pcb_any_obj_t *obj, lht_node_t *src)
{
	if (src == NULL)
		return 0;

	if (src->type != LHT_LIST)
		return iolht_error(src, "heavy thermal must be a list\n");

	src->user_data = obj;
	vtp0_append(&post_thermal_heavy, src);

	return 0;
}

static int parse_line(pcb_layer_t *ly, lht_node_t *obj, int no_id, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_line_t *line;
	unsigned char intconn = 0;

	if (ly != NULL)
		line = pcb_line_alloc(ly);
	else
		return iolht_error(obj, "failed to allocate line object\n");

	if (no_id)
		line->ID = 0;
	else
		parse_id(&line->ID, obj, 5);
	parse_flags(&line->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_LINE, &intconn, 0);
	pcb_attrib_compat_set_intconn(&line->Attributes, intconn);
	parse_attributes(&line->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rdver >= 4)
		parse_thermal_heavy((pcb_any_obj_t *)line, lht_dom_hash_get(obj, "thermal"));

	parse_coord(&line->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&line->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&line->Point1.X, lht_dom_hash_get(obj, "x1"));
	parse_coord(&line->Point1.Y, lht_dom_hash_get(obj, "y1"));
	parse_coord(&line->Point2.X, lht_dom_hash_get(obj, "x2"));
	parse_coord(&line->Point2.Y, lht_dom_hash_get(obj, "y2"));

	line->Point1.X += dx;
	line->Point2.X += dx;
	line->Point1.Y += dy;
	line->Point2.Y += dy;

	if (!no_id) {
		post_id_req(&line->Point1);
		post_id_req(&line->Point2);
	}

	if (ly != NULL)
		pcb_add_line_on_layer(ly, line);

	return 0;
}

static int parse_rat(pcb_data_t *dt, lht_node_t *obj)
{
	pcb_rat_t rat, *new_rat;
	int tmp;

	parse_id(&rat.ID, obj, 4);
	parse_flags(&rat.Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_LINE, NULL, 0);
	parse_attributes(&rat.Attributes, lht_dom_hash_get(obj, "attributes"));

	parse_coord(&rat.Point1.X, lht_dom_hash_get(obj, "x1"));
	parse_coord(&rat.Point1.Y, lht_dom_hash_get(obj, "y1"));
	parse_coord(&rat.Point2.X, lht_dom_hash_get(obj, "x2"));
	parse_coord(&rat.Point2.Y, lht_dom_hash_get(obj, "y2"));

	parse_int(&tmp, lht_dom_hash_get(obj, "lgrp1"));
	rat.group1 = tmp;
	parse_int(&tmp, lht_dom_hash_get(obj, "lgrp2"));
	rat.group2 = tmp;


	new_rat = pcb_rat_new(dt, rat.Point1.X, rat.Point1.Y, rat.Point2.X, rat.Point2.Y, rat.group1, rat.group2,
		conf_core.appearance.rat_thickness, rat.Flags);

	post_id_req(&new_rat->Point1);
	post_id_req(&new_rat->Point2);

	new_rat->ID = rat.ID;

	return 0;
}

static int parse_arc(pcb_layer_t *ly, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_arc_t *arc;
	unsigned char intconn = 0;

	if (ly != NULL)
		arc = pcb_arc_alloc(ly);
	else
		return iolht_error(obj, "failed to allocate arc object\n");

	parse_id(&arc->ID, obj, 4);
	parse_flags(&arc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ARC, &intconn, 0);
	pcb_attrib_compat_set_intconn(&arc->Attributes, intconn);
	parse_attributes(&arc->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rdver >= 4)
		parse_thermal_heavy((pcb_any_obj_t *)arc, lht_dom_hash_get(obj, "thermal"));

	parse_coord(&arc->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&arc->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&arc->X, lht_dom_hash_get(obj, "x"));
	parse_coord(&arc->Y, lht_dom_hash_get(obj, "y"));
	parse_coord(&arc->Width, lht_dom_hash_get(obj, "width"));
	parse_coord(&arc->Height, lht_dom_hash_get(obj, "height"));
	parse_angle(&arc->StartAngle, lht_dom_hash_get(obj, "astart"));
	parse_angle(&arc->Delta, lht_dom_hash_get(obj, "adelta"));

	arc->X += dx;
	arc->Y += dy;

	if (ly != NULL)
		pcb_add_arc_on_layer(ly, arc);

	return 0;

}

static int parse_polygon(pcb_layer_t *ly, lht_node_t *obj)
{
	pcb_poly_t *poly = pcb_poly_alloc(ly);
	lht_node_t *geo;
	pcb_cardinal_t n = 0, c;
	unsigned char intconn = 0;

	parse_id(&poly->ID, obj, 8);
	parse_flags(&poly->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_POLY, &intconn, 0);
	pcb_attrib_compat_set_intconn(&poly->Attributes, intconn);
	parse_attributes(&poly->Attributes, lht_dom_hash_get(obj, "attributes"));

	if (rdver >= 3)
		parse_coord(&poly->Clearance, lht_dom_hash_get(obj, "clearance"));

	if (rdver >= 4)
		parse_thermal_heavy((pcb_any_obj_t *)poly, lht_dom_hash_get(obj, "thermal"));

	geo = lht_dom_hash_get(obj, "geometry");
	if ((geo != NULL) && (geo->type == LHT_LIST)) {
		lht_node_t *cnt;
		lht_dom_iterator_t it;

		/* count points and holes */
		poly->PointN = 0;
		for(c = 0, cnt = lht_dom_first(&it, geo); cnt != NULL; c++, cnt = lht_dom_next(&it)) {
			if (cnt->type != LHT_TABLE)
				continue;
			poly->PointN += cnt->data.table.rows;
		}
		poly->PointMax = poly->PointN;
		poly->Points = malloc(sizeof(pcb_point_t) * poly->PointMax);
		poly->HoleIndexMax = poly->HoleIndexN = c-1;
		if (poly->HoleIndexN > 0)
			poly->HoleIndex = malloc(sizeof(pcb_cardinal_t) * poly->HoleIndexMax);
		else
			poly->HoleIndex = NULL;

		/* convert points and build hole index */
		for(c = 0, cnt = lht_dom_first(&it, geo); cnt != NULL; c++, cnt = lht_dom_next(&it)) {
			pcb_cardinal_t r;
			if (cnt->type != LHT_TABLE)
				continue;
			if (c > 0)
				poly->HoleIndex[c-1] = n;
			for(r = 0; r < cnt->data.table.rows; r++) {
				parse_coord(&poly->Points[n].X, cnt->data.table.r[r][0]);
				parse_coord(&poly->Points[n].Y, cnt->data.table.r[r][1]);
				/* a point also needs to be a box on paper, but the poly code sets X2 and Y2 to 0 in reality... */
				poly->Points[n].X2 = poly->Points[n].Y2 = 0;
				post_id_req(&poly->Points[n]);
				n++;
			}
		}
	}

	pcb_add_poly_on_layer(ly, poly);
	pcb_poly_init_clip(ly->parent.data, ly, poly);

	return 0;
}

static int parse_pcb_text(pcb_layer_t *ly, lht_node_t *obj)
{
	pcb_text_t *text;
	lht_node_t *role;
	int tmp;
	unsigned char intconn = 0;

	role = lht_dom_hash_get(obj, "role");

	if (ly != NULL) {
		if (role != NULL)
			return iolht_error(obj, "invalid role: text on layer shall not have a role\n");
		text = pcb_text_alloc(ly);
		if (text == NULL)
			return iolht_error(obj, "failed to allocate yrcy object\n");
	}

	parse_id(&text->ID, obj, 5);

	parse_flags(&text->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_TEXT, &intconn, 0);
	pcb_attrib_compat_set_intconn(&text->Attributes, intconn);
	parse_attributes(&text->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_int(&text->Scale, lht_dom_hash_get(obj, "scale"));
	parse_int(&tmp, lht_dom_hash_get(obj, "fid"));
	text->fid = tmp;
	parse_int(&tmp, lht_dom_hash_get(obj, "direction"));
	text->Direction = tmp;
	parse_coord(&text->X, lht_dom_hash_get(obj, "x"));
	parse_coord(&text->Y, lht_dom_hash_get(obj, "y"));
	parse_text(&text->TextString, lht_dom_hash_get(obj, "string"));

#warning TODO: get the font
	if (ly != NULL)
		pcb_add_text_on_layer(ly, text, pcb_font(PCB, text->fid, 1));

	return 0;
}

static int parse_layer_type(pcb_layer_type_t *dst, lht_node_t *nd, const char *loc)
{
	lht_node_t *flg;
	lht_dom_iterator_t itt;

	if (nd == NULL)
		return -1;

	for(flg = lht_dom_first(&itt, nd); flg != NULL; flg = lht_dom_next(&itt)) {
		pcb_layer_type_t val = pcb_layer_type_str2bit(flg->name);
		if (val == 0)
			iolht_error(flg, "Invalid type name: '%s' in %s (ignoring the type flag)\n", flg->name, loc);
		*dst |= val;
	}

	return 0;
}

static pcb_layer_combining_t parse_comb(pcb_board_t *pcb, lht_node_t *ncmb)
{
	lht_node_t *n;
	pcb_layer_combining_t comb = 0;
	lht_dom_iterator_t it;

	for(n = lht_dom_first(&it, ncmb); n != NULL; n = lht_dom_next(&it)) {
		pcb_layer_combining_t cval;
		if (n->type != LHT_TEXT) {
			iolht_error(n, "Ignoring non-text combining flag\n");
			continue;
		}
		cval = pcb_layer_comb_str2bit(n->name);
		if (cval == 0)
			iolht_error(n, "Ignoring unknown combining flag: '%s'\n", n->name);
		comb |= cval;
	}

	return comb;
}

static int parse_data_layer(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp, int layer_id, pcb_data_t *subc_parent)
{
	lht_node_t *n, *lst, *ncmb;
	lht_dom_iterator_t it;
	int bound = (subc_parent != NULL);

	pcb_layer_t *ly = &dt->Layer[layer_id];
	if (layer_id >= dt->LayerN)
		dt->LayerN = layer_id+1;

	ly->parent.data = dt;
	ly->parent_type = PCB_PARENT_DATA;
	ly->type = PCB_OBJ_LAYER;

	parse_attributes(&ly->Attributes, lht_dom_hash_get(grp, "attributes"));

	ncmb = lht_dom_hash_get(grp, "combining");
	if (ncmb != NULL) {
		if (rdver < 2)
			iolht_warn(ncmb, 0, "Version 1 lihata board should not have combining subtree for layers\n");
		ly->comb = parse_comb(pcb, ncmb);
	}

	if (bound) {
		ly->is_bound = 1;
		ly->name = pcb_strdup(grp->name);
		parse_int(&dt->Layer[layer_id].meta.bound.stack_offs, lht_dom_hash_get(grp, "stack_offs"));
		parse_layer_type(&dt->Layer[layer_id].meta.bound.type, lht_dom_hash_get(grp, "type"), "bound layer");
		if (pcb != NULL) {
			dt->Layer[layer_id].meta.bound.real = pcb_layer_resolve_binding(pcb, &dt->Layer[layer_id]);
			if (dt->Layer[layer_id].meta.bound.real != NULL)
				pcb_layer_link_trees(&dt->Layer[layer_id], dt->Layer[layer_id].meta.bound.real);
			else if (!(dt->Layer[layer_id].meta.bound.type & PCB_LYT_VIRTUAL))
				iolht_warn(ncmb, 1, "Can't bind subcircuit layer %s: can't find anything similar on the current board\n", dt->Layer[layer_id].name);
			dt->padstack_tree = subc_parent->padstack_tree;
		}
	}
	else {
		/* real */
	ly->name = pcb_strdup(grp->name);
		parse_bool(&ly->meta.real.vis, lht_dom_hash_get(grp, "visible"));
		if (pcb != NULL) {
			int grp_id;
			parse_int(&grp_id, lht_dom_hash_get(grp, "group"));
			dt->Layer[layer_id].meta.real.grp = grp_id;
	/*		pcb_trace("parse_data_layer name: %d,%d '%s' grp=%d\n", layer_id, dt->LayerN-1, ly->name, grp_id);*/
		}
	}

	lst = lht_dom_hash_get(grp, "objects");
	if (lst != NULL) {
		if (lst->type != LHT_LIST)
			return iolht_error(lst, "objects must be in a list\n");

		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(ly, n, 0, 0, 0);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(ly, n, 0, 0);
			if (strncmp(n->name, "polygon.", 8) == 0)
				parse_polygon(ly, n);
			if (strncmp(n->name, "text.", 5) == 0)
				parse_pcb_text(ly, n);
		}
	}

	return 0;
}

static int parse_data_layers(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp, pcb_data_t *subc_parent)
{
	int id;
	lht_node_t *n;
	lht_dom_iterator_t it;

	for(id = 0, n = lht_dom_first(&it, grp); n != NULL; id++, n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			parse_data_layer(pcb, dt, n, id, subc_parent);

	return 0;
}

static int parse_pstk(pcb_data_t *dt, lht_node_t *obj)
{
	pcb_pstk_t *ps;
	lht_node_t *thl, *t;
	unsigned char intconn = 0;
	unsigned long int pid;
	int tmp;

	parse_ulong(&pid, lht_dom_hash_get(obj, "proto"));
	if (pcb_pstk_get_proto_(dt, pid) == NULL)
		return iolht_error(obj, "Padstack references to non-existent prototype\n");

	ps = pcb_pstk_alloc(dt);

	parse_id(&ps->ID, obj, 13);
	parse_flags(&ps->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_PSTK, &intconn, 0);
	pcb_attrib_compat_set_intconn(&ps->Attributes, intconn);
	parse_attributes(&ps->Attributes, lht_dom_hash_get(obj, "attributes"));

	parse_coord(&ps->x, lht_dom_hash_get(obj, "x"));
	parse_coord(&ps->y, lht_dom_hash_get(obj, "y"));
	parse_double(&ps->rot, lht_dom_hash_get(obj, "rot"));
	tmp = 0;
	parse_int(&tmp, lht_dom_hash_get(obj, "xmirror"));
	ps->xmirror = tmp;
	tmp = 0;
	parse_int(&tmp, lht_dom_hash_get(obj, "smirror"));
	ps->smirror = tmp;
	parse_coord(&ps->Clearance, lht_dom_hash_get(obj, "clearance"));
	ps->proto = pid;

	thl = lht_dom_hash_get(obj, "thermal");
	if ((thl != NULL) && (thl->type == LHT_LIST)) {
		int max, n;
		max = 0;
		for(t = thl->data.list.first, n = 0; t != NULL; t = t->next,n++)
		{
			if (t->type == LHT_LIST) {
				char *end;
				int ly = strtol(t->name, &end, 10);
				if ((*end == '\0') && (ly > max))
					max = ly;
			}
		}
		if (n > max)
			max = n;

		ps->thermals.used = max+1;
		ps->thermals.shape = calloc(sizeof(ps->thermals.shape[0]), ps->thermals.used);
		for(t = thl->data.list.first, n = 0; t != NULL; t = t->next, n++) {
			int i;
			if (t->type == LHT_TEXT) {
				parse_int(&i, t);
				ps->thermals.shape[n] = i;
			}
			else if (t->type == LHT_LIST) {
				unsigned char dst;
				char *end;
				int ly = strtol(t->name, &end, 10);
				if ((*end == '\0') && (ly < ps->thermals.used)) {
					parse_thermal(&dst, t);
					ps->thermals.shape[ly] = dst;
				}
			}
		}
	}

	pcb_pstk_add(dt, ps);

	return 0;
}

static int parse_via(pcb_data_t *dt, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy, int subc_on_bottom)
{
	pcb_pstk_t *ps;
	unsigned char intconn = 0;
	pcb_coord_t Thickness, Clearance, Mask, DrillingHole, X, Y;
	char *Name = NULL, *Number = NULL;
	pcb_flag_t flg;
	lht_node_t *fln;

	if (dt == NULL)
		return -1;

	parse_flags(&flg, fln=lht_dom_hash_get(obj, "flags"), PCB_OBJ_VIA, &intconn, 1);
	parse_coord(&Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&Mask, lht_dom_hash_get(obj, "mask"));
	parse_coord(&DrillingHole, lht_dom_hash_get(obj, "hole"));
	parse_coord(&X, lht_dom_hash_get(obj, "x"));
	parse_coord(&Y, lht_dom_hash_get(obj, "y"));
	parse_text(&Name, lht_dom_hash_get(obj, "name"));
	parse_text(&Number, lht_dom_hash_get(obj, "number"));

	ps = pcb_old_via_new(dt, X+dx, Y+dy, Thickness, Clearance, Mask, DrillingHole, Name, flg);
	if (ps == NULL) {
		iolht_error(obj, "Failed to convert old via to padstack (this via is LOST)\n");
		return 0;
	}

	parse_id(&ps->ID, obj, 4);
	pcb_attrib_compat_set_intconn(&ps->Attributes, intconn);
	parse_attributes(&ps->Attributes, lht_dom_hash_get(obj, "attributes"));

	parse_thermal_old((pcb_any_obj_t *)ps, fln);

	if (Number != NULL)
		pcb_attribute_put(&ps->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&ps->Attributes, "name", Name);

	if (subc_on_bottom)
		pcb_pstk_mirror(ps, 0, 1);

	return 0;
}

static int parse_pad(pcb_subc_t *subc, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy, int subc_on_bottom)
{
	pcb_pstk_t *p;
	unsigned char intconn = 0;
	pcb_flag_t flg;
	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance, Mask;
	char *Name = NULL, *Number = NULL;

	parse_flags(&flg, lht_dom_hash_get(obj, "flags"), PCB_OBJ_PAD, &intconn, 0);

	parse_coord(&Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&Mask, lht_dom_hash_get(obj, "mask"));
	parse_coord(&X1, lht_dom_hash_get(obj, "x1"));
	parse_coord(&Y1, lht_dom_hash_get(obj, "y1"));
	parse_coord(&X2, lht_dom_hash_get(obj, "x2"));
	parse_coord(&Y2, lht_dom_hash_get(obj, "y2"));
	parse_text(&Name, lht_dom_hash_get(obj, "name"));
	parse_text(&Number, lht_dom_hash_get(obj, "number"));

	p = pcb_pstk_new_compat_pad(subc->data, X1+dx, Y1+dy, X2+dx, Y2+dy, Thickness, Clearance, Mask, flg.f & PCB_FLAG_SQUARE, flg.f & PCB_FLAG_NOPASTE, (!!(flg.f & PCB_FLAG_ONSOLDER)) != subc_on_bottom);
	if (Number != NULL)
		pcb_attribute_put(&p->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	if (subc_on_bottom)
		pcb_pstk_mirror(p, 0, 1);

	parse_id(&p->ID, obj, 4);
	pcb_attrib_compat_set_intconn(&p->Attributes, intconn);
	parse_attributes(&p->Attributes, lht_dom_hash_get(obj, "attributes"));

	return 0;
}


static int parse_element(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj)
{
	pcb_subc_t *subc = pcb_subc_alloc();
	pcb_layer_t *silk = NULL;
	lht_node_t *lst, *n;
	lht_dom_iterator_t it;
	int onsld, tdir = 0, tscale = 100;
	pcb_coord_t ox = 0, oy = 0, tx, ty;
	pcb_text_t *txt;

	pcb_add_subc_to_data(dt, subc);

	parse_id(&subc->ID, obj, 8);
	parse_flags(&subc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ELEMENT, NULL, 0);
	parse_attributes(&subc->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_coord(&ox, lht_dom_hash_get(obj, "x"));
	parse_coord(&oy, lht_dom_hash_get(obj, "y"));
	tx = ox;
	ty = oy;

	onsld = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, subc);
	subc->Flags.f &= ~PCB_FLAG_ONSOLDER;

	/* bind the via rtree so that vias added in this subc show up on the board */
	if (pcb != NULL)
		pcb_subc_bind_globals(pcb, subc);

	/* the only layer objects are put on from and old element is the primary silk layer */
	{
		pcb_layer_type_t silk_side = onsld ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
		const char *name = onsld ? "bottom-silk" : "top-silk";
		silk = pcb_subc_get_layer(subc, PCB_LYT_SILK | silk_side, /*PCB_LYC_AUTO*/0, pcb_true, name, pcb_false);
	}

	lst = lht_dom_hash_get(obj, "objects");
	if (lst->type == LHT_LIST) {
		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(silk, n, 0, ox, oy);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(silk, n, ox, oy);
			if (strncmp(n->name, "text.", 5) == 0) {
				lht_node_t *role   = lht_dom_hash_get(n, "role");
				lht_node_t *string = lht_dom_hash_get(n, "string");
				if ((role != NULL) && (role->type == LHT_TEXT) && (string != NULL) && (string->type == LHT_TEXT)) {
					const char *key = role->data.text.value;
					const char *val = string->data.text.value;
					if (strcmp(key, "desc") == 0)   pcb_attribute_put(&subc->Attributes, "footprint", val);
					if (strcmp(key, "value") == 0)  pcb_attribute_put(&subc->Attributes, "value", val);
					if (strcmp(key, "name") == 0)   pcb_attribute_put(&subc->Attributes, "refdes", val);
					parse_coord(&tx, lht_dom_hash_get(n, "x"));
					parse_coord(&ty, lht_dom_hash_get(n, "y"));
					parse_coord(&tdir, lht_dom_hash_get(n, "direction"));
					parse_coord(&tscale, lht_dom_hash_get(n, "scale"));
				}
			}
			if (strncmp(n->name, "pin.", 4) == 0)
				parse_via(subc->data, n, ox, oy, onsld);
			if (strncmp(n->name, "pad.", 4) == 0)
				parse_pad(subc, n, ox, oy, onsld);
		}
	}

#warning subc TODO: TextFlags
	txt = pcb_subc_add_refdes_text(subc, tx, ty, tdir, tscale, onsld);

	pcb_subc_xy_rot_pnp(subc, ox, oy, onsld);

	pcb_subc_bbox(subc);

	if (dt->subc_tree == NULL)
		dt->subc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dt->subc_tree, (pcb_box_t *)subc);

	pcb_subc_rebind(pcb, subc);

	return 0;
}

static int parse_subc(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj, pcb_subc_t **subc_out)
{
	pcb_subc_t *sc = pcb_subc_alloc();
	unsigned char intconn = 0;
	int n;

	parse_id(&sc->ID, obj, 5);
	parse_flags(&sc->Flags, lht_dom_hash_get(obj, "flags"), PCB_OBJ_ELEMENT, &intconn, 0);
	pcb_attrib_compat_set_intconn(&sc->Attributes, intconn);
	parse_attributes(&sc->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_minuid(sc->uid, lht_dom_hash_get(obj, "uid"));

	if (!dt->padstack_tree)
		dt->padstack_tree = pcb_r_create_tree();
	sc->data->padstack_tree = dt->padstack_tree;

	pcb_add_subc_to_data(dt, sc);

	if (parse_data(pcb, sc->data, lht_dom_hash_get(obj, "data"), dt) == 0)
		return iolht_error(obj, "Invalid subc: no data\n");

	for(n = 0; n < sc->data->LayerN; n++)
		sc->data->Layer[n].is_bound = 1;

	pcb_data_bbox(&sc->BoundingBox, sc->data, pcb_true);

	if (!dt->subc_tree)
		dt->subc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dt->subc_tree, (pcb_box_t *)sc);

	if (subc_out != NULL)
		*subc_out = sc;

	return 0;
}


static int parse_data_objects(pcb_board_t *pcb_for_font, pcb_data_t *dt, lht_node_t *grp)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (grp->type != LHT_LIST)
		return iolht_error(grp, "groups must be a list\n");

	for(n = lht_dom_first(&it, grp); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "padstack_ref.", 13) == 0)
			parse_pstk(dt, n);
		if (strncmp(n->name, "via.", 4) == 0)
			parse_via(dt, n, 0, 0, 0);
		if (strncmp(n->name, "rat.", 4) == 0)
			parse_rat(dt, n);
		else if (strncmp(n->name, "element.", 8) == 0)
			parse_element(pcb_for_font, dt, n);
		else if (strncmp(n->name, "subc.", 5) == 0)
			if (parse_subc(pcb_for_font, dt, n, NULL) != 0)
				return iolht_error(n, "failed to parse subcircuit\n");
	}

	return 0;
}

static void layer_fixup(pcb_board_t *pcb)
{
	int n;
	pcb_layergrp_id_t top_silk, bottom_silk;
	pcb_layergrp_t *g;

	pcb_layergrp_inhibit_inc();

	pcb_layer_group_setup_default(pcb);

	/* old silk assumption: last two layers are silk, bottom and top */
	bottom_silk = pcb->Data->Layer[pcb->Data->LayerN-2].meta.real.grp;
	top_silk = pcb->Data->Layer[pcb->Data->LayerN-1].meta.real.grp;
	pcb->Data->Layer[pcb->Data->LayerN-2].meta.real.grp = -1;
	pcb->Data->Layer[pcb->Data->LayerN-1].meta.real.grp = -1;

/*	pcb_trace("NAME: '%s' '%s'\n", pcb->Data->Layer[pcb->Data->LayerN-1].Name,pcb->Data->Layer[pcb->Data->LayerN-2].Name);*/

	for(n = 0; n < pcb->Data->LayerN - 2; n++) {
		pcb_layer_t *l = &pcb->Data->Layer[n];
		pcb_layergrp_id_t grp = l->meta.real.grp;
		/*pcb_trace("********* l=%d %s g=%ld (top=%ld bottom=%ld)\n", n, l->name, grp, top_silk, bottom_silk);*/
		l->meta.real.grp = -1;

		if (grp == bottom_silk)
			g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_COPPER);
		else if (grp == top_silk)
			g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_TOP, PCB_LYT_COPPER);
		else
			g = pcb_get_grp_new_intern(pcb, grp);
/*			pcb_trace(" add %ld\n", g - pcb->LayerGroups.meta.real.grp);*/
		if (g != NULL) {
			pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, n);
			if (strcmp(l->name, "outline") == 0)
				pcb_layergrp_fix_turn_to_outline(g);
		}
		else
			pcb_message(PCB_MSG_ERROR, "failed to create layer %s\n", l->name);
	}

	pcb_layergrp_fix_old_outline(pcb);

	/* link in the 2 hardwired silks and mark them auto */
	g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, pcb->Data->LayerN-2);
	pcb->Data->Layer[pcb->Data->LayerN-2].comb |= PCB_LYC_AUTO;
	
	g = pcb_get_grp(&pcb->LayerGroups, PCB_LYT_TOP, PCB_LYT_SILK);
	pcb_layer_add_in_group_(pcb, g, g - pcb->LayerGroups.grp, pcb->Data->LayerN-1);
	pcb->Data->Layer[pcb->Data->LayerN-1].comb |= PCB_LYC_AUTO;

	pcb_layergrp_inhibit_dec();
}

static int parse_layer_stack(pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *grps, *grp, *name, *layers, *lyr;
	lht_dom_iterator_t it, itt;
	long int n;

	for(n = 0; n < PCB_MAX_LAYERGRP; n++)
		pcb_layergrp_free(pcb, n);
	pcb->LayerGroups.len = 0;

	grps = lht_dom_hash_get(nd, "groups");
	for(grp = lht_dom_first(&it, grps); grp != NULL; grp = lht_dom_next(&it)) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_t *g;
		char *end;
		gid = strtol(grp->name, &end, 10);
		if ((*end != '\0') || (gid < 0)) {
			iolht_error(grp, "Invalid group id in layer stack: '%s' (not an int or negative; ignoring the group)\n", grp->name);
			continue;
		}
		if (gid >= PCB_MAX_LAYERGRP) {
			iolht_error(grp, "Invalid group id in layer stack: '%s' (too many layers; ignoring the group)\n", grp->name);
			continue;
		}

		pcb_layergrp_free(pcb, n); /* just in case of double initialization of the same layer id */

		g = &pcb->LayerGroups.grp[gid];
		if (pcb->LayerGroups.len <= gid)
			pcb->LayerGroups.len = gid+1;
		g->parent.board = pcb;
		g->parent_type = PCB_PARENT_BOARD;
		g->type = PCB_OBJ_LAYERGRP;
		g->valid = 1;

		/* set name and type*/
		name = lht_dom_hash_get(grp, "name");
		if ((name == NULL) || (name->type != LHT_TEXT) || (name->data.text.value == NULL)) {
			g->name = malloc(32);
			sprintf(g->name, "grp_%ld", gid);
		}
		else
			g->name = pcb_strdup(name->data.text.value);
		parse_layer_type(&g->ltype, lht_dom_hash_get(grp, "type"), g->name);

		/* load layers */
		layers = lht_dom_hash_get(grp, "layers");
		if (layers != NULL) {
			for(lyr = lht_dom_first(&itt, layers); lyr != NULL; lyr = lht_dom_next(&itt)) {
				pcb_layer_id_t lid;
				if (lyr->type != LHT_TEXT) {
					iolht_error(lyr, "Invalid layer node type in group '%s' (ignoring the layer)\n", g->name);
					continue;
				}
				lid = strtol(lyr->data.text.value, &end, 10);
				if ((*end != '\0') || (lid < 0)) {
					iolht_error(lyr, "Invalid layer id '%s' in group '%s' (not an int or negative; ignoring the layer)\n", lyr->data.text.value, g->name);
					continue;
				}
				if (g->len >= PCB_MAX_LAYER) {
					iolht_error(lyr, "Too many layers  in group '%s' (ignoring the layer)\n", g->name);
					continue;
				}
				g->lid[g->len] = lid;
				g->len++;
			}
		}
	}
	return 0;
}

static int parse_data_pstk_shape_poly(pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	lht_node_t *n;
	pcb_cardinal_t i;

	dst->shape = PCB_PSSH_POLY;
	dst->data.poly.x = NULL; /* if we return before the allocation... */
	dst->data.poly.len = 0;
	for(n = nshape->data.list.first; n != NULL; n = n->next)
		dst->data.poly.len++;

	if ((dst->data.poly.len % 2) != 0)
		return iolht_error(n, "odd number of padstack shape polygon points\n");

	dst->data.poly.len /= 2;

	pcb_pstk_shape_alloc_poly(&dst->data.poly, dst->data.poly.len);
	for(n = nshape->data.list.first, i = 0; n != NULL; i++) {
		if (parse_coord(&dst->data.poly.x[i], n) != 0) return -1;
		n = n->next;
		if (parse_coord(&dst->data.poly.y[i], n) != 0) return -1;
		n = n->next;
	}
	return 0;
}

static int parse_data_pstk_shape_line(pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	int sq;

	dst->shape = PCB_PSSH_LINE;

	if (parse_coord(&dst->data.line.x1, lht_dom_hash_get(nshape, "x1")) != 0) return -1;
	if (parse_coord(&dst->data.line.y1, lht_dom_hash_get(nshape, "y1")) != 0) return -1;
	if (parse_coord(&dst->data.line.x2, lht_dom_hash_get(nshape, "x2")) != 0) return -1;
	if (parse_coord(&dst->data.line.y2, lht_dom_hash_get(nshape, "y2")) != 0) return -1;
	if (parse_coord(&dst->data.line.thickness, lht_dom_hash_get(nshape, "thickness")) != 0) return -1;
	if (parse_int(&sq, lht_dom_hash_get(nshape, "square")) != 0) return -1;
	dst->data.line.square = sq;
	return 0;
}

static int parse_data_pstk_shape_circ(pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	dst->shape = PCB_PSSH_CIRC;

	if (parse_coord(&dst->data.circ.x, lht_dom_hash_get(nshape, "x")) != 0) return -1;
	if (parse_coord(&dst->data.circ.y, lht_dom_hash_get(nshape, "y")) != 0) return -1;
	if (parse_coord(&dst->data.circ.dia, lht_dom_hash_get(nshape, "dia")) != 0) return -1;
	return 0;
}

static int parse_data_pstk_shape_v4(pcb_board_t *pcb, pcb_pstk_shape_t *dst, lht_node_t *nshape, pcb_data_t *subc_parent)
{
	lht_node_t *ncmb, *nlyt, *ns;
	int res = -1;

	nlyt = lht_dom_hash_get(nshape, "layer_mask");
	if ((nlyt != NULL) && (nlyt->type == LHT_HASH))
		res = parse_layer_type(&dst->layer_mask, nlyt, "padstack shape");

	if (res != 0)
		return iolht_error(nlyt != NULL ? nlyt : nshape, "Failed to parse pad stack shape (layer mask)\n");

	ncmb = lht_dom_hash_get(nshape, "combining");
	if ((ncmb != NULL) && (ncmb->type == LHT_HASH))
		dst->comb = parse_comb(pcb, ncmb);

	if (parse_coord(&dst->clearance, lht_dom_hash_get(nshape, "clearance")) != 0) return -1;

	ns = lht_dom_hash_get(nshape, "ps_poly");
	if ((ns != NULL) && (ns->type == LHT_LIST)) return parse_data_pstk_shape_poly(pcb, dst, ns, subc_parent);

	ns = lht_dom_hash_get(nshape, "ps_line");
	if ((ns != NULL) && (ns->type == LHT_HASH)) return parse_data_pstk_shape_line(pcb, dst, ns, subc_parent);

	ns = lht_dom_hash_get(nshape, "ps_circ");
	if ((ns != NULL) && (ns->type == LHT_HASH)) return parse_data_pstk_shape_circ(pcb, dst, ns, subc_parent);

	return iolht_error(nshape, "Failed to parse pad stack: missing shape\n");
}


static int parse_data_pstk_proto(pcb_board_t *pcb, pcb_pstk_proto_t *dst, lht_node_t *nproto, pcb_data_t *subc_parent)
{
	int itmp, i;
	lht_node_t *nshape, *n;
	pcb_pstk_tshape_t *ts;

	/* read the hole */
	if (parse_coord(&dst->hdia, lht_dom_hash_get(nproto, "hdia")) != 0) return -1;
	if (parse_int(&dst->htop, lht_dom_hash_get(nproto, "htop")) != 0) return -1;
	if (parse_int(&dst->hbottom, lht_dom_hash_get(nproto, "hbottom")) != 0) return -1;
	if (parse_int(&itmp, lht_dom_hash_get(nproto, "hplated")) != 0) return -1;
	dst->hplated = itmp;
	dst->in_use = 1;
	if (subc_parent != NULL)
		dst->parent = subc_parent;
	else
		dst->parent = pcb->Data;

	/* read shapes */
	nshape = lht_dom_hash_get(nproto, "shape");
	if ((nshape == NULL) || (nshape->type != LHT_LIST))
		return iolht_error(nshape, "shape must be an existing list\n");

	ts = pcb_vtpadstack_tshape_get(&dst->tr, 0, 1);

	for(n = nshape->data.list.first, ts->len = 0; n != NULL; n = n->next) ts->len++;
	ts->shape = calloc(sizeof(pcb_pstk_shape_t), ts->len);

	for(n = nshape->data.list.first, i = 0; n != NULL; n = n->next, i++)
		if ((n->type == LHT_HASH) && (strcmp(n->name, "ps_shape_v4") == 0))
			if (parse_data_pstk_shape_v4(pcb, ts->shape+i, n, subc_parent) != 0)
				goto error;

	return 0;
	error:;
	free(ts->shape);
	ts->shape = NULL;
	ts->len = 0;
	return iolht_error(n, "failed to parse padstack due to bad shape\n");
}

static int parse_data_pstk_protos(pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *pp, pcb_data_t *subc_parent)
{
	pcb_cardinal_t pid, len;
	lht_node_t *pr;
	int res = 0;

	for(len = 0, pr = pp->data.list.first; pr != NULL; pr = pr->next) len++;

	if (len == 0)
		return 0; /* there are no prototypes to load */

	pcb_vtpadstack_proto_enlarge(&dst->ps_protos, len-1);
	for(pid = 0, pr = pp->data.list.first; ((pr != NULL) && (res == 0)); pr = pr->next, pid++) {
		if ((pr->type == LHT_TEXT) && (strcmp(pr->name, "unused") == 0))
			continue;
		else if ((pr->type == LHT_HASH) && (strncmp(pr->name, "ps_proto_v4", 11) == 0)) {
			char *sid = pr->name+11, *end;
			long int pid_in_file;
			if (*sid == '.') {
				sid++;
				pid_in_file = strtol(sid, &end, 0);
				if (*end != '\0')
					return iolht_error(pr, "Invalid padstack proto ID '%s' (not an integer)\n", sid);
				else if (pid_in_file < pid)
					return iolht_error(pr, "Invalid padstack proto ID '%s' (can't rewind)\n", sid);
				pid = pid_in_file;
			}
			else if (*sid != '\0')
				return iolht_error(pr, "Invalid padstack proto ID '%s' (syntax)\n", sid);
			if (pid >= dst->ps_protos.used)
				pcb_vtpadstack_proto_enlarge(&dst->ps_protos, pid);
			res = parse_data_pstk_proto(pcb, dst->ps_protos.array + pid, pr, subc_parent);
			if (res != 0)
				return iolht_error(pr, "Invalid padstack proto definition\n");
		}
		else
			return iolht_error(pr, "Invalid padstack proto definition\n", pp->name);
	}

	return res;
}

static pcb_data_t *parse_data(pcb_board_t *pcb, pcb_data_t *dst, lht_node_t *nd, pcb_data_t *subc_parent)
{
	pcb_data_t *dt;
	lht_node_t *grp;
	int bound_layers = (subc_parent != NULL);

	if ((nd == NULL) || (nd->type != LHT_HASH))
		return NULL;

	if (dst == NULL)
		dt = pcb_buffer_new(pcb);
	else
		dt = dst;

	if (!bound_layers)
		pcb->Data = dt;

	grp = lht_dom_hash_get(nd, "layers");
	if ((grp != NULL) && (grp->type == LHT_LIST))
		parse_data_layers(pcb, dt, grp, subc_parent);

	if (rdver == 1)
		layer_fixup(pcb);

	if (rdver >= 4) {
		grp = lht_dom_hash_get(nd, "padstack_prototypes");
		if ((grp != NULL) && (grp->type == LHT_LIST))
			parse_data_pstk_protos(pcb, dt, grp, subc_parent);
	}

	grp = lht_dom_hash_get(nd, "objects");
	if (grp != NULL)
		if (parse_data_objects(pcb, dt, grp) != 0)
			return NULL;

	return dt;
}

static int parse_symbol(pcb_symbol_t *sym, lht_node_t *nd)
{
	lht_node_t *grp, *obj, *n;
	lht_dom_iterator_t it;

	parse_coord(&sym->Width, lht_dom_hash_get(nd, "width"));
	parse_coord(&sym->Height, lht_dom_hash_get(nd, "height"));
	parse_coord(&sym->Delta, lht_dom_hash_get(nd, "delta"));

	grp = lht_dom_hash_get(nd, "objects");
	for(obj = lht_dom_first(&it, grp); obj != NULL; obj = lht_dom_next(&it)) {
		pcb_coord_t x1, y1, x2, y2, th, r;
		double sa, da;

		if (strncmp(obj->name, "line.", 5) == 0) {
			parse_coord(&x1, lht_dom_hash_get(obj, "x1"));
			parse_coord(&y1, lht_dom_hash_get(obj, "y1"));
			parse_coord(&x2, lht_dom_hash_get(obj, "x2"));
			parse_coord(&y2, lht_dom_hash_get(obj, "y2"));
			parse_coord(&th, lht_dom_hash_get(obj, "thickness"));
			pcb_font_new_line_in_sym(sym, x1, y1, x2, y2, th);
		}
		else if (strncmp(obj->name, "simplearc.", 10) == 0) {
			parse_coord(&x1, lht_dom_hash_get(obj, "x"));
			parse_coord(&y1, lht_dom_hash_get(obj, "y"));
			parse_coord(&r, lht_dom_hash_get(obj, "r"));
			parse_coord(&th, lht_dom_hash_get(obj, "thickness"));
			parse_double(&sa, lht_dom_hash_get(obj, "astart"));
			parse_double(&da, lht_dom_hash_get(obj, "adelta"));
			pcb_font_new_arc_in_sym(sym, x1, y1, r, sa, da, th);
		}
		else if (strncmp(obj->name, "simplepoly.", 11) == 0) {
			int len;
			pcb_poly_t *sp;
			if (obj->type != LHT_LIST) {
				iolht_error(obj, "Symbol error: simplepoly is not a list! (ignoring this poly)\n");
				continue;
			}
			for(len = 0, n = obj->data.list.first; n != NULL; len++, n = n->next) ;
			if ((len % 2 != 0) || (len < 6)) {
				iolht_error(obj, "Symbol error: sumplepoly has wrong number of points (%d, expected an even integer >= 6)! (ignoring this poly)\n", len);
				continue;
			}
			sp = pcb_font_new_poly_in_sym(sym, len/2);
			for(len = 0, n = obj->data.list.first; n != NULL; len++, n = n->next) {
				parse_coord(&x1, n);
				n = n->next;
				parse_coord(&y1, n);
				sp->Points[len].X = x1;
				sp->Points[len].Y = y1;
			}
		}
	}

	sym->Valid = 1;
	return 0;
}

static int parse_font(pcb_font_t *font, lht_node_t *nd)
{
	lht_node_t *grp, *sym;
	lht_dom_iterator_t it;

	if (nd->type != LHT_HASH)
		return iolht_error(nd, "font must be a hash\n");

	parse_coord(&font->MaxHeight, lht_dom_hash_get(nd, "cell_height"));
	parse_coord(&font->MaxWidth, lht_dom_hash_get(nd, "cell_width"));

	grp = lht_dom_hash_get(nd, "symbols");

	for(sym = lht_dom_first(&it, grp); sym != NULL; sym = lht_dom_next(&it)) {
		int chr;
		if (sym->type != LHT_HASH)
			continue;
		if (*sym->name == '&') {
			char *end;
			chr = strtol(sym->name+1, &end, 16);
			if (*end != '\0') {
				iolht_error(sym, "Ignoring symbol with invalid symbol name '%s'.\n", sym->name);
				continue;
			}
		}
		else
			chr = *sym->name;
		if ((chr >= 0) && (chr < sizeof(font->Symbol) / sizeof(font->Symbol[0]))) {
			parse_symbol(font->Symbol+chr, sym);
		}
	}

	return 0;
}

static int parse_fontkit(pcb_fontkit_t *fk, lht_node_t *nd)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (nd->type != LHT_HASH)
		return iolht_error(nd, "fontkit must be a hash\n");

	pcb_fontkit_reset (fk);

	for(n = lht_dom_first(&it, nd); n != NULL; n = lht_dom_next(&it)) {
		pcb_font_t *f;

		if (strcmp(n->name, "geda_pcb") != 0) {
			char *end;
			int id = strtol(n->name, &end, 10);
			if (*end != '\0')
				continue; /* ingore fonts with invalid name for now - maybe it'd be safer to read the ID field */
			f = pcb_new_font(fk, id, NULL);
		}
		else {
			pcb_font_free (&fk->dflt);
			fk->dflt.id = 0; /* restore default font's ID */
			f = &fk->dflt;
		}

#warning TODO: check return val
		parse_font(f, n);
	}

	return 0;
}


static void post_ids_assign(vtp0_t *ids)
{
	int n;
	for(n = 0; n < vtp0_len(ids); n++) {
		long int *id = ids->array[n];
		*id = pcb_create_ID_get();
	}
	vtp0_uninit(ids);
}

static int parse_styles(vtroutestyle_t *styles, lht_node_t *nd)
{
	lht_node_t *stn;
	lht_dom_iterator_t it;

	if (nd->type != LHT_LIST)
		return iolht_error(nd, "route styles must be a list\n");

	for(stn = lht_dom_first(&it, nd); stn != NULL; stn = lht_dom_next(&it)) {
		pcb_route_style_t *s = vtroutestyle_alloc_append(styles, 1);
		int name_len = strlen(stn->name);

		if (stn->type != LHT_HASH)
			return iolht_error(stn, "route style entry must be a hash\n");
		
		/* safe copy the name */
		if (name_len > sizeof(s->name)-1) {
			iolht_warn(stn, -1, "Route style name too long: '%s' (should be less than %d characters); name will be truncated\n", stn->name, sizeof(s->name)-1);
			memcpy(s->name, stn->name, sizeof(s->name)-2);
			s->name[sizeof(s->name)-1] = '\0';
		}
		else
			memcpy(s->name, stn->name, name_len+1);

		parse_coord(&s->Thick, lht_dom_hash_get(stn, "thickness"));
		parse_coord(&s->Diameter, lht_dom_hash_get(stn, "diameter"));
		parse_coord(&s->Hole, lht_dom_hash_get(stn, "hole"));
		parse_coord(&s->Clearance, lht_dom_hash_get(stn, "clearance"));
		parse_attributes(&s->attr, lht_dom_hash_get(stn, "attributes"));
	}
	return 0;
}

static int parse_netlist_input(pcb_lib_t *lib, lht_node_t *netlist)
{
	lht_node_t *nnet;
	if (netlist->type != LHT_LIST)
		return iolht_error(netlist, "netlist (parent) must be a list\n");

	for(nnet = netlist->data.list.first; nnet != NULL; nnet = nnet->next) {
		lht_node_t *nconn, *nstyle, *nt;
		pcb_lib_menu_t *net;
		const char *style = NULL;

		if (nnet->type != LHT_HASH)
			return iolht_error(nnet, "netlist must be a hash\n");
		nconn  = lht_dom_hash_get(nnet, "conn");
		nstyle = lht_dom_hash_get(nnet, "style");

		if ((nconn != NULL) && (nconn->type != LHT_LIST))
			return iolht_error(nconn, "conn must be a list\n");

		if (nstyle != NULL) {
			if (nstyle->type != LHT_TEXT)
				return iolht_error(nstyle, "style must be text\n");
			style = nstyle->data.text.value;
		}

		net = pcb_lib_net_new(lib, nnet->name, style);
		if (nconn != NULL) {
			for(nt = nconn->data.list.first; nt != NULL; nt = nt->next) {
				if ((nt->type != LHT_TEXT) || (*nt->data.text.value == '\0'))
					return iolht_error(nt, "terminal id must be a non-empty text\n");
				pcb_lib_conn_new(net, nt->data.text.value);
			}
		}
	}
	return 0;
}

static int parse_netlist_patch(pcb_board_t *pcb, lht_node_t *patches)
{
	lht_node_t *np;

	if (patches->type != LHT_LIST)
		return iolht_error(patches, "netlist patches must be a list\n");

	for(np = patches->data.list.first; np != NULL; np = np->next) {
		lht_node_t *nnet, *nkey, *nval;
		if (np->type != LHT_HASH)
			return iolht_error(np, "each netlist patch must be a hash\n");
		nnet = lht_dom_hash_get(np, "net");
		if ((nnet == NULL) || (nnet->type != LHT_TEXT) || (*nnet->data.text.value == '\0'))
			return iolht_error(nnet, "netlist patch net name must be a non-empty text\n");

		if (strcmp(np->name, "del_conn") == 0) {
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return iolht_error(nval, "netlist patch terminal ID must be a non-empty string (del_conn)\n");
			pcb_ratspatch_append(pcb, RATP_ADD_CONN, nval->data.text.value, nnet->data.text.value, NULL);
		}
		else if (strcmp(np->name, "add_conn") == 0) {
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return iolht_error(nval, "netlist patch terminal ID must be a non-empty string (add_conn)\n");
			pcb_ratspatch_append(pcb, RATP_DEL_CONN, nval->data.text.value, nnet->data.text.value, NULL);
		}
		else if (strcmp(np->name, "change_attrib") == 0) {
			nkey = lht_dom_hash_get(np, "key");
			if ((nkey == NULL) || (nkey->type != LHT_TEXT) || (*nkey->data.text.value == '\0'))
				return iolht_error(nval, "netlist patch attrib key must be a non-empty string (change_attrib)\n");
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT))
				return iolht_error(nval, "netlist patch attrib value must be a non-empty string (change_attrib)\n");
			pcb_ratspatch_append(pcb, RATP_CHANGE_ATTRIB, nnet->data.text.value, nkey->data.text.value, nval->data.text.value);
		}
	}
	return 0;
}

static int parse_netlists(pcb_board_t *pcb, lht_node_t *netlists)
{
	lht_node_t *sub;

	if (netlists->type != LHT_HASH)
		return iolht_error(netlists, "netlists must be a hash\n");

	sub = lht_dom_hash_get(netlists, "input");
	if ((sub != NULL) && (parse_netlist_input(pcb->NetlistLib+PCB_NETLIST_INPUT, sub) != 0))
		return iolht_error(sub, "failed to parse the input netlist\n");

	sub = lht_dom_hash_get(netlists, "netlist_patch");
	if ((sub != NULL) && (parse_netlist_patch(pcb, sub) != 0))
		return iolht_error(sub, "failed to parse the netlist patch\n");

	return 0;
}

static void parse_conf(pcb_board_t *pcb, lht_node_t *sub)
{
	if (conf_insert_tree_as(CFR_DESIGN, sub) != 0)
		pcb_message(PCB_MSG_ERROR, "Failed to insert the config subtree '%s' found in %s\n", sub->name, pcb->Filename);
	else
		conf_update(NULL, -1);
}


static int parse_board(pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *sub;
	pcb_plug_io_t *loader;

	warned = 0;
	rdver = atoi(nd->name+15);
	switch(rdver) {
		case 1: loader = &plug_io_lihata_v1; break;
		case 2: loader = &plug_io_lihata_v2; break;
		case 3: loader = &plug_io_lihata_v3; break;
		case 4: loader = &plug_io_lihata_v4; break;
		case 5: loader = &plug_io_lihata_v5; break;
		default:
			return iolht_error(nd, "Lihata board version %d not supported;\n"
				"must be 1, 2, 3, 4 or 5.\n", rdver);
	}

	vtp0_init(&post_ids);
	vtp0_init(&post_thermal_old);
	vtp0_init(&post_thermal_heavy);

	memset(&pcb->LayerGroups, 0, sizeof(pcb->LayerGroups));

	if (parse_attributes(&pcb->Attributes, lht_dom_hash_get(nd, "attributes")) != 0)
		return -1;

	sub = lht_dom_hash_get(nd, "meta");
	if ((sub != NULL) && (parse_meta(pcb, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "font");
	if ((sub != NULL) && (parse_fontkit(&PCB->fontkit, sub) != 0))
		return -1;
	PCB->fontkit.valid = 1;

	if (rdver >= 2) {
		sub = lht_dom_hash_get(nd, "layer_stack");
		if ((sub != NULL) && ((parse_layer_stack(pcb, sub)) != 0))
			return -1;
	}

	pcb_data_clip_inhibit_inc(pcb->Data);

	sub = lht_dom_hash_get(nd, "data");
	if ((sub != NULL) && ((parse_data(pcb, pcb->Data, sub, NULL)) == NULL)) {
		pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);
		return -1;
	}

	sub = lht_dom_hash_get(nd, "styles");
	if ((sub != NULL) && (parse_styles(&pcb->RouteStyle, sub) != 0)) {
		pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);
		return -1;
	}

	sub = lht_dom_hash_get(nd, "netlists");
	if ((sub != NULL) && (parse_netlists(pcb, sub) != 0)) {
		pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);
		return -1;
	}

	post_ids_assign(&post_ids);
	if (post_thermal_assign(pcb, &post_thermal_old, &post_thermal_heavy) != 0) {
		pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);
		return -1;
	}

	/* Run poly clipping at the end so we have all IDs and we can
	   announce the clipping (it's slow, we may need a progress bar) */
	{
		pcb_rtree_it_t it;
		pcb_box_t *b;
		int l;
		for(l = 0; l < pcb->Data->LayerN; l++) {
			pcb_layer_t *layer = pcb->Data->Layer + l;
			for(b = pcb_r_first(layer->polygon_tree, &it); b != NULL; b = pcb_r_next(&it)) {
				pcb_poly_t *p = (pcb_poly_t *)b;
				pcb_poly_init_clip(pcb->Data, layer, p);
			}
			pcb_r_end(&it);
		}
	}

	sub = lht_dom_hash_get(nd, "pcb-rnd-conf-v1");
	if (sub != NULL)
		parse_conf(pcb, sub);

	pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);

	pcb->Data->loader = loader; /* set this manually so the version is remembered */
	return 0;
}

int io_lihata_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;

	realfn = pcb_fopen_check(Filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error loading '%s': %s\n", Filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type == LHT_HASH) && (strncmp(doc->root->name, "pcb-rnd-board-v", 15) == 0)) {
		res = parse_board(Ptr, doc->root);
	}
	else if ((doc->root->type == LHT_LIST) && (strncmp(doc->root->name, "pcb-rnd-subcircuit-v", 20) == 0)) {
		pcb_subc_t *sc;

		warned = 0;
		rdver = atoi(doc->root->name+20);
		Ptr->is_footprint = 1;
		res = parse_subc(NULL, Ptr->Data, doc->root->data.list.first, &sc);

		if (res == 0) {
			pcb_layergrp_upgrade_to_pstk(Ptr);
			pcb_layer_create_all_for_recipe(Ptr, sc->data->Layer, sc->data->LayerN);
			pcb_subc_rebind(Ptr, sc);
			pcb_data_clip_polys(sc->data);
		}
	}
	else {
		iolht_error(doc->root, "Error loading '%s': neither a board nor a subcircuit\n", Filename);
		res = -1;
	}

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}



typedef enum {
	TPS_UNDECIDED,
	TPS_GOOD,
	TPS_BAD
} test_parse_t;

/* expect root to be a ha:pcb-rnd-board-v* */
void test_parse_ev(lht_parse_t *ctx, lht_event_t ev, lht_node_type_t nt, const char *name, const char *value)
{
	test_parse_t *state = ctx->user_data;
	if (ev == LHT_OPEN) {
		if ((nt == LHT_HASH) && (strncmp(name, "pcb-rnd-board-v", 15) == 0))
			*state = TPS_GOOD;
		else if ((nt == LHT_LIST) && (strncmp(name, "pcb-rnd-subcircuit-v", 20) == 0))
			*state = TPS_GOOD;
		else
			*state = TPS_BAD;
	}
}


/* run an event parser for the first 32k of the file; accept the file if it
   has a valid looking root; refuse if:
    - no root in the first 32k (or till eof)
    - not a valid lihata doc (parser error)
    - lihata, but the wrong root
*/
int io_lihata_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	lht_parse_t ctx;
	int count;
	test_parse_t state = TPS_UNDECIDED;

	lht_parser_init(&ctx);
	ctx.event = test_parse_ev;
	ctx.user_data = &state;

	for(count = 0; count < 32768; count++) {
		int c = fgetc(f);
		if (lht_parser_char(&ctx, c) != LHTE_SUCCESS) {
			/* parse error or end */
			state = TPS_BAD;
			break;
		}
		if (state != TPS_UNDECIDED)
			break;
	}
	lht_parser_uninit(&ctx);
	return (state == TPS_GOOD);
}

int io_lihata_parse_font(pcb_plug_io_t *ctx, pcb_font_t *Ptr, const char *Filename)
{
	int res;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;

	realfn = pcb_fopen_check(Filename, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "Error loading '%s': %s\n", Filename, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type != LHT_LIST) || (strcmp(doc->root->name, "pcb-rnd-font-v1"))) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "Not a font lihata.\n");
		res = -1;
	}
	else
		res = parse_font(Ptr, doc->root->data.list.first);

	free(errmsg);
	lht_dom_uninit(doc);
	return res;
}


int io_lihata_parse_element(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name)
{
	int res;
	char *errmsg = NULL;
	lht_doc_t *doc = NULL;
	pcb_fp_fopen_ctx_t st;
	FILE *f;
	pcb_subc_t *sc;

	f = pcb_fp_fopen(pcb_fp_default_search_path(), name, &st);

	if (f != NULL) {
		doc = lht_dom_load_stream(f, name, &errmsg);
		pcb_fp_fclose(f, &st);
	}

	if (doc == NULL) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "Error loading '%s': %s\n", name, errmsg);
		free(errmsg);
		return -1;
	}

	if ((doc->root->type != LHT_LIST) || (strncmp(doc->root->name, "pcb-rnd-subcircuit-v", 20))) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "Not a subcircuit lihata.\n");
		free(errmsg);
		lht_dom_uninit(doc);
		return -1;
	}

	warned = 0;
	rdver = atoi(doc->root->name+20);
	if (rdver < 3) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "io_lihata: invalid subc file version: %s (expected 3 or higher)\n", doc->root->name+20);
		free(errmsg);
		lht_dom_uninit(doc);
		return -1;
	}

	res = parse_subc(NULL, Ptr, doc->root->data.list.first, &sc);
	if (res == 0)
		pcb_data_clip_polys(sc->data);

	lht_dom_uninit(doc);
	free(errmsg);
	return res;
}

