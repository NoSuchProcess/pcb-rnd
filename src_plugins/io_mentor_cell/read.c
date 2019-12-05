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
#include "netlist.h"

#include "obj_subc.h"

#define ltrim(s) while(isspace(*s)) (s)++

typedef struct node_s node_t;
typedef struct hkp_tree_s hkp_tree_t;

struct node_s {
	char **argv;
	int argc;
	int level;
	hkp_tree_t *tree;
	long lineno;
	node_t *parent, *next;
	node_t *first_child, *last_child;
};

struct hkp_tree_s {
	char *filename;
	node_t *root, *curr;
};

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
	pcb_coord_t dia; /* Should be zero when defining w,h slot */
	pcb_coord_t w,h; /* Slot width and height */
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
static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *nd, int on_bottom);
static hkp_pstk_t *parse_pstk(hkp_ctx_t *ctx, const char *ps);


/*** local ***/
static pcb_layer_t *parse_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, const char *ln, int user, node_t *err_node);

/*** High level parser ***/

static int hkp_error(node_t *nd, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	gds_append_str(&str, "io_mentor_cell ERROR");
	if (nd != NULL)
		pcb_append_printf(&str, " at %s:%d: ", nd->tree->filename, nd->lineno);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

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

/* split s and parse  it into (x,y,r) - modifies s */
static int parse_xyr(hkp_ctx_t *ctx, char *s, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t *r, int xform)
{
	char *sy, *sr;
	pcb_coord_t xx, yy, rr;
	pcb_bool suc1, suc2, suc3;

	sy = strchr(s, ',');
	if (sy == NULL)
		return -1;

	*sy = '\0';
	sy++;

	sr = strchr(sy, ',');
	if (sr == NULL)
		return -1;

	*sr = '\0';
	sr++;

	xx = pcb_get_value(s, ctx->unit->suffix, NULL, &suc1);
	yy = pcb_get_value(sy, ctx->unit->suffix, NULL, &suc2);
	rr = pcb_get_value(sr, ctx->unit->suffix, NULL, &suc3);

	if (xform) {
		yy = -yy;
	}
	
	*x = xx;
	*y = yy;
	*r = rr;

	return !(suc1 && suc2 && suc3);
}

static int parse_rot(hkp_ctx_t *ctx, node_t *nd, double *rot_out, int on_bottom)
{
	double rot;
	char *end;

	/* hkp rotation: top side positive value is CW; in pcb-rnd: that's negative */
	rot = -strtod(nd->argv[1], &end);
	if (*end != '\0')
		return hkp_error(nd, "Wrong rotation value '%s' (expected numeric)\n", nd->argv[1]);
	if ((rot < -360) || (rot > 360))
		return hkp_error(nd, "Wrong rotation value '%s' (out of range)\n", nd->argv[1]);
	if (on_bottom == 0)
		rot = -rot;
	*rot_out = rot;
	return 0;
}

static void d1() {}

#define DWG_LY(node, name, name2) \
	if (ly == NULL) { \
		node_t *__tmp__ = find_nth(node->first_child, name, 0), *__tmp2__ = NULL; \
		if ((__tmp__ == NULL) && (name2 != NULL)) \
			__tmp2__ = find_nth(node->first_child, name2, 0); \
		if (__tmp__ != NULL) { \
			ly = parse_layer(ctx, subc, __tmp__->argv[1], 0, __tmp__); \
			if (ly == NULL) { \
				hkp_error(__tmp__, "Invalid layer %s in %s\n", __tmp__->argv[1], name); \
				return; \
			} \
		} \
		else if (__tmp2__ != NULL) { \
			ly = parse_layer(ctx, subc, __tmp2__->argv[1], 1, __tmp2__); \
			if (ly == NULL) { \
				hkp_error(__tmp2__, "Invalid layer %s in %s\n", __tmp2__->argv[1], name2); \
				return; \
			} \
		}\
		else {\
			hkp_error(node, "Missing layer reference (%s)\n", name); \
			return; \
		} \
	}


#define DWG_REQ_LY(node) \
	if (ly == NULL) { \
		hkp_error(node, "Internal error: expected existing layer from the caller\n"); \
		return; \
	}

static int parse_dwg_path_polyline(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *pp)
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
	if (tmp == NULL)
		return hkp_error(pp, "Missing polyline XY, can't place via\n");
	parse_xy(ctx, tmp->argv[1], &px, &py, 1);
	for(n = 2; n < tmp->argc; n++) {
		parse_xy(ctx, tmp->argv[n], &x, &y, 1);
		if (pcb_line_new(ly, px, py, x, y, th, 0, pcb_no_flags()) == NULL)
			return hkp_error(pp, "Failed to create line for POLYLINE_PATH\n");
		px = x;
		py = y;
	}
	return 0;
}

static int parse_dwg_path_polyarc(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *pp)
{
	node_t *tmp;
	pcb_coord_t th = 1, sx, sy, cx, cy, r, ex, ey, dummy;
	pcb_coord_t srx, sry, erx, ery; /* relative x;y from the center for start and end */
	double sa, ea, da;
	int n;

	DWG_REQ_LY(pp);

	th = PCB_MM_TO_COORD(0.5);
	tmp = find_nth(pp->first_child, "WIDTH", 0);
	if (tmp != NULL)
		parse_coord(ctx, tmp->argv[1], &th);
	tmp = find_nth(pp->first_child, "XYR", 0);
	if (tmp == NULL)
		return hkp_error(pp, "Missing polyarc XYR, can't place arc\n");

	if (parse_xyr(ctx, tmp->argv[1], &sx, &sy, &dummy, 1) != 0)
		return hkp_error(pp, "Failed to parse polyarc XYR start point, can't place arc\n");
	if (parse_xyr(ctx, tmp->argv[2], &cx, &cy, &r, 1) != 0)
		return hkp_error(pp, "Failed to parse polyarc XYR center point, can't place arc\n");
	if (parse_xyr(ctx, tmp->argv[3], &ex, &ey, &dummy, 1) != 0)
		return hkp_error(pp, "Failed to parse polyarc XYR end point, can't place arc\n");

	srx = sx - cx; sry = sy - cy;
	erx = ex - cx; ery = ey - cy;
	sa = atan2(sry, srx) * PCB_RAD_TO_DEG - 90.0;
	ea = atan2(ery, erx) * PCB_RAD_TO_DEG - 90.0;
	da = ea-sa;
pcb_trace("arc: c-r=%mm;%mm;%mm ", cx, cy, r);

	if (r < 0) {
		r = -r;
	}
	else {
		if (da < 0)
			da = 360+da;
		else
			da = 360-da;
	}
pcb_trace(" sa=%f ea=%f ->da=%f\n", sa, ea, da);
	if (pcb_arc_new(ly, cx, cy, r, r, sa, da, th, 0, pcb_no_flags(), 0) == NULL)
		return hkp_error(pp, "Failed to create arc for POLYARC_PATH\n");

	return 0;
}

static void parse_dwg_rect(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *rp, int is_shape)
{
	node_t *tmp;
	pcb_coord_t th = 1, x1, y1, x2, y2;

	DWG_REQ_LY(rp);

	if (!is_shape) {
		tmp = find_nth(rp->first_child, "WIDTH", 0);
		if (tmp != NULL)
			parse_coord(ctx, tmp->argv[1], &th);
	}

TODO("check for FILLED, do a poly instead");

	tmp = find_nth(rp->first_child, "XY", 0);
	parse_xy(ctx, tmp->argv[1], &x1, &y1, 1);
	parse_xy(ctx, tmp->argv[2], &x2, &y2, 1);
	pcb_line_new(ly, x1, y1, x2, y1, th, 0, pcb_no_flags());
	pcb_line_new(ly, x2, y1, x2, y2, th, 0, pcb_no_flags());
	pcb_line_new(ly, x2, y2, x1, y2, th, 0, pcb_no_flags());
	pcb_line_new(ly, x1, y2, x1, y1, th, 0, pcb_no_flags());
}

static void parse_dwg_text(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *nt, int omit_on_silk, pcb_flag_values_t flg)
{
	node_t *attr, *tmp;
	pcb_coord_t tx, ty;
	double rot = 0;
	unsigned long mirrored = 0;

	attr = find_nth(nt->first_child, "DISPLAY_ATTR", 0);
	if (attr == NULL)
		return;

	DWG_LY(attr, "TEXT_LYR", "USER_LYR")

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
		parse_rot(ctx, tmp, &rot, (pcb_layer_flags_(ly) & PCB_LYT_BOTTOM));

	tmp = find_nth(attr->first_child, "TEXT_OPTIONS", 0);
	if (tmp != NULL)
		if (strcmp(tmp->argv[1], "MIRRORED") == 0) {
			mirrored = PCB_FLAG_ONSOLDER;
			rot = rot + 180;
			if (rot > 360) rot = rot - 360;
		}

TODO("we should compensate for HOTIZ_JUST and VERT_JUST but for that we need to figure how big the text is originally");
TODO("HEIGHT should become scale");
TODO("figure what TEXT_OPTIONS we have. One of them is MIRRORED (brd2 example)");
TODO("[easy] STROKE_WIDTH: we have support for that, but what's the unit? what if it is 0?. STROKE_WIDTH == 0 and != 0 in brd1 example");
/* STROKE_WIDTH is the thickness of the "pen" drawing the text. */
/*  Its units are mm or inches, whatever is used in the file at that moment */
/*  When it is 0, original SW draws the outline of the letters, with 0.005mm pen thickness.*/
/*  When drawing the outline of a vertical trace for '1' character, there are two thin traces.*/
/*  The distance between those traces seems to depend on text height.*/
/*  In brd2 example, it is 0.04mm for J1, assembly layer, and 0.02mm for R1.*/

	pcb_text_new(ly, pcb_font(ctx->pcb, 0, 0), tx, ty, rot, 100, 0, nt->argv[1], pcb_flag_make(flg | mirrored));
		

}

static void parse_dgw_via(hkp_ctx_t *ctx, node_t *nv)
{
	pcb_coord_t vx, vy;
	node_t *tmp;
	hkp_pstk_t *hps;
	pcb_cardinal_t pid;
	pcb_pstk_t *ps;

	tmp = find_nth(nv->first_child, "XY", 0);
	if (tmp == NULL) {
		hkp_error(nv, "Missing VIA XY, can't place via\n");
		return;
	}
	parse_x(ctx, tmp->argv[1], &vx);
	parse_y(ctx, tmp->argv[2], &vy);

	TODO("bbvia:");
/*
	tmp = find_nth(nv->first_child, "LAYER_PAIR", 0);
	if (tmp != NULL) {
	
	}
*/

	tmp = find_nth(nv->first_child, "VIA_OPTIONS", 0);
	if (tmp != NULL) {
		TODO("what's this?");
	}

	tmp = find_nth(nv->first_child, "PADSTACK", 0);
	if (tmp == NULL) {
		hkp_error(nv, "Missing VIA PADSTACK, can't place via\n"); \
		return;
	}
	hps = parse_pstk(ctx, tmp->argv[1]);
	if (hps == NULL) {
		hkp_error(nv, "Unknown VIA PADSTACK '%s', can't place via\n", tmp->argv[1]);
		return;
	}

	pid = pcb_pstk_proto_insert_dup(ctx->pcb->Data, &hps->proto, 1);
	ps = pcb_pstk_alloc(ctx->pcb->Data);
	ps->x = vx;
	ps->y = vy;
	ps->proto = pid;
	pcb_pstk_add(ctx->pcb->Data, ps);
}


/* Returns number of objects drawn: 0 or 1 */
static int parse_dwg(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, node_t *n)
{
	if (strcmp(n->argv[0], "POLYLINE_PATH") == 0)
		parse_dwg_path_polyline(ctx, subc, ly, n);
	if (strcmp(n->argv[0], "POLYARC_PATH") == 0)
		parse_dwg_path_polyarc(ctx, subc, ly, n);
	else if ((strcmp(n->argv[0], "RECT_PATH") == 0) || (strcmp(n->argv[0], "RECT_SHAPE") == 0))
		parse_dwg_rect(ctx, subc, ly, n, n->argv[0][5] == 'S');
	else if ((strcmp(n->argv[0], "TEXT") == 0) && (subc == NULL))
		parse_dwg_text(ctx, subc, ly, n, 0, 0);
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

/* Returns number of objects drawn: 0 or more for valid layers, -1 for invalid layer */
static long parse_dwg_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, node_t *n)
{
	node_t *tmp;
	pcb_layer_type_t type = 0;
	pcb_layer_type_t side = PCB_LYT_TOP;
	pcb_layer_combining_t lyc = 0;
	const char *lyname = NULL, *purpose = NULL;
	pcb_layer_t *ly;


	if (strcmp(n->argv[0], "SILKSCREEN_OUTLINE") == 0) {
		type = PCB_LYT_SILK;
		lyc = PCB_LYC_AUTO;
		tmp = find_nth(n->first_child, "SIDE", 0);
		if (tmp != NULL) {
			if ((strcmp(tmp->argv[1], "MNT_SIDE") == 0) || (strcmp(tmp->argv[1], "TOP") == 0)) {
				side = PCB_LYT_TOP;
				lyname = "top-silk";
			}
			else {
				side = PCB_LYT_BOTTOM;
				lyname = "bot-silk";
			}
		}
	}
	else if (strcmp(n->argv[0], "SOLDER_MASK") == 0) {
		type = PCB_LYT_MASK;
		lyc = PCB_LYC_AUTO;
		tmp = find_nth(n->first_child, "SIDE", 0);
		if (tmp != NULL) {
			if ((strcmp(tmp->argv[1], "MNT_SIDE") == 0) || (strcmp(tmp->argv[1], "TOP") == 0)) {
				side = PCB_LYT_TOP;
				lyname = "top-mask";
			}
			else {
				side = PCB_LYT_BOTTOM;
				lyname = "bot-mask";
			}
		}
	}
	else if (strcmp(n->argv[0], "SOLDER_PASTE") == 0) {
		type = PCB_LYT_PASTE;
		lyc = PCB_LYC_AUTO;
		tmp = find_nth(n->first_child, "SIDE", 0);
		if (tmp != NULL) {
			if ((strcmp(tmp->argv[1], "MNT_SIDE") == 0) || (strcmp(tmp->argv[1], "TOP") == 0)) {
				side = PCB_LYT_TOP;
				lyname = "top-paste";
			}
			else {
				side = PCB_LYT_BOTTOM;
				lyname = "bot-paste";
			}
		}
	}
	else if (strcmp(n->argv[0], "ASSEMBLY_OUTLINE") == 0) {
		type = PCB_LYT_DOC;
		lyc = PCB_LYC_AUTO;
		tmp = find_nth(n->first_child, "SIDE", 0);
		purpose = "assy";
		if (tmp != NULL) {
			if ((strcmp(tmp->argv[1], "MNT_SIDE") == 0) || (strcmp(tmp->argv[1], "TOP") == 0)) {
				side = PCB_LYT_TOP;
				lyname = "top_assy";
			}
			else {
				side = PCB_LYT_BOTTOM;
				lyname = "bottom_assy";
			}
		}
	}
	else if (strcmp(n->argv[0], "ROUTE_OUTLINE") == 0) {
		type = PCB_LYT_BOUNDARY;
		side = 0; /* global */
		lyname = "outline";
		purpose = "uroute";
	}
	else
		return -1;

	if (subc == NULL) {
		pcb_layer_id_t lid;
		if (purpose == NULL) {
			if (pcb_layer_list(ctx->pcb, side | type, &lid, 1) != 1)
				return 0;
		}
		else {
			if (pcb_layer_listp(ctx->pcb, side | type, &lid, 1, -1, purpose) != 1)
				return 0;
		}
		ly = &ctx->pcb->Data->Layer[lid];
	}
	else
		ly = pcb_subc_get_layer(subc, side | type, lyc, 1, lyname, 0);
	return parse_dwg_all(ctx, subc, ly, n);
}

static pcb_layer_t *parse_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, const char *ln, int user, node_t *err_node)
{
	pcb_layer_t *ly;
	pcb_layer_type_t lyt = 0;
	pcb_layer_combining_t lyc = 0;
	char *purpose = NULL;
	char *name = NULL;

	if (user) { /* user layers are DOC layers identified by name and are allocated on the go */
		pcb_layer_id_t lid;
		pcb_layergrp_t *grp;
		pcb_dflgmap_t map[2];
		int retry;

TODO("this should be done only when subc == NULL");

		for(retry = 0; retry < 2; retry++) {
			lid = pcb_layer_by_name(ctx->pcb->Data, ln);
			if (lid >= 0)
				return &ctx->pcb->Data->Layer[lid];

			memset(map, 0, sizeof(map));
			map[0].name = ln;
			map[0].lyt = PCB_LYT_DOC;
			map[0].flags = PCB_DFLGMAP_FORCE_END;
			pcb_layergrp_create_by_map(ctx->pcb, map);
		}
		return NULL;
	}

	if (strncmp(ln, "LYR_", 4) == 0) {
		int cidx;
		char *end;
		pcb_layergrp_id_t gid;

		TODO("can a subcircuit have intern copper objects? assume NO for now:");
		assert(subc == NULL);
		cidx = strtol(ln+4, &end, 10);
		if (*end != '\0') {
			hkp_error(err_node, "Unknown layer '%s' (expected integer after LYR_)\n", ln);
			return NULL;
		}
		cidx--; /* hkp numbers from 1, we number from 0 */
		if ((cidx < 0) || (cidx >= ctx->num_cop_layers)) {
			hkp_error(err_node, "Layer '%s' is out of range (1..%d)\n", ln, ctx->num_cop_layers);
			return NULL;
		}
		gid = pcb_layergrp_step(ctx->pcb, pcb_layergrp_get_top_copper(), cidx, PCB_LYT_COPPER);
		if (gid < 0) {
			hkp_error(err_node, "Layer '%s' is out of range (copper layer group not found)\n", ln);
			return NULL;
		}
		assert(ctx->pcb->LayerGroups.grp[gid].len > 0);
		return &ctx->pcb->Data->Layer[ctx->pcb->LayerGroups.grp[gid].lid[0]];
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
		hkp_error(err_node, "Unknown layer '%s'\n", ln);
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

	parse_dwg_text(ctx, subc, NULL, textnode, omit_on_silk, flg);
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
				hkp_error(n, "Can't load package: broken placement XY coord\n");
				return NULL;
			}
			seen_oxy = 1;
		}
		else if (strcmp(n->argv[0], "ROTATION") == 0) {
			if (parse_rot(ctx, n, &rot, on_bottom) != 0) {
				hkp_error(n, "Can't load package due to wrong rotation value\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "FACEMENT") == 0) {
			if (strcmp(n->argv[1], "TOP") == 0) on_bottom = 0;
			else if (strcmp(n->argv[1], "BOTTOM") == 0) on_bottom = 1;
			else {
				hkp_error(n, "Can't load package: broken facement (should be TOP or BOTTOM)\n");
				return NULL;
			}
		}
		else if (strcmp(n->argv[0], "TEXT") == 0) {
			parse_subc_text(ctx, subc, n);
		}
	}

	if (!seen_oxy) {
		pcb_subc_free(subc);
		hkp_error(nd, "Can't load package: no placement XY coord\n");
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
			parse_pin(ctx, subc, n, on_bottom);
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

static void parse_net(hkp_ctx_t *ctx, node_t *netroot)
{
	node_t *n, *lyn;
	pcb_layer_t *ly;
	const char *netname = netroot->argv[1];

	if (strcmp(netname, "Unconnected_Net") != 0)
		pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], netname, 1);

	for(n = netroot->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "CONDUCTIVE_AREA") == 0) {
			TODO("poly!");
		}
		else if (strcmp(n->argv[0], "TRACE") == 0) {
			lyn = find_nth(n->first_child, "ROUTE_LYR", 0);
			if (lyn == NULL)
				continue;
			ly = parse_layer(ctx, NULL, lyn->argv[1], 0, lyn);
			if (ly == NULL) {
				hkp_error(lyn, "Unknown trace layer '%s'\n", lyn->argv[1]);
				continue;
			}
			parse_dwg_all(ctx, NULL, ly, n);
		}
		else if (strcmp(n->argv[0], "VIA") == 0)
			parse_dgw_via(ctx, n);
	}
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
				return hkp_error(n, "Invalid number of layers '%s' (expected integer)\n", n->argv[1]);
			}
			if ((ctx->num_cop_layers < 1) || (ctx->num_cop_layers > ((PCB_MAX_LAYER/2)-8))) {
				return hkp_error(n, "Invalid number of layers '%s' (out of range)\n", n->argv[1]);
			}
		}
		else if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->unit = parse_units(n->argv[1]);
			if (ctx->unit == NULL) {
				return hkp_error(n, "Unknown unit '%s'\n", n->argv[1]);
			}
		}
	}

	if (ctx->num_cop_layers < 0) {
		return hkp_error(tree->root, "Missing NUMBER_OF_LAYERS\n");
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
		if (strcmp(n->argv[0], "NET") == 0)
			parse_net(ctx, n);
		else { /* global drawing objects in layers or outside of layers */
			parse_dwg(ctx, NULL, NULL, n);
			parse_dwg_layer(ctx, NULL, n);
		}

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

static void node_destroy(node_t *nd)
{
	node_t *n, *next;

	if (nd == NULL)
		return;

	qparse_free(nd->argc, &nd->argv);

	for(n = nd->first_child; n != NULL; n = next) {
		next = n->next;
		node_destroy(n);
	}

	free(nd);
}

static void tree_destroy(hkp_tree_t *tree)
{
	node_destroy(tree->root);
	free(tree->filename);
	tree->root = NULL;
	tree->filename = NULL;
}

/* Split up a virtual line and save it in the tree */
void save_vline(hkp_tree_t *tree, char *vline, int level, long lineno)
{
	node_t *nd;

	nd = calloc(sizeof(node_t), 1);
	nd->argc = qparse2(vline, &nd->argv, QPARSE_DOUBLE_QUOTE | QPARSE_PAREN | QPARSE_MULTISEP);
	nd->level = level;
	nd->lineno = lineno;
	nd->tree = tree;

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

static void load_hkp(hkp_tree_t *tree, FILE *f, const char *fn)
{
	char *s, line[1024];
	gds_t vline;
	int level;
	long lineno = 0;

	tree->filename = pcb_strdup(fn);
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
				save_vline(tree, vline.array, level, lineno);
				gds_truncate(&vline, 0);
			}
			level = 0;
			while(*s == '.') { s++; level++; };
		}

		if (gds_len(&vline) > 0)
			gds_append(&vline, ' ');
		gds_append_str(&vline, s);
		lineno++;
	}

	/* the last virtual line before eof */
	if (gds_len(&vline) > 0) {
		rtrim(&vline);
		save_vline(tree, vline.array, level, lineno);
	}
	gds_uninit(&vline);

}

static int io_mentor_cell_netlist(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fnet;
	node_t *p, *nnet, *pinsect;
	hkp_tree_t net_tree; /* no need to keep the tree in ctx, no data is needed after the function returns */

	fnet = pcb_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fnet == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open netprops hkp '%s' for read\n", fn);
		return -1;
	}

	load_hkp(&net_tree, fnet, fn);
	fclose(fnet);

	for(nnet = net_tree.root->first_child; nnet != NULL; nnet = nnet->next) {
		if (strcmp(nnet->argv[0], "NETNAME") == 0) {
			pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], nnet->argv[1], 1);
			if (net == NULL) {
				hkp_error(nnet, "Failed to create net '%s' - netlist will be incomplete\n", nnet->argv[1]);
				continue;
			}
			pinsect = find_nth(nnet->first_child, "PIN_SECTION", 0);
			if (pinsect == NULL)
				continue;
			for(p = pinsect->first_child; p != NULL; p = p->next) {
				pcb_net_term_t *term;
				if (strcmp(p->argv[0], "REF_PINNAME") != 0)
					continue;
				term = pcb_net_term_get_by_pinname(net, p->argv[1], 1);
				if (net == NULL)
					hkp_error(p, "Failed to create pin '%s' in net '%s' - netlist will be incomplete\n", p->argv[1], nnet->argv[1]);
			}
		}
	}


	tree_destroy(&net_tree);
	return 0;
}


#include "read_pstk.c"

int io_mentor_cell_read_pcb(pcb_plug_io_t *pctx, pcb_board_t *pcb, const char *fn, conf_role_t settings_dest)
{
	hkp_ctx_t ctx;
	int res = -1;
	FILE *flay;
	char *end, fn2[PCB_PATH_MAX];

	pcb_data_clip_inhibit_inc(pcb->Data);
	pcb_layergrp_inhibit_inc();

	memset(&ctx, 0, sizeof(ctx));

	flay = pcb_fopen(&pcb->hidlib, fn, "r");
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

	strcpy(end, "NetProps.hkp");
	if (io_mentor_cell_netlist(&ctx, fn2) != 0) {
		fclose(flay);
		goto err;
	}


	load_hkp(&ctx.layout, flay, fn);
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
	tree_destroy(&ctx.padstacks);
	tree_destroy(&ctx.layout);

	pcb_layergrp_inhibit_dec();
	pcb_data_clip_inhibit_dec(pcb->Data, 1);

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
