/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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

#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "layer.h"
#include "layer_vis.h"
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>

#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"

static rnd_hid_t debug_hid;

const char *debug_cookie = "debug HID";

static pcb_cam_t debug_cam;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_color_t color, last_color;
	rnd_cap_style_t style, last_style;
	rnd_coord_t capw, last_capw;
} rnd_hid_gc_s;

static FILE *f = NULL;

static const rnd_export_opt_t debug_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_debugfile 0

	{"compact", "Single line per object",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_compact 1

	{"stateless", "For stateless parsing: include gc in every object as a suffix instead of printing separate color/width/cap commands",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_stateless 2

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_cam 3
};

#define NUM_OPTIONS (sizeof(debug_attribute_list)/sizeof(debug_attribute_list[0]))

static rnd_hid_attr_val_t debug_values[NUM_OPTIONS];

static const rnd_export_opt_t *debug_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".dump";
	const char *val = debug_values[HA_debugfile].str;

	if ((PCB != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &debug_values[HA_debugfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return debug_attribute_list;
}

void debug_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.view.X1 = PCB->hidlib.dwg.X1;
	ctx.view.Y1 = PCB->hidlib.dwg.X1;
	ctx.view.X2 = PCB->hidlib.dwg.X2;
	ctx.view.Y2 = PCB->hidlib.dwg.Y2;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	rnd_app.expose_main(&debug_hid, &ctx, xform);

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void debug_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	if (!options) {
		debug_get_export_options(hid, 0);
		options = debug_values;
	}

	pcb_cam_begin(PCB, &debug_cam, &xform, options[HA_cam].str, debug_attribute_list, NUM_OPTIONS, options);

	if (debug_cam.fn_template == NULL) {
		filename = options[HA_debugfile].str;
		if (!filename)
			filename = "export.dump";

		f = rnd_fopen_askovr(&PCB->hidlib, debug_cam.active ? debug_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	if (!debug_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	debug_hid_export_to_file(f, options, &xform);

	if (!debug_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	if (f != NULL)
		fclose(f);
	f = NULL;

	if (!debug_cam.active) debug_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&debug_cam) == 0) {
		if (!debug_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "debug cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
}

static int debug_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, debug_attribute_list, sizeof(debug_attribute_list) / sizeof(debug_attribute_list[0]), debug_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int debug_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&debug_cam, group, purpose, purpi, flags, xform);


	if (!debug_cam.active) {
		if (flags & PCB_LYT_INVIS)
			return 0;

		if (flags & PCB_LYT_MASK) {
			if (!PCB->LayerGroups.grp[group].vis)
				return 0; /* not in photo mode or not visible */
		}
	}

	{
		gds_t tmp_ln;
		const char *name;
		gds_init(&tmp_ln);
		name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);
		fprintf(f, "layer %ld | %s\n", group, name);
		gds_uninit(&tmp_ln);
	}

	return 1;
}


static rnd_hid_gc_t debug_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	return rv;
}

static void debug_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void debug_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
/*	drawing_mode = op;*/

	if (direct)
		return;

	switch(op) {
		case RND_HID_COMP_RESET:
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
		case RND_HID_COMP_NEGATIVE:
			break;

		case RND_HID_COMP_FLUSH:
			break;
	}
}


static void debug_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	gc->color = *color;
}

static void use_gc(rnd_hid_gc_t gc)
{
	if (gc->color.packed != gc->last_color.packed) {
		if (!debug_values[HA_stateless].lng)
			fprintf(f, "color %d %d %d %d\n", gc->color.r, gc->color.g, gc->color.b, gc->color.a);
		gc->last_color = gc->color;
	}
	if (gc->style != gc->last_style) {
		const char *caps = NULL;
		switch(gc->style) {
			case rnd_cap_invalid: caps = "invalid"; break;
			case rnd_cap_square:  caps = "square"; break;
			case rnd_cap_round:   caps = "round"; break;
		}
		if (!debug_values[HA_stateless].lng)
			fprintf(f, "cap %s\n", caps);
		gc->last_style = gc->style;
	}
	if (gc->capw != gc->last_capw) {
		if (!debug_values[HA_stateless].lng)
			rnd_fprintf(f, "width %mm\n", gc->capw);
		gc->last_capw = gc->capw;
	}
}

static void end_gc(rnd_hid_gc_t gc)
{
	if (debug_values[HA_stateless].lng) {
		const char *caps = "<unknown>";

		switch(gc->style) {
			case rnd_cap_invalid: caps = "invalid"; break;
			case rnd_cap_square:  caps = "square"; break;
			case rnd_cap_round:   caps = "round"; break;
		}

		rnd_fprintf(f, "    @ %d %d %d %d %s %mm\n", gc->color.r, gc->color.g, gc->color.b, gc->color.a, caps, gc->capw);
	}
	else
		fprintf(f, "\n");
}

static void debug_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->style = style;
}

static void debug_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->capw = width;
}

static void debug_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void debug_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
	rnd_fprintf(f, "rect %mm %mm %mm %mm", x1, y1, x2, y2);
	end_gc(gc);
}

static void debug_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
	rnd_fprintf(f, "fillrect %mm %mm %mm %mm", x1, y1, x2, y2);
	end_gc(gc);
}

static void debug_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
	rnd_fprintf(f, "line %mm %mm %mm %mm", x1, y1, x2, y2);
	end_gc(gc);
}

static void debug_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	use_gc(gc);
	rnd_fprintf(f, "arc %mm %mm %mm %mm %f %f", cx, cy, width, height, start_angle, delta_angle);
	end_gc(gc);
}

static void debug_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(gc);
	rnd_fprintf(f, "fillcircle %mm %mm %mm\n", cx, cy, radius);
}

static void debug_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	long n;

	use_gc(gc);

	rnd_fprintf(f, "poly %d", n_coords, dx, dy);
	if (debug_values[HA_compact].lng) {
		for(n = 0; n < n_coords; n++)
			rnd_fprintf(f, " %mm %mm", x[n]+dx, y[n]+dy);
	}
	else {
		rnd_printf("\n");
		for(n = 0; n < n_coords; n++)
			rnd_fprintf(f, " pt %mm %mm\n", x[n]+dx, y[n]+dy);
		rnd_fprintf(f, " endpoly");
	}
	end_gc(gc);
}

static void debug_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	debug_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void debug_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int debug_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\ndebug exporter command line arguments:\n\n");
	rnd_hid_usage(debug_attribute_list, sizeof(debug_attribute_list) / sizeof(debug_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x debug [debug options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_debug(int ver_needed) { return 0; }

void pplg_uninit_export_debug(void)
{
	rnd_export_remove_opts_by_cookie(debug_cookie);
	rnd_hid_remove_hid(&debug_hid);
}

int pplg_init_export_debug(void)
{
	RND_API_CHK_VER;

	memset(&debug_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&debug_hid);

	debug_hid.struct_size = sizeof(rnd_hid_t);
	debug_hid.name = "debug";
	debug_hid.description = "dump drawing objects for debug purposes";
	debug_hid.exporter = 1;

	debug_hid.get_export_options = debug_get_export_options;
	debug_hid.do_export = debug_do_export;
	debug_hid.parse_arguments = debug_parse_arguments;
	debug_hid.set_layer_group = debug_set_layer_group;
	debug_hid.make_gc = debug_make_gc;
	debug_hid.destroy_gc = debug_destroy_gc;
	debug_hid.set_drawing_mode = debug_set_drawing_mode;
	debug_hid.set_color = debug_set_color;
	debug_hid.set_line_cap = debug_set_line_cap;
	debug_hid.set_line_width = debug_set_line_width;
	debug_hid.set_draw_xor = debug_set_draw_xor;
	debug_hid.draw_line = debug_draw_line;
	debug_hid.draw_arc = debug_draw_arc;
	debug_hid.draw_rect = debug_draw_rect;
	debug_hid.fill_circle = debug_fill_circle;
	debug_hid.fill_polygon = debug_fill_polygon;
	debug_hid.fill_polygon_offs = debug_fill_polygon_offs;
	debug_hid.fill_rect = debug_fill_rect;
	debug_hid.set_crosshair = debug_set_crosshair;
	debug_hid.argument_array = debug_values;

	debug_hid.usage = debug_usage;

	rnd_hid_register_hid(&debug_hid);
	rnd_hid_load_defaults(&debug_hid, debug_attribute_list, NUM_OPTIONS);

	return 0;
}
