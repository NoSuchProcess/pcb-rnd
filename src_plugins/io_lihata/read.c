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

/* Load a lihata document in-memory and walk the tree and build pcb native
   structs. A full dom load is used instead of the event parser so that
   symlinks and tree merges can be supported later. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <liblihata/tree.h>
#include "global.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "strflags.h"
#include "compat_misc.h"
#include "macro.h"
#include "error.h"
#include "misc.h"
#include "misc_util.h"
#include "layer.h"
#include "create.h"
#include "vtptr.h"
#include "common.h"

#warning TODO: put these in a gloal load-context-struct
vtptr_t post_ids;

/* Collect objects that has unknown ID on a list. Once all objects with
   known-IDs are allocated, the unknonw-ID objects are allocated a fresh
   ID. This makes sure they don't occupy IDs that would be used by known-ID
   objects during the load. */
#define post_id_req(obj) vtptr_append(&post_ids, &((obj)->ID))

static int parse_attributes(AttributeListType *list, lht_node_t *nd)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (nd == NULL)
		return 0;

	if (nd->type != LHT_HASH)
		return -1;

	for(n = lht_dom_first(&it, nd); n != NULL; n = lht_dom_next(&it)) {
		if (n->type == LHT_TEXT)
			AttributePutToList(list, n->name, n->data.text.value, 0);
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

/* Load the Coord value of a text node into res. Return 0 on success */
static int parse_coord(Coord *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;
	
	tmp = GetValueEx(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return -1;

	*res = tmp;
	return 0;
}

/* Load the Angle value of a text node into res. Return 0 on success */
static int parse_angle(Angle *res, lht_node_t *nd)
{
	double tmp;
	pcb_bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;
	
	tmp = GetValueEx(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return -1;

	*res = tmp;
	return 0;
}

/* Load the Coord value of a text node into res. Return 0 on success */
static int parse_int(int *res, lht_node_t *nd)
{
	long int tmp;
	int base = 10;
	char *end;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;

	if ((nd->data.text.value[0] == '0') && (nd->data.text.value[1] == 'x'))
		base = 16;
	tmp = strtol(nd->data.text.value, &end, base);
	if (*end != '\0')
		return -1;

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
		return -1;

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

	return -1;
}

static int parse_meta(PCBType *pcb, lht_node_t *nd)
{
	lht_node_t *grp;

	if (nd->type != LHT_HASH)
		return -1;

	parse_text(&pcb->Name, lht_dom_hash_get(nd, "meta"));

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

	return 0;
}

static int parse_flags(FlagType *f, lht_node_t *fn, int object_type)
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
					SET_FLAG(pcb_object_flagbits[n].mask, &fh);
			}
		}

		thr = lht_dom_hash_get(fn, "thermal");
		if (thr != NULL) {
			lht_node_t *n;
			lht_dom_iterator_t it;
			int layer;

			for(layer = 0, n = lht_dom_first(&it, thr); n != NULL; layer++, n = lht_dom_next(&it))
				if (n->type == LHT_TEXT)
					ASSIGN_THERM(layer, io_lihata_resolve_thermal_style(n->data.text.value), &fh);
		}

		if (parse_int(&n, lht_dom_hash_get(fn, "shape")) == 0)
			fh.Flags.q = n;

		if (parse_int(&n, lht_dom_hash_get(fn, "intconn")) == 0)
			fh.Flags.int_conn_grp = n;
	}

	*f = fh.Flags;
	return 0;
}


static int parse_line(LayerType *ly, ElementType *el, lht_node_t *obj)
{
	LineType *line;

	if (ly != NULL)
		line = GetLineMemory(ly);
	else if (el != NULL)
		line = GetElementLineMemory(el);
	else
		return -1;

	parse_id(&line->ID, obj, 5);
	parse_attributes(&line->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&line->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_LINE);

	parse_coord(&line->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&line->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&line->Point1.X, lht_dom_hash_get(obj, "x1"));
	parse_coord(&line->Point1.Y, lht_dom_hash_get(obj, "y1"));
	parse_coord(&line->Point2.X, lht_dom_hash_get(obj, "x2"));
	parse_coord(&line->Point2.Y, lht_dom_hash_get(obj, "y2"));

	post_id_req(&line->Point1);
	post_id_req(&line->Point2);

	if (ly != NULL)
		pcb_add_line_on_layer(ly, line);

	return 0;
}

static int parse_arc(LayerType *ly, ElementType *el, lht_node_t *obj)
{
	ArcType *arc;

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

	if (ly != NULL)
		pcb_add_arc_on_layer(ly, arc);

	return 0;

}

static int parse_polygon(LayerType *ly, ElementType *el, lht_node_t *obj)
{
	PolygonType *poly = GetPolygonMemory(ly);
	lht_node_t *geo;
	Cardinal n, c;

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
		poly->Points = malloc(sizeof(PointType) * poly->PointMax);
		poly->HoleIndexMax = poly->HoleIndexN = c-1;
		poly->HoleIndex = malloc(sizeof(Cardinal) * poly->HoleIndexMax);

		/* convert points and build hole index */
		for(c = 0, cnt = lht_dom_first(&it, geo); cnt != NULL; c++, cnt = lht_dom_next(&it)) {
			Cardinal r;
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

static int parse_pcb_text(LayerType *ly, ElementType *el, lht_node_t *obj)
{
	TextType *text;
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

static int parse_data_layer(PCBType *pcb, DataType *dt, lht_node_t *grp, int layer_id)
{
	lht_node_t *n, *lst;
	lht_dom_iterator_t it;

	LayerType *ly = &dt->Layer[dt->LayerN];
	dt->LayerN++;

	ly->Name = pcb_strdup(grp->name);
	parse_bool(&ly->On, lht_dom_hash_get(grp, "visible"));
	if (pcb != NULL) {
		int grp_id;
		parse_int(&grp_id, lht_dom_hash_get(grp, "group"));
		pcb_layer_add_in_group(layer_id, grp_id);
	}

	lst = lht_dom_hash_get(grp, "objects");
	if (lst->type != LHT_LIST)
		return -1;

	for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "line.", 5) == 0)
			parse_line(ly, NULL, n);
		if (strncmp(n->name, "arc.", 4) == 0)
			parse_arc(ly, NULL, n);
		if (strncmp(n->name, "polygon.", 8) == 0)
			parse_polygon(ly, NULL, n);
		if (strncmp(n->name, "text.", 5) == 0)
			parse_pcb_text(ly, NULL, n);
	}

	return 0;
}

static int parse_data_layers(PCBType *pcb, DataType *dt, lht_node_t *grp)
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
static int parse_pin(DataType *dt, ElementType *el, lht_node_t *obj)
{
	PinType *via;

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

	if (dt != NULL)
		pcb_add_via(dt, via);
	if (el != NULL)
		via->Element = el;

	return 0;
}

static int parse_pad(ElementType *el, lht_node_t *obj)
{
	PadType *pad;

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

	post_id_req(&pad->Point1);
	post_id_req(&pad->Point2);
	pad->Element = el;

	return 0;
}


static int parse_element(PCBType *pcb, DataType *dt, lht_node_t *obj)
{
	ElementType *elem = GetElementMemory(dt);
	lht_node_t *lst, *n;
	lht_dom_iterator_t it;

	parse_id(&elem->ID, obj, 4);
	parse_attributes(&elem->Attributes, lht_dom_hash_get(obj, "attributes"));
	parse_flags(&elem->Flags, lht_dom_hash_get(obj, "flags"), PCB_TYPE_VIA);

	lst = lht_dom_hash_get(obj, "objects");
	if (lst->type == LHT_LIST) {
		for(n = lht_dom_first(&it, lst); n != NULL; n = lht_dom_next(&it)) {
			if (strncmp(n->name, "line.", 5) == 0)
				parse_line(NULL, elem, n);
			if (strncmp(n->name, "arc.", 4) == 0)
				parse_arc(NULL, elem, n);
/*		if (strncmp(n->name, "polygon.", 8) == 0)
			parse_polygon(ly, elem, n);*/
			if (strncmp(n->name, "text.", 5) == 0)
				parse_pcb_text(NULL, elem, n);
			if (strncmp(n->name, "pin.", 4) == 0)
				parse_pin(NULL, elem, n);
			if (strncmp(n->name, "pad.", 4) == 0)
				parse_pad(elem, n);
		}
	}

	/* Make sure we use some sort of font */
	if (pcb == NULL)
		pcb = PCB;
	SetElementBoundingBox(dt, elem, &pcb->Font);
	return 0;
}

static int parse_data_objects(PCBType *pcb_for_font, DataType *dt, lht_node_t *grp)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (grp->type != LHT_LIST)
		return -1;

	for(n = lht_dom_first(&it, grp); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "via.", 4) == 0)
			parse_pin(dt, NULL, n);
		else if (strncmp(n->name, "element.", 8) == 0)
			parse_element(pcb_for_font, dt, n);
	}

	return 0;
}

static DataType *parse_data(PCBType *pcb, lht_node_t *nd)
{
	DataType *dt;
	lht_node_t *grp;
	if (nd->type != LHT_HASH)
		return NULL;

	dt = calloc(sizeof(DataType), 1);

	grp = lht_dom_hash_get(nd, "layers");
	if ((grp != NULL) && (grp->type == LHT_LIST))
		parse_data_layers(pcb, dt, grp);

	grp = lht_dom_hash_get(nd, "objects");
	if (grp != NULL)
		parse_data_objects(pcb, dt, grp);

	dt->pcb = pcb;

	return dt;
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

static int parse_board(PCBType *pcb, lht_node_t *nd)
{
	lht_node_t *sub;

	if ((nd->type != LHT_HASH) || (strcmp(nd->name, "pcb-rnd-board-v1"))) {
		Message(PCB_MSG_DEFAULT, "Not a board lihata.\n");
		return -1;
	}

	vtptr_init(&post_ids);

	memset(&pcb->LayerGroups, 0, sizeof(pcb->LayerGroups));

	if (parse_attributes(&pcb->Attributes, lht_dom_hash_get(nd, "attributes")) != 0)
		return -1;

	sub = lht_dom_hash_get(nd, "meta");
	if ((sub != NULL) && (parse_meta(pcb, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "data");
	if ((sub != NULL) && ((pcb->Data = parse_data(pcb, sub)) == NULL))
		return -1;

	post_ids_assign(&post_ids);

	return 0;
}

int io_lihata_parse_pcb(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest)
{
	int res;
	char *errmsg;
	lht_doc_t *doc = lht_dom_load(Filename, &errmsg);

	if (doc == NULL) {
		Message(PCB_MSG_DEFAULT, "Error loading '%s': %s\n", Filename, errmsg);
		return -1;
	}

	res = parse_board(Ptr, doc->root);
	lht_dom_uninit(doc);
	return res;
}

