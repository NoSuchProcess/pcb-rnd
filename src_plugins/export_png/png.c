 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  (this file is based on PCB, interactive printed circuit board design)
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2019,2022 Tibor 'Igor2' Palinkas
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

/*
 *  Heavily based on the ps HID written by DJ Delorie
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

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
#include "hid_cam.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/compat_misc.h>

/* we depend on <gd.h> for photo mode */
#include <gd.h>

/* only because photo mode also uses direct gd calls so we included gd.h already */
#define FROM_DRAW_PIXMAP_C
#include <librnd/plugins/lib_exp_pixmap/draw_pixmap.h>

#include "png.h"


static rnd_hid_t png_hid;

const char *png_cookie = "png HID";

static pcb_cam_t png_cam;

static rnd_drwpx_t pctx_, *pctx = &pctx_;

/* If this table has no elements in it, then we have no reason to
   register this HID and will refrain from doing so at the end of this
   file.  */

#include "png_photo1.c"

static FILE *png_f;

static const rnd_export_opt_t png_attribute_list[] = {
/* other HIDs expect this to be first.  */

/* %start-doc options "93 PNG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_pngfile 0

/* %start-doc options "93 PNG Options"
@ftable @code
@item --dpi
Scale factor in pixels/inch. Set to 0 to scale to size specified in the layout.
@end ftable
%end-doc
*/
	{"dpi", "Scale factor (pixels/inch). 0 to scale to specified size",
	 RND_HATT_INTEGER, 0, 10000, {100, 0, 0}, 0},
#define HA_dpi 1

/* %start-doc options "93 PNG Options"
@ftable @code
@item --x-max
Width of the png image in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"x-max", "Maximum width (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0},
#define HA_xmax 2

/* %start-doc options "93 PNG Options"
@ftable @code
@item --y-max
Height of the png output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"y-max", "Maximum height (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0},
#define HA_ymax 3

/* %start-doc options "93 PNG Options"
@ftable @code
@item --xy-max
Maximum width and height of the PNG output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"xy-max", "Maximum width and height (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0},
#define HA_xymax 4

/* %start-doc options "93 PNG Options"
@ftable @code
@item --as-shown
Export layers as shown on screen.
@end ftable
%end-doc
*/
	{"as-shown", "Export layers as shown on screen",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_as_shown 5

/* %start-doc options "93 PNG Options"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
	{"monochrome", "Convert to monochrome",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_mono 6

/* %start-doc options "93 PNG Options"
@ftable @code
@item --only-vivible
Limit the bounds of the exported PNG image to the visible items.
@end ftable
%end-doc
*/
	{"only-visible", "Limit the bounds of the PNG image to the visible items",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_only_visible 7

/* %start-doc options "93 PNG Options"
@ftable @code
@item --use-alpha
Make the background and any holes transparent.
@end ftable
%end-doc
*/
	{"use-alpha", "Make the background and any holes transparent",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_use_alpha 8

/* %start-doc options "93 PNG Options"
@ftable @code
@item --format <string>
File format to be exported. Parameter @code{<string>} can be @samp{PNG},
@samp{GIF}, or @samp{JPEG}.
@end ftable
%end-doc
*/
	{"format", "Export file format",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, rnd_drwpx_filetypes},
#define HA_filetype 9

/* %start-doc options "93 PNG Options"
@ftable @code
@item --png-bloat <num><dim>
Amount of extra thickness to add to traces, pads, or pin edges. The parameter
@samp{<num><dim>} is a number, appended by a dimension @samp{mm}, @samp{mil}, or
@samp{pix}. If no dimension is given, the default dimension is 1/100 mil.
OBSOLETE - do not use this feature!
@end ftable
%end-doc
*/
	{"png-bloat", "Amount (in/mm/mil/pix) to add to trace/pad/pin edges (1 = 1/100 mil) (OBSOLETE - do not use this feature!)",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_bloat 10

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-mode
@item --photo-mode
Export a photo realistic image of the layout.
@end ftable
%end-doc
*/
	{"photo-mode", "Photo-realistic export mode",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_photo_mode 11

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-x
In photo-realistic mode, export the reverse side of the layout. Left-right flip.
@end ftable
%end-doc
*/
	{"photo-flip-x", "Show reverse side of the board, left-right flip",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_photo_flip_x 12

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-y
In photo-realistic mode, export the reverse side of the layout. Up-down flip.
@end ftable
%end-doc
*/
	{"photo-flip-y", "Show reverse side of the board, up-down flip",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_photo_flip_y 13

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-mask-colour
@item --photo-mask-colour <colour>
In photo-realistic mode, export the solder mask as this colour. Parameter
@code{<colour>} can be @samp{green}, @samp{red}, @samp{blue}, or @samp{purple}.
@end ftable
%end-doc
*/
	{"photo-mask-colour", "Colour for the exported colour mask (green, red, blue, purple)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, mask_colour_names},
#define HA_photo_mask_colour 14

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-plating
@item --photo-plating
In photo-realistic mode, export the exposed copper as though it has this type
of plating. Parameter @code{<colour>} can be @samp{tinned}, @samp{gold},
@samp{silver}, or @samp{copper}.
@end ftable
%end-doc
*/
	{"photo-plating", "Type of plating applied to exposed copper in photo-mode (tinned, gold, silver, copper)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, plating_type_names},
#define HA_photo_plating 15

/* %start-doc options "93 PNG Options"
@ftable @code
@cindex photo-silk-colour
@item --photo-silk-colour
In photo-realistic mode, export the silk screen as this colour. Parameter
@code{<colour>} can be @samp{white}, @samp{black}, or @samp{yellow}.
@end ftable
%end-doc
*/
	{"photo-silk-colour", "Colour for the exported colour silk (white, black, yellow)",
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, silk_colour_names},
#define HA_photo_silk_colour 16

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 17
};

#define NUM_OPTIONS (sizeof(png_attribute_list)/sizeof(png_attribute_list[0]))

static rnd_hid_attr_val_t png_values[NUM_OPTIONS];

static const rnd_export_opt_t *png_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = rnd_drwpx_get_file_suffix(png_values[HA_filetype].lng);
	const char *val = png_values[HA_pngfile].str;

	if ((PCB != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &png_values[HA_pngfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return png_attribute_list;
}


static rnd_layergrp_id_t group_for_layer(int l)
{
	if (l < pcb_max_layer(PCB) && l >= 0)
		return pcb_layer_get_group(PCB, l);
	/* else something unique */
	return pcb_max_group(PCB) + 3 + l;
}

static int is_solder(rnd_layergrp_id_t grp)     { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_BOTTOM; }
static int is_component(rnd_layergrp_id_t grp)  { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_TOP; }

static int layer_sort(const void *va, const void *vb)
{
	int a = *(int *) va;
	int b = *(int *) vb;
	rnd_layergrp_id_t al = group_for_layer(a);
	rnd_layergrp_id_t bl = group_for_layer(b);
	int d = bl - al;

	if (a >= 0 && a < pcb_max_layer(PCB)) {
		int aside = (is_solder(al) ? 0 : is_component(al) ? 2 : 1);
		int bside = (is_solder(bl) ? 0 : is_component(bl) ? 2 : 1);

		if (bside != aside)
			return bside - aside;
	}
	if (d)
		return d;
	return b - a;
}

static const char *filename;
static rnd_box_t *bounds;
static int as_shown;

static rnd_hid_attr_val_t *png_options;

#include "png_photo2.c"

static void png_head(void)
{
	pctx->ymirror = conf_core.editor.show_solder_side;
	rnd_drwpx_start(pctx);
	png_photo_head();
}

static void png_finish(FILE *f)
{
	if (pctx->photo_mode)
		png_photo_foot();

	rnd_drwpx_finish(pctx, f, png_options[HA_filetype].lng);
}


void png_hid_export_to_file(FILE *the_file, rnd_hid_attr_val_t *options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_box_t tmp, region;
	rnd_hid_expose_ctx_t ctx;

	png_f = the_file;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->hidlib.size_x;
	region.Y2 = PCB->hidlib.size_y;

	png_options = options;
	if (options[HA_only_visible].lng)
		bounds = pcb_data_bbox(&tmp, PCB->Data, rnd_false);
	else
		bounds = &region;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	as_shown = options[HA_as_shown].lng;
	if (options[HA_as_shown].lng)
		pcb_draw_setup_default_gui_xform(xform);
	if (!options[HA_as_shown].lng) {
		rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

		qsort(pcb_layer_stack, pcb_max_layer(PCB), sizeof(pcb_layer_stack[0]), layer_sort);

		if (pctx->photo_mode)
			png_photo_as_shown();
	}

	pctx->in_mono = options[HA_mono].lng;
	png_head();

	if (!pctx->photo_mode && conf_core.editor.show_solder_side) {
		int i, j;
		for (i = 0, j = pcb_max_layer(PCB) - 1; i < j; i++, j--) {
			int k = pcb_layer_stack[i];
			pcb_layer_stack[i] = pcb_layer_stack[j];
			pcb_layer_stack[j] = k;
		}
	}

	if (!png_cam.active || as_shown)
		xform->enable_silk_invis_clr = 1;

	if (as_shown) {
		/* disable (exporter default) hiding overlay in as_shown */
		xform->omit_overlay = 0;
		xform->add_gui_xform = 1;
	}

	ctx.view = *bounds;
	rnd_app.expose_main(&png_hid, &ctx, xform);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));
	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void png_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	int save_ons[PCB_MAX_LAYER];
	rnd_box_t tmp, *bbox;
	rnd_xform_t xform;

	rnd_drwpx_init(pctx, &PCB->hidlib);

	if (!options) {
		png_get_export_options(hid, 0);
		options = png_values;
	}

	pcb_cam_begin(PCB, &png_cam, &xform, options[HA_cam].str, png_attribute_list, NUM_OPTIONS, options);

	if (options[HA_photo_mode].lng) {
		pctx->photo_mode = 1;
		png_photo_do_export(options);
	}
	else
		pctx->photo_mode = 0;

	filename = options[HA_pngfile].str;
	if (!filename)
		filename = "pcb-rnd-out.png";

	/* figure out width and height of the board */
	if (options[HA_only_visible].lng)
		bbox = pcb_data_bbox(&tmp, PCB->Data, rnd_false);
	else
		bbox = NULL;

	if (rnd_drwpx_set_size(pctx, bbox, options[HA_dpi].lng, options[HA_xmax].lng, options[HA_ymax].lng, options[HA_xymax].lng) != 0) {
		rnd_drwpx_uninit(pctx);
		return;
	}

	rnd_drwpx_parse_bloat(pctx, options[HA_bloat].str);

	if (rnd_drwpx_create(pctx, options[HA_use_alpha].lng) != 0) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  Failed to create bitmap of %d * %d returned NULL.  Aborting export.\n", pctx->w, pctx->h);
		rnd_drwpx_uninit(pctx);
		return;
	}

	if (png_cam.fn_template == NULL) {
		png_f = rnd_fopen_askovr(&PCB->hidlib, png_cam.active ? png_cam.fn : filename, "wb", NULL);
		if (png_f == NULL) {
			perror(filename);
			rnd_drwpx_uninit(pctx);
			return;
		}
	}
	else
		png_f = NULL;

	png_hid.force_compositing = !!pctx->photo_mode;

	if ((!png_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_save_and_show_layer_ons(save_ons);

	png_hid_export_to_file(png_f, options, &xform);

	if ((!png_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_restore_layer_ons(save_ons);

	png_finish(png_f);
	if (png_f != NULL)
		fclose(png_f);


	png_photo_post_export();
	rnd_drwpx_uninit(pctx);

	if (!png_cam.active) png_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&png_cam) == 0) {
		if (!png_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "png cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (pctx->png_drawn_objs == 0) {
		if (!png_cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "png cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}
}

static int png_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, png_attribute_list, sizeof(png_attribute_list) / sizeof(png_attribute_list[0]), png_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}


static int is_mask;

static int png_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (flags & PCB_LYT_UI)
		return 0;

	pctx->is_photo_drill = pctx->is_photo_mech = 0;

	pcb_cam_set_layer_group(&png_cam, group, purpose, purpi, flags, xform);
	if (png_cam.fn_changed) {
		if (png_f != NULL) {
			png_finish(png_f);
			fclose(png_f);
		}
		png_f = rnd_fopen_askovr(&PCB->hidlib, png_cam.fn, "wb", NULL);
		if (!png_f) {
			perror(filename);
			return 0;
		}
		png_head();
	}

	if (!png_cam.active) {
		if (flags & PCB_LYT_NOEXPORT)
			return 0;

		if (PCB_LAYER_IS_ASSY(flags, purpi) || PCB_LAYER_IS_FAB(flags, purpi) || (flags & PCB_LYT_PASTE) || (flags & PCB_LYT_INVIS) || PCB_LAYER_IS_CSECT(flags, purpi))
			return 0;
	}

	is_mask = (flags & PCB_LYT_MASK);

	if (pctx->photo_mode)
		return png_set_layer_group_photo(group, purpose, purpi, layer, flags, is_empty, xform);

	if (png_cam.active) /* CAM decided already */
		return 1;

	if (as_shown) {
		if ((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) {
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_silk_on(PCB);
			return 0;
		}

		if ((flags & PCB_LYT_ANYTHING) == PCB_LYT_MASK)
			return PCB->LayerGroups.grp[group].vis && PCB_LAYERFLG_ON_VISIBLE_SIDE(flags);
	}
	else {
		if (is_mask)
			return 0;

		if ((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK)
			return !!(flags & PCB_LYT_TOP);
	}

	return 1;
}

static void png_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_drwpx_set_drawing_mode(pctx, hid, op, direct, screen);
}

static void png_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_drwpx_set_color(pctx, gc, color);
}

static void png_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_drwpx_fill_rect(pctx, gc, x1, y1, x2, y2);
}

static void png_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_drwpx_draw_line(pctx, gc, x1, y1, x2, y2);
}

static void png_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_draw_line(gc, x1, y1, x2, y1);
	png_draw_line(gc, x2, y1, x2, y2);
	png_draw_line(gc, x2, y2, x1, y2);
	png_draw_line(gc, x1, y2, x1, y1);
}

static void png_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_drwpx_draw_arc(pctx, gc, cx, cy, width, height, start_angle, delta_angle);
}

static void png_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_drwpx_fill_circle(pctx, gc, cx, cy, radius);
}

static void png_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_drwpx_fill_polygon_offs(pctx, gc, n_coords, x, y, dx, dy);
}


static void png_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	png_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void png_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int png_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\npng exporter command line arguments:\n\n");
	rnd_hid_usage(png_attribute_list, sizeof(png_attribute_list) / sizeof(png_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x png [png options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_png(int ver_needed) { return 0; }

void pplg_uninit_export_png(void)
{
	rnd_export_remove_opts_by_cookie(png_cookie);

	if (rnd_drwpx_has_any_format())
		rnd_hid_remove_hid(&png_hid);
}

int pplg_init_export_png(void)
{
	RND_API_CHK_VER;

	memset(&png_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&png_hid);

	png_hid.struct_size = sizeof(rnd_hid_t);
	png_hid.name = "png";
	png_hid.description = "GIF/JPEG/PNG export";
	png_hid.exporter = 1;

	png_hid.get_export_options = png_get_export_options;
	png_hid.do_export = png_do_export;
	png_hid.parse_arguments = png_parse_arguments;
	png_hid.set_layer_group = png_set_layer_group;
	png_hid.make_gc = rnd_drwpx_make_gc;
	png_hid.destroy_gc = rnd_drwpx_destroy_gc;
	png_hid.set_drawing_mode = png_set_drawing_mode;
	png_hid.set_color = png_set_color;
	png_hid.set_line_cap = rnd_drwpx_set_line_cap;
	png_hid.set_line_width = rnd_drwpx_set_line_width;
	png_hid.set_draw_xor = rnd_drwpx_set_draw_xor;
	png_hid.draw_line = png_draw_line;
	png_hid.draw_arc = png_draw_arc;
	png_hid.draw_rect = png_draw_rect;
	png_hid.fill_circle = png_fill_circle;
	png_hid.fill_polygon = png_fill_polygon;
	png_hid.fill_polygon_offs = png_fill_polygon_offs;
	png_hid.fill_rect = png_fill_rect;
	png_hid.set_crosshair = png_set_crosshair;
	png_hid.argument_array = png_values;

	png_hid.usage = png_usage;

	if (rnd_drwpx_has_any_format()) {
		rnd_hid_register_hid(&png_hid);
		rnd_hid_load_defaults(&png_hid, png_attribute_list, NUM_OPTIONS);
	}

	return 0;
}
