/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <qparse/qparse.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>

#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "plug_io.h"
#include "error.h"
#include "safe_fs.h"
#include "compat_misc.h"

#include "obj_subc.h"

#define ltrim(s) while(isspace(*s)) (s)++

typedef struct node_s node_t;

struct node_s {
	char **argv;
	int argc;
	int level;
	node_t *parent, *next;
	node_t *first_child, *last_child;
};

typedef struct {
	node_t *root, *curr;
} hkp_tree_t;

typedef struct {
	const pcb_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	pcb_pstk_shape_t shp;
} hkp_shape_t;

typedef struct {
	const pcb_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	unsigned plated:1;
	pcb_coord_t dia;
} hkp_hole_t;

typedef struct {
	const pcb_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	pcb_pstk_proto_t proto;
} hkp_pstk_t;


typedef struct {
	pcb_board_t *pcb;

	int num_cop_layers;

	const pcb_unit_t *unit;      /* default unit used while converting coords any given time */
	const pcb_unit_t *pstk_unit; /* default unit for the padstacks file */

	htsp_t shapes;   /* name -> hkp_shape_t */
	htsp_t holes;    /* name -> hkp_hole_t */
	htsp_t pstks;    /* name -> hkp_pstk_t */
	hkp_tree_t layout, padstacks;
} hkp_ctx_t;

/*** read_pstk.c ***/
static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd);


/*** High level parser ***/

/* Return the idxth sibling with matching name */
static node_t *find_nth(node_t *nd, char *name, int idx)
{
	for(; nd != NULL; nd = nd->next) {
		if (strcmp(nd->argv[0], name) == 0) {
			if (idx == 0)
				return nd;
			idx--;
		}
	}
	return NULL;
}

/* parse a string it into a coord - modifies s; returns 0 on success */
static int parse_coord(hkp_ctx_t *ctx, char *s, pcb_coord_t *crd)
{
	char *end;
	pcb_bool suc;

	end = strchr(s, ',');
	if (end != NULL)
		*end = '\0';

	*crd = pcb_get_value(s, ctx->unit->suffix, NULL, &suc);
	return !suc;
}

static int parse_x(hkp_ctx_t *ctx, char *s, pcb_coord_t *crd)
{
	return parse_coord(ctx, s, crd);
}


static int parse_y(hkp_ctx_t *ctx, char *s, pcb_coord_t *crd)
{
	pcb_coord_t tmp;
	if (parse_coord(ctx, s, &tmp) != 0)
		return -1;
	*crd = -tmp;
	return 0;
}


/* split s and parse  it into (x,y) - modifies s */
static int parse_xy(hkp_ctx_t *ctx, char *s, pcb_coord_t *x, pcb_coord_t *y, int xform)
{
	char *sy;
	pcb_coord_t xx, yy;
	pcb_bool suc1, suc2;

	sy = strchr(s, ',');
	if (sy == NULL)
		return -1;

	*sy = '\0';
	sy++;


	xx = pcb_get_value(s, ctx->unit->suffix, NULL, &suc1);
	yy = pcb_get_value(sy, ctx->unit->suffix, NULL, &suc2);

	if (xform) {
		yy = -yy;
	}
	
	*x = xx;
	*y = yy;

	return !(suc1 && suc2);
}

TODO("May need to get a side arg, if rotation is specified differently on the back side");
static int parse_rot(hkp_ctx_t *ctx, node_t *nd, double *rot_out)
{
	double rot;
	char *end;

	/* hkp rotation: top side positive value is CW; in pcb-rnd: that's negative */
	rot = -strtod(nd->argv[1], &end);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "Wrong rotation value '%s' (expected numeric)\n", nd->argv[1]);
		return -1;
	}
	if ((rot < -360) || (rot > 360)) {
		pcb_message(PCB_MSG_ERROR, "Wrong rotation value '%s' (out of range)\n", nd->argv[1]);
		return -1;
	}
	*rot_out = rot;
	return 0;
}

#define DWG_LY(node, name) \
	if (ly == NULL) { \
		node_t *__tmp__ = find_nth(node->first_child, name, 0); \
		if (__tmp__ == NULL) { \
			pcb_message(PCB_MSG_ERROR, "Missing layer reference (%s)\n", name); \
			return; \
		} \
	}

TODO("^^^ process the layer name");


#define DWG_REQ_LY(node) \
	if (ly == NULL) { \
		pcb_message(PCB_MSG_ERROR, "Internal error: expected existing layer from the caller\n"); \
		return; \
	}

static void parse_dwg_path_polyline(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *pp)
{
	node_t *tmp;
	pcb_coord_t th = 1, px, py, x, y;
	int n;

	DWG_REQ_LY(pp);

	th = PCB_MM_TO_COORD(0.5);
	tmp = find_nth(pp->first_child, "WIDTH", 0);
	if (tmp != NULL)
		parse_coord(ctx, tmp->argv[1], &th);
	tmp = find_nth(pp->first_child, "XY", 0);
TODO("handle error: what it tmp == NULL?");
	parse_xy(ctx, tmp->argv[1], &px, &py, 1);
	for(n = 2; n < tmp->argc; n++) {
		parse_xy(ctx, tmp->argv[n], &x, &y, 1);
		pcb_line_new(ly, px, py, x, y, th, 0, pcb_no_flags());
		px = x;
		py = y;
	}
}

static void parse_dwg_path_rect(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *rp)
{
	node_t *tmp;
	pcb_coord_t th = 1, x1, y1, x2, y2;

	DWG_REQ_LY(rp);

	tmp = find_nth(rp->first_child, "WIDTH", 0);
	if (tmp != NULL)
		parse_coord(ctx, tmp->argv[1], &th);
	tmp = find_nth(rp->first_child, "XY", 0);
	parse_xy(ctx, tmp->argv[1], &x1, &y1, 1);
	parse_xy(ctx, tmp->argv[2], &x2, &y2, 1);
	pcb_line_new(ly, x1, y1, x2, y1, th, 0, pcb_no_flags());
	pcb_line_new(ly, x2, y1, x2, y2, th, 0, pcb_no_flags());
	pcb_line_new(ly, x2, y2, x1, y2, th, 0, pcb_no_flags());
	pcb_line_new(ly, x1, y2, x1, y1, th, 0, pcb_no_flags());
}

/* Returns number of objects drawn: 0 or 1 */
static int parse_dwg(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *n)
{
	if (strcmp(n->argv[0], "POLYLINE_PATH") == 0)
		parse_dwg_path_polyline(ctx, subc, ly, n);
	else if (strcmp(n->argv[0], "RECT_PATH") == 0)
		parse_dwg_path_rect(ctx, subc, ly, n);
	else
		return 0;

	return 1;
}

/* Returns number of objects drawn: 0 or more */
static long parse_dwg_all(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *nd)
{
	node_t *n;
	long cnt = 0;

	for(n = nd->first_child; n != NULL; n = n->next)
		cnt += parse_dwg(ctx, subc, ly, n);

	return cnt;
}

/* Returns number of objects drawn: 0 or more */
static long parse_dwg_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *n)
{
	long cnt = 0;
	node_t *tmp;

	if (strcmp(n->argv[0], "SILKSCREEN_OUTLINE") == 0) {
		pcb_layer_t *ly;
		pcb_layer_type_t side = PCB_LYT_TOP;;
		tmp = find_nth(n->first_child, "SIDE", 0);
		if (tmp != NULL) {
			if (strcmp(tmp->argv[1], "MNT_SIDE") == 0) side = PCB_LYT_TOP;
			else side = PCB_LYT_BOTTOM;
		}
		ly = pcb_subc_get_layer(subc, side | PCB_LYT_SILK, PCB_LYC_AUTO, 1, "top-silk", 0);
		cnt += parse_dwg_all(ctx, subc, ly, n);
	}

	return cnt;
}

static pcb_layer_t *parse_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, const char *ln)
{
	pcb_layer_t *ly;
	pcb_layer_type_t lyt = 0;
	pcb_layer_combining_t lyc = 0;
	char *purpose = NULL;
	char *name = NULL;

	if (strncmp(ln, "LYR_", 4) == 0) {
		int cidx;
		char *end;
		TODO("can a subcircuit have intern copper objects? assume NO for now:");
		assert(subc == NULL);
		cidx = strtol(ln+4, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "Unknown layer '%s' (expected integer after LYR_)\n", ln);
			return NULL;
		}
		cidx--; /* hkp numbers from 1, we number from 0 */
		if ((cidx < 0) || (cidx >= ctx->num_cop_layers)) {
			pcb_message(PCB_MSG_ERROR, "Layer '%s' is out of range (1..%d)\n", ln, ctx->num_cop_layers);
			return NULL;
		}
		TODO("pick board copper layer");
		return NULL;
	}
	else if (strcmp(ln, "SILKSCREEN_TOP") == 0) {
		lyt = PCB_LYT_TOP | PCB_LYT_SILK;
		lyc = PCB_LYC_AUTO;
		name = "silk-top";
	}
	else if (strcmp(ln, "SILKSCREEN_BOTTOM") == 0) {
		lyt = PCB_LYT_BOTTOM | PCB_LYT_SILK;
		lyc = PCB_LYC_AUTO;
		name = "silk-bot";
	}
	else if (strcmp(ln, "SOLDERMASK_TOP") == 0) {
		lyt = PCB_LYT_TOP | PCB_LYT_MASK;
		lyc = PCB_LYC_AUTO;
	}
	else if (strcmp(ln, "SOLDERMASK_BOTTOM") == 0) {
		lyt = PCB_LYT_BOTTOM | PCB_LYT_MASK;
		lyc = PCB_LYC_AUTO;
	}
	else if (strcmp(ln, "ASSEMBLY_TOP") == 0) {
		lyt = PCB_LYT_TOP | PCB_LYT_DOC;
		lyc = PCB_LYC_AUTO;
		purpose = "assy";
		name = "top-assy";
	}
	else if (strcmp(ln, "ASSEMBLY_BOTTOM") == 0) {
		lyt = PCB_LYT_BOTTOM | PCB_LYT_DOC;
		lyc = PCB_LYC_AUTO;
		purpose = "assy";
		name = "bot-assy";
	}
	else if (strncmp(ln, "DRILLDRAWING_", 13) == 0) {
		lyt = PCB_LYT_TOP | PCB_LYT_DOC;
		lyc = PCB_LYC_AUTO;
		purpose = "fab";
		name = "top-fab";
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Unknown layer '%s'\n", ln);
		return NULL;
	}

	if (subc != NULL) {
		ly = pcb_subc_get_layer(subc, lyt, lyc, 1, name, (purpose != NULL));
		if ((purpose != NULL) && (ly->meta.bound.purpose == NULL)) /* newly created bound layer */
			ly->meta.bound.purpose = pcb_strdup(purpose);
	}
	else {
		int ln = pcb_layer_listp(ctx->pcb, lyt, &ly, 1, -1, purpose);
		if (ln == 0)
			return NULL;
		return ly;
	}
	return ly;
}

static void parse_subc_text(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *textnode)
{
	node_t *tt, *attr, *tmp;
	pcb_coord_t tx, ty;
	double rot = 0;
	pcb_layer_t *ly;
	const char *text_str;
	pcb_flag_values_t flg = PCB_FLAG_FLOATER;
	int omit_on_silk = 0;

	tt = find_nth(textnode->first_child, "TEXT_TYPE", 0);
	if (tt == NULL)
		return;

	text_str = textnode->argv[1];
	if (strcmp(tt->argv[1], "REF_DES") == 0) {
		pcb_attribute_put(&subc->Attributes, "refdes", textnode->argv[1]);
		text_str = "%a.parent.refdes%";
		flg |= PCB_FLAG_DYNTEXT;
	}
	else if (strcmp(tt->argv[1], "PARTNO") == 0) {
		pcb_attribute_put(&subc->Attributes, "footprint", textnode->argv[1]);
		text_str = "%a.parent.footprint%";
		flg |= PCB_FLAG_DYNTEXT;
		omit_on_silk = 1;
	}

	attr = find_nth(tt, "DISPLAY_ATTR", 0);
	if (attr == NULL)
		return;

	tmp = find_nth(attr->first_child, "TEXT_LYR", 0);
	if (tmp == NULL)
		return;
	ly = parse_layer(ctx, subc, tmp->argv[1]);
	if (ly == NULL)
		return;

	/* Special case: although the PARTNO is written on the silk layer by the
	   hkp file, it does not show up on the gerber so there must be some
	   mechanism in the original software to hide it */
	if (omit_on_silk && (pcb_layer_flags_(ly) & PCB_LYT_SILK))
		return;

	tmp = find_nth(attr->first_child, "XY", 0);
	if (tmp != NULL) {
		parse_x(ctx, tmp->argv[1], &tx);
		parse_y(ctx, tmp->argv[2], &ty);
	}

	tmp = find_nth(attr->first_child, "ROTATION", 0);
	if (tmp != NULL)
		parse_rot(ctx, tmp, &rot);

TODO("we should compensate for HOTIZ_JUST and VERT_JUST but for that we need to figure how big the text is originally");
TODO("HEIGHT should become scale");
TODO("figure what TEXT_OPTIONS we have");
TODO("STROKE_WIDTH: we have support for that, but what's the unit? what if it is 0?");
	pcb_text_new(ly, pcb_font(PCB, 0, 0), tx, ty, rot, 100, 0, text_str, pcb_flag_make(flg));
}


static pcb_subc_t *parse_package(hkp_ctx_t *ctx, pcb_data_t *dt, node_t *nd)
{
	pcb_subc_t *subc;
	node_t *n, *tmp;
	pcb_coord_t ox, oy;
	double rot = 0;
	int on_bottom = 0, seen_oxy = 0;

	subc = pcb_subc_alloc();

	/* extract global */
	for(n = nd->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "DESCRIPTION") == 0) {
			pcb_attribute_put(&subc->Attributes, "description", n->argv[1]);
		}
		else if (strcmp(n->argv[0], "XY") == 0) {
			if ((parse_x(ctx, n->argv[1], &ox) != 0) || (parse_y(ctx, n->argv[2], &oy) != 0)) {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken placement XY coord\n");
				return NULL;
			}
			seen_oxy = 1;
		}
		else if (strcmp(n->argv[0], "ROTATION") == 0) {
			if (parse_rot(ctx, n, &rot) != 0) {
				pcb_message(PCB_MSG_ERROR, "Can't load package due to wrong rotation value\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "FACEMENT") == 0) {
			if (strcmp(n->argv[1], "TOP") == 0) on_bottom = 0;
			else if (strcmp(n->argv[1], "BOTTOM") == 0) on_bottom = 1;
			else {
				pcb_message(PCB_MSG_ERROR, "Can't load package: broken facement (should be TOP or BOTTOM)\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "TEXT") == 0) {
			parse_subc_text(ctx, subc, n);
		}
	}

	if (!seen_oxy) {
		pcb_subc_free(subc);
		pcb_message(PCB_MSG_ERROR, "Can't load package: no placement XY coord\n");
		return NULL;
	}

	if (dt != NULL) {
		pcb_subc_reg(dt, subc);
		pcb_obj_id_reg(dt, subc);

		/* bind the via rtree so that vias added in this subc show up on the board */
		if (ctx->pcb != NULL)
			pcb_subc_bind_globals(ctx->pcb, subc);
	}

	pcb_subc_create_aux(subc, ox, oy, rot, on_bottom);

	/* extract pins and silk lines */
	for(n = nd->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "PIN") == 0)
			parse_pin(ctx, subc, n);
		else 
			parse_dwg_layer(ctx, subc, n);
	}

#if 0
	/* extract tags */
	for(n = nd->first_child; n != NULL; n = n->next) {
		for(t = tagnames; *t != NULL; t++) {
			if (strcmp(n->argv[0], *t) == 0) {
/*				printf("TAG %s=%s\n", *t, n->argv[1]); */
			}
		}
	}
#endif


	pcb_subc_bbox(subc);
	if ((dt != NULL) && (ctx->pcb != NULL)) {

		if (dt->subc_tree == NULL)
			dt->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(dt->subc_tree, (pcb_box_t *)subc);

		pcb_subc_rebind(ctx->pcb, subc);
	}
	return subc;
}

static const pcb_unit_t *parse_units(const char *ust)
{
	if (strcmp(ust, "MIL") == 0) return get_unit_struct("mil");
	if (strcmp(ust, "TH") == 0)  return get_unit_struct("mil");
	if (strcmp(ust, "MM") == 0)  return get_unit_struct("mm");
	if (strcmp(ust, "IN") == 0)  return get_unit_struct("inch");
	return NULL;
}

static int parse_layout_root(hkp_ctx_t *ctx, hkp_tree_t *tree)
{
	node_t *n;
	char *end;

	ctx->num_cop_layers = -1;

	/* extract globals */
	for(n = tree->root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "NUMBER_OF_LAYERS") == 0) {
			ctx->num_cop_layers = strtol(n->argv[1], &end, 10);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Invalid number of layers '%s' (expected integer)\n", n->argv[1]);
				return -1;
			}
			if ((ctx->num_cop_layers < 1) || (ctx->num_cop_layers > ((PCB_MAX_LAYER/2)-8))) {
				pcb_message(PCB_MSG_ERROR, "Invalid number of layers '%s' (out of range)\n", n->argv[1]);
				return -1;
			}
		}
		else if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->unit = parse_units(n->argv[1]);
			if (ctx->unit == NULL) {
				pcb_message(PCB_MSG_ERROR, "Unknown unit '%s'\n", n->argv[1]);
				return -1;
			}
		}
	}

	if (ctx->num_cop_layers < 0) {
		pcb_message(PCB_MSG_ERROR, "Missing NUMBER_OF_LAYERS\n");
		return -1;
	}

	{ /* create the layer stack: copper layers, as many as required by the header */
		int len = 0, n;
		pcb_dflgmap_t map[PCB_MAX_LAYERGRP];
		const pcb_dflgmap_t *m;

		while(ctx->pcb->LayerGroups.len > 0)
			pcb_layergrp_del(ctx->pcb, 0, 1);

		for(m = pcb_dflg_top_noncop; m->name != NULL; m++) map[len++] = *m;

		map[len++] = pcb_dflg_top_copper;
		map[len++] = pcb_dflg_substrate;
		for(n = 0; n < (ctx->num_cop_layers-2); n++) {
			map[len++] = pcb_dflg_int_copper;
			map[len++] = pcb_dflg_substrate;
		}
		map[len++] = pcb_dflg_bot_copper;

		for(m = pcb_dflg_bot_noncop; m->name != NULL; m++) map[len++] = *m;
		for(m = pcb_dflg_glob_noncop; m->name != NULL; m++) map[len++] = *m;

		map[len].name = NULL; /* terminator */
		pcb_layergrp_create_by_map(ctx->pcb, map);
	}

	/* plus assy and fab layers */
	pcb_layergrp_upgrade_by_map(ctx->pcb, pcb_dflgmap_doc);

	/* build packages and draw objects */
	for(n = tree->root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "PACKAGE_CELL") == 0)
			parse_package(ctx, ctx->pcb->Data, n);
		else
			parse_dwg(ctx, NULL, NULL, n);
	}

	/* 'autocrop' the board for now (required by y mirror and unknown extents) */
	{
		pcb_box_t bb;
		pcb_data_normalize(ctx->pcb->Data);
		pcb_data_bbox(&bb, ctx->pcb->Data, 0);
		ctx->pcb->hidlib.size_x = bb.X2;
		ctx->pcb->hidlib.size_y = bb.Y2;
	}

	return 0;
}


/*** Low level parser ***/

static void dump(node_t *nd)
{
	int n;
	for(n = 0; n < nd->level; n++)
		printf(" ");
	if (nd->argc > 0) {
		printf("%s", nd->argv[0]);
		for(n = 1; n < nd->argc; n++)
			printf("|%s", nd->argv[n]);
	}
	else
		printf("<empty>");
	printf("\n");
	for(nd = nd->first_child; nd != NULL; nd = nd->next)
		dump(nd);
}

static void destroy(node_t *nd)
{
	node_t *n, *next;

	if (nd == NULL)
		return;

	qparse_free(nd->argc, &nd->argv);

	for(n = nd->first_child; n != NULL; n = next) {
		next = n->next;
		destroy(n);
	}

	free(nd);
}

/* Split up a virtual line and save it in the tree */
void save_vline(hkp_tree_t *tree, char *vline, int level)
{
	node_t *nd;

	nd = calloc(sizeof(node_t), 1);
	nd->argc = qparse2(vline, &nd->argv, QPARSE_DOUBLE_QUOTE | QPARSE_PAREN | QPARSE_MULTISEP);
	nd->level = level;

	if (level == tree->curr->level) { /* sibling */
		sibling:;
		nd->parent = tree->curr->parent;
		tree->curr->next = nd;
		nd->parent->last_child = nd;
	}
	else if (level == tree->curr->level+1) { /* first child */
		tree->curr->first_child = tree->curr->last_child = nd;
		nd->parent = tree->curr;
	}
	else if (level < tree->curr->level) { /* step back to a previous level */
		while(level < tree->curr->level) tree->curr = tree->curr->parent;
		goto sibling;
	}
	tree->curr = nd;
}

static void rtrim(gds_t *s)
{
	int n;
	for(n = gds_len(s)-1; (n >= 0) && isspace(s->array[n]); n--)
		s->array[n] = '\0';
}

static void load_hkp(hkp_tree_t *tree, FILE *f)
{
	char *s, line[1024];
	gds_t vline;
	int level;

	tree->curr = tree->root = calloc(sizeof(node_t), 1);
	gds_init(&vline);

	/* read physical lines, build virtual lines and save them in the tree*/
	while(fgets(line, sizeof(line), f) != NULL) {
		s = line;
		if (*s == '!') s++; /* header line prefix? */
		while(isspace(*s)) s++;

		/* first char is '.' means it's a new virtual line */
		if (*s == '.') {
			if (gds_len(&vline) > 0) {
				rtrim(&vline);
				save_vline(tree, vline.array, level);
				gds_truncate(&vline, 0);
			}
			level = 0;
			while(*s == '.') { s++; level++; };
		}

		if (gds_len(&vline) > 0)
			gds_append(&vline, ' ');
		gds_append_str(&vline, s);
	}

	/* the last virtual line before eof */
	if (gds_len(&vline) > 0) {
		rtrim(&vline);
		save_vline(tree, vline.array, level);
	}
	gds_uninit(&vline);

}

#include "read_pstk.c"

int io_mentor_cell_read_pcb(pcb_plug_io_t *pctx, pcb_board_t *pcb, const char *fn, conf_role_t settings_dest)
{
	hkp_ctx_t ctx;
	int res = -1;
	FILE *flay;
	char *end, fn2[PCB_PATH_MAX];

	memset(&ctx, 0, sizeof(ctx));

	flay = pcb_fopen(&PCB->hidlib, fn, "r");
	if (flay == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open layout hkp '%s' for read\n", fn);
		goto err;
	}

	/* create the file name for the padstacks */
	strncpy(fn2, fn, PCB_PATH_MAX);
	fn2[PCB_PATH_MAX-1] = '\0';
	end = strrchr(fn2, PCB_DIR_SEPARATOR_C);
	if (end == NULL)
		end = fn2;
	else
		end++;
	strcpy(end, "Padstack.hkp");

	if (io_mentor_cell_pstks(&ctx, fn2) != 0) {
		fclose(flay);
		goto err;
	}

	ctx.pcb = pcb;
	ctx.unit = get_unit_struct("mm");

	load_hkp(&ctx.layout, flay);
	fclose(flay);

	/* we are loading the cells into a board, make a default layer stack for that */
	pcb_layergrp_inhibit_inc();
	pcb_layers_reset(pcb);
	pcb_layer_group_setup_default(pcb);
	pcb_layer_group_setup_silks(pcb);
	pcb_layer_auto_fixup(pcb);
	pcb_layergrp_inhibit_dec();

	/* parse the root */
	res = parse_layout_root(&ctx, &ctx.layout);

	pcb_layer_colors_from_conf(pcb, 1);

	res = 0; /* all ok */

	err:;
	free_pstks(&ctx);
	if (res != 0) {
		if (ctx.layout.root != NULL) {
			printf("### layout tree:\n");
			dump(ctx.layout.root);
		}
		if (ctx.padstacks.root != NULL) {
			printf("### padstack tree:\n");
			dump(ctx.padstacks.root);
		}
	}
	destroy(ctx.padstacks.root);
	destroy(ctx.layout.root);

	return res;
}

int io_mentor_cell_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s)) s++; /* strip leading whitespace */
			if (strncmp(s, ".FILETYPE LAYOUT", 16) == 0) /* valid root */
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line - and we don't have our root -> it's not an s-expression */
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}
