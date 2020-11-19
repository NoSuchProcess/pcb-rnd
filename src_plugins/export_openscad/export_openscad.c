/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <genvector/gds_char.h>
#include <genvector/vti0.h>
#include <genht/htsp.h>

#include <librnd/core/compat_misc.h>
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/math_helper.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "obj_pstk_inlines.h"
#include "funchash_core.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>

#include <librnd/core/hid_init.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"


static rnd_hid_t openscad_hid;

const char *openscad_cookie = "openscad HID";


typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	int width;
} rnd_hid_gc_s;

static FILE *f = NULL;
static unsigned layer_open;
static double layer_thickness = 0.01, effective_layer_thickness;
static gds_t layer_group_calls, model_calls;
static const char *scad_group_name;
static int scad_group_level;
static const char *scad_group_color;
static int scad_layer_cnt;
static vti0_t scad_comp;

static rnd_hid_attr_val_t *openscad_options;

rnd_export_opt_t openscad_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 DXF Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_openscadfile 0

	{"copper", "enable exporting outer copper layers",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_copper 1

	{"silk", "enable exporting silk layers",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_silk 2

	{"mask", "enable exporting mask layers",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_mask 3

	{"models", "enable searching and inserting model files",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_models 4

	{"drill", "enable drilling holes",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_drill 5

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 6

};

#define NUM_OPTIONS (sizeof(openscad_attribute_list)/sizeof(openscad_attribute_list[0]))

#include "scad_draw.c"
#include "scad_models.c"

static rnd_hid_attr_val_t openscad_values[NUM_OPTIONS];

static rnd_export_opt_t *openscad_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".scad";
	char **val = openscad_attribute_list[HA_openscadfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &openscad_attribute_list[HA_openscadfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return openscad_attribute_list;
}

void openscad_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	openscad_options = options;
	rnd_expose_main(&openscad_hid, &ctx, NULL);
	openscad_options = NULL;

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void scad_close_layer()
{
	if (layer_open) {
		fprintf(f, "		}\n");
		fprintf(f, "}\n\n");
		layer_open = 0;
	}
}

static void scad_dump_comp(void)
{
	int n, closes = 0;

/*
	a layer list like this: +4 -5 -0 +6 -7
	becomes script like this:

	difference()
	{
		union() {
			difference() {
				4
				5
				0
			}
			6
		}
		7
	}

*/

	/* add an union or difference on polarity change */
	for(n = vti0_len(&scad_comp)-2; n >= 0; n--) {
		int p1 = (scad_comp.array[n]) > 0, p2 = (scad_comp.array[n+1] > 0);
		if (p2 && !p1) {
			fprintf(f, "	union() {\n");
			closes++;
		}
		if (!p2 && p1) {
			fprintf(f, "	difference() {\n");
			closes++;
		}
	}

	/* list layer calls, add a close on polarity change */
	for(n = 0; n < vti0_len(&scad_comp); n++) {
		int id = scad_comp.array[n], is_pos = id > 0;
		if (id < 0)
			id = -id;
		fprintf(f, "	layer_%s_%s_%d();\n", scad_group_name, is_pos ? "pos" : "neg", id);
		if ((n > 0) && (n < vti0_len(&scad_comp)-1)) {
			int id2 = scad_comp.array[n+1];
			int p1 = is_pos, p2 = (id2 > 0);
			if (p2 != p1) {
				fprintf(f, "}\n");
				closes--;
			}
		}
	}

	/* the first open needs to be closed manually */
	if (closes)
		fprintf(f, "}\n");
}

static void scad_close_layer_group()
{
	if (scad_group_name != NULL) {
		fprintf(f, "module layer_group_%s() {\n", scad_group_name);
		scad_dump_comp();
		fprintf(f, "}\n\n");

		rnd_append_printf(&layer_group_calls, "	layer_group_%s();\n", scad_group_name);
		scad_group_name = NULL;
		scad_group_color = NULL;
		scad_group_level = 0;
		vti0_truncate(&scad_comp, 0);
	}
}

static void scad_new_layer(int is_pos)
{
	double h;
	scad_layer_cnt++;

	if (is_pos)
		vti0_append(&scad_comp, scad_layer_cnt);
	else
		vti0_append(&scad_comp, -scad_layer_cnt);

	scad_close_layer();

	if (is_pos) {
		effective_layer_thickness = layer_thickness;
		if (scad_group_level > 0)
			h = 0.8+((double)scad_group_level*1.1) * effective_layer_thickness;
		else
			h = -0.8+((double)scad_group_level*1.1) * effective_layer_thickness;
	}
	else {
		double mult = 2.0;
		effective_layer_thickness = layer_thickness * mult;
		if (scad_group_level > 0)
			h = 0.8+((double)scad_group_level*1.1) * layer_thickness;
		else
			h = -0.8+((double)scad_group_level*1.1) * layer_thickness;
		h -= effective_layer_thickness/2.0;
		effective_layer_thickness+=1.0;
	}
	fprintf(f, "module layer_%s_%s_%d() {\n", scad_group_name, is_pos ? "pos" : "neg", scad_layer_cnt);
	fprintf(f, "	color([%s])\n", scad_group_color);
	fprintf(f, "		translate([0,0,%f]) {\n", h);
	layer_open = 1;

}


static void scad_new_layer_group(const char *group_name, int level, const char *color)
{
	scad_close_layer_group();

	scad_group_name = group_name;
	scad_group_level = level;
	scad_group_color = color;
	scad_group_level = level;
}


static void openscad_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	pcb_cam_t cam;

	if (!options) {
		openscad_get_export_options(hid, 0);
		options = openscad_values;
	}

	filename = options[HA_openscadfile].str;
	if (!filename)
		filename = "pcb.openscad";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &filename);

	f = rnd_fopen_askovr(&PCB->hidlib, filename, "wb", NULL);
	if (!f) {
		perror(filename);
		return;
	}

	pcb_hid_save_and_show_layer_ons(save_ons);

	scad_layer_cnt = 0;
	scad_draw_primitives();

	if (scad_draw_outline() < 0)
		return;

	gds_init(&layer_group_calls);
	gds_init(&model_calls);
	vti0_init(&scad_comp);

	if (options[HA_models].lng)
		scad_insert_models();

	openscad_hid_export_to_file(f, options);
	scad_close_layer_group();

	if (options[HA_drill].lng)
		scad_draw_drills();

	scad_draw_finish(options);

	pcb_hid_restore_layer_ons(save_ons);

	gds_uninit(&layer_group_calls);
	gds_uninit(&model_calls);
	vti0_uninit(&scad_comp);

	fclose(f);
	f = NULL;
	pcb_cam_end(&cam);
}

static int openscad_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, openscad_attribute_list, sizeof(openscad_attribute_list) / sizeof(openscad_attribute_list[0]), openscad_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}



static int openscad_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (flags & PCB_LYT_UI)
		return 0;

	if (flags & PCB_LYT_INVIS)
		return 0;

	if (PCB_LAYER_IS_ROUTE(flags, purpi)) {
		return 0;
	}

	if (PCB_LAYER_IS_DRILL(flags, purpi))
		return 0;

	if (flags & PCB_LYT_MASK) {
		if (!openscad_options[HA_mask].lng)
			return 0;
		if (flags & PCB_LYT_TOP) {
			scad_new_layer_group("top_mask", +2, "0,0.7,0,0.5");
			return 1;
		}
		if (flags & PCB_LYT_BOTTOM) {
			scad_new_layer_group("bottom_mask", -2, "0,0.7,0,0.5");
			return 1;
		}
	}

	if (flags & PCB_LYT_SILK) {
		if (!openscad_options[HA_silk].lng)
			return 0;
		if (flags & PCB_LYT_TOP) {
			scad_new_layer_group("top_silk", +3, "0,0,0");
			return 1;
		}
		if (flags & PCB_LYT_BOTTOM) {
			scad_new_layer_group("bottom_silk", -3, "0,0,0");
			return 1;
		}
	}

	if (flags & PCB_LYT_COPPER) {
		if (!openscad_options[HA_copper].lng) {
			printf("skip copper\n");
			return 0;
		}
		if (flags & PCB_LYT_TOP) {
			scad_new_layer_group("top_copper", +1, "1,0.4,0.2");
			return 1;
		}
		if (flags & PCB_LYT_BOTTOM) {
			scad_new_layer_group("bottom_copper", -1, "1,0.4,0.2");
			return 1;
		}
	}

	return 0;
}


static rnd_hid_gc_t openscad_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	rv->me_pointer = &openscad_hid;
	return rv;
}

static void openscad_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void openscad_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	switch(op) {
		case RND_HID_COMP_RESET:
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
			scad_new_layer(1);
			break;
		case RND_HID_COMP_NEGATIVE:
			scad_new_layer(0);
			break;

		case RND_HID_COMP_FLUSH:
			scad_close_layer();
			break;
	}
}

static void openscad_set_color(rnd_hid_gc_t gc, const rnd_color_t *name)
{
}

static void openscad_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void openscad_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}


static void openscad_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

#define fix_rect_coords() \
	if (x1 > x2) {\
		rnd_coord_t t = x1; \
		x1 = x2; \
		x2 = t; \
	} \
	if (y1 > y2) { \
		rnd_coord_t t = y1; \
		y1 = y2; \
		y2 = t; \
	}


static void openscad_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	TRX(x1); TRY(y1);
	TRX(x2); TRY(y2);

	fix_rect_coords();
	rnd_fprintf(f, "			pcb_fill_rect(%mm, %mm, %mm, %mm, %f, %f);\n",
		x1, y1, x2, y2, 0.0, effective_layer_thickness);
}

static void openscad_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	double length, angle;
	const char *cap_style;

	TRX(x1); TRY(y1);
	TRX(x2); TRY(y2);

	length = rnd_distance(x1, y1, x2, y2);
	angle = atan2((double)y2-y1, (double)x2-x1);

	if (gc->cap == rnd_cap_square)
		cap_style = "sc";
	else
		cap_style = "rc";

	rnd_fprintf(f, "			pcb_line_%s(%mm, %mm, %mm, %f, %mm, %f);\n", cap_style,
		x1, y1, (rnd_coord_t)rnd_round(length), angle * RND_RAD_TO_DEG, gc->width, effective_layer_thickness);
}

static void openscad_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	TRX(x1); TRY(y1);
	TRX(x2); TRY(y2);

	fix_rect_coords();
	openscad_draw_line(gc, x1, y1, x2, y1);
	openscad_draw_line(gc, x2, y1, x2, y2);
	openscad_draw_line(gc, x2, y2, x1, y2);
	openscad_draw_line(gc, x1, y2, x1, y1);
}

typedef struct {
	int first;
	rnd_coord_t lx, ly;
	rnd_hid_gc_t gc;
} openscad_draw_arc_t;

static int openscad_draw_arc_cb(void *uctx, rnd_coord_t x, rnd_coord_t y)
{
	openscad_draw_arc_t *ctx = uctx;

	if (!ctx->first)
		openscad_draw_line(ctx->gc, ctx->lx, ctx->ly, x, y);

	ctx->first = 0;
	ctx->lx = x; ctx->ly = y;
	return 0;
}

static void openscad_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	pcb_arc_t a;
	openscad_draw_arc_t ctx;
	rnd_coord_t res = (width+height)/20;

	a.X = cx; a.Y = cy;
	a.Width = width; a.Height = height;
	a.StartAngle = start_angle;
	a.Delta = delta_angle;

	ctx.first = 1;
	ctx.gc = gc;
	pcb_arc_approx(&a, -RND_MIN(RND_MM_TO_COORD(2), res), 0, &ctx, openscad_draw_arc_cb);
}

static void openscad_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	TRX(cx); TRY(cy);

	rnd_fprintf(f, "			pcb_fcirc(%mm, %mm, %mm, %f);\n", cx, cy, radius, effective_layer_thickness);
}

static void openscad_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int n;
	fprintf(f, "			pcb_fill_poly([");
	for(n = 0; n < n_coords-1; n++)
		rnd_fprintf(f, "[%mm,%mm],", TRX_(x[n]+dx), TRY_(y[n]+dy));
	rnd_fprintf(f, "[%mm,%mm]], %f);\n", TRX_(x[n]+dx), TRY_(y[n]+dy), effective_layer_thickness);
}

static void openscad_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	openscad_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void openscad_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	rnd_message(RND_MSG_ERROR, "openscad_calibrate() not implemented");
	return;
}

static void openscad_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int openscad_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nopenscad exporter command line arguments:\n\n");
	rnd_hid_usage(openscad_attribute_list, sizeof(openscad_attribute_list) / sizeof(openscad_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x openscad [openscad options] foo.pcb\n\n");
	return 0;
}


static const char pcb_acts_scad_export_poly[] = "ScadExportPoly(filename)\n";
static const char pcb_acth_scad_export_poly[] = "exports all selected polygons to an openscad script; only the outmost contour of each poly is exported";
static fgw_error_t pcb_act_scad_export_poly(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	FILE *f;
	const char *name;

	RND_ACT_CONVARG(1, FGW_STR, scad_export_poly, name = argv[1].val.str);

	f = rnd_fopen_askovr(&PCB->hidlib, name, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open %s for writing\n", name);
		RND_ACT_IRES(-1);
		return 0;
	}


	PCB_POLY_ALL_LOOP(PCB->Data); {
		pcb_poly_it_t it;
		rnd_polyarea_t *pa;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
			continue;

		/* iterate over all islands of a polygon */
		for(pa = pcb_poly_island_first(polygon, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			rnd_coord_t x, y;
			rnd_pline_t *pl;
			int go;

			/* check if we have a contour for the given island */
			pl = pcb_poly_contour(&it);
			if (pl != NULL) {
				int cnt;

				fprintf(f, "polygon([");
				/* iterate over the vectors of the contour */
				for(go = pcb_poly_vect_first(&it, &x, &y),cnt = 0; go; go = pcb_poly_vect_next(&it, &x, &y), cnt++)
					rnd_fprintf(f, "%s[%mm,%mm]", (cnt > 0 ? "," : ""), x, y);
				fprintf(f, "]);\n");
			}
		}
	} PCB_ENDALL_LOOP;

	fclose(f);

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t scad_action_list[] = {
	{"ExportScadPoly", pcb_act_scad_export_poly, pcb_acth_scad_export_poly, pcb_acts_scad_export_poly}
};

int pplg_check_ver_export_openscad(int ver_needed) { return 0; }

void pplg_uninit_export_openscad(void)
{
	rnd_export_remove_opts_by_cookie(openscad_cookie);
	rnd_remove_actions_by_cookie(openscad_cookie);
	rnd_hid_remove_hid(&openscad_hid);
}

int pplg_init_export_openscad(void)
{
	RND_API_CHK_VER;

	memset(&openscad_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&openscad_hid);

	openscad_hid.struct_size = sizeof(rnd_hid_t);
	openscad_hid.name = "openscad";
	openscad_hid.description = "OpenSCAD exporter";
	openscad_hid.exporter = 1;

	openscad_hid.get_export_options = openscad_get_export_options;
	openscad_hid.do_export = openscad_do_export;
	openscad_hid.parse_arguments = openscad_parse_arguments;
	openscad_hid.set_layer_group = openscad_set_layer_group;
	openscad_hid.make_gc = openscad_make_gc;
	openscad_hid.destroy_gc = openscad_destroy_gc;
	openscad_hid.set_drawing_mode = openscad_set_drawing_mode;
	openscad_hid.set_color = openscad_set_color;
	openscad_hid.set_line_cap = openscad_set_line_cap;
	openscad_hid.set_line_width = openscad_set_line_width;
	openscad_hid.set_draw_xor = openscad_set_draw_xor;
	openscad_hid.draw_line = openscad_draw_line;
	openscad_hid.draw_arc = openscad_draw_arc;
	openscad_hid.draw_rect = openscad_draw_rect;
	openscad_hid.fill_circle = openscad_fill_circle;
	openscad_hid.fill_polygon = openscad_fill_polygon;
	openscad_hid.fill_polygon_offs = openscad_fill_polygon_offs;
	openscad_hid.fill_rect = openscad_fill_rect;
	openscad_hid.calibrate = openscad_calibrate;
	openscad_hid.set_crosshair = openscad_set_crosshair;
	openscad_hid.argument_array = openscad_values;

	openscad_hid.usage = openscad_usage;

	rnd_hid_register_hid(&openscad_hid);

	RND_REGISTER_ACTIONS(scad_action_list, openscad_cookie)

	return 0;
}
