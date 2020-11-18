 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  (this file is based on PCB, interactive printed circuit board design)
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genht/htpp.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "board.h"
#include <librnd/core/color.h>
#include <librnd/core/color_cache.h>
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
#include "png.h"

/* the gd library which makes this all so easy */
#include <gd.h>

#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"

#define PNG_SCALE_HACK1 0
#include <librnd/core/compat_misc.h>
#define pcb_hack_round(d) rnd_round(d)

#define CRASH(func) fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", func); abort()

static rnd_hid_t png_hid;

const char *png_cookie = "png HID";

static pcb_cam_t png_cam;

static rnd_clrcache_t color_cache;
static int color_cache_inited = 0;
static htpp_t brush_cache;
static int brush_cache_inited = 0;

static double bloat = 0;
static double scale = 1;
static rnd_coord_t x_shift = 0;
static rnd_coord_t y_shift = 0;
static int show_solder_side;
static long png_drawn_objs = 0;
#define SCALE(w)   ((int)rnd_round((w)/scale))
#define SCALE_X(x) ((int)pcb_hack_round(((x) - x_shift)/scale))
#define SCALE_Y(y) ((int)pcb_hack_round(((show_solder_side ? (PCB->hidlib.size_y-(y)) : (y)) - y_shift)/scale))
#define SWAP_IF_SOLDER(a,b) do { int c; if (show_solder_side) { c=a; a=b; b=c; }} while (0)

/* Used to detect non-trivial outlines */
#define NOT_EDGE_X(x) ((x) != 0 && (x) != PCB->hidlib.size_x)
#define NOT_EDGE_Y(y) ((y) != 0 && (y) != PCB->hidlib.size_y)
#define NOT_EDGE(x,y) (NOT_EDGE_X(x) || NOT_EDGE_Y(y))

static void png_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);

/* The result of a failed gdImageColorAllocate() call */
#define BADC -1

typedef struct color_struct {
	/* the descriptor used by the gd library */
	int c;

	/* so I can figure out what rgb value c refers to */
	unsigned int r, g, b, a;

} color_struct;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	int width, r, g, b;
	color_struct *color;
	gdImagePtr brush;
	int is_erase;
} hid_gc_t;

static color_struct *black = NULL, *white = NULL;
static gdImagePtr im = NULL, master_im, comp_im = NULL, erase_im = NULL;
static FILE *f = 0;
static int linewidth = -1;
static int lastgroup = -1;
static gdImagePtr lastbrush = (gdImagePtr) ((void *) -1);
static int lastcap = -1;
static int last_color_r, last_color_g, last_color_b, last_cap;

static int doing_outline, have_outline;

#define FMT_gif "GIF"
#define FMT_jpg "JPEG"
#define FMT_png "PNG"

/* If this table has no elements in it, then we have no reason to
   register this HID and will refrain from doing so at the end of this
   file.  */

#undef HAVE_SOME_FORMAT

static const char *filetypes[] = {
#ifdef RND_HAVE_GDIMAGEPNG
	FMT_png,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef RND_HAVE_GDIMAGEGIF
	FMT_gif,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef RND_HAVE_GDIMAGEJPEG
	FMT_jpg,
#define HAVE_SOME_FORMAT 1
#endif

	NULL
};

#include "png_photo1.c"

rnd_export_opt_t png_attribute_list[] = {
/* other HIDs expect this to be first.  */

/* %start-doc options "93 PNG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_pngfile 0

/* %start-doc options "93 PNG Options"
@ftable @code
@item --dpi
Scale factor in pixels/inch. Set to 0 to scale to size specified in the layout.
@end ftable
%end-doc
*/
	{"dpi", "Scale factor (pixels/inch). 0 to scale to specified size",
	 RND_HATT_INTEGER, 0, 10000, {100, 0, 0}, 0, 0},
#define HA_dpi 1

/* %start-doc options "93 PNG Options"
@ftable @code
@item --x-max
Width of the png image in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"x-max", "Maximum width (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xmax 2

/* %start-doc options "93 PNG Options"
@ftable @code
@item --y-max
Height of the png output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"y-max", "Maximum height (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_ymax 3

/* %start-doc options "93 PNG Options"
@ftable @code
@item --xy-max
Maximum width and height of the PNG output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"xy-max", "Maximum width and height (pixels).  0 to not constrain",
	 RND_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xymax 4

/* %start-doc options "93 PNG Options"
@ftable @code
@item --as-shown
Export layers as shown on screen.
@end ftable
%end-doc
*/
	{"as-shown", "Export layers as shown on screen",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 5

/* %start-doc options "93 PNG Options"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
	{"monochrome", "Convert to monochrome",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 6

/* %start-doc options "93 PNG Options"
@ftable @code
@item --only-vivible
Limit the bounds of the exported PNG image to the visible items.
@end ftable
%end-doc
*/
	{"only-visible", "Limit the bounds of the PNG image to the visible items",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_only_visible 7

/* %start-doc options "93 PNG Options"
@ftable @code
@item --use-alpha
Make the background and any holes transparent.
@end ftable
%end-doc
*/
	{"use-alpha", "Make the background and any holes transparent",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, filetypes, 0},
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
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_mode 11

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-x
In photo-realistic mode, export the reverse side of the layout. Left-right flip.
@end ftable
%end-doc
*/
	{"photo-flip-x", "Show reverse side of the board, left-right flip",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_flip_x 12

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-y
In photo-realistic mode, export the reverse side of the layout. Up-down flip.
@end ftable
%end-doc
*/
	{"photo-flip-y", "Show reverse side of the board, up-down flip",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, mask_colour_names, 0},
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
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, plating_type_names, 0},
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
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, silk_colour_names, 0},
#define HA_photo_silk_colour 16

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 17
};

#define NUM_OPTIONS (sizeof(png_attribute_list)/sizeof(png_attribute_list[0]))

static rnd_hid_attr_val_t png_values[NUM_OPTIONS];

static const char *get_file_suffix(void)
{
	const char *result = NULL;
	const char *fmt;

	fmt = filetypes[png_attribute_list[HA_filetype].default_val.lng];

	if (fmt == NULL) { /* Do nothing */ }
	else if (strcmp(fmt, FMT_gif) == 0)
		result = ".gif";
	else if (strcmp(fmt, FMT_jpg) == 0)
		result = ".jpg";
	else if (strcmp(fmt, FMT_png) == 0)
		result = ".png";

	if (result == NULL) {
		fprintf(stderr, "Error:  Invalid graphic file format\n");
		result = ".???";
	}
	return result;
}

static rnd_export_opt_t *png_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = get_file_suffix();
	char **val = png_attribute_list[HA_pngfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &png_attribute_list[HA_pngfile], suffix);

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
static int in_mono, as_shown;

static void parse_bloat(const char *str)
{
	int n;
	rnd_unit_list_t extra_units = {
		{"pix", 0, 0},
		{"px", 0, 0},
		{"", 0, 0}
	};
	for(n = 0; n < (sizeof(extra_units)/sizeof(extra_units[0]))-1; n++)
		extra_units[n].scale = scale;
	if (str == NULL)
		return;
	bloat = rnd_get_value_ex(str, NULL, NULL, extra_units, "", NULL);
}

static void rgb(color_struct *dest, int r, int g, int b)
{
	dest->r = r;
	dest->g = g;
	dest->b = b;
}

static rnd_hid_attr_val_t *png_options;

#include "png_photo2.c"

static void png_head(void)
{
	linewidth = -1;
	lastbrush = (gdImagePtr) ((void *) -1);
	lastcap = -1;
	lastgroup = -1;
	png_photo_head();
	show_solder_side = conf_core.editor.show_solder_side;
	last_color_r = last_color_g = last_color_b = last_cap = -1;

	gdImageFilledRectangle(im, 0, 0, gdImageSX(im), gdImageSY(im), white->c);
}

static void png_foot(void)
{
	const char *fmt;
	rnd_bool format_error = rnd_false;

	if (photo_mode)
		png_photo_foot();

	/* actually write out the image */
	fmt = filetypes[png_options[HA_filetype].lng];

	if (fmt == NULL)
		format_error = rnd_true;
	else if (strcmp(fmt, FMT_gif) == 0)
#ifdef RND_HAVE_GDIMAGEGIF
		gdImageGif(im, f);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, FMT_jpg) == 0)
#ifdef RND_HAVE_GDIMAGEJPEG
		gdImageJpeg(im, f, -1);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, FMT_png) == 0)
#ifdef RND_HAVE_GDIMAGEPNG
		gdImagePng(im, f);
#else
		format_error = rnd_true;
#endif
	else
		format_error = rnd_true;

	if (format_error)
		fprintf(stderr, "Error:  Invalid graphic file format." "  This is a bug.  Please report it.\n");
}

void png_hid_export_to_file(FILE *the_file, rnd_hid_attr_val_t *options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_box_t tmp, region;
	rnd_hid_expose_ctx_t ctx;

	f = the_file;

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

		if (photo_mode)
			png_photo_as_shown();
	}

	in_mono = options[HA_mono].lng;
	png_head();

	if (!photo_mode && conf_core.editor.show_solder_side) {
		int i, j;
		for (i = 0, j = pcb_max_layer(PCB) - 1; i < j; i++, j--) {
			int k = pcb_layer_stack[i];
			pcb_layer_stack[i] = pcb_layer_stack[j];
			pcb_layer_stack[j] = k;
		}
	}

	if (as_shown) {
		/* disable (exporter default) hiding overlay in as_shown */
		xform->omit_overlay = 0;
	}

	ctx.view = *bounds;
	rnd_expose_main(&png_hid, &ctx, xform);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));
	rnd_conf_update(NULL, -1); /* restore forced sets */
}


static void png_free_cache(void)
{
	if (color_cache_inited) {
		rnd_clrcache_uninit(&color_cache);
		color_cache_inited = 0;
	}
	if (brush_cache_inited) {
		htpp_entry_t *e;
		for(e = htpp_first(&brush_cache); e != NULL; e = htpp_next(&brush_cache, e))
			gdImageDestroy(e->value);
		htpp_uninit(&brush_cache);
		brush_cache_inited = 0;
	}
}

static void png_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	int save_ons[PCB_MAX_LAYER];
	rnd_box_t tmp, *bbox;
	int w, h;
	int xmax, ymax, dpi;
	rnd_xform_t xform;

	png_free_cache();

	if (!options) {
		png_get_export_options(hid, 0);
		options = png_values;
	}

	png_drawn_objs = 0;
	pcb_cam_begin(PCB, &png_cam, &xform, options[HA_cam].str, png_attribute_list, NUM_OPTIONS, options);

	if (options[HA_photo_mode].lng) {
		photo_mode = 1;
		png_photo_do_export(options);
	}
	else
		photo_mode = 0;

	filename = options[HA_pngfile].str;
	if (!filename)
		filename = "pcb-out.png";

	/* figure out width and height of the board */
	if (options[HA_only_visible].lng) {
		bbox = pcb_data_bbox(&tmp, PCB->Data, rnd_false);
		x_shift = bbox->X1;
		y_shift = bbox->Y1;
		h = bbox->Y2 - bbox->Y1;
		w = bbox->X2 - bbox->X1;
	}
	else {
		x_shift = 0;
		y_shift = 0;
		h = PCB->hidlib.size_y;
		w = PCB->hidlib.size_x;
	}

	/* figure out the scale factor to fit in the specified PNG file size */
	xmax = ymax = dpi = 0;
	if (options[HA_dpi].lng != 0) {
		dpi = options[HA_dpi].lng;
		if (dpi < 0) {
			fprintf(stderr, "ERROR:  dpi may not be < 0\n");
			return;
		}
	}

	if (options[HA_xmax].lng > 0) {
		xmax = options[HA_xmax].lng;
		dpi = 0;
	}

	if (options[HA_ymax].lng > 0) {
		ymax = options[HA_ymax].lng;
		dpi = 0;
	}

	if (options[HA_xymax].lng > 0) {
		dpi = 0;
		if (options[HA_xymax].lng < xmax || xmax == 0)
			xmax = options[HA_xymax].lng;
		if (options[HA_xymax].lng < ymax || ymax == 0)
			ymax = options[HA_xymax].lng;
	}

	if (xmax < 0 || ymax < 0) {
		fprintf(stderr, "ERROR:  xmax and ymax may not be < 0\n");
		return;
	}

	if (dpi > 0) {
		/* a scale of 1  means 1 pixel is 1 inch
		   a scale of 10 means 1 pixel is 10 inches */
		scale = RND_INCH_TO_COORD(1) / dpi;
		w = rnd_round(w / scale) - PNG_SCALE_HACK1;
		h = rnd_round(h / scale) - PNG_SCALE_HACK1;
	}
	else if (xmax == 0 && ymax == 0) {
		fprintf(stderr, "ERROR:  You may not set both xmax, ymax," "and xy-max to zero\n");
		return;
	}
	else {
		if (ymax == 0 || ((xmax > 0)
											&& ((w / xmax) > (h / ymax)))) {
			h = (h * xmax) / w;
			scale = w / xmax;
			w = xmax;
		}
		else {
			w = (w * ymax) / h;
			scale = h / ymax;
			h = ymax;
		}
	}

	im = gdImageCreate(w, h);
	if (im == NULL) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", w, h);
		return;
	}

#ifdef RND_HAVE_GD_RESOLUTION
	gdImageSetResolution(im, dpi, dpi);
#endif

	master_im = im;

	parse_bloat(options[HA_bloat].str);

	/* Allocate white and black; the first color allocated becomes the background color */
	white = (color_struct *) malloc(sizeof(color_struct));
	white->r = white->g = white->b = 255;
	if (options[HA_use_alpha].lng)
		white->a = 127;
	else
		white->a = 0;
	white->c = gdImageColorAllocateAlpha(im, white->r, white->g, white->b, white->a);
	if (white->c == BADC) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return;
	}


	black = (color_struct *) malloc(sizeof(color_struct));
	black->r = black->g = black->b = black->a = 0;
	black->c = gdImageColorAllocate(im, black->r, black->g, black->b);
	if (black->c == BADC) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return;
	}

	if (!png_cam.fn_template) {
		f = rnd_fopen_askovr(&PCB->hidlib, png_cam.active ? png_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	png_hid.force_compositing = !!photo_mode;

	if ((!png_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_save_and_show_layer_ons(save_ons);

	png_hid_export_to_file(f, options, &xform);

	if ((!png_cam.active) && (!options[HA_as_shown].lng))
		pcb_hid_restore_layer_ons(save_ons);

	png_foot();
	if (f != NULL)
		fclose(f);

	if (master_im != NULL) {
		gdImageDestroy(master_im);
		master_im = NULL;
	}
	if (comp_im != NULL) {
		gdImageDestroy(comp_im);
		comp_im = NULL;
	}
	if (erase_im != NULL) {
		gdImageDestroy(erase_im);
		erase_im = NULL;
	}
	
	png_photo_post_export();
	png_free_cache();
	free(white);
	free(black);

	if (!png_cam.active) png_cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&png_cam) == 0) {
		if (!png_cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "png cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (png_drawn_objs == 0) {
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

	pcb_cam_set_layer_group(&png_cam, group, purpose, purpi, flags, xform);
	if (png_cam.fn_changed) {
		if (f != NULL) {
			png_foot();
			fclose(f);
		}
		f = rnd_fopen_askovr(&PCB->hidlib, png_cam.fn, "wb", NULL);
		if (!f) {
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

	if (photo_mode)
		return png_set_layer_group_photo(group, purpose, purpi, layer, flags, is_empty, xform);

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


static rnd_hid_gc_t png_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(hid_gc_t), 1);
	rv->me_pointer = &png_hid;
	rv->cap = rnd_cap_round;
	rv->width = 1;
	rv->color = (color_struct *) malloc(sizeof(color_struct));
	rv->color->r = rv->color->g = rv->color->b = rv->color->a = 0;
	rv->color->c = 0;
	rv->is_erase = 0;
	return rv;
}

static void png_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static rnd_composite_op_t drawing_mode;
static void png_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	static gdImagePtr dst_im;
	drawing_mode = op;
	if ((direct) || (is_photo_drill)) /* photo drill is a special layer, no copositing on that */
		return;
	switch(op) {
		case RND_HID_COMP_RESET:

			/* the main pixel buffer; drawn with color */
			if (comp_im == NULL) {
				comp_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
				if (!comp_im) {
					rnd_message(RND_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on comp_im.  Corrupt export!\n", gdImageSY(im), gdImageSY(im));
					return;
				}
			}

			/* erase mask: for composite layers, tells whether the color pixel
			   should be combined back to the master image; white means combine back,
			   anything else means cut-out/forget/ignore that pixel */
			if (erase_im == NULL) {
				erase_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
				if (!erase_im) {
					rnd_message(RND_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on erase_im.  Corrupt export!\n", gdImageSY(im), gdImageSY(im));
					return;
				}
			}
			gdImagePaletteCopy(comp_im, im);
			dst_im = im;
			gdImageFilledRectangle(comp_im, 0, 0, gdImageSX(comp_im), gdImageSY(comp_im), white->c);

			gdImagePaletteCopy(erase_im, im);
			gdImageFilledRectangle(erase_im, 0, 0, gdImageSX(erase_im), gdImageSY(erase_im), black->c);
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
			im = comp_im;
			break;
		case RND_HID_COMP_NEGATIVE:
			im = erase_im;
			break;

		case RND_HID_COMP_FLUSH:
		{
			int x, y, c, e;
			im = dst_im;
			gdImagePaletteCopy(im, comp_im);
			for (x = 0; x < gdImageSX(im); x++) {
				for (y = 0; y < gdImageSY(im); y++) {
					e = gdImageGetPixel(erase_im, x, y);
					c = gdImageGetPixel(comp_im, x, y);
					if ((e == white->c) && (c))
						gdImageSetPixel(im, x, y, c);
				}
			}
			break;
		}
	}
}

static void png_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	color_struct *cc;

	if (im == NULL)
		return;

	if (color == NULL)
		color = rnd_color_red;

	if (rnd_color_is_drill(color)) {
		gc->color = white;
		gc->is_erase = 1;
		return;
	}
	gc->is_erase = 0;

	if (in_mono || (color->packed == 0)) {
		gc->color = black;
		return;
	}

	if (!color_cache_inited) {
		rnd_clrcache_init(&color_cache, sizeof(color_struct), NULL);
		color_cache_inited = 1;
	}

	if ((cc = rnd_clrcache_get(&color_cache, color, 0)) != NULL) {
		gc->color = cc;
	}
	else if (color->str[0] == '#') {
		cc = rnd_clrcache_get(&color_cache, color, 1);
		gc->color = cc;
		gc->color->r = color->r;
		gc->color->g = color->g;
		gc->color->b = color->b;
		gc->color->c = gdImageColorAllocate(im, gc->color->r, gc->color->g, gc->color->b);
		if (gc->color->c == BADC) {
			rnd_message(RND_MSG_ERROR, "png_set_color():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
			return;
		}
	}
	else {
		fprintf(stderr, "WE SHOULD NOT BE HERE!!!\n");
		gc->color = black;
	}
}

static void png_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void png_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

static void png_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static unsigned brush_hash(const void *kv)
{
	const hid_gc_t *k = kv;
	return ((((unsigned)k->r) << 24) | (((unsigned)k->g) << 16) | (((unsigned)k->b) << 8) | k->cap) + (unsigned)k->width;
}

static int brush_keyeq(const void *av, const void *bv)
{
	const hid_gc_t *a = av, *b = bv;
	if (a->cap != b->cap) return 0;
	if (a->width != b->width) return 0;
	if (a->r != b->r) return 0;
	if (a->g != b->g) return 0;
	if (a->b != b->b) return 0;
	return 1;
}

static int unerase_override = 0;
static void use_gc(gdImagePtr im, rnd_hid_gc_t gc)
{
	int need_brush = 0;
	rnd_hid_gc_t agc;
	gdImagePtr brp;

	png_drawn_objs++;

	agc = gc;
	if (unerase_override) {
		agc->r = -1;
		agc->g = -1;
		agc->b = -1;
	}
	else {
		agc->r = gc->color->r;
		agc->g = gc->color->g;
		agc->b = gc->color->b;
	}

	if (agc->me_pointer != &png_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to png HID\n");
		abort();
	}

	if (linewidth != agc->width) {
		/* Make sure the scaling doesn't erase lines completely */
		if (SCALE(agc->width) == 0 && agc->width > 0)
			gdImageSetThickness(im, 1);
		else
			gdImageSetThickness(im, SCALE(agc->width + 2 * bloat));
		linewidth = agc->width;
		need_brush = 1;
	}

	need_brush |= (agc->r != last_color_r) || (agc->g != last_color_g) || (agc->b != last_color_b) || (agc->cap != last_cap);

	if (lastbrush != agc->brush || need_brush) {
		int r;

		if (agc->width)
			r = SCALE(agc->width + 2 * bloat);
		else
			r = 1;

		/* do not allow a brush size that is zero width.  In this case limit to a single pixel. */
		if (r == 0)
			r = 1;

		last_color_r = agc->r;
		last_color_g = agc->g;
		last_color_b = agc->b;
		last_cap = agc->cap;

		if (!brush_cache_inited) {
			htpp_init(&brush_cache, brush_hash, brush_keyeq);
			brush_cache_inited = 1;
		}

		if ((brp = htpp_get(&brush_cache, agc)) != NULL) {
			agc->brush = brp;
		}
		else {
			int bg, fg;
			agc->brush = gdImageCreate(r, r);
			if (agc->brush == NULL) {
				rnd_message(RND_MSG_ERROR, "use_gc():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", r, r);
				return;
			}

			bg = gdImageColorAllocate(agc->brush, 255, 255, 255);
			if (bg == BADC) {
				rnd_message(RND_MSG_ERROR, "use_gc():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return;
			}
			if (unerase_override)
				fg = gdImageColorAllocateAlpha(agc->brush, 255, 255, 255, 0);
			else
				fg = gdImageColorAllocateAlpha(agc->brush, agc->r, agc->g, agc->b, 0);
			if (fg == BADC) {
				rnd_message(RND_MSG_ERROR, "use_gc():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return;
			}
			gdImageColorTransparent(agc->brush, bg);

			/* if we shrunk to a radius/box width of zero, then just use
			   a single pixel to draw with. */
			if (r <= 1)
				gdImageFilledRectangle(agc->brush, 0, 0, 0, 0, fg);
			else {
				if (agc->cap != rnd_cap_square) {
					gdImageFilledEllipse(agc->brush, r / 2, r / 2, r, r, fg);
					/* Make sure the ellipse is the right exact size.  */
					gdImageSetPixel(agc->brush, 0, r / 2, fg);
					gdImageSetPixel(agc->brush, r - 1, r / 2, fg);
					gdImageSetPixel(agc->brush, r / 2, 0, fg);
					gdImageSetPixel(agc->brush, r / 2, r - 1, fg);
				}
				else
					gdImageFilledRectangle(agc->brush, 0, 0, r - 1, r - 1, fg);
			}
			brp = agc->brush;
			htpp_set(&brush_cache, agc, brp);
		}

		gdImageSetBrush(im, agc->brush);
		lastbrush = agc->brush;

	}
}


static void png_fill_rect_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(im, gc);
	gdImageSetThickness(im, 0);
	linewidth = 0;

	if (x1 > x2) {
		rnd_coord_t t = x1;
		x2 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		rnd_coord_t t = y1;
		y2 = y2;
		y2 = t;
	}
	y1 -= bloat;
	y2 += bloat;
	SWAP_IF_SOLDER(y1, y2);

	gdImageFilledRectangle(im, SCALE_X(x1 - bloat), SCALE_Y(y1), SCALE_X(x2 + bloat) - 1, SCALE_Y(y2) - 1, unerase_override ? white->c : gc->color->c);
	have_outline |= doing_outline;
}

static void png_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_fill_rect_(im, gc, x1, y1, x2, y2);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_rect_(erase_im, gc, x1, y1, x2, y2);
		unerase_override = 0;
	}
}


static void png_draw_line_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	int x1o = 0, y1o = 0, x2o = 0, y2o = 0;
	if (x1 == x2 && y1 == y2 && !photo_mode) {
		rnd_coord_t w = gc->width / 2;
		if (gc->cap != rnd_cap_square)
			png_fill_circle(gc, x1, y1, w);
		else
			png_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	use_gc(im, gc);

	if (NOT_EDGE(x1, y1) || NOT_EDGE(x2, y2))
		have_outline |= doing_outline;
	
		/* libgd clipping bug - lines drawn along the bottom or right edges
		   need to be brought in by a pixel to make sure they are not clipped
		   by libgd - even tho they should be visible because of thickness, they
		   would not be because the center line is off the image */
	if (x1 == PCB->hidlib.size_x && x2 == PCB->hidlib.size_x) {
		x1o = -1;
		x2o = -1;
	}
	if (y1 == PCB->hidlib.size_y && y2 == PCB->hidlib.size_y) {
		y1o = -1;
		y2o = -1;
	}

	gdImageSetThickness(im, 0);
	linewidth = 0;
	if (gc->cap != rnd_cap_square || x1 == x2 || y1 == y2) {
		gdImageLine(im, SCALE_X(x1) + x1o, SCALE_Y(y1) + y1o, SCALE_X(x2) + x2o, SCALE_Y(y2) + y2o, gdBrushed);
	}
	else {
		/* if we are drawing a line with a square end cap and it is
		   not purely horizontal or vertical, then we need to draw
		   it as a filled polygon. */
		int fg, w = gc->width, dx = x2 - x1, dy = y2 - y1, dwx, dwy;
		gdPoint p[4];
		double l = sqrt((double)dx * (double)dx + (double)dy * (double)dy) * 2.0;

		if (unerase_override)
			fg = gdImageColorResolve(im, 255, 255, 255);
		else
			fg = gdImageColorResolve(im, gc->color->r, gc->color->g, gc->color->b);

		w += 2 * bloat;
		dwx = -w / l * dy;
		dwy = w / l * dx;
		p[0].x = SCALE_X(x1 + dwx - dwy);
		p[0].y = SCALE_Y(y1 + dwy + dwx);
		p[1].x = SCALE_X(x1 - dwx - dwy);
		p[1].y = SCALE_Y(y1 - dwy + dwx);
		p[2].x = SCALE_X(x2 - dwx + dwy);
		p[2].y = SCALE_Y(y2 - dwy - dwx);
		p[3].x = SCALE_X(x2 + dwx + dwy);
		p[3].y = SCALE_Y(y2 + dwy - dwx);
		gdImageFilledPolygon(im, p, 4, fg);
	}
}

static void png_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_draw_line_(im, gc, x1, y1, x2, y2);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_draw_line_(erase_im, gc, x1, y1, x2, y2);
		unerase_override = 0;
	}
}

static void png_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_draw_line(gc, x1, y1, x2, y1);
	png_draw_line(gc, x2, y1, x2, y2);
	png_draw_line(gc, x2, y2, x1, y2);
	png_draw_line(gc, x1, y2, x1, y1);
}


static void png_draw_arc_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_angle_t sa, ea;

	use_gc(im, gc);
	gdImageSetThickness(im, 0);
	linewidth = 0;

	/* zero angle arcs need special handling as gd will output either
	   nothing at all or a full circle when passed delta angle of 0 or 360. */
	if (delta_angle == 0) {
		rnd_coord_t x = (width * cos(start_angle * M_PI / 180));
		rnd_coord_t y = (width * sin(start_angle * M_PI / 180));
		x = cx - x;
		y = cy + y;
		png_fill_circle(gc, x, y, gc->width / 2);
		return;
	}

	if ((delta_angle >= 360) || (delta_angle <= -360)) {
		/* save some expensive calculations if we are going to draw a full circle anyway */
		sa = 0;
		ea = 360;
	}
	else {
		/* in gdImageArc, 0 degrees is to the right and +90 degrees is down
		   in pcb, 0 degrees is to the left and +90 degrees is down */
		start_angle = 180 - start_angle;
		delta_angle = -delta_angle;
		if (show_solder_side) {
			start_angle = -start_angle;
			delta_angle = -delta_angle;
		}
		if (delta_angle > 0) {
			sa = start_angle;
			ea = start_angle + delta_angle;
		}
		else {
			sa = start_angle + delta_angle;
			ea = start_angle;
		}

		/* make sure we start between 0 and 360 otherwise gd does
		   strange things */
		sa = rnd_normalize_angle(sa);
		ea = rnd_normalize_angle(ea);
	}

	have_outline |= doing_outline;

#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n", cx, cy, width, height, start_angle, delta_angle, sa, ea);
	printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
				 (void *)im, SCALE_X(cx), SCALE_Y(cy), SCALE(width), SCALE(height), sa, ea, gc->color->c);
#endif
	gdImageArc(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * width), SCALE(2 * height), sa, ea, gdBrushed);
}

static void png_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	png_draw_arc_(im, gc, cx, cy, width, height, start_angle, delta_angle);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_draw_arc_(erase_im, gc, cx, cy, width, height, start_angle, delta_angle);
		unerase_override = 0;
	}
}

static void png_fill_circle_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_coord_t my_bloat;

	use_gc(im, gc);

	if (gc->is_erase)
		my_bloat = -2 * bloat;
	else
		my_bloat = 2 * bloat;

	have_outline |= doing_outline;

	gdImageSetThickness(im, 0);
	linewidth = 0;
	gdImageFilledEllipse(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * radius + my_bloat), SCALE(2 * radius + my_bloat), unerase_override ? white->c : gc->color->c);
}

static void png_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	png_fill_circle_(im, gc, cx, cy, radius);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_circle_(erase_im, gc, cx, cy, radius);
		unerase_override = 0;
	}
}


static void png_fill_polygon_offs_(gdImagePtr im, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	gdPoint *points;

	points = (gdPoint *) malloc(n_coords * sizeof(gdPoint));
	if (points == NULL) {
		fprintf(stderr, "ERROR:  png_fill_polygon():  malloc failed\n");
		exit(1);
	}

	use_gc(im, gc);
	for (i = 0; i < n_coords; i++) {
		if (NOT_EDGE(x[i], y[i]))
			have_outline |= doing_outline;
		points[i].x = SCALE_X(x[i]+dx);
		points[i].y = SCALE_Y(y[i]+dy);
	}
	gdImageSetThickness(im, 0);
	linewidth = 0;
	gdImageFilledPolygon(im, points, n_coords, unerase_override ? white->c : gc->color->c);
	free(points);
}

static void png_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	png_fill_polygon_offs_(im, gc, n_coords, x, y, dx, dy);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_polygon_offs_(erase_im, gc, n_coords, x, y, dx, dy);
		unerase_override = 0;
	}
}


static void png_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	png_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}


static void png_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	CRASH("png_calibrate");
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
#ifdef HAVE_SOME_FORMAT
	rnd_hid_remove_hid(&png_hid);
#endif
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
	png_hid.make_gc = png_make_gc;
	png_hid.destroy_gc = png_destroy_gc;
	png_hid.set_drawing_mode = png_set_drawing_mode;
	png_hid.set_color = png_set_color;
	png_hid.set_line_cap = png_set_line_cap;
	png_hid.set_line_width = png_set_line_width;
	png_hid.set_draw_xor = png_set_draw_xor;
	png_hid.draw_line = png_draw_line;
	png_hid.draw_arc = png_draw_arc;
	png_hid.draw_rect = png_draw_rect;
	png_hid.fill_circle = png_fill_circle;
	png_hid.fill_polygon = png_fill_polygon;
	png_hid.fill_polygon_offs = png_fill_polygon_offs;
	png_hid.fill_rect = png_fill_rect;
	png_hid.calibrate = png_calibrate;
	png_hid.set_crosshair = png_set_crosshair;
	png_hid.argument_array = png_values;

	png_hid.usage = png_usage;

#ifdef HAVE_SOME_FORMAT
	rnd_hid_register_hid(&png_hid);
#endif
	return 0;
}
