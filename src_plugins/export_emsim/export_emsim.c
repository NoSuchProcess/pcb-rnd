/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - export to emsim-rnd (tEDAx)
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include "layer.h"
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>

#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"

static rnd_hid_t exp_emsim_hid;

const char *exp_emsim_cookie = "exp_emsim HID";

static pcb_cam_t exp_emsim_cam;

/* global states during export */
static char *filesuff = NULL;
static char *filename = NULL;
static int fn_baselen = 0;
static gds_t fn_gds;
static FILE *f;


#define TRX(x)  ((double)RND_COORD_TO_MM(x))
#define TRY(y)  ((double)RND_COORD_TO_MM(y))

static const rnd_export_opt_t exp_emsim_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_exp_emsimfile 0

	{"lumped", "semicolon separated list of lumped elements",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_exp_emsim_lumped 1

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_cam 2
};

#define NUM_OPTIONS (sizeof(exp_emsim_attribute_list)/sizeof(exp_emsim_attribute_list[0]))

static rnd_hid_attr_val_t exp_emsim_values[NUM_OPTIONS];

static const rnd_export_opt_t *exp_emsim_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *suffix = ".emsim";
	const char *val = exp_emsim_values[HA_exp_emsimfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &exp_emsim_values[HA_exp_emsimfile], suffix);

	if (n)
		*n = NUM_OPTIONS;

	return exp_emsim_attribute_list;
}

static void exp_emsim_hid_export_to_file(rnd_design_t *dsg, FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.design = dsg;
	ctx.view.X1 = dsg->dwg.X1;
	ctx.view.Y1 = dsg->dwg.Y1;
	ctx.view.X2 = dsg->dwg.X2;
	ctx.view.Y2 = dsg->dwg.Y2;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	rnd_app.expose_main(&exp_emsim_hid, &ctx, xform);

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void exp_emsim_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	const char *filename;
	rnd_xform_t xform;

	if (!options) {
		exp_emsim_get_export_options(hid, 0, design, appspec);
		options = exp_emsim_values;
	}

	pcb_cam_begin(PCB, &exp_emsim_cam, &xform, options[HA_cam].str, exp_emsim_attribute_list, NUM_OPTIONS, options);

	if (exp_emsim_cam.fn_template == NULL) {
		filename = options[HA_exp_emsimfile].str;
		if (!filename)
			filename = "board.emsim";

		if (f != NULL) {
			fclose(f);
			f = NULL;
		}

		f = rnd_fopen_askovr(&PCB->hidlib, exp_emsim_cam.active ? exp_emsim_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			return;
		}
	}

	exp_emsim_hid_export_to_file(design, f, options, &xform);

	if (f != NULL) {
		fclose(f);
		f = NULL;
	}
	gds_uninit(&fn_gds);

	if (!exp_emsim_cam.active) exp_emsim_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&exp_emsim_cam) == 0) {
		if (!exp_emsim_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "exp_emsim cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
}

static int exp_emsim_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, exp_emsim_attribute_list, sizeof(exp_emsim_attribute_list) / sizeof(exp_emsim_attribute_list[0]), exp_emsim_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static void append_file_suffix(gds_t *dst, rnd_layergrp_id_t gid, rnd_layer_id_t lid, unsigned int flags, const char *purpose, int purpi, int drill, int *merge_same)
{
	const char *sext = ".emsim";

	fn_gds.used = fn_baselen;
	if (merge_same != NULL) *merge_same = 0;

	pcb_layer_to_file_name_append(dst, lid, flags, purpose, purpi, PCB_FNS_pcb_rnd);
	gds_append_str(dst, sext);

	filename = fn_gds.array;
	filesuff = fn_gds.array + fn_baselen;
}

static int exp_emsim_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&exp_emsim_cam, group, purpose, purpi, flags, xform);

	if ((!exp_emsim_cam.active) && (flags & PCB_LYT_INVIS))
			return 0;

	/* can't export non-copper: compositing is not supported */
	if (!(flags & PCB_LYT_COPPER))
		return 0;

	TODO("flush previous layer, start new layer");

	return 1;
}

static rnd_hid_gc_t exp_emsim_make_gc(rnd_hid_t *hid)
{
	return 0;
}

static void exp_emsim_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void exp_emsim_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	if (direct)
		return;

	switch(op) {
		case RND_HID_COMP_RESET:
			break;

		case RND_HID_COMP_POSITIVE:
			break;

		case RND_HID_COMP_POSITIVE_XOR:
		case RND_HID_COMP_NEGATIVE:
			rnd_message(RND_MSG_ERROR, "emsim does not support composite layers\n");
			break;

		case RND_HID_COMP_FLUSH:
			break;
	}
}


static void exp_emsim_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
}

static void exp_emsim_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
}

static void exp_emsim_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
}

static void exp_emsim_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
}


static void exp_emsim_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void exp_emsim_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void exp_emsim_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void exp_emsim_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
}

static void exp_emsim_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
}

static void exp_emsim_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
}

static void exp_emsim_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	exp_emsim_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static int exp_emsim_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nexp_emsim exporter command line arguments:\n\n");
	rnd_hid_usage(exp_emsim_attribute_list, sizeof(exp_emsim_attribute_list) / sizeof(exp_emsim_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x exp_emsim [exp_emsim options] foo.rp\n\n");
	return 0;
}

int pplg_check_ver_export_emsim(int ver_needed) { return 0; }

void pplg_uninit_export_emsim(void)
{
	rnd_export_remove_opts_by_cookie(exp_emsim_cookie);
	rnd_hid_remove_hid(&exp_emsim_hid);
}

int pplg_init_export_emsim(void)
{
	RND_API_CHK_VER;

	memset(&exp_emsim_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&exp_emsim_hid);

	exp_emsim_hid.struct_size = sizeof(rnd_hid_t);
	exp_emsim_hid.name = "emsim";
	exp_emsim_hid.description = "export board for electromagnetic simulation with emsim-rnd";
	exp_emsim_hid.exporter = 1;

	exp_emsim_hid.get_export_options = exp_emsim_get_export_options;
	exp_emsim_hid.do_export = exp_emsim_do_export;
	exp_emsim_hid.parse_arguments = exp_emsim_parse_arguments;
	exp_emsim_hid.set_layer_group = exp_emsim_set_layer_group;
	exp_emsim_hid.make_gc = exp_emsim_make_gc;
	exp_emsim_hid.destroy_gc = exp_emsim_destroy_gc;
	exp_emsim_hid.set_drawing_mode = exp_emsim_set_drawing_mode;
	exp_emsim_hid.set_color = exp_emsim_set_color;
	exp_emsim_hid.set_line_cap = exp_emsim_set_line_cap;
	exp_emsim_hid.set_line_width = exp_emsim_set_line_width;
	exp_emsim_hid.set_draw_xor = exp_emsim_set_draw_xor;
	exp_emsim_hid.draw_line = exp_emsim_draw_line;
	exp_emsim_hid.draw_arc = exp_emsim_draw_arc;
	exp_emsim_hid.draw_rect = exp_emsim_draw_rect;
	exp_emsim_hid.fill_circle = exp_emsim_fill_circle;
	exp_emsim_hid.fill_polygon = exp_emsim_fill_polygon;
	exp_emsim_hid.fill_polygon_offs = exp_emsim_fill_polygon_offs;
	exp_emsim_hid.fill_rect = exp_emsim_fill_rect;
	exp_emsim_hid.argument_array = exp_emsim_values;

	exp_emsim_hid.usage = exp_emsim_usage;

	rnd_hid_register_hid(&exp_emsim_hid);
	rnd_hid_load_defaults(&exp_emsim_hid, exp_emsim_attribute_list, NUM_OPTIONS);

	return 0;
}
