 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  (this file is based on PCB, interactive printed circuit board design)
  *  Copyright (C) 2006 Dan McMahill
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
#include "error.h"
#include "layer.h"
#include "layer_vis.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "plugins.h"
#include "safe_fs.h"
#include "funchash_core.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"
#include "png.h"

/* the gd library which makes this all so easy */
#include <gd.h>

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_cam.h"

#define PNG_SCALE_HACK1 0
#include "compat_misc.h"
#define pcb_hack_round(d) pcb_round(d)

#define CRASH(func) fprintf(stderr, "HID error: pcb called unimplemented PNG function %s.\n", func); abort()

static pcb_hid_t png_hid;

const char *png_cookie = "png HID";

static pcb_cam_t png_cam;

static void *color_cache = NULL;
static void *brush_cache = NULL;

static double bloat = 0;
static double scale = 1;
static pcb_coord_t x_shift = 0;
static pcb_coord_t y_shift = 0;
static int show_solder_side;
#define SCALE(w)   ((int)pcb_round((w)/scale))
#define SCALE_X(x) ((int)pcb_hack_round(((x) - x_shift)/scale))
#define SCALE_Y(y) ((int)pcb_hack_round(((show_solder_side ? (PCB->hidlib.size_y-(y)) : (y)) - y_shift)/scale))
#define SWAP_IF_SOLDER(a,b) do { int c; if (show_solder_side) { c=a; a=b; b=c; }} while (0)

/* Used to detect non-trivial outlines */
#define NOT_EDGE_X(x) ((x) != 0 && (x) != PCB->hidlib.size_x)
#define NOT_EDGE_Y(y) ((y) != 0 && (y) != PCB->hidlib.size_y)
#define NOT_EDGE(x,y) (NOT_EDGE_X(x) || NOT_EDGE_Y(y))

static void png_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);

/* The result of a failed gdImageColorAllocate() call */
#define BADC -1

typedef struct color_struct {
	/* the descriptor used by the gd library */
	int c;

	/* so I can figure out what rgb value c refers to */
	unsigned int r, g, b, a;

} color_struct;

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
	unsigned char r, g, b;
	color_struct *color;
	gdImagePtr brush;
	int is_erase;
} hid_gc_s;

static color_struct *black = NULL, *white = NULL;
static gdImagePtr im = NULL, master_im, comp_im = NULL, erase_im = NULL;
static FILE *f = 0;
static int linewidth = -1;
static int lastgroup = -1;
static gdImagePtr lastbrush = (gdImagePtr) ((void *) -1);
static int lastcap = -1;
static int last_color_r, last_color_g, last_color_b, last_cap;

/* For photo-mode we need the following layers as monochrome masks:

   top soldermask
   top silk
   copper layers
   drill
*/

#define PHOTO_FLIP_X	1
#define PHOTO_FLIP_Y	2

static int photo_mode, photo_flip;
static gdImagePtr photo_copper[PCB_MAX_LAYER + 2];
static gdImagePtr photo_silk, photo_mask, photo_drill, *photo_im;
static gdImagePtr photo_outline;
static int photo_groups[PCB_MAX_LAYERGRP + 2], photo_ngroups;
static int photo_has_inners;

static int doing_outline, have_outline;

#define FMT_gif "GIF"
#define FMT_jpg "JPEG"
#define FMT_png "PNG"

/* If this table has no elements in it, then we have no reason to
   register this HID and will refrain from doing so at the end of this
   file.  */

#undef HAVE_SOME_FORMAT

static const char *filetypes[] = {
#ifdef HAVE_GDIMAGEPNG
	FMT_png,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef HAVE_GDIMAGEGIF
	FMT_gif,
#define HAVE_SOME_FORMAT 1
#endif

#ifdef HAVE_GDIMAGEJPEG
	FMT_jpg,
#define HAVE_SOME_FORMAT 1
#endif

	NULL
};

static const char *mask_colour_names[] = {
	"green",
	"red",
	"blue",
	"purple",
	"black",
	"white",
	NULL
};

/*  These values were arrived at through trial and error.
    One potential improvement (especially for white) is
    to use separate color_structs for the multiplication
    and addition parts of the mask math.
 */
static const color_struct mask_colours[] = {
#define MASK_COLOUR_GREEN 0
	{0x3CA03CFF, 60, 160, 60, 255},
#define MASK_COLOUR_RED 1
	{0x8C1919FF, 140, 25, 25, 255},
#define MASK_COLOUR_BLUE 2
	{0x3232A0FF, 50, 50, 160, 255},
#define MASK_COLOUR_PURPLE 3
	{0x3C1446FF, 60, 20, 70, 255},
#define MASK_COLOUR_BLACK 4
	{0x141414FF, 20, 20, 20, 255},
#define MASK_COLOUR_WHITE 5
	{0xA7E6A2FF, 167, 230, 162, 255}	/* <-- needs improvement over FR4 */
};

static const char *plating_type_names[] = {
#define PLATING_TIN 0
	"tinned",
#define PLATING_GOLD 1
	"gold",
#define PLATING_SILVER 2
	"silver",
#define PLATING_COPPER 3
	"copper",
	NULL
};

static const char *silk_colour_names[] = {
	"white",
	"black",
	"yellow",
	NULL
};

static const color_struct silk_colours[] = {
#define SILK_COLOUR_WHITE 0
	{0xE0E0E0FF, 224, 224, 224, 255},
#define SILK_COLOUR_BLACK 1
	{0x0E0E0EFF, 14, 14, 14, 255},
#define SILK_COLOUR_YELLOW 2
	{0xB9B90AFF, 185, 185, 10, 255}
};

static const color_struct silk_top_shadow = {0x151515FF, 21, 21, 21, 255};
static const color_struct silk_bottom_shadow = {0x0E0E0EFF, 14, 14, 14, 255};

pcb_hid_attribute_t png_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 PNG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_pngfile 0

/* %start-doc options "93 PNG Options"
@ftable @code
@item --dpi
Scale factor in pixels/inch. Set to 0 to scale to size specified in the layout.
@end ftable
%end-doc
*/
	{"dpi", "Scale factor (pixels/inch). 0 to scale to specified size",
	 PCB_HATT_INTEGER, 0, 10000, {100, 0, 0}, 0, 0},
#define HA_dpi 1

/* %start-doc options "93 PNG Options"
@ftable @code
@item --x-max
Width of the png image in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"x-max", "Maximum width (pixels).  0 to not constrain",
	 PCB_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xmax 2

/* %start-doc options "93 PNG Options"
@ftable @code
@item --y-max
Height of the png output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"y-max", "Maximum height (pixels).  0 to not constrain",
	 PCB_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_ymax 3

/* %start-doc options "93 PNG Options"
@ftable @code
@item --xy-max
Maximum width and height of the PNG output in pixels. No constraint, when set to 0.
@end ftable
%end-doc
*/
	{"xy-max", "Maximum width and height (pixels).  0 to not constrain",
	 PCB_HATT_INTEGER, 0, 10000, {0, 0, 0}, 0, 0},
#define HA_xymax 4

/* %start-doc options "93 PNG Options"
@ftable @code
@item --as-shown
Export layers as shown on screen.
@end ftable
%end-doc
*/
	{"as-shown", "Export layers as shown on screen",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 5

/* %start-doc options "93 PNG Options"
@ftable @code
@item --monochrome
Convert output to monochrome.
@end ftable
%end-doc
*/
	{"monochrome", "Convert to monochrome",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mono 6

/* %start-doc options "93 PNG Options"
@ftable @code
@item --only-vivible
Limit the bounds of the exported PNG image to the visible items.
@end ftable
%end-doc
*/
	{"only-visible", "Limit the bounds of the PNG image to the visible items",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_only_visible 7

/* %start-doc options "93 PNG Options"
@ftable @code
@item --use-alpha
Make the background and any holes transparent.
@end ftable
%end-doc
*/
	{"use-alpha", "Make the background and any holes transparent",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, filetypes, 0},
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
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
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
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_mode 11

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-x
In photo-realistic mode, export the reverse side of the layout. Left-right flip.
@end ftable
%end-doc
*/
	{"photo-flip-x", "Show reverse side of the board, left-right flip",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_photo_flip_x 12

/* %start-doc options "93 PNG Options"
@ftable @code
@item --photo-flip-y
In photo-realistic mode, export the reverse side of the layout. Up-down flip.
@end ftable
%end-doc
*/
	{"photo-flip-y", "Show reverse side of the board, up-down flip",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, mask_colour_names, 0},
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
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, plating_type_names, 0},
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
	 PCB_HATT_ENUM, 0, 0, {0, 0, 0}, silk_colour_names, 0},
#define HA_photo_silk_colour 16

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 17

	{"ben-mode", ATTR_UNDOCUMENTED,
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_ben_mode 11

	{"ben-flip-x", ATTR_UNDOCUMENTED,
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_ben_flip_x 12

	{"ben-flip-y", ATTR_UNDOCUMENTED,
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0}
#define HA_ben_flip_y 13

};

#define NUM_OPTIONS (sizeof(png_attribute_list)/sizeof(png_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(png_attribute_list, png_cookie)

static pcb_hid_attr_val_t png_values[NUM_OPTIONS];

static const char *get_file_suffix(void)
{
	const char *result = NULL;
	const char *fmt;

	fmt = filetypes[png_attribute_list[HA_filetype].default_val.int_value];

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

static pcb_hid_attribute_t *png_get_export_options(int *n)
{
	const char *suffix = get_file_suffix();

	if ((PCB != NULL)  && (png_attribute_list[HA_pngfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &png_attribute_list[HA_pngfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return png_attribute_list;
}


static pcb_layergrp_id_t group_for_layer(int l)
{
	if (l < pcb_max_layer && l >= 0)
		return pcb_layer_get_group(PCB, l);
	/* else something unique */
	return pcb_max_group(PCB) + 3 + l;
}

static int is_solder(pcb_layergrp_id_t grp)     { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_BOTTOM; }
static int is_component(pcb_layergrp_id_t grp)  { return pcb_layergrp_flags(PCB, grp) & PCB_LYT_TOP; }

static int layer_sort(const void *va, const void *vb)
{
	int a = *(int *) va;
	int b = *(int *) vb;
	pcb_layergrp_id_t al = group_for_layer(a);
	pcb_layergrp_id_t bl = group_for_layer(b);
	int d = bl - al;

	if (a >= 0 && a < pcb_max_layer) {
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
static pcb_box_t *bounds;
static int in_mono, as_shown;
static pcb_layergrp_id_t photo_last_grp;

static void parse_bloat(const char *str)
{
	int n;
	pcb_unit_list_t extra_units = {
		{"pix", 0, 0},
		{"px", 0, 0},
		{"", 0, 0}
	};
	for(n = 0; n < (sizeof(extra_units)/sizeof(extra_units[0]))-1; n++)
		extra_units[n].scale = scale;
	if (str == NULL)
		return;
	bloat = pcb_get_value_ex(str, NULL, NULL, extra_units, "", NULL);
}

static int smshadows[3][3] = {
	{1, 0, -1},
	{0, 0, -1},
	{-1, -1, -1},
};

static int shadows[5][5] = {
	{1, 1, 0, 0, -1},
	{1, 1, 1, -1, 0},
	{0, 1, 0, -1, 0},
	{0, -1, -1, -1, 0},
	{-1, 0, 0, 0, -1},
};

/* black and white are 0 and 1 */
#define TOP_SHADOW 2
#define BOTTOM_SHADOW 3

static void ts_bs(gdImagePtr im)
{
	int x, y, sx, sy, si;
	for (x = 0; x < gdImageSX(im); x++)
		for (y = 0; y < gdImageSY(im); y++) {
			si = 0;
			for (sx = -2; sx < 3; sx++)
				for (sy = -2; sy < 3; sy++)
					if (!gdImageGetPixel(im, x + sx, y + sy))
						si += shadows[sx + 2][sy + 2];
			if (gdImageGetPixel(im, x, y)) {
				if (si > 1)
					gdImageSetPixel(im, x, y, TOP_SHADOW);
				else if (si < -1)
					gdImageSetPixel(im, x, y, BOTTOM_SHADOW);
			}
		}
}

static void ts_bs_sm(gdImagePtr im)
{
	int x, y, sx, sy, si;
	for (x = 0; x < gdImageSX(im); x++)
		for (y = 0; y < gdImageSY(im); y++) {
			si = 0;
			for (sx = -1; sx < 2; sx++)
				for (sy = -1; sy < 2; sy++)
					if (!gdImageGetPixel(im, x + sx, y + sy))
						si += smshadows[sx + 1][sy + 1];
			if (gdImageGetPixel(im, x, y)) {
				if (si > 1)
					gdImageSetPixel(im, x, y, TOP_SHADOW);
				else if (si < -1)
					gdImageSetPixel(im, x, y, BOTTOM_SHADOW);
			}
		}
}

static void clip(color_struct * dest, color_struct * source)
{
#define CLIP(var) \
  dest->var = source->var;	\
  if (dest->var > 255) dest->var = 255;	\
  if (dest->var < 0)   dest->var = 0;

	CLIP(r);
	CLIP(g);
	CLIP(b);
#undef CLIP
}

static void blend(color_struct * dest, float a_amount, color_struct * a, color_struct * b)
{
	dest->r = a->r * a_amount + b->r * (1 - a_amount);
	dest->g = a->g * a_amount + b->g * (1 - a_amount);
	dest->b = a->b * a_amount + b->b * (1 - a_amount);
}

static void multiply(color_struct * dest, color_struct * a, color_struct * b)
{
	dest->r = (a->r * b->r) / 255;
	dest->g = (a->g * b->g) / 255;
	dest->b = (a->b * b->b) / 255;
}

static void add(color_struct * dest, double a_amount, const color_struct * a, double b_amount, const color_struct * b)
{
	dest->r = a->r * a_amount + b->r * b_amount;
	dest->g = a->g * a_amount + b->g * b_amount;
	dest->b = a->b * a_amount + b->b * b_amount;

	clip(dest, dest);
}

static void subtract(color_struct * dest, double a_amount, const color_struct * a, double b_amount, const color_struct * b)
{
	dest->r = a->r * a_amount - b->r * b_amount;
	dest->g = a->g * a_amount - b->g * b_amount;
	dest->b = a->b * a_amount - b->b * b_amount;

	clip(dest, dest);
}

static void rgb(color_struct * dest, int r, int g, int b)
{
	dest->r = r;
	dest->g = g;
	dest->b = b;
}

static pcb_hid_attr_val_t *png_options;

static void png_head(void)
{
	linewidth = -1;
	lastbrush = (gdImagePtr) ((void *) -1);
	lastcap = -1;
	lastgroup = -1;
	photo_last_grp = -1;
	show_solder_side = conf_core.editor.show_solder_side;
	last_color_r = last_color_g = last_color_b = last_cap = -1;

	gdImageFilledRectangle(im, 0, 0, gdImageSX(im), gdImageSY(im), white->c);
}

static void png_foot(void)
{
	const char *fmt;
	pcb_bool format_error = pcb_false;

	if (photo_mode) {
		int x, y, darken, lg;
		color_struct white, black, fr4;

		rgb(&white, 255, 255, 255);
		rgb(&black, 0, 0, 0);
		rgb(&fr4, 70, 70, 70);

		im = master_im;

		ts_bs(photo_copper[photo_groups[0]]);
		if (photo_silk != NULL)
			ts_bs(photo_silk);
		if (photo_mask != NULL)
			ts_bs_sm(photo_mask);

		if (photo_outline && have_outline) {
			int black = gdImageColorResolve(photo_outline, 0x00, 0x00, 0x00);

			/* go all the way around the image, trying to fill the outline */
			for (x = 0; x < gdImageSX(im); x++) {
				gdImageFillToBorder(photo_outline, x, 0, black, black);
				gdImageFillToBorder(photo_outline, x, gdImageSY(im) - 1, black, black);
			}
			for (y = 1; y < gdImageSY(im) - 1; y++) {
				gdImageFillToBorder(photo_outline, 0, y, black, black);
				gdImageFillToBorder(photo_outline, gdImageSX(im) - 1, y, black, black);

			}
		}


		for (x = 0; x < gdImageSX(im); x++) {
			for (y = 0; y < gdImageSY(im); y++) {
				color_struct p, cop;
				color_struct mask_colour, silk_colour;
				int cc, mask, silk;
				int transparent;

				if (photo_outline && have_outline) {
					transparent = gdImageGetPixel(photo_outline, x, y);
				}
				else {
					transparent = 0;
				}

				mask = photo_mask ? gdImageGetPixel(photo_mask, x, y) : 0;
				silk = photo_silk ? gdImageGetPixel(photo_silk, x, y) : 0;

				darken = 0;
				for(lg = 1; lg < photo_ngroups; lg++) {
					if (photo_copper[photo_groups[lg]] && gdImageGetPixel(photo_copper[photo_groups[lg]], x, y)) {
						darken = 1;
						break;
					}
				}

				if (darken)
					rgb(&cop, 40, 40, 40);
				else
					rgb(&cop, 100, 100, 110);

				blend(&cop, 0.3, &cop, &fr4);

				cc = gdImageGetPixel(photo_copper[photo_groups[0]], x, y);
				if (cc) {
					int r;

					if (mask)
						rgb(&cop, 220, 145, 230);
					else {

						if (png_options[HA_photo_plating].int_value == PLATING_GOLD) {
							/* ENIG */
							rgb(&cop, 185, 146, 52);

							/* increase top shadow to increase shininess */
							if (cc == TOP_SHADOW)
								blend(&cop, 0.7, &cop, &white);
						}
						else if (png_options[HA_photo_plating].int_value == PLATING_TIN) {
							/* tinned */
							rgb(&cop, 140, 150, 160);

							/* add some variation to make it look more matte */
							r = (rand() % 5 - 2) * 2;
							cop.r += r;
							cop.g += r;
							cop.b += r;
						}
						else if (png_options[HA_photo_plating].int_value == PLATING_SILVER) {
							/* silver */
							rgb(&cop, 192, 192, 185);

							/* increase top shadow to increase shininess */
							if (cc == TOP_SHADOW)
								blend(&cop, 0.7, &cop, &white);
						}
						else if (png_options[HA_photo_plating].int_value == PLATING_COPPER) {
							/* copper */
							rgb(&cop, 184, 115, 51);

							/* increase top shadow to increase shininess */
							if (cc == TOP_SHADOW)
								blend(&cop, 0.7, &cop, &white);
						}
						/*FIXME: old code...can be removed after validation.   rgb(&cop, 140, 150, 160);
						   r = (pcb_rand() % 5 - 2) * 2;
						   cop.r += r;
						   cop.g += r;
						   cop.b += r; */
					}

					if (cc == TOP_SHADOW) {
						cop.r = 255 - (255 - cop.r) * 0.7;
						cop.g = 255 - (255 - cop.g) * 0.7;
						cop.b = 255 - (255 - cop.b) * 0.7;
					}
					if (cc == BOTTOM_SHADOW) {
						cop.r *= 0.7;
						cop.g *= 0.7;
						cop.b *= 0.7;
					}
				}

				if (photo_drill && !gdImageGetPixel(photo_drill, x, y)) {
					rgb(&p, 0, 0, 0);
					transparent = 1;
				}
				else if (silk) {
					silk_colour = silk_colours[png_options[HA_photo_silk_colour].int_value];
					blend(&p, 1.0, &silk_colour, &silk_colour);

					if (silk == TOP_SHADOW)
						add(&p, 1.0, &p, 1.0, &silk_top_shadow);
					else if (silk == BOTTOM_SHADOW)
						subtract(&p, 1.0, &p, 1.0, &silk_bottom_shadow);
				}
				else if (mask) {
					p = cop;
					mask_colour = mask_colours[png_options[HA_photo_mask_colour].int_value];
					multiply(&p, &p, &mask_colour);
					add(&p, 1, &p, 0.2, &mask_colour);
					if (mask == TOP_SHADOW)
						blend(&p, 0.7, &p, &white);
					if (mask == BOTTOM_SHADOW)
						blend(&p, 0.7, &p, &black);
				}
				else
					p = cop;

				if (png_options[HA_use_alpha].int_value) {

					cc = (transparent) ? gdImageColorResolveAlpha(im, 0, 0, 0, 127) : gdImageColorResolveAlpha(im, p.r, p.g, p.b, 0);

				}
				else {
					cc = (transparent) ? gdImageColorResolve(im, 0, 0, 0) : gdImageColorResolve(im, p.r, p.g, p.b);
				}

				if (photo_flip == PHOTO_FLIP_X)
					gdImageSetPixel(im, gdImageSX(im) - x - 1, y, cc);
				else if (photo_flip == PHOTO_FLIP_Y)
					gdImageSetPixel(im, x, gdImageSY(im) - y - 1, cc);
				else
					gdImageSetPixel(im, x, y, cc);
			}
		}
	}

	/* actually write out the image */
	fmt = filetypes[png_options[HA_filetype].int_value];

	if (fmt == NULL)
		format_error = pcb_true;
	else if (strcmp(fmt, FMT_gif) == 0)
#ifdef HAVE_GDIMAGEGIF
		gdImageGif(im, f);
#else
		format_error = pcb_true;
#endif
	else if (strcmp(fmt, FMT_jpg) == 0)
#ifdef HAVE_GDIMAGEJPEG
		gdImageJpeg(im, f, -1);
#else
		format_error = pcb_true;
#endif
	else if (strcmp(fmt, FMT_png) == 0)
#ifdef HAVE_GDIMAGEPNG
		gdImagePng(im, f);
#else
		format_error = pcb_true;
#endif
	else
		format_error = pcb_true;

	if (format_error)
		fprintf(stderr, "Error:  Invalid graphic file format." "  This is a bug.  Please report it.\n");
}

void png_hid_export_to_file(FILE * the_file, pcb_hid_attr_val_t * options)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	pcb_box_t tmp, region;
	pcb_hid_expose_ctx_t ctx;

	f = the_file;

	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->hidlib.size_x;
	region.Y2 = PCB->hidlib.size_y;

	png_options = options;
	if (options[HA_only_visible].int_value)
		bounds = pcb_data_bbox(&tmp, PCB->Data, pcb_false);
	else
		bounds = &region;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	as_shown = options[HA_as_shown].int_value;
	if (!options[HA_as_shown].int_value) {
		conf_force_set_bool(conf_core.editor.thin_draw, 0);
		conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
		conf_force_set_bool(conf_core.editor.show_solder_side, 0);

		qsort(pcb_layer_stack, pcb_max_layer, sizeof(pcb_layer_stack[0]), layer_sort);

		if (photo_mode) {
			int i, n = 0;
			pcb_layergrp_id_t solder_layer = -1, comp_layer = -1;
	
			pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_layer, 1);
			pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_layer, 1);
			assert(solder_layer >= 0);
			assert(comp_layer >= 0);

			photo_has_inners = 0;
			if (comp_layer < solder_layer)
				for (i = comp_layer; i <= solder_layer; i++) {
					photo_groups[n++] = i;
					if (i != comp_layer && i != solder_layer && !pcb_layergrp_is_empty(PCB, i))
						photo_has_inners = 1;
				}
			else
				for (i = comp_layer; i >= solder_layer; i--) {
					photo_groups[n++] = i;
					if (i != comp_layer && i != solder_layer && !pcb_layergrp_is_empty(PCB, i))
						photo_has_inners = 1;
				}
			if (!photo_has_inners) {
				photo_groups[1] = photo_groups[n - 1];
				n = 2;
			}
			photo_ngroups = n;

			if (photo_flip) {
				for (i = 0, n = photo_ngroups - 1; i < n; i++, n--) {
					int tmp = photo_groups[i];
					photo_groups[i] = photo_groups[n];
					photo_groups[n] = tmp;
				}
			}
		}
	}

	in_mono = options[HA_mono].int_value;
	png_head();

	if (!photo_mode && conf_core.editor.show_solder_side) {
		int i, j;
		for (i = 0, j = pcb_max_layer - 1; i < j; i++, j--) {
			int k = pcb_layer_stack[i];
			pcb_layer_stack[i] = pcb_layer_stack[j];
			pcb_layer_stack[j] = k;
		}
	}

	ctx.view = *bounds;
	pcb_hid_expose_all(&png_hid, &ctx, NULL);

	memcpy(pcb_layer_stack, saved_layer_stack, sizeof(pcb_layer_stack));
	conf_update(NULL, -1); /* restore forced sets */
}



static void png_brush_free(void **vcache, const char *name, pcb_hidval_t *val)
{
	gdImage *brush = (gdImage *)val->ptr;
	if (brush != NULL)
		gdImageDestroy(brush);
}

static void png_color_free(void **vcache, const char *name, pcb_hidval_t *val)
{
	color_struct *color = (color_struct *)val->ptr;
	free(color);
}

static void png_free_cache(void)
{
	if (color_cache)
		pcb_hid_cache_color_destroy(&color_cache, png_color_free);
	if (brush_cache != NULL)
		pcb_hid_cache_color_destroy(&brush_cache, png_brush_free);
}

static void png_do_export(pcb_hidlib_t *hidlib, pcb_hid_attr_val_t *options)
{
	int save_ons[PCB_MAX_LAYER + 2];
	int i;
	pcb_box_t tmp, *bbox;
	int w, h;
	int xmax, ymax, dpi;

	png_free_cache();

	if (!options) {
		png_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			png_values[i] = png_attribute_list[i].default_val;
		options = png_values;
	}

	pcb_cam_begin(PCB, &png_cam, options[HA_cam].str_value, png_attribute_list, NUM_OPTIONS, options);

	if (options[HA_photo_mode].int_value || options[HA_ben_mode].int_value) {
		photo_mode = 1;
		options[HA_mono].int_value = 1;
		options[HA_as_shown].int_value = 0;
		memset(photo_copper, 0, sizeof(photo_copper));
		photo_silk = photo_mask = photo_drill = 0;
		photo_outline = 0;
		if (options[HA_photo_flip_x].int_value || options[HA_ben_flip_x].int_value)
			photo_flip = PHOTO_FLIP_X;
		else if (options[HA_photo_flip_y].int_value || options[HA_ben_flip_y].int_value)
			photo_flip = PHOTO_FLIP_Y;
		else
			photo_flip = 0;
	}
	else
		photo_mode = 0;

	filename = options[HA_pngfile].str_value;
	if (!filename)
		filename = "pcb-out.png";

	/* figure out width and height of the board */
	if (options[HA_only_visible].int_value) {
		bbox = pcb_data_bbox(&tmp, PCB->Data, pcb_false);
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

	/*
	 * figure out the scale factor we need to make the image
	 * fit in our specified PNG file size
	 */
	xmax = ymax = dpi = 0;
	if (options[HA_dpi].int_value != 0) {
		dpi = options[HA_dpi].int_value;
		if (dpi < 0) {
			fprintf(stderr, "ERROR:  dpi may not be < 0\n");
			return;
		}
	}

	if (options[HA_xmax].int_value > 0) {
		xmax = options[HA_xmax].int_value;
		dpi = 0;
	}

	if (options[HA_ymax].int_value > 0) {
		ymax = options[HA_ymax].int_value;
		dpi = 0;
	}

	if (options[HA_xymax].int_value > 0) {
		dpi = 0;
		if (options[HA_xymax].int_value < xmax || xmax == 0)
			xmax = options[HA_xymax].int_value;
		if (options[HA_xymax].int_value < ymax || ymax == 0)
			ymax = options[HA_xymax].int_value;
	}

	if (xmax < 0 || ymax < 0) {
		fprintf(stderr, "ERROR:  xmax and ymax may not be < 0\n");
		return;
	}

	if (dpi > 0) {
		/*
		 * a scale of 1  means 1 pixel is 1 inch
		 * a scale of 10 means 1 pixel is 10 inches
		 */
		scale = PCB_INCH_TO_COORD(1) / dpi;
		w = pcb_round(w / scale) - PNG_SCALE_HACK1;
		h = pcb_round(h / scale) - PNG_SCALE_HACK1;
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
		pcb_message(PCB_MSG_ERROR, "png_do_export():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", w, h);
		return;
	}

#ifdef HAVE_GD_RESOLUTION
	gdImageSetResolution(im, dpi, dpi);
#endif

	master_im = im;

	parse_bloat(options[HA_bloat].str_value);

	/* 
	 * Allocate white and black -- the first color allocated
	 * becomes the background color
	 */

	white = (color_struct *) malloc(sizeof(color_struct));
	white->r = white->g = white->b = 255;
	if (options[HA_use_alpha].int_value)
		white->a = 127;
	else
		white->a = 0;
	white->c = gdImageColorAllocateAlpha(im, white->r, white->g, white->b, white->a);
	if (white->c == BADC) {
		pcb_message(PCB_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return;
	}


	black = (color_struct *) malloc(sizeof(color_struct));
	black->r = black->g = black->b = black->a = 0;
	black->c = gdImageColorAllocate(im, black->r, black->g, black->b);
	if (black->c == BADC) {
		pcb_message(PCB_MSG_ERROR, "png_do_export():  gdImageColorAllocateAlpha() returned NULL.  Aborting export.\n");
		return;
	}

	if (!png_cam.fn_template) {
		f = pcb_fopen(&PCB->hidlib, png_cam.active ? png_cam.fn : filename, "wb");
		if (!f) {
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	png_hid.force_compositing = !!photo_mode;

	if ((!png_cam.active) && (!options[HA_as_shown].int_value))
		pcb_hid_save_and_show_layer_ons(save_ons);

	png_hid_export_to_file(f, options);

	if ((!png_cam.active) && (!options[HA_as_shown].int_value))
		pcb_hid_restore_layer_ons(save_ons);

	png_foot();
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
	if (photo_silk != NULL) {
		gdImageDestroy(photo_silk);
		photo_silk = NULL;
	}
	if (photo_mask != NULL) {
		gdImageDestroy(photo_mask);
		photo_mask = NULL;
	}
	if (photo_drill != NULL) {
		gdImageDestroy(photo_drill);
		photo_drill = NULL;
	}
	if (photo_outline != NULL) {
		gdImageDestroy(photo_outline);
		photo_outline = NULL;
	}
	for(i = 0; i < PCB_MAX_LAYER + 2; i++) {
		if (photo_copper[i] != NULL) {
			gdImageDestroy(photo_copper[i]);
			photo_copper[i] = NULL;
		}
	}
	png_free_cache();
	free(white);
	free(black);

	if (pcb_cam_end(&png_cam) == 0)
		pcb_message(PCB_MSG_ERROR, "png cam export for '%s' failed to produce any content\n", options[HA_cam].str_value);
}

static int png_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(png_attribute_list, sizeof(png_attribute_list) / sizeof(png_attribute_list[0]), png_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}


static int is_mask;
static int is_photo_drill;


static int png_set_layer_group_photo(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{

	/* workaround: the outline layer vs. alpha breaks if set twice and the draw
	   code may set it twice (if there's no mech layer), but always in a row */
	if (group == photo_last_grp)
		return 1;
	photo_last_grp = group;

	is_photo_drill = (PCB_LAYER_IS_DRILL(flags, purpi) || ((flags & PCB_LYT_MECH) && PCB_LAYER_IS_ROUTE(flags, purpi)));
		if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_TOP)) {
			if (photo_flip)
				return 0;
			photo_im = &photo_silk;
		}
		else if (((flags & PCB_LYT_ANYTHING) == PCB_LYT_SILK) && (flags & PCB_LYT_BOTTOM)) {
			if (!photo_flip)
				return 0;
			photo_im = &photo_silk;
		}
		else if ((flags & PCB_LYT_MASK) && (flags & PCB_LYT_TOP)) {
			if (photo_flip)
				return 0;
			photo_im = &photo_mask;
		}
		else if ((flags & PCB_LYT_MASK) && (flags & PCB_LYT_BOTTOM)) {
			if (!photo_flip)
				return 0;
			photo_im = &photo_mask;
		}
		else if (is_photo_drill) {
			photo_im = &photo_drill;
		}
		else {
			if (PCB_LAYER_IS_OUTLINE(flags, purpi)) {
				doing_outline = 1;
				have_outline = 0;
				photo_im = &photo_outline;
			}
			else if (flags & PCB_LYT_COPPER) {
				photo_im = photo_copper + group;
			}
			else
				return 0;
		}

		if (!*photo_im) {
			static color_struct *black = NULL, *white = NULL;
			*photo_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
			if (photo_im == NULL) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", gdImageSX(im), gdImageSY(im));
				return 0;
			}


			white = (color_struct *) malloc(sizeof(color_struct));
			white->r = white->g = white->b = 255;
			white->a = 0;
			white->c = gdImageColorAllocate(*photo_im, white->r, white->g, white->b);
			if (white->c == BADC) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return 0;
			}

			black = (color_struct *) malloc(sizeof(color_struct));
			black->r = black->g = black->b = black->a = 0;
			black->c = gdImageColorAllocate(*photo_im, black->r, black->g, black->b);
			if (black->c == BADC) {
				pcb_message(PCB_MSG_ERROR, "png_set_layer(): gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return 0;
			}

			if (is_photo_drill)
				gdImageFilledRectangle(*photo_im, 0, 0, gdImageSX(im), gdImageSY(im), black->c);
		}
		im = *photo_im;
		return 1;
}

static int png_set_layer_group(pcb_hidlib_t *hidlib, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	doing_outline = 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&png_cam, group, purpose, purpi, flags, xform);
	if (png_cam.fn_changed) {
		if (f != NULL) {
			png_foot();
			fclose(f);
		}
		f = pcb_fopen(&PCB->hidlib, png_cam.fn, "wb");
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

	if (PCB_LAYER_IS_OUTLINE(flags, purpi)) {
		doing_outline = 1;
		have_outline = 0;
	}

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


static pcb_hid_gc_t png_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &png_hid;
	rv->cap = pcb_cap_round;
	rv->width = 1;
	rv->color = (color_struct *) malloc(sizeof(color_struct));
	rv->color->r = rv->color->g = rv->color->b = rv->color->a = 0;
	rv->color->c = 0;
	rv->is_erase = 0;
	return rv;
}

static void png_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static pcb_composite_op_t drawing_mode;
static void png_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	static gdImagePtr dst_im;
	drawing_mode = op;
	if ((direct) || (is_photo_drill)) /* photo drill is a special layer, no copositing on that */
		return;
	switch(op) {
		case PCB_HID_COMP_RESET:

			/* the main pixel buffer; drawn with color */
			if (comp_im == NULL) {
				comp_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
				if (!comp_im) {
					pcb_message(PCB_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on comp_im.  Corrupt export!\n", gdImageSY(im), gdImageSY(im));
					return;
				}
			}

			/* erase mask: for composite layers, tells whether the color pixel
			   should be combined back to the master image; white means combine back,
			   anything else means cut-out/forget/ignore that pixel */
			if (erase_im == NULL) {
				erase_im = gdImageCreate(gdImageSX(im), gdImageSY(im));
				if (!erase_im) {
					pcb_message(PCB_MSG_ERROR, "png_set_drawing_mode():  gdImageCreate(%d, %d) returned NULL on erase_im.  Corrupt export!\n", gdImageSY(im), gdImageSY(im));
					return;
				}
			}
			gdImagePaletteCopy(comp_im, im);
			dst_im = im;
			gdImageFilledRectangle(comp_im, 0, 0, gdImageSX(comp_im), gdImageSY(comp_im), white->c);

			gdImagePaletteCopy(erase_im, im);
			gdImageFilledRectangle(erase_im, 0, 0, gdImageSX(erase_im), gdImageSY(erase_im), black->c);
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_POSITIVE_XOR:
			im = comp_im;
			break;
		case PCB_HID_COMP_NEGATIVE:
			im = erase_im;
			break;

		case PCB_HID_COMP_FLUSH:
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

static void png_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	pcb_hidval_t cval;

	if (im == NULL)
		return;

	if (color == NULL)
		color = pcb_color_red;

	if (pcb_color_is_drill(color)) {
		gc->color = white;
		gc->is_erase = 1;
		return;
	}
	gc->is_erase = 0;

	if (in_mono || (strcmp(color->str, "#000000") == 0)) {
		gc->color = black;
		return;
	}

	if (pcb_hid_cache_color(0, color->str, &cval, &color_cache)) {
		gc->color = (color_struct *) cval.ptr;
	}
	else if (color->str[0] == '#') {
		gc->color = (color_struct *) malloc(sizeof(color_struct));
		gc->color->r = color->r;
		gc->color->g = color->g;
		gc->color->b = color->b;
		gc->color->c = gdImageColorAllocate(im, gc->color->r, gc->color->g, gc->color->b);
		if (gc->color->c == BADC) {
			pcb_message(PCB_MSG_ERROR, "png_set_color():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
			return;
		}
		cval.ptr = gc->color;
		pcb_hid_cache_color(1, color->str, &cval, &color_cache);
	}
	else {
		fprintf(stderr, "WE SHOULD NOT BE HERE!!!\n");
		gc->color = black;
	}

}

static void png_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void png_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}

static void png_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

static int unerase_override = 0;
static void use_gc(gdImagePtr im, pcb_hid_gc_t gc)
{
	int need_brush = 0;
	unsigned int gc_r = gc->color->r, gc_g = gc->color->g, gc_b = gc->color->b;

	if (unerase_override) {
		gc_r = -1;
		gc_g = -1;
		gc_b = -1;
	}
	else {
		gc_r = gc->color->r;
		gc_g = gc->color->g;
		gc_b = gc->color->b;
	}

	if (gc->me_pointer != &png_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to png HID\n");
		abort();
	}

	if (linewidth != gc->width) {
		/* Make sure the scaling doesn't erase lines completely */
		if (SCALE(gc->width) == 0 && gc->width > 0)
			gdImageSetThickness(im, 1);
		else
			gdImageSetThickness(im, SCALE(gc->width + 2 * bloat));
		linewidth = gc->width;
		need_brush = 1;
	}

	need_brush |= (gc_r != last_color_r) || (gc_g != last_color_g) || (gc_b != last_color_b) || (gc->cap != last_cap);

	if (lastbrush != gc->brush || need_brush) {
		pcb_hidval_t bval;
		char name[256];
		char type;
		int r;

		switch (gc->cap) {
		case pcb_cap_round:
			type = 'C';
			break;
		case pcb_cap_square:
			type = 'S';
			break;
		default:
			assert(!"unhandled cap");
			type = 'C';
		}
		if (gc->width)
			r = SCALE(gc->width + 2 * bloat);
		else
			r = 1;

		/* do not allow a brush size that is zero width.  In this case limit to a single pixel. */
		if (r == 0) {
			r = 1;
		}

		sprintf(name, "#%.2x%.2x%.2x_%c_%d", gc_r, gc_g, gc_b, type, r);

		last_color_r = gc_r;
		last_color_g = gc_g;
		last_color_b = gc_b;
		last_cap = gc->cap;

		if (pcb_hid_cache_color(0, name, &bval, &brush_cache)) {
			gc->brush = (gdImagePtr) bval.ptr;
		}
		else {
			int bg, fg;
			gc->brush = gdImageCreate(r, r);
			if (gc->brush == NULL) {
				pcb_message(PCB_MSG_ERROR, "use_gc():  gdImageCreate(%d, %d) returned NULL.  Aborting export.\n", r, r);
				return;
			}

			bg = gdImageColorAllocate(gc->brush, 255, 255, 255);
			if (bg == BADC) {
				pcb_message(PCB_MSG_ERROR, "use_gc():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return;
			}
			if (unerase_override)
				fg = gdImageColorAllocateAlpha(gc->brush, 255, 255, 255, 0);
			else
				fg = gdImageColorAllocateAlpha(gc->brush, gc_r, gc_g, gc_b, 0);
			if (fg == BADC) {
				pcb_message(PCB_MSG_ERROR, "use_gc():  gdImageColorAllocate() returned NULL.  Aborting export.\n");
				return;
			}
			gdImageColorTransparent(gc->brush, bg);

			/*
			 * if we shrunk to a radius/box width of zero, then just use
			 * a single pixel to draw with.
			 */
			if (r <= 1)
				gdImageFilledRectangle(gc->brush, 0, 0, 0, 0, fg);
			else {
				if (type == 'C') {
					gdImageFilledEllipse(gc->brush, r / 2, r / 2, r, r, fg);
					/* Make sure the ellipse is the right exact size.  */
					gdImageSetPixel(gc->brush, 0, r / 2, fg);
					gdImageSetPixel(gc->brush, r - 1, r / 2, fg);
					gdImageSetPixel(gc->brush, r / 2, 0, fg);
					gdImageSetPixel(gc->brush, r / 2, r - 1, fg);
				}
				else
					gdImageFilledRectangle(gc->brush, 0, 0, r - 1, r - 1, fg);
			}
			bval.ptr = gc->brush;
			pcb_hid_cache_color(1, name, &bval, &brush_cache);
		}

		gdImageSetBrush(im, gc->brush);
		lastbrush = gc->brush;

	}
}


static void png_fill_rect_(gdImagePtr im, pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	use_gc(im, gc);
	gdImageSetThickness(im, 0);
	linewidth = 0;

	if (x1 > x2) {
		pcb_coord_t t = x1;
		x2 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		pcb_coord_t t = y1;
		y2 = y2;
		y2 = t;
	}
	y1 -= bloat;
	y2 += bloat;
	SWAP_IF_SOLDER(y1, y2);

	gdImageFilledRectangle(im, SCALE_X(x1 - bloat), SCALE_Y(y1), SCALE_X(x2 + bloat) - 1, SCALE_Y(y2) - 1, unerase_override ? white->c : gc->color->c);
	have_outline |= doing_outline;
}

static void png_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	png_fill_rect_(im, gc, x1, y1, x2, y2);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_rect_(erase_im, gc, x1, y1, x2, y2);
		unerase_override = 0;
	}
}


static void png_draw_line_(gdImagePtr im, pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int x1o = 0, y1o = 0, x2o = 0, y2o = 0;
	if (x1 == x2 && y1 == y2 && !photo_mode) {
		pcb_coord_t w = gc->width / 2;
		if (gc->cap != pcb_cap_square)
			png_fill_circle(gc, x1, y1, w);
		else
			png_fill_rect(gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	use_gc(im, gc);

	if (NOT_EDGE(x1, y1) || NOT_EDGE(x2, y2))
		have_outline |= doing_outline;
	if (doing_outline) {
		/* Special case - lines drawn along the bottom or right edges
		   are brought in by a pixel to make sure we have contiguous
		   outlines.  */
		if (x1 == PCB->hidlib.size_x && x2 == PCB->hidlib.size_x) {
			x1o = -1;
			x2o = -1;
		}
		if (y1 == PCB->hidlib.size_y && y2 == PCB->hidlib.size_y) {
			y1o = -1;
			y2o = -1;
		}
	}

	gdImageSetThickness(im, 0);
	linewidth = 0;
	if (gc->cap != pcb_cap_square || x1 == x2 || y1 == y2) {
		gdImageLine(im, SCALE_X(x1) + x1o, SCALE_Y(y1) + y1o, SCALE_X(x2) + x2o, SCALE_Y(y2) + y2o, gdBrushed);
	}
	else {
		/*
		 * if we are drawing a line with a square end cap and it is
		 * not purely horizontal or vertical, then we need to draw
		 * it as a filled polygon.
		 */
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

static void png_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	png_draw_line_(im, gc, x1, y1, x2, y2);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_draw_line_(erase_im, gc, x1, y1, x2, y2);
		unerase_override = 0;
	}
}

static void png_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	png_draw_line(gc, x1, y1, x2, y1);
	png_draw_line(gc, x2, y1, x2, y2);
	png_draw_line(gc, x2, y2, x1, y2);
	png_draw_line(gc, x1, y2, x1, y1);
}


static void png_draw_arc_(gdImagePtr im, pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	pcb_angle_t sa, ea;

	use_gc(im, gc);
	gdImageSetThickness(im, 0);
	linewidth = 0;

	/*
	 * zero angle arcs need special handling as gd will output either
	 * nothing at all or a full circle when passed delta angle of 0 or 360.
	 */
	if (delta_angle == 0) {
		pcb_coord_t x = (width * cos(start_angle * M_PI / 180));
		pcb_coord_t y = (width * sin(start_angle * M_PI / 180));
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
		/*
		 * in gdImageArc, 0 degrees is to the right and +90 degrees is down
		 * in pcb, 0 degrees is to the left and +90 degrees is down
		 */
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

		/*
		 * make sure we start between 0 and 360 otherwise gd does
		 * strange things
		 */
		sa = pcb_normalize_angle(sa);
		ea = pcb_normalize_angle(ea);
	}

	have_outline |= doing_outline;

#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n", cx, cy, width, height, start_angle, delta_angle, sa, ea);
	printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
				 (void *)im, SCALE_X(cx), SCALE_Y(cy), SCALE(width), SCALE(height), sa, ea, gc->color->c);
#endif
	gdImageArc(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * width), SCALE(2 * height), sa, ea, gdBrushed);
}

static void png_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	png_draw_arc_(im, gc, cx, cy, width, height, start_angle, delta_angle);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_draw_arc_(erase_im, gc, cx, cy, width, height, start_angle, delta_angle);
		unerase_override = 0;
	}
}

static void png_fill_circle_(gdImagePtr im, pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	pcb_coord_t my_bloat;

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

static void png_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	png_fill_circle_(im, gc, cx, cy, radius);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_circle_(erase_im, gc, cx, cy, radius);
		unerase_override = 0;
	}
}


static void png_fill_polygon_offs_(gdImagePtr im, pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
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

static void png_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	png_fill_polygon_offs_(im, gc, n_coords, x, y, dx, dy);
	if ((im != erase_im) && (erase_im != NULL)) {
		unerase_override = 1;
		png_fill_polygon_offs_(erase_im, gc, n_coords, x, y, dx, dy);
		unerase_override = 0;
	}
}


static void png_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	png_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}


static void png_calibrate(double xval, double yval)
{
	CRASH("png_calibrate");
}

static void png_set_crosshair(pcb_coord_t x, pcb_coord_t y, int a)
{
}

static int png_usage(const char *topic)
{
	fprintf(stderr, "\npng exporter command line arguments:\n\n");
	pcb_hid_usage(png_attribute_list, sizeof(png_attribute_list) / sizeof(png_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x png [png options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_png(int ver_needed) { return 0; }

void pplg_uninit_export_png(void)
{
	pcb_hid_remove_attributes_by_cookie(png_cookie);
}

int pplg_init_export_png(void)
{
	PCB_API_CHK_VER;

	memset(&png_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&png_hid);
	pcb_dhlp_draw_helpers_init(&png_hid);

	png_hid.struct_size = sizeof(pcb_hid_t);
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

	png_hid.usage = png_usage;

#ifdef HAVE_SOME_FORMAT
	pcb_hid_register_hid(&png_hid);

#endif
	return 0;
}
