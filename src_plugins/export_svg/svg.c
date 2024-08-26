/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2006 Dan McMahill
 *  Copyright (C) 2016,2022,2024 Tibor 'Igor2' Palinkas
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

/* Based on the png exporter by Dan McMahill */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <genvector/gds_char.h>

#include <librnd/core/math_helper.h>
#include <librnd/core/rnd_conf.h>
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

#include <librnd/plugins/lib_exp_text/draw_svg.h>

static rnd_hid_t svg_hid;

const char *svg_cookie = "svg HID";

static pcb_cam_t svg_cam;

static rnd_svg_t pctx_, *pctx = &pctx_;

static const char *colorspace_names[] = {
	"color",
	"grayscale",
	"monochrome",
	NULL
};

static const rnd_export_opt_t svg_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 SVG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_svgfile 0

/* %start-doc options "93 SVG Options"
@ftable @code
@cindex photo-mode
@item --photo-mode
Export a photo realistic image of the layout.
@end ftable
%end-doc
*/
	{"photo-mode", "Photo-realistic export mode",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_photo_mode 1

/* %start-doc options "93 SVG Options"
@ftable @code
@cindex opacity
@item --opacity
Layer opacity
@end ftable
%end-doc
*/
	{"opacity", "Layer opacity",
	 RND_HATT_INTEGER, 0, 100, {100, 0, 0}, 0},
#define HA_opacity 2

/* %start-doc options "93 SVG Options"
@ftable @code
@cindex flip
@item --flip
Flip board, look at it from the bottom side
@end ftable
%end-doc
*/
	{"flip", "Flip board",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_flip 3

	{"as-shown", "Render similar to as shown on screen (display overlays)",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_as_shown 4

	{"true-size", "Attempt to preserve true size for printing",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_true_size 5

	{"photo-noise", "Add noise in photo mode to make the output more realistic",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_photo_noise 6

	{"colorspace", "Export in color or reduce colorspace",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, colorspace_names},
#define HA_colorspace 7

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_cam 8
};

#define NUM_OPTIONS (sizeof(svg_attribute_list)/sizeof(svg_attribute_list[0]))

static rnd_hid_attr_val_t svg_values[NUM_OPTIONS];

static const rnd_export_opt_t *svg_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *suffix = ".svg";
	const char *val = svg_values[HA_svgfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &svg_values[HA_svgfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return svg_attribute_list;
}

static void svg_hid_export_to_file(rnd_design_t *dsg, FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.design = dsg;
	ctx.view.X1 = dsg->dwg.X1;
	ctx.view.Y1 = dsg->dwg.Y1;
	ctx.view.X2 = dsg->dwg.X2;
	ctx.view.Y2 = dsg->dwg.Y2;

	pctx->outf = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	{
		rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

		if (options[HA_photo_mode].lng) {
			pctx->photo_mode = 1;
		}
		else
			pctx->photo_mode = 0;

		if (options[HA_photo_noise].lng)
			pctx->photo_noise = 1;
		else
			pctx->photo_noise = 0;

		if (options[HA_flip].lng) {
			rnd_layer_id_t topcop[32];
			int n, v;

			rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 1);

			/* make sure bottom side copper is top on visibility so it is rendered last */
			pcb_layervis_save_stack();
			v = pcb_layer_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, topcop, sizeof(topcop)/sizeof(topcop[0]));
			for(n = 0; n < v; n++)
				pcb_layervis_change_group_vis(&PCB->hidlib, topcop[n], 1, 1);
		}
	}

	rnd_svg_background(pctx);

	if (options[HA_as_shown].lng) {
		pcb_draw_setup_default_gui_xform(xform);

		/* disable (exporter default) hiding overlay in as_shown */
		xform->omit_overlay = 0;
		xform->add_gui_xform = 1;
		xform->enable_silk_invis_clr = 1;
	}

	rnd_app.expose_main(&svg_hid, &ctx, xform);

	if (pctx->flip)
		pcb_layervis_restore_stack();
	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void svg_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;
	FILE *f = NULL;

	pctx->comp_cnt = 0;

	if (!options) {
		svg_get_export_options(hid, 0, design, appspec);
		options = svg_values;
	}

	;
	pcb_cam_begin(PCB, &svg_cam, &xform, options[HA_cam].str, svg_attribute_list, NUM_OPTIONS, options);

	if (svg_cam.fn_template == NULL) {
		const char *fn;

		filename = options[HA_svgfile].str;
		if (!filename)
			filename = "pcb.svg";

		fn = svg_cam.active ? svg_cam.fn : filename;
		f = rnd_fopen_askovr(&PCB->hidlib, fn, "wb", NULL);
		if (f == NULL) {
			int ern = errno;
			rnd_message(RND_MSG_ERROR, "svg_do_export(): failed to open %s: %s\n", fn, strerror(ern));
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	rnd_svg_init(pctx, design, f, options[HA_opacity].lng, options[HA_flip].lng, options[HA_true_size].lng);
	if (f != NULL)
		rnd_svg_header(pctx);

	pctx->colorspace = options[HA_colorspace].lng;

	if (!svg_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	svg_hid_export_to_file(design, pctx->outf, options, &xform);


	if (!svg_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	if (pctx->outf != NULL) {
		rnd_svg_footer(pctx);
		fclose(pctx->outf);
	}
	pctx->outf = NULL;

	if (!svg_cam.active) svg_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&svg_cam) == 0) {
		if (!svg_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "svg cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (pctx->drawn_objs == 0) {
		if (!svg_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "svg cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}

	rnd_svg_uninit(pctx);
}

static int svg_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]), svg_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int svg_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	int is_our_mask = 0, is_our_silk = 0;

	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&svg_cam, group, purpose, purpi, flags, xform);

	if (svg_cam.fn_changed || (pctx->outf == NULL)) {
		FILE *f = rnd_fopen_askovr(&PCB->hidlib, svg_cam.fn, "wb", NULL);

		if (rnd_svg_new_file(pctx, f, svg_cam.fn) != 0)
			return 0;
		rnd_svg_header(pctx);
	}

	if (!svg_cam.active) {
		if (flags & PCB_LYT_INVIS)
			return 0;

		if (flags & PCB_LYT_MASK) {
			if ((!pctx->photo_mode) && (!PCB->LayerGroups.grp[group].vis))
				return 0; /* not in photo mode or not visible */
		}
	}

	switch(flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK: is_our_mask = PCB_LAYERFLG_ON_VISIBLE_SIDE(flags); break;
		case PCB_LYT_SILK: is_our_silk = PCB_LAYERFLG_ON_VISIBLE_SIDE(flags); break;
		default:;
	}

	if (!svg_cam.active) {
		if (!(flags & PCB_LYT_COPPER) && (!is_our_silk) && (!is_our_mask) && !(PCB_LAYER_IS_DRILL(flags, purpi)) && !(PCB_LAYER_IS_ROUTE(flags, purpi)))
			return 0;
	}

	if (pctx->photo_mode && (group < 0)) {
		pctx->drawing_hole = PCB_LAYER_IS_DRILL(flags, purpi);
		return 1; /* photo mode drill: do not create a separate group */
	}

	{
		gds_t tmp_ln;
		const char *name;

		gds_init(&tmp_ln);
		name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);
		rnd_svg_layer_group_begin(pctx, group, name, is_our_mask);
		gds_uninit(&tmp_ln);
	}

	if (pctx->photo_mode) {
		if (is_our_silk)
			rnd_svg_photo_color = RND_SVG_PHOTO_SILK;
		else if (is_our_mask)
			rnd_svg_photo_color = RND_SVG_PHOTO_MASK;
		else if (group >= 0) {
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				rnd_svg_photo_color = RND_SVG_PHOTO_COPPER;
			else
				rnd_svg_photo_color = RND_SVG_PHOTO_INNER;
		}
	}

	pctx->drawing_hole = PCB_LAYER_IS_DRILL(flags, purpi);
	return 1;
}

static void svg_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_svg_set_drawing_mode(pctx, hid, op, direct, screen);
}

static void svg_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_svg_set_color(pctx, gc, color);
}

static void svg_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_svg_draw_rect(pctx, gc, x1, y1, x2, y2);
}

static void svg_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_svg_fill_rect(pctx, gc, x1, y1, x2, y2);
}

static void svg_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_svg_draw_line(pctx, gc, x1, y1, x2, y2);
}

static void svg_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_svg_draw_arc(pctx, gc, cx, cy, width, height, start_angle, delta_angle);
}

static void svg_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_svg_fill_circle(pctx, gc, cx, cy, radius);
}

static void svg_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_svg_fill_polygon_offs(pctx, gc, n_coords, x, y, dx, dy);
}

static void svg_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	rnd_svg_fill_polygon_offs(pctx, gc, n_coords, x, y, 0, 0);
}

static int svg_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nsvg exporter command line arguments:\n\n");
	rnd_hid_usage(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x svg [svg options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_svg(int ver_needed) { return 0; }

void pplg_uninit_export_svg(void)
{
	rnd_export_remove_opts_by_cookie(svg_cookie);
	rnd_hid_remove_hid(&svg_hid);
}

int pplg_init_export_svg(void)
{
	RND_API_CHK_VER;

	memset(&svg_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&svg_hid);

	svg_hid.struct_size = sizeof(rnd_hid_t);
	svg_hid.name = "svg";
	svg_hid.description = "Scalable Vector Graphics export";
	svg_hid.exporter = 1;

	svg_hid.get_export_options = svg_get_export_options;
	svg_hid.do_export = svg_do_export;
	svg_hid.parse_arguments = svg_parse_arguments;
	svg_hid.set_layer_group = svg_set_layer_group;
	svg_hid.make_gc = rnd_svg_make_gc;
	svg_hid.destroy_gc = rnd_svg_destroy_gc;
	svg_hid.set_drawing_mode = svg_set_drawing_mode;
	svg_hid.set_color = svg_set_color;
	svg_hid.set_line_cap = rnd_svg_set_line_cap;
	svg_hid.set_line_width = rnd_svg_set_line_width;
	svg_hid.set_draw_xor = rnd_svg_set_draw_xor;
	svg_hid.draw_line = svg_draw_line;
	svg_hid.draw_arc = svg_draw_arc;
	svg_hid.draw_rect = svg_draw_rect;
	svg_hid.fill_circle = svg_fill_circle;
	svg_hid.fill_polygon = svg_fill_polygon;
	svg_hid.fill_polygon_offs = svg_fill_polygon_offs;
	svg_hid.fill_rect = svg_fill_rect;
	svg_hid.set_crosshair = rnd_svg_set_crosshair;
	svg_hid.argument_array = svg_values;

	svg_hid.usage = svg_usage;

	rnd_hid_register_hid(&svg_hid);
	rnd_hid_load_defaults(&svg_hid, svg_attribute_list, NUM_OPTIONS);

	return 0;
}
