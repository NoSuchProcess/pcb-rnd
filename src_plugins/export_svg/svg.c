/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2006 Dan McMahill
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Based on the png exporter by Dan McMahill
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

#include "math_helper.h"
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

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_cam.h"


static pcb_hid_t svg_hid;

const char *svg_cookie = "svg HID";

static pcb_cam_t svg_cam;

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
	char *color;
	int drill;
	unsigned warned_elliptical:1;
} hid_gc_s;

static const char *CAPS(pcb_cap_style_t cap)
{
	switch (cap) {
		case pcb_cap_round:
			return "round";
		case pcb_cap_square:
			return "square";
		default:
			assert(!"unhandled cap");
			return "round";
	}
	return "";
}

static FILE *f = NULL;
static int group_open = 0;
static int opacity = 100, drawing_mask, drawing_hole, photo_mode, flip;

static gds_t sbright, sdark, snormal, sclip;
static pcb_composite_op_t drawing_mode;
static int comp_cnt;

/* Photo mode colors and hacks */
static const char *board_color = "#464646";
static const char *mask_color = "#00ff00";
static float mask_opacity_factor = 0.5;

static enum {
	PHOTO_MASK,
	PHOTO_SILK,
	PHOTO_COPPER,
	PHOTO_INNER
} photo_color;

static struct {
	const char *bright;
	const char *normal;
	const char *dark;
	pcb_coord_t offs;
} photo_palette[] = {
	/* MASK */   { "#00ff00", "#00ff00", "#00ff00", PCB_MM_TO_COORD(0) },
	/* SILK */   { "#ffffff", "#eeeeee", "#aaaaaa", PCB_MM_TO_COORD(0) },
	/* COPPER */ { "#bbbbbb", "#707090", "#555555", PCB_MM_TO_COORD(0.05) },
	/* INNER */  { "#222222", "#111111", "#000000", PCB_MM_TO_COORD(0) }
};

pcb_hid_attribute_t svg_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 SVG Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
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
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
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
	 PCB_HATT_INTEGER, 0, 100, {100, 0, 0}, 0, 0},
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
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_flip 3

	{"as-shown", "Render similar to as shown on screen (display overlays)",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_as_shown 4

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0}
#define HA_cam 5
};

#define NUM_OPTIONS (sizeof(svg_attribute_list)/sizeof(svg_attribute_list[0]))

#define TRX(x)
#define TRY(y) \
do { \
	if (flip) \
		y = PCB->hidlib.size_y - y; \
} while(0)


PCB_REGISTER_ATTRIBUTES(svg_attribute_list, svg_cookie)

static pcb_hid_attr_val_t svg_values[NUM_OPTIONS];

static pcb_hid_attribute_t *svg_get_export_options(pcb_hid_t *hid, int *n)
{
	const char *suffix = ".svg";

	if ((PCB != NULL)  && (svg_attribute_list[HA_svgfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &svg_attribute_list[HA_svgfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return svg_attribute_list;
}

void svg_hid_export_to_file(FILE * the_file, pcb_hid_attr_val_t * options)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	pcb_hid_expose_ctx_t ctx;
	pcb_xform_t *xform = NULL, xform_tmp;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	f = the_file;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	{
		conf_force_set_bool(conf_core.editor.thin_draw, 0);
		conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
		conf_force_set_bool(conf_core.editor.show_solder_side, 0);

		if (options[HA_photo_mode].int_value) {
			photo_mode = 1;
		}
		else
			photo_mode = 0;

		if (options[HA_flip].int_value) {
			flip = 1;
			conf_force_set_bool(conf_core.editor.show_solder_side, 1);
		}
		else
			flip = 0;
	}

	if (photo_mode) {
		pcb_fprintf(f, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y, board_color);
	}

	opacity = options[HA_opacity].int_value;

	gds_init(&sbright);
	gds_init(&sdark);
	gds_init(&snormal);

	if (options[HA_as_shown].int_value) {
		/* disable (exporter default) hiding overlay in as_shown */
		memset(&xform_tmp, 0, sizeof(xform_tmp));
		xform = &xform_tmp;
		xform_tmp.omit_overlay = 0;
	}

	pcbhl_expose_main(&svg_hid, &ctx, xform);

	conf_update(NULL, -1); /* restore forced sets */
}

static void group_close()
{
	if (group_open == 1) {
		if (gds_len(&sdark) > 0) {
			fprintf(f, "<!--dark-->\n");
			fprintf(f, "%s", sdark.array);
			gds_truncate(&sdark, 0);
		}

		if (gds_len(&sbright) > 0) {
			fprintf(f, "<!--bright-->\n");
			fprintf(f, "%s", sbright.array);
			gds_truncate(&sbright, 0);
		}

		if (gds_len(&snormal) > 0) {
			fprintf(f, "<!--normal-->\n");
			fprintf(f, "%s", snormal.array);
			gds_truncate(&snormal, 0);
		}

	}
	fprintf(f, "</g>\n");
}

static void svg_header(void)
{
	pcb_coord_t w, h, x1, y1, x2, y2;

	fprintf(f, "<?xml version=\"1.0\"?>\n");
	w = PCB->hidlib.size_x;
	h = PCB->hidlib.size_y;
	while((w < PCB_MM_TO_COORD(1024)) && (h < PCB_MM_TO_COORD(1024))) {
		w *= 2;
		h *= 2;
	}

	x1 = PCB_MM_TO_COORD(2);
	y1 = PCB_MM_TO_COORD(2);
	x2 = PCB->hidlib.size_x;
	y2 = PCB->hidlib.size_y;
	x2 += PCB_MM_TO_COORD(5);
	y2 += PCB_MM_TO_COORD(5);
	pcb_fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"%mm\" height=\"%mm\" viewBox=\"-%mm -%mm %mm %mm\">\n", w, h, x1, y1, x2, y2);
}

static void svg_footer(void)
{
	while(group_open) {
		group_close();
		group_open--;
	}

	fprintf(f, "</svg>\n");
}

static void svg_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	int i;

	comp_cnt = 0;

	if (!options) {
		svg_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			svg_values[i] = svg_attribute_list[i].default_val;
		options = svg_values;
	}

	pcb_cam_begin(PCB, &svg_cam, options[HA_cam].str_value, svg_attribute_list, NUM_OPTIONS, options);

	if (svg_cam.fn_template == NULL) {
		filename = options[HA_svgfile].str_value;
		if (!filename)
			filename = "pcb.svg";

		f = pcb_fopen(&PCB->hidlib, svg_cam.active ? svg_cam.fn : filename, "wb");
		if (!f) {
			perror(filename);
			return;
		}
		svg_header();
	}
	else
		f = NULL;

	if (!svg_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	svg_hid_export_to_file(f, options);

	if (!svg_cam.active)
		pcb_hid_restore_layer_ons(save_ons);

	svg_footer();
	fclose(f);
	f = NULL;

	if (pcb_cam_end(&svg_cam) == 0)
		pcb_message(PCB_MSG_ERROR, "svg cam export for '%s' failed to produce any content\n", options[HA_cam].str_value);
}

static int svg_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_hid_register_attributes(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]), svg_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

static int svg_set_layer_group(pcb_hid_t *hid, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	int opa, is_our_mask = 0, is_our_silk = 0;

	if (is_empty)
		return 0;

	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&svg_cam, group, purpose, purpi, flags, xform);

	if (svg_cam.fn_changed) {
		if (f != NULL) {
			svg_footer();
			fclose(f);
		}

		f = pcb_fopen(&PCB->hidlib, svg_cam.fn, "wb");
		if (f == NULL) {
			perror(svg_cam.fn);
			return 0;
		}
		svg_header();
	}

	if (!svg_cam.active) {
		if (flags & PCB_LYT_INVIS)
			return 0;

		if (flags & PCB_LYT_MASK) {
			if ((!photo_mode) && (!PCB->LayerGroups.grp[group].vis))
				return 0; /* not in photo mode or not visible */
		}
	}

	switch(flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK: is_our_mask = PCB_LAYERFLG_ON_VISIBLE_SIDE(flags); break;
		case PCB_LYT_SILK: is_our_silk = PCB_LAYERFLG_ON_VISIBLE_SIDE(flags); break;
		default:;
	}

	if (!(flags & PCB_LYT_COPPER) && (!is_our_silk) && (!is_our_mask) && !(PCB_LAYER_IS_DRILL(flags, purpi)) && !(PCB_LAYER_IS_ROUTE(flags, purpi)))
		return 0;

	while(group_open) {
		group_close();
		group_open--;
	}

	{
		gds_t tmp_ln;
		const char *name;
		gds_init(&tmp_ln);
		name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);
		fprintf(f, "<g id=\"layer_%ld_%s\"", group, name);
		gds_uninit(&tmp_ln);
	}

	opa = opacity;
	if (is_our_mask)
		opa *= mask_opacity_factor;
	if (opa != 100)
		fprintf(f, " opacity=\"%.2f\"", ((float)opa) / 100.0);
	fprintf(f, ">\n");
	group_open = 1;

	if (photo_mode) {
		if (is_our_silk)
			photo_color = PHOTO_SILK;
		else if (is_our_mask)
			photo_color = PHOTO_MASK;
		else if (group >= 0) {
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				photo_color = PHOTO_COPPER;
			else
				photo_color = PHOTO_INNER;
		}
	}

	drawing_hole = PCB_LAYER_IS_DRILL(flags, purpi);

	return 1;
}


static pcb_hid_gc_t svg_make_gc(pcb_hid_t *hid)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &svg_hid;
	rv->cap = pcb_cap_round;
	rv->width = 1;
	rv->color = NULL;
	return rv;
}

static void svg_destroy_gc(pcb_hid_t *hid, pcb_hid_gc_t gc)
{
	free(gc);
}

static void svg_set_drawing_mode(pcb_hid_t *hid, pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	drawing_mode = op;

	if (direct)
		return;

	switch(op) {
		case PCB_HID_COMP_RESET:
			comp_cnt++;
			gds_init(&sclip);
			pcb_append_printf(&snormal, "<!-- Composite: reset -->\n");
			pcb_append_printf(&snormal, "<defs>\n");
			pcb_append_printf(&snormal, "<g id=\"comp_pixel_%d\">\n", comp_cnt);
			pcb_append_printf(&sclip, "<mask id=\"comp_clip_%d\" maskUnits=\"userSpaceOnUse\" x=\"0\" y=\"0\" width=\"%mm\" height=\"%mm\">\n", comp_cnt, PCB->hidlib.size_x, PCB->hidlib.size_y);
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_POSITIVE_XOR:
		case PCB_HID_COMP_NEGATIVE:
			break;

		case PCB_HID_COMP_FLUSH:
			pcb_append_printf(&snormal, "</g>\n");
			pcb_append_printf(&sclip, "</mask>\n");
			gds_append_str(&snormal, sclip.array);
			pcb_append_printf(&snormal, "</defs>\n");
			pcb_append_printf(&snormal, "<use xlink:href=\"#comp_pixel_%d\" mask=\"url(#comp_clip_%d)\"/>\n", comp_cnt, comp_cnt);
			pcb_append_printf(&snormal, "<!-- Composite: finished -->\n");
			gds_uninit(&sclip);
			break;
	}
}

static const char *svg_color(pcb_hid_gc_t gc)
{
	return gc->color;
}

static const char *svg_clip_color(pcb_hid_gc_t gc)
{
	if ((drawing_mode == PCB_HID_COMP_POSITIVE) || (drawing_mode == PCB_HID_COMP_POSITIVE_XOR))
		return "#FFFFFF";
	if (drawing_mode == PCB_HID_COMP_NEGATIVE)
		return "#000000";
	return NULL;
}

static void svg_set_color(pcb_hid_gc_t gc, const pcb_color_t *color)
{
	const char *name;
	gc->drill = 0;

	if (color == NULL)
		name = "#ff0000";
	else
		name = color->str;

	if (pcb_color_is_drill(color)) {
		name = "#ffffff";
		gc->drill = 1;
	}
	if (drawing_mask)
		name = mask_color;
	if ((gc->color != NULL) && (strcmp(gc->color, name) == 0))
		return;
	free(gc->color);
	gc->color = pcb_strdup(name);
}

static void svg_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void svg_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width < PCB_MM_TO_COORD(0.01) ? PCB_MM_TO_COORD(0.01) : width;
}

static void indent(gds_t *s)
{
	static char ind[] = "                                                                              ";
	if (group_open < sizeof(ind)-1) {
		ind[group_open] = '\0';
		if (s == NULL)
			pcb_fprintf(f, ind);
		else
			pcb_append_printf(s, ind);
		ind[group_open] = ' ';
		return;
	}

	if (s == NULL)
		pcb_fprintf(f, ind);
	else
		pcb_append_printf(s, ind);
}

static void svg_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

#define fix_rect_coords() \
	if (x1 > x2) {\
		pcb_coord_t t = x1; \
		x1 = x2; \
		x2 = t; \
	} \
	if (y1 > y2) { \
		pcb_coord_t t = y1; \
		y1 = y2; \
		y2 = t; \
	}

static void draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t w, pcb_coord_t h, pcb_coord_t stroke)
{
	const char *clip_color = svg_clip_color(gc);

	indent(&snormal);
	pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
		x1, y1, w, h, stroke, svg_color(gc), CAPS(gc->cap));
	if (clip_color != NULL) {
		indent(&sclip);
		pcb_append_printf(&sclip, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, w, h, stroke, clip_color, CAPS(gc->cap));
	}
}

static void svg_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	fix_rect_coords();
	draw_rect(gc, x1, y1, x2-x1, y2-y1, gc->width);
}

static void draw_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t w, pcb_coord_t h)
{
	const char *clip_color = svg_clip_color(gc);
	if ((photo_mode) && (clip_color == NULL)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sdark);
			pcb_append_printf(&sdark, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, w, h, photo_palette[photo_color].dark);
			indent(&sbright);
			pcb_append_printf(&sbright, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, w, h, photo_palette[photo_color].bright);
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, photo_palette[photo_color].normal);
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, svg_color(gc));
		if (clip_color != NULL)
			pcb_append_printf(&sclip, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1, y1, w, h, clip_color);
	}
}

static void svg_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
	draw_fill_rect(gc, x1, y1, x2-x1, y2-y1);
}

static void pcb_line_draw(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	const char *clip_color = svg_clip_color(gc);
	if ((photo_mode) && (clip_color == NULL)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sbright);
			pcb_append_printf(&sbright, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1-photo_offs, y1-photo_offs, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(&sdark);
			pcb_append_printf(&sdark, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1+photo_offs, y1+photo_offs, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, svg_color(gc), CAPS(gc->cap));
		if (clip_color != NULL) {
			pcb_append_printf(&sclip, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1, y1, x2, y2, gc->width, clip_color, CAPS(gc->cap));
		}
	}
}

static void svg_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	pcb_line_draw(gc, x1, y1, x2, y2);
}

static void pcb_arc_draw(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t r, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t stroke, int large, int sweep)
{
	const char *clip_color = svg_clip_color(gc);
	if ((photo_mode) && (clip_color == NULL)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(&sbright);
			pcb_append_printf(&sbright, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, r, r, large, sweep, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(&sdark);
			pcb_append_printf(&sdark, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, r, r, large, sweep, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(&snormal);
		pcb_append_printf(&snormal, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(&snormal);
		pcb_append_printf(&snormal, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, svg_color(gc), CAPS(gc->cap));
		if (clip_color != NULL)
			pcb_append_printf(&sclip, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1, y1, r, r, large, sweep, x2, y2, gc->width, clip_color, CAPS(gc->cap));
	}
}

static void svg_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	pcb_coord_t x1, y1, x2, y2, diff = 0, diff2, maxdiff;
	pcb_angle_t sa, ea;

 /* degenerate case: r=0 means a single dot */
	if ((width == 0) && (height == 0)) {
		pcb_line_draw(gc, cx, cy, cx, cy);
		return;
	}

	/* detect elliptical arcs: if diff between radii is above 0.1% */
	diff2 = width - height;
	if (diff2 < 0)
		diff2 = -diff2;
	maxdiff = width;
	if (height > maxdiff)
		maxdiff = height;
	if (diff2 > maxdiff / 1000) {
		if (!gc->warned_elliptical) {
			pcb_message(PCB_MSG_ERROR, "Can't draw elliptical arc on svg; object omitted; expect BROKEN TRACE\n");
			gc->warned_elliptical = 1;
		}
		return;
	}

	TRX(cx); TRY(cy);

	/* calculate start and end angles considering flip */
	start_angle = 180 - start_angle;
	delta_angle = -delta_angle;
	if (flip) {
		start_angle = -start_angle;
		delta_angle = -delta_angle;
	}

	/* workaround for near-360 deg rendering bugs */
	if ((delta_angle >= +360.0) || (delta_angle <= -360.0)) {
		svg_draw_arc(gc, cx, cy, width, height, 0, 180);
		svg_draw_arc(gc, cx, cy, width, height, 180, 180);
		return;
	}

	if (fabs(delta_angle) <= 0.001) { delta_angle = 0.001; diff=1; }

	sa = start_angle;
	ea = start_angle + delta_angle;

	/* calculate the endpoints */
	x2 = pcb_round((double)cx + ((double)width * cos(sa * M_PI / 180)));
	y2 = pcb_round((double)cy + ((double)width * sin(sa * M_PI / 180)));
	x1 = pcb_round((double)cx + ((double)width * cos(ea * M_PI / 180))+diff);
	y1 = pcb_round((double)cy + ((double)width * sin(ea * M_PI / 180))+diff);

	pcb_arc_draw(gc, x1, y1, width, x2, y2, gc->width, (fabs(delta_angle) > 180), (delta_angle < 0.0));
}

static void draw_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r, pcb_coord_t stroke)
{
	const char *clip_color = svg_clip_color(gc);
	if ((photo_mode) && (clip_color == NULL)) {
		if (!drawing_hole) {
			pcb_coord_t photo_offs = photo_palette[photo_color].offs;
			if ((!gc->drill) && (photo_offs != 0)) {
				indent(&sbright);
				pcb_append_printf(&sbright, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx-photo_offs, cy-photo_offs, r, stroke, photo_palette[photo_color].bright);

				indent(&sdark);
				pcb_append_printf(&sdark, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx+photo_offs, cy+photo_offs, r, stroke, photo_palette[photo_color].dark);
			}
			indent(&snormal);
			pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, photo_palette[photo_color].normal);
		}
		else {
			indent(&snormal);
			pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, "#000000");
		}
	}
	else{
		indent(&snormal);
		pcb_append_printf(&snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			cx, cy, r, stroke, svg_color(gc));
		if (clip_color != NULL)
			pcb_append_printf(&sclip, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, clip_color);
	}
}

static void svg_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	TRX(cx); TRY(cy);
	draw_fill_circle(gc, cx, cy, radius, 0);
}

static void draw_poly(gds_t *s, pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y, pcb_coord_t dx, pcb_coord_t dy, const char *clr)
{
	int i;
	float poly_bloat = 0.01;

	indent(s);
	gds_append_str(s, "<polygon points=\"");
	for (i = 0; i < n_coords; i++) {
		pcb_coord_t px = x[i], py = y[i];
		TRX(px); TRY(py);
		pcb_append_printf(s, "%mm,%mm ", px+dx, py+dy);
	}
	pcb_append_printf(s, "\" stroke-width=\"%.3f\" stroke=\"%s\" fill=\"%s\"/>\n", poly_bloat, clr, clr);
}

static void svg_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	const char *clip_color = svg_clip_color(gc);
	if ((photo_mode) && (clip_color == NULL)) {
		pcb_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			draw_poly(&sbright, gc, n_coords, x, y, dx-photo_offs, dy-photo_offs, photo_palette[photo_color].bright);
			draw_poly(&sdark, gc, n_coords, x, y, dx+photo_offs, dy+photo_offs, photo_palette[photo_color].dark);
		}
		draw_poly(&snormal, gc, n_coords, x, y, dx, dy, photo_palette[photo_color].normal);
	}
	else {
		draw_poly(&snormal, gc, n_coords, x, y, dx, dy, svg_color(gc));
		if (clip_color != NULL)
			draw_poly(&sclip, gc, n_coords, x, y, dx, dy, clip_color);
	}
}

static void svg_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	svg_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}


static void svg_calibrate(pcb_hid_t *hid, double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "svg_calibrate() not implemented");
	return;
}

static void svg_set_crosshair(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int a)
{
}

static int svg_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nsvg exporter command line arguments:\n\n");
	pcb_hid_usage(svg_attribute_list, sizeof(svg_attribute_list) / sizeof(svg_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x svg [svg options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_svg(int ver_needed) { return 0; }

void pplg_uninit_export_svg(void)
{
	pcb_hid_remove_attributes_by_cookie(svg_cookie);
}

int pplg_init_export_svg(void)
{
	PCB_API_CHK_VER;

	memset(&svg_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&svg_hid);

	svg_hid.struct_size = sizeof(pcb_hid_t);
	svg_hid.name = "svg";
	svg_hid.description = "Scalable Vector Graphics export";
	svg_hid.exporter = 1;

	svg_hid.get_export_options = svg_get_export_options;
	svg_hid.do_export = svg_do_export;
	svg_hid.parse_arguments = svg_parse_arguments;
	svg_hid.set_layer_group = svg_set_layer_group;
	svg_hid.make_gc = svg_make_gc;
	svg_hid.destroy_gc = svg_destroy_gc;
	svg_hid.set_drawing_mode = svg_set_drawing_mode;
	svg_hid.set_color = svg_set_color;
	svg_hid.set_line_cap = svg_set_line_cap;
	svg_hid.set_line_width = svg_set_line_width;
	svg_hid.set_draw_xor = svg_set_draw_xor;
	svg_hid.draw_line = svg_draw_line;
	svg_hid.draw_arc = svg_draw_arc;
	svg_hid.draw_rect = svg_draw_rect;
	svg_hid.fill_circle = svg_fill_circle;
	svg_hid.fill_polygon = svg_fill_polygon;
	svg_hid.fill_polygon_offs = svg_fill_polygon_offs;
	svg_hid.fill_rect = svg_fill_rect;
	svg_hid.calibrate = svg_calibrate;
	svg_hid.set_crosshair = svg_set_crosshair;

	svg_hid.usage = svg_usage;

	pcb_hid_register_hid(&svg_hid);

	return 0;
}
