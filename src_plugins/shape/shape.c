/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "shape.h"

#include "plugins.h"
#include "actions.h"

#include "board.h"
#include "buffer.h"
#include "compat_misc.h"
#include "conf_core.h"
#include "data.h"
#include "error.h"
#include "event.h"
#include "layer.h"
#include "math_helper.h"
#include "obj_poly.h"
#include "obj_poly_draw.h"
#include "rotate.h"
#include "tool.h"

const char *pcb_shape_corner_name[] = {"Rn", "Ch", "Sq", NULL};

const char *pcb_shape_cookie = "shape plugin";
static pcb_layer_t pcb_shape_current_layer_;
pcb_layer_t *pcb_shape_current_layer = &pcb_shape_current_layer_;

static pcb_poly_t *regpoly(pcb_layer_t *layer, int corners, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy)
{
	int n, flags = PCB_FLAG_CLEARPOLY;
	double cosra, sinra, a, da = 2*M_PI / (double)corners;
	pcb_poly_t *p;

	if (corners < 3)
		return NULL;

	if ((rx < 10) || (ry < 10))
		return NULL;

	if ((rot_deg >= 360.0) || (rot_deg <= -360.0))
		rot_deg = fmod(rot_deg, 360.0);

	if (conf_core.editor.full_poly)
		flags |= PCB_FLAG_FULLPOLY;
	if (conf_core.editor.clear_polypoly)
		flags |= PCB_FLAG_CLEARPOLYPOLY;

	p = pcb_poly_new(layer, 2 * conf_core.design.clearance, pcb_flag_make(flags));
	if (p == NULL)
		return NULL;

	if (rot_deg != 0.0) {
		cosra = cos(rot_deg / PCB_RAD_TO_DEG);
		sinra = sin(rot_deg / PCB_RAD_TO_DEG);
	}

	for(n = 0,a = 0; n < corners; n++,a+=da) {
		pcb_coord_t x,  y;
		x = pcb_round(cos(a) * (double)rx + (double)cx);
		y = pcb_round(sin(a) * (double)ry + (double)cy);
		if (rot_deg != 0.0)
			pcb_rotate(&x, &y, cx, cy, cosra, sinra);
		pcb_poly_point_new(p, x, y);
	}

	pcb_add_poly_on_layer(layer, p);

	return p;
}

static void elarc90(pcb_poly_t *p, pcb_coord_t ccx, pcb_coord_t ccy, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy, pcb_coord_t ex, pcb_coord_t ey, pcb_coord_t rx, pcb_coord_t ry, double sa, int segs, int need_rot, double cosra, double sinra, pcb_coord_t rotcx, pcb_coord_t rotcy, pcb_shape_corner_t style)
{
	pcb_coord_t lx, ly;
	double da = M_PI/2.0/((double)segs-1);
	int n;

	if (style == PCB_CORN_SQUARE) {
		/* add original corner point */
		pcb_poly_point_new(p, ccx, ccy);
		return;
	}

	/* add exact start point */
	if (need_rot)
		pcb_rotate(&sx, &sy, rotcx, rotcy, cosra, sinra);
	pcb_poly_point_new(p, sx, sy);
	lx = sx;
	ly = sy;

	if (style == PCB_CORN_ROUND) {
	/* add approximated ellipse points */
	segs -= 2;
	for(n = 0,sa+=da; n < segs; n++,sa+=da) {
		pcb_coord_t x, y;
		x = pcb_round((double)cx + cos(sa) * (double)rx);
		y = pcb_round((double)cy - sin(sa) * (double)ry);
		if (need_rot)
			pcb_rotate(&x, &y, rotcx, rotcy, cosra, sinra);
		if ((x != lx) || (y != ly))
			pcb_poly_point_new(p, x, y);
		lx = x;
		ly = y;
	}
	}

	/* add exact end point */
	if (need_rot)
		pcb_rotate(&ex, &ey, rotcx, rotcy, cosra, sinra);
	if ((ex != lx) || (ey != ly))
		pcb_poly_point_new(p, ex, ey);
}

#define CORNER(outx, outy, rect_signx, rect_signy, rsignx, rsigny) \
	outx = pcb_round((double)cx + rect_signx * (double)w/2 + rsignx*rx); \
	outy = pcb_round((double)cy + rect_signy * (double)h/2 + rsigny*ry);
static pcb_poly_t *roundrect(pcb_layer_t *layer, pcb_coord_t w, pcb_coord_t h, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy, pcb_shape_corner_t corner[4])
{
	pcb_poly_t *p;
	pcb_coord_t maxr = (w < h ? w : h) / 2, x, y, ex, ey, acx, acy, ccx, ccy;
	int segs, need_rot, flags = PCB_FLAG_CLEARPOLY;
	double cosra, sinra;

	if ((w <= 10) || (h <= 10))
		return NULL;

	if ((rx > maxr) || (ry > maxr))
		return NULL;

	if (rot_deg != 0.0) {
		cosra = cos(rot_deg / PCB_RAD_TO_DEG);
		sinra = sin(rot_deg / PCB_RAD_TO_DEG);
		need_rot = 1;
	}
	else
		need_rot = 0;

	segs = sqrt((double)(rx+ry) / (double)PCB_MM_TO_COORD(0.1));
	if (segs < 3)  segs = 3;
	if (segs > 15) segs = 15;

	p = pcb_poly_new(layer, 2 * conf_core.design.clearance, pcb_flag_make(flags));
	if (p == NULL)
		return NULL;

	/* top right */
	CORNER(  x,   y, +1, -1,  0, +1); /* start */
	CORNER(acx, acy, +1, -1, -1, +1);  /* center */
	CORNER( ex,  ey, +1, -1, -1,  0); /* end */
	CORNER(ccx, ccy, +1, -1,  0,  0); /* corner point */
	elarc90(p, ccx, ccy, acx, acy, x, y, ex, ey, rx, ry, 0, segs, need_rot, cosra, sinra, cx, cy, corner[1]);

	/* top left */
	CORNER(  x,   y, -1, -1, +1,  0); /* start */
	CORNER(acx, acy, -1, -1, +1, +1);  /* center */
	CORNER( ex,  ey, -1, -1,  0, +1); /* end */
	CORNER(ccx, ccy, -1, -1,  0,  0); /* corner point */
	elarc90(p, ccx, ccy, acx, acy, x, y, ex, ey, rx, ry, M_PI/2.0, segs, need_rot, cosra, sinra, cx, cy, corner[0]);

	/* bottom left */
	CORNER(  x,   y, -1, +1,  0, -1); /* start */
	CORNER(acx, acy, -1, +1, +1, -1);  /* center */
	CORNER( ex,  ey, -1, +1, +1,  0); /* end */
	CORNER(ccx, ccy, -1, +1,  0,  0); /* corner point */
	elarc90(p, ccx, ccy, acx, acy, x, y, ex, ey, rx, ry, M_PI, segs, need_rot, cosra, sinra, cx, cy, corner[2]);

	/* bottom right*/
	CORNER(  x,   y, +1, +1, -1,  0); /* start */
	CORNER(acx, acy, +1, +1, -1, -1);  /* center */
	CORNER( ex,  ey, +1, +1,  0, -1); /* end */
	CORNER(ccx, ccy, +1, +1,  0,  0); /* corner point */
	elarc90(p, ccx, ccy, acx, acy, x, y, ex, ey, rx, ry, M_PI*3.0/2.0, segs, need_rot, cosra, sinra, cx, cy, corner[3]);

	pcb_add_poly_on_layer(layer, p);

	return p;
}
#undef CORNER

static pcb_poly_t *any_poly_place(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *p)
{
	if (p == NULL)
		return NULL;

	pcb_poly_init_clip(PCB->Data, CURRENT, p);
	pcb_poly_invalidate_draw(CURRENT, p);

	if (data != PCB->Data) {
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		pcb_copy_obj_to_buffer(PCB, data, PCB->Data, PCB_OBJ_POLY, CURRENT, p, p);
		pcb_r_delete_entry(CURRENT->polygon_tree, (pcb_box_t *)p);
		pcb_poly_free_fields(p);
		pcb_poly_free(p);
		pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
	}
	return p;
}

static pcb_poly_t *regpoly_place(pcb_data_t *data, pcb_layer_t *layer, int corners, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy)
{
	pcb_poly_t *p;

	if (layer == pcb_shape_current_layer)	
		layer = CURRENT;

	p = regpoly(layer, corners, rx, ry, rot_deg, cx, cy);
	return any_poly_place(data, layer, p);
}

static pcb_poly_t *roundrect_place(pcb_data_t *data, pcb_layer_t *layer, pcb_coord_t w, pcb_coord_t h, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy, pcb_shape_corner_t corner[4])
{
	pcb_poly_t *p;

	if (layer == pcb_shape_current_layer)
		layer = CURRENT;

	p = roundrect(layer, w, h, rx, ry, rot_deg, cx, cy, corner);
	return any_poly_place(data, layer, p);
}

static pcb_line_t *circle_place(pcb_data_t *data, pcb_layer_t *layer, pcb_coord_t dia, pcb_coord_t cx, pcb_coord_t cy)
{
	int flags = 0;
	pcb_line_t *l;

	if (layer == pcb_shape_current_layer)
		layer = CURRENT;

	if (conf_core.editor.clear_line)
		flags |= PCB_FLAG_CLEARLINE;

	l = pcb_line_new(layer, cx, cy, cx, cy, dia, conf_core.design.clearance*2, pcb_flag_make(flags));
	if (l != NULL) {
		if ((conf_core.editor.clear_line) && (data == PCB->Data))
			pcb_poly_clear_from_poly(data, PCB_OBJ_LINE, layer, l);

		if (data != PCB->Data) {
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			pcb_copy_obj_to_buffer(PCB, data, PCB->Data, PCB_OBJ_LINE, CURRENT, l, l);
			pcb_r_delete_entry(CURRENT->line_tree, (pcb_box_t *)l);
			pcb_line_free(l);
			pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
		}
	}
	return l;
}

static int get_where(const char *arg, pcb_data_t **data, pcb_coord_t *x, pcb_coord_t *y, pcb_bool *have_coords)
{
	const char *dst;
	char *end;
	int a = 0;
	pcb_bool succ;

	dst = arg;
	if (pcb_strncasecmp(dst, "buffer", 6) == 0) {
		*data = PCB_PASTEBUFFER->Data;
		dst += 6;
		a = 1;
	}
	else
		*data = PCB->Data;

	end = strchr(dst, ';');
	if (end != NULL) {
		char *sx, *sy, *tmp;
		int offs = end - dst;
		*have_coords = pcb_true;
		a = 1;
		
		tmp = pcb_strdup(dst);
		tmp[offs] = '\0';
		sx = tmp;
		sy = tmp + offs + 1;
		*x = pcb_get_value(sx, NULL, NULL, &succ);
		if (succ)
			*y = pcb_get_value(sy, NULL, NULL, &succ);
		free(tmp);
		if (!succ) {
			pcb_message(PCB_MSG_ERROR, "regpoly(): invalid center coords '%s'\n", dst);
			return -1;
		}
	}
	return a;
}

static pcb_bool parse2coords(const char *arg, pcb_coord_t *rx, pcb_coord_t *ry)
{
	const char *dst;
	char *end;
	pcb_bool succ;

	if (arg == NULL)
		return pcb_false;

	dst = arg;
	end = strchr(dst, ';');
	if (end != NULL) {
		char *sx, *sy, *tmp = pcb_strdup(dst);
		int offs = end - dst;
		tmp[offs] = '\0';
		sx = tmp;
		sy = tmp + offs + 1;
		*rx = pcb_get_value(sx, NULL, NULL, &succ);
		if (succ)
			*ry = pcb_get_value(sy, NULL, NULL, &succ);
		free(tmp);
	}
	else
		*rx = *ry = pcb_get_value(dst, NULL, NULL, &succ);
	return succ;
}

static const char pcb_acts_regpoly[] = "regpoly([where,] corners, radius [,rotation])";
static const char pcb_acth_regpoly[] = "Generate regular polygon. Where is x;y and radius is either r or rx;ry. Rotation is in degrees.";
fgw_error_t pcb_act_regpoly(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *args[6];
	double rot = 0;
	pcb_coord_t x = 0, y = 0, rx, ry = 0;
	pcb_bool succ, have_coords = pcb_false;
	int corners = 0, a, n;
	pcb_data_t *data;
	char *end;

	if (argc < 3) {
		pcb_message(PCB_MSG_ERROR, "regpoly() needs at least two parameters\n");
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (argc > 5) {
		pcb_message(PCB_MSG_ERROR, "regpoly(): too many arguments\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	memset(args, 0, sizeof(args));
	for(n = 1; n < argc; n++)
		PCB_ACT_MAY_CONVARG(n, FGW_STR, regpoly, args[n-1] = argv[n].val.str);

	a = get_where(args[0], &data, &x, &y, &have_coords);
	if (a < 0) {
		PCB_ACT_IRES(-1);
		return 0;
	}

	corners = strtol(args[a], &end, 10);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "regpoly(): invalid number of corners '%s'\n", args[a]);
		PCB_ACT_IRES(-1);
		return 0;
	}
	a++;

	/* convert radii */
	succ = parse2coords(args[a], &rx, &ry);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "regpoly(): invalid radius '%s'\n", args[a]);
		PCB_ACT_IRES(-1);
		return 0;
	}
	a++;

	if (a < argc-1) {
		rot = strtod(args[a], &end);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "regpoly(): invalid rotation '%s'\n", args[a]);
			PCB_ACT_IRES(-1);
			return 0;
		}
	}

	if ((data == PCB->Data) && (!have_coords))
		pcb_hid_get_coords("Click on the center of the polygon", &x, &y, 0);

	if (regpoly_place(data, CURRENT, corners, rx, ry, rot, x, y) == NULL)
		pcb_message(PCB_MSG_ERROR, "regpoly(): failed to create the polygon\n");

	PCB_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_roundrect[] = "roundrect([where,] width[;height] [,rx[;ry] [,rotation]])";
static const char pcb_acth_roundrect[] = "Generate a rectangle with round corners";
fgw_error_t pcb_act_roundrect(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *args[8];
	int n, a;
	pcb_data_t *data;
	pcb_bool succ, have_coords = pcb_false;
	pcb_coord_t x = 0, y = 0, w, h, rx, ry;
	double rot = 0.0;
	char *end;
	pcb_shape_corner_t corner[4] = { PCB_CORN_ROUND, PCB_CORN_ROUND, PCB_CORN_ROUND, PCB_CORN_ROUND};

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "roundrect() needs at least one parameters\n");
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (argc > 6) {
		pcb_message(PCB_MSG_ERROR, "roundrect(): too many arguments\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	memset(args, 0, sizeof(args));
	for(n = 1; n < argc; n++)
		PCB_ACT_MAY_CONVARG(n, FGW_STR, regpoly, args[n-1] = argv[n].val.str);

	if (argc > 2) {
		a = get_where(args[0], &data, &x, &y, &have_coords);
		if (a < 0) {
			PCB_ACT_IRES(-1);
			return 0;
		}
	}
	else {
		a = 0;
		data = PCB->Data;
		have_coords = 1;
		pcb_hid_get_coords("Click on the center of the rectangle", &x, &y, 0);
	}

	/* convert width;height */
	succ = parse2coords(args[a], &w, &h);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "roundrect(): invalid width or height '%s'\n", args[a]);
		PCB_ACT_IRES(-1);
		return 0;
	}
	a++;

	/* convert radii */
	if (a < argc-1) {
		succ = parse2coords(args[a], &rx, &ry);
		if (!succ) {
			pcb_message(PCB_MSG_ERROR, "roundrect(): invalid width or height '%s'\n", args[a]);
			PCB_ACT_IRES(-1);
			return 0;
		}
		a++;
	}
	else
		rx = ry = (w+h)/16; /* 1/8 of average w & h */

	if (a < argc-1) {
		rot = strtod(args[a], &end);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "roundrect(): invalid rotation '%s'\n", args[a]);
			PCB_ACT_IRES(-1);
			return 0;
		}
	}

	if ((data == PCB->Data) && (!have_coords))
		pcb_hid_get_coords("Click on the center of the polygon", &x, &y, 0);

	if (roundrect_place(data, CURRENT, w, h, rx, ry, rot, x, y, corner) == NULL)
		pcb_message(PCB_MSG_ERROR, "roundrect(): failed to create the polygon\n");

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_circle[] = "circle([where,] diameter)";
static const char pcb_acth_circle[] = "Generate a filled circle (zero length round cap line)";
fgw_error_t pcb_act_circle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *args[6];
	int n, a;
	pcb_data_t *data;
	pcb_bool succ, have_coords = pcb_false;
	pcb_coord_t x = 0, y = 0, dia;

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "circle() needs at least one parameters (diameter)\n");
		PCB_ACT_IRES(-1);
		return 0;
	}
	if (argc > 3) {
		pcb_message(PCB_MSG_ERROR, "circle(): too many arguments\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	memset(args, 0, sizeof(args));
	for(n = 1; n < argc; n++)
		PCB_ACT_MAY_CONVARG(n, FGW_STR, circle, args[n-1] = argv[n].val.str);

	a = get_where(args[0], &data, &x, &y, &have_coords);
	if (a < 0) {
		PCB_ACT_IRES(-1);
		return 0;
	}

	dia = pcb_get_value(args[a], NULL, NULL, &succ);
	if (!succ) {
		pcb_message(PCB_MSG_ERROR, "circle(): failed to convert dia: invalid coord (%s)\n", args[a]);
		PCB_ACT_IRES(1);
		return 0;
	}

	if ((dia < 1) || (dia > (PCB->MaxWidth + PCB->MaxHeight)/4)) {
		pcb_message(PCB_MSG_ERROR, "circle(): invalid diameter\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	if ((data == PCB->Data) && (!have_coords))
		pcb_hid_get_coords("Click on the center of the circle", &x, &y, 0);

	if (circle_place(PCB->Data, CURRENT, dia, x, y) == NULL)
		pcb_message(PCB_MSG_ERROR, "circle(): failed to create the polygon\n");

	PCB_ACT_IRES(0);
	return 0;
}

#include "shape_dialog.c"

pcb_action_t shape_action_list[] = {
	{"regpoly", pcb_act_regpoly, pcb_acth_regpoly, pcb_acts_regpoly},
	{"roundrect", pcb_act_roundrect, pcb_acth_roundrect, pcb_acts_roundrect},
	{"circle", pcb_act_circle, pcb_acth_circle, pcb_acts_circle},
	{"shape", pcb_act_shape, pcb_acth_shape, pcb_acts_shape}
};

PCB_REGISTER_ACTIONS(shape_action_list, pcb_shape_cookie)

int pplg_check_ver_shape(int ver_needed) { return 0; }

void pplg_uninit_shape(void)
{
	pcb_event_unbind_allcookie(pcb_shape_cookie);
	pcb_remove_actions_by_cookie(pcb_shape_cookie);
}


#include "dolists.h"
int pplg_init_shape(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(shape_action_list, pcb_shape_cookie)

	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, shape_layer_chg, NULL, pcb_shape_cookie);

	return 0;
}
