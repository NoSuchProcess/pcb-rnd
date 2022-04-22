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

/* The result of a failed gdImageColorAllocate() call */
#define BADC -1

typedef struct color_struct {
	/* the descriptor used by the gd library */
	int c;

	/* so I can figure out what rgb value c refers to */
	unsigned int r, g, b, a;

} color_struct;


typedef struct {
	/* public: config */
	rnd_hidlib_t *hidlib;
	double scale; /* should be 1 by default */
	double bloat;
	rnd_coord_t x_shift, y_shift;
	int show_solder_side, in_mono;

	/* public: result */
	long png_drawn_objs;

	/* private */
	rnd_clrcache_t color_cache;
	int color_cache_inited;
	htpp_t brush_cache;
	int brush_cache_inited;
	int w, h; /* in pixels */
	int dpi, xmax, ymax;
	color_struct *black, *white;
	gdImagePtr im, master_im, comp_im, erase_im;
	int last_color_r, last_color_g, last_color_b, last_cap;
	gdImagePtr lastbrush;
	int linewidth;
} rnd_png_t;

static rnd_png_t pctx_, *pctx = &pctx_;

void rnd_png_init(rnd_png_t *pctx, rnd_hidlib_t *hidlib)
{
	memset(pctx, 0, sizeof(rnd_png_t));
	pctx->hidlib = hidlib;
	pctx->scale = 1;
	pctx->lastbrush = (gdImagePtr)((void *)-1);
	pctx->linewidth = -1;
}


#define SCALE(w)   ((int)rnd_round((w)/pctx->scale))
#define SCALE_X(x) ((int)pcb_hack_round(((x) - pctx->x_shift)/pctx->scale))
#define SCALE_Y(y) ((int)pcb_hack_round(((pctx->show_solder_side ? (pctx->hidlib->size_y-(y)) : (y)) - pctx->y_shift)/pctx->scale))
#define SWAP_IF_SOLDER(a,b) do { int c; if (pctx->show_solder_side) { c=a; a=b; b=c; }} while (0)

/* Used to detect non-trivial outlines */
#define NOT_EDGE_X(x) ((x) != 0 && (x) != pctx->hidlib->size_x)
#define NOT_EDGE_Y(y) ((y) != 0 && (y) != pctx->hidlib->size_y)
#define NOT_EDGE(x,y) (NOT_EDGE_X(x) || NOT_EDGE_Y(y))

static void png_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	int width, r, g, b;
	color_struct *color;
	gdImagePtr brush;
	int is_erase;
} hid_gc_t;

#define FMT_gif "GIF"
#define FMT_jpg "JPEG"
#define FMT_png "PNG"

/* If this table has no elements in it, then we have no reason to
   register this HID and will refrain from doing so at the end of this
   file.  */

#undef HAVE_SOME_FORMAT

static const char *filetypes[] = {
#ifdef PCB_HAVE_GDIMAGEPNG
	FMT_png,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef PCB_HAVE_GDIMAGEGIF
	FMT_gif,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef PCB_HAVE_GDIMAGEJPEG
	FMT_jpg,
#define HAVE_SOME_FORMAT 1
#endif

	NULL
};

#include "png_photo1.c"

static int doing_outline, have_outline;

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
	 RND_HATT_ENUM, 0, 0, {0, 0, 0}, filetypes},
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

static const char *get_file_suffix(void)
{
	const char *result = NULL;
	const char *fmt;

	fmt = filetypes[png_values[HA_filetype].lng];

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

static const rnd_export_opt_t *png_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = get_file_suffix();
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

void rnd_png_parse_bloat(rnd_png_t *pctx, const char *str)
{
	int n;
	rnd_unit_list_t extra_units = {
		{"pix", 0, 0},
		{"px", 0, 0},
		{"", 0, 0}
	};
	for(n = 0; n < (sizeof(extra_units)/sizeof(extra_units[0]))-1; n++)
		extra_units[n].scale = pctx->scale;
	if (str == NULL)
		return;
	pctx->bloat = rnd_get_value_ex(str, NULL, NULL, extra_units, "", NULL);
}

static void rgb(color_struct *dest, int r, int g, int b)
{
	dest->r = r;
	dest->g = g;
	dest->b = b;
}

static rnd_hid_attr_val_t *png_options;

#include "png_photo2.c"

void rnd_png_start(rnd_png_t *pctx)
{
	pctx->linewidth = -1;
	pctx->lastbrush = (gdImagePtr)((void *)-1);
	pctx->show_solder_side = conf_core.editor.show_solder_side;
	pctx->last_color_r = pctx->last_color_g = pctx->last_color_b = pctx->last_cap = -1;

	gdImageFilledRectangle(pctx->im, 0, 0, gdImageSX(pctx->im), gdImageSY(pctx->im), pctx->white->c);
}

static void png_head(void)
{
	rnd_png_start(pctx);
	png_photo_head();
}



void rnd_png_finish(FILE *f)
{
	const char *fmt;
	rnd_bool format_error = rnd_false;


	/* actually write out the image */
	fmt = filetypes[png_options[HA_filetype].lng];

	if (fmt == NULL)
		format_error = rnd_true;
	else if (strcmp(fmt, FMT_gif) == 0)
#ifdef PCB_HAVE_GDIMAGEGIF
		gdImageGif(pctx->im, f);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, FMT_jpg) == 0)
#ifdef PCB_HAVE_GDIMAGEJPEG
		gdImageJpeg(pctx->im, f, -1);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, FMT_png) == 0)
#ifdef PCB_HAVE_GDIMAGEPNG
		gdImagePng(pctx->im, f);
#else
		format_error = rnd_true;
#endif
	else
		format_error = rnd_true;

	if (format_error)
		fprintf(stderr, "Error:  Invalid graphic file format." "  This is a bug.  Please report it.\n");
}

static void png_finish(FILE *f)
{
	if (photo_mode)
		png_photo_foot();

	rnd_png_finish(f);
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

		if (photo_mode)
			png_photo_as_shown();
	}

	pctx->in_mono = options[HA_mono].lng;
	png_head();

	if (!photo_mode && conf_core.editor.show_solder_side) {
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


void rnd_png_uninit(rnd_png_t *pctx)
{
	if (pctx->color_cache_inited) {
		rnd_clrcache_uninit(&pctx->color_cache);
		pctx->color_cache_inited = 0;
	}
	if (pctx->brush_cache_inited) {
		htpp_entry_t *e;
		for(e = htpp_first(&pctx->brush_cache); e != NULL; e = htpp_next(&pctx->brush_cache, e))
			gdImageDestroy(e->value);
		htpp_uninit(&pctx->brush_cache);
		pctx->brush_cache_inited = 0;
	}

	free(pctx->white);
	free(pctx->black);

	if (pctx->master_im != NULL) {
		gdImageDestroy(pctx->master_im);
		pctx->master_im = NULL;
	}
	if (pctx->comp_im != NULL) {
		gdImageDestroy(pctx->comp_im);
		pctx->comp_im = NULL;
	}
	if (pctx->erase_im != NULL) {
		gdImageDestroy(pctx->erase_im);
		pctx->erase_im = NULL;
	}
}

int rnd_png_set_size(rnd_png_t *pctx, rnd_box_t *bbox, int dpi_in, int xmax_in, int ymax_in, int xymax_in)
{
	if (bbox != NULL) {
		pctx->x_shift = bbox->X1;
		pctx->y_shift = bbox->Y1;
		pctx->h = bbox->Y2 - bbox->Y1;
		pctx->w = bbox->X2 - bbox->X1;
	}
	else {
		pctx->x_shift = 0;
		pctx->y_shift = 0;
		pctx->h = PCB->hidlib.size_y;
		pctx->w = PCB->hidlib.size_x;
	}

	/* figure out the scale factor to fit in the specified PNG file size */
	if (dpi_in != 0) {
		pctx->dpi = dpi_in;
		if (pctx->dpi < 0) {
			fprintf(stderr, "ERROR:  dpi may not be < 0\n");
			return -1;
		}
	}

	if (xmax_in > 0) {
		pctx->xmax = xmax_in;
		pctx->dpi = 0;
	}

	if (ymax_in > 0) {
		pctx->ymax = ymax_in;
		pctx->dpi = 0;
	}

	if (xymax_in > 0) {
		pctx->dpi = 0;
		if (xymax_in < pctx->xmax || pctx->xmax == 0)
			pctx->xmax = xymax_in;
		if (xymax_in < pctx->ymax || pctx->ymax == 0)
			pctx->ymax = xymax_in;
	}

	if (pctx->xmax < 0 || pctx->ymax < 0) {
		fprintf(stderr, "ERROR:  xmax and ymax may not be < 0\n");
		return -1;
	}

	return 0;
}

int rnd_png_create(rnd_png_t *pctx, int use_alpha)
{
	if (pctx->dpi > 0) {
		/* a scale of 1  means 1 pixel is 1 inch
		   a scale of 10 means 1 pixel is 10 inches */
		pctx->scale = RND_INCH_TO_COORD(1) / pctx->dpi;
		pctx->w = rnd_round(pctx->w / pctx->scale) - PNG_SCALE_HACK1;
		pctx->h = rnd_round(pctx->h / pctx->scale) - PNG_SCALE_HACK1;
	}
	else if (pctx->xmax == 0 && pctx->ymax == 0) {
		fprintf(stderr, "ERROR:  You may not set both xmax, ymax, and xy-max to zero\n");
		return -1;
	}
	else {
		if (pctx->ymax == 0 || ((pctx->xmax > 0) && ((pctx->w / pctx->xmax) > (pctx->h / pctx->ymax)))) {
			pctx->h = (pctx->h * pctx->xmax) / pctx->w;
			pctx->scale = pctx->w / pctx->xmax;
			pctx->w = pctx->xmax;
		}
		else {
			pctx->w = (pctx->w * pctx->ymax) / pctx->h;
			pctx->scale = pctx->h / pctx->ymax;
			pctx->h = pctx->ymax;
		}
	}

	pctx->im = gdImageCreate(pctx->w, pctx->h);

#ifdef PCB_HAVE_GD_RESOLUTION
	gdImageSetResolution(pctx->im, pctx->dpi, pctx->dpi);
#endif

	pctx->master_im = pctx->im;

	/* Allocate white and black; the first color allocated becomes the background color */
	pctx->white = (color_struct *)malloc(sizeof(color_struct));
	pctx->white->r = pctx->white->g = pctx->white->b = 255;
	if (use_alpha)
		pctx->white->a = 127;
	else
		pctx->white->a = 0;
	pctx->white->c = gdImageColorAllocateAlpha(pctx->im, pctx->white->r, pctx->white->g, pctx->white->b, pctx->white->a);
	if (pctx->white->c == BADC) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return -1;
	}


	pctx->black = (color_struct *)malloc(sizeof(color_struct));
	pctx->black->r = pctx->black->g = pctx->black->b = pctx->black->a = 0;
	pctx->black->c = gdImageColorAllocate(pctx->im, pctx->black->r, pctx->black->g, pctx->black->b);
	if (pctx->black->c == BADC) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return -1;
	}

	return 0;
}

static void png_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	int save_ons[PCB_MAX_LAYER];
	rnd_box_t tmp, *bbox;
	rnd_xform_t xform;
	FILE *f;

	rnd_png_init(pctx, &PCB->hidlib);

	if (!options) {
		png_get_export_options(hid, 0);
		options = png_values;
	}

	pcb_cam_begin(PCB, &png_cam, &xform, options[HA_cam].str, png_attribute_list, NUM_OPTIONS, options);

	if (options[HA_photo_mode].lng) {
		photo_mode = 1;
		png_photo_do_export(options);
	}
	else
		photo_mode = 0;

	filename = options[HA_pngfile].str;
	if (!filename)
		filename = "pcb-rnd-out.png";

	/* figure out width and height of the board */
	if (options[HA_only_visible].lng)
		bbox = pcb_data_bbox(&tmp, PCB->Data, rnd_false);
	else
		bbox = NULL;

	if (rnd_png_set_size(pctx, bbox, options[HA_dpi].lng, options[HA_xmax].lng, options[HA_ymax].lng, options[HA_xymax].lng) != 0) {
		rnd_png_uninit(pctx);
		return;
	}

	rnd_png_parse_bloat(pctx, options[HA_bloat].str);

	if (rnd_png_create(pctx, options[HA_use_alpha].lng) != 0) {
		rnd_message(RND_MSG_ERROR, "png_do_export():  Failed to create bitmap of %d * %d returned NULL.  Aborting export.\n", pctx->w, pctx->h);
		rnd_png_uninit(pctx);
		return;
	}

	if (!png_cam.fn_template) {
		f = rnd_fopen_askovr(&PCB->hidlib, png_cam.active ? png_cam.fn : filename, "wb", NULL);
		if (!f) {
			perror(filename);
			rnd_png_uninit(pctx);
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

	png_finish(f);
	if (f != NULL)
		fclose(f);


	png_photo_post_export();
	rnd_png_uninit(pctx);

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

	is_photo_drill = is_photo_mech = 0;

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

	if (photo_mode)
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
			if (pctx->comp_im == NULL) {
				pctx->comp_im = gdImageCreate(gdImageSX(pctx->im), gdImageSY(pctx->im));
				if (!pctx->comp_im) {
					rnd_message(RND_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on pctx->comp_im.  Corrupt export!\n", gdImageSY(pctx->im), gdImageSY(pctx->im));
					return;
				}
			}

			/* erase mask: for composite layers, tells whether the color pixel
			   should be combined back to the master image; white means combine back,
			   anything else means cut-out/forget/ignore that pixel */
			if (pctx->erase_im == NULL) {
				pctx->erase_im = gdImageCreate(gdImageSX(pctx->im), gdImageSY(pctx->im));
				if (!pctx->erase_im) {
					rnd_message(RND_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on pctx->erase_im.  Corrupt export!\n", gdImageSY(pctx->im), gdImageSY(pctx->im));
					return;
				}
			}
			gdImagePaletteCopy(pctx->comp_im, pctx->im);
			dst_im = pctx->im;
			gdImageFilledRectangle(pctx->comp_im, 0, 0, gdImageSX(pctx->comp_im), gdImageSY(pctx->comp_im), pctx->white->c);

			gdImagePaletteCopy(pctx->erase_im, pctx->im);
			gdImageFilledRectangle(pctx->erase_im, 0, 0, gdImageSX(pctx->erase_im), gdImageSY(pctx->erase_im), pctx->black->c);
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
			pctx->im = pctx->comp_im;
			break;
		case RND_HID_COMP_NEGATIVE:
			pctx->im = pctx->erase_im;
			break;

		case RND_HID_COMP_FLUSH:
		{
			int x, y, c, e;
			pctx->im = dst_im;
			gdImagePaletteCopy(pctx->im, pctx->comp_im);
			for (x = 0; x < gdImageSX(pctx->im); x++) {
				for (y = 0; y < gdImageSY(pctx->im); y++) {
					e = gdImageGetPixel(pctx->erase_im, x, y);
					c = gdImageGetPixel(pctx->comp_im, x, y);
					if ((e == pctx->white->c) && (c))
						gdImageSetPixel(pctx->im, x, y, c);
				}
			}
			break;
		}
	}
}

static void png_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	color_struct *cc;

	if (pctx->im == NULL)
		return;

	if (color == NULL)
		color = rnd_color_red;

	if (rnd_color_is_drill(color) || is_photo_mech) {
		gc->color = pctx->white;
		gc->is_erase = 1;
		return;
	}
	gc->is_erase = 0;

	if (pctx->in_mono || (color->packed == 0)) {
		gc->color = pctx->black;
		return;
	}

	if (!pctx->color_cache_inited) {
		rnd_clrcache_init(&pctx->color_cache, sizeof(color_struct), NULL);
		pctx->color_cache_inited = 1;
	}

	if ((cc = rnd_clrcache_get(&pctx->color_cache, color, 0)) != NULL) {
		gc->color = cc;
	}
	else if (color->str[0] == '#') {
		cc = rnd_clrcache_get(&pctx->color_cache, color, 1);
		gc->color = cc;
		gc->color->r = color->r;
		gc->color->g = color->g;
		gc->color->b = color->b;
		gc->color->c = gdImageColorAllocate(pctx->im, gc->color->r, gc->color->g, gc->color->b);
		if (gc->color->c == BADC) {
			rnd_message(RND_MSG_ERROR, "png_set_color():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
			return;
		}
	}
	else {
		fprintf(stderr, "WE SHOULD NOT BE HERE!!!\n");
		gc->color = pctx->black;
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

	pctx->png_drawn_objs++;

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

	if (pctx->linewidth != agc->width) {
		/* Make sure the scaling doesn't erase lines completely */
		if (SCALE(agc->width) == 0 && agc->width > 0)
			gdImageSetThickness(im, 1);
		else
			gdImageSetThickness(im, SCALE(agc->width + 2 * pctx->bloat));
		pctx->linewidth = agc->width;
		need_brush = 1;
	}

	need_brush |= (agc->r != pctx->last_color_r) || (agc->g != pctx->last_color_g) || (agc->b != pctx->last_color_b) || (agc->cap != pctx->last_cap);

	if (pctx->lastbrush != agc->brush || need_brush) {
		int r;

		if (agc->width)
			r = SCALE(agc->width + 2 * pctx->bloat);
		else
			r = 1;

		/* do not allow a brush size that is zero width.  In this case limit to a single pixel. */
		if (r == 0)
			r = 1;

		pctx->last_color_r = agc->r;
		pctx->last_color_g = agc->g;
		pctx->last_color_b = agc->b;
		pctx->last_cap = agc->cap;

		if (!pctx->brush_cache_inited) {
			htpp_init(&pctx->brush_cache, brush_hash, brush_keyeq);
			pctx->brush_cache_inited = 1;
		}

		if ((brp = htpp_get(&pctx->brush_cache, agc)) != NULL) {
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
			htpp_set(&pctx->brush_cache, agc, brp);
		}

		gdImageSetBrush(im, agc->brush);
		pctx->lastbrush = agc->brush;
	}
}


static void png_fill_rect_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(im, gc);
	gdImageSetThickness(im, 0);
	pctx->linewidth = 0;

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
	y1 -= pctx->bloat;
	y2 += pctx->bloat;
	SWAP_IF_SOLDER(y1, y2);

	gdImageFilledRectangle(im, SCALE_X(x1 - pctx->bloat), SCALE_Y(y1), SCALE_X(x2 + pctx->bloat) - 1, SCALE_Y(y2) - 1, unerase_override ? pctx->white->c : gc->color->c);
	have_outline |= doing_outline;
}

static void png_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_fill_rect_(pctx->im, gc, x1, y1, x2, y2);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		unerase_override = 1;
		png_fill_rect_(pctx->erase_im, gc, x1, y1, x2, y2);
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
	pctx->linewidth = 0;
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

		w += 2 * pctx->bloat;
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
	png_draw_line_(pctx->im, gc, x1, y1, x2, y2);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		unerase_override = 1;
		png_draw_line_(pctx->erase_im, gc, x1, y1, x2, y2);
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
	pctx->linewidth = 0;

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
		if (pctx->show_solder_side) {
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
	png_draw_arc_(pctx->im, gc, cx, cy, width, height, start_angle, delta_angle);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		unerase_override = 1;
		png_draw_arc_(pctx->erase_im, gc, cx, cy, width, height, start_angle, delta_angle);
		unerase_override = 0;
	}
}

static void png_fill_circle_(gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_coord_t my_bloat;

	use_gc(im, gc);

	if (gc->is_erase)
		my_bloat = -2 * pctx->bloat;
	else
		my_bloat = 2 * pctx->bloat;

	have_outline |= doing_outline;

	gdImageSetThickness(im, 0);
	pctx->linewidth = 0;
	gdImageFilledEllipse(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * radius + my_bloat), SCALE(2 * radius + my_bloat), unerase_override ? pctx->white->c : gc->color->c);
}

static void png_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	png_fill_circle_(pctx->im, gc, cx, cy, radius);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		unerase_override = 1;
		png_fill_circle_(pctx->erase_im, gc, cx, cy, radius);
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
	pctx->linewidth = 0;
	gdImageFilledPolygon(im, points, n_coords, unerase_override ? pctx->white->c : gc->color->c);
	free(points);
}

static void png_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	png_fill_polygon_offs_(pctx->im, gc, n_coords, x, y, dx, dy);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		unerase_override = 1;
		png_fill_polygon_offs_(pctx->erase_im, gc, n_coords, x, y, dx, dy);
		unerase_override = 0;
	}
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
	png_hid.set_crosshair = png_set_crosshair;
	png_hid.argument_array = png_values;

	png_hid.usage = png_usage;

#ifdef HAVE_SOME_FORMAT
	rnd_hid_register_hid(&png_hid);
	rnd_hid_load_defaults(&png_hid, png_attribute_list, NUM_OPTIONS);
#endif

	return 0;
}
