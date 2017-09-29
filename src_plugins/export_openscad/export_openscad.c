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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
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
#include <genht/htsp.h>

#include "compat_misc.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "layer.h"
#include "math_helper.h"
#include "misc_util.h"
#include "plugins.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_helper.h"
#include "hid_flags.h"


static pcb_hid_t openscad_hid;

const char *openscad_cookie = "openscad HID";


typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
} hid_gc_s;

static FILE *f = NULL;
unsigned layer_open;
double layer_thickness = 0.01;
gds_t layer_calls, model_calls;

pcb_hid_attribute_t openscad_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 DXF Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_openscadfile 0
};

#define NUM_OPTIONS (sizeof(openscad_attribute_list)/sizeof(openscad_attribute_list[0]))

#include "scad_draw.c"
#include "scad_models.c"

PCB_REGISTER_ATTRIBUTES(openscad_attribute_list, openscad_cookie)

static pcb_hid_attr_val_t openscad_values[NUM_OPTIONS];

static pcb_hid_attribute_t *openscad_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".scad";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &openscad_attribute_list[HA_openscadfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return openscad_attribute_list;
}

void openscad_hid_export_to_file(FILE * the_file, pcb_hid_attr_val_t * options)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	pcb_hid_expose_ctx_t ctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->MaxWidth;
	ctx.view.Y2 = PCB->MaxHeight;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	conf_force_set_bool(conf_core.editor.thin_draw, 0);
	conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	pcb_hid_expose_all(&openscad_hid, &ctx);

	conf_update(NULL, -1); /* restore forced sets */
}

static void scad_close_layer()
{
	if (layer_open) {
		fprintf(f, "	}\n");
		fprintf(f, "}\n\n");
		layer_open = 0;
	}
}

static void scad_new_layer(const char *layer_name, int level, const char *color)
{
	double h;

	scad_close_layer();

	if (level > 0)
		h = 0.8+(double)level * layer_thickness;
	else
		h = -0.8+(double)level * layer_thickness;

	fprintf(f, "module layer_%s() {\n", layer_name);
	fprintf(f, "	color([%s])\n", color);
	fprintf(f, "		translate([0,0,%f]) {\n", h);
	layer_open = 1;

	pcb_append_printf(&layer_calls, "	layer_%s();\n", layer_name);
}

static void openscad_do_export(pcb_hid_attr_val_t * options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER + 2];
	int i;

	if (!options) {
		openscad_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			openscad_values[i] = openscad_attribute_list[i].default_val;
		options = openscad_values;
	}

	filename = options[HA_openscadfile].str_value;
	if (!filename)
		filename = "pcb.openscad";

	f = pcb_fopen(filename, "wb");
	if (!f) {
		perror(filename);
		return;
	}

	pcb_hid_save_and_show_layer_ons(save_ons);

	scad_draw_primitives();

	if (scad_draw_outline() < 0)
		return;

	gds_init(&layer_calls);
	gds_init(&model_calls);

	scad_insert_models();

	openscad_hid_export_to_file(f, options);
	scad_close_layer();

	scad_draw_drills();
	scad_draw_finish();

	pcb_hid_restore_layer_ons(save_ons);

	gds_uninit(&layer_calls);
	gds_uninit(&model_calls);

	fclose(f);
	f = NULL;
}

static void openscad_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(openscad_attribute_list, sizeof(openscad_attribute_list) / sizeof(openscad_attribute_list[0]), openscad_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}



static int openscad_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	if (flags & PCB_LYT_UI)
		return 0;

	if (flags & PCB_LYT_INVIS)
		return 0;

	if (flags & PCB_LYT_OUTLINE) {
		return 0;
	}

	if (flags & PCB_LYT_PDRILL)
		return 0;

	if (flags & PCB_LYT_UDRILL)
		return 0;

	if (flags & PCB_LYT_SILK) {
		if (flags & PCB_LYT_TOP) {
			scad_new_layer("top_silk", +2, "0,0,0");
			return 1;
		}
		if (flags & PCB_LYT_BOTTOM) {
			scad_new_layer("bottom_silk", -2, "0,0,0");
			return 1;
		}
	}

	if (flags & PCB_LYT_COPPER) {
		if (flags & PCB_LYT_TOP) {
			scad_new_layer("top_copper", +1, "1,0.4,0.2");
			return 1;
		}
		if (flags & PCB_LYT_BOTTOM) {
			scad_new_layer("bottom_copper", -1, "1,0.4,0.2");
			return 1;
		}
	}

	return 0;
}


static pcb_hid_gc_t openscad_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &openscad_hid;
	return rv;
}

static void openscad_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void openscad_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	if (direct)
		return;

	switch(op) {
		case PCB_HID_COMP_RESET:
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_NEGATIVE:
			break;

		case PCB_HID_COMP_FLUSH:
			break;
	}
}

static void openscad_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void openscad_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void openscad_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}


static void openscad_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

#define fix_rect_coords() \
	if (x1 > x2) {\
		pcb_coord_t t = x1; \
		x1 = x2; \
		x2 = t; \
	} \
	if (y1 > y2) { \
		pcb_coord_t t = y1; \
		y1 = y2; \
		y2 = t; \
	}


static void openscad_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	fix_rect_coords();
	pcb_fprintf(f, "			pcb_fill_rect(%mm, %mm, %mm, %mm, %f, %f);\n",
		x1, y1, x2, y2, 0.0, layer_thickness);
}

static void openscad_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double length, angle;
	const char *cap_style;

	length = pcb_distance(x1, y1, x2, y2);
	angle = atan2((double)y2-y1, (double)x2-x1);

	if (gc->cap == Square_Cap)
		cap_style = "sc";
	else
		cap_style = "rc";

	pcb_fprintf(f, "			pcb_line_%s(%mm, %mm, %mm, %f, %mm, %f);\n", cap_style,
		x1, y1, (pcb_coord_t)pcb_round(length), angle * PCB_RAD_TO_DEG, gc->width, layer_thickness);
}

static void openscad_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	fix_rect_coords();
	openscad_draw_line(gc, x1, y1, x2, y1);
	openscad_draw_line(gc, x2, y1, x2, y2);
	openscad_draw_line(gc, x2, y2, x1, y2);
	openscad_draw_line(gc, x1, y2, x1, y1);
}

static void openscad_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	TRX(cx); TRY(cy);
}

static void openscad_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	pcb_fprintf(f, "			pcb_fcirc(%mm, %mm, %mm, %f);\n", cx, cy, radius, layer_thickness);
}

static void openscad_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	int n;
	fprintf(f, "			pcb_fill_poly([");
	for(n = 0; n < n_coords-1; n++)
		pcb_fprintf(f, "[%mm,%mm],", x[n], y[n]);
	pcb_fprintf(f, "[%mm,%mm]], %f);\n", x[n], y[n], layer_thickness);
}

static void openscad_calibrate(double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "openscad_calibrate() not implemented");
	return;
}

static void openscad_set_crosshair(int x, int y, int a)
{
}

static int openscad_usage(const char *topic)
{
	fprintf(stderr, "\nopenscad exporter command line arguments:\n\n");
	pcb_hid_usage(openscad_attribute_list, sizeof(openscad_attribute_list) / sizeof(openscad_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x openscad [openscad options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_openscad(int ver_needed) { return 0; }

void pplg_uninit_export_openscad(void)
{
	pcb_hid_remove_attributes_by_cookie(openscad_cookie);
}

int pplg_init_export_openscad(void)
{
	memset(&openscad_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&openscad_hid);
	pcb_dhlp_draw_helpers_init(&openscad_hid);

	openscad_hid.struct_size = sizeof(pcb_hid_t);
	openscad_hid.name = "openscad";
	openscad_hid.description = "OpenSCAD exporter";
	openscad_hid.exporter = 1;
	openscad_hid.holes_after = 1;

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
	openscad_hid.fill_rect = openscad_fill_rect;
	openscad_hid.calibrate = openscad_calibrate;
	openscad_hid.set_crosshair = openscad_set_crosshair;

	openscad_hid.usage = openscad_usage;

	pcb_hid_register_hid(&openscad_hid);

	return 0;
}
