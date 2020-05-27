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
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
#include "netlist.h"

#include "obj_subc.h"

#define DEFAULT_OBJ_FLAG    pcb_flag_make(PCB_FLAG_CLEARLINE)
#define DEFAULT_POLY_FLAG   pcb_flag_make(PCB_FLAG_CLEARPOLY)

#define ltrim(s) while(isspace(*s)) (s)++

#include "../src_plugins/lib_compat_help/pstk_help.h"

#include "font.c"

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
	const rnd_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	pcb_pstk_shape_t shp;
} hkp_shape_t;

typedef struct {
	const rnd_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	unsigned plated:1;
	rnd_coord_t dia; /* Should be zero when defining w,h slot */
	rnd_coord_t w,h; /* Slot width and height */
} hkp_hole_t;

typedef struct {
	const rnd_unit_t *unit;
	node_t *subtree;
	unsigned valid:1; /* whether it's already parsed */
	pcb_pstk_proto_t proto;
} hkp_pstk_t;

typedef enum {
	HKP_CLR_POLY2TRACE,
	HKP_CLR_POLY2TERM,
	HKP_CLR_POLY2VIA,
	HKP_CLR_POLY2POLY,
	HKP_CLR_max
} hkp_clearance_type_t;

typedef struct {
	rnd_coord_t clearance[PCB_MAX_LAYER][HKP_CLR_max];
} hkp_netclass_t;

typedef struct {
	pcb_board_t *pcb;

	int num_cop_layers;

	const rnd_unit_t *unit;      /* default unit used while converting coords any given time */
	const rnd_unit_t *pstk_unit; /* default unit for the padstacks file */

	hkp_netclass_t nc_dflt; /* for default clearances */

	htsp_t shapes;   /* name -> hkp_shape_t */
	htsp_t holes;    /* name -> hkp_hole_t */
	htsp_t pstks;    /* name -> hkp_pstk_t */
	hkp_tree_t layout, padstacks;
} hkp_ctx_t;

/*** read_pstk.c ***/
static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, const hkp_netclass_t *nc, node_t *nd, int on_bottom);
static hkp_pstk_t *parse_pstk(hkp_ctx_t *ctx, const char *ps);
static void set_pstk_clearance(hkp_ctx_t *ctx, const hkp_netclass_t *nc, pcb_pstk_t *ps, node_t *errnd);

/*** read_net.c ***/
static rnd_coord_t net_get_clearance(hkp_ctx_t *ctx, pcb_layer_t *ly, const hkp_netclass_t *nc, hkp_clearance_type_t type, node_t *errnode);

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
		rnd_append_printf(&str, " at %s:%d: ", nd->tree->filename, nd->lineno);
	else
		gds_append_str(&str, ": ");

	va_start(ap, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, ap);
	va_end(ap);

	rnd_message(RND_MSG_ERROR, "%s", str.array);

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
static int parse_coord(hkp_ctx_t *ctx, char *s, rnd_coord_t *crd)
{
	char *end;
	rnd_bool suc;

	end = strchr(s, ',');
	if (end != NULL)
		*end = '\0';

	*crd = rnd_get_value(s, ctx->unit->suffix, NULL, &suc);
	return !suc;
}

static int parse_x(hkp_ctx_t *ctx, char *s, rnd_coord_t *crd)
{
	return parse_coord(ctx, s, crd);
}


static int parse_y(hkp_ctx_t *ctx, char *s, rnd_coord_t *crd)
{
	rnd_coord_t tmp;
	if (parse_coord(ctx, s, &tmp) != 0)
		return -1;
	*crd = -tmp;
	return 0;
}


/* split s and parse  it into (x,y) - modifies s */
static int parse_xy(hkp_ctx_t *ctx, char *s, rnd_coord_t *x, rnd_coord_t *y, int xform)
{
	char *sy;
	rnd_coord_t xx, yy;
	rnd_bool suc1, suc2;

        if (s == NULL)
                return -1;
	sy = strchr(s, ',');
	if (sy == NULL)
		return -1;

	*sy = '\0';
	sy++;


	xx = rnd_get_value(s, ctx->unit->suffix, NULL, &suc1);
	yy = rnd_get_value(sy, ctx->unit->suffix, NULL, &suc2);

	if (xform)
		yy = -yy;

	*x = xx;
	*y = yy;

	return !(suc1 && suc2);
}

/* split s and parse  it into (x,y,r) - modifies s */
static int parse_xyr(hkp_ctx_t *ctx, char *s, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t *r, int xform)
{
	char *sy, *sr;
	rnd_coord_t xx, yy, rr;
	rnd_bool suc1, suc2, suc3;

	if (s == NULL)
		return -1;

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

	xx = rnd_get_value(s, ctx->unit->suffix, NULL, &suc1);
	yy = rnd_get_value(sy, ctx->unit->suffix, NULL, &suc2);
	rr = rnd_get_value(sr, ctx->unit->suffix, NULL, &suc3);

	if (xform)
		yy = -yy;

	*x = xx;
	*y = yy;
	*r = rr;

	if (suc1 == rnd_false)
		hkp_error(NULL, "parse_xyr error parsing %s.\n", s);
	if (suc2 == rnd_false)
		hkp_error(NULL, "parse_xyr error parsing %s.\n", sy);
	if (suc3 == rnd_false)
		hkp_error(NULL, "parse_xyr error parsing %s.\n", sr);
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
		return -1; \
	}

static int parse_dwg_path_polyline(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *pp, int is_shape)
{
	node_t *tmp;
	rnd_coord_t th = 1, px, py, x, y;
	int n, filled = 0;

	DWG_REQ_LY(pp);

	if (is_shape) {
		tmp = find_nth(pp->first_child, "SHAPE_OPTIONS", 0);
		if (tmp != NULL)
			filled = (strcmp(tmp->argv[1], "FILLED") == 0);
	}
	else {
		th = RND_MM_TO_COORD(0.5);
		tmp = find_nth(pp->first_child, "WIDTH", 0);
		if (tmp != NULL)
			parse_coord(ctx, tmp->argv[1], &th);
	}

	tmp = find_nth(pp->first_child, "XY", 0);
	if (tmp == NULL)
		return hkp_error(pp, "Missing polyline XY, can't place via\n");

	if (filled) { /* filled = polygon */
		pcb_poly_t *poly = pcb_poly_new(ly, 0, DEFAULT_POLY_FLAG);
		for(n = 1; n < tmp->argc; n++) {
			if (parse_xy(ctx, tmp->argv[n], &x, &y, 1) != 0) {
				pcb_poly_free(poly);
				return hkp_error(pp, "Failed to parse filled polygon point (%s), can't place polygon\n", tmp->argv[n]);
			}
			pcb_poly_point_new(poly, x, y);
		}
		pcb_add_poly_on_layer(ly, poly);
		if (subc == NULL)
			pcb_poly_init_clip(ctx->pcb->Data, ly, poly);
	}
	else { /* "polyline" = a bunch of line objects */
		rnd_coord_t cl = net_get_clearance(ctx, ly, nc, HKP_CLR_POLY2TRACE, tmp) * 2;
		if (parse_xy(ctx, tmp->argv[1], &px, &py, 1) != 0)
			return hkp_error(pp, "Failed to parse polyline start point (%s), can't place polygon\n", tmp->argv[1]);
		for(n = 2; n < tmp->argc; n++) {
			if (parse_xy(ctx, tmp->argv[n], &x, &y, 1) != 0)
				return hkp_error(pp, "Failed to parse polyline point (%s), can't place polygon\n", tmp->argv[n]);
			if (pcb_line_new(ly, px, py, x, y, th, cl, DEFAULT_OBJ_FLAG) == NULL)
				return hkp_error(pp, "Failed to create line for POLYLINE_PATH\n");
			px = x;
			py = y;
		}
	}
	return 0;
}

static void convert_arc(rnd_coord_t sx, rnd_coord_t sy, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t ex, rnd_coord_t ey, rnd_coord_t *r, double *sa, double *da)
{
	/* In pcb-rnd, start angle 0 is towards the left of the screen (-x direction)*/
	/* da > 0 is counterclockwise */
	/* In HKP format, r<0 means counterclockwise, r>0 means clockwise */
	double ea;
	rnd_coord_t srx, sry, erx, ery; /* relative x;y from the center for start and end */

	srx = -(sx - cx); sry = sy - cy; /* Since angle = 0 is towards -x, change sign to x part */
	erx = -(ex - cx); ery = ey - cy; /* Since angle = 0 is towards -x, change sign to x part */
	*sa = atan2(sry, srx) * RND_RAD_TO_DEG;
	*sa = rnd_normalize_angle(360 + *sa); /* normalize angle between 0 and 359 */
	ea = atan2(ery, erx) * RND_RAD_TO_DEG;
	ea = rnd_normalize_angle(360 + ea); /* normalize angle between 0 and 359 */

	if (*r < 0) {
		/* counterclockwise */
		*r = -(*r);
		if (*sa < ea)
			*da = ea - *sa;
		else
			*da = (360 - *sa) + ea;
	}
	else {
		/* clockwise */
		if (*sa > ea)
			*da = *sa - ea;
		else
			*da = *sa + (360 - ea);
		*da = -*da;
	}
}

static int parse_dwg_path_polyarc(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *pp, int is_shape)
{
	node_t *tmp;
	rnd_coord_t th = 1, r, ex, ey, dummy, x, y, px, py;
	double sa, da;
	int n, filled = 0;
	rnd_coord_t cl;

	DWG_REQ_LY(pp);


	if (is_shape) {
		tmp = find_nth(pp->first_child, "SHAPE_OPTIONS", 0);
		if (tmp != NULL)
			filled = (strcmp(tmp->argv[1], "FILLED") == 0);
	}
	else {
		th = RND_MM_TO_COORD(0.5);
		tmp = find_nth(pp->first_child, "WIDTH", 0);
		if (tmp != NULL)
			parse_coord(ctx, tmp->argv[1], &th);
	}

	tmp = find_nth(pp->first_child, "XYR", 0);
	if (tmp == NULL)
		return hkp_error(pp, "Missing polyarc XYR, can't place arc\n");


	if (parse_xyr(ctx, tmp->argv[1], &px, &py, &r, 1) != 0)
		return hkp_error(pp, "Failed to parse polyarc XYR start point, can't place polyarc\n");
	if (r != 0)
		return hkp_error(pp, "Failed to parse polyarc XYR start point (r must be zero), can't place polyarc\n");

	cl = net_get_clearance(ctx, ly, nc, HKP_CLR_POLY2TRACE, tmp) * 2;

	for(n = 2; n < tmp->argc; n++) {
		if (parse_xyr(ctx, tmp->argv[n], &x, &y, &r, 1) != 0)
			return hkp_error(pp, "Failed to parse %dth polyarc XYR point (%s), can't place polyarc\n", n, tmp->argv[n]);
		if (r != 0) { /* arc: px;py=start, x;y=center, ex;ey=end */
			n++;
			if (parse_xyr(ctx, tmp->argv[n], &ex, &ey, &dummy, 1) != 0)
				return hkp_error(pp, "Failed to parse %dth polyarc XYR point (arc end) (%s), can't place polyarc\n", n, tmp->argv[n]);
			if (dummy != 0)
				return hkp_error(pp, "Failed to parse %dth polyarc XYR point (r must be zero), can't place polyarc\n", n);

			convert_arc(px, py, x, y, ex, ey, &r, &sa, &da);

			if (pcb_arc_new(ly, x, y, r, r, sa, da, th, cl, DEFAULT_OBJ_FLAG, 0) == NULL)
				return hkp_error(pp, "Failed to create arc for POLYARC_PATH\n");

			px = ex; py = ey;
		}
		else { /* plain old line: px;py=start, x;y=end */
			if (pcb_line_new(ly, px, py, x, y, th, cl, DEFAULT_OBJ_FLAG) == NULL)
				return hkp_error(pp, "Failed to create line for POLYARC_PATH\n");
			px = x; py = y;
		}
	}

	return 0;
}

static int parse_dwg_rect(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *rp, int is_shape)
{
	node_t *tmp;
	rnd_coord_t th = 1, x1, y1, x2, y2;
	int filled = 0;

	DWG_REQ_LY(rp);

	if (is_shape) {
		tmp = find_nth(rp->first_child, "SHAPE_OPTIONS", 0);
		if (tmp != NULL)
			filled = (strcmp(tmp->argv[1], "FILLED") == 0);
	}
	else {
		tmp = find_nth(rp->first_child, "WIDTH", 0);
		if (tmp != NULL)
			parse_coord(ctx, tmp->argv[1], &th);
	}

	tmp = find_nth(rp->first_child, "XY", 0);
	if (parse_xy(ctx, tmp->argv[1], &x1, &y1, 1) != 0)
		return hkp_error(tmp, "Failed to parse rect start point (%s), can't place rectangle\n", tmp->argv[1]);
	if (parse_xy(ctx, tmp->argv[2], &x2, &y2, 1) != 0)
		return hkp_error(tmp, "Failed to parse rect end point (%s), can't place rectangle\n", tmp->argv[2]);
	if (filled) {
		rnd_coord_t cl = net_get_clearance(ctx, ly, nc, HKP_CLR_POLY2POLY, tmp) * 2;
TODO("when to generate a rounded corner?");
		pcb_poly_new_from_rectangle(ly, x1, y1, x2, y2, cl, DEFAULT_POLY_FLAG);
	}
	else {
		rnd_coord_t cl = net_get_clearance(ctx, ly, nc, HKP_CLR_POLY2TRACE, tmp) * 2;
		pcb_line_new(ly, x1, y1, x2, y1, th, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, x2, y1, x2, y2, th, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, x2, y2, x1, y2, th, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, x1, y2, x1, y1, th, cl, DEFAULT_OBJ_FLAG);
	}
	return 0;
}

static void parse_dwg_text(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *nt, int omit_on_silk, pcb_flag_values_t flg)
{
	node_t *attr, *tmp;
	rnd_coord_t tx, ty, h, thickness = 0, width = 0, height = 0, ymin = 0;
	rnd_coord_t anchx = 0, anchy = 0;
	double rot = 0;
	unsigned long mirrored = 0;
	long int font_id = 0;
	char *tmp_char;
	font_size_res_t result;

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
	else {
		hkp_error(attr, "Can not find XY position of text. Text will be NOT rendered.\n");
		return;
	}
	tmp = find_nth(attr->first_child, "ROTATION", 0);
	if (tmp != NULL) {
		parse_rot(ctx, tmp, &rot, (pcb_layer_flags_(ly) & PCB_LYT_BOTTOM));
/*		rotbb = rot; remove bbox */
	}
	else {
		hkp_error(attr, "Can not find rotation of text. Text will be NOT rendered.\n");
		return;
	}

	tmp = find_nth(attr->first_child, "TEXT_OPTIONS", 0);
	if (tmp != NULL)
		if (strcmp(tmp->argv[1], "MIRRORED") == 0) {
			mirrored = PCB_FLAG_ONSOLDER;
			rot = rot + 180;
			if (rot > 360) rot = rot - 360;
		}

	tmp = find_nth(attr->first_child, "HEIGHT", 0);
	if (tmp != NULL)
		parse_x(ctx, tmp->argv[1], &h);
	else {
		hkp_error(attr, "Can not find height of text. Text will be NOT rendered.\n");
		return;
	}

	tmp = find_nth(attr->first_child, "STROKE_WIDTH", 0);
	if (tmp != NULL)
		parse_x(ctx, tmp->argv[1], &thickness);
	else {
		hkp_error(attr, "Can not find thickness (STROKE WIDTH) of text. Text will be NOT rendered.\n");
		return;
	}

	tmp = find_nth(attr->first_child, "FONT", 0);
	if (tmp != NULL) {
		if (rnd_strncasecmp(tmp->argv[1], "VeriBest Gerber ", 16) != 0)
			hkp_error(tmp, "Unknown font (%s). Text will be rendered, but it may not have a correct size.\n", tmp->argv[1]);
		font_id = strtol(tmp->argv[1]+16, &tmp_char, 10);
		if (*tmp_char != '\0')
			hkp_error(tmp, "Unparsed text (%s) after font ID. Text will be rendered, but it may not have a correct size.\n", tmp_char);
	}
	else {
		hkp_error(attr, "Can not find font of text. Text will be NOT rendered.\n");
		return;
	}

	result = font_text_nominal_size(font_id, nt->argv[1], &width, &height, &ymin);
	if (result == FSR_INVALID_GLYPH_ID)
			hkp_error(tmp, "Invalid glyph ID. Text will be rendered, but it may not have a correct size.\n");
	else if (result == FSR_INVALID_FONT_ID) {
		hkp_error(tmp, "Invalid font ID. Text will be NOT rendered.\n");
		return;
	}

	width = width * RND_COORD_TO_MM(h) + thickness;
	height = height * RND_COORD_TO_MM(h) + thickness;
	ymin = ymin * RND_COORD_TO_MM(h);

	tmp = find_nth(attr->first_child, "HORZ_JUST", 0);
	if (tmp != NULL) {
/*		xc = tx; remove bbox */
		if (strcmp(tmp->argv[1], "Left") == 0) {
/*			x1 = tx; remove bbox */
			anchx = 0;
/*			if (mirrored == 0)
				x2 = tx+width;
			else
				x2 = tx-width; remove bbox */
		}
		else if (strcmp(tmp->argv[1], "Center") == 0) {
/*			x1 = tx - (width >> 1); remove bbox*/
			anchx = width >> 1;
/*			if (mirrored == 0)
				x2 = tx + (width >> 1);
			else
				x2 = tx - (width >> 1); remove bbox*/
		}
		else if (strcmp(tmp->argv[1], "Right") == 0) {
/*			x2 = tx; remove bbox */
			anchx = width;
/*			if (mirrored == 0)
				x1 = tx-width;
			else
				x1 = tx+width;*/
		}
		else
			hkp_error(tmp, "Unknown horizontal alignment (%s). Text will be rendered, but it may not have a correct size.\n", tmp->argv[1]);
	}
	else {
		hkp_error(attr, "Can not find horizontal justification of text. Text will be NOT rendered.\n");
		return;
	}

	tmp = find_nth(attr->first_child, "VERT_JUST", 0);
	if (tmp != NULL) {
/*		yc = ty; remove bbox*/
		rnd_coord_t ymax = height+ymin;
		TODO(
			"Consider rotation, using:"
			"  rnd_rotate(rnd_coord_t * x, rnd_coord_t * y, rnd_coord_t cx, rnd_coord_t cy, double cosa, double sina)"
			"Maybe:"
			"  double sina = sin(-(double)rot / RND_RAD_TO_DEG), cosa = cos(-(double)rot / RND_RAD_TO_DEG);");
		if (strcmp(tmp->argv[1], "Top") == 0) {
			anchy = 0;
/*			y1 = ty+height; y2 = ty; remove bbox */
		}
		else if (strcmp(tmp->argv[1], "Center") == 0) {
/*			y1 = ty - ymin + (ymax >> 1); y2 = ty - (ymax >> 1); remove bbox*/
			anchy = ymax >> 1;
		}
		else if (strcmp(tmp->argv[1], "Bottom") == 0) {
/*			y1=ty-ymin; y2=ty-ymin-height; remove bbox; ymin is negative */
			anchy = height+ymin; /* ymin is negative */
		}
		else
			hkp_error(tmp, "Unknown horizontal alignment (%s). Text will be rendered, but it may not have a correct size.\n", tmp->argv[1]);
	}
	else {
		hkp_error(attr, "Can not find vertical justification of text. Text will be NOT rendered.\n");
		return;
	}

#if 0
before enabling this, fix the code: do not ever draw debug on board layers;
if you want to have debug draws, create a dedicated UI layer
	{
		double sina = 0, cosa = 0, rotbb = 0;
		rnd_coord_t xc = 0, yc = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
		rnd_coord_t cl = net_get_clearance(ctx, ly, nc, HKP_CLR_POLY2TRACE, tmp) * 2;

		TODO("Remove this block after checking text bounding box calculations");
		TODO("Use an UI layer for this.  UI layer API in src/layer_ui.h");
		TODO("Use pcb_text_new_by_bbox() in src/obj_text.h ");
		/* Rotate the points around center (xc,yc) */
		sina = sin((double)rotbb / RND_RAD_TO_DEG);
		cosa = cos((double)rotbb / RND_RAD_TO_DEG);
		{
		rnd_coord_t xaux1 = x2, yaux1 = y1;
		rnd_coord_t xaux3 = x1, yaux3 = y2;
		/* This handles bbox rotation. */
		rnd_rotate(&xaux1, &yaux1, xc, yc, cosa, sina);
		rnd_rotate(&xaux3, &yaux3, xc, yc, cosa, sina);
		rnd_rotate(&x1, &y1, xc, yc, cosa, sina);
		rnd_rotate(&x2, &y2, xc, yc, cosa, sina);

		pcb_line_new(ly, x1, y1, xaux1, yaux1, thickness, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, xaux1, yaux1, x2, y2, thickness, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, x2, y2, xaux3, yaux3, thickness, cl, DEFAULT_OBJ_FLAG);
		pcb_line_new(ly, xaux3, yaux3, x1, y1, thickness, cl, DEFAULT_OBJ_FLAG);
		}
	}
#endif

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

	pcb_text_new_by_bbox(ly, pcb_font(ctx->pcb, 0, 0), tx, ty, width, height, anchx, anchy, 1, 100, rot, thickness, nt->argv[1], pcb_flag_make(flg | mirrored));
}

static void parse_dgw_via(hkp_ctx_t *ctx, const hkp_netclass_t *nc, node_t *nv)
{
	rnd_coord_t vx, vy;
	node_t *tmp;
	hkp_pstk_t *hps;
	rnd_cardinal_t pid;
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

	pid = pcb_pstk_proto_insert_dup(ctx->pcb->Data, &hps->proto, 1, 0);
	ps = pcb_pstk_alloc(ctx->pcb->Data);
	ps->Flags = DEFAULT_OBJ_FLAG;
	ps->x = vx;
	ps->y = vy;
	ps->proto = pid;
	pcb_pstk_add(ctx->pcb->Data, ps);
	set_pstk_clearance(ctx, nc, ps, nv);
}


/* Returns number of objects drawn: 0 or 1 */
static int parse_dwg(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *n)
{
	if ((strcmp(n->argv[0], "POLYLINE_PATH") == 0) || (strcmp(n->argv[0], "POLYLINE_SHAPE") == 0))
		parse_dwg_path_polyline(ctx, subc, ly, nc, n, n->argv[0][9] == 'S');
	if ((strcmp(n->argv[0], "POLYARC_PATH") == 0) || (strcmp(n->argv[0], "POLYARC_SHAPE") == 0))
		parse_dwg_path_polyarc(ctx, subc, ly, nc, n, n->argv[0][8] == 'S');
	else if ((strcmp(n->argv[0], "RECT_PATH") == 0) || (strcmp(n->argv[0], "RECT_SHAPE") == 0))
		parse_dwg_rect(ctx, subc, ly, nc, n, n->argv[0][5] == 'S');
	else if ((strcmp(n->argv[0], "TEXT") == 0) && (subc == NULL))
		parse_dwg_text(ctx, subc, ly, nc, n, 0, 0);
	else
		return 0;

	return 1;
}

/* Returns number of objects drawn: 0 or more */
static long parse_dwg_all(hkp_ctx_t *ctx, pcb_subc_t *subc, pcb_layer_t *ly, const hkp_netclass_t *nc, node_t *nd)
{
	node_t *n;
	long cnt = 0;

	for(n = nd->first_child; n != NULL; n = n->next)
		cnt += parse_dwg(ctx, subc, ly, nc, n);

	return cnt;
}

/* Return 1 on success, 0 on fail
   Return the side of "side", and set lyname to lyname_top or lyname_bottom depending on side
   For a subcircuit, the side is given by MNT_SIDE argument */
static int parse_side(node_t *n, pcb_subc_t *subc, pcb_layer_type_t *side, const char *lyname, const char *lyname_top, const char *lyname_bottom)
{
	node_t *tmp;
	pcb_layer_type_t subc_side = 0;
	int on_bottom=-1;

	if (n == NULL)
		return 0;

	if (subc != NULL) {
		if (pcb_subc_get_side(subc, &on_bottom) == -1) {
			hkp_error(n, "Error getting subc side\n");
		}
		else {
			if (on_bottom == 1)
				subc_side = PCB_LYT_BOTTOM;
			else
				subc_side = PCB_LYT_TOP;
		}
	}

	/* In some sections, like ..ASSEMBLY_OUTLINE inside .PACKAGE_CELL (a subcircuit), there is no explicit SIDE line. */
	/* So if this is a subcircuit, and there is no SIDE line, get the side from subc side.*/
	tmp = find_nth(n->first_child, "SIDE", 0);
	if (tmp == NULL) {
		if (subc != NULL) {
			*side = subc_side;
			if ((subc_side & PCB_LYT_TOP) != 0)
				lyname = lyname_top;
			else
				lyname = lyname_bottom;
			return 1;
		}
		else
			return 0;
	}

	if (strcmp(tmp->argv[1], "MNT_SIDE") == 0) {
		if (subc_side != 0) {
			*side = subc_side;
			if ((subc_side & PCB_LYT_TOP) != 0)
				lyname = lyname_top;
			else
				lyname = lyname_bottom;
			return 1;
		}
		else
			hkp_error(n, "Unknown MNT_SIDE while parsing package.\n");
	}
	if (strcmp(tmp->argv[1], "OPP_SIDE") == 0) {
		if (subc_side != 0) {
			*side = (subc_side ^ PCB_LYT_TOP) ^ PCB_LYT_BOTTOM;
			if ((subc_side & PCB_LYT_TOP) != 0)
				lyname = lyname_bottom;
			else
				lyname = lyname_top;
			return 1;
		}
		else
			hkp_error(tmp, "Unknown MNT_SIDE while parsing package.\n");
	}
	else if (strcmp(tmp->argv[1], "TOP") == 0) {
		*side = PCB_LYT_TOP;
		lyname = lyname_top;
		return 1;
	}
	else if (strcmp(tmp->argv[1], "BOTTOM") == 0) {
		*side = PCB_LYT_BOTTOM;
		lyname = lyname_bottom;
		return 1;
	}

	hkp_error(n, "Unknown SIDE argument\n");
	return 0;
}

/* Returns number of objects drawn: 0 or more for valid layers, -1 for invalid layer */
static long parse_dwg_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, const hkp_netclass_t *nc, node_t *n)
{
	pcb_layer_type_t type = 0;
	pcb_layer_type_t side = PCB_LYT_TOP, subc_side = 0;
	pcb_layer_combining_t lyc = 0;
	const char *lyname = NULL, *purpose = NULL;
	pcb_layer_t *ly;

	if (strcmp(n->argv[0], "SILKSCREEN_OUTLINE") == 0) {
		type = PCB_LYT_SILK;
		lyc = PCB_LYC_AUTO;
		if (parse_side(n, subc, &side, lyname, "top-silk", "bot-silk") != 1)
			hkp_error(n, "Error parsing silkscreen side.\n");
	}
	else if (strcmp(n->argv[0], "SOLDER_MASK") == 0) {
		type = PCB_LYT_MASK;
		lyc = PCB_LYC_AUTO;
		if (parse_side(n, subc, &side, lyname, "top-mask", "bot-mask") != 1)
			hkp_error(n, "Error parsing solder mask side.\n");
	}
	else if (strcmp(n->argv[0], "SOLDER_PASTE") == 0) {
		type = PCB_LYT_PASTE;
		lyc = PCB_LYC_AUTO;
		if (parse_side(n, subc, &side, lyname, "top-paste", "bot-paste") != 1)
			hkp_error(n, "Error parsing paste side.\n");
	}
	else if (strcmp(n->argv[0], "ASSEMBLY_OUTLINE") == 0) {
		type = PCB_LYT_DOC;
		lyc = PCB_LYC_AUTO;
		if (parse_side(n, subc, &side, lyname, "top-assy", "bot-assy") != 1)
			hkp_error(n, "Error parsing assembly outline.\n");
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
		rnd_layer_id_t lid;
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
	return parse_dwg_all(ctx, subc, ly, nc, n);
}

static pcb_layer_t *parse_layer(hkp_ctx_t *ctx, pcb_subc_t *subc, const char *ln, int user, node_t *err_node)
{
	pcb_layer_t *ly;
	pcb_layer_type_t lyt = 0;
	pcb_layer_combining_t lyc = 0;
	char *purpose = NULL;
	char *name = NULL;

	if (user) { /* user layers are DOC layers identified by name and are allocated on the go */
		rnd_layer_id_t lid;
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
		rnd_layergrp_id_t gid;

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
			ly->meta.bound.purpose = rnd_strdup(purpose);
	}
	else {
		rnd_layer_id_t lid;
		int ln = pcb_layer_listp(ctx->pcb, lyt, &lid, 1, -1, purpose);
		if (ln == 0)
			return NULL;
		return pcb_get_layer(ctx->pcb->Data, lid);
	}
	return ly;
}

static void parse_subc_text(hkp_ctx_t *ctx, pcb_subc_t *subc, const hkp_netclass_t *nc, node_t *textnode)
{
	node_t *tt;
	const char *text_str;
	pcb_flag_values_t flg = PCB_FLAG_FLOATER;
	int omit_on_silk = 0;

	tt = find_nth(textnode->first_child, "TEXT_TYPE", 0);
	if (tt == NULL)
		return;

	text_str = textnode->argv[1];
	if (strcmp(tt->argv[1], "REF_DES") == 0) {
		rnd_attribute_put(&subc->Attributes, "refdes", textnode->argv[1]);
		text_str = "%a.parent.refdes%";
		flg |= PCB_FLAG_DYNTEXT;
	}
	else if (strcmp(tt->argv[1], "PARTNO") == 0) {
		rnd_attribute_put(&subc->Attributes, "footprint", textnode->argv[1]);
		text_str = "%a.parent.footprint%";
		flg |= PCB_FLAG_DYNTEXT;
		omit_on_silk = 1;
	}

	parse_dwg_text(ctx, subc, NULL, nc, textnode, omit_on_silk, flg);
}


static pcb_subc_t *parse_package(hkp_ctx_t *ctx, pcb_data_t *dt, node_t *nd)
{
	pcb_subc_t *subc;
	node_t *n;
	rnd_coord_t ox, oy;
	double rot = 0;
	int on_bottom = 0, seen_oxy = 0;
	const hkp_netclass_t *nc = NULL;

	subc = pcb_subc_alloc();

	/* extract global */
	for(n = nd->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "DESCRIPTION") == 0) {
			rnd_attribute_put(&subc->Attributes, "description", n->argv[1]);
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
		else if (strcmp(n->argv[0], "TEXT") == 0)
			parse_subc_text(ctx, subc, nc, n);
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
			parse_pin(ctx, subc, nc, n, on_bottom);
		else
			parse_dwg_layer(ctx, subc, nc, n);
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
			dt->subc_tree = rnd_r_create_tree();
		rnd_r_insert_entry(dt->subc_tree, (rnd_rnd_box_t *)subc);

		pcb_subc_rebind(ctx->pcb, subc);
	}
	return subc;
}

static void parse_net(hkp_ctx_t *ctx, node_t *netroot)
{
	node_t *n, *lyn;
	pcb_layer_t *ly;
	const char *netname = netroot->argv[1];
TODO("netclass: fill this in:")
	const hkp_netclass_t *nc = NULL;

	if (strcmp(netname, "Unconnected_Net") != 0)
		pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], netname, PCB_NETA_ALLOC);

	for(n = netroot->first_child; n != NULL; n = n->next) {
		if ((strcmp(n->argv[0], "TRACE") == 0) || (strcmp(n->argv[0], "CONDUCTIVE_AREA") == 0)) {
			lyn = find_nth(n->first_child, "ROUTE_LYR", 0);
			if (lyn == NULL)
				continue;
			ly = parse_layer(ctx, NULL, lyn->argv[1], 0, lyn);
			if (ly == NULL) {
				hkp_error(lyn, "Unknown trace layer '%s'\n", lyn->argv[1]);
				continue;
			}
			parse_dwg_all(ctx, NULL, ly, nc, n);
		}
		else if (strcmp(n->argv[0], "VIA") == 0)
			parse_dgw_via(ctx, nc, n);
	}
}

static const rnd_unit_t *parse_units(const char *ust)
{
	if (strcmp(ust, "MIL") == 0) return rnd_get_unit_struct("mil");
	if (strcmp(ust, "TH") == 0)  return rnd_get_unit_struct("mil");
	if (strcmp(ust, "MM") == 0)  return rnd_get_unit_struct("mm");
	if (strcmp(ust, "IN") == 0)  return rnd_get_unit_struct("inch");
	return NULL;
}

static int parse_layout_globals(hkp_ctx_t *ctx, hkp_tree_t *tree)
{
	node_t *n;
	char *end;

	ctx->num_cop_layers = -1;

	/* extract globals */
	for(n = tree->root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "NUMBER_OF_LAYERS") == 0) {
			ctx->num_cop_layers = strtol(n->argv[1], &end, 10);
			if (*end != '\0')
				return hkp_error(n, "Invalid number of layers '%s' (expected integer)\n", n->argv[1]);
			if ((ctx->num_cop_layers < 1) || (ctx->num_cop_layers > ((PCB_MAX_LAYER/2)-8)))
				return hkp_error(n, "Invalid number of layers '%s' (out of range)\n", n->argv[1]);
		}
		else if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->unit = parse_units(n->argv[1]);
			if (ctx->unit == NULL)
				return hkp_error(n, "Unknown unit '%s'\n", n->argv[1]);
		}
	}

	if (ctx->num_cop_layers < 0)
		return hkp_error(tree->root, "Missing NUMBER_OF_LAYERS\n");

	{ /* create the layer stack: copper layers, as many as required by the header */
		int len = 0, n;
		pcb_dflgmap_t map[PCB_MAX_LAYERGRP];
		const pcb_dflgmap_t *m;

		while(ctx->pcb->LayerGroups.len > 0)
			pcb_layergrp_del(ctx->pcb, 0, 1, 0);

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

	return 0;
}

static int parse_layout_root(hkp_ctx_t *ctx, hkp_tree_t *tree)
{
	node_t *n;
	const hkp_netclass_t *nc = NULL;

	/* build packages and draw objects */
	for(n = tree->root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "PACKAGE_CELL") == 0)
			parse_package(ctx, ctx->pcb->Data, n);
		if (strcmp(n->argv[0], "NET") == 0) {
			parse_net(ctx, n);
TODO("netclass: set nc for net's netclass");
		}
		if (strcmp(n->argv[0], "GRAPHIC") == 0) {
			pcb_layer_t *ly;
			node_t *lyn;

			lyn = find_nth(n->first_child, "USER_LYR", 0);
			if (lyn == NULL)
				continue;
			ly = parse_layer(ctx, NULL, lyn->argv[1], 1, lyn);
			if (ly == NULL) {
				hkp_error(lyn, "Unknown graphic layer '%s'\n", lyn->argv[1]);
				continue;
			}
			parse_dwg_all(ctx, NULL, ly, nc, n);
		}
		else { /* global drawing objects in layers or outside of layers */
			parse_dwg(ctx, NULL, NULL, nc, n);
			parse_dwg_layer(ctx, NULL, nc, n);
		}

	}

	/* 'autocrop' the board for now (required by y mirror and unknown extents) */
	{
		rnd_rnd_box_t bb;
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

	tree->filename = rnd_strdup(fn);
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

#include "read_net.c"
#include "read_pstk.c"

int io_mentor_cell_read_pcb(pcb_plug_io_t *pctx, pcb_board_t *pcb, const char *fn, rnd_conf_role_t settings_dest)
{
	hkp_ctx_t ctx;
	int res = -1;
	FILE *flay;
	char *end, fn2[RND_PATH_MAX];

	pcb_data_clip_inhibit_inc(pcb->Data);
	pcb_layergrp_inhibit_inc();

	memset(&ctx, 0, sizeof(ctx));

	flay = rnd_fopen(&pcb->hidlib, fn, "r");
	if (flay == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open layout hkp '%s' for read\n", fn);
		goto err;
	}

	/* create the file name for the padstacks */
	strncpy(fn2, fn, RND_PATH_MAX);
	fn2[RND_PATH_MAX-1] = '\0';
	end = strrchr(fn2, RND_DIR_SEPARATOR_C);
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
	ctx.unit = rnd_get_unit_struct("mm");



	load_hkp(&ctx.layout, flay, fn);
	fclose(flay);

	/* we are loading the cells into a board, make a default layer stack for that */
	pcb_layergrp_inhibit_inc();
	pcb_layers_reset(pcb);
	pcb_layer_group_setup_default(pcb);
	pcb_layer_group_setup_silks(pcb);
	pcb_layer_auto_fixup(pcb);
	pcb_layergrp_inhibit_dec();

	/* parse the stackup and some global settings first, since netclass depends on this */
	parse_layout_globals(&ctx, &ctx.layout);

	strcpy(end, "NetClass.hkp");
	if (io_mentor_cell_netclass(&ctx, fn2) != 0) {
		fclose(flay);
		goto err;
	}

	strcpy(end, "NetProps.hkp");
	if (io_mentor_cell_netlist(&ctx, fn2) != 0) {
		fclose(flay);
		goto err;
	}

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
