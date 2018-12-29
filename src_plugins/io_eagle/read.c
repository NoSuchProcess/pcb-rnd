/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Erich S. Heinzle
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 *
 */

#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "board.h"
#include "data.h"
#include "read.h"
#include "conf.h"
#include "conf_core.h"
#include "error.h"
#include "polygon.h"
#include "rtree.h"
#include "actions.h"
#include "undo.h"
#include "compat_misc.h"
#include "trparse.h"
#include "trparse_xml.h"
#include "trparse_bin.h"
#include "obj_subc.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"

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

	pcb_layer_id_t lid;
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
	IN_SUBC = 1,
	ON_BOARD
} eagle_loc_t;

typedef	int eagle_layerid_t;

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



int io_eagle_test_parse_xml(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
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

int io_eagle_test_parse_bin(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
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

	pcb_layergrp_inhibit_inc();

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "layer") == 0) {
			eagle_layer_t *ly = calloc(sizeof(eagle_layer_t), 1);
			int reuse = 0;
			long tmp_id;
			eagle_layerid_t id;
			unsigned long typ;
			pcb_layergrp_id_t gid;
			pcb_layergrp_t *grp;

			ly->name    = eagle_get_attrs(st, n, "name", NULL);
			ly->color   = eagle_get_attrl(st, n, "color", -1);
			ly->fill    = eagle_get_attrl(st, n, "fill", -1);
			ly->visible = eagle_get_attrl(st, n, "visible", -1);
			ly->active  = eagle_get_attrl(st, n, "active", -1);
			ly->lid     = -1;
			tmp_id = eagle_get_attrl(st, n, "number", -1);
			if (tmp_id < 1 || tmp_id > 254) {
				pcb_message(PCB_MSG_ERROR, "invalid layer definition layer number found: '%d', skipping\n", tmp_id);
				return -1;
			}
			id = tmp_id;
			htip_set(&st->layers, id, ly); /* all valid layers get a hash */
			typ = 0;
			switch(id) {
				case 1: typ = PCB_LYT_COPPER | PCB_LYT_TOP; break;
				case 16: typ = PCB_LYT_COPPER | PCB_LYT_BOTTOM; break;
				case 121:
				case 21: /* tplace element silk */
				/*case 51: tDocu, second layer in top silk group */ 
				case 25: /* names */
				case 27: /* values */
				case 39: /* keepout */
					reuse = 1;
					typ = PCB_LYT_SILK | PCB_LYT_TOP;
					break;
				case 122:
				case 22: /* bplace element silk */
				/* case 52: bDocu, second layer in bottom silk group */
				case 26: /* names */
				case 28: /* values */
				case 40: /* keepout */
					reuse = 1;
					typ = PCB_LYT_SILK | PCB_LYT_BOTTOM;
					break;
				case 20: /*199:   20 is dimension, 199 is contour */
					grp = pcb_get_grp_new_intern(st->pcb, -1);
					ly->lid = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, ly->name);
					pcb_layergrp_fix_turn_to_outline(grp);
					break;

				default:
					if ((id > 1) && (id < 16)) {
						/* new internal layer */
						grp = pcb_get_grp_new_intern(st->pcb, -1);
						ly->lid = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, ly->name);
					}
			}
			if (typ != 0) {
				if (reuse)
					pcb_layer_list(st->pcb, typ, &ly->lid, 1);
				if ((ly->lid < 0) && (pcb_layergrp_list(st->pcb, typ, &gid, 1) > 0))
					ly->lid = pcb_layer_create(st->pcb, gid, ly->name);
			}
		}
	}
	pcb_layer_group_setup_silks(st->pcb);
	pcb_layer_auto_fixup(st->pcb);
	pcb_layergrp_inhibit_dec();
	return 0;
}

static pcb_layer_t *eagle_layer_get(read_state_t *st, eagle_layerid_t id, eagle_loc_t loc, void *obj)
{
	/* tDocu & bDocu are used for info used when designing, but not necessarily for
	   exporting to Gerber i.e. package outlines that cross pads, or instructions.
	   These layers within the silk groups will be needed when subc replaces elements
	   since most Eagle packages use tDocu, bDocu for some of their artwork */

	eagle_layer_t *ly = htip_get(&st->layers, id);
	pcb_layer_id_t lid;
	pcb_subc_t *subc = obj;
	pcb_layer_type_t lyt;
	pcb_layer_combining_t comb;

	/* if more than 51 or 52 are considered useful, we could relax the test here: */
	if ((ly == NULL) || (ly->lid < 0)) {
TODO(": move this out to a separate function")
		if (id == 51 || id == 52) {
			/* create docu on the first reference */
		pcb_layer_type_t typ;
		pcb_layergrp_id_t gid;
		switch (id) {
			case 51: /* = tDocu */
				typ        = PCB_LYT_SILK | PCB_LYT_TOP;
				ly->name   = "tDocu";
				ly->color  = 14;
				break;
			default: /* i.e. 52 = bDocu: */
				typ        = PCB_LYT_SILK | PCB_LYT_BOTTOM;
				ly->name   = "bDocu";
				ly->color  = 7;
				break;
		}

		ly->fill    = 1;
		ly->visible = 0;
		ly->active  = 1;
		pcb_layergrp_list(st->pcb, typ, &gid, 1);
		ly->lid = pcb_layer_create(st->pcb, gid, ly->name);	
		}
		else
			return NULL; /* not found and not supported */
	}

	switch(loc) {
		case ON_BOARD:
			return &st->pcb->Data->Layer[ly->lid];
		case IN_SUBC:
			/* check if the layer already exists (by name) */
			lid = pcb_layer_by_name(subc->data, ly->name);
			if (lid >= 0)
				return &subc->data->Layer[lid];

			if (ly->lid < 0) {
				pcb_message(PCB_MSG_ERROR, "\tfp_* layer '%s' not found for module object, using unbound subc layer instead.\n", ly->name);
				lyt = PCB_LYT_VIRTUAL;
				comb = 0;
				return pcb_subc_get_layer(subc, lyt, comb, 1, ly->name, pcb_true);
			}
			lyt = pcb_layer_flags(st->pcb, ly->lid);
			comb = 0;
			return pcb_subc_get_layer(subc, lyt, comb, 1, ly->name, pcb_true);
	}
	return NULL;
}

static pcb_subc_t *eagle_libelem_by_name(read_state_t *st, const char *lib, const char *elem)
{
	eagle_library_t *l;
	l = htsp_get(&st->libs, lib);
	if (l == NULL)
		return NULL;
	return htsp_get(&l->elems, elem);
}

static pcb_subc_t *eagle_libelem_by_idx(read_state_t *st, trnode_t *libs, long libi, long pkgi)
{
	trnode_t *n;
	pcb_subc_t *res;

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

/* Convert eagle Rxxx to degrees. Binary n*1024 string converted to Rxxx in eagle_bin.c */
static int eagle_rot2degrees(const char *rot)
{
	long deg;
	char *end;

	if (rot == NULL) {
		return -1;
	} else {
		deg = strtol(rot+1, &end, 10);
		if (*end != '\0')
			return -1;
	}
	while (deg >= 360) {
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
	pcb_message(PCB_MSG_WARNING, "Unexpected non n*90 degree rotation value '%s' ignored\n", rot);
	return -1;
}

/****************** drawing primitives ******************/


static int eagle_read_text(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_coord_t X, Y, height;
	const char *rot, *text_val;
	unsigned int text_direction = 0, text_scaling = 100;
	pcb_flag_t text_flags = pcb_flag_make(0);
	pcb_layer_t *ly;
	ly = eagle_layer_get(st, ln, type, obj);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to allocate text layer 'ly' to 'ln:%d' in eagle_read_text()\n", ln);
		return 0;
	}
TODO("TODO text - need better filtering/exclusion of unsupported text layers +/- correct flags")
TODO(": remove this hack - if there is a bug, fix it, do not work it around like this")
	if (ln == 51) {
		ln = 21; /* we seem to trigger a segfault if we create text with ln = 51 */
		pcb_message(PCB_MSG_WARNING, "Moved text on tDocu layer: 51 to top silk\n", ln);
	} else if (!(ln == 21 || ln == 16 )) {
		pcb_message(PCB_MSG_WARNING, "Ignoring text on non top copper/silk Eagle layer: %ld\n", ln);
		return 0;
	}
	if (!(text_val = eagle_get_attrs(st, subtree, "textfield", NULL)) && CHILDREN(subtree) == NULL) {
		pcb_message(PCB_MSG_WARNING, "Ignoring empty text field\n");
		return 0;
	}
	if (text_val == NULL && !IS_TEXT(CHILDREN(subtree))) {
		pcb_message(PCB_MSG_WARNING, "Ignoring text field (invalid child node)\n");
		return 0;
	}

TODO(": need to convert")
	if (text_val == NULL) {
		text_val = (const char *)GET_TEXT(CHILDREN(subtree));
	}
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

TODO("textrot: check if we can just use the angle")
	pcb_text_new(ly, pcb_font(st->pcb, 0, 1), X, Y, 90.0*text_direction, text_scaling, 0, text_val, text_flags);
	return 0;
}

static long eagle_degrees_to_pcb_degrees(long const eagle_degrees) {
	long e_degrees = eagle_degrees;
	while (e_degrees < 0) {
		e_degrees += 360;
	}
	while (e_degrees > 360) {
		e_degrees -= 360;
	}
	return (360 - e_degrees);
}

static int eagle_read_circle(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_arc_t *circ;
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_layer_t *ly;

	ly = eagle_layer_get(st, ln, loc, obj);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to allocate circle layer 'ly' to 'ln:%d' in eagle_read_circle()\n", ln);
		return 0;
	}

	circ = pcb_arc_alloc(ly);
	circ->X = eagle_get_attrc(st, subtree, "x", -1);
	circ->Y = eagle_get_attrc(st, subtree, "y", -1);
	circ->Width = eagle_get_attrc(st, subtree, "radius", -1);
	circ->Height = circ->Width; /* no ellipse support */
	circ->StartAngle = eagle_degrees_to_pcb_degrees(eagle_get_attrl(st, subtree, "StartAngle", 0));
	circ->Delta = -eagle_get_attrl(st, subtree, "Delta", -360);
	circ->Thickness = eagle_get_attrc(st, subtree, "width", -1);
	circ->Clearance = st->md_wire_wire*2;
	circ->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);


	switch(loc) {
		case IN_SUBC: break;
		case ON_BOARD:
			size_bump(st, circ->X + circ->Width + circ->Thickness, circ->Y + circ->Width + circ->Thickness);
			pcb_add_arc_on_layer(ly, circ);
			break;
	}

	return 0;
}

static int eagle_read_rect(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin1, *lin2, *lin3, *lin4;
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_layer_t *ly;
	pcb_coord_t x1, y1, x2, y2;

	ly = eagle_layer_get(st, ln, loc, obj);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to allocate rect layer 'ly' to 'ln:%d' in eagle_read_rect()\n", ln);
		return 0;
	}

	x1 = eagle_get_attrc(st, subtree, "x1", -1);
	y1 = eagle_get_attrc(st, subtree, "y1", -1);
	x2 = eagle_get_attrc(st, subtree, "x2", -1);
	y2 = eagle_get_attrc(st, subtree, "y2", -1);

	lin1 = pcb_line_alloc(ly);
	lin2 = pcb_line_alloc(ly);
	lin3 = pcb_line_alloc(ly);
	lin4 = pcb_line_alloc(ly);

	lin1->Point1.X = x1;
	lin1->Point1.Y = y1;
	lin1->Point2.X = x2;
	lin1->Point2.Y = y1;

	lin2->Point1.X = x2;
	lin2->Point1.Y = y1;
	lin2->Point2.X = x2;
	lin2->Point2.Y = y2;

	lin3->Point1.X = x2;
	lin3->Point1.Y = y2;
	lin3->Point2.X = x1;
	lin3->Point2.Y = y2;

	lin4->Point1.X = x1;
	lin4->Point1.Y = y2;
	lin4->Point2.X = x1;
	lin4->Point2.Y = y1;

	lin1->Thickness = lin2->Thickness = lin3->Thickness = lin4->Thickness = st->ms_width;

	switch(loc) {
		case IN_SUBC:
			break;

		case ON_BOARD:
			size_bump(st, x1 - lin1->Thickness, y1 - lin1->Thickness);
			size_bump(st, x2 + lin1->Thickness, y2 + lin1->Thickness);
			pcb_add_line_on_layer(ly, lin1);
			pcb_add_line_on_layer(ly, lin2);
			pcb_add_line_on_layer(ly, lin3);
			pcb_add_line_on_layer(ly, lin4);

			break;
	}

	return 0;
}

static int eagle_read_wire(read_state_t * st, trnode_t * subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_line_t *lin;
	pcb_layer_t *ly;
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	long linetype = eagle_get_attrl(st, subtree, "linetype", -1);/* only present if bin file */
	double curve = eagle_get_attrd(st, subtree, "curve", 0); /*present if a wire "arc" */

TODO(": need to process curve value if present to draw an arc, not a line")
	if (curve) {
		pcb_message(PCB_MSG_ERROR, "Curved wire not yet handled in eagle_read_wire()\n");
	}
	if (linetype > 0) { /* only occurs if loading eagle binary wire type != 0 */
		return eagle_read_circle(st, subtree, obj, type);
	}

	ly = eagle_layer_get(st, ln, loc, obj);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to allocate wire layer 'ly' to ln:%d in eagle_read_wire()\n");
		return 0;
	}

	lin = pcb_line_alloc(ly);
	lin->Point1.X = eagle_get_attrc(st, subtree, "x1", -1);
	lin->Point1.Y = eagle_get_attrc(st, subtree, "y1", -1);
	lin->Point2.X = eagle_get_attrc(st, subtree, "x2", -1);
	lin->Point2.Y = eagle_get_attrc(st, subtree, "y2", -1);
	lin->Thickness = eagle_get_attrc(st, subtree, "width", -1); 
	lin->Clearance = st->md_wire_wire*2;
	lin->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);
	lin->ID = pcb_create_ID_get();

	switch (loc) {
		case IN_SUBC:
			break;
		case ON_BOARD:
			size_bump(st, lin->Point1.X + lin->Thickness, lin->Point1.Y + lin->Thickness);
			size_bump(st, lin->Point2.X + lin->Thickness, lin->Point2.Y + lin->Thickness);
			pcb_add_line_on_layer(ly, lin);
			break;
	}

	return 0;
}

typedef enum {
	EAGLE_PSH_SQUARE,
	EAGLE_PSH_ROUND,
	EAGLE_PSH_OCTAGON,
	EAGLE_PSH_LONG,
	EAGLE_PSH_OFFSET, /* need example of this */
	EAGLE_PSH_SMD     /* special round rect */
} eagle_pstk_shape_t;

/* Create a padstack at x;y; roundness and onbottom, applies only to
   EAGLE_PSH_SMD. dx and dy are the size; for some shapes they have to
   be equal. Returns NULL on error. */
static pcb_pstk_t *eagle_create_pstk(read_state_t *st, pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, eagle_pstk_shape_t shape, pcb_coord_t dx, pcb_coord_t dy, pcb_coord_t clr, pcb_coord_t drill_dia, int roundness, int rot, int onbottom, pcb_bool plated)
{
	pcb_pstk_shape_t shapes[8];
TODO("TODO need to establish how mask clearance is defined and done in eagle")
	pcb_coord_t mask_gap = clr;
TODO("TODO need to establish how paste clearance, if any, is defined and done in eagle")
	pcb_coord_t paste_gap = 0;
	switch (shape) {
		case EAGLE_PSH_SQUARE:
			shapes[0].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK;
			shapes[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_rect(&shapes[0], dx + mask_gap, dy + mask_gap);
			shapes[1].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE;
			shapes[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_rect(&shapes[1], dx + paste_gap, dy + paste_gap);
			shapes[2].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER;
			shapes[2].comb = 0;
			pcb_shape_rect(&shapes[2], dx, dy);
			shapes[3].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER;
			shapes[3].comb = 0;
			pcb_shape_rect(&shapes[3], dx, dy);
			shapes[4].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER;
			shapes[4].comb = 0;
			pcb_shape_rect(&shapes[4], dx, dy);
			shapes[5].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE;
			shapes[5].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_rect(&shapes[5], dx + paste_gap, dy + paste_gap);
			shapes[6].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK;
			shapes[6].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_rect(&shapes[6], dx + mask_gap, dy + mask_gap);
			shapes[7].layer_mask = 0;
			break;
		case EAGLE_PSH_OCTAGON:
TODO("TODO need octagon shape generation function/API accessible from read.c padstack creation function")
		case EAGLE_PSH_ROUND:
			assert(dx == dy);
		case EAGLE_PSH_LONG:
			shapes[0].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK;
			shapes[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_oval(&shapes[0], dx + mask_gap, dy + mask_gap);
			shapes[1].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE;
			shapes[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_oval(&shapes[1], dx + paste_gap, dy + paste_gap);
			shapes[2].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER;
			shapes[2].comb = 0;
			pcb_shape_oval(&shapes[2], dx, dy);				
			shapes[3].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER;
			shapes[3].comb = 0;
			pcb_shape_oval(&shapes[3], dx, dy);
			shapes[4].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER;
			shapes[4].comb = 0;
			pcb_shape_oval(&shapes[4], dx, dy);
			shapes[5].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE;
			shapes[5].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_oval(&shapes[5], dx + paste_gap, dy + paste_gap);
			shapes[6].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK;
			shapes[6].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
			pcb_shape_oval(&shapes[6], dx + mask_gap, dy + mask_gap);
			shapes[7].layer_mask = 0;
			break;
		case EAGLE_PSH_SMD:
			{
TODO("TODO need to implement SMD roundness ")
				pcb_layer_type_t side = onbottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
				shapes[0].layer_mask = side | PCB_LYT_MASK;
				shapes[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
				pcb_shape_rect(&shapes[0], dx + mask_gap, dy + mask_gap);
				shapes[1].layer_mask = side | PCB_LYT_PASTE;
				shapes[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
				pcb_shape_rect(&shapes[1], dx + paste_gap, dy + paste_gap);
				shapes[2].layer_mask = side | PCB_LYT_COPPER;
				shapes[2].comb = 0;
				pcb_shape_rect(&shapes[2], dx, dy);
				shapes[3].layer_mask = 0;
			}
			break;
		case EAGLE_PSH_OFFSET:
TODO("TODO need OFFSET shape generation function, once OFFSET object understood")
			return NULL;
	}
	return pcb_pstk_new_from_shape(data, x, y, drill_dia, plated, clr, shapes);
}

static int eagle_read_smd(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	pcb_coord_t x, y, dx, dy;
	pcb_pstk_t *ps;
	pcb_subc_t *subc = obj;
	const char *name;
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	long roundness = 0;
	pcb_pstk_shape_t sh[4];
	pcb_coord_t clr;
	int rot;

	assert(type == IN_SUBC);

	name = eagle_get_attrs(st, subtree, "name", NULL);
	x = eagle_get_attrc(st, subtree, "x", 0);
	y = eagle_get_attrc(st, subtree, "y", 0);
	dx = eagle_get_attrc(st, subtree, "dx", 0);
	dy = eagle_get_attrc(st, subtree, "dy", 0);
	rot = eagle_rot2degrees(eagle_get_attrs(st, subtree, "rot", 0));
	roundness = eagle_get_attrl(st, subtree, "roundness", 0);

TODO("TODO need to load thermals flags to set clearance; may in fact be more contactref related.")

TODO(": this should be coming from the eagle file")
	clr = conf_core.design.clearance;

TODO("TODO padstacks - consider roundrect, oval etc shapes when padstacks available")
#if 0
	if (roundness >= 65) /* round smd pads found in fiducials, some discretes, it seems */
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, pad);
#endif

	memset(sh, 0, sizeof(sh));
	sh[0].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK; sh[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[0], dx+clr, dy+clr);
	sh[1].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE; sh[1].comb = PCB_LYC_AUTO; pcb_shape_rect(&sh[1], dx, dy);
	sh[2].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; pcb_shape_rect(&sh[2], dx, dy);
	sh[3].layer_mask = 0;

	if (rot != 0) {
		double sina = sin(rot / PCB_RAD_TO_DEG), cosa = cos(rot / PCB_RAD_TO_DEG);
		pcb_pstk_shape_rot(&sh[0], sina, cosa, rot);
		pcb_pstk_shape_rot(&sh[1], sina, cosa, rot);
		pcb_pstk_shape_rot(&sh[2], sina, cosa, rot);
	}

TODO(": call eagle_create_pstk() instead")
	ps = pcb_pstk_new_from_shape(subc->data, x, y, 0, 0, clr, sh);

	if (ps == NULL)
		pcb_message(PCB_MSG_ERROR, "Failed to load smd pad\n");

	if (name != NULL)
		pcb_attribute_put(&ps->Attributes, "term", name);

	return 0;
}

static int eagle_read_pad_or_hole(read_state_t *st, trnode_t *subtree, void *obj, int type, int hole)
{
	eagle_loc_t loc = type;
	pcb_coord_t x, y, drill, dia, clr, mask;
	pcb_pstk_t *ps;
	const char *name, *shape;
	pcb_pstk_compshape_t cshp = PCB_PSTK_COMPAT_INVALID;
	pcb_data_t *data;

	switch(loc) {
		case IN_SUBC:
			data = ((pcb_subc_t *)obj)->data;
			break;
		case ON_BOARD:
			data = st->pcb->Data;
			break;
	}

	name = eagle_get_attrs(st, subtree, "name", NULL);
	x = eagle_get_attrc(st, subtree, "x", 0);
	y = eagle_get_attrc(st, subtree, "y", 0);
	drill = eagle_get_attrc(st, subtree, "drill", 0);
	dia = eagle_get_attrc(st, subtree, "diameter", drill * (1.0+st->rv_pad_top*2.0));
	shape = eagle_get_attrs(st, subtree, "shape", 0);

	clr = conf_core.design.clearance; /* eagle doesn't seem to support per via clearance */
	mask = (loc == IN_SUBC) ? conf_core.design.clearance : 0; /* board vias don't have mask */

	if ((dia - drill) / 2.0 < st->ms_width)
		dia = drill + 2*st->ms_width;

TODO("padstack: process the extent attribute for bbvia")
TODO("padstack: revise this for numeric values ?")
	/* shape = {square, round, octagon, long, offset} binary */

TODO(": call eagle_create_pstk() instead")
	if (shape != NULL) {
		if ((strcmp(shape, "octagon") == 0) || (strcmp(shape, "2") == 0))
			cshp = PCB_PSTK_COMPAT_OCTAGON;
		else if ((strcmp(shape, "square") == 0) || (strcmp(shape, "0") == 0))
			cshp = PCB_PSTK_COMPAT_SQUARE;
		else if ((strcmp(shape, "round") == 0) || (strcmp(shape, "1") == 0))
			cshp = PCB_PSTK_COMPAT_ROUND;
	}

	if (cshp == PCB_PSTK_COMPAT_INVALID) {
		cshp = PCB_PSTK_COMPAT_OCTAGON;
		pcb_message(PCB_MSG_ERROR, "Invalid padstack shape: '%s' - using octagon instead\n", shape);
	}

	ps = pcb_pstk_new_compat_via(data, -1, x, y, drill, dia, clr, mask,  cshp, !hole);

	if (name != NULL)
		pcb_attribute_put(&ps->Attributes, "term", name);

	switch(loc) {
		case IN_SUBC: break;
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
	pcb_coord_t x, y;
	int dir = 0, scale;
	eagle_layerid_t layer;
	const char *pattern;

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n))
		if (IS_TEXT(n))
			break;

	if ((n == NULL) || ((cont = GET_TEXT(n)) == NULL))
		return 0;

	/* create dyntext for name and value only (the core could handle any attribute, but we can't pick them up on the element ref) */
	if (STRCMP(cont, ">NAME") == 0)
		pattern = "%a.parent.refdes%";
	else if (STRCMP(cont, ">VALUE") == 0)
		pattern = "%a.parent.value%";
	else
		return 0;

	layer = eagle_get_attrl(st, subtree, "layer", 0);
	size = eagle_get_attrc(st, subtree, "size", EAGLE_TEXT_SIZE_100);
	x = eagle_get_attrc(st, subtree, "x", 0);
	y = eagle_get_attrc(st, subtree, "y", 0);
	y += size; /* different text object origin in eagle */
	scale = (int)(((double)size/(double)EAGLE_TEXT_SIZE_100) * 100.0);

	pcb_subc_add_dyntex((pcb_subc_t *)obj, x, y, dir, scale, (layer == 27), pattern);

	return 0;
}

static void eagle_read_poly_corner(read_state_t *st, trnode_t *n, pcb_poly_t *poly, const char *xname, const char *yname, eagle_loc_t loc)
{
	pcb_coord_t x, y;
	x = eagle_get_attrc(st, n, xname, 0);
	y = eagle_get_attrc(st, n, yname, 0);
	pcb_poly_point_new(poly, x, y);
	switch (loc) {
		case IN_SUBC:
			break;
		case ON_BOARD:
			size_bump(st, x, y);
			break;
	}
}

static int eagle_read_poly(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	eagle_loc_t loc = type;
	pcb_layer_t *ly;
	eagle_layerid_t ln = eagle_get_attrl(st, subtree, "layer", -1);
	pcb_poly_t *poly;
	trnode_t *n;

	ly = eagle_layer_get(st, ln, loc, obj);
	if (ly == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to allocate polygon layer 'ly' to 'ln:%d' in eagle_read_poly()\n", ln);
		return 0;
	}

	poly = pcb_poly_new(ly, 0, pcb_flag_make(PCB_FLAG_CLEARPOLY));
TODO("TODO need to check XML never defines a polygon outline with arcs or curves")
	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "vertex") == 0) {
			pcb_coord_t x, y;
			x = eagle_get_attrc(st, n, "x", 0);
			y = eagle_get_attrc(st, n, "y", 0);
			pcb_poly_point_new(poly, x, y);
			switch (loc) {
				case IN_SUBC:
					break;
				case ON_BOARD:
					size_bump(st, x, y);
					break;
			}
TODO("TODO need to check if binary format is sometimes using arcs or curves for polygn outlines")
TODO("TODO can remove the following if dealt with in post processor for binary tree")
		} else if (STRCMP(NODENAME(n), "wire") == 0) { /* binary format vertices it seems */
			eagle_read_poly_corner(st, n, poly, "linetype_0_x1", "linetype_0_y1", loc);
			eagle_read_poly_corner(st, n, poly, "linetype_0_x2", "linetype_0_y2", loc);
		}
	}

	pcb_add_poly_on_layer(ly, poly);
	pcb_poly_init_clip(st->pcb->Data, ly, poly);

	return 0;
}

static int eagle_read_pkg(read_state_t *st, trnode_t *subtree, pcb_subc_t *subc)
{
	static const dispatch_t disp[] = { /* possible children of package */
		{"description", eagle_read_nop},
		{"wire",        eagle_read_wire},
		{"hole",        eagle_read_hole},
		{"circle",      eagle_read_circle},
		{"arc",         eagle_read_circle},
		{"smd",         eagle_read_smd},
		{"pad",         eagle_read_pad},
		{"text",        eagle_read_pkg_txt},
		{"polygon",     eagle_read_poly}, /* TODO: check this to handle subc */
		{"rectangle",   eagle_read_rect},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, subc, IN_SUBC);
}

static int eagle_read_library_file_pkgs(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	pcb_text_t *t;
	int direction = 0;
	pcb_flag_t TextFlags = pcb_no_flags();

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		pcb_trace("looking at child %s of packages node\n", NODENAME(n)); 
		if (STRCMP(NODENAME(n), "package") == 0) {
			pcb_subc_t *subc;
			pcb_coord_t x, y;

			subc = pcb_subc_alloc();
			pcb_attribute_put(&subc->Attributes, "refdes", "K1");
			pcb_subc_reg(st->pcb->Data, subc);
			pcb_subc_bind_globals(st->pcb, subc);
			eagle_read_pkg(st, n, subc);
			if (pcb_data_is_empty(subc->data)) {
				pcb_subc_free(subc);
				pcb_message(PCB_MSG_WARNING, "Ignoring empty package in library\n");
				continue;
			}

			pcb_attribute_put(&subc->Attributes, "refdes", eagle_get_attrs(st, n, "name", NULL));
			pcb_attribute_put(&subc->Attributes, "value", eagle_get_attrs(st, n, "value", NULL));
			pcb_attribute_put(&subc->Attributes, "footprint", eagle_get_attrs(st, n, "package", NULL));

			pcb_subc_bbox(subc);
TODO("subc: revise this: are we loading an instance here? do we need to place it? do not even bump if not!")
			if (st->pcb->Data->subc_tree == NULL)
				st->pcb->Data->subc_tree = pcb_r_create_tree();
			pcb_r_insert_entry(st->pcb->Data->subc_tree, (pcb_box_t *)subc);
			pcb_subc_rebind(st->pcb, subc);

TODO(": revise rotation and flip")
#if 0
			if ((moduleRotation == 90) || (moduleRotation == 180) || (moduleRotation == 270)) {
				/* lossles module rotation for round steps */
				moduleRotation = moduleRotation / 90;
				pcb_subc_rotate90(subc, moduleX, moduleY, moduleRotation);
			}
			else if (moduleRotation != 0) {
				double rot = moduleRotation;
				pcb_subc_rotate(subc, moduleX, moduleY, cos(rot/PCB_RAD_TO_DEG), sin(rot/PCB_RAD_TO_DEG), rot);
			}
#endif

			size_bump(st, subc->BoundingBox.X2, subc->BoundingBox.Y2);
		}
	}
	return 0;
}

static int eagle_read_lib_pkgs(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	eagle_library_t *lib = obj;
	pcb_text_t *t;

	for(n = CHILDREN(subtree); n != NULL; n = NEXT(n)) {
		if (STRCMP(NODENAME(n), "package") == 0) {
			pcb_subc_t *subc;
			const char *name = eagle_get_attrs(st, n, "name", NULL);

			if ((st->elem_by_name) && (name == NULL)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring package with no name\n");
				continue;
			}

			subc = pcb_subc_alloc();
			eagle_read_pkg(st, n, subc);
			if (pcb_subc_is_empty(subc)) {
				pcb_message(PCB_MSG_WARNING, "Ignoring empty package %s\n", name);
				free(subc);
				continue;
			}

			if (st->elem_by_name)
				htsp_set(&lib->elems, (char *)name, subc);
			st->parser.calls->set_user_data(n, subc);
		}
	}
	return 0;
}

static int eagle_read_library(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"description", eagle_read_nop},
		{"devices",     eagle_read_nop},
		{"symbols",     eagle_read_nop},
		{"devicesets",  eagle_read_nop},
		{"packages",    eagle_read_library_file_pkgs},/* read & place element(s) in library */
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};
	return eagle_foreach_dispatch(st, CHILDREN(subtree), disp, subtree, 0);
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
		pcb_message(PCB_MSG_WARNING, "Failed to parse contactref node: missing \"element\" or \"pad\" netlist attributes\n");
		return -1;
	}

	if (elem != NULL && elem[0] == '-' && elem[1] == '\0') {
		pcb_snprintf(conn, sizeof(conn), "%s-%s", "HYPHEN", pad);
		pcb_message(PCB_MSG_WARNING, "Substituted invalid element name '-' with 'HYPHEN'\n");
	} else {
		pcb_snprintf(conn, sizeof(conn), "%s-%s", elem, pad);
	}

	net = eagle_get_attrs(st, PARENT(subtree), "name", NULL);

	if (net != NULL && net[0] == '-' && net[1] == '\0') { /* pcb-rnd doesn't like it when Eagle uses '-' for GND*/
		pcb_actionl("Netlist", "Add", "GND", conn, NULL);
		pcb_message(PCB_MSG_WARNING, "Substituted contactref net \"GND\" instead of original invalid '-'\n");
	} else {
		pcb_actionl("Netlist", "Add",  net, conn, NULL);
	}
	return 0;
}

static int eagle_read_signals(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	trnode_t *n;
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"contactref",  eagle_read_contactref}, /* if this fails, rest of disp acts up */
		{"wire",        eagle_read_wire},
		{"arc",         eagle_read_circle}, /*binary format */
		{"polygon",     eagle_read_poly},
		{"via",         eagle_read_via},
		{"text",        eagle_read_text},
		{"@text",       eagle_read_nop},
		{NULL, NULL}
	};

	pcb_actionl("Netlist", "Freeze", NULL);
	pcb_actionl("Netlist", "Clear", NULL);

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

	pcb_actionl("Netlist", "Sort", NULL);
	pcb_actionl("Netlist", "Thaw", NULL);

	return 0;
}

static void eagle_read_subc_attrs(read_state_t *st, trnode_t *nd, pcb_subc_t *subc, pcb_coord_t x, pcb_coord_t y, const char *attname, const char *subc_attr, const char *str, pcb_bool add_text)
{
	int direction = 0;
	pcb_coord_t size;

	pcb_attribute_put(&subc->Attributes, subc_attr, str);
	if (!add_text)
		return;

TODO(": text objects should be already created in the library; we should probably set the attributes here only")
#if 0
	y += EAGLE_TEXT_SIZE_100;

	for(nd = CHILDREN(nd); nd != NULL; nd = NEXT(nd)) {
		const char *this_attr = eagle_get_attrs(st, nd, "name", "");
		if (((STRCMP(NODENAME(nd), "attribute") == 0) ||
				(STRCMP(NODENAME(nd), "element2") == 0) )
				&& (strcmp(attname, this_attr) == 0)) {
			direction = eagle_rot2steps(eagle_get_attrs(st, nd, "rot", NULL));
			if (direction < 0)
				direction = 0;
			size = eagle_get_attrc(st, nd, "size", EAGLE_TEXT_SIZE_100);
			break;
		}
	}
#endif
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
			pcb_subc_t *subc, *new_subc;
			int back = 0;

			name = eagle_get_attrs(st, n, "name", NULL);
			val = eagle_get_attrs(st, n, "value", NULL);

			if (name == NULL) {
				pcb_message(PCB_MSG_ERROR, "Element name not found in tree\n");
				name = "refdes_not_found";
				val = "parse_error";
			}

			/* need to get these as string because error messages will use them */
			lib = eagle_get_attrs(st, n, "library", NULL);
			pkg = eagle_get_attrs(st, n, "package", NULL);

			if (st->elem_by_name) { /* xml: library and package are named */
				if ((lib == NULL) || (pkg == NULL)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring element with incomplete library reference\n");
					continue;
				}
				subc = eagle_libelem_by_name(st, lib, pkg);
			}
			else {
				long libi = eagle_get_attrl(st, n, "library", -1);
				long pkgi = eagle_get_attrl(st, n, "package", -1);
				if ((libi < 0) || (pkgi < 0)) {
					pcb_message(PCB_MSG_WARNING, "Ignoring element with broken library reference: %s/%s\n", lib, pkg);
					continue;
				}
				subc = eagle_libelem_by_idx(st, nlib, libi, pkgi);
			}

			/* sanity checks: the element exists and is non-empty */
			if (subc == NULL) {
				pcb_message(PCB_MSG_ERROR, "Library element not found: %s/%s\n", lib, pkg);
				continue;
			}
			if (pcb_subc_is_empty(subc)) {
				pcb_message(PCB_MSG_ERROR, "Not placing empty element: %s/%s\n", lib, pkg);
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

			new_subc = pcb_subc_dup_at(st->pcb, st->pcb->Data, subc, x, y, pcb_false);
			new_subc->Flags = pcb_no_flags();
			new_subc->ID = pcb_create_ID_get();

			eagle_read_subc_attrs(st, n, new_subc, x, y, "PROD_ID", "footprint", pkg,  0);
			eagle_read_subc_attrs(st, n, new_subc, x, y, "NAME",    "refdes",    name, 1);
			eagle_read_subc_attrs(st, n, new_subc, x, y, "VALUE",   "value",     val,  0);

			pcb_subc_bbox(new_subc);
			pcb_subc_rebind(st->pcb, new_subc);

TODO("subc: why not use the arbtirary angle rot?")
			if (rot != NULL) {
				char *end;
				double ang = strtod(rot+1, &end);
				if (*end == '\0') {
					if (fmod(ang, 90) == 0) {
						int steps = eagle_rot2steps(rot);
						if (back)
							steps = (steps + 2)%4;
						if (steps > 0)
							pcb_subc_rotate90(new_subc, x, y, steps);
						else
							pcb_message(PCB_MSG_WARNING, "0 degree element rotation/steps used for '%s'/'%d': %s/%s/%s\n", rot, steps, name, pkg, lib);
					}
					else {
						double sina, cosa;
						ang = -ang;
						sina = sin(ang / PCB_RAD_TO_DEG);
						cosa = cos(ang / PCB_RAD_TO_DEG);
						pcb_subc_rotate(new_subc, x, y, cosa, sina, ang);
					}
				}
				else
					pcb_message(PCB_MSG_ERROR, "syntax error in element rotation '%s': %s/%s/%s\n", rot, name, pkg, lib);
			}

			if (back)
				pcb_subc_change_side(new_subc, 2 * y - st->pcb->MaxHeight);

			size_bump(st, new_subc->BoundingBox.X2, new_subc->BoundingBox.Y2);
		}
	}
	return 0;
}

static int eagle_read_plain(read_state_t *st, trnode_t *subtree, void *obj, int type)
{
	static const dispatch_t disp[] = { /* possible children of <library> */
		{"contactref",  eagle_read_contactref},
		{"wire",        eagle_read_wire},
		{"arc",         eagle_read_circle}, /* binary format */
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

TODO(": test (should process these probably no-net-no-signal objects)")
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
	int res;

	static const dispatch_t disp_1[] = { /* possible children of <drawing> */
		{"settings",  eagle_read_nop},
		{"layers",    eagle_read_layers},
		{"grid",      eagle_read_nop},
		{"board",     eagle_read_nop},
		{"library",   eagle_read_nop}, 
		{"unknown11", eagle_read_nop}, /* TODO: temporary; from the binary tree */
		{"@text",     eagle_read_nop},
		{NULL, NULL}
	};

	static const dispatch_t disp_2[] = { /* possible children of <drawing> */
		{"settings",  eagle_read_nop},
		{"layers",    eagle_read_nop},
		{"grid",      eagle_read_nop},
		{"board",     eagle_read_board},
		{"library",   eagle_read_library},
		{"unknown11", eagle_read_nop}, /* TODO: temporary; from the binary tree */
		{"@text",     eagle_read_nop},
		{NULL, NULL}
	};

	res = eagle_foreach_dispatch(st, CHILDREN(subtree), disp_1, NULL, 0);
	res |= eagle_foreach_dispatch(st, CHILDREN(subtree), disp_2, NULL, 0);
	return res;
}

static int eagle_read_design_rules(read_state_t *st)
{
	trnode_t *dr, *n;
	const char *name;

	/* st->ms_width the default minimum feature width, already defined  */
	st->rv_pad_top = 0.25;
	st->rv_pad_inner = 0.25;
	st->rv_pad_bottom = 0.25;
	st->md_wire_wire = PCB_MIL_TO_COORD(10); /* default minimum wire to wire spacing */

	dr = eagle_trpath(st, st->parser.root, "drawing", "board", "designrules", NULL);
	if (dr == NULL) {
		pcb_message(PCB_MSG_WARNING, "can't find design rules, using sane defaults\n");
	} else {
		for(n = CHILDREN(dr); n != NULL; n = NEXT(n)) {
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
	}
	return 0;
}

static int eagle_read_ver(const char *ver)
{
	int v1, v2, v3;
	char *end;

	v3 = 0;

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
	} else if (*end == '.') {
		v3 = strtol(end+1, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "malformed version string [3] in <eagle>\n");
			return -1;
		}
	}

	/* version check */
	if (v1 < 6) {
		pcb_message(PCB_MSG_ERROR, "file version too old\n");
		return -1;
	}
	if (v1 > 8) {
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
	pcb_layer_group_setup_default(st->pcb);
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

static int post_process_thermals(read_state_t *st)
{
TODO(": process thermals")
	PCB_PADSTACK_LOOP(st->pcb->Data);
	{
	}
	PCB_END_LOOP;
	return 0;
}

int io_eagle_read_pcb_xml(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	int pp_res, res, old_leni;
	read_state_t st;

	static const dispatch_t disp[] = { /* possible children of root */
		{"drawing",        eagle_read_drawing},
		{"compatibility",  eagle_read_nop},
		{"@text",          eagle_read_nop},
		{NULL, NULL}
	};

	/* have not read design rules section yet but need this for rectangle parsing */
	st.ms_width = PCB_MIL_TO_COORD(10); /* default minimum feature width */

	st.parser.calls = &trparse_xml_calls;

	if (st.parser.calls->load(&st.parser, Filename) != 0)
		return -1;

	st.pcb = pcb;
	st.elem_by_name = 1;
	st.default_unit = "mm";
	st_init(&st);

	if (eagle_read_ver(GET_PROP_(&st, st.parser.root, "version")) < 0) {
		pcb_message(PCB_MSG_ERROR, "Eagle XML version parse error\n");
		goto err;
	}

	eagle_read_design_rules(&st);
	old_leni = pcb_create_being_lenient;
	pcb_create_being_lenient = 1;
	res = eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp, NULL, 0);
	if (res == 0) {
		pcb_undo_freeze_add();
		pcb_data_mirror(pcb->Data, 0, PCB_TXM_COORD, 0);
		pcb_undo_unfreeze_add();
	}
	pcb_create_being_lenient = old_leni;
	pcb_board_normalize(pcb);
	pcb_layer_colors_from_conf(pcb, 1);

	pp_res = post_process_thermals(&st);
	st_uninit(&st);
	return pp_res;

err:;
	st_uninit(&st);
	pcb_message(PCB_MSG_ERROR, "Eagle XML parsing error.\n");
	return -1;
}

int io_eagle_read_pcb_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	int pp_res, res, old_leni;
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

	if (st.parser.calls->load(&st.parser, Filename) != 0) {
		printf("parser error\n");
		return -1;
	}

	st.pcb = pcb;
	st.elem_by_name = 0;
	st.default_unit = "du"; /* du = decimicron = 0.1 micron unit for eagle bin format */
	st_init(&st);

	eagle_read_design_rules(&st);

	old_leni = pcb_create_being_lenient;
	pcb_create_being_lenient = 1;
	res = eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_1, NULL, 0);
	res |= eagle_foreach_dispatch(&st, st.parser.calls->children(&st.parser, st.parser.root), disp_2, NULL, 0);
	if (res == 0) {
		pcb_undo_freeze_add();
		pcb_data_mirror(pcb->Data, 0, PCB_TXM_COORD, 0);
		pcb_undo_unfreeze_add();
	}
	pcb_create_being_lenient = old_leni;
	pcb_board_normalize(pcb);
	pcb_layer_colors_from_conf(pcb, 1);

	pp_res = post_process_thermals(&st);
	st_uninit(&st);
	return pp_res;
}

