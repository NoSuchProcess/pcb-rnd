/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>

#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"

static rnd_hid_t c_draw_hid;

const char *c_draw_cookie = "c_draw HID";

static pcb_cam_t c_draw_cam;

typedef struct rnd_hid_gc_s {
	rnd_color_t color, last_color;
	rnd_cap_style_t style, last_style;
	rnd_coord_t capw, last_capw;
} rnd_hid_gc_s;

static FILE *f = NULL;

rnd_export_opt_t c_draw_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_c_drawfile 0

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0}
#define HA_cam 1
};

#define NUM_OPTIONS (sizeof(c_draw_attribute_list)/sizeof(c_draw_attribute_list[0]))

static rnd_hid_attr_val_t c_draw_values[NUM_OPTIONS];

static rnd_export_opt_t *c_draw_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".c";
	char **val = c_draw_attribute_list[HA_c_drawfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &c_draw_attribute_list[HA_c_drawfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return c_draw_attribute_list;
}

void c_draw_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	rnd_expose_main(&c_draw_hid, &ctx, xform);

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void c_draw_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	if (!options) {
		c_draw_get_export_options(hid, 0);
		options = c_draw_values;
	}

	pcb_cam_begin(PCB, &c_draw_cam, &xform, options[HA_cam].str, c_draw_attribute_list, NUM_OPTIONS, options);

	if (c_draw_cam.fn_template == NULL) {
		filename = options[HA_c_drawfile].str;
		if (!filename)
			filename = "pcb.c";

		f = rnd_fopen_askovr(&PCB->hidlib, c_draw_cam.active ? c_draw_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	if (!c_draw_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	c_draw_hid_export_to_file(f, options, &xform);

	if (!c_draw_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	if (f != NULL)
		fclose(f);
	f = NULL;

	if (!c_draw_cam.active) c_draw_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&c_draw_cam) == 0) {
		if (!c_draw_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "c_draw cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
}

static int c_draw_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, c_draw_attribute_list, sizeof(c_draw_attribute_list) / sizeof(c_draw_attribute_list[0]), c_draw_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int c_draw_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&c_draw_cam, group, purpose, purpi, flags, xform);


	if (!c_draw_cam.active) {
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
		fprintf(f, "/* Layer #%ld: '%s' */\n", group, name);
		gds_uninit(&tmp_ln);
	}

	return 1;
}


static rnd_hid_gc_t c_draw_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	return rv;
}

static void c_draw_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void c_draw_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
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


static void c_draw_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	gc->color = *color;
}

static void use_gc(rnd_hid_gc_t gc)
{
	if (gc->color.packed != gc->last_color.packed) {
		fprintf(f, "rnd_color_load_int(&clr, %d, %d, %d, %d);\n", gc->color.r, gc->color.g, gc->color.b, gc->color.a);
		fprintf(f, "rnd_render->set_color(gc, &clr)\n");
		gc->last_color = gc->color;
	}
	if (gc->style != gc->last_style) {
		const char *caps = NULL;
		switch(gc->style) {
			case rnd_cap_invalid: caps = "rnd_cap_invalid"; break;
			case rnd_cap_square:  caps = "rnd_cap_square"; break;
			case rnd_cap_round:   caps = "rnd_cap_round"; break;
		}
		fprintf(f, "rnd_hid_set_line_cap(gc, %s);\n", caps);
		gc->last_style = gc->style;
	}
	if (gc->capw != gc->last_capw) {
		rnd_fprintf(f, "rnd_hid_set_line_cap(gc, PCB_MM_TO_COORD(%mm));\n", gc->capw);
		gc->last_capw = gc->capw;
	}

}

static void c_draw_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->style = style;
}

static void c_draw_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->capw = width;
}

static void c_draw_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void c_draw_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	;
}

static void c_draw_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	;
}

static void c_draw_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(gc);
	rnd_fprintf(f, "rnd_render->draw_line(gc, RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm));\n", x1, y1, x2, y2);
}

static void c_draw_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	use_gc(gc);
	rnd_fprintf(f, "rnd_render->draw_arc(gc, RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), %f, %f\n",
		cx, cy, width, height, start_angle, delta_angle);
}

static void c_draw_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	use_gc(gc);
	rnd_fprintf(f, "rnd_render->draw_fill_circle(gc, RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm), RND_MM_TO_COORD(%mm)\n",
		cx, cy, radius);
}

static void c_draw_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
;
}

static void c_draw_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
;
}


static void c_draw_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	rnd_message(RND_MSG_ERROR, "c_draw_calibrate() not implemented");
	return;
}

static void c_draw_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int c_draw_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nc_draw exporter command line arguments:\n\n");
	rnd_hid_usage(c_draw_attribute_list, sizeof(c_draw_attribute_list) / sizeof(c_draw_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x c_draw [c_draw options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_c_draw(int ver_needed) { return 0; }

void pplg_uninit_export_c_draw(void)
{
	rnd_export_remove_opts_by_cookie(c_draw_cookie);
	rnd_hid_remove_hid(&c_draw_hid);
}

int pplg_init_export_c_draw(void)
{
	RND_API_CHK_VER;

	memset(&c_draw_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&c_draw_hid);

	c_draw_hid.struct_size = sizeof(rnd_hid_t);
	c_draw_hid.name = "c_draw";
	c_draw_hid.description = "low level pcb-rnd draw code in C";
	c_draw_hid.exporter = 1;

	c_draw_hid.get_export_options = c_draw_get_export_options;
	c_draw_hid.do_export = c_draw_do_export;
	c_draw_hid.parse_arguments = c_draw_parse_arguments;
	c_draw_hid.set_layer_group = c_draw_set_layer_group;
	c_draw_hid.make_gc = c_draw_make_gc;
	c_draw_hid.destroy_gc = c_draw_destroy_gc;
	c_draw_hid.set_drawing_mode = c_draw_set_drawing_mode;
	c_draw_hid.set_color = c_draw_set_color;
	c_draw_hid.set_line_cap = c_draw_set_line_cap;
	c_draw_hid.set_line_width = c_draw_set_line_width;
	c_draw_hid.set_draw_xor = c_draw_set_draw_xor;
	c_draw_hid.draw_line = c_draw_draw_line;
	c_draw_hid.draw_arc = c_draw_draw_arc;
	c_draw_hid.draw_rect = c_draw_draw_rect;
	c_draw_hid.fill_circle = c_draw_fill_circle;
	c_draw_hid.fill_polygon = c_draw_fill_polygon;
	c_draw_hid.fill_polygon_offs = c_draw_fill_polygon_offs;
	c_draw_hid.fill_rect = c_draw_fill_rect;
	c_draw_hid.calibrate = c_draw_calibrate;
	c_draw_hid.set_crosshair = c_draw_set_crosshair;
	c_draw_hid.argument_array = c_draw_values;

	c_draw_hid.usage = c_draw_usage;

	rnd_hid_register_hid(&c_draw_hid);

	return 0;
}
