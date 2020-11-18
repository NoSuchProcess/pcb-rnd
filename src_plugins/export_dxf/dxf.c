/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
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
#include <librnd/core/plugins.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include "lht_template.h"
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>

#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"

static const char *layer_names[] = {
	"outline",
	"bottom_copper", "top_copper",
	"bottom_silk", "top_silk",
	"drill_plated", "drill_unplated",
	NULL
};

static rnd_hid_t dxf_hid;

const char *dxf_cookie = "dxf HID";

static pcb_cam_t dxf_cam;


typedef struct {
	FILE *f;
	unsigned long handle;
	lht_doc_t *temp;
	const char *layer_name;
	long drawn_objs;
	unsigned force_thin:1;
	unsigned enable_force_thin:1;
	unsigned poly_fill:1;
	unsigned poly_contour:1;
	unsigned drill_fill:1;
	unsigned drill_contour:1;
} dxf_ctx_t;

static dxf_ctx_t dxf_ctx;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	rnd_coord_t width;
	char *color;
	int drill;
	unsigned warned_elliptical:1;
	unsigned drawing_hole:1;
} rnd_hid_gc_s;

static struct rnd_hid_gc_s thin = {
	{0},
	NULL,
	0, 1,
	NULL, 0, 0
};


#include "dxf_draw.c"


rnd_export_opt_t dxf_attribute_list[] = {
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
#define HA_dxffile 0

/* %start-doc options "93 DXF Options"
@ftable @code
@item --template <string>
Name of the lihata template file to be used instead of the default dxf template. Can contain a path.
@end ftable
%end-doc
*/
	{"template", "DXF template (lihata file)",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_template 1

/* %start-doc options "93 DXF Options"
@ftable @code
@item --thin
Draw outline and drills with thin lines.
@end ftable
%end-doc
*/
	{"thin", "Draw outline and drill with thin lines",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_thin 2

/* %start-doc options "93 DXF Options"
@ftable @code
@item --poly-fill
Fill polygons using hatch
@end ftable
%end-doc
*/
	{"poly-fill", "Fill polygons using hatch",
	 RND_HATT_BOOL, 0, 0, {1, (void *)1, 1}, 0, 0},
#define HA_poly_fill 3

/* %start-doc options "93 DXF Options"
@ftable @code
@item --poly-fill
Draw polygons contour with thin line
@end ftable
%end-doc
*/
	{"poly-contour", "Draw polygons contour with thin line",
	 RND_HATT_BOOL, 0, 0, {1, (void *)1, 1}, 0, 0},
#define HA_poly_contour 4

/* %start-doc options "93 DXF Options"
@ftable @code
@item --drill-fill
Fill drill (hole) circles using hatch
@end ftable
%end-doc
*/
	{"drill-fill", "Fill drill (hole) circles using hatch",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_drill_fill 5

/* %start-doc options "93 DXF Options"
@ftable @code
@item --polyfill
Draw drill contour with thin line
@end ftable
%end-doc
*/
	{"drill-contour", "Draw drill contour with thin line",
	 RND_HATT_BOOL, 0, 0, {1, (void *)1, 1}, 0, 0},
#define HA_drill_contour 6

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 7

};

#define NUM_OPTIONS (sizeof(dxf_attribute_list)/sizeof(dxf_attribute_list[0]))

static rnd_hid_attr_val_t dxf_values[NUM_OPTIONS];

static rnd_export_opt_t *dxf_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".dxf";
	char **val = dxf_attribute_list[HA_dxffile].value;
	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &dxf_attribute_list[HA_dxffile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return dxf_attribute_list;
}

void dxf_hid_export_to_file(dxf_ctx_t *ctx, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t hectx;

	hectx.view.X1 = 0;
	hectx.view.Y1 = 0;
	hectx.view.X2 = PCB->hidlib.size_x;
	hectx.view.Y2 = PCB->hidlib.size_y;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	dxf_ctx.enable_force_thin = options[HA_thin].lng;
	dxf_ctx.poly_fill = options[HA_poly_fill].lng;
	dxf_ctx.poly_contour = options[HA_poly_contour].lng;
	dxf_ctx.drill_fill = options[HA_drill_fill].lng;
	dxf_ctx.drill_contour = options[HA_drill_contour].lng;

	rnd_expose_main(&dxf_hid, &hectx, xform);

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

int insert_hdr(FILE *f, const char *prefix, char *name, lht_err_t *err)
{
	if (strcmp(name, "extmin") == 0)
		fprintf(f, "10\n0\n20\n0\n30\n0\n");
	else if (strcmp(name, "extmax") == 0)
		rnd_fprintf(f, "10\n%mm\n20\n0\n30\n%mm\n", PCB->hidlib.size_x, PCB->hidlib.size_y);
	else if (strcmp(name, "layers") == 0) {
		const char **s;
		for(s = layer_names; *s != NULL; s++)
			dxf_gen_layer(&dxf_ctx, *s);
	}
	else {
		rnd_message(RND_MSG_ERROR, "Invalid header insertion: '%s'\n", name);
		return -1;
	}

	return 0;
}

int insert_ftr(FILE *f, const char *prefix, char *name, lht_err_t *err)
{
	rnd_message(RND_MSG_ERROR, "Invalid footer insertion: '%s'\n", name);
	return -1;
}

extern const char dxf_templ_default_arr[];
static void dxf_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	const char *fn;
	char *errmsg;
	lht_err_t err;
	rnd_xform_t xform;

	if (!options) {
		dxf_get_export_options(hid, 0);
		options = dxf_values;
	}

	dxf_ctx.drawn_objs = 0;
	pcb_cam_begin(PCB, &dxf_cam, &xform, options[HA_cam].str, dxf_attribute_list, NUM_OPTIONS, options);

	filename = options[HA_dxffile].str;
	if (!filename)
		filename = "pcb.dxf";

	if (dxf_cam.fn_template == NULL) {
		dxf_ctx.f = rnd_fopen_askovr(&PCB->hidlib, dxf_cam.active ? dxf_cam.fn : filename, "wb", NULL);
		if (!dxf_ctx.f) {
			perror(filename);
			return;
		}
	}
	else
		dxf_ctx.f = NULL;

	fn = options[HA_template].str;
	if (fn == NULL) {
		fn = "<embedded template>";
		dxf_ctx.temp = lht_dom_load_string(dxf_templ_default_arr, fn, &errmsg);
	}
	else {
		char *real_fn;
		dxf_ctx.temp = NULL;
		real_fn = rnd_fopen_check(&PCB->hidlib, fn, "r");
		if (real_fn != NULL)
			dxf_ctx.temp = lht_dom_load(real_fn, &errmsg);
		free(real_fn);
	}

	if (dxf_ctx.temp == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open dxf template: %s\n", fn);
		fclose(dxf_ctx.f);
		return;
	}

	dxf_ctx.handle = 100;
	if (dxf_ctx.f != NULL) {
		if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "header", insert_hdr, &err) != 0)
			rnd_message(RND_MSG_ERROR, "Can't render dxf template header\n");
	}

	if (!dxf_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	dxf_hid_export_to_file(&dxf_ctx, options, &xform);

	if (!dxf_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "footer", insert_ftr, &err) != 0)
		rnd_message(RND_MSG_ERROR, "Can't render dxf template header\n");
	fclose(dxf_ctx.f);

	if (!dxf_cam.active) dxf_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&dxf_cam) == 0) {
		if (!dxf_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "dxf cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (dxf_ctx.drawn_objs == 0) {
		if (!dxf_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "dxf cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}
}

static int dxf_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, dxf_attribute_list, sizeof(dxf_attribute_list) / sizeof(dxf_attribute_list[0]), dxf_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int dxf_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&dxf_cam, group, purpose, purpi, flags, xform);

	if (dxf_cam.fn_changed) {
		lht_err_t err;

		if (dxf_ctx.f != NULL) {
			if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "footer", insert_ftr, &err) != 0)
				rnd_message(RND_MSG_ERROR, "Can't render dxf template header\n");
			fclose(dxf_ctx.f);
		}

		dxf_ctx.f = rnd_fopen_askovr(&PCB->hidlib, dxf_cam.fn, "wb", NULL);
		if (!dxf_ctx.f) {
			perror(dxf_cam.fn);
			return 0;
		}
		if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "header", insert_hdr, &err) != 0)
			rnd_message(RND_MSG_ERROR, "Can't render dxf template header\n");
	}

	if (!dxf_cam.active) {
		if (flags & PCB_LYT_INVIS)
			return 0;
	}

	dxf_ctx.force_thin = 0;

	if (PCB_LAYER_IS_ROUTE(flags, purpi)) {
		dxf_ctx.layer_name = "outline";
		dxf_ctx.force_thin = 1;
		return 1;
	}

	if (dxf_cam.active) {
		pcb_layergrp_t *grp = pcb_get_layergrp(PCB, group);
		dxf_ctx.layer_name = grp->name;
		return 1;
	}

	if (PCB_LAYER_IS_PDRILL(flags, purpi)) {
		dxf_ctx.layer_name = "drill_plated";
		dxf_ctx.force_thin = 1;
		return 1;
	}

	if (PCB_LAYER_IS_UDRILL(flags, purpi)) {
		dxf_ctx.layer_name = "drill_unplated";
		dxf_ctx.force_thin = 1;
		return 1;
	}

	if ((flags & PCB_LYT_TOP) && (flags & PCB_LYT_COPPER)) {
		dxf_ctx.layer_name = "top_copper";
		return 1;
	}

	if ((flags & PCB_LYT_TOP) && (flags & PCB_LYT_SILK)) {
		dxf_ctx.layer_name = "top_silk";
		return 1;
	}

	if ((flags & PCB_LYT_BOTTOM) && (flags & PCB_LYT_COPPER)) {
		dxf_ctx.layer_name = "bottom_copper";
		return 1;
	}

	if ((flags & PCB_LYT_BOTTOM) && (flags & PCB_LYT_SILK)) {
		dxf_ctx.layer_name = "bottom_silk";
		return 1;
	}

	return 0;
}


static rnd_hid_gc_t dxf_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	rv->me_pointer = &dxf_hid;
	return rv;
}

static void dxf_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void dxf_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
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

static void dxf_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (rnd_color_is_drill(color) == 0)
		gc->drawing_hole = 1;
}

static void dxf_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void dxf_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}


static void dxf_set_draw_xor(rnd_hid_gc_t gc, int xor_)
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

static void dxf_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	fix_rect_coords();
}

static void dxf_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	fix_rect_coords();
}

static void dxf_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	rnd_message(RND_MSG_ERROR, "dxf_calibrate() not implemented");
	return;
}

static void dxf_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int dxf_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\ndxf exporter command line arguments:\n\n");
	rnd_hid_usage(dxf_attribute_list, sizeof(dxf_attribute_list) / sizeof(dxf_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x dxf [dxf options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_dxf(int ver_needed) { return 0; }

void pplg_uninit_export_dxf(void)
{
	rnd_export_remove_opts_by_cookie(dxf_cookie);
	rnd_hid_remove_hid(&dxf_hid);
}

int pplg_init_export_dxf(void)
{
	RND_API_CHK_VER;

	memset(&dxf_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&dxf_hid);

	dxf_hid.struct_size = sizeof(rnd_hid_t);
	dxf_hid.name = "dxf";
	dxf_hid.description = "Drawing eXchange Format exporter";
	dxf_hid.exporter = 1;

	dxf_hid.get_export_options = dxf_get_export_options;
	dxf_hid.do_export = dxf_do_export;
	dxf_hid.parse_arguments = dxf_parse_arguments;
	dxf_hid.set_layer_group = dxf_set_layer_group;
	dxf_hid.make_gc = dxf_make_gc;
	dxf_hid.destroy_gc = dxf_destroy_gc;
	dxf_hid.set_drawing_mode = dxf_set_drawing_mode;
	dxf_hid.set_color = dxf_set_color;
	dxf_hid.set_line_cap = dxf_set_line_cap;
	dxf_hid.set_line_width = dxf_set_line_width;
	dxf_hid.set_draw_xor = dxf_set_draw_xor;
	dxf_hid.draw_line = dxf_draw_line;
	dxf_hid.draw_arc = dxf_draw_arc;
	dxf_hid.draw_rect = dxf_draw_rect;
	dxf_hid.fill_circle = dxf_fill_circle;
	dxf_hid.fill_polygon = dxf_fill_polygon;
	dxf_hid.fill_polygon_offs = dxf_fill_polygon_offs;
	dxf_hid.fill_rect = dxf_fill_rect;
	dxf_hid.calibrate = dxf_calibrate;
	dxf_hid.set_crosshair = dxf_set_crosshair;
	dxf_hid.argument_array = dxf_values;

	dxf_hid.usage = dxf_usage;

	rnd_hid_register_hid(&dxf_hid);

	return 0;
}
