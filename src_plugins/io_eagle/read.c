/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2017 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "../src_plugins/boardflip/boardflip.h"

#include "board.h"
#include "read.h"
#include "conf.h"
#include "conf_core.h"
#include "error.h"
#include "hid_actions.h"

typedef struct eagle_layer_s {
	const char *name;
	int color;
	int fill;
	int visible;
	int active;

	pcb_layer_id_t ly;
} eagle_layer_t;

typedef struct eagle_library_s {
	const char *desc;
	htsp_t elems; /* -> pcb_elemement_t */
} eagle_library_t;


typedef struct read_state_s {
	xmlDoc *doc;
	xmlNode *root;
	pcb_board_t *pcb;

	htip_t layers;
	htsp_t libs;

	/* design rules */
	pcb_coord_t md_wire_wire; /* minimal distance between wire and wire (clearance) */
	pcb_coord_t ms_width; /* minimal trace width */
	double rv_pad_top, rv_pad_inner, rv_pad_bottom; /* pad size-to-drill ration on different layers */
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser)(read_state_t *st, xmlNode *subtree, void *obj, int type);
} dispatch_t;

typedef enum {
	IN_ELEM = 1,
	ON_BOARD
} eagle_loc_t;


/* Xml path walk that's much simpler than xpath; the ... is a NULL
   terminated list of node names */
static xmlNode *eagle_xml_path(xmlNode *subtree, ...)
{
	xmlNode *nd = subtree;
	const char *target;
	va_list ap;

	va_start(ap, subtree);

	/* get next path element */
	while((target = va_arg(ap, const char *)) != NULL) {
		/* look for target on this level */
		for(nd = nd->children;;nd = nd->next) {
			if (nd == NULL) {/* target not found on this level */
				va_end(ap);
				return NULL;
			}
			if (xmlStrcmp(nd->name, (const xmlChar *)target) == 0) /* found, skip to next level */
				break;
		}
	}

	va_end(ap);
	return nd;
}

/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int eagle_dispatch(read_state_t *st, xmlNode *subtree, const dispatch_t *disp_table, void *obj, int type)
{
	const dispatch_t *d;
	const xmlChar *name;

	/* do not tolerate empty/NIL node */
	if (subtree->name == NULL)
		return -1;

	if (subtree->type == XML_TEXT_NODE)
		name = (const xmlChar *)"@text";
	else
		name = subtree->name;

	for(d = disp_table; d->node_name != NULL; d++)
		if (xmlStrcmp((xmlChar *)d->node_name, name) == 0)
			return d->parser(st, subtree, obj, type);

	pcb_message(PCB_MSG_ERROR, "eagle: unknown node: '%s'\n", name);
	/* node name not found in the dispatcher table */
	return -1;
}

/* Take each children of tree and execute them using eagle_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */
static int eagle_foreach_dispatch(read_state_t *st, xmlNode *tree, const dispatch_t *disp_table, void *obj, int type)
{
	xmlNode *n;

	for(n = tree; n != NULL; n = n->next)
		if (eagle_dispatch(st, n, disp_table, obj, type) != 0)
			return -1;

	return 0; /* success */
}

/* No-op: ignore the subtree */
static int eagle_read_nop(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	return 0;
}



int io_eagle_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	char line[1024];
	int found = 0, lineno = 0;

	/* look for <!DOCTYPE.*eagle.dtd in the first 32 lines, not assuming it is in a single line */
	while(fgets(line, sizeof(line), f) != NULL) {
		if ((found == 0) && (strstr(line, "<!DOCTYPE")))
			found = 1;
		if ((found == 1) && (strstr(line, "eagle.dtd")))
			return 1;
		lineno++;
		if (lineno > 32)
			break;
	}
	return 0;
}

/* Return a node attribute value converted to long, or return invalid_val
   for synatx error or if the attribute doesn't exist */
static long eagle_get_attrl(xmlNode *nd, const char *name, long invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	char *end;
	long res;

	if (p == NULL)
		return invalid_val;
	res = strtol((char *)p, &end, 10);
	if (*end != '\0')
		return invalid_val;
	return res;
}

/* Return a node attribute value converted to double, or return invalid_val
   for synatx error or if the attribute doesn't exist */
static double eagle_get_attrd(xmlNode *nd, const char *name, double invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	char *end;
	double res;

	if (p == NULL)
		return invalid_val;
	res = strtod((char *)p, &end);
	if (*end != '\0')
		return invalid_val;
	return res;
}

/* Return a node attribute value converted to char *, or return invalid_val
   if the attribute doesn't exist */
static const char *eagle_get_attrs(xmlNode *nd, const char *name, const char *invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	if (p == NULL)
		return invalid_val;
	return (const char *)p;
}

/* Return a node attribute value converted to coord, or return invalid_val
   if the attribute doesn't exist */
static pcb_coord_t eagle_get_attrc(xmlNode *nd, const char *name, pcb_coord_t invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	pcb_coord_t c;
	pcb_bool succ;

	if (p == NULL)
		return invalid_val;

	c = pcb_get_value((char *)p, "mm", NULL, &succ);
	if (!succ)
		return invalid_val;
	return c;
}

/* same as eagle_get_attrc() but assume the input has units */
static pcb_coord_t eagle_get_attrcu(xmlNode *nd, const char *name, pcb_coord_t invalid_val)
{
	xmlChar *p = xmlGetProp(nd, (xmlChar *)name);
	pcb_coord_t c;
	pcb_bool succ;

	if (p == NULL)
		return invalid_val;

	c = pcb_get_value((char *)p, NULL, NULL, &succ);
	if (!succ)
		return invalid_val;
	return c;
}

static int eagle_read_layers(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	xmlNode *n;

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"layer") == 0) {
			eagle_layer_t *ly = calloc(sizeof(eagle_layer_t), 1);
			int id, reuse = 0;
			unsigned long typ;
			pcb_layergrp_id_t gid;
			pcb_layer_group_t *grp;

			ly->name    = eagle_get_attrs(n, "name", NULL);
			ly->color   = eagle_get_attrl(n, "color", -1);
			ly->fill    = eagle_get_attrl(n, "fill", -1);
			ly->visible = eagle_get_attrl(n, "visible", -1);
			ly->active  = eagle_get_attrl(n, "active", -1);
			ly->ly      = -1;
			id = eagle_get_attrl(n, "number", -1);
			if (id >= 0)
				htip_set(&st->layers, id, ly);

			typ = 0;
			switch(id) {
				case 1: typ = PCB_LYT_COPPER | PCB_LYT_TOP; break;
				case 16: typ = PCB_LYT_COPPER | PCB_LYT_BOTTOM; break;
				case 121:
				case 25: /* names */
				case 27: /* values */
				case 39: /* keepout */
					reuse = 1;
					typ = PCB_LYT_SILK | PCB_LYT_TOP;
					break;
				case 122:
				case 26: /* names */
				case 28: /* values */
				case 40: /* keepout */
					reuse = 1;
					typ = PCB_LYT_SILK | PCB_LYT_BOTTOM;
					break;
				case 199:
					grp = pcb_get_grp_new_intern(st->pcb, -1);
					ly->ly = pcb_layer_create(grp - st->pcb->LayerGroups.grp, ly->name);
					pcb_layergrp_fix_turn_to_outline(grp);
					break;

				default:
					if ((id > 1) && (id < 16)) {
						/* new internal layer */
						grp = pcb_get_grp_new_intern(st->pcb, -1);
						ly->ly = pcb_layer_create(grp - st->pcb->LayerGroups.grp, ly->name);
					}
			}
			if (typ != 0) {
				if (reuse)
					pcb_layer_list(typ, &ly->ly, 1);
				if ((ly->ly < 0) && (pcb_layergrp_list(st->pcb, typ, &gid, 1) > 0))
					ly->ly = pcb_layer_create(gid, ly->name);
			}
		}
	}
	pcb_layer_group_setup_silks(&st->pcb->LayerGroups);
	pcb_layer_auto_fixup(st->pcb);
	return 0;
}

static eagle_layer_t *eagle_layer_get(read_state_t *st, int id)
{
	return htip_get(&st->layers, id);
}

static pcb_element_t *eagle_libelem_get(read_state_t *st, const char *lib, const char *elem)
{
	eagle_library_t *l;
	l = htsp_get(&st->libs, lib);
	if (l == NULL)
		return NULL;
	return htsp_get(&l->elems, elem);
}

static void size_bump(read_state_t *st, pcb_coord_t x, pcb_coord_t y)
{
	if (x > st->pcb->MaxWidth)
		st->pcb->MaxWidth = x;
	if (y > st->pcb->MaxHeight)
		st->pcb->MaxHeight = y;
}

/****************** drawing primitives ******************/


static int eagle_read_text(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
/*	eagle_loc_t loc = type; */
/*	pcb_text_t *text; */
	long ln = eagle_get_attrl(subtree, "layer", -1);
/*	eagle_layer_t *ly; */
	pcb_coord_t X, Y, height;
	const char *rot, *text_val;
	unsigned int text_direction = 0;

	if (subtree->children == NULL) {
		pcb_message(PCB_MSG_WARNING, "Ignoring empty text field\n");
		return 0;
	}
	if (subtree->children->type != XML_TEXT_NODE) {
		pcb_message(PCB_MSG_WARNING, "Ignoring text field (invalid child node)\n");
		return 0;
	}

#warning TODO: need to convert
	text_val = (const char *)xmlNodeGetContent(subtree->children);
	X = eagle_get_attrc(subtree, "x", -1);
	Y = eagle_get_attrc(subtree, "y", -1);
	height = PCB_MM_TO_COORD(eagle_get_attrc(subtree, "size", -1));
	rot = eagle_get_attrs(subtree, "rot", NULL);
	if (rot == NULL) {
		rot = "R0";
	}

	if (rot[0] == 'R') {
		int deg = atoi(rot+1);
		printf("text rot: %s\n", rot);
		if (deg < 45 || deg >= 315) {
			text_direction = 0;
		} else if (deg < 135 && deg >= 45) {
			text_direction = 1;
		} else if (deg < 225 && deg >= 135) {
			text_direction = 2;
		} else {
			text_direction = 3;
		}
	}
	

	pcb_printf("\ttext found on Eagle layout at with rot: %s at %mm;%mm %mm: '%s' ln=%d dir=%d\n", rot, X, Y, height, text_val, ln, text_direction);

	return 0;
}


static int eagle_read_circle(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_arc_t *circ;
	long ln = eagle_get_attrl(subtree, "layer", -1);
	eagle_layer_t *ly;

	switch(loc) {
		case IN_ELEM:
			if ((ln != 121) && (ln != 122) && (ln != 51) && (ln != 52)) /* consider silk circles only */
				return 0;
			circ = pcb_element_arc_alloc((pcb_element_t *)obj);
			if ((ln == 122) || (ln == 52))
				PCB_FLAG_SET(PCB_FLAG_ONSOLDER, circ);
			break;
		case ON_BOARD:
			ly = eagle_layer_get(st, ln);
			if (ly->ly < 0) {
				pcb_message(PCB_MSG_WARNING, "Ignoring circle on layer %s\n", ly->name);
				return 0;
			}
			circ = pcb_arc_alloc(pcb_get_layer(ly->ly));
			break;
	}
	circ->X = eagle_get_attrc(subtree, "x", -1);
	circ->Y = eagle_get_attrc(subtree, "y", -1);
	circ->Width = eagle_get_attrc(subtree, "radius", -1);
	circ->Height = circ->Width; /* no ellipse support */
	circ->StartAngle = 0;
	circ->Delta = 360;
	circ->Thickness = eagle_get_attrc(subtree, "width", -1);
	circ->Clearance = st->md_wire_wire*2;
	circ->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);


	switch(loc) {
		case IN_ELEM: break;
		case ON_BOARD:
			size_bump(st, circ->X + circ->Width + circ->Thickness, circ->Y + circ->Width + circ->Thickness);
			pcb_add_arc_on_layer(pcb_get_layer(ly->ly), circ);
			break;
	}

	return 0;
}

static int eagle_read_rect(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin1, *lin2, *lin3, *lin4;
	long ln = eagle_get_attrl(subtree, "layer", -1);
	eagle_layer_t *ly;
	unsigned long int flags;

	ly = eagle_layer_get(st, ln);
	switch(loc) {
		case IN_ELEM:
			if (ly->ly < 0)
				return 0;
			flags = pcb_layer_flags(st->pcb, ly->ly);
			if (!(flags & PCB_LYT_SILK)) /* consider silk lines only */
				return 0;
			lin1 = pcb_element_line_alloc((pcb_element_t *)obj);
			lin2 = pcb_element_line_alloc((pcb_element_t *)obj);
			lin3 = pcb_element_line_alloc((pcb_element_t *)obj);
			lin4 = pcb_element_line_alloc((pcb_element_t *)obj);
			if (flags & PCB_LYT_BOTTOM)
				PCB_FLAG_SET(PCB_FLAG_ONSOLDER, lin1);
				PCB_FLAG_SET(PCB_FLAG_ONSOLDER, lin2);
				PCB_FLAG_SET(PCB_FLAG_ONSOLDER, lin3);
				PCB_FLAG_SET(PCB_FLAG_ONSOLDER, lin4);
			break;
		case ON_BOARD:
			if (ly->ly < 0) {
				pcb_message(PCB_MSG_WARNING, "Ignoring rectangle on layer %s\n", ly->name);
				return 0;
			}
			lin1 = pcb_line_alloc(pcb_get_layer(ly->ly));
			lin2 = pcb_line_alloc(pcb_get_layer(ly->ly));
			lin3 = pcb_line_alloc(pcb_get_layer(ly->ly));
			lin4 = pcb_line_alloc(pcb_get_layer(ly->ly));
			break;
	}

	lin1->Point1.X = eagle_get_attrc(subtree, "x1", -1);
	lin1->Point1.Y = eagle_get_attrc(subtree, "y1", -1);
	lin1->Point2.X = eagle_get_attrc(subtree, "x2", -1);
	lin1->Point2.Y = lin1->Point1.Y;

	lin2->Point1.X = lin1->Point2.X;
	lin2->Point1.Y = lin1->Point2.Y;
	lin2->Point2.X = lin1->Point2.X;
	lin2->Point2.Y = eagle_get_attrc(subtree, "y2", -1);

	lin3->Point1.X = lin2->Point2.X;
	lin3->Point1.Y = lin2->Point2.Y;
	lin3->Point2.X = lin1->Point1.X;
	lin3->Point2.Y = lin2->Point2.Y;

	lin4->Point1.X = lin3->Point2.X;
	lin4->Point1.Y = lin3->Point2.Y;
	lin4->Point2.X = lin1->Point1.X;
	lin4->Point2.Y = lin1->Point1.Y;

#warning hard coded rectangle line thicknesses need to be changed to design rules value
 
	lin1->Thickness = PCB_MIL_TO_COORD(10);
	lin2->Thickness = lin1->Thickness;
	lin3->Thickness = lin1->Thickness;
	lin4->Thickness = lin1->Thickness;

	switch(loc) {
		case IN_ELEM:
			break;

		case ON_BOARD:
			size_bump(st, lin1->Point1.X + lin1->Thickness, lin1->Point1.Y + lin1->Thickness);
			size_bump(st, lin3->Point1.X + lin3->Thickness, lin3->Point1.Y + lin3->Thickness);
			pcb_add_line_on_layer(pcb_get_layer(ly->ly), lin1);
			pcb_add_line_on_layer(pcb_get_layer(ly->ly), lin2);
			pcb_add_line_on_layer(pcb_get_layer(ly->ly), lin3);
			pcb_add_line_on_layer(pcb_get_layer(ly->ly), lin4);

			break;
	}

	return 0;
}

static int eagle_read_wire(read_state_t * st, xmlNode * subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin;
	long ln = eagle_get_attrl(subtree, "layer", -1);
	eagle_layer_t *ly;
	unsigned long flags;

	ly = eagle_layer_get(st, ln);

	switch (loc) {
	case IN_ELEM:
		if (ly->ly < 0)
			return 0;
		flags = pcb_layer_flags(st->pcb, ly->ly);
		if (!(flags & PCB_LYT_SILK)) /* consider silk lines only */
			return 0;
		lin = pcb_element_line_alloc((pcb_element_t *) obj);
		if (flags & PCB_LYT_BOTTOM)
			PCB_FLAG_SET(PCB_FLAG_ONSOLDER, lin);
		break;
	case ON_BOARD:
		if (ly->ly < 0) {
			pcb_message(PCB_MSG_WARNING, "Ignoring wire on layer %s\n", ly->name);
			return 0;
		}
		lin = pcb_line_alloc(pcb_get_layer(ly->ly));
	}

	lin->Point1.X = eagle_get_attrc(subtree, "x1", -1);
	lin->Point1.Y = eagle_get_attrc(subtree, "y1", -1);
	lin->Point2.X = eagle_get_attrc(subtree, "x2", -1);
	lin->Point2.Y = eagle_get_attrc(subtree, "y2", -1);
	lin->Thickness = eagle_get_attrc(subtree, "width", -1);
	lin->Clearance = st->md_wire_wire*2;
	lin->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);
	lin->ID = pcb_create_ID_get();

	switch (loc) {
		case IN_ELEM:
			break;
		case ON_BOARD:
			size_bump(st, lin->Point1.X + lin->Thickness, lin->Point1.Y + lin->Thickness);
			size_bump(st, lin->Point2.X + lin->Thickness, lin->Point2.Y + lin->Thickness);
			pcb_add_line_on_layer(pcb_get_layer(ly->ly), lin);
			break;
	}

	return 0;
}

static int eagle_read_smd(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	pcb_coord_t x, y, dx, dy;
	pcb_pad_t *pad;
	long ln = eagle_get_attrl(subtree, "layer", -1);
	const char *name, *rot;

	assert(type == IN_ELEM);

	name = eagle_get_attrs(subtree, "name", NULL);
	x = eagle_get_attrc(subtree, "x", 0);
	y = eagle_get_attrc(subtree, "y", 0);
	dx = eagle_get_attrc(subtree, "dx", 0);
	dy = eagle_get_attrc(subtree, "dy", 0);

	rot = eagle_get_attrs(subtree, "rot", NULL);

	if (dx < 0) {
		x -= dx;
		dx = -dx;
	}

	if (dy < 0) {
		y -= dy;
		dy = -dy;
	}

	if (rot == NULL) {
		rot = "R0";
	}

	if ((rot != NULL) && (rot[0] == 'R')) { 
		int deg = atoi(rot+1);
		printf("smd pad rot? %s %d\n", rot, deg);
		switch(deg) {
			case 0:
				x -= dx/2;
				y -= dy/2;
				pad = pcb_element_pad_new_rect((pcb_element_t *)obj,
					x+dx, y+dy, x, y,
					conf_core.design.clearance, conf_core.design.clearance,
					name, name, pcb_flag_make(0));
				break;
			case 180:
				x -= dx/2;
				y -= dy/2;
				pad = pcb_element_pad_new_rect((pcb_element_t *)obj,
					x+dx, y+dy, x, y,
					conf_core.design.clearance, conf_core.design.clearance,
					name, name, pcb_flag_make(0));
				break;
			case 90:
				y -= dx/2;
				x -= dy/2;
				pad = pcb_element_pad_new_rect((pcb_element_t *)obj,
					x+dy, y+dx, x, y, /* swap coords for 90, 270 rotation */
					conf_core.design.clearance, conf_core.design.clearance,
					name, name, pcb_flag_make(0));
				break;
			case 270:
				y -= dx/2;
				x -= dy/2;
				pad = pcb_element_pad_new_rect((pcb_element_t *)obj,
					x+dy, y+dx, x, y, /* swap coords for 90, 270 rotation */
					conf_core.design.clearance, conf_core.design.clearance,
					name, name, pcb_flag_make(0));
				break;
			default:
				pcb_message(PCB_MSG_WARNING, "Ignored non-90 deg rotation of smd pad: %s\n", rot);
		}
	}

	pcb_printf("%mm %mm -> %mm %mm\n", x, y, dx, dy);

	if (ln == 16)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, pad);

	return 0;
}

static int eagle_read_pad_or_hole(read_state_t *st, xmlNode *subtree, void *obj, int type, int hole)
{
	eagle_loc_t loc = type;
	pcb_coord_t x, y, drill, dia;
	pcb_pin_t *pin;
	const char *name, *shape;

	name = eagle_get_attrs(subtree, "name", NULL);
	x = eagle_get_attrc(subtree, "x", 0);
	y = eagle_get_attrc(subtree, "y", 0);
	drill = eagle_get_attrc(subtree, "drill", 0);
	dia = eagle_get_attrc(subtree, "diameter", drill * (1.0+st->rv_pad_top*2.0));
	shape = eagle_get_attrs(subtree, "shape", 0);


	if ((dia - drill) / 2.0 < st->ms_width)
		dia = drill + 2*st->ms_width;

/*	pcb_printf("dia=%mm drill=%mm\n", dia, drill);*/

	switch((eagle_loc_t)type) {
		case IN_ELEM:
			pin = pcb_element_pin_new((pcb_element_t *)obj, x, y, dia,
				conf_core.design.clearance, 0, drill, name, name, pcb_no_flags());
			break;
		case ON_BOARD:
			pin = pcb_via_new(st->pcb->Data, x, y, dia,
				conf_core.design.clearance, 0, drill, name, pcb_no_flags());
			break;
	}

	if (hole)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, pin);

	if ((shape != NULL) && (strcmp(shape, "octagon") == 0))
		PCB_FLAG_SET(PCB_FLAG_OCTAGON, pin);
	else if ((shape != NULL) && (strcmp(shape, "square") == 0))
		PCB_FLAG_SET(PCB_FLAG_SQUARE, pin);
	else
		PCB_FLAG_SET(PCB_FLAG_VIA, pin);

	switch(loc) {
		case IN_ELEM: break;
		case ON_BOARD:
			size_bump(st, x + dia, y + dia);
			break;
	}

	return 0;
}

static int eagle_read_hole(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, type, 1);
}

static int eagle_read_pad(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, type, 0);
}

static int eagle_read_via(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, ON_BOARD, 0);
}


/****************** composite objects ******************/

static int eagle_read_pkg(read_state_t *st, xmlNode *subtree, pcb_element_t *elem)
{
	static const dispatch_t disp[] = { /* possible children of <board> */
		{"description", eagle_read_nop},
		{"wire",        eagle_read_wire},
		{"hole",        eagle_read_hole},
		{"circle",      eagle_read_circle},
		{"smd",         eagle_read_smd},
		{"pad",         eagle_read_pad},
		{"text",        eagle_read_nop},
		{"rectangle",   eagle_read_rect},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};
	printf("   read pkg: TODO\n");
	return eagle_foreach_dispatch(st, subtree->children, disp, elem, IN_ELEM);
}

static int eagle_read_lib_pkgs(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	xmlNode *n;
	eagle_library_t *lib = obj;

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"package") == 0) {
			const char *name = eagle_get_attrs(n, "name", NULL);
			pcb_element_t *elem;
			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Ignoring package with no name\n");
				continue;
			}
			printf(" pkg %s\n", name);
			elem = calloc(sizeof(pcb_element_t), 1);
			eagle_read_pkg(st, n, elem);
			if (pcb_element_is_empty(elem)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring empty package %s\n", name);
				free(elem);
				continue;
			}
			htsp_set(&lib->elems, (char *)name, elem);
		}
	}
	return 0;
}

static int eagle_read_libs(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	xmlNode *n;
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"description", eagle_read_nop},
		{"packages",    eagle_read_lib_pkgs},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"library") == 0) {
			const char *name = eagle_get_attrs(n, "name", NULL);
			eagle_library_t *lib;
			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Ignoring library with no name\n");
				continue;
			}
			lib = calloc(sizeof(eagle_library_t), 1);
			printf("Name: %s\n", name);
			htsp_init(&lib->elems, strhash, strkeyeq);
			eagle_foreach_dispatch(st, n->children, disp, lib, 0);
			htsp_set(&st->libs, (char *)name, lib);
		}
	}
	return 0;
}

static int eagle_read_contactref(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	const char *elem, *pad, *net;
	char conn[256];

	elem = eagle_get_attrs(subtree, "element", NULL);
	pad = eagle_get_attrs(subtree, "pad", NULL);


	if ((elem == NULL) || (pad == NULL)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring contactref: missing element or pad\n");
		return 0;
	}

	net = eagle_get_attrs(subtree->parent, "name", NULL);

	pcb_snprintf(conn, sizeof(conn), "%s-%s", elem, pad);

	pcb_hid_actionl("Netlist", "Add",  net, conn, NULL);
	return 0;
}


static int eagle_read_poly(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	eagle_layer_t *ly;
	long ln = eagle_get_attrl(subtree, "layer", -1);
	pcb_polygon_t *poly;
	xmlNode *n;

	ly = eagle_layer_get(st, ln);
	if (ly->ly < 0) {
		pcb_message(PCB_MSG_WARNING, "Ignoring polygon on layer %s\n", ly->name);
		return 0;
	}

	poly = pcb_poly_new(&st->pcb->Data->Layer[ly->ly], pcb_flag_make(PCB_FLAG_CLEARPOLY));

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"vertex") == 0) {
			pcb_coord_t x, y;
			x = eagle_get_attrc(n, "x", 0);
			y = eagle_get_attrc(n, "y", 0);
			pcb_poly_point_new(poly, x, y);
		}
	}

	pcb_add_polygon_on_layer(&st->pcb->Data->Layer[ly->ly], poly);
	pcb_poly_init_clip(st->pcb->Data, &st->pcb->Data->Layer[ly->ly], poly);

	return 0;
}


static int eagle_read_signals(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	xmlNode *n;
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"contactref",  eagle_read_contactref},
		{"wire",        eagle_read_wire},
		{"polygon",     eagle_read_poly},
		{"via",         eagle_read_via},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"signal") == 0) {
			const char *name = eagle_get_attrs(n, "name", NULL);
			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Ignoring signal with no name\n");
				continue;
			}
			eagle_foreach_dispatch(st, n->children, disp, (char *)name, ON_BOARD);
		}
	}

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

	return 0;
}

static void eagle_read_elem_text(read_state_t *st, pcb_element_t *elem, pcb_text_t *text, pcb_coord_t x, pcb_coord_t y, const char *attname, const char *str)
{
	int Direction = 0, TextScale = 100;
			pcb_flag_t TextFlags = pcb_no_flags();

	pcb_element_text_set(text, pcb_font(st->pcb, 0, 1), x, y, Direction, str, TextScale, TextFlags);
	text->Element = elem;
}

static int eagle_read_elements(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	xmlNode *n;

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"element") == 0) {
			pcb_coord_t x, y;
			const char *name = eagle_get_attrs(n, "name", NULL);
			const char *lib = eagle_get_attrs(n, "library", NULL);
			const char *pkg = eagle_get_attrs(n, "package", NULL);
			const char *val = eagle_get_attrs(n, "value", NULL);
			pcb_element_t *elem, *new_elem;
			const char *rot;

			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Ignoring element with no name\n");
				continue;
			}
			if ((lib == NULL) || (pkg == NULL)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring element with incomplete library reference\n");
				continue;
			}

			elem = eagle_libelem_get(st, lib, pkg);
			if (elem == NULL) {
				pcb_message(PCB_MSG_WARNING, "Library element not found: %s/%s\n", lib, pkg);
				continue;
			}
			if (pcb_element_is_empty(elem)) {
				pcb_message(PCB_MSG_WARNING, "Not placing empty element: %s/%s\n", lib, pkg);
				continue;
			}

			x = eagle_get_attrc(n, "x", -1);
			y = eagle_get_attrc(n, "y", -1);
			rot = eagle_get_attrs(n, "rot", NULL);

#warning TODO: use pcb_elem_new() instead of this?
			new_elem = pcb_element_alloc(st->pcb->Data);
			pcb_element_copy(st->pcb->Data, new_elem, elem, pcb_false, x, y);
			new_elem->Flags = pcb_no_flags();
			new_elem->ID = pcb_create_ID_get();

			PCB_ELEMENT_PCB_TEXT_LOOP(new_elem);
			{
				if (st->pcb->Data && st->pcb->Data->name_tree[n])
					pcb_r_delete_entry(st->pcb->Data->name_tree[n], (pcb_box_t *) text);
			}
			PCB_END_LOOP;

			eagle_read_elem_text(st, new_elem, &PCB_ELEM_TEXT_DESCRIPTION(new_elem), x, y, "PROD_ID", pkg);
			eagle_read_elem_text(st, new_elem, &PCB_ELEM_TEXT_REFDES(new_elem), x, y, "NAME", name);
			eagle_read_elem_text(st, new_elem, &PCB_ELEM_TEXT_VALUE(new_elem), x, y, "VALUE", val);


			if ((rot != NULL) && (rot[0] == 'R')) {
				int deg = atoi(rot+1);
				printf("rot? %s %d\n", rot, deg);
				switch(deg) {
					case 0: break;
					case 90: pcb_element_rotate90(st->pcb->Data, new_elem, x, y, 3); break;
					case 180: pcb_element_rotate90(st->pcb->Data, new_elem, x, y, 2); break;
					case 270: pcb_element_rotate90(st->pcb->Data, new_elem, x, y, 1); break;
					default:
						pcb_message(PCB_MSG_WARNING, "Ignored non-90 deg rotation: %s/%s\n", lib, pkg);
				}
			}

			pcb_element_bbox(st->pcb->Data, new_elem, pcb_font(st->pcb, 0, 1));
			size_bump(st, new_elem->BoundingBox.X2, new_elem->BoundingBox.Y2);

			printf("placing %s: %s/%s -> %p\n", name, lib, pkg, (void *)elem);
		}
	}
	return 0;
}

static int eagle_read_board(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <board> */
		{"plain",       eagle_read_nop},
		{"libraries",   eagle_read_libs},
		{"attributes",  eagle_read_nop},
		{"variantdefs", eagle_read_nop},
		{"classes",     eagle_read_nop},
		{"designrules", eagle_read_nop},
		{"autorouter",  eagle_read_nop},
		{"elements",    eagle_read_elements},
		{"signals",     eagle_read_signals},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};
	return eagle_foreach_dispatch(st, subtree->children, disp, NULL, 0);
}


static int eagle_read_drawing(read_state_t *st, xmlNode *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <drawing> */
		{"settings",  eagle_read_nop},
		{"grid",      eagle_read_nop},
		{"layers",    eagle_read_layers},
		{"board",     eagle_read_board},
		{"@text",     eagle_read_nop},
		{NULL, NULL}
	};
	return eagle_foreach_dispatch(st, subtree->children, disp, NULL, 0);
}

static int eagle_read_design_rules(read_state_t *st, xmlNode *subtree)
{
	xmlNode *n;
	const char *name;

	for(n = subtree->children; n != NULL; n = n->next) {
		if (xmlStrcmp(n->name, (xmlChar *)"param") != 0)
			continue;
		name = eagle_get_attrs(n, "name", NULL);
		if (strcmp(name, "mdWireWire") == 0) st->md_wire_wire = eagle_get_attrcu(n, "value", 0);
		else if (strcmp(name, "msWidth") == 0) st->ms_width = eagle_get_attrcu(n, "value", 0);
		else if (strcmp(name, "rvPadTop") == 0) st->rv_pad_top = eagle_get_attrd(n, "value", 0);
		else if (strcmp(name, "rvPadInner") == 0) st->rv_pad_inner = eagle_get_attrd(n, "value", 0);
		else if (strcmp(name, "rvPadBottom") == 0) st->rv_pad_bottom = eagle_get_attrd(n, "value", 0);
	}
	if ((st->rv_pad_top != st->rv_pad_inner) || (st->rv_pad_top != st->rv_pad_inner))
		pcb_message(PCB_MSG_WARNING, "top/inner/bottom default pad sizes differ - using top size only\n");
	return 0;
}

static int eagle_read_ver(xmlChar *ver)
{
	int v1, v2, v3;
	char *end;

	if (ver == NULL) {
		pcb_message(PCB_MSG_ERROR, "no version attribute in <eagle>\n");
		return -1;
	}

	v1 = strtol((char *)ver, &end, 10);
	if (*end != '.') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [1] in <eagle>\n");
		return -1;
	}
	v2 = strtol((char *)end+1, &end, 10);
	if (*end != '.') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [2] in <eagle>\n");
		return -1;
	}
	v3 = strtol((char *)end+1, &end, 10);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [3] in <eagle>\n");
		return -1;
	}

	/* version check */
	if (v1 < 6) {
		pcb_message(PCB_MSG_ERROR, "file version too old\n");
		return -1;
	}
	if (v1 > 7) {
		pcb_message(PCB_MSG_ERROR, "file version too new\n");
		return -1;
	}
	pcb_message(PCB_MSG_DEBUG, "Loading eagle board version %d.%d.%d\n", v1, v2, v3);
	return 0;
}

static void st_init(read_state_t *st)
{
	htip_init(&st->layers, longhash, longkeyeq);
	htsp_init(&st->libs, strhash, strkeyeq);
	pcb_layer_group_setup_default(&st->pcb->LayerGroups);
}

static void st_uninit(read_state_t *st)
{
	htip_entry_t *ei;
	htsp_entry_t *es;

	pcb_layergrp_fix_old_outline(st->pcb);

	for (ei = htip_first(&st->layers); ei; ei = htip_next(&st->layers, ei))
		free(ei->value);
	htip_uninit(&st->layers);

	for (es = htsp_first(&st->libs); es; es = htsp_next(&st->libs, es)) {
		htsp_entry_t *e;
		eagle_library_t *l = es->value;
		for (e = htsp_first(&l->elems); e; e = htsp_next(&l->elems, e))
			free(e->value);
		htsp_uninit(&l->elems);
		free(l);
	}
	htsp_uninit(&st->libs);

}

int io_eagle_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	xmlDoc *doc;
	xmlNode *root, *dr;
	int res, old_leni;
	read_state_t st;

	static const dispatch_t disp[] = { /* possible children of root */
		{"drawing",        eagle_read_drawing},
		{"compatibility",  eagle_read_nop},
		{"@text",          eagle_read_nop},
		{NULL, NULL}
	};

	doc = xmlReadFile(Filename, NULL, 0);
	if (doc == NULL) {
		pcb_message(PCB_MSG_ERROR, "xml parsing error\n");
		return -1;
	}

	root = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root->name, (xmlChar *)"eagle") != 0) {
		pcb_message(PCB_MSG_ERROR, "xml error: root is not <eagle>\n");
		goto err;
	}

	if (eagle_read_ver(xmlGetProp(root, (xmlChar *)"version")) < 0)
		goto err;

	st.doc = doc;
	st.root = root;
	st.pcb = pcb;

	st_init(&st);

	dr = eagle_xml_path(root, "drawing", "board", "designrules", NULL);
	if (dr != NULL)
		eagle_read_design_rules(&st, dr);
	else
		pcb_message(PCB_MSG_WARNING, "can't find design rules\n");

	old_leni = pcb_create_being_lenient;
	pcb_create_being_lenient = 1;
	res = eagle_foreach_dispatch(&st, root->children, disp, NULL, 0);
	if (res == 0)
		pcb_flip_data(pcb->Data, 0, 1, 0, pcb->MaxHeight, 0);
	pcb_create_being_lenient = old_leni;

	st_uninit(&st);

	pcb_trace("Houston, the Eagle has landed. %d\n", res);

	xmlFreeDoc(doc);
	return 0;

err:;
	xmlFreeDoc(doc);
	return -1;
}

