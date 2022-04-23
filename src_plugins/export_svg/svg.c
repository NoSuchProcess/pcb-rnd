/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2006 Dan McMahill
 *  Copyright (C) 2016,2022 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hidlib_conf.h>
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


static rnd_hid_t svg_hid;

const char *svg_cookie = "svg HID";

static pcb_cam_t svg_cam;

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	int width;
	char *color;
	int drill;
	unsigned warned_elliptical:1;
} rnd_hid_gc_s;

static const char *CAPS(rnd_cap_style_t cap)
{
	switch (cap) {
		case rnd_cap_round:
			return "round";
		case rnd_cap_square:
			return "square";
		default:
			assert(!"unhandled cap");
			return "round";
	}
	return "";
}

typedef struct {
	/* public: config */
	rnd_hidlib_t *hidlib;
	FILE *outf;
	gds_t sbright, sdark, snormal, sclip; /* accumulators for various groups generated parallel */
	int opacity;
	int flip;
	int true_size;

	/* public: result */
	long drawn_objs;

	/* private: cache */
	int group_open, drawing_mask, drawing_hole, comp_cnt;
	rnd_composite_op_t drawing_mode;

	/* private: pcb-rnd leftover */
	int photo_mode, photo_noise;
} rnd_svg_t;

static rnd_svg_t pctx_, *pctx = &pctx_;

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
	rnd_coord_t offs;
} photo_palette[] = {
	/* MASK */   { "#00ff00", "#00ff00", "#00ff00", RND_MM_TO_COORD(0) },
	/* SILK */   { "#ffffff", "#eeeeee", "#aaaaaa", RND_MM_TO_COORD(0) },
	/* COPPER */ { "#bbbbbb", "#707090", "#555555", RND_MM_TO_COORD(0.05) },
	/* INNER */  { "#222222", "#111111", "#000000", RND_MM_TO_COORD(0) }
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

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_cam 7
};

#define NUM_OPTIONS (sizeof(svg_attribute_list)/sizeof(svg_attribute_list[0]))

#define TRX(x)
#define TRY(y) \
do { \
	if (pctx->flip) \
		y = pctx->hidlib->size_y - y; \
} while(0)

static rnd_hid_attr_val_t svg_values[NUM_OPTIONS];

static const rnd_export_opt_t *svg_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".svg";
	const char *val = svg_values[HA_svgfile].str;

	if ((PCB != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &svg_values[HA_svgfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return svg_attribute_list;
}

void rnd_svg_init(rnd_svg_t *pctx, rnd_hidlib_t *hidlib, FILE *f, int opacity, int flip, int true_size)
{
	memset(pctx, 0, sizeof(rnd_svg_t));
	pctx->hidlib = hidlib;
	pctx->outf = f;
	pctx->opacity = opacity;
	pctx->flip = flip;
	pctx->true_size = true_size;
}

void rnd_svg_uninit(rnd_svg_t *pctx)
{
	gds_uninit(&pctx->sbright);
	gds_uninit(&pctx->sdark);
	gds_uninit(&pctx->snormal);
	gds_uninit(&pctx->sclip);
}

void svg_hid_export_to_file(FILE * the_file, rnd_hid_attr_val_t * options, rnd_xform_t *xform)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	rnd_hid_expose_ctx_t ctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

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

	if (pctx->photo_mode) {
		rnd_fprintf(pctx->outf, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			0, 0, pctx->hidlib->size_x, pctx->hidlib->size_y, board_color);
	}

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

static void group_close(rnd_svg_t *pctx)
{
	if (pctx->group_open == 1) {
		if (gds_len(&pctx->sdark) > 0) {
			fprintf(pctx->outf, "<!--dark-->\n");
			fprintf(pctx->outf, "%s", pctx->sdark.array);
			gds_truncate(&pctx->sdark, 0);
		}

		if (gds_len(&pctx->sbright) > 0) {
			fprintf(pctx->outf, "<!--bright-->\n");
			fprintf(pctx->outf, "%s", pctx->sbright.array);
			gds_truncate(&pctx->sbright, 0);
		}

		if (gds_len(&pctx->snormal) > 0) {
			fprintf(pctx->outf, "<!--normal-->\n");
			fprintf(pctx->outf, "%s", pctx->snormal.array);
			gds_truncate(&pctx->snormal, 0);
		}

	}
	fprintf(pctx->outf, "</g>\n");
}

void rnd_svg_header(rnd_svg_t *pctx)
{
	rnd_coord_t w, h, x1, y1, x2, y2;

	fprintf(pctx->outf, "<?xml version=\"1.0\"?>\n");
	w = pctx->hidlib->size_x;
	h = pctx->hidlib->size_y;
	while((w < RND_MM_TO_COORD(1024)) && (h < RND_MM_TO_COORD(1024))) {
		w *= 2;
		h *= 2;
	}

	x2 = pctx->hidlib->size_x;
	y2 = pctx->hidlib->size_y;
	if (pctx->true_size) {
		rnd_fprintf(pctx->outf, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"%$$mm\" height=\"%$$mm\" viewBox=\"0 0 %mm %mm\">\n", x2, y2, x2, y2);
	}
	else {
		x1 = RND_MM_TO_COORD(2);
		y1 = RND_MM_TO_COORD(2);
		x2 += RND_MM_TO_COORD(5);
		y2 += RND_MM_TO_COORD(5);
		rnd_fprintf(pctx->outf, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"%mm\" height=\"%mm\" viewBox=\"-%mm -%mm %mm %mm\">\n", w, h, x1, y1, x2, y2);
	}
}

void rnd_svg_footer(rnd_svg_t *pctx)
{
	while(pctx->group_open) {
		group_close(pctx);
		pctx->group_open--;
	}

	/* blend some noise on top to make it a bit more artificial */
	if (pctx->photo_mode && pctx->photo_noise) {
		fprintf(pctx->outf, "<filter id=\"noise\">\n");
		fprintf(pctx->outf, "	<feTurbulence type=\"fractalNoise\" baseFrequency=\"30\" result=\"noisy\" />\n");
		fprintf(pctx->outf, "</filter>\n");
		fprintf(pctx->outf, "<g opacity=\"0.25\">\n");
		rnd_fprintf(pctx->outf, "	<rect filter=\"url(#noise)\" x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"none\" stroke=\"none\"/>\n",
			0, 0, pctx->hidlib->size_x, pctx->hidlib->size_y);
		fprintf(pctx->outf, "</g>\n");
	}

	fprintf(pctx->outf, "</svg>\n");
}

static void svg_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER];
	rnd_xform_t xform;
	FILE *f = NULL;

	pctx->comp_cnt = 0;

	if (!options) {
		svg_get_export_options(hid, 0);
		options = svg_values;
	}

	;
	pcb_cam_begin(PCB, &svg_cam, &xform, options[HA_cam].str, svg_attribute_list, NUM_OPTIONS, options);

	if (svg_cam.fn_template == NULL) {
		filename = options[HA_svgfile].str;
		if (!filename)
			filename = "pcb.svg";

		f = rnd_fopen_askovr(&PCB->hidlib, svg_cam.active ? svg_cam.fn : filename, "wb", NULL);
		if (f == NULL) {
			TODO("copy error handling from ps");
			perror(filename);
			return;
		}
	}
	else
		f = NULL;

	rnd_svg_init(pctx, &PCB->hidlib, f, options[HA_opacity].lng, options[HA_flip].lng, options[HA_true_size].lng);
	if (f != NULL)
		rnd_svg_header(pctx);

	if (!svg_cam.active)
		pcb_hid_save_and_show_layer_ons(save_ons);

	svg_hid_export_to_file(pctx->outf, options, &xform);


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

int rnd_svg_new_file(rnd_svg_t *pctx, FILE *f, const char *fn)
{
	int ern = errno;

	if (pctx->outf != NULL) {
		rnd_svg_footer(pctx);
		fclose(pctx->outf);
	}

	if (f == NULL) {
		TODO("copy error print from ps");
		perror(fn);
		return -1;
	}

	pctx->outf = f;
	return 0;
}

static int svg_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	int opa, is_our_mask = 0, is_our_silk = 0;

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

printf("GRP: '%s'\n", PCB->LayerGroups.grp[group].name);

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

	while(pctx->group_open) {
		group_close(pctx);
		pctx->group_open--;
	}

	{
		gds_t tmp_ln;
		const char *name;
		gds_init(&tmp_ln);
		name = pcb_layer_to_file_name(&tmp_ln, layer, flags, purpose, purpi, PCB_FNS_fixed);
		fprintf(pctx->outf, "<g id=\"layer_%ld_%s\"", group, name);
		gds_uninit(&tmp_ln);
	}

	opa = pctx->opacity;
	if (is_our_mask)
		opa *= mask_opacity_factor;
	if (opa != 100)
		fprintf(pctx->outf, " opacity=\"%.2f\"", ((float)opa) / 100.0);
	fprintf(pctx->outf, ">\n");
	pctx->group_open = 1;

	if (pctx->photo_mode) {
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

	pctx->drawing_hole = PCB_LAYER_IS_DRILL(flags, purpi);
	return 1;
}


static rnd_hid_gc_t svg_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	rv->me_pointer = &svg_hid;
	rv->cap = rnd_cap_round;
	rv->width = 1;
	rv->color = NULL;
	return rv;
}

static void svg_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void svg_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	pctx->drawing_mode = op;

	if (direct)
		return;

	switch(op) {
		case RND_HID_COMP_RESET:
			pctx->comp_cnt++;
			gds_init(&pctx->sclip);
			rnd_append_printf(&pctx->snormal, "<!-- Composite: reset -->\n");
			rnd_append_printf(&pctx->snormal, "<defs>\n");
			rnd_append_printf(&pctx->snormal, "<g id=\"comp_pixel_%d\">\n", pctx->comp_cnt);
			rnd_append_printf(&pctx->sclip, "<mask id=\"comp_clip_%d\" maskUnits=\"userSpaceOnUse\" x=\"0\" y=\"0\" width=\"%mm\" height=\"%mm\">\n", pctx->comp_cnt, pctx->hidlib->size_x, pctx->hidlib->size_y);
			break;

		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
		case RND_HID_COMP_NEGATIVE:
			break;

		case RND_HID_COMP_FLUSH:
			rnd_append_printf(&pctx->snormal, "</g>\n");
			rnd_append_printf(&pctx->sclip, "</mask>\n");
			gds_append_str(&pctx->snormal, pctx->sclip.array);
			rnd_append_printf(&pctx->snormal, "</defs>\n");
			rnd_append_printf(&pctx->snormal, "<use xlink:href=\"#comp_pixel_%d\" mask=\"url(#comp_clip_%d)\"/>\n", pctx->comp_cnt, pctx->comp_cnt);
			rnd_append_printf(&pctx->snormal, "<!-- Composite: finished -->\n");
			gds_uninit(&pctx->sclip);
			break;
	}
}

static const char *svg_color(rnd_hid_gc_t gc)
{
	return gc->color;
}

static const char *svg_clip_color(rnd_hid_gc_t gc)
{
	if ((pctx->drawing_mode == RND_HID_COMP_POSITIVE) || (pctx->drawing_mode == RND_HID_COMP_POSITIVE_XOR))
		return "#FFFFFF";
	if (pctx->drawing_mode == RND_HID_COMP_NEGATIVE)
		return "#000000";
	return NULL;
}

static void svg_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	const char *name;
	gc->drill = 0;

	if (color == NULL)
		name = "#ff0000";
	else
		name = color->str;

	if (rnd_color_is_drill(color)) {
		name = "#ffffff";
		gc->drill = 1;
	}
	if (pctx->drawing_mask)
		name = mask_color;
	if ((gc->color != NULL) && (strcmp(gc->color, name) == 0))
		return;
	free(gc->color);
	gc->color = rnd_strdup(name);
}

static void svg_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void svg_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width < RND_MM_TO_COORD(0.01) ? RND_MM_TO_COORD(0.01) : width;
}

static void indent(rnd_svg_t *pctx, gds_t *s)
{
	static char ind[] = "                                                                              ";
	if (pctx->group_open < sizeof(ind)-1) {
		ind[pctx->group_open] = '\0';
		if (s == NULL)
			rnd_fprintf(pctx->outf, ind);
		else
			rnd_append_printf(s, ind);
		ind[pctx->group_open] = ' ';
		return;
	}

	if (s == NULL)
		rnd_fprintf(pctx->outf, ind);
	else
		rnd_append_printf(s, ind);
}

static void svg_set_draw_xor(rnd_hid_gc_t gc, int xor_)
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

static void draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t w, rnd_coord_t h, rnd_coord_t stroke)
{
	const char *clip_color = svg_clip_color(gc);

	indent(pctx, &pctx->snormal);
	rnd_append_printf(&pctx->snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
		x1, y1, w, h, stroke, svg_color(gc), CAPS(gc->cap));
	if (clip_color != NULL) {
		indent(pctx, &pctx->sclip);
		rnd_append_printf(&pctx->sclip, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, w, h, stroke, clip_color, CAPS(gc->cap));
	}
}

static void svg_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	fix_rect_coords();
	draw_rect(gc, x1, y1, x2-x1, y2-y1, gc->width);
}

static void draw_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t w, rnd_coord_t h)
{
	const char *clip_color = svg_clip_color(gc);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, w, h, photo_palette[photo_color].dark);
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, w, h, photo_palette[photo_color].bright);
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, photo_palette[photo_color].normal);
	}
	else {
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, svg_color(gc));
	}
	if (clip_color != NULL)
		rnd_append_printf(&pctx->sclip, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, clip_color);
}

static void svg_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
	draw_fill_rect(gc, x1, y1, x2-x1, y2-y1);
}

static void pcb_line_draw(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	const char *clip_color = svg_clip_color(gc);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1-photo_offs, y1-photo_offs, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1+photo_offs, y1+photo_offs, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, svg_color(gc), CAPS(gc->cap));
	}
	if (clip_color != NULL) {
		rnd_append_printf(&pctx->sclip, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, clip_color, CAPS(gc->cap));
	}
}

static void svg_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	pcb_line_draw(gc, x1, y1, x2, y2);
}

static void pcb_arc_draw(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t r, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t stroke, int large, int sweep)
{
	const char *clip_color = svg_clip_color(gc);
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, r, r, large, sweep, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[photo_color].bright, CAPS(gc->cap));
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, r, r, large, sweep, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[photo_color].dark, CAPS(gc->cap));
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, photo_palette[photo_color].normal, CAPS(gc->cap));
	}
	else {
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, svg_color(gc), CAPS(gc->cap));
	}
	if (clip_color != NULL)
		rnd_append_printf(&pctx->sclip, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, clip_color, CAPS(gc->cap));
}

static void svg_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_coord_t x1, y1, x2, y2, diff = 0, diff2, maxdiff;
	rnd_angle_t sa, ea;

	pctx->drawn_objs++;

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
			rnd_message(RND_MSG_ERROR, "Can't draw elliptical arc on svg; object omitted; expect BROKEN TRACE\n");
			gc->warned_elliptical = 1;
		}
		return;
	}

	TRX(cx); TRY(cy);

	/* calculate start and end angles considering flip */
	start_angle = 180 - start_angle;
	delta_angle = -delta_angle;
	if (pctx->flip) {
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
	x2 = rnd_round((double)cx + ((double)width * cos(sa * M_PI / 180)));
	y2 = rnd_round((double)cy + ((double)width * sin(sa * M_PI / 180)));
	x1 = rnd_round((double)cx + ((double)width * cos(ea * M_PI / 180))+diff);
	y1 = rnd_round((double)cy + ((double)width * sin(ea * M_PI / 180))+diff);

	pcb_arc_draw(gc, x1, y1, width, x2, y2, gc->width, (fabs(delta_angle) > 180), (delta_angle < 0.0));
}

static void draw_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_coord_t stroke)
{
	const char *clip_color = svg_clip_color(gc);

	pctx->drawn_objs++;

	if (pctx->photo_mode) {
		if (!pctx->drawing_hole) {
			rnd_coord_t photo_offs = photo_palette[photo_color].offs;
			if ((!gc->drill) && (photo_offs != 0)) {
				indent(pctx, &pctx->sbright);
				rnd_append_printf(&pctx->sbright, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx-photo_offs, cy-photo_offs, r, stroke, photo_palette[photo_color].bright);

				indent(pctx, &pctx->sdark);
				rnd_append_printf(&pctx->sdark, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx+photo_offs, cy+photo_offs, r, stroke, photo_palette[photo_color].dark);
			}
			indent(pctx, &pctx->snormal);
			rnd_append_printf(&pctx->snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, photo_palette[photo_color].normal);
		}
		else {
			indent(pctx, &pctx->snormal);
			rnd_append_printf(&pctx->snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, "#000000");
		}
	}
	else{
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			cx, cy, r, stroke, svg_color(gc));
	}
	if (clip_color != NULL)
		rnd_append_printf(&pctx->sclip, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			cx, cy, r, stroke, clip_color);
}

static void svg_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	pctx->drawn_objs++;
	TRX(cx); TRY(cy);
	draw_fill_circle(gc, cx, cy, radius, 0);
}

static void draw_poly(gds_t *s, rnd_hid_gc_t gc, int n_coords, rnd_coord_t * x, rnd_coord_t * y, rnd_coord_t dx, rnd_coord_t dy, const char *clr)
{
	int i;
	float poly_bloat = 0.01;

	indent(pctx, s);
	gds_append_str(s, "<polygon points=\"");
	for (i = 0; i < n_coords; i++) {
		rnd_coord_t px = x[i] + dx, py = y[i] + dy;
		TRX(px); TRY(py);
		rnd_append_printf(s, "%mm,%mm ", px, py);
	}
	rnd_append_printf(s, "\" stroke-width=\"%.3f\" stroke=\"%s\" fill=\"%s\"/>\n", poly_bloat, clr, clr);
}

static void svg_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	const char *clip_color = svg_clip_color(gc);
	pctx->drawn_objs++;
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs_x = photo_palette[photo_color].offs, photo_offs_y = photo_palette[photo_color].offs;
		if (photo_offs_x != 0) {
			if (pctx->flip)
				photo_offs_y = -photo_offs_y;
			draw_poly(&pctx->sbright, gc, n_coords, x, y, dx-photo_offs_x, dy-photo_offs_y, photo_palette[photo_color].bright);
			draw_poly(&pctx->sdark, gc, n_coords, x, y, dx+photo_offs_x, dy+photo_offs_y, photo_palette[photo_color].dark);
		}
		draw_poly(&pctx->snormal, gc, n_coords, x, y, dx, dy, photo_palette[photo_color].normal);
	}
	else
		draw_poly(&pctx->snormal, gc, n_coords, x, y, dx, dy, svg_color(gc));

	if (clip_color != NULL)
		draw_poly(&pctx->sclip, gc, n_coords, x, y, dx, dy, clip_color);
}

static void svg_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	svg_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}


static void svg_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
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
	svg_hid.set_crosshair = svg_set_crosshair;
	svg_hid.argument_array = svg_values;

	svg_hid.usage = svg_usage;

	rnd_hid_register_hid(&svg_hid);
	rnd_hid_load_defaults(&svg_hid, svg_attribute_list, NUM_OPTIONS);

	return 0;
}
