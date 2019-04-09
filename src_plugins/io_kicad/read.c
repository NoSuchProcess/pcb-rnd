/*
 *			COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016, 2017 Erich S. Heinzle
 *	Copyright (C) 2017 Miloh
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include <stddef.h>
#include <assert.h>
#include <math.h>
#include <gensexpr/gsxl.h>
#include <genht/htsi.h>
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "data.h"
#include "read.h"
#include "layer.h"
#include "polygon.h"
#include "plug_footprint.h"
#include "misc_util.h" /* for distance calculations */
#include "conf_core.h"
#include "move.h"
#include "macro.h"
#include "rotate.h"
#include "safe_fs.h"
#include "attrib.h"
#include "netlist2.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"
#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/media.h"

/* a reasonable approximation of pcb glyph width, ~=  5000 centimils; in mm */
#define GLYPH_WIDTH (1.27)

typedef enum {
	DIM_PAGE,
	DIM_AREA,
	DIM_FALLBACK,
	DIM_max
} kicad_dim_prio_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_data_t *fp_data;
	const char *Filename;
	conf_role_t settings_dest;
	gsxl_dom_t dom;
	unsigned auto_layers:1;
	htsi_t layer_k2i; /* layer name-to-index hash; name is the kicad name, index is the pcb-rnd layer index */
	long ver;

	pcb_coord_t width[DIM_max];
	pcb_coord_t height[DIM_max];
	pcb_coord_t dim_valid[DIM_max];

	/* setup */
	pcb_coord_t pad_to_mask_clearance;
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser) (read_state_t *st, gsxl_node_t *subtree);
} dispatch_t;

typedef struct {
	const char *node_name;
	int offset;
	enum {
		KICAD_COORD,
		KICAD_DOUBLE
	} type;
} autoload_t;

static int kicad_error(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	pcb_append_printf(&str, "io_kicad parse error at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

static int kicad_warning(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	pcb_append_printf(&str, "io_kicad warning at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	pcb_message(PCB_MSG_WARNING, "%s", str.array);

	gds_uninit(&str);
	return 0;
}



/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int kicad_dispatch(read_state_t *st, gsxl_node_t *subtree, const dispatch_t *disp_table)
{
	const dispatch_t *d;

	/* do not tolerate empty/NIL node */
	if (subtree->str == NULL)
		return kicad_error(subtree, "unexpected empty/NIL subtree");

	for(d = disp_table; d->node_name != NULL; d++)
		if (strcmp(d->node_name, subtree->str) == 0)
			return d->parser(st, subtree->children);

	/* node name not found in the dispatcher table */
	return kicad_error(subtree, "Unknown node: '%s'", subtree->str);
}

/* Take each children of tree and execute them using kicad_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */
static int kicad_foreach_dispatch(read_state_t *st, gsxl_node_t *tree, const dispatch_t *disp_table)
{
	gsxl_node_t *n;

	for(n = tree; n != NULL; n = n->next)
		if (kicad_dispatch(st, n, disp_table) != 0)
			return -1;

	return 0; /* success */
}

/* No-op: ignore the subtree */
static int kicad_parse_nop(read_state_t *st, gsxl_node_t *subtree)
{
	return 0;
}

/* kicad_pcb/version */
static int kicad_parse_version(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {
		st->ver = atoi(subtree->str);
		if (st->ver == 3 || st->ver == 4 || st->ver == 20170123)
			return 0;
	}
	return kicad_error(subtree, "unexpected layout version");
}

/* Parse a layer definition and do all the administration needed for the layer */
static int kicad_create_layer(read_state_t *st, int lnum, const char *lname, const char *ltype, gsxl_node_t *subtree, int last_copper)
{
	pcb_layer_id_t id = -1;
	pcb_layergrp_id_t gid = -1;
	pcb_layer_t *ly;

	if (lnum <= last_copper) { /* handle copper layers */
		pcb_layer_type_t loc = PCB_LYT_INTERN;

		if (strcmp(lname+1, ".Cu") == 0) {
			if (st->ver > 20170000) {
				if ((lnum == 0) && (lname[0] != 'F'))
					kicad_warning(subtree, "layer 0 should be named F.Cu (recoverable error; new stack)\n");
				if ((lnum == last_copper) && (lname[0] != 'B'))
					kicad_warning(subtree, "layer %d should be named B.Cu (recoverable error; new stack)\n", last_copper);
			}
			else {
				if ((lnum == 0) && (lname[0] != 'B'))
					kicad_warning(subtree, "layer 0 should be named B.Cu (recoverable error; old stack)\n");
				if ((lnum == last_copper) && (lname[0] != 'F'))
					kicad_warning(subtree, "layer %d should be named F.Cu (recoverable error; old stack)\n", last_copper);
			}
		}
		else
			kicad_warning(subtree, "layer %d name should end in .Cu (recoverable error)\n", last_copper);

		if (st->ver > 20170000) {
			if (lnum == 0) loc = PCB_LYT_TOP;
			else if (lnum == last_copper) loc = PCB_LYT_BOTTOM;
		}
		else {
			if (lnum == 0) loc = PCB_LYT_BOTTOM;
			else if (lnum == last_copper) loc = PCB_LYT_TOP;
		}

		pcb_layergrp_list(st->pcb, PCB_LYT_COPPER | loc, &gid, 1);
		id = pcb_layer_create(st->pcb, gid, lname);
		htsi_set(&st->layer_k2i, pcb_strdup(lname), id);
		return 0;
	}

TODO(": we should not depend on layer IDs other than 0")
	switch (lnum) {
		default:
			if (strcmp(lname, "Edge.Cuts") == 0) {
				/* Edge must be the outline */
				pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);
				pcb_layergrp_fix_turn_to_outline(g);
				id = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, lname);
			}
			else if ((strncmp(lname, "Dwgs.", 4) == 0) || (strncmp(lname, "Cmts.", 4) == 0) || (strncmp(lname, "Eco", 3) == 0)) {
				/* Create a new inner layer for emulating misc layers */
				pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);
				id = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, lname);
			}
			else if ((lname[1] == '.') && ((lname[0] == 'F') || (lname[0] == 'B'))) { /* F. or B. layers */
				if (strcmp(lname + 2, "SilkS") == 0) {
					pcb_layergrp_list(st->pcb, PCB_LYT_SILK | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP), &gid, 1);
					id = pcb_layer_create(st->pcb, gid, lname);
					ly = pcb_get_layer(st->pcb->Data, id);
					ly->comb |= PCB_LYC_AUTO;
					break;
				}
				if (strcmp(lname + 2, "Paste") == 0) {
					pcb_layergrp_list(st->pcb, PCB_LYT_PASTE | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP), &gid, 1);
					id = pcb_layer_create(st->pcb, gid, lname);
					ly = pcb_get_layer(st->pcb->Data, id);
					ly->comb |= PCB_LYC_AUTO;
					break;
				}
				if (strcmp(lname + 2, "Mask") == 0) {
					pcb_layergrp_list(st->pcb, PCB_LYT_MASK | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP), &gid, 1);
					id = pcb_layer_create(st->pcb, gid, lname);
					ly = pcb_get_layer(st->pcb->Data, id);
					ly->comb |= PCB_LYC_AUTO | PCB_LYC_SUB;
					break;
				}
				if (strcmp(lname + 2, "Adhes") == 0) { /* mech layer */
					pcb_layergrp_t *grp = pcb_get_grp_new_misc(st->pcb);
					grp->name = pcb_strdup(lname);
					grp->ltype = PCB_LYT_MECH | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP);
					grp->purpose = pcb_strdup(lname+2);
					id = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, lname);
					break;
				}
				if ((strcmp(lname + 2, "CrtYd") == 0) || (strcmp(lname + 2, "Fab") == 0)) { /* documentation layers */
					pcb_layergrp_t *grp = pcb_get_grp_new_misc(st->pcb);
					grp->name = pcb_strdup(lname);
					grp->ltype = PCB_LYT_DOC | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP);
					grp->purpose = pcb_strdup(lname+2);
					id = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, lname);
					break;
				}
				kicad_warning(subtree, "unknown layer: %s", lname);
				goto hack1;
			}
			else if (lnum > last_copper) {
			hack1:;
				/* HACK/WORKAROUND: remember kicad layers for those that are unsupported */
				htsi_set(&st->layer_k2i, pcb_strdup(lname), -lnum);
				return 0;
			}
			else
				return -1; /* unknown field */
	}

/* valid layer, save it in the hash */
	if (id >= 0) {
		htsi_set(&st->layer_k2i, pcb_strdup(lname), id);
	}
	else {
		assert(id < -1);
	}
	return 0;
}

/* Register a kicad layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int kicad_reg_layer(read_state_t *st, const char *kicad_name, unsigned int mask, const char *purpose)
{
	pcb_layer_id_t id;
	if (st->pcb != NULL) {
		if (pcb_layer_listp(st->pcb, mask, &id, 1, -1, purpose) != 1) {
			pcb_layergrp_id_t gid;
			pcb_layergrp_listp(PCB, mask, &gid, 1, -1, purpose);
			id = pcb_layer_create(st->pcb, gid, kicad_name);
		}
	}
	else {
		/* registering a new layer in buffer */
		pcb_layer_t *ly = pcb_layer_new_bound(st->fp_data, mask, kicad_name, purpose);
		id = ly - st->fp_data->Layer;
	}
	htsi_set(&st->layer_k2i, pcb_strdup(kicad_name), id);
	return 0;
}

static int kicad_get_layeridx_auto(read_state_t *st, const char *kicad_name);

/* Returns the pcb-rnd layer index for a kicad_name, or -1 if not found */
static int kicad_get_layeridx(read_state_t *st, const char *kicad_name)
{
	htsi_entry_t *e;
	e = htsi_getentry(&st->layer_k2i, kicad_name);
	if (e == NULL) {
		if ((kicad_name[0] == 'I') && (kicad_name[1] == 'n')) {
			/* Workaround: specal case InX.Cu, where X is an integer */
			char *end;
			long id = strtol(kicad_name + 2, &end, 10);
			if ((pcb_strcasecmp(end, ".Cu") == 0) && (id >= 1) && (id <= 30)) {
				if (kicad_reg_layer(st, kicad_name, PCB_LYT_COPPER | PCB_LYT_INTERN, NULL) == 0)
					return kicad_get_layeridx(st, kicad_name);
			}
		}
		if (st->auto_layers)
			return kicad_get_layeridx_auto(st, kicad_name);
		return -1;
	}
	return e->value;
}

static int kicad_get_layeridx_auto(read_state_t *st, const char *kicad_name)
{
	pcb_layer_type_t lyt = 0;
	const char *purpose = NULL;

	if ((kicad_name[0] == 'F') && (kicad_name[1] == '.')) lyt |= PCB_LYT_TOP;
	else if ((kicad_name[0] == 'B') && (kicad_name[1] == '.')) lyt |= PCB_LYT_BOTTOM;
	else if ((kicad_name[0] == 'I') && (kicad_name[1] == 'n')) lyt |= PCB_LYT_INTERN;

	if (pcb_strcasecmp(kicad_name, "Edge.Cuts") == 0) { lyt |= PCB_LYT_BOUNDARY; purpose = "uroute"; }
	else if (kicad_name[1] == '.') {
		const char *ln = kicad_name + 2;
		if (pcb_strcasecmp(ln, "SilkS") == 0) lyt |= PCB_LYT_SILK;
		else if (pcb_strcasecmp(ln, "Mask") == 0) lyt |= PCB_LYT_MASK;
		else if (pcb_strcasecmp(ln, "Paste") == 0) lyt |= PCB_LYT_PASTE;
		else if (pcb_strcasecmp(ln, "Cu") == 0) lyt |= PCB_LYT_COPPER;
		else lyt |= PCB_LYT_VIRTUAL;
	}
	else lyt |= PCB_LYT_VIRTUAL;

	if (kicad_reg_layer(st, kicad_name, lyt, purpose) == 0)
		return kicad_get_layeridx(st, kicad_name);
	return -1;
}


static pcb_layer_t *kicad_get_subc_layer(read_state_t *st, pcb_subc_t *subc, const char *layer_name, const char *default_layer_name)
{
	int pcb_idx = -1;
	pcb_layer_id_t lid;
	pcb_layer_type_t lyt;
	pcb_layer_combining_t comb;
	const char *lnm;

	if (layer_name != NULL) {
		/* check if the layer already exists (by name) */
		lid = pcb_layer_by_name(subc->data, layer_name);
		if (lid >= 0)
			return &subc->data->Layer[lid];

		pcb_idx = kicad_get_layeridx(st, layer_name);
		lnm = layer_name;
		if (pcb_idx < 0) {
			pcb_message(PCB_MSG_ERROR, "\tfp_* layer '%s' not found for module object, using unbound subc layer instead.\n", layer_name);
			lyt = PCB_LYT_VIRTUAL;
			comb = 0;
			return pcb_subc_get_layer(subc, lyt, comb, 1, lnm, pcb_true);
		}
	}
	else {
		/* check if the layer already exists (by name) */
		lid = pcb_layer_by_name(subc->data, default_layer_name);
		if (lid >= 0)
			return &subc->data->Layer[lid];

		pcb_message(PCB_MSG_ERROR, "\tfp_* layer '%s' not found for module object, using module layer '%s' instead.\n", layer_name, default_layer_name);
		pcb_idx = kicad_get_layeridx(st, default_layer_name);
		if (pcb_idx < 0)
			return NULL;
		lnm = default_layer_name;
	}

	if (st->pcb == NULL)
		lyt = st->fp_data->Layer[pcb_idx].meta.bound.type;
	else
		lyt = pcb_layer_flags(st->pcb, pcb_idx);
	comb = 0;
	return pcb_subc_get_layer(subc, lyt, comb, 1, lnm, pcb_true);
}

#define SEEN_NO_DUP(bucket, bit) \
do { \
	int __mask__ = (1<<(bit)); \
	if ((bucket) & __mask__) \
		return -1; \
	bucket |= __mask__; \
} while(0)

#define SEEN(bucket, bit) \
do { \
	int __mask__ = (1<<(bit)); \
	bucket |= __mask__; \
} while(0)

#define BV(bit) (1<<(bit))

#define ignore_value(n, err) \
do { \
	if ((n)->children != NULL && (n)->children->str != NULL) { } \
	else \
		return kicad_error(n, err); \
} while(0)

#define ignore_value_nodup(n, tally, bitval, err) \
do { \
	SEEN_NO_DUP(tally, bitval); \
	ignore_value(n, err); \
} while(0)

static int kicad_update_size(read_state_t *st)
{
	int n;
	for(n = 0; n < DIM_max; n++) {
		if (st->dim_valid[n]) {
			st->pcb->MaxWidth = st->width[n];
			st->pcb->MaxHeight = st->height[n];
			return 0;
		}
	}

	return 0;
}

/* kicad_pcb/parse_page */
static int kicad_parse_page_size(read_state_t *st, gsxl_node_t *subtree)
{
	pcb_media_t *m;

	if ((subtree == NULL) || (subtree->str == NULL))
		return kicad_error(subtree, "error parsing KiCad layout size.");

	for(m = pcb_media_data; m->name != NULL; m++) {
		if (strcmp(m->name, subtree->str) == 0) {
			/* pivot: KiCad assumes portrait */
			st->width[DIM_PAGE] = m->height;
			st->height[DIM_PAGE] = m->width;
			st->dim_valid[DIM_PAGE] = 1;
			return kicad_update_size(st);
		}
	}

	kicad_error(subtree, "Unknown layout size '%s', using fallback.\n", subtree->str);
	return kicad_update_size(st);
}


/* kicad_pcb/parse_title_block */
static int kicad_parse_title_block(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	const char *prefix = "kicad_titleblock_";
	char *name;

	if (subtree->str == NULL)
		return kicad_error(subtree, "error parsing KiCad titleblock: empty");

	name = pcb_concat(prefix, subtree->str, NULL);
	pcb_attrib_put(st->pcb, name, subtree->children->str);
	free(name);
	for(n = subtree->next; n != NULL; n = n->next) {
		if (n->str != NULL && strcmp("comment", n->str) != 0) {
			name = pcb_concat(prefix, n->str, NULL);
			pcb_attrib_put(st->pcb, name, n->children->str);
			free(name);
		}
		else { /* if comment field has extra children args */
			name = pcb_concat(prefix, n->str, "_", n->children->str, NULL);
			pcb_attrib_put(st->pcb, name, n->children->next->str);
			free(name);
		}
	}
	return 0;
}

static int rotdeg_to_dir(double rotdeg)
{
	if ((rotdeg > 45.0) && (rotdeg <= 135.0))   return 1;
	if ((rotdeg > 135.0) && (rotdeg <= 225.0))  return 2;
	if ((rotdeg > 225.0) && (rotdeg <= 315.0))  return 3;
	return 0;
}

/* Convert the string value of node to double and store it in res. On conversion
   error report error on node using errmsg. If node == NULL: report error
   on missnode, or ignore the problem if missnode is NULL. */
#define PARSE_DOUBLE(res, missnode, node, errmsg) \
do { \
	gsxl_node_t *__node__ = (node); \
	if ((__node__ != NULL) && (__node__->str != NULL)) { \
		char *__end__; \
		double __val__ = strtod(__node__->str, &__end__); \
		if (*__end__ != 0) \
			return kicad_error(node, "Invalid numeric (double) " errmsg); \
		else \
			(res) = __val__; \
	} \
	else if (missnode != NULL) \
		return kicad_error(missnode, "Missing child node for " errmsg); \
} while(0) \

/* same as PARSE_DOUBLE() but res is a pcb_coord_t, input string is in mm */
#define PARSE_COORD(res, missnode, node, errmsg) \
do { \
	double __dtmp__; \
	PARSE_DOUBLE(__dtmp__, missnode, node, errmsg); \
	(res) = pcb_round(PCB_MM_TO_COORD(__dtmp__)); \
} while(0) \

/* parse nd->str into a pcb_layer_t in ly, either as a subc layer or if subc
   is NULL, as an st->pcb board layer. loc is a string literal, parent context
   of the text, used in the error message.
   NOTE: subc case: for text there is no subc default layer (assumes KiCad has text on silk only) */
#define PARSE_LAYER(ly, nd, subc, loc) \
	do { \
		if ((nd == NULL) || (nd->str == NULL)) \
			return kicad_error(n, "unexpected empty/NULL " loc " layer node"); \
		if (subc == NULL) { \
			pcb_layer_id_t lid = kicad_get_layeridx(st, nd->str); \
			if (lid < 0) \
				return kicad_error(nd, "unhandled " loc " layer: (%s)", nd->str); \
			ly = &st->pcb->Data->Layer[lid]; \
		} \
		else \
			ly = kicad_get_subc_layer(st, subc, nd->str, NULL); \
	} while(0)

/* Search the autoload table for subtree->str, convert data to a struct field
   on match with the children ("parameters") of the subtree */
static int kicad_autoload(read_state_t *st, gsxl_node_t *subtree, void *dst_struct, const autoload_t *auto_table, int allow_unknown)
{
	const autoload_t *a;

	/* do not tolerate empty/NIL node */
	if (subtree->str == NULL)
		return kicad_error(subtree, "unexpected empty/NIL subtree");

	for(a = auto_table; a->node_name != NULL; a++) {
		if (strcmp(a->node_name, subtree->str) == 0) {
			char *dst = ((char *)dst_struct) + a->offset;
			switch(a->type) {
				case KICAD_DOUBLE:
					{
						double *d = (double *)dst;
						PARSE_DOUBLE(*d, NULL, subtree->children, "");
					}
					break;
				case KICAD_COORD:
					{
						pcb_coord_t *d = (pcb_coord_t *)dst;
						PARSE_COORD(*d, NULL, subtree->children, "");
					}
					break;
			}
		}
	}

	if (allow_unknown)
		return 0;

	/* node name not found in the dispatcher table */
	return kicad_error(subtree, "Unknown node: '%s'", subtree->str);
}

/* Take each children of tree and execute them using kicad_autoload
   Useful for procssing nodes that may host various subtrees of different
   nodes in a flexible way. Return non-zero if any subtree processor failed. */
static int kicad_foreach_autoload(read_state_t *st, gsxl_node_t *tree, void *dst, const autoload_t *auto_table, int allow_unknown)
{
	gsxl_node_t *n;

	for(n = tree; n != NULL; n = n->next)
		if (kicad_autoload(st, n, dst, auto_table, allow_unknown) != 0)
			return -1;

	return 0; /* success */
}


static int kicad_parse_general_area(read_state_t *st, gsxl_node_t *subtree)
{
	if ((subtree->str == NULL) || (subtree->next->str == NULL) || (subtree->next->next->str == NULL) || (subtree->next->next->next->str == NULL))
		return kicad_error(subtree, "area requires 4 arguments.\n");

	PARSE_COORD(st->width[DIM_AREA], NULL, subtree->next->next, "area x2");
	PARSE_COORD(st->height[DIM_AREA], NULL, subtree->next->next->next, "area y2");
	st->dim_valid[DIM_AREA] = 1;
	return kicad_update_size(st);
}

static int kicad_parse_general(read_state_t *st, gsxl_node_t *subtree)
{
	static const dispatch_t disp[] = { /* possible children of root */
		{"links", kicad_parse_nop},
		{"no_connects", kicad_parse_nop},
		{"area", kicad_parse_general_area},
		{"thickness", kicad_parse_nop},
		{"drawings", kicad_parse_nop},
		{"tracks", kicad_parse_nop},
		{"zones", kicad_parse_nop},
		{"modules", kicad_parse_nop},
		{"nets", kicad_parse_nop},
		{NULL, NULL}
	};

	/* Call the corresponding subtree parser for each child node of the root
	   node; if any of them fail, parse fails */
	return kicad_foreach_dispatch(st, subtree, disp);
}

static int kicad_parse_setup(read_state_t *st, gsxl_node_t *subtree)
{
	static const autoload_t atbl[] = {
		{"pad_to_mask_clearance", offsetof(read_state_t, pad_to_mask_clearance), KICAD_COORD},
		{NULL, 0, 0}
	};
	return kicad_foreach_autoload(st, subtree, st, atbl, 1);
}


/* kicad_pcb/gr_text and fp_text */
static int kicad_parse_any_text(read_state_t *st, gsxl_node_t *subtree, char *text, pcb_subc_t *subc, double mod_rot)
{
	gsxl_node_t *l, *n, *m;
	int i;
	unsigned long tally = 0, required;
	double rotdeg = 0.0;  /* default is horizontal */
	pcb_coord_t X, Y, thickness = 0;
	int scaling = 100;
	int txt_len;
	int mirrored = 0;
	unsigned direction;
	pcb_flag_t flg = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_t *ly;

	txt_len = strlen(text);
	for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
		if (n->str == NULL)
			return kicad_error(n, "empty text node");
		if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(X, n, n->children, "text X1");
			PARSE_COORD(Y, n, n->children->next, "text Y1");
			PARSE_DOUBLE(rotdeg, NULL, n->children->next->next, "text rotation");
			rotdeg -= mod_rot;
			if (subc != NULL) {
				pcb_coord_t sx, sy;
				if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
					X += sx;
					Y += sy;
				}
			}
			direction = rotdeg_to_dir(rotdeg); /* used for centering only */
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_LAYER(ly, n->children, subc, "text");
			if (subc == NULL) {
				if (pcb_layer_flags_(ly) & PCB_LYT_BOTTOM)
					flg = pcb_flag_make(PCB_FLAG_ONSOLDER);
			}
		}
		else if (strcmp("hide", n->str) == 0) {
			if (subc != NULL)
				return 0; /* simply don't create the object */
			else
				kicad_warning(n, "'hide' is invalid for gr_text (ignored)");
		}
		else if (strcmp("effects", n->str) == 0) {
			for(m = n->children; m != NULL; m = m->next) {
				if (m->str == NULL)
					return kicad_error(m, "empty text effect");
				if (strcmp("font", m->str) == 0) {
					for(l = m->children; l != NULL; l = l->next) {
						if (m->str != NULL && strcmp("size", l->str) == 0) {
							double sx, sy;
							SEEN_NO_DUP(tally, 2);
							PARSE_DOUBLE(sx, l, l->children, "text size X");
							PARSE_DOUBLE(sy, l, l->children->next, "text size Y");
							scaling = (int)(100 * ((sx+sy)/2.0) / 1.27); /* standard glyph width ~= 1.27mm */
							if (sx != sy)
								kicad_warning(l->children, "text font size mismatch in X and Y direction - skretching is not yet supported, using the average");
						}
						else if (strcmp("thickness", l->str) == 0) {
							SEEN_NO_DUP(tally, 3);
							PARSE_COORD(thickness, l, l->children, "text thickness");
						}
					}
				}
				else if (strcmp("justify", m->str) == 0) {
					if ((m->children == NULL) || (m->children->str == NULL))
						return kicad_error(m, "unexpected empty/NULL text justify node");
					if (strcmp("mirror", m->children->str) == 0) {
						mirrored = 1;
						SEEN_NO_DUP(tally, 4);
					}
					TODO("right or left justification is ignored CUCP#38");
				}
				else
					kicad_warning(m, "Unknown text effects argument %s:", m->str);
			}
		}
	}

	/* has location, layer, size and stroke thickness at a minimum */
	required = BV(0) | BV(1) | BV(2) | BV(3);
	if ((tally & required) != required)
		return kicad_error(subtree, "failed to create text due to missing fields");

	{
		pcb_coord_t mx, my;
		int swap;
		if (mirrored != 0) {
			if (direction % 2 == 0) {
			rotdeg = fmod((rotdeg + 180.0), 360.0);
				direction += 2;
				direction = direction % 4;
			}
			switch(direction) {
				case 0: mx = -1; my = +1; swap = 0; break;
				case 1: mx = -1; my = -1; swap = 1; break;
				case 2: mx = +1; my = -1; swap = 0; break;
				case 3: mx = +1; my = +1; swap = 1; break;
			}
		}
		else { /* not back of board text */
			switch(direction) {
				case 0: mx = -1; my = -1; swap = 0; break;
				case 1: mx = +1; my = -1; swap = 1; break;
				case 2: mx = +1; my = +1; swap = 0; break;
				case 3: mx = -1; my = +1; swap = 1; break;
			}
		}
		if (swap) {
			Y += mx * PCB_MM_TO_COORD((GLYPH_WIDTH * txt_len) / 2.0);
			X += my * PCB_MM_TO_COORD(GLYPH_WIDTH / 2.0); /* centre it vertically */
		}
		else {
			X += mx * PCB_MM_TO_COORD((GLYPH_WIDTH * txt_len) / 2.0);
			Y += my * PCB_MM_TO_COORD(GLYPH_WIDTH / 2.0); /* centre it vertically */
		}
		pcb_text_new(ly, pcb_font(st->pcb, 0, 1), X, Y, rotdeg, scaling, thickness, text, flg);
	}
	return 0; /* create new font */
}

/* kicad_pcb/gr_text */
static int kicad_parse_gr_text(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL)
		return kicad_parse_any_text(st, subtree, subtree->str, NULL, 0.0);
	return kicad_error(subtree, "failed to create text: missing text string");
}

/* kicad_pcb/gr_line or fp_line */
static int kicad_parse_any_line(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, pcb_flag_values_t flag, int is_seg)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;
	pcb_coord_t x1, y1, x2, y2, thickness, clearance;
	pcb_flag_t flg = pcb_flag_make(flag);
	pcb_layer_t *ly = NULL;

	TODO("figure how default clearance works, probably not subc specific CUCP#39");
	clearance = thickness = 1; /* start with sane default of one nanometre */
	if (subc != NULL)
		clearance = 0;

	TODO("this workaround is for segment - remove it when clearance is figured CUCP#39");
	if (flag & PCB_FLAG_CLEARLINE)
		clearance = PCB_MM_TO_COORD(0.250);

	for(n = subtree; n != NULL; n = n->next) {
		if (n->str == NULL)
			return kicad_error(n, "empty line parameter");
		if (strcmp("start", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(x1, n, n->children, "line x1 coord");
			PARSE_COORD(y1, n, n->children->next, "line y1 coord");
		}
		else if (strcmp("end", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_COORD(x2, n, n->children, "line x2 coord");
			PARSE_COORD(y2, n, n->children->next, "line y2 coord");
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 2);
			PARSE_LAYER(ly, n->children, subc, "line");
		}
		else if (strcmp("width", n->str) == 0) {
			SEEN_NO_DUP(tally, 3);
			PARSE_COORD(thickness, n, n->children, "line thickness");
		}
		else if (strcmp("angle", n->str) == 0) { /* unlikely to be used or seen */
			ignore_value_nodup(n, tally, 4, "unexpected empty/NULL line angle");
		}
		else if (strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
			ignore_value_nodup(n, tally, 5, "unexpected empty/NULL line net.");
		}
		else if (strcmp("status", n->str) == 0) {
			if (is_seg) {
				/* Karl says: the status field dissappears when opening the file pcbnew and "save as" some name.
				   Some wild guesses:
				    if polygon has been recalculated
				    something with rats nests
				    used in older versions, obsolete now
				*/
				/* Conclusion: ignore */
			}
			else
				return kicad_error(n, "unexpected status in line object (only segment should have a status)");
		}
		else if (strcmp("tstamp", n->str) == 0) {
			/* ignore */
		}
		else
			kicad_warning(n, "unexpected line node: %s", n->str);
	}

	/* need start, end, layer, thickness at a minimum */
	required = BV(0) | BV(1) | BV(2); /* | BV(3); now have 1nm default width, i.e. for edge cut */
	if ((tally & required) != required)
		return kicad_error(subtree, "failed to create line: missing fields");

	if (subc != NULL) {
		pcb_coord_t sx, sy;
		if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
			x1 += sx;
			y1 += sy;
			x2 += sx;
			y2 += sy;
		}
	}
	pcb_line_new(ly, x1, y1, x2, y2, thickness, clearance, flg);
	return 0;
}

/* kicad_pcb/gr_line */
static int kicad_parse_gr_line(read_state_t *st, gsxl_node_t *subtree)
{
	return kicad_parse_any_line(st, subtree, NULL, 0, 0);
}

/* kicad_pcb/gr_arc, gr_cicle, fp_arc, fp_circle */
static int kicad_parse_any_arc(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;
	pcb_coord_t cx, cy, endx, endy, thickness, clearance, deltax, deltay;
	pcb_angle_t end_angle = 0.0, delta = 360.0; /* these defaults allow a gr_circle to be parsed, which does not specify (angle XXX) */
	pcb_flag_t flg = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_t *ly = NULL;

	TODO("Figure the clearance and what happens without thickness; shouldn't depend on subc CUCP#39");
	if (subc == NULL)
		clearance = thickness = PCB_MM_TO_COORD(0.250); /* start with sane defaults */
	else
		clearance = thickness = PCB_MM_TO_COORD(0);

	for(n = subtree; n != NULL; n = n->next) {
		if (n->str == NULL)
			return kicad_error(n, "empty arc argument");
		if (strcmp("start", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(cx, n, n->children, "arc start X coord");
			PARSE_COORD(cy, n, n->children->next, "arc start Y coord");
		}
		else if (strcmp("center", n->str) == 0) { /* this lets us parse a circle too */
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(cx, n, n->children, "arc start X coord");
			PARSE_COORD(cy, n, n->children->next, "arc start Y coord");
		}
		else if (strcmp("end", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_COORD(endx, n, n->children, "arc end X coord");
			PARSE_COORD(endy, n, n->children->next, "arc end Y coord");
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 2);
			PARSE_LAYER(ly, n->children, subc, "arc");
		}
		else if (strcmp("width", n->str) == 0) {
			SEEN_NO_DUP(tally, 3);
			PARSE_COORD(thickness, n, n->children, "arc width");
		}
		else if (strcmp("angle", n->str) == 0) {
			PARSE_DOUBLE(delta, n, n->children, "arc angle");
		}
		else if (strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
			ignore_value(n, "unexpected empty/NULL arc net.");
		}
		else if (strcmp("tstamp", n->str) == 0) {
			/* ignore */
		}
		else
			kicad_warning(n, "Unknown arc argument %s:", n->str);
	}

 /* need start, end, layer, thickness at a minimum */
	required = BV(0) | BV(1) | BV(2) | BV(3); /* | BV(4); not needed for circles */
	if ((tally & required) != required)
		return kicad_error(subtree, "unexpected empty/NULL node in arc.");

	{
		pcb_angle_t start_angle;
		pcb_coord_t width, height;

		width = height = pcb_distance(cx, cy, endx, endy); /* calculate radius of arc */
		deltax = endx - cx;
		deltay = endy - cy;
		if (width < 1) { /* degenerate case */
			start_angle = 0;
		}
		else {
			end_angle = 180 + 180 * atan2(-deltay, deltax) / M_PI;
			/* avoid using atan2 with zero parameters */

			if (end_angle < 0.0)
				end_angle += 360.0; /* make it 0...360 */
			start_angle = (end_angle - delta); /* pcb-rnd is 180 degrees out of phase with kicad, and opposite direction rotation */
			if (start_angle > 360.0)
				start_angle -= 360.0;
			if (start_angle < 0.0)
				start_angle += 360.0;
		}

		if (subc != NULL) {
			pcb_coord_t sx, sy;
			if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
				cx += sx;
				cy += sy;
			}
		}
		pcb_arc_new(ly, cx, cy, width, height, start_angle, delta, thickness, clearance, flg, pcb_true);
	}

	return 0;
}

static int kicad_parse_gr_arc(read_state_t *st, gsxl_node_t *subtree)
{
	return kicad_parse_any_arc(st, subtree, NULL);
}


/* kicad_pcb/via */
static int kicad_parse_via(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *m, *n;
	unsigned long tally = 0, required;
	pcb_coord_t x, y, thickness, clearance, mask, drill; /* not sure what to do with mask */

	TODO("need to figure the clearance CUCP#39");
	clearance = mask = PCB_MM_TO_COORD(0.250); /* start with something bland here */
	drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	for(n = subtree; n != NULL; n = n->next) {
		if (n->str == NULL)
			return kicad_error(n, "empty via argument node");
		if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(x, n, n->children, "via X coord");
			PARSE_COORD(y, n, n->children->next, "via Y coord");
		}
		else if (strcmp("size", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_COORD(thickness, n, n->children, "via size coord");
		}
		else if (strcmp("layers", n->str) == 0) {
			SEEN_NO_DUP(tally, 2);
			for(m = n->children; m != NULL; m = m->next) {
				if (m->str != NULL) {
TODO("bbvia CUCP#41");
					/*PARSE_LAYER(ly, n->children, subc, "via");*/
				}
				else
					return kicad_error(m, "unexpected empty/NULL via layer node");
			}
		}
		else if (strcmp("net", n->str) == 0) {
			ignore_value_nodup(n, tally, 3, "unexpected empty/NULL via net node");
		}
		else if (strcmp("tstamp", n->str) == 0) {
			ignore_value_nodup(n, tally, 4, "unexpected empty/NULL via tstamp node");
		}
		else if (strcmp("drill", n->str) == 0) {
			SEEN_NO_DUP(tally, 5);
			PARSE_COORD(drill, n, n->children, "via drill size");
		}
		else
			kicad_warning(n, "Unknown via argument %s:", n->str);
	}

	/* need start, end, layer, thickness at a minimum */
	required = BV(0) | BV(1);
	if ((tally & required) != required)
		return kicad_error(subtree, "insufficient via arguments");

	if (pcb_pstk_new_compat_via(st->pcb->Data, -1, x, y, drill, thickness, clearance, mask, PCB_PSTK_COMPAT_ROUND, pcb_true) == NULL)
		return kicad_error(subtree, "failed to create via-padstack");
	return 0;
}

/* kicad_pcb/segment */
static int kicad_parse_segment(read_state_t *st, gsxl_node_t *subtree)
{
	return kicad_parse_any_line(st, subtree, NULL, PCB_FLAG_CLEARLINE, 1);
}

/* kicad_pcb  parse (layers  )  - either board layer defintions, or module pad/via layer defs */
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	int last_copper;

	if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) /* test if deeper in tree than layer definitions for entire board */
		return kicad_error(subtree, "layer definition found in unexpected location in KiCad layout");

	/* we are just below the top level or root of the tree, so this must be a layer definitions section */
	pcb_layergrp_inhibit_inc();
	pcb_layer_group_setup_default(st->pcb);


TODO("check if we really need these excess layers");
#if 0
	/* set up the hash for implicit layers for modules */
	res = 0;
	res |= kicad_reg_layer(st, "Top", PCB_LYT_COPPER | PCB_LYT_TOP, NULL);
	res |= kicad_reg_layer(st, "Bottom", PCB_LYT_COPPER | PCB_LYT_BOTTOM, NULL);

	if (res != 0) {
		kicad_error(subtree, "Internal error: can't find a silk or mask layer while parsing KiCad layout");
		goto error;
	}
#endif

	/* find the first non-signal layer */
	last_copper = -1;
	for(n = subtree; n != NULL; n = n->next) {
		int lnum, is_copper;
		if ((n->str == NULL) || (n->children->str == NULL) || (n->children->next == NULL) || (n->children->next->str == NULL)) {
			kicad_error(n, "unexpected board layer definition encountered\n");
			goto error;
		}
		lnum = atoi(n->str);
		is_copper = (strcmp(n->children->next->str, "signal") == 0) || (strcmp(n->children->next->str, "power") == 0);
		if ((lnum == 0) && (!is_copper)) {
			kicad_error(n, "unexpected board layer definition: layer 0 must be signal\n");
			goto error;
		}
		if (is_copper && (lnum > last_copper))
			last_copper = lnum;
	}

	if (last_copper < 2) {
		kicad_error(subtree, "broken layer stack: need at least 2 signal layers (copper layers)\n");
		goto error;
	}
	if ((last_copper != 15) && (last_copper != 31))
		kicad_error(subtree, "unusual KiCad layer stack: there should be 16 or 32 copper layers, you seem to have %d instead\n", last_copper+1);

	for(n = subtree; n != NULL; n = n->next) {
		int lnum;
		char *end;
		const char *lname = n->children->str, *ltype = n->children->next->str;

		lnum = strtol(n->str, &end, 10);
		if (*end != '\0') {
			kicad_error(n, "Invalid numeric in layer number (must be a small integer)\n");
			goto error;
		}
		if (kicad_create_layer(st, lnum, lname, ltype, n, last_copper) < 0) {
			kicad_error(n, "Unrecognized layer: %d, %s, %s\n", lnum, lname, ltype);
			goto error;
		}
	}

	pcb_layergrp_fix_old_outline(PCB);
	pcb_layergrp_inhibit_dec();
	return 0;

	error:;
	pcb_layergrp_inhibit_dec();
	return -1;
}

/* kicad_pcb parse global (board level) net */
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
	const char *netname;

	if ((subtree == NULL) || (subtree->str == NULL))
		return kicad_error(subtree, "missing net number in net descriptors.");

	if ((subtree->next == NULL) || (subtree->next->str == NULL))
		return kicad_error(subtree->next, "missing net label in net descriptors.");

	netname = subtree->next->str;
	if (*netname == '\0')
		return 0; /* do not create the anonymous net (is it the no-connect net in kicad?) */

	if (pcb_net_get(st->pcb, &st->pcb->netlist[PCB_NETLIST_INPUT], netname, 1) == NULL)
		return kicad_error(subtree->next, "Failed to create net %s", netname);

	return 0;
}

static pcb_pstk_t *kicad_make_pad_thr(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t padXsize, pcb_coord_t padYsize, pcb_coord_t clearance, double paste_ratio, pcb_coord_t drill, const char *pad_shape)
{
	if (pad_shape == NULL) {
		kicad_error(subtree, "pin with no shape");
		return NULL;
	}

	if (strcmp(pad_shape, "rect") == 0) {
		pcb_pstk_shape_t sh[6];
		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = PCB_LYT_TOP    | PCB_LYT_MASK; sh[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[0], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[1].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[1], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[2].layer_mask = PCB_LYT_TOP    | PCB_LYT_COPPER; pcb_shape_rect(&sh[2], padXsize, padYsize);
		sh[3].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_rect(&sh[3], padXsize, padYsize);
		sh[4].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_rect(&sh[4], padXsize, padYsize);
		sh[5].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, drill, pcb_true, clearance, sh);
	}
	else if ((strcmp(pad_shape, "oval") == 0) || (strcmp(pad_shape, "circle") == 0)) {
		pcb_pstk_shape_t sh[6];
		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = PCB_LYT_TOP    | PCB_LYT_MASK; sh[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[0], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[1].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[1], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[2].layer_mask = PCB_LYT_TOP    | PCB_LYT_COPPER; pcb_shape_oval(&sh[2], padXsize, padYsize);
		sh[3].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_oval(&sh[3], padXsize, padYsize);
		sh[4].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_oval(&sh[4], padXsize, padYsize);
		sh[5].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, drill, pcb_true, clearance, sh);
	}

	kicad_error(subtree, "unsupported pad shape '%s'.", pad_shape);
	return NULL;
}

static pcb_pstk_t *kicad_make_pad_smd(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t padXsize, pcb_coord_t padYsize, pcb_coord_t clearance, double paste_ratio, pcb_coord_t drill, const char *pad_shape, pcb_layer_type_t side)
{
	if ((side & PCB_LYT_TOP) && (side & PCB_LYT_BOTTOM)) {
		kicad_error(subtree, "can't place the same smd pad on both sides");
		return NULL;
	}

	if (!(side & PCB_LYT_TOP) && !(side & PCB_LYT_BOTTOM)) {
		kicad_error(subtree, "can't place smd pad on no side");
		return NULL;
	}

	paste_ratio = 1.0 + (2.0 * paste_ratio);

	if (strcmp(pad_shape, "rect") == 0) {
		pcb_pstk_shape_t sh[4];
		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = side | PCB_LYT_MASK;   sh[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect(&sh[0], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[1].layer_mask = side | PCB_LYT_PASTE;  sh[1].comb = PCB_LYC_AUTO; pcb_shape_rect(&sh[1], padXsize * paste_ratio, padYsize * paste_ratio);
		sh[2].layer_mask = side | PCB_LYT_COPPER; pcb_shape_rect(&sh[2], padXsize, padYsize);
		sh[3].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, 0, pcb_false, clearance, sh);
	}
	else if ((strcmp(pad_shape, "oval") == 0) || (strcmp(pad_shape, "circle") == 0)) {
		pcb_pstk_shape_t sh[4];
		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = side | PCB_LYT_MASK; sh[0].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[0], padXsize+st->pad_to_mask_clearance*2, padYsize+st->pad_to_mask_clearance*2);
		sh[1].layer_mask = side | PCB_LYT_PASTE; pcb_shape_oval(&sh[1], padXsize * paste_ratio, padYsize * paste_ratio);
		sh[2].layer_mask = side | PCB_LYT_COPPER; pcb_shape_oval(&sh[2], padXsize, padYsize);
		sh[3].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, 0, pcb_false, clearance, sh);
	}

	kicad_error(subtree, "unsupported pad shape '%s'.", pad_shape);
	return NULL;
}

static int kicad_make_pad(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, const char *netname, int throughHole, pcb_coord_t moduleX, pcb_coord_t moduleY, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t padXsize, pcb_coord_t padYsize, unsigned int padRotation, unsigned int moduleRotation, pcb_coord_t clearance, double paste_ratio, pcb_coord_t drill, const char *pin_name, const char *pad_shape, unsigned long *featureTally, int *moduleEmpty, pcb_layer_type_t smd_side)
{
	pcb_pstk_t *ps;
	unsigned long required;

	if (subc == NULL)
		return kicad_error(subtree, "error - unable to create incomplete module definition.");

	X += moduleX;
	Y += moduleY;

	if (throughHole) {
		required = BV(0) | BV(1) | BV(3) | BV(5);
		if ((*featureTally & required) != required)
			return kicad_error(subtree, "malformed module pad/pin definition.");
		ps = kicad_make_pad_thr(st, subtree, subc, X, Y, padXsize, padYsize, clearance, paste_ratio, drill, pad_shape);
	}
	else {
		required = BV(0) | BV(1) | BV(2) | BV(5);
		if ((*featureTally & required) != required)
			return kicad_error(subtree, "error parsing incomplete module definition.");
		ps = kicad_make_pad_smd(st, subtree, subc, X, Y, padXsize, padYsize, clearance, paste_ratio, drill, pad_shape, smd_side);
	}

	if (ps == NULL)
		return kicad_error(subtree, "failed to created padstack");
	else
		*moduleEmpty = 0;

	if (pin_name != NULL)
		pcb_attribute_put(&ps->Attributes, "term", pin_name);

	if (netname != NULL) {
		pcb_net_term_t *term;
		pcb_net_t *net;

		if (subc->refdes == NULL)
			return kicad_error(subtree, "Can not connect pad to net '%s': no parent module refdes", netname);

		if (pin_name == NULL)
			return kicad_error(subtree, "Can not connect pad to net '%s': no pad name", netname);

		net = pcb_net_get(st->pcb, &st->pcb->netlist[PCB_NETLIST_INPUT], netname, 0);
		if (net == NULL)
			return kicad_error(subtree, "Can not connect pad %s-%s to net '%s': no such net", subc->refdes, pin_name, netname);

		term = pcb_net_term_get(net, subc->refdes, pin_name, 1);
		if (term == NULL)
			return kicad_error(subtree, "Failed to connect pad to net '%s'", netname);
	}

	return 0;
}

static int kicad_parse_fp_text(read_state_t *st, gsxl_node_t *n, pcb_subc_t *subc, unsigned long *tally, int *foundRefdes, double mod_rot)
{
	char *text;

	if (n->children != NULL && n->children->str != NULL) {
		char *key = n->children->str;
		if (n->children->next != NULL && n->children->next->str != NULL) {
			text = n->children->next->str;
			if (strcmp("reference", key) == 0) {
				SEEN_NO_DUP(*tally, 7);
				pcb_obj_id_fix(text);
				pcb_attribute_put(&subc->Attributes, "refdes", text);
				*foundRefdes = 1;
			}
			else if (strcmp("value", key) == 0) {
				SEEN_NO_DUP(*tally, 8);
				pcb_attribute_put(&subc->Attributes, "value", text);
			}
			else if (strcmp("descr", key) == 0) {
				SEEN_NO_DUP(*tally, 12);
				pcb_attribute_put(&subc->Attributes, "footprint", text);
			}
			else if (strcmp("hide", key) == 0) {
				TODO("figure what 'hide' is doing CUCP#42");
			}
		}
		else /* only key, no value */
			text = key;
	}
	if (kicad_parse_any_text(st, n->children->next->next, text, subc, mod_rot) != 0)
		return -1;
	return 0;
}

static int kicad_parse_pad(read_state_t *st, gsxl_node_t *n, pcb_subc_t *subc, unsigned long *tally, pcb_coord_t moduleX, pcb_coord_t moduleY, unsigned int moduleRotation, int *moduleEmpty)
{
	gsxl_node_t *l, *m;
	pcb_coord_t x, y, drill, sx, sy, clearance;
	const char *netname = NULL;
	char *pin_name = NULL, *pad_shape = NULL;
	unsigned long feature_tally = 0;
	int through_hole = 0;
	int layer_cnt = 0;
	unsigned int pad_rot = 0;
	pcb_layer_type_t smd_side;
	pcb_layer_id_t lid;
	double paste_ratio = 0;

TODO("this should be coming from the s-expr file preferences part CUCP#39")
	clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	if (n->children != 0 && n->children->str != NULL) {
		pin_name = n->children->str;
		pcb_obj_id_fix(pin_name);
		SEEN_NO_DUP(feature_tally, 0);
		if ((n->children->next == NULL) || (n->children->next->str == NULL))
			return kicad_error(n->children->next, "unexpected empty/NULL module pad type node");

		through_hole = (strcmp("thru_hole", n->children->next->str) == 0);
		if (n->children->next->next != NULL && n->children->next->next->str != NULL)
			pad_shape = n->children->next->next->str;
		else
			return kicad_error(n->children->next, "unexpected empty/NULL module pad shape node");
	}
	else
		return kicad_error(n->children, "unexpected empty/NULL module pad name  node");

	if (n->children->next->next->next == NULL || n->children->next->next->next->str == NULL)
		return kicad_error(n->children->next->next, "unexpected empty/NULL module node");

	for(m = n->children->next->next->next; m != NULL; m = m->next) {
		if (m->str == NULL)
			return kicad_error(m, "empty parameter in pad description");
		if (strcmp("at", m->str) == 0) {
			double rot;
			SEEN_NO_DUP(feature_tally, 1);
			PARSE_COORD(x, m, m->children, "module pad X");
			PARSE_COORD(y, m, m->children->next, "module pad Y");
			PARSE_DOUBLE(rot, NULL, m->children->next->next, "module pad rotation");
			pad_rot = (int)rot;
		}
		else if (strcmp("layers", m->str) == 0) {
			TODO("rather pass this subtree directly to the shape generator code so it does not need to guess the layers")
			if (!through_hole) { /* skip testing for pins */
				SEEN_NO_DUP(feature_tally, 2);
				smd_side = 0;
				for(l = m->children; l != NULL; l = l->next) {
					if (l->str != NULL) {
						if ((l->str[0] == 'F') || (l->str[0] == '*'))
							smd_side |= PCB_LYT_TOP;
						if ((l->str[0] == 'B') || (l->str[0] == '*'))
							smd_side |= PCB_LYT_BOTTOM;
						
						lid = kicad_get_layeridx(st, l->str);
						if (lid < 0) {
							/* we ignore *.mask, *.paste, etc., if valid layer def already found */
							if (!layer_cnt)
								return kicad_error(l, "Unknown pad layer %s\n", l->str);
						}
						else if (lid < -1)
							return kicad_error(l, "Unimplemented pad layer %s\n", l->str);
						else
							layer_cnt++;
					}
					else
						return kicad_error(l, "unexpected empty/NULL module layer node");
				}
			}
			else {
				TODO("Ignoring layer definitions for through hole pin - should set which layers have shape CUCP#43");
			}
		}
		else if (strcmp("drill", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 3);
			PARSE_COORD(drill, m, m->children, "module pad drill");
		}
		else if (strcmp("net", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 4);
			if ((m->children == NULL) || (m->children->str == NULL))
				return kicad_error(m, "unexpected empty/NULL module pad net node");
			if ((m->children->next == NULL) || (m->children->next->str == NULL))
				return kicad_error(m->children, "unexpected empty/NULL module pad net name node");
			netname = m->children->next->str;
		}
		else if (strcmp("size", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 5);
			PARSE_COORD(sx, m, m->children, "module pad size X");
			PARSE_COORD(sy, m, m->children->next, "module pad size Y");
		}
		else if (strcmp("solder_paste_margin_ratio", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 6);
			PARSE_DOUBLE(paste_ratio, m, m->children, "module pad solder mask ratio");
		}
		else
			return kicad_error(m, "Unknown pad argument: %s", m->str);
	}

	if (subc != NULL)
		if (kicad_make_pad(st, n, subc, netname, through_hole, moduleX, moduleY, x, y, sx, sy, pad_rot, moduleRotation, clearance, paste_ratio, drill, pin_name, pad_shape, &feature_tally, moduleEmpty, smd_side) != 0)
			return -1;

	return 0;
}

static int kicad_parse_module(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n, *p;
	pcb_layer_id_t lid = 0;
	int on_bottom = 0, found_refdes = 0, module_empty = 1, module_defined = 0, i;
	double mod_rot = 0;
	unsigned long tally = 0;
	pcb_coord_t mod_x = 0, mod_y = 0;
	char *mod_name;
	pcb_subc_t *subc = NULL;

	if (st->pcb == NULL) {
		/* loading a module as a footprint - always create the subc in advance */
		subc = pcb_subc_new();
		pcb_subc_create_aux(subc, 0, 0, 0.0, 0);
		pcb_attribute_put(&subc->Attributes, "refdes", "K1");
	}

	if (subtree->str == NULL)
		return kicad_error(subtree, "module parsing failure: empty");

	mod_name = subtree->str;
	p = subtree->next;
	if ((p != NULL) && (p->str != NULL) && (strcmp("locked", p->str) == 0)) {
		p = p->next;
		TODO("The module is locked, which is being ignored.\n");
	}

	SEEN_NO_DUP(tally, 0);
	for(n = p, i = 0; n != NULL; n = n->next, i++) {
		if (n->str == NULL)
			return kicad_error(n, "empty module parameter");
		if (strcmp("layer", n->str) == 0) { /* need this to sort out ONSOLDER flags etc... */
			SEEN_NO_DUP(tally, 1);
			if (n->children != NULL && n->children->str != NULL) {
				lid = kicad_get_layeridx(st, n->children->str);
				if (lid < 0)
					return kicad_error(n->children, "module layer error - unhandled layer %s", n->children->str);
				else if (pcb_layer_flags(PCB, lid) & PCB_LYT_BOTTOM)
					on_bottom = 1;
			}
			else
				return kicad_error(n, "unexpected empty/NULL module layer node");
		}
		else if (strcmp("tedit", n->str) == 0) {
			SEEN_NO_DUP(tally, 2);
			if (n->children != NULL && n->children->str != NULL) {
				/* ingore time stamp */
			}
			else
				return kicad_error(n, "unexpected empty/NULL module tedit node");
		}
		else if (strcmp("tstamp", n->str) == 0) {
			ignore_value_nodup(n, tally, 3, "unexpected empty/NULL module tstamp node");
		}
		else if (strcmp("attr", n->str) == 0) {
			char *key;

			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected empty/NULL module attr node");

			key = pcb_concat("kicad_attr_", n->children->str, NULL);
			pcb_attribute_put(&subc->Attributes, key, "1");
			free(key);
		}
		else if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 4);
			PARSE_COORD(mod_x, n, n->children, "module X");
			PARSE_COORD(mod_y, n, n->children->next, "module Y");
			PARSE_DOUBLE(mod_rot, NULL, n->children->next->next, "module rotation");

			/* if we have been provided with a Module Name and location, create a new subc with default "" and "" for refdes and value fields */
			if (mod_name != NULL && module_defined == 0) {
				module_defined = 1; /* but might be empty, wait and see */
				if (subc == NULL) {
					subc = pcb_subc_new();
					/* modules are specified in rot=0; build time like that and rotate
					   the whole subc at the end. Text is special case because kicad
					   has no 'floater' - rotation is encoded both in text and subc
					   and it has to be subtracted from text later on */
					pcb_subc_create_aux(subc, mod_x, mod_y, 0.0, on_bottom);
					pcb_attribute_put(&subc->Attributes, "refdes", "K1");
				}
				if (st->pcb != NULL) {
					pcb_subc_reg(st->pcb->Data, subc);
					pcb_subc_bind_globals(st->pcb, subc);
				}
			}
		}
		else if (strcmp("model", n->str) == 0) {
			TODO("save this as attribute");
		}
		else if (strcmp("fp_text", n->str) == 0) {
			kicad_parse_fp_text(st, n, subc, &tally, &found_refdes, mod_rot);
		}
		else if (strcmp("descr", n->str) == 0) {
			SEEN_NO_DUP(tally, 9);
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected empty/NULL module descr node");
			pcb_attribute_put(&subc->Attributes, "kicad_descr", n->children->str);
		}
		else if (strcmp("tags", n->str) == 0) {
			SEEN_NO_DUP(tally, 10);
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected empty/NULL module tags node");
			pcb_attribute_put(&subc->Attributes, "kicad_tags", n->children->str);
		}
		else if (strcmp("path", n->str) == 0) {
			ignore_value_nodup(n, tally, 11, "unexpected empty/NULL module model node");
		}
		else if (strcmp("pad", n->str) == 0) { /* pads next  - have thru_hole, circle, rect, roundrect, to think about */
			if (kicad_parse_pad(st, n, subc, &tally, mod_x, mod_y, mod_rot, &module_empty) != 0)
				return -1;
		}
		else if (strcmp("fp_line", n->str) == 0) {
			if (kicad_parse_any_line(st, n->children, subc, 0, 0) != 0)
				return -1;
		}
		else if ((strcmp("fp_arc", n->str) == 0) || (strcmp("fp_circle", n->str) == 0)) {
			if (kicad_parse_any_arc(st, n->children, subc) != 0)
				return -1;
		}
		else
			return kicad_error(n, "Unknown module argument: %s\n", n->str);
	}

	if (subc == NULL)
		return kicad_error(subtree, "unable to create incomplete subc.");

	pcb_subc_bbox(subc);
	if (st->pcb != NULL) {
		if (st->pcb->Data->subc_tree == NULL)
			st->pcb->Data->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(st->pcb->Data->subc_tree, (pcb_box_t *)subc);
		pcb_subc_rebind(st->pcb, subc);
	}
	else
		pcb_subc_reg(st->fp_data, subc);

	if ((mod_rot == 90) || (mod_rot == 180) || (mod_rot == 270)) {
		/* lossles module rotation for round steps */
		mod_rot = pcb_round(mod_rot / 90);
		pcb_subc_rotate90(subc, mod_x, mod_y, mod_rot);
	}
	else if (mod_rot != 0)
		pcb_subc_rotate(subc, mod_x, mod_y, cos(mod_rot/PCB_RAD_TO_DEG), sin(mod_rot/PCB_RAD_TO_DEG), mod_rot);

	return 0;
}

static int kicad_parse_zone(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n, *m;
	int i;
	long j = 0, polycount = 0;
	unsigned long tally = 0, required;
	pcb_poly_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_coord_t x, y;
	pcb_layer_t *ly = NULL;

	for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
		if (n->str == NULL)
			return kicad_error(n, "empty zone parameter");
		if (strcmp("net", n->str) == 0) {
			ignore_value_nodup(n, tally, 0, "unexpected zone net null node.");
		}
		else if (strcmp("net_name", n->str) == 0) {
			ignore_value_nodup(n, tally, 1, "unexpected zone net_name null node.");
		}
		else if (strcmp("tstamp", n->str) == 0) {
			ignore_value_nodup(n, tally, 2, "unexpected zone tstamp null node.");
		}
		else if (strcmp("hatch", n->str) == 0) {
			SEEN_NO_DUP(tally, 3);
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected zone hatch null node.");
			SEEN_NO_DUP(tally, 4); /* same as ^= 1 was */
			if ((n->children->next == NULL) || (n->children->next->str == NULL))
				return kicad_error(n, "unexpected zone hatching null node.");
			SEEN_NO_DUP(tally, 5);
		}
		else if (strcmp("connect_pads", n->str) == 0) {
			SEEN_NO_DUP(tally, 6);
			if (n->children != NULL && n->children->str != NULL && (strcmp("clearance", n->children->str) == 0) && (n->children->children->str != NULL)) {
				SEEN_NO_DUP(tally, 7); /* same as ^= 1 was */
			}
			else if (n->children != NULL && n->children->str != NULL && n->children->next->str != NULL) {
				SEEN_NO_DUP(tally, 8); /* same as ^= 1 was */
				if (n->children->next != NULL && n->children->next->str != NULL && n->children->next->children != NULL && n->children->next->children->str != NULL) {
					if (strcmp("clearance", n->children->next->str) == 0) {
						SEEN_NO_DUP(tally, 9);
					}
					else
						kicad_warning(n->children->next, "Unrecognised zone connect_pads option %s\n", n->children->next->str);
				}
			}
		}
		else if (strcmp("layer", n->str) == 0) {

			SEEN_NO_DUP(tally, 10);
			PARSE_LAYER(ly, n->children, NULL, "zone polygon");
			polygon = pcb_poly_new(ly, 0, flags);
		}
		else if (strcmp("polygon", n->str) == 0) {
			polycount++; /*keep track of number of polygons in zone */
			if (n->children != NULL && n->children->str != NULL) {
				if (strcmp("pts", n->children->str) == 0) {
					if (polygon != NULL) {
						for(m = n->children->children, j = 0; m != NULL; m = m->next, j++) {
							if (m->str != NULL && strcmp("xy", m->str) == 0) {
								PARSE_COORD(x, m, m->children, "zone polygon vertex X");
								PARSE_COORD(y, m, m->children->next, "zone polygon vertex Y");
								pcb_poly_point_new(polygon, x, y);
							}
							else
								return kicad_error(m, "empty pts element");
						}
					}
				}
				else
					return kicad_error(n->children, "pts section vertices not found in zone polygon.");
			}
			else
				return kicad_error(n, "error parsing empty polygon.");
		}
		else if (strcmp("fill", n->str) == 0) {
			SEEN_NO_DUP(tally, 11);
			for(m = n->children; m != NULL; m = m->next) {
				if (m->str == NULL)
					return kicad_error(m, "empty fill parameter");
				if (strcmp("arc_segments", m->str) == 0) {
					ignore_value(m, "unexpected zone arc_segment null node.");
				}
				else if (strcmp("thermal_gap", m->str) == 0) {
					ignore_value(m, "unexpected zone thermal_gap null node.");
				}
				else if (strcmp("thermal_bridge_width", m->str) == 0) {
					ignore_value(m, "unexpected zone thermal_bridge_width null node.");
				}
				else if (strcmp("yes", m->str) == 0) {
					/* ignored */
				}
				else
					kicad_warning(m, "Unknown zone fill argument:\t%s\n", m->str);
			}
		}
		else if (strcmp("min_thickness", n->str) == 0) {
			ignore_value_nodup(n, tally, 12, "unexpected zone min_thickness null node");
		}
		else if (strcmp("priority", n->str) == 0) {
			ignore_value_nodup(n, tally, 13, "unexpected zone min_thickness null node.");
		}
		else if (strcmp("filled_polygon", n->str) == 0) {
			TODO("isn't this the same as polygon? CUCP#44");
		}
		else
			kicad_warning(n, "Unknown polygon argument:\t%s\n", n->str);
	}

	required = BV(10); /* layer at a minimum */
	if ((tally & required) != required)
		return kicad_error(subtree, "can not create zone because required fields are missing");

	if (polygon != NULL) {
		pcb_add_poly_on_layer(ly, polygon);
		pcb_poly_init_clip(st->pcb->Data, ly, polygon);
	}
	return 0;
}


/* Parse a board from &st->dom into st->pcb */
static int kicad_parse_pcb(read_state_t *st)
{
	static const dispatch_t disp[] = { /* possible children of root */
		{"version", kicad_parse_version},
		{"host", kicad_parse_nop},
		{"general", kicad_parse_general},
		{"page", kicad_parse_page_size},
		{"title_block", kicad_parse_title_block},
		{"layers", kicad_parse_layer_definitions}, /* board layer defs */
		{"setup", kicad_parse_setup},
		{"net", kicad_parse_net}, /* net labels if child of root, otherwise net attribute of element */
		{"net_class", kicad_parse_nop},
		{"module", kicad_parse_module}, /* for footprints */
		{"dimension", kicad_parse_nop}, /* for dimensioning features */
		{"gr_line", kicad_parse_gr_line},
		{"gr_arc", kicad_parse_gr_arc},
		{"gr_circle", kicad_parse_gr_arc},
		{"gr_text", kicad_parse_gr_text},
		{"via", kicad_parse_via},
		{"segment", kicad_parse_segment},
		{"zone", kicad_parse_zone}, /* polygonal zones */
		{NULL, NULL}
	};

	/* require the root node to be kicad_pcb */
	if ((st->dom.root->str == NULL) || (strcmp(st->dom.root->str, "kicad_pcb") != 0))
		return -1;

	/* Call the corresponding subtree parser for each child node of the root
	   node; if any of them fail, parse fails */
	return kicad_foreach_dispatch(st, st->dom.root->children, disp);
}

static gsx_parse_res_t kicad_parse_file(FILE *FP, gsxl_dom_t *dom)
{
	int c;
	gsx_parse_res_t res;

	gsxl_init(dom, gsxl_node_t);
	dom->parse.line_comment_char = '#';
	do {
		c = fgetc(FP);
	} while((res = gsxl_parse_char(dom, c)) == GSX_RES_NEXT);

	if (res == GSX_RES_EOE) {
		/* compact and simplify the tree */
		gsxl_compact_tree(dom);
	}

	return res;
}

int io_kicad_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int readres = 0;
	read_state_t st;
	gsx_parse_res_t res;
	FILE *FP;

	FP = pcb_fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));

	st.pcb = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_k2i, strhash, strkeyeq);

	/* A0 */
	st.width[DIM_FALLBACK] = PCB_MM_TO_COORD(1189.0);
	st.height[DIM_FALLBACK] = PCB_MM_TO_COORD(841.0);
	st.dim_valid[DIM_FALLBACK] = 1;

	/* load the file into the dom */
	res = kicad_parse_file(FP, &st.dom);
	fclose(FP);

	if (res == GSX_RES_EOE) {
		/* recursively parse the dom */
		readres = kicad_parse_pcb(&st);
	}
	else
		readres = -1;

	/* clean up */
	gsxl_uninit(&st.dom);

	pcb_layer_auto_fixup(Ptr);

	if (pcb_board_normalize(Ptr) > 0)
		pcb_message(PCB_MSG_WARNING, "Had to make changes to the coords so that the design fits the board.\n");
	pcb_layer_colors_from_conf(Ptr, 1);

	{ /* free layer hack */
		htsi_entry_t *e;
		for(e = htsi_first(&st.layer_k2i); e != NULL; e = htsi_next(&st.layer_k2i, e))
			free(e->key);
		htsi_uninit(&st.layer_k2i);
	}


	return readres;
}

int io_kicad_parse_element(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name)
{
	int mres;
	pcb_fp_fopen_ctx_t fpst;
	FILE *f;
	read_state_t st;
	gsx_parse_res_t res;

	f = pcb_fp_fopen(pcb_fp_default_search_path(), name, &fpst, NULL);

	if (f == NULL) {
		return -1;
	}

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.pcb = NULL;
	st.fp_data = Ptr;
	st.Filename = name;
	st.settings_dest = CFR_invalid;
	st.auto_layers = 1;

	res = kicad_parse_file(f, &st.dom);
	pcb_fp_fclose(f, &fpst);

	if (res != GSX_RES_EOE) {
		if (!pcb_io_err_inhibit)
			pcb_message(PCB_MSG_ERROR, "Error parsing s-expression '%s'\n", name);
		gsxl_uninit(&st.dom);
		return -1;
	}

	if ((st.dom.root->str == NULL) || (strcmp(st.dom.root->str, "module") != 0)) {
		pcb_message(PCB_MSG_ERROR, "Wrong root node '%s', expected 'module'\n", st.dom.root->str);
		gsxl_uninit(&st.dom);
		return -1;
	}

	htsi_init(&st.layer_k2i, strhash, strkeyeq);

	mres = kicad_parse_module(&st, st.dom.root->children);
/*	if (mres == 0)
		pcb_data_clip_polys(sc->data);*/

	gsxl_uninit(&st.dom);
	return mres;
}

int io_kicad_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;

	if ((typ != PCB_IOT_PCB) && (typ != PCB_IOT_FOOTPRINT))
		return 0; /* support only boards for now - kicad footprints are in the legacy format */

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s))
				s++; /* strip leading whitespace */
			if ((strncmp(s, "(kicad_pcb", 10) == 0) && (typ == PCB_IOT_PCB)) /* valid root */
				return 1;
			if (strncmp(s, "(module", 7) == 0) /* valid root */
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '#') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line - and we don't have our root -> it's not an s-expression */
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}
