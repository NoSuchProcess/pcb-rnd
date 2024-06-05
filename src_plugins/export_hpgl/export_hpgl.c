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
#include "endp.h"

static rnd_hid_t exp_hpgl_hid;

const char *exp_hpgl_cookie = "exp_hpgl HID";

static pcb_cam_t exp_hpgl_cam;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_color_t color, last_color;
	rnd_cap_style_t style, last_style;
	rnd_coord_t capw, last_capw;
} rnd_hid_gc_s;

static FILE *f = NULL;
static htendp_t ht;
static int ht_active;

static void hpgl_flush_layer(void)
{
	if (ht_active) {
		hpgl_endp_render(&ht);
		hpgl_endp_uninit(&ht);
		ht_active = 0;
	}
}

static const rnd_export_opt_t exp_hpgl_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_exp_hpglfile 0

	{"sepfiles", "Write one file per layer instead of using one per per layer",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_exp_hpgl_sepfiles 1

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_cam 2
};

#define NUM_OPTIONS (sizeof(exp_hpgl_attribute_list)/sizeof(exp_hpgl_attribute_list[0]))

static rnd_hid_attr_val_t exp_hpgl_values[NUM_OPTIONS];

static const rnd_export_opt_t *exp_hpgl_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *suffix = ".hpgl";
	const char *val = exp_hpgl_values[HA_exp_hpglfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &exp_hpgl_values[HA_exp_hpglfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return exp_hpgl_attribute_list;
}

static void exp_hpgl_hid_export_to_file(rnd_design_t *dsg, FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
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

	rnd_app.expose_main(&exp_hpgl_hid, &ctx, xform);
	hpgl_flush_layer();

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void exp_hpgl_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;

	if (!options) {
		exp_hpgl_get_export_options(hid, 0, design, appspec);
		options = exp_hpgl_values;
	}

	pcb_cam_begin(PCB, &exp_hpgl_cam, &xform, options[HA_cam].str, exp_hpgl_attribute_list, NUM_OPTIONS, options);

	if (exp_hpgl_cam.fn_template == NULL) {
		filename = options[HA_exp_hpglfile].str;
		if (!filename)
			filename = "board.hpgl";

		f = rnd_fopen_askovr(&PCB->hidlib, exp_hpgl_cam.active ? exp_hpgl_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	if (!exp_hpgl_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	exp_hpgl_hid_export_to_file(design, f, options, &xform);

	if (!exp_hpgl_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	if (f != NULL)
		fclose(f);
	f = NULL;

	if (!exp_hpgl_cam.active) exp_hpgl_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&exp_hpgl_cam) == 0) {
		if (!exp_hpgl_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "exp_hpgl cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
}

static int exp_hpgl_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, exp_hpgl_attribute_list, sizeof(exp_hpgl_attribute_list) / sizeof(exp_hpgl_attribute_list[0]), exp_hpgl_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int exp_hpgl_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&exp_hpgl_cam, group, purpose, purpi, flags, xform);


	if (!exp_hpgl_cam.active) {
		if (flags & PCB_LYT_INVIS)
			return 0;

		if ((flags & PCB_LYT_VIRTUAL) || (flags & PCB_LYT_DOC))
			return 0;

		if (flags & PCB_LYT_MASK) {
			if (!PCB->LayerGroups.grp[group].vis)
				return 0; /* not in photo mode or not visible */
		}
	}

	hpgl_flush_layer();

	hpgl_endp_init(&ht);
	ht_active = 1;

	return 1;
}


static rnd_hid_gc_t exp_hpgl_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	return rv;
}

static void exp_hpgl_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void exp_hpgl_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
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


static void exp_hpgl_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	gc->color = *color;
}

static void exp_hpgl_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
}

static void exp_hpgl_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
}

static void exp_hpgl_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
}

static void exp_hpgl_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void exp_hpgl_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void exp_hpgl_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	fprintf(f, "Line!\n");
}

static void exp_hpgl_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
}

static void exp_hpgl_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
}

static void exp_hpgl_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
}

static void exp_hpgl_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
}

static int exp_hpgl_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nexp_hpgl exporter command line arguments:\n\n");
	rnd_hid_usage(exp_hpgl_attribute_list, sizeof(exp_hpgl_attribute_list) / sizeof(exp_hpgl_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x exp_hpgl [exp_hpgl options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_hpgl(int ver_needed) { return 0; }

void pplg_uninit_export_hpgl(void)
{
	rnd_export_remove_opts_by_cookie(exp_hpgl_cookie);
	rnd_hid_remove_hid(&exp_hpgl_hid);
}

int pplg_init_export_hpgl(void)
{
	RND_API_CHK_VER;

	memset(&exp_hpgl_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&exp_hpgl_hid);

	exp_hpgl_hid.struct_size = sizeof(rnd_hid_t);
	exp_hpgl_hid.name = "hpgl";
	exp_hpgl_hid.description = "thin-draw in HP-GL";
	exp_hpgl_hid.exporter = 1;

	exp_hpgl_hid.get_export_options = exp_hpgl_get_export_options;
	exp_hpgl_hid.do_export = exp_hpgl_do_export;
	exp_hpgl_hid.parse_arguments = exp_hpgl_parse_arguments;
	exp_hpgl_hid.set_layer_group = exp_hpgl_set_layer_group;
	exp_hpgl_hid.make_gc = exp_hpgl_make_gc;
	exp_hpgl_hid.destroy_gc = exp_hpgl_destroy_gc;
	exp_hpgl_hid.set_drawing_mode = exp_hpgl_set_drawing_mode;
	exp_hpgl_hid.set_color = exp_hpgl_set_color;
	exp_hpgl_hid.set_line_cap = exp_hpgl_set_line_cap;
	exp_hpgl_hid.set_line_width = exp_hpgl_set_line_width;
	exp_hpgl_hid.set_draw_xor = exp_hpgl_set_draw_xor;
	exp_hpgl_hid.draw_line = exp_hpgl_draw_line;
	exp_hpgl_hid.draw_arc = exp_hpgl_draw_arc;
	exp_hpgl_hid.draw_rect = exp_hpgl_draw_rect;
	exp_hpgl_hid.fill_circle = exp_hpgl_fill_circle;
	exp_hpgl_hid.fill_polygon = exp_hpgl_fill_polygon;
	exp_hpgl_hid.fill_polygon_offs = exp_hpgl_fill_polygon_offs;
	exp_hpgl_hid.fill_rect = exp_hpgl_fill_rect;
	exp_hpgl_hid.argument_array = exp_hpgl_values;

	exp_hpgl_hid.usage = exp_hpgl_usage;

	rnd_hid_register_hid(&exp_hpgl_hid);
	rnd_hid_load_defaults(&exp_hpgl_hid, exp_hpgl_attribute_list, NUM_OPTIONS);

	return 0;
}
