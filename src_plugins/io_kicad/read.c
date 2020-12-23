/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2016, 2017 Erich S. Heinzle
 *  Copyright (C) 2017 Miloh
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
 *
 */
#include "config.h"

#include <stddef.h>
#include <assert.h>
#include <math.h>
#include <gensexpr/gsxl.h>
#include <genht/htsi.h>
#include <genht/htpp.h>
#include <genvector/vtp0.h>

#include <librnd/core/compat_misc.h>
#include "board.h"
#include "plug_io.h"
#include <librnd/core/error.h>
#include "data.h"
#include "read.h"
#include "layer.h"
#include "polygon.h"
#include "plug_footprint.h"
#include <librnd/core/misc_util.h> /* for distance calculations */
#include "conf_core.h"
#include "move.h"
#include "rotate.h"
#include <librnd/core/safe_fs.h>
#include "attrib.h"
#include "netlist.h"
#include <librnd/core/math_helper.h>
#include "obj_pstk_inlines.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"
#include "../src_plugins/lib_compat_help/subc_help.h"
#include "../src_plugins/lib_compat_help/media.h"
#include "../src_plugins/shape/shape.h"

#include "layertab.h"

/* a reasonable approximation of pcb glyph width, ~=  5000 centimils; in mm */
#define GLYPH_WIDTH (1.27)

/* If we are reading a pcb, use st->pcb, else we are reading a footprint and
   use global PCB */
#define PCB_FOR_FP (st->pcb == NULL ? PCB : st->pcb)

/* when a coord value is not specified in the file */
#define UNSPECIFIED RND_MAX_COORD

typedef enum {
	DIM_PAGE,
	DIM_AREA,
	DIM_FALLBACK,
	DIM_max
} kicad_dim_prio_t;

/* delayed zone connect */
typedef struct zone_connect_s zone_connect_t;
struct zone_connect_s {
	pcb_pstk_t *ps;
	const char *netname;
	int style;
	zone_connect_t *next;
};

typedef struct {
	pcb_board_t *pcb;
	pcb_data_t *fp_data;
	const char *Filename;
	rnd_conf_role_t settings_dest;
	gsxl_dom_t dom;
	unsigned auto_layers:1;
	unsigned module_pre_create:1;
	htsi_t layer_k2i; /* layer name-to-index hash; name is the kicad name, index is the pcb-rnd layer index */
	long ver;
	vtp0_t intern_copper; /* temporary storage of internal copper layers */
	double subc_rot; /* for padstacks know the final parent subc rotation in advance to compensate (this depends on the fact that the rotation appears earlier in the file than any pad) */
	pcb_subc_t *last_sc;
	const char *primitive_term; /* for gr_ objects: if not NULL, set the term attribute */
	pcb_layer_t *primitive_ly;  /* for gr_ objects: if not NULL, use this layer and expect no layer specified in the file */
	pcb_subc_t *primitive_subc; /* for gr_ objects: if not NULL, object is being added under this subc (apply offs) */
	rnd_coord_t primitive_dx, primitive_dy; /*  for gr_ objects: if not 0, add these offsets to the placement (useful for pad primitive placement) */

	rnd_coord_t width[DIM_max];
	rnd_coord_t height[DIM_max];
	rnd_coord_t dim_valid[DIM_max];

	/* setup */
	rnd_coord_t pad_to_mask_clearance;
	rnd_coord_t zone_clearance;

	/* delayed actions */
	zone_connect_t *zc_head;
	htpp_t poly2net;
	unsigned poly2net_inited:1;

	/* warnings */
	unsigned warned_poly_side_clr:1; /* whether polygon-side clearance is already warned about */
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
	rnd_append_printf(&str, "io_kicad parse error at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	rnd_message(RND_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

static int kicad_warning(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	rnd_append_printf(&str, "io_kicad warning at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	rnd_message(RND_MSG_WARNING, "%s", str.array);

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
		char *end;
		st->ver = strtol(subtree->str, &end, 10);
		if (*end != '\0')
			return kicad_error(subtree, "unexpected layout version syntax (perhaps too new, please file a feature request!)");
		if ((st->ver == 3) || (st->ver == 4))
			return 0;
		if ((st->ver > 20170000L) && (st->ver < 20180000L)) /* kicad 5 */
			return 0;
	}
	return kicad_error(subtree, "unexpected layout version number (perhaps too new, please file a feature request!)");
}


static int kicad_create_copper_layer_(read_state_t *st, rnd_layergrp_id_t gid, const char *lname, const char *ltype, gsxl_node_t *subtree)
{
	rnd_layer_id_t id;
	id = pcb_layer_create(st->pcb, gid, lname, 0);
	if (id < 0) {
		rnd_message(RND_MSG_ERROR, "failed to create copper layer %s\n", lname);
		return -1;
	}
	htsi_set(&st->layer_k2i, rnd_strdup(lname), id);
	if (ltype != NULL) {
		pcb_layer_t *ly = pcb_get_layer(st->pcb->Data, id);
		pcb_attribute_put(&ly->Attributes, "kicad::type", ltype);
	}
	return 0;
}

static int kicad_create_copper_layer(read_state_t *st, int lnum, const char *lname, const char *ltype, gsxl_node_t *subtree, int last_copper)
{
	rnd_layergrp_id_t gid = -1;
	const char *cu;

	pcb_layer_type_t loc = PCB_LYT_INTERN;

	cu = lname + strlen(lname) - 3;
	if (strcmp(cu, ".Cu") != 0) {
		if (st->ver < 4) {
			if ((strcmp(lname, "Front") != 0) && (strcmp(lname, "Back") != 0))
				kicad_warning(subtree, "layer %d: unusual name for front/back copper (recoverable error)\n", lnum);
		}
		else
			kicad_warning(subtree, "layer %d name should end in .Cu because it is a copper layer (recoverable error)\n", lnum);
	}

	if ((lnum == 0) || (lnum == last_copper)) { /* top or bottom - order depends on file version */
		if (st->ver > 3) {
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

	if (st->ver > 3) {
		if (lnum == 0) loc = PCB_LYT_TOP;
		else if (lnum == last_copper) loc = PCB_LYT_BOTTOM;
	}
	else {
		if (lnum == 0) loc = PCB_LYT_BOTTOM;
		else if (lnum == last_copper) loc = PCB_LYT_TOP;
	}

	if (loc & PCB_LYT_INTERN) {
		/* for intern copper layers we need to have an array and delay
		   creation to order them correctly by layer number even if
		   their are listed in a different order in the file */
		vtp0_set(&st->intern_copper, lnum, subtree);
		return 0;
	}

	pcb_layergrp_list(st->pcb, PCB_LYT_COPPER | loc, &gid, 1);
	return kicad_create_copper_layer_(st, gid, lname, ltype, subtree);
}


static int kicad_create_misc_layer(read_state_t *st, int lnum, const char *lname, const char *ltype, gsxl_node_t *subtree, int last_copper)
{
	rnd_layergrp_id_t gid;
	rnd_layer_id_t lid = -1;
	pcb_layergrp_t *grp, *new_grp = NULL;
	pcb_layer_t *ly;
	const kicad_layertab_t *lt, *best = NULL;
	int adj_lnum = lnum - last_copper;

	/* find the best match */
	for(lt = kicad_layertab; lt->score != 0; lt++) {
		/* optimization: if we already have a match, ignore weaker candidates */
		if ((best != NULL) && (lt->score <= best->score))
			continue;
		
		if ((lt->id != 0) && (lt->id != adj_lnum))
			continue;

		if (lt->prefix != NULL) {
			if (lt->prefix_len == 0) { /* full length name match */
				if (strcmp(lt->prefix, lname) != 0)
					continue;
			}
			else { /* prefix name match */
				if (strncmp(lt->prefix, lname, lt->prefix_len) != 0)
					continue;
			}
		}

		/* match; because of the first check, lt is guaranteed to have higher
		   score than best */
		best = lt;
	}

	if (best == NULL) {
		kicad_warning(subtree, "unknown layer: %d %s %s\n", lnum, lname, ltype);
		remember_bad:; /* HACK/WORKAROUND: remember kicad layers for those that are unsupported */
		htsi_set(&st->layer_k2i, rnd_strdup(lname), -lnum);
		return 0;
	}

	/* create the best layer by executing its actions */
	switch(best->action) {
		case LYACT_EXISTING:
			gid = -1;
			pcb_layergrp_list(st->pcb, best->type, &gid, 1);
			lid = pcb_layer_create(st->pcb, gid, lname, 0);
			new_grp = NULL;
			break;

		case LYACT_NEW_MISC:
			grp = pcb_get_grp_new_misc(st->pcb);
			grp->name = rnd_strdup(lname);
			grp->ltype = best->type;
			lid = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, lname, 0);
			new_grp = grp;
			break;

		case LYACT_NEW_OUTLINE:
			grp = pcb_get_grp_new_intern(PCB, -1);
			pcb_layergrp_fix_turn_to_outline(grp);
			lid = pcb_layer_create(st->pcb, grp - st->pcb->LayerGroups.grp, lname, 0);
			new_grp = grp;
			break;
	}

	if (lid == -1) {
		kicad_warning(subtree, "internal error creating layer: %d %s %s\n", lnum, lname, ltype);
		goto remember_bad;
	}

	ly = pcb_get_layer(st->pcb->Data, lid);
	ly->comb |= best->lyc;

	if ((new_grp != NULL) && (best->purpose != NULL))
		new_grp->purpose = rnd_strdup(best->purpose);

	htsi_set(&st->layer_k2i, rnd_strdup(lname), lid);
	return 0;
}


/* Parse a layer definition and do all the administration needed for the layer */
static int kicad_create_layer(read_state_t *st, int lnum, const char *lname, const char *ltype, gsxl_node_t *subtree, int last_copper)
{
	if (lnum <= last_copper)
		return kicad_create_copper_layer(st, lnum, lname, ltype, subtree, last_copper);
	return kicad_create_misc_layer(st, lnum, lname, ltype, subtree, last_copper);
}

/* Register a kicad layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int kicad_reg_layer(read_state_t *st, const char *kicad_name, unsigned int mask, const char *purpose)
{
	rnd_layer_id_t id;
	if (st->pcb != NULL) {
		if (pcb_layer_listp(st->pcb, mask, &id, 1, -1, purpose) != 1) {
			rnd_layergrp_id_t gid;
			pcb_layergrp_listp(PCB, mask, &gid, 1, -1, purpose);
			id = pcb_layer_create(st->pcb, gid, kicad_name, 0);
		}
	}
	else {
		/* registering a new layer in buffer */
		pcb_layer_t *ly = pcb_layer_new_bound(st->fp_data, mask, kicad_name, purpose);
		id = ly - st->fp_data->Layer;
		if (mask & PCB_LYT_MASK)
			ly->comb |= PCB_LYC_SUB;
	}
	htsi_set(&st->layer_k2i, rnd_strdup(kicad_name), id);
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
			if ((rnd_strcasecmp(end, ".Cu") == 0) && (id >= 1) && (id <= 30)) {
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

	TODO("for In, also remember the offset");

	TODO("this should use the layertab instead");
	if (rnd_strcasecmp(kicad_name, "Edge.Cuts") == 0) { lyt |= PCB_LYT_BOUNDARY; purpose = "uroute"; }
	else if (kicad_name[1] == '.') {
		const char *ln = kicad_name + 2;
		if (rnd_strcasecmp(ln, "SilkS") == 0) lyt |= PCB_LYT_SILK;
		else if (rnd_strcasecmp(ln, "Mask") == 0) lyt |= PCB_LYT_MASK;
		else if (rnd_strcasecmp(ln, "Paste") == 0) lyt |= PCB_LYT_PASTE;
		else if (rnd_strcasecmp(ln, "Cu") == 0) lyt |= PCB_LYT_COPPER;
		else lyt |= PCB_LYT_VIRTUAL;
	}
	else lyt |= PCB_LYT_VIRTUAL;

	if (kicad_reg_layer(st, kicad_name, lyt, purpose) == 0)
		return kicad_get_layeridx(st, kicad_name);
	return -1;
}

static void kicad_create_fp_layers(read_state_t *st, gsxl_node_t *subtree)
{
	const kicad_layertab_t *l;
	pcb_layergrp_t *g;
	int last_copper = 15;

	pcb_layergrp_inhibit_inc();
	pcb_layer_group_setup_default(st->pcb);

	/* one intern copper */
	g = pcb_get_grp_new_intern(st->pcb, -1);
	pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Inner1.Cu", 0);

	kicad_create_layer(st, 0,           "F.Cu",      "signal", subtree, last_copper);
	kicad_create_layer(st, 1,           "Inner1.Cu", "signal", subtree, last_copper);
	kicad_create_layer(st, last_copper, "B.Cu",      "signal", subtree, last_copper);

	for(l = kicad_layertab; l->score != 0; l++) {
		if (l->auto_create) {
			kicad_create_layer(st, last_copper+l->id, l->prefix, NULL, subtree, last_copper);
		}
	}
	pcb_layergrp_inhibit_dec();
}

static pcb_layer_t *kicad_get_subc_layer(read_state_t *st, pcb_subc_t *subc, const char *layer_name, const char *default_layer_name)
{
	int pcb_idx = -1;
	rnd_layer_id_t lid;
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
			rnd_message(RND_MSG_ERROR, "\tfp_* layer '%s' not found for module object, using unbound subc layer instead.\n", layer_name);
			lyt = PCB_LYT_VIRTUAL;
			comb = 0;
			return pcb_subc_get_layer(subc, lyt, comb, 1, lnm, rnd_true);
		}
	}
	else {
		/* check if the layer already exists (by name) */
		lid = pcb_layer_by_name(subc->data, default_layer_name);
		if (lid >= 0)
			return &subc->data->Layer[lid];

		rnd_message(RND_MSG_ERROR, "\tfp_* layer '%s' not found for module object, using module layer '%s' instead.\n", layer_name, default_layer_name);
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
	if (lyt & PCB_LYT_MASK)
		comb |= PCB_LYC_SUB;
	return pcb_subc_get_layer(subc, lyt, comb, 1, lnm, rnd_true);
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
			st->pcb->hidlib.size_x = st->width[n];
			st->pcb->hidlib.size_y = st->height[n];
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

	name = rnd_concat(prefix, subtree->str, NULL);
	pcb_attrib_put(st->pcb, name, subtree->children->str);
	free(name);
	for(n = subtree->next; n != NULL; n = n->next) {
		if (n->str != NULL && strcmp("comment", n->str) != 0) {
			name = rnd_concat(prefix, n->str, NULL);
			pcb_attrib_put(st->pcb, name, n->children->str);
			free(name);
		}
		else { /* if comment field has extra children args */
			name = rnd_concat(prefix, n->str, "_", n->children->str, NULL);
			pcb_attrib_put(st->pcb, name, n->children->next->str);
			free(name);
		}
	}
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

/* Convert the string value of node to int and store it in res. On conversion
   error report error on node using errmsg. If node == NULL: report error
   on missnode, or ignore the problem if missnode is NULL. */
#define PARSE_INT(res, missnode, node, errmsg) \
do { \
	gsxl_node_t *__node__ = (node); \
	if ((__node__ != NULL) && (__node__->str != NULL)) { \
		char *__end__; \
		double __val__ = strtol(__node__->str, &__end__, 10); \
		if (*__end__ != 0) \
			return kicad_error(node, "Invalid numeric (integer) " errmsg); \
		else \
			(res) = __val__; \
	} \
	else if (missnode != NULL) \
		return kicad_error(missnode, "Missing child node for " errmsg); \
} while(0) \

/* same as PARSE_DOUBLE() but res is a rnd_coord_t, input string is in mm */
#define PARSE_COORD(res, missnode, node, errmsg) \
do { \
	double __dtmp__; \
	PARSE_DOUBLE(__dtmp__, missnode, node, errmsg); \
	(res) = rnd_round(RND_MM_TO_COORD(__dtmp__)); \
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
			rnd_layer_id_t lid = kicad_get_layeridx(st, nd->str); \
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
						rnd_coord_t *d = (rnd_coord_t *)dst;
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
		{"zone_clearance", offsetof(read_state_t, zone_clearance), KICAD_COORD},
		{NULL, 0, 0}
	};
	return kicad_foreach_autoload(st, subtree, st, atbl, 1);
}


static char *fp_text_subst(char *text, pcb_flag_t *flg, int is_refdes)
{
	if (is_refdes || (strcmp(text, "%R") == 0)) {
		flg->f |= PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER;
		return "%a.parent.refdes%";
	}
	if (strcmp(text, "%V") == 0) {
		flg->f |= PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER;
		return "%a.parent.value%";
	}
	return text;
}

/* kicad_pcb/gr_text and fp_text */
static int kicad_parse_any_text(read_state_t *st, gsxl_node_t *subtree, char *text, pcb_subc_t *subc, double mod_rot, int mod_mirror, int is_refdes)
{
	gsxl_node_t *l, *n, *m;
	int i, mirrored = 0;
	unsigned long tally = 0, required;
	double sx, sy, rotdeg = 0.0;
	rnd_coord_t X, Y, thickness = 0, bbw, bbh, xanch, yanch;
	int align = 0; /* -1 for left, 0 for center and +1 for right */
	pcb_flag_t flg = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_t *ly;

	/* fix up text for kicad's %R and %V */
	if (subc != NULL)
		text = fp_text_subst(text, &flg, is_refdes);

	for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
		if (n->str == NULL)
			return kicad_error(n, "empty text node");
		if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			PARSE_COORD(X, n, n->children, "text X1");
			PARSE_COORD(Y, n, n->children->next, "text Y1");
			PARSE_DOUBLE(rotdeg, NULL, n->children->next->next, "text rotation");
			if (subc != NULL) {
				rnd_coord_t sx, sy;
				if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
					X += sx;
					Y += sy;
				}
			}
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_LAYER(ly, n->children, subc, "text");
			if (subc == NULL) {
				if (pcb_layer_flags_(ly) & PCB_LYT_BOTTOM)
					flg.f |= PCB_FLAG_ONSOLDER;
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
							SEEN_NO_DUP(tally, 2);
							/* accrding to gerber exports from v5, it seems the sizes are mixed up! */
							PARSE_DOUBLE(sx, l, l->children->next, "text size X");
							PARSE_DOUBLE(sy, l, l->children, "text size Y");
						}
						else if (strcmp("thickness", l->str) == 0) {
							SEEN_NO_DUP(tally, 3);
							PARSE_COORD(thickness, l, l->children, "text thickness");
						}
					}
				}
				else if (strcmp("justify", m->str) == 0) {
					gsxl_node_t *j;

					for(j = m->children; (j != NULL) && (j->str != NULL); j = j->next) {
						if (strcmp("mirror", j->str) == 0) {
							mirrored = 1;
							SEEN_NO_DUP(tally, 4);
						}
						else if (strcmp("left", j->str) == 0) {
							align = -1;
							SEEN_NO_DUP(tally, 5);
						}
						else if (strcmp("center", j->str) == 0) {
							align = 0;
							SEEN_NO_DUP(tally, 5);
						}
						else if (strcmp("right", j->str) == 0) {
							align = 1;
							SEEN_NO_DUP(tally, 5);
						}
						else
							return kicad_error(j, "unknown text justification: %s\n", j->str);
					}
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

	/* calculate input bounding box */
	bbw = RND_MM_TO_COORD(2.0 * (sx+RND_COORD_TO_MM(thickness)) / 3.0 * strlen(text));
	bbh = RND_MM_TO_COORD(1.25 * (sy+RND_COORD_TO_MM(thickness)));

	switch(align) {
		case -1: xanch = 0; break;
		case 0:  xanch = bbw/2; break;
		case +1: xanch = bbw; break;
	}
	yanch = RND_MM_TO_COORD(0.66*sy+RND_COORD_TO_MM(thickness));

	rotdeg -= mod_rot;
	if (mirrored)
		rotdeg = -rotdeg;

	if (mod_mirror) {
		rotdeg += 180;
		if (rotdeg > 360)
			rotdeg -= 360;
	}

	X += st->primitive_dx; Y += st->primitive_dy;

	pcb_text_new_by_bbox(ly, pcb_font(PCB_FOR_FP, 0, 1), X, Y, bbw, bbh,
		xanch, yanch, sx/sy, mirrored ? PCB_TXT_MIRROR_X : 0, rotdeg, thickness,
		text, flg);

	return 0; /* create new font */
}

/* kicad_pcb/gr_text */
static int kicad_parse_gr_text(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL)
		return kicad_parse_any_text(st, subtree, subtree->str, NULL, 0.0, 0, 0);
	return kicad_error(subtree, "failed to create text: missing text string");
}

/* kicad_pcb/target */
static int kicad_parse_target(read_state_t *st, gsxl_node_t *subtree)
{
	unsigned long tally = 0, required;
	rnd_coord_t x, y, thick = RND_MM_TO_COORD(0.15);
	double size = 5.0;
	pcb_layer_t *ly = NULL;
	pcb_subc_t *subc;
	gsxl_node_t *n, *lyn;
	pcb_flag_t flg = pcb_flag_make(0);
	int is_plus = 1;


	for(n = subtree; n != NULL; n = n->next) {
		if (strcmp("plus", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
		}
		else if (strcmp("x", n->str) == 0) {
			SEEN_NO_DUP(tally, 0);
			is_plus = 0;
		}
		else if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			PARSE_COORD(x, n, n->children, "target X");
			PARSE_COORD(y, n, n->children->next, "target Y");
		}
		else if (strcmp("size", n->str) == 0) {
			SEEN_NO_DUP(tally, 2);
			PARSE_DOUBLE(size, n, n->children, "target size");
		}
		else if (strcmp("width", n->str) == 0) {
			SEEN_NO_DUP(tally, 3);
			PARSE_COORD(thick, n, n->children, "target width");
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 4);
			lyn = n->children;
		}
	}

	required = BV(1) | BV(4);
	if ((tally & required) != required)
		return kicad_error(subtree, "failed to create target: missing fields");

	if (!(required & BV(0)))
		kicad_warning(subtree, "missing \"plus\" or \"x\" in target - assuming plus target graphics may not look like expected");

	/* create a subc at the given coords */
	subc = pcb_subc_new();
	pcb_subc_create_aux(subc, x, y, 0.0, 0);
	if (st->pcb != NULL) {
		pcb_subc_reg(st->pcb->Data, subc);
		pcb_subc_bind_globals(st->pcb, subc);
	}

	PARSE_LAYER(ly, lyn, subc, "target");

	/* draw the target within the subc */
	if (is_plus) {
		pcb_line_new(ly, x-RND_MM_TO_COORD(size/2), y, x+RND_MM_TO_COORD(size/2), y, thick, 0, flg);
		pcb_line_new(ly, x, y-RND_MM_TO_COORD(size/2), x, y+RND_MM_TO_COORD(size/2), thick, 0, flg);
	}
	else {
		pcb_line_new(ly, x-RND_MM_TO_COORD(size/2), y-RND_MM_TO_COORD(size/2), x+RND_MM_TO_COORD(size/2), y+RND_MM_TO_COORD(size/2), thick, 0, flg);
		pcb_line_new(ly, x+RND_MM_TO_COORD(size/2), y-RND_MM_TO_COORD(size/2), x-RND_MM_TO_COORD(size/2), y+RND_MM_TO_COORD(size/2), thick, 0, flg);
	}
	pcb_arc_new(ly, x, y, RND_MM_TO_COORD(size*0.325), RND_MM_TO_COORD(size*0.325), 0,360, thick, 0, flg, 0);

	/* finish registration of the subc */
	pcb_subc_bbox(subc);
	if (st->pcb != NULL) {
		if (st->pcb->Data->subc_tree == NULL)
			st->pcb->Data->subc_tree = rnd_r_create_tree();
		rnd_r_insert_entry(st->pcb->Data->subc_tree, (rnd_box_t *)subc);
		pcb_subc_rebind(st->pcb, subc);
	}
	else
		pcb_subc_reg(st->fp_data, subc);

	return 0;
}

/* kicad_pcb/gr_line or fp_line */
static int kicad_parse_any_line(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, pcb_flag_values_t flag, int is_seg)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;
	rnd_coord_t x1, y1, x2, y2, thickness, clearance;
	pcb_flag_t flg = pcb_flag_make(flag);
	pcb_layer_t *ly = NULL;
	pcb_line_t *line;

	TODO("apply poly clearance as in pool/io_kicad (CUCP#39)");
	clearance = thickness = 1; /* start with sane default of one nanometre */
	if (subc != NULL)
		clearance = 0;

	TODO("this workaround is for segment - remove it when clearance is figured CUCP#39");
	if (flag & PCB_FLAG_CLEARLINE)
		clearance = RND_MM_TO_COORD(0.250);

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
			if (st->primitive_ly != NULL)
				return kicad_error(n, "line in this context shall not have layer specified");
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
				/* doc: trunk/doc/developer/alien_formats/segment_status.txt */
				/* Conclusion: ignore anything but LOCKED (pcb-rnd doesn't need star/end on pad) */
				if ((n->children != NULL) && (n->children->str != NULL)) {
					unsigned long int kf;
					char *end;
					kf = strtoul(n->children->str, &end, 16);
					if (*end != '\0')
						return kicad_error(n->children, "invalid hex integer in status");
					if (kf & 0x40000UL)
						flg.f |= PCB_FLAG_LOCK;
				}
				else
					kicad_warning(n, "missing status hex integer");
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
	required = BV(0) | BV(1); /* | BV(3); now have 1nm default width, i.e. for edge cut */
	if (st->primitive_ly == NULL)
		required |= BV(2);
	else
		ly = st->primitive_ly;

	if ((tally & required) != required)
		return kicad_error(subtree, "failed to create line: missing fields");

	if (st->primitive_subc != NULL)
		subc = st->primitive_subc;

	if (subc != NULL) {
		rnd_coord_t sx, sy;
		if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
			x1 += sx;
			y1 += sy;
			x2 += sx;
			y2 += sy;
		}
	}
	x1 += st->primitive_dx; y1 += st->primitive_dy;
	x2 += st->primitive_dx; y2 += st->primitive_dy;
	line = pcb_line_new(ly, x1, y1, x2, y2, thickness, clearance, flg);
	if (st->primitive_term != NULL)
		pcb_attribute_put(&line->Attributes, "term", st->primitive_term);
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
	rnd_coord_t cx, cy, endx, endy, thickness, clearance, deltax, deltay;
	rnd_angle_t end_angle = 0.0, delta = 360.0; /* these defaults allow a gr_circle to be parsed, which does not specify (angle XXX) */
	pcb_flag_t flg = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_t *ly = NULL;
	pcb_arc_t *arc;

	TODO("apply poly clearance as in pool/io_kicad (CUCP#39)");
	if (subc == NULL)
		clearance = thickness = RND_MM_TO_COORD(0.250); /* start with sane defaults */
	else
		clearance = thickness = RND_MM_TO_COORD(0);

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
			if (st->primitive_ly != NULL)
				return kicad_error(n, "arc in this context shall not have layer specified");
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
	required = BV(0) | BV(1) | BV(3); /* | BV(4); not needed for circles */
	if (st->primitive_ly == NULL)
		required |= BV(2);
	else
		ly = st->primitive_ly;

	if ((tally & required) != required)
		return kicad_error(subtree, "unexpected empty/NULL node in arc.");

	{
		rnd_angle_t start_angle;
		rnd_coord_t width, height;

		width = height = rnd_distance(cx, cy, endx, endy); /* calculate radius of arc */
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

		if (st->primitive_subc != NULL)
			subc = st->primitive_subc;

		if (subc != NULL) {
			rnd_coord_t sx, sy;
			if (pcb_subc_get_origin(subc, &sx, &sy) == 0) {
				cx += sx;
				cy += sy;
			}
		}
		cx += st->primitive_dx; cy += st->primitive_dy;

		/* note: do not remove duplicate arcs, because term may need to be set on the result */
		arc = pcb_arc_new(ly, cx, cy, width, height, start_angle, delta, thickness, clearance, flg, rnd_false);
		if (st->primitive_term != NULL)
			pcb_attribute_put(&arc->Attributes, "term", st->primitive_term);
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
	gsxl_node_t *n, *nly1, *nly2;
	unsigned long tally = 0, required;
	int blind_cnt = 0;
	rnd_coord_t x, y, thickness, clearance, mask, drill; /* not sure what to do with mask */
	pcb_layer_t *ly1, *ly2; /* blind/buried: from-to layers */
	pcb_pstk_t *ps;

	TODO("apply poly clearance as in pool/io_kicad (CUCP#39)");
	clearance = mask = RND_MM_TO_COORD(0.250); /* start with something bland here */
	drill = RND_MM_TO_COORD(0.300); /* start with something sane */

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
			int top=0, bottom=0;
			pcb_layer_type_t lyt1, lyt2;
			
			SEEN_NO_DUP(tally, 2);
			nly1 = n->children;
			if ((nly1 == NULL) || (nly1->next == NULL) || (nly1->str == NULL) || (nly1->next->str == NULL))
				return kicad_error(n, "too few layers: \"layers\" must have exactly 2 parameters (start and end layer)");
			nly2 = nly1->next;
			if (nly2->next != NULL)
				return kicad_error(nly2->next, "too few many: \"layers\" must have exactly 2 parameters (start and end layer)");

			PARSE_LAYER(ly1, nly1, NULL, "via layer1");
			PARSE_LAYER(ly2, nly2, NULL, "via layer2");
			lyt1 = pcb_layer_flags_(ly1);
			lyt2 = pcb_layer_flags_(ly2);
			if (lyt1 & PCB_LYT_TOP) top++;
			if (lyt2 & PCB_LYT_TOP) top++;
			if (lyt1 & PCB_LYT_BOTTOM) bottom++;
			if (lyt2 & PCB_LYT_BOTTOM) bottom++;
			if ((top != 1) || (bottom != 1))
				blind_cnt++; /* a top-to-bottom via is not blind */
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
		else if (strcmp("blind", n->str) == 0) {
			SEEN_NO_DUP(tally, 6);
			blind_cnt++;
		}
		else
			kicad_warning(n, "Unknown via argument %s:", n->str);
	}

	/* need start, end, layer, thickness at a minimum */
	required = BV(0) | BV(1);
	if ((tally & required) != required)
		return kicad_error(subtree, "insufficient via arguments");

	if ((blind_cnt != 0) && (blind_cnt != 2))
		return kicad_error(subtree, "Contradiction in blind via parameters: the \"blind\" node must present and the via needs to end on an inner layer");

	ps = pcb_pstk_new_compat_via(st->pcb->Data, -1, x, y, drill, thickness, clearance, mask, PCB_PSTK_COMPAT_ROUND, rnd_true);
	if (ps == NULL)
		return kicad_error(subtree, "failed to create via-padstack");

	if (blind_cnt == 2) { /* change the prototype of the padstack to be blind or buried */
		const pcb_pstk_proto_t *orig_proto;
		pcb_pstk_proto_t *new_proto;
		rnd_cardinal_t new_pid;
		int ot1, ot2, ob1, ob2, err;
		rnd_layergrp_id_t gtop = pcb_layergrp_get_top_copper();
		rnd_layergrp_id_t gbot = pcb_layergrp_get_bottom_copper();

		orig_proto = pcb_pstk_get_proto(ps);
		assert(orig_proto != NULL);

		err = pcb_layergrp_dist(st->pcb, ly1->meta.real.grp, gtop, PCB_LYT_COPPER, &ot1);
		err |= pcb_layergrp_dist(st->pcb, ly2->meta.real.grp, gtop, PCB_LYT_COPPER, &ot2);
		err |= pcb_layergrp_dist(st->pcb, ly1->meta.real.grp, gbot, PCB_LYT_COPPER, &ob1);
		err |= pcb_layergrp_dist(st->pcb, ly2->meta.real.grp, gbot, PCB_LYT_COPPER, &ob2);

		if (err != 0)
			return kicad_error(subtree, "failed to calculate blind/buried via span: invalid layer(s) specified");
		if ((ot1 == ot2) || (ob1 == ob2))
			return kicad_error(subtree, "failed to calculate blind/buried via span: start layer group matches end layer group");

		pcb_pstk_pre(ps);
		new_proto = calloc(sizeof(pcb_pstk_proto_t), 1);
		pcb_pstk_proto_copy(new_proto, orig_proto); /* to get all the original shapes and attributes and flags */

		new_proto->htop = MIN(ot1, ot2);
		new_proto->hbottom = MIN(ob1, ob2);
		new_pid = pcb_pstk_proto_insert_or_free(st->pcb->Data, new_proto, 0, 0);
		ps->proto = new_pid;
		pcb_pstk_post(ps);
	}

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

	vtp0_init(&st->intern_copper);

	/* find the first non-signal layer */
	last_copper = -1;
	for(n = subtree; n != NULL; n = n->next) {
		int lnum, is_copper;
		if ((n->str == NULL) || (n->children->str == NULL) || (n->children->next == NULL) || (n->children->next->str == NULL)) {
			kicad_error(n, "unexpected board layer definition encountered\n");
			goto error;
		}
		lnum = atoi(n->str);
		is_copper = (strcmp(n->children->next->str, "signal") == 0) || (strcmp(n->children->next->str, "power") == 0) || (strcmp(n->children->next->str, "mixed") == 0);
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

	/* create internal copper layers, in the right order, regardless of in
	   what order they are present in the file */
	{
		int n, from, dir, reverse_order = (st->ver <= 3);

		if (reverse_order) {
			from = last_copper;
			dir = -1;
		}
		else {
			from = 0;
			dir = 1;
		}

		for(n = from; (n <= last_copper) && (n >= 0); n += dir) {
			void **ndp = vtp0_get(&st->intern_copper, n, 0);
			gsxl_node_t *nd;
			if ((ndp != NULL) && (*ndp != NULL)) {
				pcb_layergrp_t *g = pcb_get_grp_new_intern(st->pcb, -1);
				rnd_layergrp_id_t gid = g - st->pcb->LayerGroups.grp;
				const char *lname, *ltype;

				nd = *ndp;
				lname = nd->children->str;
				ltype = nd->children->next->str;

				if (kicad_create_copper_layer_(st, gid, lname, ltype, nd) != 0) {
					kicad_error(nd, "Failed to create internal copper layer: %d, %s, %s\n", n, lname, ltype);
					goto error;
				}
			}
		}
	}

	vtp0_uninit(&st->intern_copper);

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

	if (pcb_net_get(st->pcb, &st->pcb->netlist[PCB_NETLIST_INPUT], netname, PCB_NETA_ALLOC) == NULL)
		return kicad_error(subtree->next, "Failed to create net %s", netname);

	return 0;
}

/* Make a list of zone connects - these connections can not be made immediately
   because the polygons may not exist yet */
static void save_zone_connect(read_state_t *st, pcb_pstk_t *ps, int zone_connect, const char *netname)
{
	zone_connect_t *zc = malloc(sizeof(zone_connect_t));
	zc->ps = ps;
	zc->style = zone_connect;
	zc->netname = netname; /* it is safe to remember without strdup since it is a tree allocation */
	zc->next = st->zc_head;
	st->zc_head = zc;
}

static void exec_zone_connect(read_state_t *st)
{
	zone_connect_t *zc, *next;
	htpp_t poly_upd;
	htpp_entry_t *e;

	htpp_init(&poly_upd, ptrhash, ptrkeyeq);

	for(zc = st->zc_head; zc != NULL; zc = next) {
		rnd_layer_id_t lid;
		pcb_layer_t *ly;

		/* search all layers for polygons */
		for(lid = 0, ly = st->pcb->Data->Layer; lid < st->pcb->Data->LayerN; lid++,ly++) {
			rnd_rtree_it_t it;
			pcb_poly_t *p;

			if (ly->polygon_tree == NULL) continue;

			/* use rtree to search for polys that are near the padstack */
			for(p = rnd_rtree_first(&it, ly->polygon_tree, (const rnd_rtree_box_t *)&zc->ps->BoundingBox); p != NULL; p = rnd_rtree_next(&it)) {
				const char *pnet;
				pnet = htpp_get(&st->poly2net, p);
				if ((zc != NULL) && (zc->netname != NULL) && (pnet != NULL) && (strcmp(pnet, zc->netname) == 0)) {
					unsigned char *th = pcb_pstk_get_thermal(zc->ps, lid, 1);

					switch(zc->style) {
						case 1: (*th) |= PCB_THERMAL_ON | PCB_THERMAL_ROUND; break;
						case 2: (*th) |= PCB_THERMAL_ON | PCB_THERMAL_SOLID; break;
						case 3: (*th) |= PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL; break;
					}
					htpp_set(&poly_upd, p, p);
					rnd_trace("CONN lid=%ld p=%p in %s style=%d\n", lid, p, pnet, zc->style);
				}
			}
		}

		next = zc->next;
		free(zc);
	}

	/* assume many pins in a poly: it is cheaper to reclip affected polys once,
	   at the end, than to reclip on each padstack thermal installation in
	   the above loop */
	for(e = htpp_first(&poly_upd); e != NULL; e = htpp_next(&poly_upd, e)) {
		pcb_poly_t *p = e->key;
		pcb_poly_init_clip(p->parent.layer->parent.data, p->parent.layer, p);
	}
	htpp_uninit(&poly_upd);
}

typedef struct {
	pcb_layer_type_t want[PCB_LYT_INTERN+1]; /* indexed by location, contains OR'd bitmask of PCB_LYT_COPPER, PCB_LYT_MASK, PCB_LYT_PASTE */
} kicad_padly_t;



static void kicad_slot_shape(pcb_pstk_shape_t *shape, rnd_coord_t sx, rnd_coord_t sy)
{
	shape->shape = PCB_PSSH_LINE;
	if (sx > sy) { /* horizontal */
		shape->data.line.thickness = sy;
		shape->data.line.x1 = (-sx + sy)/2;
		shape->data.line.x2 = (+sx - sy)/2;
		shape->data.line.y1 = shape->data.line.y2 = 0;
	}
	else { /* vertical */
		shape->data.line.thickness = sx;
		shape->data.line.y1 = (-sy + sx)/2;
		shape->data.line.y2 = (+sy - sx)/2;
		shape->data.line.x1 = shape->data.line.x2 = 0;
	}
	shape->layer_mask = PCB_LYT_MECH;
	shape->comb = PCB_LYC_AUTO;
}


/* check if shape is wanted on a given layer - thru-hole version */
#define LYSHT(loc, typ) ((layers->want[PCB_LYT_ ## loc] & (PCB_LYT_ ## typ)))

/* check if shape is wanted on a given layer - SMD version */
#define LYSHS(loc, typ) ((layers->want[loc] & (PCB_LYT_ ## typ)))

static pcb_pstk_t *kicad_make_pad_thr(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t padXsize, rnd_coord_t padYsize, rnd_coord_t clearance, rnd_coord_t mask, rnd_coord_t paste, double paste_ratio, rnd_coord_t drillx, rnd_coord_t drilly, const char *pad_shape, int plated, kicad_padly_t *layers, double shape_arg, double shape_arg2)
{
	int len = 0, slot = (drillx != drilly);
	rnd_coord_t drill = 0;

	if (pad_shape == NULL) {
		kicad_error(subtree, "pin with no shape");
		return NULL;
	}

	paste_ratio = 1.0 + (2.0 * paste_ratio);

	if (!slot)
		drill = drillx;

	if ((strcmp(pad_shape, "rect") == 0) || (strcmp(pad_shape, "trapezoid") == 0) || (strcmp(pad_shape, "custom") == 0)) {
		/* "custom" pads always have a rectangular shape too; keep the rectangle as
		   padstack and add the custom shapes as heavy terminal together with the
		   padstack */
		pcb_pstk_shape_t sh[9];
		rnd_coord_t dy = RND_MM_TO_COORD(shape_arg), dx = RND_MM_TO_COORD(shape_arg2); /* x;y and 1;2 swapped intentionally - kicad does it like that */
		memset(sh, 0, sizeof(sh));
		if (LYSHT(TOP, MASK))      {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize+mask*2, padYsize+mask*2, dx, dy);}
		if (LYSHT(BOTTOM, MASK))   {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize+mask*2, padYsize+mask*2, dx, dy);}
		if (LYSHT(TOP, PASTE))     {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, dx, dy);}
		if (LYSHT(BOTTOM, PASTE))  {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, dx, dy);}
		if (LYSHT(TOP, COPPER))    {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_COPPER; pcb_shape_rect_trdelta(&sh[len++], padXsize, padYsize, dx, dy);}
		if (LYSHT(BOTTOM, COPPER)) {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_rect_trdelta(&sh[len++], padXsize, padYsize, dx, dy);}
		if (LYSHT(INTERN, COPPER)) {sh[len].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_rect_trdelta(&sh[len++], padXsize, padYsize, dx, dy);}
		if (slot)                  {kicad_slot_shape(&sh[len++], drillx, drilly);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, drill, plated, clearance, sh);
	}
	else if ((strcmp(pad_shape, "oval") == 0) || (strcmp(pad_shape, "circle") == 0)) {
		pcb_pstk_shape_t sh[9];
		memset(sh, 0, sizeof(sh));
		if (LYSHT(TOP, MASK))      {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize + mask * 2, padYsize + mask * 2);}
		if (LYSHT(BOTTOM, MASK))   {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize + mask * 2, padYsize + mask * 2);}
		if (LYSHT(TOP, PASTE))     {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2);}
		if (LYSHT(BOTTOM, PASTE))  {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2);}
		if (LYSHT(TOP, COPPER))    {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_COPPER; pcb_shape_oval(&sh[len++], padXsize, padYsize);}
		if (LYSHT(BOTTOM, COPPER)) {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_oval(&sh[len++], padXsize, padYsize);}
		if (LYSHT(INTERN, COPPER)) {sh[len].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_oval(&sh[len++], padXsize, padYsize);}
		if (slot)                  {kicad_slot_shape(&sh[len++], drillx, drilly);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, drill, plated, clearance, sh);
	}
	else if (strcmp(pad_shape, "roundrect") == 0) {
		pcb_pstk_shape_t sh[9];

		if ((shape_arg <= 0.0) || (shape_arg > 0.5)) {
			kicad_error(subtree, "Round rectangle ratio %f out of range: must be >0 and <=0.5", shape_arg);
			return NULL;
		}

		memset(sh, 0, sizeof(sh));
		if (LYSHT(TOP, MASK))      {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize+mask*2, padYsize+mask*2, shape_arg);}
		if (LYSHT(BOTTOM, MASK))   {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize+mask*2, padYsize+mask*2, shape_arg);}
		if (LYSHT(TOP, PASTE))     {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, shape_arg);}
		if (LYSHT(BOTTOM, PASTE))  {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, shape_arg);}
		if (LYSHT(TOP, COPPER))    {sh[len].layer_mask = PCB_LYT_TOP    | PCB_LYT_COPPER; pcb_shape_roundrect(&sh[len++], padXsize, padYsize, shape_arg);}
		if (LYSHT(BOTTOM, COPPER)) {sh[len].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; pcb_shape_roundrect(&sh[len++], padXsize, padYsize, shape_arg);}
		if (LYSHT(INTERN, COPPER)) {sh[len].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; pcb_shape_roundrect(&sh[len++], padXsize, padYsize, shape_arg);}
		if (slot)                  {kicad_slot_shape(&sh[len++], drillx, drilly);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, drill, plated, clearance, sh);
	}

	kicad_error(subtree, "unsupported pad shape '%s'.", pad_shape);
	return NULL;

	no_layer:;
	kicad_error(subtree, "thru-hole padstack ended up with no shape on any layer");
	return NULL;
}

static pcb_pstk_t *kicad_make_pad_smd(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t padXsize, rnd_coord_t padYsize, rnd_coord_t clearance, rnd_coord_t mask, rnd_coord_t paste, double paste_ratio, const char *pad_shape, pcb_layer_type_t side, kicad_padly_t *layers, double shape_arg, double shape_arg2)
{
	int len = 0;

	if ((side & PCB_LYT_TOP) && (side & PCB_LYT_BOTTOM)) {
		kicad_error(subtree, "can't place the same smd pad on both sides");
		return NULL;
	}

	if (!(side & PCB_LYT_TOP) && !(side & PCB_LYT_BOTTOM)) {
		kicad_error(subtree, "can't place smd pad on no side");
		return NULL;
	}

	paste_ratio = 1.0 + (2.0 * paste_ratio);

	if ((strcmp(pad_shape, "rect") == 0) || (strcmp(pad_shape, "trapezoid") == 0) || (strcmp(pad_shape, "custom") == 0)) {
		/* "custom" pads: see comment in kicad_make_pad_thr() */
		pcb_pstk_shape_t sh[4];
		rnd_coord_t dy = RND_MM_TO_COORD(shape_arg), dx = RND_MM_TO_COORD(shape_arg2); /* x;y and 1;2 swapped intentionally - kicad does it like that */
		memset(sh, 0, sizeof(sh));
		if (LYSHS(side, MASK))      {sh[len].layer_mask = side | PCB_LYT_MASK;   sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize+mask*2, padYsize+mask*2, dx, dy);}
		if (LYSHS(side, PASTE))     {sh[len].layer_mask = side | PCB_LYT_PASTE;  sh[len].comb = PCB_LYC_AUTO; pcb_shape_rect_trdelta(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, dx, dy);}
		if (LYSHS(side, COPPER))    {sh[len].layer_mask = side | PCB_LYT_COPPER; pcb_shape_rect_trdelta(&sh[len++], padXsize, padYsize, dx, dy);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, 0, rnd_false, clearance, sh);
	}
	else if ((strcmp(pad_shape, "oval") == 0) || (strcmp(pad_shape, "circle") == 0)) {
		pcb_pstk_shape_t sh[4];
		memset(sh, 0, sizeof(sh));
		if (LYSHS(side, MASK))      {sh[len].layer_mask = side | PCB_LYT_MASK; sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize+mask*2, padYsize+mask*2);}
		if (LYSHS(side, PASTE))     {sh[len].layer_mask = side | PCB_LYT_PASTE; sh[len].comb = PCB_LYC_AUTO; pcb_shape_oval(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2);}
		if (LYSHS(side, COPPER))    {sh[len].layer_mask = side | PCB_LYT_COPPER; pcb_shape_oval(&sh[len++], padXsize, padYsize);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, 0, rnd_false, clearance, sh);
	}
	else if (strcmp(pad_shape, "roundrect") == 0) {
		pcb_pstk_shape_t sh[4];

		if ((shape_arg <= 0.0) || (shape_arg > 0.5)) {
			kicad_error(subtree, "Round rectangle ratio %f out of range: must be >0 and <=0.5", shape_arg);
			return NULL;
		}

		memset(sh, 0, sizeof(sh));
		if (LYSHS(side, MASK))      {sh[len].layer_mask = side | PCB_LYT_MASK;   sh[len].comb = PCB_LYC_SUB | PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize+mask*2, padYsize+mask*2, shape_arg);}
		if (LYSHS(side, PASTE))     {sh[len].layer_mask = side | PCB_LYT_PASTE;  sh[len].comb = PCB_LYC_AUTO; pcb_shape_roundrect(&sh[len++], padXsize * paste_ratio + paste*2, padYsize * paste_ratio + paste*2, shape_arg);}
		if (LYSHS(side, COPPER))    {sh[len].layer_mask = side | PCB_LYT_COPPER; pcb_shape_roundrect(&sh[len++], padXsize, padYsize, shape_arg);}
		if (len == 0) goto no_layer;
		sh[len++].layer_mask = 0;
		return pcb_pstk_new_from_shape(subc->data, X, Y, 0, rnd_false, clearance, sh);
	}

	kicad_error(subtree, "unsupported pad shape '%s'.", pad_shape);
	return NULL;

	no_layer:;
	kicad_error(subtree, "SMD padstack ended up with no shape on any layer");
	return NULL;
}

#undef LYSHT
#undef LYSHS

static int kicad_make_pad(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, const char *netname, int throughHole, int plated, rnd_coord_t moduleX, rnd_coord_t moduleY, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t padXsize, rnd_coord_t padYsize, double pad_rot, unsigned int moduleRotation, rnd_coord_t clearance, rnd_coord_t mask, rnd_coord_t paste, double paste_ratio, int zone_connect, rnd_coord_t drillx, rnd_coord_t drilly, const char *pin_name, const char *pad_shape, unsigned long *featureTally, int *moduleEmpty, pcb_layer_type_t smd_side, kicad_padly_t *layers, double shape_arg, double shape_arg2)
{
	pcb_pstk_t *ps;
	unsigned long required;

	if (subc == NULL)
		return kicad_error(subtree, "error - unable to create incomplete module definition.");

	X += moduleX;
	Y += moduleY;

	if (throughHole) {
		required = BV(0) | BV(1);
		if ((*featureTally & required) != required)
			return kicad_error(subtree, "malformed module pad/pin definition.");
		if ((*featureTally & BV(3)) == 0)
			drillx = drilly = RND_MIL_TO_COORD(30); /* CUCP#63: default size is hardwired in pcbnew/class_pad.cpp */
		ps = kicad_make_pad_thr(st, subtree, subc, X, Y, padXsize, padYsize, clearance, mask, paste, paste_ratio, drillx, drilly, pad_shape, plated, layers, shape_arg, shape_arg2);
	}
	else {
		required = BV(0) | BV(1) | BV(2) | BV(5);
		if ((*featureTally & required) != required)
			return kicad_error(subtree, "error parsing incomplete module definition.");
		ps = kicad_make_pad_smd(st, subtree, subc, X, Y, padXsize, padYsize, clearance, mask, paste, paste_ratio, pad_shape, smd_side, layers, shape_arg, shape_arg2);
	}

	if (ps == NULL)
		return kicad_error(subtree, "failed to created padstack");

	if (zone_connect != 0)
		save_zone_connect(st, ps, zone_connect, netname);

	*moduleEmpty = 0;

	pad_rot -= st->subc_rot;
	if (pad_rot != 0.0) {
		pcb_pstk_pre(ps);

		if (ps->xmirror)
			ps->rot -= (double)pad_rot;
		else
			ps->rot += (double)pad_rot;

		if ((ps->rot > 360.0) || (ps->rot < -360.0))
			ps->rot = fmod(ps->rot, 360.0);

		if ((ps->rot == 360.0) || (ps->rot == -360.0))
			ps->rot = 0;

		/* force re-render the prototype */
		ps->protoi = -1;
		pcb_pstk_get_tshape(ps);
		pcb_pstk_bbox(ps);

		pcb_pstk_post(ps);
	}

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

		term = pcb_net_term_get(net, subc->refdes, pin_name, PCB_NETA_ALLOC);
		if (term == NULL)
			return kicad_error(subtree, "Failed to connect pad to net '%s'", netname);
	}

	return 0;
}

static int kicad_parse_fp_text(read_state_t *st, gsxl_node_t *n, pcb_subc_t *subc, unsigned long *tally, int *foundRefdes, double mod_rot, int mod_mirror)
{
	char *text;
	int hidden = 0, refdes = 0;

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
				hidden = 1;
			}
		}
		else /* only key, no value */
			text = key;
	}

	if (hidden)
		return 0; /* pcb-rnd policy: there are no hidden objects; if an object is not needed, it is not created */

	if (strcmp("reference", n->children->str) == 0)
		refdes = 1;

	if (kicad_parse_any_text(st, n->children->next->next, text, subc, mod_rot, mod_mirror, refdes) != 0)
		return -1;
	return 0;
}

/* Parse the (layers) subtree, set layer bits in "layers" and return smd_side 
   (PCB_LYT_ANYWHERE indicating on which side a pad is supposed to be)*/
pcb_layer_type_t kicad_parse_pad_layers(read_state_t *st, gsxl_node_t *subtree, gsxl_node_t *parent, kicad_padly_t *layers)
{
	gsxl_node_t *l;
	pcb_layer_type_t smd_side = 0;
	rnd_layer_id_t lid;
	int numly = 0, side_from_lyt = 0;

	for(l = subtree; l != NULL; l = l->next) {
		if (l->str != NULL) {
			int any = 0;
			pcb_layer_type_t lyt, lytor;

			if ((l->str[0] == 'F') || (l->str[0] == '*'))
				smd_side |= PCB_LYT_TOP;
			if ((l->str[0] == 'B') || (l->str[0] == '*'))
				smd_side |= PCB_LYT_BOTTOM;
			
			if (l->str[0] == '*') {
				any = 1;
				l->str[0] = 'F';
				lid = kicad_get_layeridx(st, l->str);
				if (lid < 0) {
					l->str[0] = 'B';
					lid = kicad_get_layeridx(st, l->str);
				}
				l->str[0] = '*';
			}
			else {
				lid = kicad_get_layeridx(st, l->str);
				side_from_lyt = 1;
			}

			if (lid < 0) {
				/* foreign language files have mismatched layer names in the stackup
				   and in the module layer reference. Work it around for a few known
				   cases */
				if (strcmp(l->str, "F.Cu")) { pcb_layer_list(st->pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &lid, 1); smd_side |= PCB_LYT_TOP; }
				else if (strcmp(l->str, "B.Cu")) { pcb_layer_list(st->pcb, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &lid, 1); smd_side |= PCB_LYT_BOTTOM; }
				else if (strcmp(l->str, "*.Cu")) pcb_layer_list(st->pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &lid, 1);
			}

			if (lid < 0)
				return kicad_error(l, "Unknown pad layer %s\n", l->str);

			if (st->pcb == NULL)
				lyt = st->fp_data->Layer[lid].meta.bound.type;
			else
				lyt = pcb_layer_flags(st->pcb, lid);
			lytor = lyt & PCB_LYT_ANYTHING;

			if ((side_from_lyt) && (smd_side == 0) && (lyt & PCB_LYT_COPPER)) {
				if (lyt & PCB_LYT_TOP) smd_side |= PCB_LYT_TOP;
				if (lyt & PCB_LYT_BOTTOM) smd_side |= PCB_LYT_BOTTOM;
			}

			if (any) {
				layers->want[PCB_LYT_TOP] |= lytor;
				layers->want[PCB_LYT_BOTTOM] |= lytor;
				if (lytor & PCB_LYT_COPPER)
					layers->want[PCB_LYT_INTERN] |= lytor;
			}
			else
				layers->want[(lyt & PCB_LYT_ANYWHERE) & 7] |= lytor;
			numly++;
		}
		else
			return kicad_error(l, "unexpected empty/NULL module layer node");
	}

	if (numly == 0) {
		kicad_warning(parent, "empty (layers) subtree in pad; assuming *.Cu");
		layers->want[PCB_LYT_TOP] |= PCB_LYT_COPPER;
		layers->want[PCB_LYT_BOTTOM] |= PCB_LYT_COPPER;
		layers->want[PCB_LYT_INTERN] |= PCB_LYT_COPPER;
	}

	return smd_side;
}

static int kicad_parse_pad_options(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	for(n = subtree; n != NULL; n = n->next) {
		if (strcmp(n->str, "clearance") == 0) {
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "missing clearance option argument");
			if (strcmp(n->children->str, "outline"))
				continue; /* normal */
			else if (strcmp(n->children->str, "convexhull"))
				kicad_warning(n, "convexhull clearance not supported - going to render a normam clearance around the pad");
			else
				return kicad_error(n, "unknwon clearance option argument %s", n->children->str);
		}
		else if (strcmp(n->str, "anchor") == 0) {
			/* Ignore: CUCP#60: used only as a geometry placeholder for
			   connection calculations, which is not required in pcb-rnd.
			   Should never be located outside of the main pad shape */
		}
		else
			return kicad_error(n, "unknwon pad option %s", n->str);
	}
	return 0;
}

TODO("eliminate this forward declaration by reordering the code")
static int kicad_parse_any_poly(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, rnd_coord_t modx, rnd_coord_t mody);

static int kicad_parse_gr_poly(read_state_t *st, gsxl_node_t *subtree)
{
	rnd_coord_t sx = 0, sy = 0;
	if (st->primitive_subc != NULL)
		pcb_subc_get_origin(st->primitive_subc, &sx, &sy);
	return kicad_parse_any_poly(st, subtree, st->primitive_subc, sx + st->primitive_dx, sy + st->primitive_dy);
}


/* Parse a primitives subtree found in custom pads - create heavy terminal on each target layer */
static int kicad_parse_primitives_(read_state_t *st, gsxl_node_t *primitives)
{
	static const dispatch_t disp[] = {
		{"gr_line", kicad_parse_gr_line},
		{"gr_arc", kicad_parse_gr_arc},
		{"gr_circle", kicad_parse_gr_arc},
		{"gr_text", kicad_parse_gr_text},
		{"gr_poly", kicad_parse_gr_poly},
		{NULL, NULL}
	};
	return kicad_foreach_dispatch(st, primitives, disp);
}

static int kicad_parse_primitives(read_state_t *st, gsxl_node_t *primitives, pcb_subc_t *subc, const char *term, const kicad_padly_t *layers)
{
	int res, warned = 0;
	const char *old_term;
	pcb_subc_t *old_subc;
	pcb_layer_t *old_ly;
	rnd_layer_id_t loc, *typ;
	rnd_layer_id_t ltypes[] = { PCB_LYT_COPPER, PCB_LYT_SILK, PCB_LYT_MASK, PCB_LYT_PASTE, 0};

	old_subc = st->primitive_subc;
	old_ly = st->primitive_ly;
	old_term = st->primitive_term;
	st->primitive_term = term;
	st->primitive_subc = subc;

	for(loc = PCB_LYT_TOP; loc <= PCB_LYT_INTERN; loc++) {
		for(typ = ltypes; *typ != 0; typ++) {
			if (layers->want[loc] & (*typ)) {
				pcb_layer_combining_t comb = 0;
				if ((*typ) & (PCB_LYT_PASTE | PCB_LYT_MASK | PCB_LYT_SILK)) comb |= PCB_LYC_AUTO;
				if ((*typ) & (PCB_LYT_MASK)) comb |= PCB_LYC_SUB;
				st->primitive_ly = pcb_subc_get_layer(subc, loc | *typ, comb, 1, "pad", rnd_false);
				if (st->primitive_ly != NULL) {
					res = kicad_parse_primitives_(st, primitives);
					if (!warned && ((*typ) & PCB_LYT_INTERN)) {
						warned = 1;
						kicad_warning(primitives, "custom pad shape on internal copper layer: creating it only on one layer\n");
					}
				}
			}
		}
	}

	st->primitive_term = old_term;
	st->primitive_ly = old_ly;
	st->primitive_subc = old_subc;
	return res;
}


static int kicad_parse_pad(read_state_t *st, gsxl_node_t *n, pcb_subc_t *subc, unsigned long *tally, rnd_coord_t moduleX, rnd_coord_t moduleY, unsigned int moduleRotation, rnd_coord_t mod_clr, rnd_coord_t mod_mask, rnd_coord_t mod_paste, double mod_paste_ratio, int mod_zone_connect, int *moduleEmpty)
{
	gsxl_node_t *m;
	rnd_coord_t x, y, drillx, drilly, sx, sy, clearance, mask = st->pad_to_mask_clearance*2, paste = 0;
	const char *netname = NULL;
	char *pin_name = NULL, *pad_shape = NULL;
	unsigned long feature_tally = 0;
	int through_hole = 0, plated = 0, definite_clearance = 0, zone_connect = mod_zone_connect;
	pcb_layer_type_t smd_side;
	double paste_ratio = mod_paste_ratio;
	kicad_padly_t layers = {0};
	double shape_arg = 0.0, shape_arg2 = 0.0;
	double rot = 0.0;

	sx = sy = RND_MIL_TO_COORD(60); /* CUCP#63: default size matching pcbnew/class_pad.cpp */

	/* Kicad clearance hierarchy (top overrides bottom):
	   - pad clearance
	   - module clearance
	   - zone_clearance (from (setup))
	*/
	clearance = 0;
	mask = 0;
	paste = 0;

	/* overwrite with module clearance if present */
	if (mod_clr != UNSPECIFIED) {
		clearance = mod_clr;
		definite_clearance = 1;
	}

	if (mod_mask  != UNSPECIFIED)
		mask = mod_mask;

	if (mod_paste != UNSPECIFIED)
		paste = mod_paste;

	if (n->children != 0 && n->children->str != NULL) {
		pin_name = n->children->str;
		pcb_obj_id_fix(pin_name);
		SEEN_NO_DUP(feature_tally, 0);
		if ((n->children->next == NULL) || (n->children->next->str == NULL))
			return kicad_error(n->children->next, "unexpected empty/NULL module pad type node");

		through_hole = (strcmp("thru_hole", n->children->next->str) == 0) || (strcmp("np_thru_hole", n->children->next->str) == 0);
		plated = through_hole && (n->children->next->str[0] != 'n');
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

			SEEN_NO_DUP(feature_tally, 1);
			PARSE_COORD(x, m, m->children, "module pad X");
			PARSE_COORD(y, m, m->children->next, "module pad Y");
			PARSE_DOUBLE(rot, NULL, m->children->next->next, "module pad rotation");
		}
		else if (strcmp("layers", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 2);
			smd_side = kicad_parse_pad_layers(st, m->children, m, &layers);
			if (smd_side == -1)
				return -1;
		}
		else if (strcmp("drill", m->str) == 0) {
			gsxl_node_t *nd;
			SEEN_NO_DUP(feature_tally, 3);
			nd = m->children;
			if (strcmp(nd->str, "offset") == 0) {
				kicad_warning(nd, "Ignoring drill offset");
				nd = nd->next;
			}
			
			if (nd == NULL) {
				/* do nothing; may happen if there's only an offset */
			}
			else if (strcmp(nd->str, "oval") == 0) {
				PARSE_COORD(drillx, m, nd->next, "module pad oval drill X");
				PARSE_COORD(drilly, m, nd->next->next, "module pad oval drill Y");
			}
			else {
				PARSE_COORD(drillx, m, nd, "module pad round drill");
				drilly = drillx;
			}
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
		else if (strcmp("solder_mask_margin", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 10);
			PARSE_COORD(mask, m, m->children, "module pad mask margin");
		}
		else if (strcmp("solder_mask_margin_ratio", m->str) == 0) {
			/* This feature doesn't seem to exist in kicad 4 or 5 sources, ignore it */
		}
		else if (strcmp("solder_paste_margin", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 6);
			PARSE_COORD(paste, m, m->children, "module pad paste margin");
		}
		else if (strcmp("solder_paste_ratio", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 6);
			PARSE_DOUBLE(paste_ratio, m, m->children, "module pad solder mask ratio");
		}
		else if (strcmp("clearance", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 7);
			PARSE_COORD(clearance, m, m->children, "module pad clearance");
			definite_clearance = 1;
		}
		else if (strcmp("primitives", m->str) == 0) {
			rnd_coord_t odx, ody;
			int res;
rnd_trace("**** prim dx;dy: %mm;%mm\n", x, y);
			odx = st->primitive_dx;
			ody = st->primitive_dy;
			st->primitive_dx += x;
			st->primitive_dy += y;
			res = kicad_parse_primitives(st, m->children, subc, pin_name, &layers);
			st->primitive_dx = odx;
			st->primitive_dy = ody;
			if (res != 0)
				return -1;
		}
		else if (strcmp("roundrect_rratio", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 8);
			PARSE_DOUBLE(shape_arg, m, m->children, "module pad roundrect_rratio");
		}
		else if (strcmp("rect_delta", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 8);
			PARSE_DOUBLE(shape_arg, m, m->children, "module pad rect_delta");
			PARSE_DOUBLE(shape_arg2, m, m->children->next, "module pad rect_delta");
		}
		else if (strcmp("options", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 9);
			if (kicad_parse_pad_options(st, m->children) != 0)
				return -1;
		}
		else if (strcmp("zone_connect", m->str) == 0) {
			SEEN_NO_DUP(feature_tally, 10);
			PARSE_DOUBLE(zone_connect, m, m->children, "module pad zone_connect");
		}
		else
			return kicad_error(m, "Unknown pad argument: %s", m->str);
	}

	if (subc != NULL) {
		if (!definite_clearance) {
			kicad_warning(n, "Couldn't determine pad clearance, using 0.25mm");
			clearance = st->zone_clearance;
		}
		if (kicad_make_pad(st, n, subc, netname, through_hole, plated, moduleX, moduleY, x, y, sx, sy, rot, moduleRotation, clearance, mask, paste, paste_ratio, zone_connect, drillx, drilly, pin_name, pad_shape, &feature_tally, moduleEmpty, smd_side, &layers, shape_arg, shape_arg2) != 0)
			return -1;
	}

	return 0;
}

static int kicad_parse_poly_pts(read_state_t *st, gsxl_node_t *subtree, pcb_poly_t *polygon, rnd_coord_t xo, rnd_coord_t yo)
{
	gsxl_node_t *m;
	rnd_coord_t x, y;

	if ((subtree == NULL) || (subtree->str == NULL))
		return kicad_error(subtree, "error parsing empty polygon.");

	if (strcmp("pts", subtree->str) != 0)
		return kicad_error(subtree, "pts section vertices not found in polygon.");

	for(m = subtree->children; m != NULL; m = m->next) {
		if ((m->str == NULL) || (strcmp("xy", m->str) != 0))
			return kicad_error(m, "empty pts element");

		PARSE_COORD(x, m, m->children, "polygon vertex X");
		PARSE_COORD(y, m, m->children->next, "polygon vertex Y");
		pcb_poly_point_new(polygon, x + xo, y + yo);
	}
	return 0;
}

static int kicad_parse_any_poly(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, rnd_coord_t modx, rnd_coord_t mody)
{
	gsxl_node_t *n, *npts = NULL;
	pcb_layer_t *ly = NULL;
	rnd_coord_t width = 0;
	unsigned long tally = 0;
	pcb_poly_t *poly;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);

	for(n = subtree; n != NULL; n = n->next) {
		if (strcmp(n->str, "pts") == 0) {
			SEEN_NO_DUP(tally, 1);
			npts = n;
		}
		else if (strcmp(n->str, "layer") == 0) {
			SEEN_NO_DUP(tally, 2);
			PARSE_LAYER(ly, n->children, subc, "line");
			if (st->primitive_ly != NULL)
				return kicad_error(n, "poly in this context shall not have layer specified");
		}
		else if (strcmp(n->str, "status") == 0) {
			SEEN_NO_DUP(tally, 3);
			TODO("do the same as for other object's status");
		}
		else if (strcmp(n->str, "tstamp") == 0) {
			SEEN_NO_DUP(tally, 4);
			/* ignore */
		}
		else if (strcmp(n->str, "width") == 0) {
			SEEN_NO_DUP(tally, 5);
			PARSE_COORD(width, n, n->children, "fp_poly width");
			if (width != 0) {
				TODO("need a core function for bloating the poly up but remember the original size?");
				kicad_warning(n, "Ignoring non-zero fp_poly width");
			}
		}
		else
			return kicad_error(subtree, "Invalid fp_poly child: %s", n->str);
	}

	if (npts == NULL)
		return kicad_error(subtree, "missing pts subtree in fp_poly or gr_poly");

	if (st->primitive_ly != NULL)
		ly = st->primitive_ly;
	if (ly == NULL)
		return kicad_error(subtree, "missing layer subtree in fp_poly or gr_poly");

	poly = pcb_poly_new(ly, 0, flags);
	if (kicad_parse_poly_pts(st, npts, poly, modx, mody) < 0)
		return -1;

	pcb_add_poly_on_layer(ly, poly);
	if (subc != NULL)
		pcb_poly_init_clip(subc->data, ly, poly);

	if (st->primitive_term != NULL)
		pcb_attribute_put(&poly->Attributes, "term", st->primitive_term);

	return 0;
}

static int kicad_parse_fp_poly(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, rnd_coord_t modx, rnd_coord_t mody)
{
	return kicad_parse_any_poly(st, subtree, subc, modx, mody);
}


static int kicad_parse_module(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n, *p;
	rnd_layer_id_t lid = 0;
	int on_bottom = 0, found_refdes = 0, module_empty = 1, module_defined = 0, i;
	int mod_zone_connect = 1; /* default seems to be connect; see CUCP#57, case labeled "-1" */
	double mod_rot = 0, mod_paste_ratio = 0;
	unsigned long tally = 0;
	rnd_coord_t mod_x = 0, mod_y = 0, mod_clr = UNSPECIFIED, mod_mask = UNSPECIFIED, mod_paste = UNSPECIFIED;
	char *mod_name;
	pcb_subc_t *subc = NULL;

	if (st->module_pre_create) {
		/* loading a module as a footprint - always create the subc in advance */
		subc = pcb_subc_new();
		pcb_subc_create_aux(subc, 0, 0, 0.0, 0);
		pcb_attribute_put(&subc->Attributes, "refdes", "K1");
		if (st->pcb != NULL) {
			pcb_subc_reg(st->pcb->Data, subc);
			pcb_subc_bind_globals(st->pcb, subc);
		}
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
				pcb_layer_type_t lyt;

				lid = kicad_get_layeridx(st, n->children->str);
				if (lid < 0)
					return kicad_error(n->children, "module layer error - unhandled layer %s", n->children->str);

				if (st->pcb == NULL)
					lyt = st->fp_data->Layer[lid].meta.bound.type;
				else
					lyt = pcb_layer_flags(st->pcb, lid);

				if (lyt & PCB_LYT_BOTTOM)
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

			key = rnd_concat("kicad_attr_", n->children->str, NULL);
			pcb_attribute_put(&subc->Attributes, key, "1");
			free(key);
		}
		else if (strcmp("at", n->str) == 0) {
			SEEN_NO_DUP(tally, 4);
			PARSE_COORD(mod_x, n, n->children, "module X");
			PARSE_COORD(mod_y, n, n->children->next, "module Y");
			PARSE_DOUBLE(mod_rot, NULL, n->children->next->next, "module rotation");
			st->subc_rot = mod_rot;

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
		else if (strcmp("clearance", n->str) == 0) {
			PARSE_COORD(mod_clr, n, n->children, "module pad clearance");
			SEEN_NO_DUP(tally, 9);
		}
		else if (strcmp("solder_mask_margin", n->str) == 0) {
			PARSE_COORD(mod_mask, n, n->children, "module pad solder_mask_margin");
			SEEN_NO_DUP(tally, 10);
		}
		else if (strcmp("model", n->str) == 0) {
			TODO("save this as attribute");
		}
		else if (strcmp("fp_text", n->str) == 0) {
			kicad_parse_fp_text(st, n, subc, &tally, &found_refdes, mod_rot, on_bottom);
		}
		else if (strcmp("descr", n->str) == 0) {
			SEEN_NO_DUP(tally, 11);
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected empty/NULL module descr node");
			pcb_attribute_put(&subc->Attributes, "kicad_descr", n->children->str);
		}
		else if (strcmp("tags", n->str) == 0) {
			SEEN_NO_DUP(tally, 12);
			if ((n->children == NULL) || (n->children->str == NULL))
				return kicad_error(n, "unexpected empty/NULL module tags node");
			pcb_attribute_put(&subc->Attributes, "kicad_tags", n->children->str);
		}
		else if (strcmp("solder_paste_margin", n->str) == 0) {
			PARSE_COORD(mod_paste, n, n->children, "module pad solder_paste_margin");
			SEEN_NO_DUP(tally, 13);
		}
		else if (strcmp("solder_paste_ratio", n->str) == 0) {
			PARSE_DOUBLE(mod_paste_ratio, n, n->children, "module pad solder_paste_ratio");
			SEEN_NO_DUP(tally, 14);
		}
		else if (strcmp("zone_connect", n->str) == 0) {
			PARSE_INT(mod_zone_connect, n, n->children, "module pad zone_connect");
			SEEN_NO_DUP(tally, 15);
		}
		else if (strcmp("path", n->str) == 0) {
			ignore_value_nodup(n, tally, 16, "unexpected empty/NULL module model node");
		}
		else if (strcmp("pad", n->str) == 0) {
			if (kicad_parse_pad(st, n, subc, &tally, mod_x, mod_y, mod_rot, mod_clr, mod_mask, mod_paste, mod_paste_ratio, mod_zone_connect, &module_empty) != 0)
				return -1;
		}
		else if (strcmp("fp_line", n->str) == 0) {
			if (kicad_parse_any_line(st, n->children, subc, 0, 0) != 0)
				return -1;
		}
		else if (strcmp("fp_poly", n->str) == 0) {
			if (kicad_parse_fp_poly(st, n->children, subc, mod_x, mod_y) != 0)
				return -1;
		}
		else if ((strcmp("fp_arc", n->str) == 0) || (strcmp("fp_circle", n->str) == 0)) {
			if (kicad_parse_any_arc(st, n->children, subc) != 0)
				return -1;
		}
		else if (strncmp("autoplace", n->str, 9) == 0) {
			/* ignore */
		}
		else
			return kicad_error(n, "Unknown module argument: %s\n", n->str);
	}

	if (subc == NULL)
		return kicad_error(subtree, "unable to create incomplete subc.");

	if ((mod_name != NULL) && (*mod_name != '\0') && (pcb_attribute_get(&subc->Attributes, "footprint") == NULL))
		pcb_attribute_put(&subc->Attributes, "footprint", mod_name);

	pcb_subc_bbox(subc);
	if (st->pcb != NULL) {
		if (st->pcb->Data->subc_tree == NULL)
			st->pcb->Data->subc_tree = rnd_r_create_tree();
		rnd_r_insert_entry(st->pcb->Data->subc_tree, (rnd_box_t *)subc);
		pcb_subc_rebind(st->pcb, subc);
	}
	else
		pcb_subc_reg(st->fp_data, subc);

	if ((mod_rot == 90) || (mod_rot == 180) || (mod_rot == 270)) {
		/* lossles module rotation for round steps */
		mod_rot = rnd_round(mod_rot / 90);
		pcb_subc_rotate90(subc, mod_x, mod_y, mod_rot);
	}
	else if (mod_rot != 0)
		pcb_subc_rotate(subc, mod_x, mod_y, cos(mod_rot/RND_RAD_TO_DEG), sin(mod_rot/RND_RAD_TO_DEG), mod_rot);

	st->subc_rot = 0.0;
	st->last_sc = subc;
	return 0;
}

static int kicad_parse_zone(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n, *m;
	unsigned long tally = 0, required;
	pcb_poly_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_layer_t *ly = NULL;
	char *net_name = NULL, *pclr = NULL;
	rnd_coord_t pclrc;

	for(n = subtree; n != NULL; n = n->next) {
		if (n->str == NULL)
			return kicad_error(n, "empty zone parameter");
		if (strcmp("net", n->str) == 0) {
			ignore_value_nodup(n, tally, 0, "unexpected zone net null node.");
		}
		else if (strcmp("net_name", n->str) == 0) {
			SEEN_NO_DUP(tally, 1);
			net_name = n->children->str;
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
				pclr = n->children->children->str;
				PARSE_COORD(pclrc, n->children, n->children->children, "zone connect_pads clearance");
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
		else if (strcmp("layers", n->str) == 0) {
			TODO("CUCP#49");
			kicad_warning(n->children, "multi-layer zones (e.g. keepouts affecting many layers) are not yet supported - polygon omitted");
			return 0;
		}
		else if (strcmp("layer", n->str) == 0) {
			SEEN_NO_DUP(tally, 10);
			PARSE_LAYER(ly, n->children, NULL, "zone polygon");
			if (ly != NULL) {
				polygon = pcb_poly_new(ly, 0, flags);
				if (polygon == NULL)
					return kicad_error(n->children, "Failed to create polygon");
			}
			kicad_warning(n->children, "no polygon layer - polygon omitted (only single layer polygons are supported)");
		}
		else if (strcmp("polygon", n->str) == 0) {
			if (polygon == NULL)
				return kicad_error(n->children, "Failed to create polygon, can not add points");
			if (kicad_parse_poly_pts(st, n->children, polygon, 0, 0) < 0)
				return -1;
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
			/* Ignore fill type: it's purely on-screen visual, doesn't affect data */
			/* doc: trunk/doc/alien_formats/io_kicad/hatch.txt */
		}
		else
			kicad_warning(n, "Unknown polygon argument:\t%s\n", n->str);
	}

	required = BV(10); /* layer at a minimum */
	if ((tally & required) != required)
		return kicad_error(subtree, "can not create zone because required fields are missing");

	if (polygon != NULL) {
		/* connect_pads/clearence field, when present, is poly-side clearance value;
		   load only when centrally enabled, else warn so the user knows how to enable */
		if (pclr != NULL) {
			if (!conf_core.import.alien_format.poly_side_clearance) {
				if (!st->warned_poly_side_clr) {
					rnd_message(RND_MSG_ERROR, "This kicad board has polygon side clearances that are IGNORED.\nTo enable loading them, change config node\nimport.alien_format.poly_side_clearance to true\n");
					st->warned_poly_side_clr = 1;
				}
			}
			else
				polygon->enforce_clearance = pclrc;
		}
		pcb_add_poly_on_layer(ly, polygon);
		pcb_poly_init_clip(st->pcb->Data, ly, polygon);

		if (st->poly2net_inited)
			htpp_set(&st->poly2net, polygon, net_name);
	}
	return 0;
}


/* Parse a board from &st->dom into st->pcb */
static int kicad_parse_pcb(read_state_t *st)
{
	int ret;

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
		{"gr_poly", kicad_parse_gr_poly},
		{"target", kicad_parse_target},
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
	ret = kicad_foreach_dispatch(st, st->dom.root->children, disp);

	/* create an extra layer for plated slots so they show up properly */
	{
		pcb_layergrp_t *g = pcb_get_grp_new_misc(st->pcb);
		rnd_layer_id_t lid = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "plated_slots", 0);
		pcb_layer_t *ly = pcb_get_layer(st->pcb->Data, lid);
		g->ltype = PCB_LYT_MECH;
		pcb_layergrp_set_purpose(g, "proute", 0);
		if (ly != NULL)
			ly->comb = PCB_LYC_AUTO;
	}

	return ret;
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

int io_kicad_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest)
{
	int readres = 0, clipi = 0;
	read_state_t st;
	gsx_parse_res_t res;
	FILE *FP;

	FP = rnd_fopen(&PCB->hidlib, Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));

	st.pcb = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_k2i, strhash, strkeyeq);
	htpp_init(&st.poly2net, ptrhash, ptrkeyeq);
	st.poly2net_inited = 1;

	/* A0 */
	st.width[DIM_FALLBACK] = RND_MM_TO_COORD(1189.0);
	st.height[DIM_FALLBACK] = RND_MM_TO_COORD(841.0);
	st.dim_valid[DIM_FALLBACK] = 1;

	/* load the file into the dom */
	res = kicad_parse_file(FP, &st.dom);
	fclose(FP);

	if (res == GSX_RES_EOE) {
		/* recursively parse the dom */
		if ((st.dom.root->str != NULL) && (strcmp(st.dom.root->str, "module") == 0)) {
			Ptr->is_footprint = 1;
			st.ver = 4;
			kicad_create_fp_layers(&st, st.dom.root);
			st.module_pre_create = 1;
			readres = kicad_parse_module(&st, st.dom.root->children);

/*			if (readres == 0) {
				pcb_layergrp_upgrade_to_pstk(Ptr);
			}*/
		}
		else {
			pcb_data_clip_inhibit_inc(st.pcb->Data);
			readres = kicad_parse_pcb(&st);
			clipi = 1;
		}
	}
	else
		readres = -1;

	exec_zone_connect(&st);
	htpp_uninit(&st.poly2net);

	/* clean up */
	gsxl_uninit(&st.dom);

	pcb_layer_auto_fixup(Ptr);

	if (pcb_board_normalize(Ptr) > 0)
		rnd_message(RND_MSG_WARNING, "Had to make changes to the coords so that the design fits the board.\n");
	pcb_layer_colors_from_conf(Ptr, 1);

	{ /* free layer hack */
		htsi_entry_t *e;
		for(e = htsi_first(&st.layer_k2i); e != NULL; e = htsi_next(&st.layer_k2i, e))
			free(e->key);
		htsi_uninit(&st.layer_k2i);
	}

	/* enable fallback to the default font picked up when creating the based
	   PCB (loading the default PCB), because we won't get a font from KiCad. */
	st.pcb->fontkit.valid = rnd_true;

	if (clipi)
		pcb_data_clip_inhibit_dec(st.pcb->Data, rnd_true);


	return readres;
}

int io_kicad_parse_module(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name, const char *subfpname)
{
	int mres;
	pcb_fp_fopen_ctx_t fpst;
	FILE *f;
	read_state_t st;
	gsx_parse_res_t res;

	f = pcb_fp_fopen(&conf_core.rc.library_search_paths, name, &fpst, NULL);

	if (f == NULL) {
		pcb_fp_fclose(f, &fpst);
		return -1;
	}

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.pcb = NULL;
	st.fp_data = Ptr;
	st.Filename = fpst.filename;
	st.settings_dest = RND_CFR_invalid;
	st.auto_layers = 1;

	res = kicad_parse_file(f, &st.dom);
	pcb_fp_fclose(f, &fpst);

	if (res != GSX_RES_EOE) {
		if (!pcb_io_err_inhibit)
			rnd_message(RND_MSG_ERROR, "Error parsing s-expression '%s'\n", name);
		gsxl_uninit(&st.dom);
		return -1;
	}

	if ((st.dom.root->str == NULL) || (strcmp(st.dom.root->str, "module") != 0)) {
		rnd_message(RND_MSG_ERROR, "Wrong root node '%s', expected 'module'\n", st.dom.root->str);
		gsxl_uninit(&st.dom);
		return -1;
	}

	htsi_init(&st.layer_k2i, strhash, strkeyeq);

	st.module_pre_create = 1;
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

/* Decide about the type of a footprint file:
   it is a kicad mdoule if the first non-comment is "(module"
   No tags are saved.
*/
pcb_plug_fp_map_t *io_kicad_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
	int c, comment_len;
	int first_module = 1;
	long pos = 0;
	enum {
		ST_WS,
		ST_COMMENT,
		ST_MODULE
	} state = ST_WS;

	while ((c = fgetc(f)) != EOF) {
		if (pos++ > 1024) break; /* header must be in the first kilobyte */
		switch (state) {
		case ST_MODULE:
			if (isspace(c))
				break;
			if (c == '(') {
				head->type = PCB_FP_FILE;
				goto out;
			}
		case ST_WS:
			if (isspace(c))
				break;
			if (c == '#') {
				comment_len = 0;
				state = ST_COMMENT;
				break;
			}
			else if ((first_module) && (c == '(')) {
				char s[8];
				fgets(s, 7, f);
				s[6] = '\0';
				if (strcmp(s, "module") == 0) {
					state = ST_MODULE;
					break;
				}
			}
			first_module = 0;
			/* fall-thru for detecting @ */
		case ST_COMMENT:
			comment_len++;
			if ((c == '\r') || (c == '\n'))
				state = ST_WS;
			break;
		}
	}

out:;
	return head;
}
