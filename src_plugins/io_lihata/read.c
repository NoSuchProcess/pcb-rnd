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

/* Load a lihata document in-memory and walk the tree and build pcb native
   structs. A full dom load is used instead of the event parser so that
   symlinks and tree merges can be supported later. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <liblihata/tree.h>
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
#include "vtptr.h"
#include "common.h"
#include "polygon.h"
#include "conf_core.h"
#include "obj_all.h"

#warning TODO: put these in a gloal load-context-struct
vtptr_t post_ids, post_thermal;

/* Collect objects that has unknown ID on a list. Once all objects with
   known-IDs are allocated, the unknonw-ID objects are allocated a fresh
   ID. This makes sure they don't occupy IDs that would be used by known-ID
   objects during the load. */
#define post_id_req(obj) vtptr_append(&post_ids, &((obj)->ID))

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
			pcb_attribute_put(list, n->name, n->data.text.value, 0);
	}

	return 0;
}

/* Load the (duplicated) string value of a text node into res. Return 0 on success */
static int parse_text(char **res, lht_node_t *nd)
{
	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;
	*res = pcb_strdup(nd->data.text.value);
	return 0;
}

/* Load the pcb_coord_t value of a text node into res. Return 0 on success */
static int parse_coord(pcb_coord_t *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;

	tmp = GetValueEx(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success) {
		pcb_message(PCB_MSG_ERROR, "#LHT1 Invalid coord value: '%s'\n", nd->data.text.value);
		return -1;
	}

	*res = tmp;
	return 0;
}

/* Load the Angle value of a text node into res. Return 0 on success */
static int parse_angle(pcb_angle_t *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT)) {
		pcb_message(PCB_MSG_ERROR, "#LHT2 Invalid angle type: '%d'\n", nd->type);
		return -1;
	}

	tmp = GetValueEx(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success) {
		pcb_message(PCB_MSG_ERROR, "#LHT3 Invalid angle value: '%s'\n", nd->data.text.value);
		return -1;
	}

	*res = tmp;
	return 0;
}

/* Load the pcb_coord_t value of a text node into res. Return 0 on success */
static int parse_int(int *res, lht_node_t *nd)
{
	long int tmp;
	int base = 10;
	char *end;

	if (nd == NULL)
		return -1;

	if (nd->type != LHT_TEXT) {
		pcb_message(PCB_MSG_ERROR, "#LHT4 Invalid int type: '%d'\n", nd->type);
		return -1;
	}

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtol(nd->data.text.value, &end, base);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "#LHT5 Invalid int value: '%s'\n", nd->data.text.value);
		return -1;
	}

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

	if (nd->type != LHT_TEXT) {
		pcb_message(PCB_MSG_ERROR, "#LHT8 Invalid double type: '%d'\n", nd->type);
		return -1;
	}

	tmp = strtod(nd->data.text.value, &end);

	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "#LHT9 Invalid double value: '%s'\n", nd->data.text.value);
		return -1;
	}

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
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "#LHT6 Invalid id value: '%s' in line %d\n", nd->data.text.value, nd->line);
		return -1;
	}

	CreateIDBump(tmp+1);

	*res = tmp;
	return 0;
}

/* Load the boolean value of a text node into res.
   Return 0 on success */
static int parse_bool(pcb_bool *res, lht_node_t *nd)
{
	if (nd == NULL)
		return -1;

	if ((strcmp(nd->data.text.value, "1") == 0) || (strcasecmp(nd->data.text.value, "on") == 0) ||
	    (strcasecmp(nd->data.text.value, "true") == 0) || (strcasecmp(nd->data.text.value, "yes") == 0)) {
		*res = 1;
		return 0;
	}

	if ((strcmp(nd->data.text.value, "0") == 0) || (strcasecmp(nd->data.text.value, "off") == 0) ||
	    (strcasecmp(nd->data.text.value, "false") == 0) || (strcasecmp(nd->data.text.value, "no") == 0)) {
		*res = 0;
		return 0;
	}

	pcb_message(PCB_MSG_ERROR, "#LHT7 Invalid bool value: '%s'\n", nd->data.text.value);
	return -1;
}

static int parse_meta(pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *grp;

	if (nd->type != LHT_HASH)
		return -1;

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

/* pt is a list of lihata node pointers to thermal nodes; each has user
   data set to the flag. Look up layer info and fill in thermal flags. This
   needs to be done in a separate pass at the end of parsing because
   vias may precede layers in the lihata input file. */
static int post_thermal_assign(vtptr_t *pt)
{
	int i;

	for(i = 0; i < vtptr_len(pt); i++) {
		lht_node_t *n;
		lht_dom_iterator_t it;
		io_lihata_flag_holder fh;
		lht_node_t *thr = pt->array[i];
		pcb_flag_t *f = thr->user_data;

		memset(&fh, 0, sizeof(fh));
		fh.Flags = *f;
		for(n = lht_dom_first(&it, thr); n != NULL; n = lht_dom_next(&it)) {
			if (n->type == LHT_TEXT) {
				int layer = pcb_layer_by_name(n->name);
				if (layer < 0) {
					pcb_message(PCB_MSG_ERROR, "#LHT10 Invalid layer name in thermal: '%s'\n", n->name);
					return -1;
				}
				PCB_FLAG_THERM_ASSIGN(layer, io_lihata_resolve_thermal_style(n->data.text.value), &fh);
			}
		}
		*f = fh.Flags;
	}
	vtptr_uninit(pt);
	return 0;
}

/* NOTE: in case of objects with thermal, f must point to the object's
   flags because termals will be filled in at the end, in a 2nd pass and
   we need to store the f pointer. */
static int parse_flags(pcb_flag_t *f, lht_node_t *fn, int object_type)
{
	io_lihata_flag_holder fh;

	memset(&fh, 0, sizeof(fh));

	if (fn != NULL) {
		int n;
		lht_node_t *thr;
		for (n = 0; n < pcb_object_flagbits_len; n++) {
			if (pcb_object_flagbits[n].object_types & object_type) {
				pcb_bool b;
				if ((parse_bool(&b, lht_dom_hash_get(fn, pcb_object_flagbits[n].name)) == 0) && b)
					PCB_FLAG_SET(pcb_object_flagbits[n].mask, &fh);
			}
		}

		thr = lht_dom_hash_get(fn, "thermal");
		if (thr != NULL) {
			thr->user_data = f;
			vtptr_append(&post_thermal, thr);
		}

		if (parse_int(&n, lht_dom_hash_get(fn, "shape")) == 0)
			fh.Flags.q = n;

		if (parse_int(&n, lht_dom_hash_get(fn, "intconn")) == 0)
			fh.Flags.int_conn_grp = n;
	}

	*f = fh.Flags;
	return 0;
}


static int parse_line(pcb_layer_t *ly, pcb_element_t *el, lht_node_t *obj, int no_id, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_line_t *line;

	if (ly != NULL)
		line = GetLineMemory(ly);
	else if (el != NULL)
		line = GetElementLineMemory(el);
	else
		return -1;

	if (no_id)
		line->ID = 0;
	else
		parse_id(&line->ID, obj, 5);
	parse_attributes(&line->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&line->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_LINE);

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

	parse_id(&rat.ID, obj, 4);
	parse_attributes(&rat.Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&rat.Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_LINE);

	parse_coord(&rat.Point1.X, lht_dom_hash_get(obj, "x1"));
	parse_coord(&rat.Point1.Y, lht_dom_hash_get(obj, "y1"));
	parse_coord(&rat.Point2.X, lht_dom_hash_get(obj, "x2"));
	parse_coord(&rat.Point2.Y, lht_dom_hash_get(obj, "y2"));

	parse_int(&rat.group1, lht_dom_hash_get(obj, "lgrp1"));
	parse_int(&rat.group2, lht_dom_hash_get(obj, "lgrp2"));

	post_id_req(&rat.Point1);
	post_id_req(&rat.Point2);

	new_rat = CreateNewRat(dt, rat.Point1.X, rat.Point1.Y, rat.Point2.X, rat.Point2.Y, rat.group1, rat.group2,
		conf_core.appearance.rat_thickness, rat.Flags);

	new_rat->ID = rat.ID;

	return 0;
}

static int parse_arc(pcb_layer_t *ly, pcb_element_t *el, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_arc_t *arc;

	if (ly != NULL)
		arc = GetArcMemory(ly);
	else if (el != NULL)
		arc = GetElementArcMemory(el);
	else
		return -1;

	parse_id(&arc->ID, obj, 4);
	parse_attributes(&arc->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&arc->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_ARC);

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

static int parse_polygon(pcb_layer_t *ly, pcb_element_t *el, lht_node_t *obj)
{
	pcb_polygon_t *poly = GetPolygonMemory(ly);
	lht_node_t *geo;
	pcb_cardinal_t n = 0, c;

	parse_id(&poly->ID, obj, 8);
	parse_attributes(&poly->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&poly->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_POLYGON);

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
				n++;
			}
		}
	}

	pcb_add_polygon_on_layer(ly, poly);

	return 0;
}

static int parse_pcb_text(pcb_layer_t *ly, pcb_element_t *el, lht_node_t *obj)
{
	pcb_text_t *text;
	lht_node_t *role;
	int tmp;

	role = lht_dom_hash_get(obj, "role");

	if (ly != NULL) {
		if (role != NULL)
			return -1;
		text = GetTextMemory(ly);
	}
	else if (el != NULL) {
		if (role == NULL)
			return -1;
		if (strcmp(role->data.text.value, "desc") == 0) text = &DESCRIPTION_TEXT(el);
		else if (strcmp(role->data.text.value, "name") == 0) text = &NAMEONPCB_TEXT(el);
		else if (strcmp(role->data.text.value, "value") == 0) text = &VALUE_TEXT(el);
		else
			return -1;
	}

	parse_attributes(&text->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&text->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_TEXT);
	parse_int(&text->Scale, lht_dom_hash_get(obj, "scale"));
	parse_int(&tmp, lht_dom_hash_get(obj, "direction"));
	text->Direction = tmp;
	parse_coord(&text->X, lht_dom_hash_get(obj, "x"));
	parse_coord(&text->Y, lht_dom_hash_get(obj, "y"));
	parse_text(&text->TextString, lht_dom_hash_get(obj, "string"));

#warning TODO: get the font
	if (ly != NULL)
		pcb_add_text_on_layer(ly, text, &PCB->Font);
	if (el != NULL)
		text->Element = el;

	return 0;
}

static int parse_data_layer(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp, int layer_id)
{
	lht_node_t *n, *lst;
	lht_dom_iterator_t it;

	pcb_layer_t *ly = &dt->Layer[dt->LayerN];
	dt->LayerN++;

	ly->Name = pcb_strdup(grp->name);
	parse_bool(&ly->On, lht_dom_hash_get(grp, "visible"));
	if (pcb != NULL) {
		int grp_id;
		parse_int(&grp_id, lht_dom_hash_get(grp, "group"));
		pcb_layer_add_in_group(layer_id, grp_id);
	}

	lst = lht_dom_hash_get(grp, "objects");
	if (lst != NULL) {
		if (lst->type != LHT_LIST)
			return -1;

		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(ly, NULL, n, 0, 0, 0);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(ly, NULL, n, 0, 0);
			if (strncmp(n->name, "polygon.", 8) == 0)
				parse_polygon(ly, NULL, n);
			if (strncmp(n->name, "text.", 5) == 0)
				parse_pcb_text(ly, NULL, n);
		}
	}

	return 0;
}

static int parse_data_layers(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *grp)
{
	int id;
	lht_node_t *n;
	lht_dom_iterator_t it;

	for(id = 0, n = lht_dom_first(&it, grp); n != NULL; id++, n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			parse_data_layer(pcb, dt, n, id);

	dt->LayerN -= 2; /* for the silk layers... */
	return 0;
}

/* If el == NULL and dt != NULL it is a via (for now). */
static int parse_pin(pcb_data_t *dt, pcb_element_t *el, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_pin_t *via;

	if (dt != NULL)
		via = GetViaMemory(dt);
	else if (el != NULL)
		via = GetPinMemory(el);
	else
		return -1;

	parse_id(&via->ID, obj, 4);
	parse_attributes(&via->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&via->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_VIA);

	parse_coord(&via->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&via->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&via->Mask, lht_dom_hash_get(obj, "mask"));
	parse_coord(&via->DrillingHole, lht_dom_hash_get(obj, "hole"));
	parse_coord(&via->X, lht_dom_hash_get(obj, "x"));
	parse_coord(&via->Y, lht_dom_hash_get(obj, "y"));
	parse_text(&via->Name, lht_dom_hash_get(obj, "name"));
	parse_text(&via->Number, lht_dom_hash_get(obj, "number"));

	via->X += dx;
	via->Y += dy;

	if (dt != NULL)
		pcb_add_via(dt, via);
	if (el != NULL)
		via->Element = el;

	return 0;
}

static int parse_pad(pcb_element_t *el, lht_node_t *obj, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_pad_t *pad;

	pad = GetPadMemory(el);

	parse_id(&pad->ID, obj, 4);
	parse_attributes(&pad->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&pad->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_PAD);

	parse_coord(&pad->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&pad->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&pad->Mask, lht_dom_hash_get(obj, "mask"));
	parse_coord(&pad->Point1.X, lht_dom_hash_get(obj, "x1"));
	parse_coord(&pad->Point1.Y, lht_dom_hash_get(obj, "y1"));
	parse_coord(&pad->Point2.X, lht_dom_hash_get(obj, "x2"));
	parse_coord(&pad->Point2.Y, lht_dom_hash_get(obj, "y2"));
	parse_text(&pad->Name, lht_dom_hash_get(obj, "name"));
	parse_text(&pad->Number, lht_dom_hash_get(obj, "number"));

	pad->Point1.X += dx;
	pad->Point2.X += dx;
	pad->Point1.Y += dy;
	pad->Point2.Y += dy;

	post_id_req(&pad->Point1);
	post_id_req(&pad->Point2);
	pad->Element = el;

	return 0;
}


static int parse_element(pcb_board_t *pcb, pcb_data_t *dt, lht_node_t *obj)
{
	pcb_element_t *elem = GetElementMemory(dt);
	lht_node_t *lst, *n;
	lht_dom_iterator_t it;
	int onsld;

	parse_id(&elem->ID, obj, 8);
	parse_attributes(&elem->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&elem->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_ELEMENT);
	parse_coord(&elem->MarkX, lht_dom_hash_get(obj, "x"));
	parse_coord(&elem->MarkY, lht_dom_hash_get(obj, "y"));

	onsld = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, elem);

	lst = lht_dom_hash_get(obj, "objects");
	if (lst->type == LHT_LIST) {
		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(NULL, elem, n, 0, elem->MarkX, elem->MarkY);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(NULL, elem, n, elem->MarkX, elem->MarkY);
/*		if (strncmp(n->name, "polygon.", 8) == 0)
			parse_polygon(ly, elem, n);*/
			if (strncmp(n->name, "text.", 5) == 0)
				parse_pcb_text(NULL, elem, n);
			if (strncmp(n->name, "pin.", 4) == 0)
				parse_pin(NULL, elem, n, elem->MarkX, elem->MarkY);
			if (strncmp(n->name, "pad.", 4) == 0)
				parse_pad(elem, n, elem->MarkX, elem->MarkY);
		}
	}

	if (onsld) {
		int n;
		for(n = 0; n < MAX_ELEMENTNAMES; n++)
			PCB_FLAG_SET(PCB_FLAG_ONSOLDER, &elem->Name[n]);
	}

	/* Make sure we use some sort of font */
	if (pcb == NULL)
		pcb = PCB;
	SetElementBoundingBox(dt, elem, &pcb->Font);
	return 0;
}

static int parse_data_objects(pcb_board_t *pcb_for_font, pcb_data_t *dt, lht_node_t *grp)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (grp->type != LHT_LIST)
		return -1;

	for(n = lht_dom_first(&it, grp); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "via.", 4) == 0)
			parse_pin(dt, NULL, n, 0, 0);
		if (strncmp(n->name, "rat.", 4) == 0)
			parse_rat(dt, n);
		else if (strncmp(n->name, "element.", 8) == 0)
			parse_element(pcb_for_font, dt, n);
	}

	return 0;
}

static pcb_data_t *parse_data(pcb_board_t *pcb, lht_node_t *nd)
{
	pcb_data_t *dt;
	lht_node_t *grp;
	if (nd->type != LHT_HASH)
		return NULL;

	dt = calloc(sizeof(pcb_data_t), 1);

	grp = lht_dom_hash_get(nd, "layers");
	if ((grp != NULL) && (grp->type == LHT_LIST))
		parse_data_layers(pcb, dt, grp);

	grp = lht_dom_hash_get(nd, "objects");
	if (grp != NULL)
		parse_data_objects(pcb, dt, grp);

	dt->pcb = pcb;


	return dt;
}

static int parse_symbol(pcb_symbol_t *sym, lht_node_t *nd)
{
	lht_node_t *grp, *obj;
	lht_dom_iterator_t it;

	parse_coord(&sym->Width, lht_dom_hash_get(nd, "width"));
	parse_coord(&sym->Height, lht_dom_hash_get(nd, "height"));
	parse_coord(&sym->Delta, lht_dom_hash_get(nd, "delta"));

	grp = lht_dom_hash_get(nd, "objects");
	for(obj = lht_dom_first(&it, grp); obj != NULL; obj = lht_dom_next(&it)) {
		pcb_coord_t x1, y1, x2, y2, th;
		parse_coord(&x1, lht_dom_hash_get(obj, "x1"));
		parse_coord(&y1, lht_dom_hash_get(obj, "y1"));
		parse_coord(&x2, lht_dom_hash_get(obj, "x2"));
		parse_coord(&y2, lht_dom_hash_get(obj, "y2"));
		parse_coord(&th, lht_dom_hash_get(obj, "thickness"));
		pcb_font_new_line_in_sym(sym, x1, y1, x2, y2, th);
	}

	sym->Valid = 1;
	return 0;
}

static int parse_font(pcb_font_t *font, lht_node_t *nd)
{
	lht_node_t *grp, *sym;
	lht_dom_iterator_t it;
	int n;

	if (nd->type != LHT_HASH)
		return -1;

	/* TODO: support only one, hard-wired font for now */
	nd = lht_dom_hash_get(nd, "geda_pcb");

	parse_coord(&font->MaxHeight, lht_dom_hash_get(nd, "cell_height"));
	parse_coord(&font->MaxWidth, lht_dom_hash_get(nd, "cell_width"));

	grp = lht_dom_hash_get(nd, "symbols");

	for(n = 0; n < sizeof(font->Symbol) / sizeof(font->Symbol[0]); n++) {
		font->Symbol[n].LineN = 0;
		font->Symbol[n].Valid = 0;
	}

	for(sym = lht_dom_first(&it, grp); sym != NULL; sym = lht_dom_next(&it)) {
		int chr;
		if (sym->type != LHT_HASH)
			continue;
		if (*sym->name == '&') {
			char *end;
			chr = strtol(sym->name+1, &end, 16);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Ignoring invalid symbol name '%s'.\n", sym->name);
				continue;
			}
		}
		else
			chr = *sym->name;
		if ((chr >= 0) && (chr < sizeof(font->Symbol) / sizeof(font->Symbol[0]))) {
			parse_symbol(font->Symbol+chr, sym);
		}
	}

	font->Valid = 1;
	return 0;
}

static void post_ids_assign(vtptr_t *ids)
{
	int n;
	for(n = 0; n < vtptr_len(ids); n++) {
		long int *id = ids->array[n];
		*id = CreateIDGet();
	}
	vtptr_uninit(ids);
}

static int parse_styles(vtroutestyle_t *styles, lht_node_t *nd)
{
	lht_node_t *stn;
	lht_dom_iterator_t it;

	if (nd->type != LHT_LIST)
		return -1;

	for(stn = lht_dom_first(&it, nd); stn != NULL; stn = lht_dom_next(&it)) {
		pcb_route_style_t *s = vtroutestyle_alloc_append(styles, 1);
		int name_len = strlen(stn->name);

		/* safe copy the name */
		if (name_len > sizeof(s->name)-1) {
			pcb_message(PCB_MSG_WARNING, "Route style name too long: '%s' (should be less than %d characters)\n", stn->name, sizeof(s->name)-1);
			memcpy(s->name, stn->name, sizeof(s->name)-2);
			s->name[sizeof(s->name)-1] = '\0';
		}
		else
			memcpy(s->name, stn->name, name_len+1);

		parse_coord(&s->Thick, lht_dom_hash_get(stn, "thickness"));
		parse_coord(&s->Diameter, lht_dom_hash_get(stn, "diameter"));
		parse_coord(&s->Hole, lht_dom_hash_get(stn, "hole"));
		parse_coord(&s->Clearance, lht_dom_hash_get(stn, "clearance"));
	}
	return 0;
}

static int parse_netlist_input(pcb_lib_t *lib, lht_node_t *netlist)
{
	lht_node_t *nnet;
	if (netlist->type != LHT_LIST)
		return -1;

	for(nnet = netlist->data.list.first; nnet != NULL; nnet = nnet->next) {
		lht_node_t *nconn, *nstyle, *nt;
		pcb_lib_menu_t *net;
		const char *style = NULL;

		if (nnet->type != LHT_HASH)
			return -1;
		nconn  = lht_dom_hash_get(nnet, "conn");
		nstyle = lht_dom_hash_get(nnet, "style");

		if ((nconn != NULL) && (nconn->type != LHT_LIST))
			return -1;

		if (nstyle != NULL) {
			if (nstyle->type != LHT_TEXT)
				return -1;
			style = nstyle->data.text.value;
		}

		net = pcb_lib_net_new(lib, nnet->name, style);
		if (nconn != NULL) {
			for(nt = nconn->data.list.first; nt != NULL; nt = nt->next) {
				if ((nt->type != LHT_TEXT) || (*nt->data.text.value == '\0'))
					return -1;
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
		return -1;

	for(np = patches->data.list.first; np != NULL; np = np->next) {
		lht_node_t *nnet, *nkey, *nval;
		if (np->type != LHT_HASH)
			return -1;
		nnet = lht_dom_hash_get(np, "net");
		if ((nnet == NULL) || (nnet->type != LHT_TEXT) || (*nnet->data.text.value == '\0'))
			return -1;

		if (strcmp(np->name, "del_conn") == 0) {
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return -1;
			rats_patch_append(pcb, RATP_ADD_CONN, nval->data.text.value, nnet->data.text.value, NULL);
		}
		else if (strcmp(np->name, "add_conn") == 0) {
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT) || (*nval->data.text.value == '\0'))
				return -1;
			rats_patch_append(pcb, RATP_DEL_CONN, nval->data.text.value, nnet->data.text.value, NULL);
		}
		else if (strcmp(np->name, "change_attrib") == 0) {
			nkey = lht_dom_hash_get(np, "key");
			if ((nkey == NULL) || (nkey->type != LHT_TEXT) || (*nkey->data.text.value == '\0'))
				return -1;
			nval = lht_dom_hash_get(np, "term");
			if ((nval == NULL) || (nval->type != LHT_TEXT))
				return -1;
			rats_patch_append(pcb, RATP_CHANGE_ATTRIB, nnet->data.text.value, nkey->data.text.value, nval->data.text.value);
		}
	}
	return 0;
}

static int parse_netlists(pcb_board_t *pcb, lht_node_t *netlists)
{
	lht_node_t *sub;

	if (netlists->type != LHT_HASH)
		return -1;

	sub = lht_dom_hash_get(netlists, "input");
	if ((sub != NULL) && (parse_netlist_input(pcb->NetlistLib+NETLIST_INPUT, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(netlists, "netlist_patch");
	if ((sub != NULL) && (parse_netlist_patch(pcb, sub) != 0))
		return -1;

	return 0;
}

static int parse_board(pcb_board_t *pcb, lht_node_t *nd)
{
	lht_node_t *sub;

	if ((nd->type != LHT_HASH) || (strcmp(nd->name, "pcb-rnd-board-v1"))) {
		pcb_message(PCB_MSG_DEFAULT, "Not a board lihata.\n");
		return -1;
	}

	vtptr_init(&post_ids);
	vtptr_init(&post_thermal);

	memset(&pcb->LayerGroups, 0, sizeof(pcb->LayerGroups));

	if (parse_attributes(&pcb->Attributes, lht_dom_hash_get(nd, "attributes")) != 0)
		return -1;

	sub = lht_dom_hash_get(nd, "meta");
	if ((sub != NULL) && (parse_meta(pcb, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "data");
	if ((sub != NULL) && ((pcb->Data = parse_data(pcb, sub)) == NULL))
		return -1;

	sub = lht_dom_hash_get(nd, "font");
	if ((sub != NULL) && (parse_font(&pcb->Font, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "styles");
	if ((sub != NULL) && (parse_styles(&pcb->RouteStyle, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "netlists");
	if ((sub != NULL) && (parse_netlists(pcb, sub) != 0))
		return -1;

	post_ids_assign(&post_ids);
	if (post_thermal_assign(&post_thermal) != 0)
		return -1;

	/* Run poly clipping at the end so we have all IDs and we can
	   announce the clipping (it's slow, we may need a progress bar) */
	ALLPOLYGON_LOOP(pcb->Data);
	{
		InitClip(pcb->Data, layer, polygon);
	}
	ENDALL_LOOP;

	return 0;
}

int io_lihata_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int res;
	char *errmsg;
	lht_doc_t *doc = lht_dom_load(Filename, &errmsg);

	if (doc == NULL) {
/*		pcb_message(PCB_MSG_DEFAULT, "Error loading '%s': %s\n", Filename, errmsg);*/
		return -1;
	}

	res = parse_board(Ptr, doc->root);
	lht_dom_uninit(doc);
	return res;
}
