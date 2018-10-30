/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
#include "conf_core.h"

#include "actions.h"
#include "board.h"
#include "compat_misc.h"
#include "flag_str.h"
#include "obj_arc.h"
#include "obj_line.h"
#include "obj_pstk.h"
#include "obj_text.h"
#include "plugins.h"

static const char *PTR_DOMAIN_POLY = "fgw_ptr_domain_poly";

static const char pcb_acts_GetValue[] = "GetValue(input, units, relative, default_unit)";
static const char pcb_acth_GetValue[] = "Convert a coordinate value. Returns an unitless double or FGW_ERR_ARG_CONV. The 3rd parameter controls whether to require relative coordinates (+- prefix). Wraps pcb_get_value_ex().";
static fgw_error_t pcb_act_GetValue(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *input, *units, *def_unit;
	int relative, a;
	double v;
	pcb_bool success;

	PCB_ACT_CONVARG(1, FGW_STR, GetValue, input = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, GetValue, units = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_INT, GetValue, relative = argv[3].val.nat_int);
	PCB_ACT_CONVARG(4, FGW_STR, GetValue, def_unit = argv[1].val.str);

	if (*units == '\0')
		units = NULL;

	v = pcb_get_value_ex(input, units, &a, NULL, def_unit, &success);
	if (!success || (relative && a))
		return FGW_ERR_ARG_CONV;

	res->type = FGW_DOUBLE;
	res->val.nat_double = v;
	return 0;
}

static int flg_error(const char *msg)
{
	pcb_message(PCB_MSG_ERROR, "act_draw flag conversion error: %s\n", msg);
	return 0;
}

static const char pcb_acts_LineNew[] = "LineNew(data, layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags)";
static const char pcb_acth_LineNew[] = "Create a pcb line segment on a layer. For now data must be \"pcb\". Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_LineNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_line_t *line;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t x1, y1, x2, y2, th, cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, LineNew, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, LineNew, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, LineNew, x1 = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, LineNew, y1 = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, LineNew, x2 = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_COORD, LineNew, y2 = fgw_coord(&argv[6]));
	PCB_ACT_CONVARG(7, FGW_COORD, LineNew, th = fgw_coord(&argv[7]));
	PCB_ACT_CONVARG(8, FGW_COORD, LineNew, cl = fgw_coord(&argv[8]));
	PCB_ACT_CONVARG(9, FGW_STR, LineNew, sflg = argv[9].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	line = pcb_line_new(layer, x1, y1, x2, y2, th, cl*2, flags);

	if (line != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = line->ID;
	}
	return 0;
}

static const char pcb_acts_ArcNew[] = "ArcNew(data, layer, centx, centy, radiusx, radiusy, start_ang, delta_ang, thickness, clearance, flags)";
static const char pcb_acth_ArcNew[] = "Create a pcb arc segment on a layer. For now data must be \"pcb\". Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_ArcNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_arc_t *arc;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t cx, cy, hr, wr, th, cl;
	double sa, da;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, ArcNew, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, ArcNew, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, ArcNew, cx = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, ArcNew, cy = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, ArcNew, wr = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_COORD, ArcNew, hr = fgw_coord(&argv[6]));
	PCB_ACT_CONVARG(7, FGW_DOUBLE, ArcNew, sa = argv[7].val.nat_double);
	PCB_ACT_CONVARG(8, FGW_DOUBLE, ArcNew, da = argv[8].val.nat_double);
	PCB_ACT_CONVARG(9, FGW_COORD, ArcNew, th = fgw_coord(&argv[9]));
	PCB_ACT_CONVARG(10, FGW_COORD, ArcNew, cl = fgw_coord(&argv[10]));
	PCB_ACT_CONVARG(11, FGW_STR, ArcNew, sflg = argv[11].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	arc = pcb_arc_new(layer, cx, cy, wr, hr, sa, da, th, cl*2, flags, 0);

	if (arc != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = arc->ID;
	}
	return 0;
}

static const char pcb_acts_TextNew[] = "TextNew(data, layer, x, y, rot, scale, thickness, test_string, flags)";
static const char pcb_acth_TextNew[] = "Create a pcb text on a layer. For now data must be \"pcb\". Font id 0 is the default font. Thickness 0 means default, calculated thickness. Scale=100 is the original font size. Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_TextNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg, *str;
	pcb_text_t *text = NULL;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t x, y, th;
	int scale, fontid;
	double rot;
	pcb_flag_t flags;
	pcb_font_t *font;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, TextNew, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, TextNew, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_INT, TextNew, fontid = argv[3].val.nat_int);
	PCB_ACT_CONVARG(4, FGW_COORD, TextNew, x = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, TextNew, y = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_DOUBLE, TextNew, rot = argv[6].val.nat_double);
	PCB_ACT_CONVARG(7, FGW_INT, TextNew, scale = argv[7].val.nat_int);
	PCB_ACT_CONVARG(8, FGW_COORD, TextNew, th = fgw_coord(&argv[8]));
	PCB_ACT_CONVARG(9, FGW_STR, TextNew, str = argv[9].val.str);
	PCB_ACT_CONVARG(10, FGW_STR, TextNew, sflg = argv[10].val.str);

	if ((data != PCB->Data) || (layer == NULL))
		return 0;

	font = pcb_font(PCB, fontid, 0);
	if (font != NULL) {
		flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
		text = pcb_text_new(layer, font, x, y, rot, scale, th, str, flags);
	}
	else
		pcb_message(PCB_MSG_ERROR, "NewText: font %d not found\n", fontid);

	if (text != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = text->ID;
	}
	return 0;
}

static const char pcb_acts_PstkNew[] = "PstkNew(data, protoID, x, y, glob_clearance, flags)";
static const char pcb_acth_PstkNew[] = "Create a padstack. For now data must be \"pcb\". glob_clearance=0 turns off global clearance. Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_PstkNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_pstk_t *pstk;
	pcb_data_t *data;
	long proto;
	pcb_coord_t x, y, cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, PstkNew, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LONG, PstkNew, proto = argv[2].val.nat_int);
	PCB_ACT_CONVARG(3, FGW_COORD, PstkNew, x = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, PstkNew, y = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, PstkNew, cl = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_STR, PstkNew, sflg = argv[6].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	pstk = pcb_pstk_new(data, proto, x, y, cl, flags);

	if (pstk != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = pstk->ID;
	}
	return 0;
}

static const char pcb_acts_PolyNewFromRectangle[] = "PolyNewFromRectangle(data, layer, x1, y1, x2, y2, clearance, flags)";
static const char pcb_acth_PolyNewFromRectangle[] = "Create a rectangular polygon. For now data must be \"pcb\". Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewFromRectangle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t x1, y1, x2, y2, cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, PolyNewFromRectangle, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, PolyNewFromRectangle, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, PolyNewFromRectangle, x1 = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_COORD, PolyNewFromRectangle, y1 = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_COORD, PolyNewFromRectangle, x2 = fgw_coord(&argv[5]));
	PCB_ACT_CONVARG(6, FGW_COORD, PolyNewFromRectangle, y2 = fgw_coord(&argv[6]));
	PCB_ACT_CONVARG(7, FGW_COORD, PolyNewFromRectangle, cl = fgw_coord(&argv[7]));
	PCB_ACT_CONVARG(8, FGW_STR, PolyNewFromRectangle, sflg = argv[8].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new_from_rectangle(layer, x1, y1, x2, y2, cl*2, flags);

	if (poly != NULL) {
		res->type = FGW_LONG;
		res->val.nat_long = poly->ID;
	}
	return 0;
}

static long poly_append_ptlist(pcb_poly_t *poly, const char *ptlist)
{
	long len;
	char *s, *next, *tmp = pcb_strdup(ptlist);
	double c[2];
	pcb_bool success;

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
		c[len % 2] = pcb_get_value_ex(s, NULL, NULL, NULL, "mm", &success);
		if (!success) {
			pcb_message(PCB_MSG_ERROR, "act_draw ptlist processing: '%s' is not a valid coordinate\n", s);
			free(tmp);
			return -1;
		}
		if ((len % 2) == 1)
			pcb_poly_point_new(poly, c[0], c[1]);
	}
	free(tmp);
	if ((len % 2) == 1) {
		pcb_message(PCB_MSG_ERROR, "act_draw ptlist processing: odd number of points\n");
		return -1;
	}
	return len/2;
}

static const char pcb_acts_PolyNewFromPoints[] = "PolyNewFromRectangle(data, layer, ptlist, clearance, flags)";
static const char pcb_acth_PolyNewFromPoints[] = "Create a polygon. For now data must be \"pcb\". ptlist is a comma separated list of coordinates (untiless coordinates are treated as mm). Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewFromPoints(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg, *ptlist;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_STR, PolyNewFromPoints, ptlist = argv[3].val.str);
	PCB_ACT_CONVARG(4, FGW_COORD, PolyNewFromPoints, cl = fgw_coord(&argv[4]));
	PCB_ACT_CONVARG(5, FGW_STR, PolyNewFromPoints, sflg = argv[5].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new(layer, cl*2, flags);

	if (poly != NULL) {
		if (poly_append_ptlist(poly, ptlist) > 2) {
			pcb_poly_init_clip(data, layer, poly);
			pcb_add_poly_on_layer(layer, poly);
			res->type = FGW_LONG;
			res->val.nat_long = poly->ID;
		}
	}
	return 0;
}

static const char pcb_acts_PolyNew[] = "PolyNew(data, layer, ptlist, clearance, flags)";
static const char pcb_acth_PolyNew[] = "Create an empty polygon. For now data must be \"pcb\". Use PolyNewPoint to add points. Returns a polygon pointer valid until PolyNewEnd() is called.";
static fgw_error_t pcb_act_PolyNew(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sflg;
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_coord_t cl;
	pcb_flag_t flags;

	PCB_ACT_IRES(0);
	PCB_ACT_CONVARG(1, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, PolyNewFromPoints, cl = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_STR, PolyNewFromPoints, sflg = argv[4].val.str);

	if (data != PCB->Data)
		return 0;

	flags = pcb_strflg_s2f(sflg, flg_error, NULL, 0);
	poly = pcb_poly_new(layer, cl*2, flags);

	if (poly != NULL)
		fgw_ptr_reg(&pcb_fgw, res, PTR_DOMAIN_POLY, FGW_PTR, poly);
	return 0;
}

static const char pcb_acts_PolyNewPoints[] = "PolyNewPoints(poly, ptlist)";
static const char pcb_acth_PolyNewPoints[] = "Add a list of points to a polygon created by PolyNew. Returns 0 on success.";
static fgw_error_t pcb_act_PolyNewPoints(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_poly_t *poly;
	const char *ptlist;

	PCB_ACT_CONVARG(1, FGW_PTR, PolyNewPoints, poly = argv[1].val.ptr_void);
	PCB_ACT_CONVARG(2, FGW_STR, PolyNewPoints, ptlist = argv[2].val.str);
	if (!fgw_ptr_in_domain(&pcb_fgw, &argv[1], PTR_DOMAIN_POLY)) {
		pcb_message(PCB_MSG_ERROR, "PolyNewPoints: invalid polygon pointer\n");
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (poly_append_ptlist(poly, ptlist) < 0) {
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PolyNewEnd[] = "PolyNewEnd(data, layer, poly)";
static const char pcb_acth_PolyNewEnd[] = "Close and place a polygon started by PolyNew. Returns the ID of the new object or 0 on error.";
static fgw_error_t pcb_act_PolyNewEnd(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_poly_t *poly;
	pcb_data_t *data;
	pcb_layer_t *layer;

	PCB_ACT_CONVARG(1, FGW_DATA, PolyNewFromPoints, data = fgw_data(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_LAYER, PolyNewFromPoints, layer = fgw_layer(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_PTR, PolyNewPoints, poly = argv[3].val.ptr_void);
	if (!fgw_ptr_in_domain(&pcb_fgw, &argv[1], PTR_DOMAIN_POLY)) {
		pcb_message(PCB_MSG_ERROR, "PolyNewEnd: invalid polygon pointer\n");
		PCB_ACT_IRES(0);
		return 0;
	}
	if (poly->PointN < 3) {
		pcb_message(PCB_MSG_ERROR, "PolyNewEnd: can not finish polygon, need at least 3 points\n");
		PCB_ACT_IRES(0);
		return 0;
	}

	pcb_poly_init_clip(data, layer, poly);
	pcb_add_poly_on_layer(layer, poly);
	res->type = FGW_LONG;
	res->val.nat_long = poly->ID;
	fgw_ptr_unreg(&pcb_fgw, &argv[1], PTR_DOMAIN_POLY);
	return 0;
}

pcb_action_t act_draw_action_list[] = {
	{"GetValue", pcb_act_GetValue, pcb_acth_GetValue, pcb_acts_GetValue},
	{"LineNew", pcb_act_LineNew, pcb_acth_LineNew, pcb_acts_LineNew},
	{"ArcNew", pcb_act_ArcNew, pcb_acth_ArcNew, pcb_acts_ArcNew},
	{"TextNew", pcb_act_TextNew, pcb_acth_TextNew, pcb_acts_TextNew},
	{"PstkNew", pcb_act_PstkNew, pcb_acth_PstkNew, pcb_acts_PstkNew},
	{"PolyNewFromRectangle", pcb_act_PolyNewFromRectangle, pcb_acth_PolyNewFromRectangle, pcb_acts_PolyNewFromRectangle},
	{"PolyNewFromPoints", pcb_act_PolyNewFromPoints, pcb_acth_PolyNewFromPoints, pcb_acts_PolyNewFromPoints},
	{"PolyNew", pcb_act_PolyNew, pcb_acth_PolyNew, pcb_acts_PolyNew},
	{"PolyNewPoints", pcb_act_PolyNewPoints, pcb_acth_PolyNewPoints, pcb_acts_PolyNewPoints},
	{"PolyNewEnd", pcb_act_PolyNewEnd, pcb_acth_PolyNewEnd, pcb_acts_PolyNewEnd},
};

static const char *act_draw_cookie = "act_draw";

PCB_REGISTER_ACTIONS(act_draw_action_list, act_draw_cookie)

int pplg_check_ver_act_draw(int ver_needed) { return 0; }

void pplg_uninit_act_draw(void)
{
	pcb_remove_actions_by_cookie(act_draw_cookie);
}

#include "dolists.h"
int pplg_init_act_draw(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(act_draw_action_list, act_draw_cookie)
	return 0;
}
