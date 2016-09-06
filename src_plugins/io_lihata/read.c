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
	bool success;

	if ((nd == NULL) || (nd->type != LHT_TEXT))
		return -1;
	
	tmp = GetValueEx(nd->data.text.value, NULL, NULL, NULL, NULL, &success);
	if (!success)
		return -1;

	*res = tmp;
	return 0;
}

/* Load the id value of a text node (with a prefixed name) into res.
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

	*res = tmp;
	return 0;
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

static int parse_data_layers(DataType *dt, lht_node_t *grp)
{
#warning TODO
	return 0;
}


static int parse_pin(DataType *dt, lht_node_t *obj, int is_via)
{
	PinType *via = GetViaMemory(dt);

	parse_id(&via->ID, obj, 4);
	parse_attributes(&via->Attributes, lht_dom_hash_get(obj, "attributes"));

	parse_coord(&via->Thickness, lht_dom_hash_get(obj, "thickness"));
	parse_coord(&via->Clearance, lht_dom_hash_get(obj, "clearance"));
	parse_coord(&via->Mask, lht_dom_hash_get(obj, "mask"));
	parse_coord(&via->DrillingHole, lht_dom_hash_get(obj, "hole"));
	parse_coord(&via->X, lht_dom_hash_get(obj, "x"));
	parse_coord(&via->Y, lht_dom_hash_get(obj, "y"));
	parse_text(&via->Name, lht_dom_hash_get(obj, "name"));

	return 0;
}

static int parse_element(DataType *dt, lht_node_t *obj)
{
#warning TODO
	return 0;
}

static int parse_data_objects(DataType *dt, lht_node_t *grp)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (grp->type != LHT_LIST)
		return -1;

	for(n = lht_dom_first(&it, grp); n != NULL; n = lht_dom_next(&it)) {
		if (strncmp(n->name, "via.", 4) == 0)
			parse_pin(dt, n, 1);
		else if (strncmp(n->name, "element.", 8) == 0)
			parse_element(dt, n);
	}

	return 0;
}

static DataType *parse_data(lht_node_t *nd)
{
	DataType *dt;
	lht_node_t *grp;
	if (nd->type != LHT_HASH)
		return NULL;

	dt = calloc(sizeof(DataType), 1);

	grp = lht_dom_hash_get(nd, "layers");
	if ((grp != NULL) && (grp->type == LHT_LIST))
		parse_data_layers(dt, grp);

	grp = lht_dom_hash_get(nd, "objects");
	if (grp != NULL)
		parse_data_objects(dt, grp);

	return dt;
}

static int parse_board(PCBType *pcb, lht_node_t *nd)
{
	lht_node_t *sub;

	if ((nd->type != LHT_HASH) || (strcmp(nd->name, "pcb-rnd-board-v1"))) {
		Message("Not a board lihata.\n");
		return -1;
	}

	if (parse_attributes(&pcb->Attributes, lht_dom_hash_get(nd, "attributes")) != 0)
		return -1;

	sub = lht_dom_hash_get(nd, "meta");
	if ((sub != NULL) && (parse_meta(pcb, sub) != 0))
		return -1;

	sub = lht_dom_hash_get(nd, "data");
	if ((sub != NULL) && ((pcb->Data = parse_data(sub)) == NULL))
		return -1;

	return 0;
}

int io_lihata_parse_pcb(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest)
{
	int res;
	char *errmsg;
	lht_doc_t *doc = lht_dom_load(Filename, &errmsg);

	if (doc == NULL) {
		Message("Error loading '%s': %s\n", Filename, errmsg);
		return -1;
	}

	res = parse_board(Ptr, doc->root);
	lht_dom_uninit(doc);
	return res;
}

