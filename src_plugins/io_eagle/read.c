/*
 *				COPYRIGHT
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

#include "../src_plugins/boardflip/boardflip.h"

#include "board.h"
#include "read.h"
#include "conf.h"
#include "conf_core.h"
#include "error.h"
#include "polygon.h"
#include "rtree.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "trparse.h"
#include "trparse_xml.h"
#include "trparse_bin.h"

/* coordinates that corresponds to pcb-rnd 100% text size in height */
#define EAGLE_TEXT_SIZE_100 PCB_MIL_TO_COORD(50)

#define PARENT(node)   st->parser.calls->parent(&st->parser, node)
#define CHILDREN(node) st->parser.calls->children(&st->parser, node)
#define NEXT(node)     st->parser.calls->next(&st->parser, node)
#define NODENAME(node) st->parser.calls->nodename(node)

#define GET_PROP(node, key) st->parser.calls->prop(&st->parser, node, key)
#define GET_PROP_(st, node, key) (st)->parser.calls->prop(&((st)->parser), node, key)
#define GET_TEXT(node)      st->parser.calls->text(&st->parser, node)

#define STRCMP(s1, s2) st->parser.calls->str_cmp(s1,s2)
#define IS_TEXT(node)  st->parser.calls->is_text(&st->parser, node)

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
	trparse_t parser;
	pcb_board_t *pcb;

	htip_t layers;
	htsp_t libs;

	/* current element refdes X, Y, height, and value X, Y, height */
	pcb_coord_t refdes_x, refdes_y, refdes_scale, value_x, value_y, value_scale;

	/* design rules */
	pcb_coord_t md_wire_wire; /* minimal distance between wire and wire (clearance) */
	pcb_coord_t ms_width; /* minimal trace width */
	double rv_pad_top, rv_pad_inner, rv_pad_bottom; /* pad size-to-drill ration on different layers */

	const char *default_unit; /* assumed unit for unitless coord values */
	unsigned elem_by_name:1; /* whether elements are addressed by name (or by index in the lib) */
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser)(read_state_t *st, trnode_t *subtree, void *obj, int type);
} dispatch_t;

typedef enum {
	IN_ELEM = 1,
	ON_BOARD
} eagle_loc_t;


/* Xml path walk that's much simpler than xpath; the ... is a NULL
   terminated list of node names */
static trnode_t *eagle_trpath(read_state_t *st, trnode_t *subtree, ...)
{
	trnode_t *nd = subtree;
	const char *target;
	va_list ap;

	va_start(ap, subtree);

	/* get next path element */
	while((target = va_arg(ap, const char *)) != NULL) {
		/* look for target on this level */
		for(nd = CHILDREN(nd);;nd = NEXT(nd)) {
			if (nd == NULL) {/* target not found on this level */
				va_end(ap);
				return NULL;
			}
			if (STRCMP(NODENAME(nd), target) == 0) /* found, skip to next level */
				break;
		}
	}

	va_end(ap);
	return nd;
}

/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int eagle_dispatch(read_state_t *st, trnode_t *subtree, const dispatch_t *disp_table, void *obj, int type)
{
	const dispatch_t *d;
	const char *name;

	/* do not tolerate empty/NIL node */
	if (NODENAME(subtree) == NULL)
		return -1;

	if (IS_TEXT(subtree))
		name = "@text";
	else
		name = NODENAME(subtree);

	for(d = disp_table; d->node_name != NULL; d++)
		if (STRCMP(d->node_name, name) == 0)
			return d->parser(st, subtree, obj, type);

	pcb_message(PCB_MSG_ERROR, "eagle: unknown node: '%s'\n", name);
	/* node name not found in the dispatcher table */
	return -1;
}

/* Take each children of tree and execute them using eagle_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */
static int eagle_foreach_dispatch(read_state_t *st, trnode_t *tree, const dispatch_t *disp_table, void *obj, int type)
{
	trnode_t *n;

	for(n = tree; n != NULL; n = NEXT(n))
		if (eagle_dispatch(st, n, disp_table, obj, type) != 0)
			return -1;

	return 0; /* success */
}

/* No-op: ignore the subtree */
static int eagle_read_nop(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	return 0;
}



int io_eagle_test_parse_pcb_xml(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
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

int io_eagle_test_parse_pcb_bin(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	unsigned char buff[2];
	int read_length = fread(buff, 1, 2, f);
	if ((read_length == 2) && (buff[0] == 0x10) && (buff[1] == 0x00)) {
		return 1; /* Eagle v4, v5 */ 
	} else if ((read_length == 2) && (buff[0] == 0x10) && (buff[1] == 0x80)) {
		return 1; /* Eagle v3 */
	}
	return 0;
}

/* Return a node attribute value converted to long, or return invalid_val
   for synatx error or if the attribute doesn't exist */
static long eagle_get_attrl(read_state_t *st, trnode_t *nd, const char *name, long invalid_val)
{
	const char *p = GET_PROP(nd, name);
	char *end;
	long res;

	if (p == NULL)
		return invalid_val;
	res = strtol(p, &end, 10);
	if (*end != '\0')
		return invalid_val;
	return res;
}

/* Return a node attribute value converted to double, or return invalid_val
   for synatx error or if the attribute doesn't exist */
static double eagle_get_attrd(read_state_t *st, trnode_t *nd, const char *name, double invalid_val)
{
	const char *p = GET_PROP(nd, name);
	char *end;
	double res;

	if (p == NULL)
		return invalid_val;
	res = strtod(p, &end);
	if (*end != '\0')
		return invalid_val;
	return res;
}

/* Return a node attribute value converted to char *, or return invalid_val
   if the attribute doesn't exist */
static const char *eagle_get_attrs(read_state_t *st, trnode_t *nd, const char *name, const char *invalid_val)
{
	const char *p = GET_PROP(nd, name);
	if (p == NULL)
		return invalid_val;
	return p;
}

/* Return a node attribute value converted to coord, or return invalid_val
   if the attribute doesn't exist */
static pcb_coord_t eagle_get_attrc(read_state_t *st, trnode_t *nd, const char *name, pcb_coord_t invalid_val)
{
	const char *p = GET_PROP(nd, name);
	pcb_coord_t c;
	pcb_bool succ;

	if (p == NULL)
		return invalid_val;

	c = pcb_get_value(p, st->default_unit, NULL, &succ);
	if (!succ)
		return invalid_val;
	return c;
}

/* same as eagle_get_attrc() but assume the input has units */
static pcb_coord_t eagle_get_attrcu(read_state_t *st, trnode_t *nd, const char *name, pcb_coord_t invalid_val)
{
	const char *p = GET_PROP(nd, name);
	pcb_coord_t c;
	pcb_bool succ;

	if (p == NULL)
		return invalid_val;

	c = pcb_get_value(p, NULL, NULL, &succ);
	if (!succ)
		return invalid_val;
	return c;
}

static int eagle_read_layers(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "layer") == 0) {
			eagle_layer_t *ly = calloc(sizeof(eagle_layer_t), 1);
			int id, reuse = 0;
			unsigned long typ;
			pcb_layergrp_id_t gid;
			pcb_layergrp_t *grp;

			ly->name    = eagle_get_attrs(st, n, "name", NULL);
			ly->color   = eagle_get_attrl(st, n, "color", -1);
			ly->fill    = eagle_get_attrl(st, n, "fill", -1);
			ly->visible = eagle_get_attrl(st, n, "visible", -1);
			ly->active  = eagle_get_attrl(st, n, "active", -1);
			ly->ly      = -1;
			id = eagle_get_attrl(st, n, "number", -1);
			if (id >= 0)
				htip_set(&st->layers, id, ly);

			typ = 0;
			switch(id) {
				case 1: typ = PCB_LYT_COPPER | PCB_LYT_TOP; break;
				case 16: typ = PCB_LYT_COPPER | PCB_LYT_BOTTOM; break;
				case 121:
				case 21: /* tplace element silk */
				case 25: /* names */
				case 27: /* values */
				case 39: /* keepout */
					reuse = 1;
					typ = PCB_LYT_SILK | PCB_LYT_TOP;
					break;
				case 122:
				case 22: /* bplace element silk */
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

static pcb_element_t *eagle_libelem_by_name(read_state_t *st, const char *lib, const char *elem)
{
	eagle_library_t *l;
	l = htsp_get(&st->libs, lib);
	if (l == NULL)
		return NULL;
	return htsp_get(&l->elems, elem);
}

static pcb_element_t *eagle_libelem_by_idx(read_state_t *st, trnode_t *libs, long libi, long pkgi)
{
	trnode_t *n;
	pcb_element_t *res;

	/* count children of libs so n ends up at the libith library */
	for(n = CHILDREN(libs); (n != NULL) && (libi > 1); n = NEXT(n), libi--) ;
	if (n == NULL) {
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_libelem_by_idx() can't find lib by idx:\n");
		return NULL;
	}

	if (STRCMP(NODENAME(n), "library") != 0) {
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_libelem_by_idx() expected library node:\n");
		return NULL;
	}
	n = CHILDREN(n);

	if (STRCMP(NODENAME(n), "packages") != 0) {
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_libelem_by_idx() expected packages node:\n");
		return NULL;
	}

	/* count children of that library so n ends up at the pkgth package */
	for(n = CHILDREN(n); (n != NULL) && (pkgi > 1); n = NEXT(n), pkgi--) ;
	if (n == NULL) {
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_libelem_by_idx() can't find pkg by idx:\n");
		return NULL;
	}

	res = st->parser.calls->get_user_data(n);
	if (res == NULL)
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_libelem_by_idx() found the element node in the tree but there's no element instance associated with it:\n");
	return res;
}

static void size_bump(read_state_t *st, pcb_coord_t x, pcb_coord_t y)
{
	if (x > st->pcb->MaxWidth)
		st->pcb->MaxWidth = x;
	if (y > st->pcb->MaxHeight)
		st->pcb->MaxHeight = y;
}

/* Convert eagle Rxxx or binary n*1024 string to degrees */
static int eagle_rot2degrees(const char *rot)
{
	int deg;
	char *end;

	if (rot == NULL) {
		return -1;
	} else if (rot[0] != 'R') {
		deg = strtol(rot, &end, 10);
		if (*end != '\0')
			return -1;
		deg = deg*(double)360/4096;
	} else {
		deg = strtol(rot+1, &end, 10);
		if (*end != '\0')
			return -1;
	}
	while (deg > 360) {
		deg -= 360;
	}
	return (int) deg;
}

/* Convert eagle Rxxx string to pcb-rnd 90-deg rotation steps */
static int eagle_rot2steps(const char *rot)
{
	int deg = eagle_rot2degrees(rot);

	switch(deg) {
		case 0: return 0;
		case 90: return 3;
		case 180: return 2;
		case 270: return 1;
	}
	return -1;
}

/****************** drawing primitives ******************/


static int eagle_read_text(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_coord_t X, Y, height;
	const char *rot, *text_val;
	unsigned int text_direction = 0, text_scaling = 100;

	if (CHILDREN(subtree) == NULL) {
		pcb_message(PCB_MSG_WARNING, "Ignoring empty text field\n");
		return 0;
	}
	if (!IS_TEXT(CHILDREN(subtree))) {
		pcb_message(PCB_MSG_WARNING, "Ignoring text field (invalid child node)\n");
		return 0;
	}

#warning TODO: need to convert
	text_val = (const char *)GET_TEXT(CHILDREN(subtree));
	X = eagle_get_attrc(st, subtree, "x", -1);
	Y = eagle_get_attrc(st, subtree, "y", -1);
	height = eagle_get_attrc(st, subtree, "size", -1);
	text_scaling = (int)((double)height/(double)EAGLE_TEXT_SIZE_100 * 100);
	rot = eagle_get_attrs(st, subtree, "rot", NULL);
	if (rot == NULL) {
		rot = "R0";
	}

	if (rot[0] == 'R') {
		int deg = atoi(rot+1);
		if (deg < 45 || deg >= 315) {
			text_direction = 0;
		}
		else if (deg < 135 && deg >= 45) {
			text_direction = 1;
		}
		else if (deg < 225 && deg >= 135) {
			text_direction = 2;
		}
		else {
			text_direction = 3;
		}
	}

	pcb_trace("\ttext found on Eagle layout at with rot: %s at %mm;%mm %mm: '%s' ln=%d dir=%d\n", rot, X, Y, text_scaling, text_val, ln, text_direction);

	return 0;
}


static int eagle_read_circle(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_arc_t *circ;
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
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
	circ->X = eagle_get_attrc(st, subtree, "x", -1);
	circ->Y = eagle_get_attrc(st, subtree, "y", -1);
	circ->Width = eagle_get_attrc(st, subtree, "radius", -1);
	circ->Height = circ->Width; /* no ellipse support */
	circ->StartAngle = 0;
	circ->Delta = 360;
	circ->Thickness = eagle_get_attrc(st, subtree, "width", -1);
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

static int eagle_read_rect(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin1, *lin2, *lin3, *lin4;
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
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

	lin1->Point1.X = eagle_get_attrc(st, subtree, "x1", -1);
	lin1->Point1.Y = eagle_get_attrc(st, subtree, "y1", -1);
	lin1->Point2.X = eagle_get_attrc(st, subtree, "x2", -1);
	lin1->Point2.Y = lin1->Point1.Y;

	lin2->Point1.X = lin1->Point2.X;
	lin2->Point1.Y = lin1->Point2.Y;
	lin2->Point2.X = lin1->Point2.X;
	lin2->Point2.Y = eagle_get_attrc(st, subtree, "y2", -1);

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

static int eagle_read_wire(read_state_t * st, trnode_t * subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin;
	eagle_layer_t *ly;
	unsigned long flags;
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
	long lt = eagle_get_attrl(st, subtree, "linetype", -1); /* present if bin file */

	if (lt != -1) {
		pcb_trace("Found wire type %ld\n", lt);
	}
	else {
		pcb_trace("Found null wire type 'lt'\n");
	}
	if (ln != -1) {
		pcb_trace("Found wire layer %ld\n", ln);
	}
	else {
		pcb_trace("Found null wire layer number 'ln'");
	}

	ly = eagle_layer_get(st, ln); 
	if (ly != NULL) {
		pcb_trace("Allocated layer 'ly' via eagle_layer_get(st, ln)\n");
	}
	else {
		pcb_trace("Failed to allocate layer 'ly' via eagle_layer_get(st, ln)\n");
		return 0;
	}

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
	lin->Point1.X = eagle_get_attrc(st, subtree, "x1", -1);
	lin->Point1.Y = eagle_get_attrc(st, subtree, "y1", -1);
	lin->Point2.X = eagle_get_attrc(st, subtree, "x2", -1);
	lin->Point2.Y = eagle_get_attrc(st, subtree, "y2", -1);
#warning TODO can remove if width dealt with in binary tree post processor code, but this is v. simple:
	lin->Thickness = eagle_get_attrc(st, subtree, "width", -1); /* bin format is half the width*/
	lin->Thickness += eagle_get_attrc(st, subtree, "width_doubling_bin", 0);
	pcb_trace("new line thickness: %ml\n", lin->Thickness);
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

static int eagle_read_smd(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	pcb_coord_t x, y, dx, dy;
	pcb_pad_t *pad;
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
	const char *name, *rot;
	int deg = 0;

	assert(type == IN_ELEM);

	name = eagle_get_attrs(st, subtree, "name", NULL);
	x = eagle_get_attrc(st, subtree, "x", 0);
	y = eagle_get_attrc(st, subtree, "y", 0);
	dx = eagle_get_attrc(st, subtree, "dx", 0);
	dy = eagle_get_attrc(st, subtree, "dy", 0);

	rot = eagle_get_attrs(st, subtree, "rot", NULL);
	deg = eagle_rot2degrees(rot);

	if (dx < 0) {
		x -= dx;
		dx = -dx;
	}

	if (dy < 0) {
		y -= dy;
		dy = -dy;
	}

	if (rot == NULL) {
		deg = 0;
	} else {
		deg = eagle_rot2degrees(rot);
	}

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

	pcb_trace("%mm %mm -> %mm %mm\n", x, y, dx, dy);

	if (ln == 16)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, pad);

	return 0;
}

static int eagle_read_pad_or_hole(read_state_t *st, trnode_t *subtree, void *obj, int type, int hole)
{
	eagle_loc_t loc = type;
	pcb_coord_t x, y, drill, dia;
	pcb_pin_t *pin;
	const char *name, *shape;

	name = eagle_get_attrs(st, subtree, "name", NULL);
	x = eagle_get_attrc(st, subtree, "x", 0);
	y = eagle_get_attrc(st, subtree, "y", 0);
	drill = eagle_get_attrc(st, subtree, "drill", 0);
	dia = eagle_get_attrc(st, subtree, "diameter", drill * (1.0+st->rv_pad_top*2.0));
	shape = eagle_get_attrs(st, subtree, "shape", 0);


	if ((dia - drill) / 2.0 < st->ms_width)
		dia = drill + 2*st->ms_width;

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

static int eagle_read_hole(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, type, 1);
}

static int eagle_read_pad(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, type, 0);
}

static int eagle_read_via(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	return eagle_read_pad_or_hole(st, subtree, obj, ON_BOARD, 0);
}


/****************** composite objects ******************/

/* Save the relative coords and size of the each relevant text fields */
static int eagle_read_pkg_txt(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	pcb_coord_t size;
	trnode_t *n;
	const char *cont;

#warning subc TODO subcircuits will allow distinct refdes, descr and value text field attributes
/*	pcb_text_t *t;*/
/*	pcb_element_t *elem = obj;*/

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n))
		if (IS_TEXT(n))
			break;

	if ((n == NULL) || ((cont = GET_TEXT(n)) == NULL))
		return 0;

	if (STRCMP(cont, ">NAME") == 0) {
/*		t = &elem->Name[PCB_ELEMNAME_IDX_REFDES];*/
		size = eagle_get_attrc(st, subtree, "size", EAGLE_TEXT_SIZE_100);
		st->refdes_scale = (int)(((double)size/ (double)EAGLE_TEXT_SIZE_100) * 100.0);
		st->refdes_x = eagle_get_attrc(st, subtree, "x", 0);
		st->refdes_y = eagle_get_attrc(st, subtree, "y", 0);
	} else if (STRCMP(cont, ">VALUE") == 0) {
/*		t = &elem->Name[PCB_ELEMNAME_IDX_VALUE];*/
		size = eagle_get_attrc(st, subtree, "size", EAGLE_TEXT_SIZE_100);
		st->value_scale = (int)(((double)size/ (double)EAGLE_TEXT_SIZE_100) * 100.0);
		st->value_x = eagle_get_attrc(st, subtree, "x", 0);
		st->value_y = eagle_get_attrc(st, subtree, "y", 0); 
	} else {
		return 0;
	}

	return 0;
}

static int eagle_read_pkg(read_state_t *st, trnode_t *subtree, pcb_element_t *elem)
{
	static const dispatch_t disp[] = { /* possible children of package */
		{"description", eagle_read_nop},
		{"wire",        eagle_read_wire},
		{"hole",        eagle_read_hole},
		{"circle",      eagle_read_circle},
		{"smd",         eagle_read_smd},
		{"pad",         eagle_read_pad},
		{"text",        eagle_read_pkg_txt},
		{"rectangle",   eagle_read_rect},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
#warning subc TODO can dd polygon to package
	};
	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, elem, IN_ELEM);
}

static int eagle_read_lib_pkgs(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	eagle_library_t *lib = obj;
	pcb_text_t *t;

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "package") == 0) {
			const char *name = eagle_get_attrs(st, n, "name", NULL);
			pcb_element_t *elem;
			if ((st->elem_by_name) && (name == NULL)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring package with no name\n");
				continue;
			}

			elem = calloc(sizeof(pcb_element_t), 1);
			eagle_read_pkg(st, n, elem);
			if (pcb_element_is_empty(elem)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring empty package %s\n", name);
				free(elem);
				continue;
			}
#warning subc TODO subcircuits can have distinct refdes, value, description text attributes
			t = &elem->Name[PCB_ELEMNAME_IDX_VALUE];
			t->X = st->refdes_x;
			t->Y = st->refdes_y;
			t->Scale = st->refdes_scale;
			t = &elem->Name[PCB_ELEMNAME_IDX_REFDES];
			t->X = st->refdes_x;
			t->Y = st->refdes_y;
			t->Scale = st->refdes_scale;
			t = &elem->Name[PCB_ELEMNAME_IDX_DESCRIPTION];
			t->X = st->refdes_x;
			t->Y = st->refdes_y;
			t->Scale = st->refdes_scale;

			if (st->elem_by_name)
				htsp_set(&lib->elems, (char *)name, elem);
			st->parser.calls->set_user_data(n, elem);
		}
	}
	return 0;
}

static int eagle_read_libs(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"description", eagle_read_nop},
		{"packages",    eagle_read_lib_pkgs},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "library") == 0) {
			const char *name = eagle_get_attrs(st, n, "name", NULL);
			eagle_library_t *lib;
			if ((st->elem_by_name) && (name == NULL)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring library with no name\n");
				continue;
			}
			lib = calloc(sizeof(eagle_library_t), 1);

			if (st->elem_by_name)
				htsp_init(&lib->elems, strhash, strkeyeq);

			eagle_foreach_dispatch(st, CHILDREN(n), disp, lib, 0);

			if (st->elem_by_name)
				htsp_set(&st->libs, (char *)name, lib);
		}
	}
	return 0;
}

static int eagle_read_contactref(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	const char *elem, *pad, *net;
	char conn[256];

	elem = eagle_get_attrs(st, subtree, "element", NULL);
	pad = eagle_get_attrs(st, subtree, "pad", NULL);


	if ((elem == NULL) || (pad == NULL)) {
		pcb_message(PCB_MSG_WARNING, "Ignoring contactref: missing element or pad\n");
		return 0;
	}

	net = eagle_get_attrs(st, PARENT(subtree), "name", NULL);

	pcb_snprintf(conn, sizeof(conn), "%s-%s", elem, pad);

	pcb_hid_actionl("Netlist", "Add",  net, conn, NULL);
	return 0;
}


static int eagle_read_poly(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	eagle_layer_t *ly;
	long ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_polygon_t *poly;
	trnode_t *n;

	ly = eagle_layer_get(st, ln);
	if (ly->ly < 0) {
		pcb_message(PCB_MSG_WARNING, "Ignoring polygon on layer %s\n", ly->name);
		return 0;
	}

	poly = pcb_poly_new(&st->pcb->Data->Layer[ly->ly], pcb_flag_make(PCB_FLAG_CLEARPOLY));

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "vertex") == 0) {
			pcb_coord_t x, y;
			x = eagle_get_attrc(st, n, "x", 0);
			y = eagle_get_attrc(st, n, "y", 0);
			pcb_poly_point_new(poly, x, y);
			switch (loc) {
				case IN_ELEM:
					break;
				case ON_BOARD:
					size_bump(st, x, y);
					break;
			}
#warning TODO can remove the following if dealt with in post processor for binary tree
		} else if (STRCMP(NODENAME(n), "wire") == 0) { /* binary format vertices it seems */
			pcb_coord_t x, y;
			x = eagle_get_attrc(st, n, "linetype_0_x1", 0);
			y = eagle_get_attrc(st, n, "linetype_0_y1", 0);
			pcb_poly_point_new(poly, x, y);
			x = eagle_get_attrc(st, n, "linetype_0_x2", 0);
			y = eagle_get_attrc(st, n, "linetype_0_y2", 0);
			pcb_poly_point_new(poly, x, y);
			switch (loc) {
				case IN_ELEM:
					break;
				case ON_BOARD:
					size_bump(st, x, y);
					break;
			}
		}
	}

	pcb_add_polygon_on_layer(&st->pcb->Data->Layer[ly->ly], poly);
	pcb_poly_init_clip(st->pcb->Data, &st->pcb->Data->Layer[ly->ly], poly);

	return 0;
}


static int eagle_read_signals(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"contactref",  eagle_read_contactref},
		{"wire",        eagle_read_wire},
		{"polygon",     eagle_read_poly},
		{"via",         eagle_read_via},
		{"text",        eagle_read_text},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "signal") == 0) {
			const char *name = eagle_get_attrs(st, n, "name", NULL);
			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Ignoring signal with no name\n");
				continue; 
			}
			eagle_foreach_dispatch(st, CHILDREN(n), disp, (char *)name, ON_BOARD);
		}
	}

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

	return 0;
}

static void eagle_read_elem_text(read_state_t *st, trnode_t *nd, pcb_element_t *elem, pcb_text_t *text, pcb_text_t *def_text, pcb_coord_t x, pcb_coord_t y, const char *attname, const char *str)
{
	int direction = 0;
	pcb_flag_t TextFlags = pcb_no_flags();

	/*x += def_text->X;
	y += def_text->Y + EAGLE_TEXT_SIZE_100; */

	for(nd = CHILDREN(nd); nd != NULL; nd = NEXT(nd)) {
		const char *this_attr = eagle_get_attrs(st, nd, "name", "");
		if ((STRCMP(NODENAME(nd), "attribute") == 0) && (strcmp(attname, this_attr) == 0)) {
			direction = eagle_rot2steps(eagle_get_attrs(st, nd, "rot", NULL));
			if (direction < 0)
				direction = 0;
/*			size = eagle_get_attrc(st, nd, "size", -1);*/
			x += eagle_get_attrc(st, nd, "x", x);
			y += eagle_get_attrc(st, nd, "y", y);
			break;
		}
	}
#warning subc TODO can have unique text scaling in subcircuits
/*
	int TextScale = (def_text->Scale == 0 ? 100 : def_text->Scale);
	pcb_coord_t size;
	if (size >= 0)
		TextScale = (int)(((double)size/ (double)EAGLE_TEXT_SIZE_100) * 100.0);
	pcb_trace("About to use text scale %d for element.\n", TextScale); 

	pcb_element_text_set(text, pcb_font(st->pcb, 0, 1), x, y, direction, str, TextScale, TextFlags);
*/
	pcb_element_text_set(text, pcb_font(st->pcb, 0, 1), x, y, direction, str, st->refdes_scale, TextFlags);
	text->Element = elem;
}

static int eagle_read_elements(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n, *nlib;
	if (st->elem_by_name)
		nlib = NULL;
	else
		nlib = eagle_trpath(st, st->parser.root, "drawing", "board", "libraries", NULL);

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "element") == 0) {
			pcb_coord_t x, y;
			const char *name, *val, *lib, *pkg, *rot, *mirrored;
			pcb_element_t *elem, *new_elem;
			int steps, back = 0;

			name = eagle_get_attrs(st, n, "name", NULL);
			val = eagle_get_attrs(st, n, "value", NULL);

			if (name == NULL) {
				pcb_message(PCB_MSG_WARNING, "Element name not found in tree\n");
				name = pcb_strdup("refdes_not_found");
				val = pcb_strdup("parse_error");
			}
			/* need to get these as string because error messages will use them */
			lib = eagle_get_attrs(st, n, "library", NULL);
			pkg = eagle_get_attrs(st, n, "package", NULL);

			if (st->elem_by_name) { /* xml: library and package are named */
				if ((lib == NULL) || (pkg == NULL)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring element with incomplete library reference\n");
					continue;
				}
				elem = eagle_libelem_by_name(st, lib, pkg);
			}
			else {
				long libi = eagle_get_attrl(st, n, "library", -1);
				long pkgi = eagle_get_attrl(st, n, "package", -1);
				if ((libi < 0) || (pkgi < 0)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring element with broken library reference: %s/%s\n", lib, pkg);
					continue;
				}
				elem = eagle_libelem_by_idx(st, nlib, libi, pkgi);
			}

			/* sanity checks: the element exists and is non-empty */
			if (elem == NULL) {
				pcb_message(PCB_MSG_WARNING, "Library element not found: %s/%s\n", lib, pkg);
				continue;
			}
			if (pcb_element_is_empty(elem)) {
				pcb_message(PCB_MSG_WARNING, "Not placing empty element: %s/%s\n", lib, pkg);
				continue;
			}

			x = eagle_get_attrc(st, n, "x", -1);
			y = eagle_get_attrc(st, n, "y", -1);
			rot = eagle_get_attrs(st, n, "rot", NULL);
			mirrored = eagle_get_attrs(st, n, "mirrored", NULL);
			if ((rot != NULL) && (*rot == 'M')) {
				rot++;
				back = 1;
			} else if ((mirrored != NULL) && (*mirrored == '1')) {
				back = 1;
			}

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

#warning subc TODO this code ensures mainline element refdes, value, descr texts all get refdes x,y,scale
			st->refdes_x = st->refdes_y = 0;
			st->value_x = st->value_y = 0;
			st->refdes_scale = st->value_scale = 100; /* default values */
			eagle_read_elem_text(st, n, new_elem, &PCB_ELEM_TEXT_DESCRIPTION(new_elem), &PCB_ELEM_TEXT_DESCRIPTION(elem), x, y, "PROD_ID", pkg);
			eagle_read_elem_text(st, n, new_elem, &PCB_ELEM_TEXT_REFDES(new_elem), &PCB_ELEM_TEXT_REFDES(elem), x, y, "NAME", name);
			eagle_read_elem_text(st, n, new_elem, &PCB_ELEM_TEXT_VALUE(new_elem), &PCB_ELEM_TEXT_VALUE(elem), x, y, "VALUE", val);

			pcb_trace("used refdes x: %ml\n", st->refdes_x);
			pcb_trace("used refdes y: %ml\n", st->refdes_y);
			pcb_trace("used refdes scale: %d\n", st->refdes_scale);
			if (rot != NULL) {
				steps = eagle_rot2steps(rot);
				if (back) {
					steps = (steps + 2)%4;
				}
				if (steps > 0)
					pcb_element_rotate90(st->pcb->Data, new_elem, x, y, steps);
				else
					pcb_message(PCB_MSG_WARNING, "Ignored non-90 deg rotation '%s': %s/%s/%s\n", rot, name, pkg, lib);
			}

			pcb_element_bbox(st->pcb->Data, new_elem, pcb_font(st->pcb, 0, 1));
			size_bump(st, new_elem->BoundingBox.X2, new_elem->BoundingBox.Y2);

			if (back)
				pcb_element_change_side(new_elem, 2 * y - st->pcb->MaxHeight);

			pcb_trace("placing %s: %s/%s -> %p\n", name, lib, pkg, (void *)elem);
			if (st->refdes_x != st->value_x || st->refdes_y != st->value_y || st->refdes_scale != st->value_scale) {
				pcb_message(PCB_MSG_WARNING, "element \"value\" text x ,y, scaling != those of refdes text; set to refdes x, y, scaling.\n");
			}
		}
	}
	return 0;
}

static int eagle_read_plain(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"contactref",  eagle_read_contactref},
		{"wire",        eagle_read_wire},
		{"polygon",     eagle_read_poly},
		{"rectangle",   eagle_read_rect},
		{"via",         eagle_read_via},
		{"circle",      eagle_read_circle},
		{"text",        eagle_read_text},
		{"hole",        eagle_read_hole},
		{"dimension",   eagle_read_nop},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

#warning TODO: test (should process these probably no-net-no-signal objects)

	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, NULL, ON_BOARD);
}

static int eagle_read_board(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <board> */
		{"plain",       eagle_read_plain},
		{"libraries",   eagle_read_libs},
		{"attributes",  eagle_read_nop},
		{"variantdefs", eagle_read_nop},
		{"classes",     eagle_read_nop},
		{"description", eagle_read_nop},
		{"designrules", eagle_read_nop},
		{"autorouter",  eagle_read_nop},
		{"elements",    eagle_read_elements},
		{"signals",     eagle_read_signals},
		{"errors",      eagle_read_nop},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};
	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, NULL, 0);
}


static int eagle_read_drawing(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	
	static const dispatch_t disp[] = { /* possible children of <drawing> */
		{"settings",  eagle_read_nop},
		{"layers",    eagle_read_layers},
		{"grid",      eagle_read_nop},
		{"board",     eagle_read_board},
		{"unknown11", eagle_read_nop}, /* TODO: temporary; from the binary tree */
		{"@text",     eagle_read_nop},
		{NULL, NULL}
	};

	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, NULL, 0);
}

static int eagle_read_design_rules(read_state_t *st, trnode_t *subtree)
{
	trnode_t *n;
	const char *name;

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "param") != 0)
			continue;
		name = eagle_get_attrs(st, n, "name", NULL);
		if (strcmp(name, "mdWireWire") == 0) st->md_wire_wire = eagle_get_attrcu(st, n, "value", 0);
		else if (strcmp(name, "msWidth") == 0) st->ms_width = eagle_get_attrcu(st, n, "value", 0);
		else if (strcmp(name, "rvPadTop") == 0) st->rv_pad_top = eagle_get_attrd(st, n, "value", 0);
		else if (strcmp(name, "rvPadInner") == 0) st->rv_pad_inner = eagle_get_attrd(st, n, "value", 0);
		else if (strcmp(name, "rvPadBottom") == 0) st->rv_pad_bottom = eagle_get_attrd(st, n, "value", 0);
	}
	if ((st->rv_pad_top != st->rv_pad_inner) || (st->rv_pad_top != st->rv_pad_inner))
		pcb_message(PCB_MSG_WARNING, "top/inner/bottom default pad sizes differ - using top size only\n");
	return 0;
}

static int eagle_read_ver(const char *ver)
{
	int v1, v2, v3;
	char *end;

	if (ver == NULL) {
		pcb_message(PCB_MSG_ERROR, "no version attribute in <eagle>\n");
		return -1;
	}

	v1 = strtol(ver, &end, 10);
	if (*end != '.') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [1] in <eagle>\n");
		return -1;
	}
	v2 = strtol(end+1, &end, 10);
	if (*end != '.' && *end != '\0') {
		pcb_message(PCB_MSG_ERROR, "malformed version string [2] in <eagle>\n");
		return -1;
	}
	v3 = strtol(end+1, &end, 10);
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
	st->parser.calls->unload(&st->parser);
}

int io_eagle_read_pcb_xml(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	trnode_t *dr;
	int res, old_leni;
	read_state_t st;

	static const dispatch_t disp_1[] = { /* possible children of root */
		{"layers",         eagle_read_layers}, /* this is hanging off root */
		{"drawing",        eagle_read_nop},
		{"compatibility",  eagle_read_nop},
		{"@text",          eagle_read_nop},
		{NULL, NULL}
	};

	static const dispatch_t disp_2[] = { /* possible children of root */
		{"layers",         eagle_read_nop},
		{"drawing",        eagle_read_drawing},
		{"compatibility",  eagle_read_nop},
		{"@text",          eagle_read_nop},
		{NULL, NULL}
	};

	st.parser.calls = &trparse_xml_calls;

	if (st.parser.calls->load(&st.parser, Filename) != 0)
		return -1;

	if (eagle_read_ver(GET_PROP_(&st, st.parser.root, "version")) < 0)
		goto err;

	st.pcb = pcb;
	st.elem_by_name = 1;
	st.default_unit = "mm";

	st_init(&st);

	dr = eagle_trpath(&st, st.parser.root, "drawing", "board", "designrules", NULL);
	if (dr != NULL)
		eagle_read_design_rules(&st, dr);
	else
		pcb_message(PCB_MSG_WARNING, "can't find design rules\n");

	old_leni = pcb_create_being_lenient;
	pcb_create_being_lenient = 1;
	res = eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_1, NULL, 0);
	res |= eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_2, NULL, 0);
	if (res == 0)
		pcb_flip_data(pcb->Data, 0, 1, 0, pcb->MaxHeight, 0);
	pcb_create_being_lenient = old_leni;
	pcb_board_normalize(pcb);

	st_uninit(&st);

	pcb_trace("Houston, the Eagle-xml has landed. %d\n", res);

	return 0;

err:;
	st_uninit(&st);
	pcb_trace("Eagle XML parsing error. Bailing out now.\n");
	return -1;
}

int io_eagle_read_pcb_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	int res, old_leni;
	read_state_t st;

	static const dispatch_t disp_1[] = { /* possible children of root */
		{"drawing",        eagle_read_nop},
		{"layers",         eagle_read_layers}, /* trying this */
		{NULL, NULL}
	};


	static const dispatch_t disp_2[] = { /* possible children of root */
		{"drawing",        eagle_read_drawing},
		{"layers",         eagle_read_nop},
		{NULL, NULL}
	};

	st.parser.calls = &trparse_bin_calls;

	if (st.parser.calls->load(&st.parser, Filename) != 0)
		return -1;

	st.pcb = pcb;
	st.elem_by_name = 0;
	st.default_unit = "du"; /* du = decimicron = 0.1 micron unit for eagle bin format */
#warning TODO have defaults for v3 binary + design rule override for v4, v5 binary:
	st.ms_width = PCB_MIL_TO_COORD(15);
	st.rv_pad_top = PCB_MIL_TO_COORD(10); /*wild guess*/
	st.md_wire_wire = PCB_MIL_TO_COORD(10);
	st_init(&st);

#warning TODO: find and read the DRC block
#if 0
	dr = eagle_trpath(&st, st.parser.root, "drawing", "board", "designrules", NULL);
	if (dr != NULL)
		eagle_read_design_rules(&st, dr);
	else
		pcb_message(PCB_MSG_WARNING, "can't find design rules\n");
#endif

	old_leni = pcb_create_being_lenient;
	pcb_create_being_lenient = 1;
	pcb_trace("About to dispatch to layer handler.\n");
	res = eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_1, NULL, 0);
	pcb_trace("About to dispatch to board handler.\n");
	res |= eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_2, NULL, 0);
	if (res == 0)
		pcb_flip_data(pcb->Data, 0, 1, 0, pcb->MaxHeight, 0);
	pcb_create_being_lenient = old_leni;
	pcb_board_normalize(pcb);
	st_uninit(&st);

	pcb_trace("Houston, the Eagle-bin has landed. %d\n", res);

	return 0;
}

