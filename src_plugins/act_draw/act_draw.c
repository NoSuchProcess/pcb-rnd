/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include <librnd/core/actions.h>
#include "board.h"
#include <librnd/core/compat_misc.h>
#include "flag_str.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_pstk.h"
#include "obj_text.h"
#include <librnd/core/plugins.h>
#include "undo.h"

#include "obj_text.h"
#include "obj_line.h"
#include "obj_text_draw.h"
#include "obj_line_draw.h"

#include "keywords_sphash.h"

static const char *PTR_DOMAIN_POLY = "fgw_ptr_domain_poly";

static int flg_error(const char *msg)
{
	rnd_message(RND_MSG_ERROR, "act_draw flag conversion error: %s\n", msg);
	return 0;
}

#define DRAWOPTARG \
	int noundo = 0, ao = 0; \
	if (((argv[1].type & FGW_STR) == FGW_STR) && (strcmp(argv[1].val.str, "noundo") == 0)) { \
		noundo = 1; \
		ao++; \
	}

#define RET_IDPATH(obj) \
	do { \
		pcb_idpath_t *idp = pcb_obj2idpath((pcb_any_obj_t *)obj); \
		res->type = FGW_IDPATH; \
		fgw_ptr_reg(&rnd_fgw, res, RND_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, idp); \
	} while(0)

#include "act_pstk_proto.c"
#include "act_polybool.c"

static const char pcb_acts_LineNew[] = "LineNew([noundo,] data, layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags)";
static const char pcb_acth_LineNew[] = "Create a pcb line segment on a layer. For now data must be \"pcb\". Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_LineNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_line_t *line;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t x1, y1, x2, y2, th, cl;
	pcb_flag_t flags;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, LineNew, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, LineNew, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_COORD, LineNew, x1 = fgw_coord(&argv[3+ao]));
	RND_ACT_CONVARG(4+ao, FGW_COORD, LineNew, y1 = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_COORD, LineNew, x2 = fgw_coord(&argv[5+ao]));
	RND_ACT_CONVARG(6+ao, FGW_COORD, LineNew, y2 = fgw_coord(&argv[6+ao]));
	RND_ACT_CONVARG(7+ao, FGW_COORD, LineNew, th = fgw_coord(&argv[7+ao]));
	RND_ACT_CONVARG(8+ao, FGW_COORD, LineNew, cl = fgw_coord(&argv[8+ao]));
	RND_ACT_CONVARG(9+ao, FGW_STR, LineNew, sflg = argv[9+ao].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	line = pcb_line_new(layer, x1, y1, x2, y2, th, cl*2, flags);

	if (line != NULL) {
		RET_IDPATH(line);
		if (!noundo)
			pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, line, line);
	}
	return 0;
}

static const char pcb_acts_ArcNew[] = "ArcNew([noundo,] data, layer, centx, centy, radiusx, radiusy, start_ang, delta_ang, thickness, clearance, flags)";
static const char pcb_acth_ArcNew[] = "Create a pcb arc segment on a layer. For now data must be \"pcb\". Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_ArcNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_arc_t *arc;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t cx, cy, hr, wr, th, cl;
	double sa, da;
	pcb_flag_t flags;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, ArcNew, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, ArcNew, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_COORD, ArcNew, cx = fgw_coord(&argv[3+ao]));
	RND_ACT_CONVARG(4+ao, FGW_COORD, ArcNew, cy = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_COORD, ArcNew, wr = fgw_coord(&argv[5+ao]));
	RND_ACT_CONVARG(6+ao, FGW_COORD, ArcNew, hr = fgw_coord(&argv[6+ao]));
	RND_ACT_CONVARG(7+ao, FGW_DOUBLE, ArcNew, sa = argv[7+ao].val.nat_double);
	RND_ACT_CONVARG(8+ao, FGW_DOUBLE, ArcNew, da = argv[8+ao].val.nat_double);
	RND_ACT_CONVARG(9+ao, FGW_COORD, ArcNew, th = fgw_coord(&argv[9+ao]));
	RND_ACT_CONVARG(10+ao, FGW_COORD, ArcNew, cl = fgw_coord(&argv[10+ao]));
	RND_ACT_CONVARG(11+ao, FGW_STR, ArcNew, sflg = argv[11+ao].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	arc = pcb_arc_new(layer, cx, cy, wr, hr, sa, da, th, cl*2, flags, 0);

	if (arc != NULL) {
		RET_IDPATH(arc);
		if (!noundo)
			pcb_undo_add_obj_to_create(PCB_OBJ_ARC, layer, arc, arc);
	}
	return 0;
}

static const char pcb_acts_TextNew[] = "TextNew([noundo,] data, layer, fontID, x, y, rot, scale, thickness, text_string, flags)";
static const char pcb_acth_TextNew[] = "Create a pcb text on a layer. For now data must be \"pcb\". Font id 0 is the default font. Thickness 0 means default, calculated thickness. Scale=100 is the original font size. Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_TextNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg, *str;
	pcb_text_t *text = NULL;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t x, y, th;
	int scale, fontid;
	double rot;
	pcb_flag_t flags;
	pcb_font_t *font;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, TextNew, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, TextNew, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_INT, TextNew, fontid = argv[3+ao].val.nat_int);
	RND_ACT_CONVARG(4+ao, FGW_COORD, TextNew, x = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_COORD, TextNew, y = fgw_coord(&argv[5+ao]));
	RND_ACT_CONVARG(6+ao, FGW_DOUBLE, TextNew, rot = argv[6+ao].val.nat_double);
	RND_ACT_CONVARG(7+ao, FGW_INT, TextNew, scale = argv[7+ao].val.nat_int);
	RND_ACT_CONVARG(8+ao, FGW_COORD, TextNew, th = fgw_coord(&argv[8+ao]));
	RND_ACT_CONVARG(9+ao, FGW_STR, TextNew, str = argv[9+ao].val.str);
	RND_ACT_CONVARG(10+ao, FGW_STR, TextNew, sflg = argv[10+ao].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	font = pcb_font(PCB, fontid, 0);
	if (font != NULL) {
		flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
		text = pcb_text_new(layer, font, x, y, rot, scale, th, str, flags);
	}
	else
		rnd_message(RND_MSG_ERROR, "NewText: font %d not found\n", fontid);

	if (text != NULL) {
		RET_IDPATH(text);
		if (!noundo)
			pcb_undo_add_obj_to_create(PCB_OBJ_TEXT, layer, text, text);
	}
	return 0;
}

static const char pcb_acts_PstkNew[] = "PstkNew([noundo,] data, protoID, x, y, glob_clearance, flags)";
static const char pcb_acth_PstkNew[] = "Create a padstack. For now data must be \"pcb\". glob_clearance=0 turns off global clearance. Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_PstkNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_pstk_t *pstk;
	pcb_data_t *data;
	long proto;
	rnd_coord_t x, y, cl;
	pcb_flag_t flags;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, PstkNew, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LONG, PstkNew, proto = argv[2+ao].val.nat_int);
	RND_ACT_CONVARG(3+ao, FGW_COORD, PstkNew, x = fgw_coord(&argv[3+ao]));
	RND_ACT_CONVARG(4+ao, FGW_COORD, PstkNew, y = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_COORD, PstkNew, cl = fgw_coord(&argv[5+ao]));
	RND_ACT_CONVARG(6+ao, FGW_STR, PstkNew, sflg = argv[6+ao].val.str);

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	pstk = pcb_pstk_new(data, -1, proto, x, y, cl, flags);

	if (pstk != NULL) {
		RET_IDPATH(pstk);
		if (!noundo)
			pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, data, pstk, pstk);
	}
	return 0;
}

static const char pcb_acts_PolyNewFromRectangle[] = "PolyNewFromRectangle([noundo,] data, layer, x1, y1, x2, y2, clearance, flags)";
static const char pcb_acth_PolyNewFromRectangle[] = "Create a rectangular polygon. For now data must be \"pcb\". Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewFromRectangle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t x1, y1, x2, y2, cl;
	pcb_flag_t flags;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, PolyNewFromRectangle, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, PolyNewFromRectangle, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_COORD, PolyNewFromRectangle, x1 = fgw_coord(&argv[3+ao]));
	RND_ACT_CONVARG(4+ao, FGW_COORD, PolyNewFromRectangle, y1 = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_COORD, PolyNewFromRectangle, x2 = fgw_coord(&argv[5+ao]));
	RND_ACT_CONVARG(6+ao, FGW_COORD, PolyNewFromRectangle, y2 = fgw_coord(&argv[6+ao]));
	RND_ACT_CONVARG(7+ao, FGW_COORD, PolyNewFromRectangle, cl = fgw_coord(&argv[7+ao]));
	RND_ACT_CONVARG(8+ao, FGW_STR, PolyNewFromRectangle, sflg = argv[8+ao].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new_from_rectangle(layer, x1, y1, x2, y2, cl*2, flags);

	if (poly != NULL) {
		RET_IDPATH(poly);
		if (!noundo)
			pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, poly, poly);
	}
	return 0;
}

static long poly_append_ptlist(pcb_poly_t *poly, const char *ptlist)
{
	long len;
	char *s, *next, *tmp = rnd_strdup(ptlist);
	double c[2];
	rnd_bool success;

	s = tmp;
	while (isspace(*s) || (*s == ',')) s++;
	for(len = 0; s != NULL; s = next, len++) {
		next = strchr(s, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
			while (isspace(*next) || (*next == ',')) next++;
			if (*next == '\0')
				next = NULL;
		}
		c[len % 2] = rnd_get_value_ex(s, NULL, NULL, NULL, "mm", &success);
		if (!success) {
			rnd_message(RND_MSG_ERROR, "act_draw ptlist processing: '%s' is not a valid coordinate\n", s);
			free(tmp);
			return -1;
		}
		if ((len % 2) == 1)
			pcb_poly_point_new(poly, c[0], c[1]);
	}
	free(tmp);
	if ((len % 2) == 1) {
		rnd_message(RND_MSG_ERROR, "act_draw ptlist processing: odd number of points\n");
		return -1;
	}
	return len/2;
}

static const char pcb_acts_PolyNewFromPoints[] = "PolyNewFromRectangle([noundo,] data, layer, ptlist, clearance, flags)";
static const char pcb_acth_PolyNewFromPoints[] = "Create a polygon. For now data must be \"pcb\". ptlist is a comma separated list of coordinates (untiless coordinates are treated as mm). Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewFromPoints(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg, *ptlist;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t cl;
	pcb_flag_t flags;
	DRAWOPTARG;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_STR, PolyNewFromPoints, ptlist = argv[3+ao].val.str);
	RND_ACT_CONVARG(4+ao, FGW_COORD, PolyNewFromPoints, cl = fgw_coord(&argv[4+ao]));
	RND_ACT_CONVARG(5+ao, FGW_STR, PolyNewFromPoints, sflg = argv[5+ao].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new(layer, cl*2, flags);

	if (poly != NULL) {
		if (poly_append_ptlist(poly, ptlist) > 2) {
			pcb_poly_init_clip(data, layer, poly);
			pcb_add_poly_on_layer(layer, poly);
			RET_IDPATH(poly);
			if (!noundo)
				pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, poly, poly);
		}
	}
	return 0;
}

static const char pcb_acts_PolyNew[] = "PolyNew([noundo,] data, layer, ptlist, clearance, flags)";
static const char pcb_acth_PolyNew[] = "Create an empty polygon. For now data must be \"pcb\". Use PolyNewPoint to add points. Returns a polygon pointer valid until PolyNewEnd() is called.";
static fgw_error_t pcb_act_PolyNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	rnd_coord_t cl;
	pcb_flag_t flags;
	DRAWOPTARG;
	(void)noundo;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_COORD, PolyNewFromPoints, cl = fgw_coord(&argv[3+ao]));
	RND_ACT_CONVARG(4+ao, FGW_STR, PolyNewFromPoints, sflg = argv[4+ao].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new(layer, cl*2, flags);

	if (poly != NULL)
		fgw_ptr_reg(&rnd_fgw, res, PTR_DOMAIN_POLY, FGW_PTR, poly);
	return 0;
}

static const char pcb_acts_PolyNewPoints[] = "PolyNewPoints([noundo,] poly, ptlist)";
static const char pcb_acth_PolyNewPoints[] = "Add a list of points to a polygon created by PolyNew. Returns 0 on success.";
static fgw_error_t pcb_act_PolyNewPoints(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_poly_t *poly;
	const char *ptlist;
	DRAWOPTARG;
	(void)noundo;

	RND_ACT_CONVARG(1+ao, FGW_PTR, PolyNewPoints, poly = argv[1+ao].val.ptr_void);
	RND_ACT_CONVARG(2+ao, FGW_STR, PolyNewPoints, ptlist = argv[2+ao].val.str);
	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_POLY)) {
		rnd_message(RND_MSG_ERROR, "PolyNewPoints: invalid polygon pointer\n");
		RND_ACT_IRES(-1);
		return 0;
	}
	if (poly_append_ptlist(poly, ptlist) < 0) {
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PolyNewEnd[] = "PolyNewEnd([noundo,] data, layer, poly)";
static const char pcb_acth_PolyNewEnd[] = "Close and place a polygon started by PolyNew. Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewEnd(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	DRAWOPTARG;

	RND_ACT_CONVARG(1+ao, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_PTR, PolyNewPoints, poly = argv[3+ao].val.ptr_void);
	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_POLY)) {
		rnd_message(RND_MSG_ERROR, "PolyNewEnd: invalid polygon pointer\n");
		RND_ACT_IRES(0);
		return 0;
	}
	if (poly->PointN < 3) {
		rnd_message(RND_MSG_ERROR, "PolyNewEnd: can not finish polygon, need at least 3 points\n");
		RND_ACT_IRES(0);
		return 0;
	}

	pcb_poly_init_clip(data, layer, poly);
	pcb_add_poly_on_layer(layer, poly);

	RET_IDPATH(poly);
	if (!noundo)
		pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, poly, poly);
	fgw_ptr_unreg(&rnd_fgw, &argv[1], PTR_DOMAIN_POLY);
	return 0;
}

static const char pcb_acts_LayerObjDup[] = "LayerObjDup([noundo,] data, layer, srcobj)";
static const char pcb_acth_LayerObjDup[] = "Duplicate srcobj on a layer. Srcobj is specified by an idpath. For now data must be \"pcb\". Returns the idpath of the new object or 0 on error.";
static fgw_error_t pcb_act_LayerObjDup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_any_obj_t *src, *dst;
	pcb_idpath_t *idp;
	DRAWOPTARG;

TODO("implement noundo");
	(void)noundo;

	RND_ACT_IRES(0);
	RND_ACT_CONVARG(1+ao, FGW_DATA, LayerObjDup, data = fgw_data(&argv[1+ao]));
	RND_ACT_CONVARG(2+ao, FGW_LAYER, LayerObjDup, layer = fgw_layer(&argv[2+ao]));
	RND_ACT_CONVARG(3+ao, FGW_PTR, LayerObjDup, idp = argv[3+ao].val.ptr_void);
	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[3], RND_PTR_DOMAIN_IDPATH)) {
		rnd_message(RND_MSG_ERROR, "LayerObjDup: invalid object pointer\n");
		return FGW_ERR_PTR_DOMAIN;
	}
	src = pcb_idpath2obj(PCB, idp);
	if (src == NULL)
		return FGW_ERR_ARG_CONV;

	switch(src->type) {
		case PCB_OBJ_ARC:   dst = (pcb_any_obj_t *)pcb_arc_dup(layer, (pcb_arc_t *)src); break;
		case PCB_OBJ_LINE:  dst = (pcb_any_obj_t *)pcb_line_dup(layer, (pcb_line_t *)src); break;
		case PCB_OBJ_POLY:  dst = (pcb_any_obj_t *)pcb_poly_dup(layer, (pcb_poly_t *)src); break;
		case PCB_OBJ_TEXT:  dst = (pcb_any_obj_t *)pcb_text_dup(layer, (pcb_text_t *)src); break;
		default:
			return FGW_ERR_ARG_CONV;
	}

	if (dst != NULL) {
		idp = pcb_obj2idpath(dst);
		fgw_ptr_reg(&rnd_fgw, res, RND_PTR_DOMAIN_IDPATH, FGW_PTR | FGW_STRUCT, idp);

		pcb_poly_clear_from_poly(data, dst->type, layer, dst);
	}

	return 0;
}

/*** low level draw, e.g. for preview widgets ***/
static pcb_draw_info_t def_info;
static rnd_xform_t def_xform;

static const char pcb_acts_DrawText[] = "DrawText(gc, x, y, string, rot, scale_percent)";
static const char pcb_acth_DrawText[] = "Low level draw (render) a text object using graphic context gc.";
static fgw_error_t pcb_act_DrawText(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static pcb_text_t t = {0};
	rnd_hid_gc_t gc, ogc;

	RND_ACT_CONVARG(1, FGW_PTR, DrawText, gc = argv[1].val.ptr_void);
	RND_ACT_CONVARG(2, FGW_COORD, DrawText, t.X = fgw_coord(&argv[2]));
	RND_ACT_CONVARG(3, FGW_COORD, DrawText, t.Y = fgw_coord(&argv[3]));
	RND_ACT_CONVARG(4, FGW_STR, DrawText, t.TextString = argv[4].val.str);
	RND_ACT_CONVARG(5, FGW_DOUBLE, DrawText, t.rot = argv[5].val.nat_double);
	RND_ACT_CONVARG(6, FGW_INT, DrawText, t.Scale = argv[6].val.nat_int);

	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], RND_PTR_DOMAIN_GC)) {
		rnd_message(RND_MSG_ERROR, "DrawText(): invalid gc (pointer domain error)\n");
		return FGW_ERR_ARG_CONV;
	}


	t.fid = 0; /* use the default font */
	t.Flags = pcb_no_flags();
	ogc = pcb_draw_out.fgGC;
	pcb_draw_out.fgGC = gc;
	pcb_text_draw_(&def_info, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
	pcb_draw_out.fgGC = ogc;
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DrawLine[] = "DrawLine(gc, x1, y1, x2, y2, thickness)";
static const char pcb_acth_DrawLine[] = "Low level draw (render) a line using graphic context gc.";
static fgw_error_t pcb_act_DrawLine(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_gc_t gc, ogc;
	pcb_line_t l = {0};

	RND_ACT_CONVARG(1, FGW_PTR, DrawLine, gc = argv[1].val.ptr_void);
	RND_ACT_CONVARG(2, FGW_COORD, DrawLine, l.Point1.X  = fgw_coord(&argv[2]));
	RND_ACT_CONVARG(3, FGW_COORD, DrawLine, l.Point1.Y  = fgw_coord(&argv[3]));
	RND_ACT_CONVARG(4, FGW_COORD, DrawLine, l.Point2.X  = fgw_coord(&argv[4]));
	RND_ACT_CONVARG(5, FGW_COORD, DrawLine, l.Point2.Y  = fgw_coord(&argv[5]));
	RND_ACT_CONVARG(6, FGW_COORD, DrawLine, l.Thickness = fgw_coord(&argv[6]));

	if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], RND_PTR_DOMAIN_GC)) {
		rnd_message(RND_MSG_ERROR, "DrawLine(): invalid gc (pointer domain error)\n");
		return FGW_ERR_ARG_CONV;
	}

	ogc = pcb_draw_out.fgGC;
	pcb_draw_out.fgGC = gc;
	pcb_line_draw_(&def_info, &l, 0);
	pcb_draw_out.fgGC = ogc;
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DrawColor[] = "DrawColor(gc, colorstr)";
static const char pcb_acth_DrawColor[] = "Set pen color in of a gc.";
static fgw_error_t pcb_act_DrawColor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_gc_t gc, ogc;
	static rnd_color_t clr;
	const char *scolor;

	RND_ACT_CONVARG(1, FGW_PTR, DrawText, gc = argv[1].val.ptr_void);
	RND_ACT_CONVARG(2, FGW_STR, DrawText, scolor = argv[2].val.str);

	if (rnd_color_load_str(&clr, scolor) != 0) {
		rnd_message(RND_MSG_ERROR, "DrawColor(): invalid color value '%s'\n", scolor);
		return FGW_ERR_ARG_CONV;
	}

	ogc = pcb_draw_out.fgGC;
	pcb_draw_out.fgGC = gc;
	rnd_render->set_color(gc, &clr);
	pcb_draw_out.fgGC = ogc;
	RND_ACT_IRES(0);
	return 0;
}

rnd_action_t act_draw_action_list[] = {
	{"LineNew", pcb_act_LineNew, pcb_acth_LineNew, pcb_acts_LineNew},
	{"ArcNew", pcb_act_ArcNew, pcb_acth_ArcNew, pcb_acts_ArcNew},
	{"TextNew", pcb_act_TextNew, pcb_acth_TextNew, pcb_acts_TextNew},
	{"PstkNew", pcb_act_PstkNew, pcb_acth_PstkNew, pcb_acts_PstkNew},
	{"PolyNewFromRectangle", pcb_act_PolyNewFromRectangle, pcb_acth_PolyNewFromRectangle, pcb_acts_PolyNewFromRectangle},
	{"PolyNewFromPoints", pcb_act_PolyNewFromPoints, pcb_acth_PolyNewFromPoints, pcb_acts_PolyNewFromPoints},
	{"PolyNew", pcb_act_PolyNew, pcb_acth_PolyNew, pcb_acts_PolyNew},
	{"PolyNewPoints", pcb_act_PolyNewPoints, pcb_acth_PolyNewPoints, pcb_acts_PolyNewPoints},
	{"PolyNewEnd", pcb_act_PolyNewEnd, pcb_acth_PolyNewEnd, pcb_acts_PolyNewEnd},
	{"PstkProtoTmp", pcb_act_PstkProtoTmp, pcb_acth_PstkProtoTmp, pcb_acts_PstkProtoTmp},
	{"PstkProtoEdit", pcb_act_PstkProtoEdit, pcb_acth_PstkProtoEdit, pcb_acts_PstkProtoEdit},
	{"LayerObjDup", pcb_act_LayerObjDup, pcb_acth_LayerObjDup, pcb_acts_LayerObjDup},
	{"DrawText", pcb_act_DrawText, pcb_acth_DrawText, pcb_acts_DrawText},
	{"DrawLine", pcb_act_DrawLine, pcb_acth_DrawLine, pcb_acts_DrawLine},
	{"DrawColor", pcb_act_DrawColor, pcb_acth_DrawColor, pcb_acts_DrawColor},
	{"PolyBool", pcb_act_PolyBool, pcb_acth_PolyBool, pcb_acts_PolyBool}
};

static const char *act_draw_cookie = "act_draw";

int pplg_check_ver_act_draw(int ver_needed) { return 0; }

void pplg_uninit_act_draw(void)
{
	rnd_remove_actions_by_cookie(act_draw_cookie);
}

int pplg_init_act_draw(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(act_draw_action_list, act_draw_cookie)
	def_info.xform = &def_xform;
	return 0;
}
