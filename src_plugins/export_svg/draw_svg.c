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
#include <librnd/core/error.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/hid.h>

#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>

#include "draw_svg.h"

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

/* Photo mode colors and hacks */
static const char *rnd_svg_board_color = "#464646";
static const char *mask_color = "#00ff00";
static float mask_opacity_factor = 0.5;

rnd_svg_photo_color_t rnd_svg_photo_color;

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

#define TRX(x)
#define TRY(y) \
do { \
	if (pctx->flip) \
		y = pctx->hidlib->size_y - y; \
} while(0)

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

void rnd_svg_background(rnd_svg_t *pctx)
{
	if (pctx->photo_mode) {
		rnd_fprintf(pctx->outf, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			0, 0, pctx->hidlib->size_x, pctx->hidlib->size_y, rnd_svg_board_color);
	}
}

void rnd_svg_layer_group_begin(rnd_svg_t *pctx, long group, const char *name, int is_our_mask)
{
	int opa;

	while(pctx->group_open) {
		group_close(pctx);
		pctx->group_open--;
	}
	fprintf(pctx->outf, "<g id=\"layer_%ld_%s\"", group, name);

	opa = pctx->opacity;
	if (is_our_mask)
		opa *= mask_opacity_factor;
	if (opa != 100)
		fprintf(pctx->outf, " opacity=\"%.2f\"", ((float)opa) / 100.0);
	fprintf(pctx->outf, ">\n");
	pctx->group_open = 1;
}

static int rnd_svg_me;

rnd_hid_gc_t rnd_svg_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	rv->me_pointer = (rnd_hid_t *)&rnd_svg_me;
	rv->cap = rnd_cap_round;
	rv->width = 1;
	rv->color = NULL;
	return rv;
}

void rnd_svg_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

void rnd_svg_set_drawing_mode(rnd_svg_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
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

static const char *svg_clip_color(rnd_svg_t *pctx, rnd_hid_gc_t gc)
{
	if ((pctx->drawing_mode == RND_HID_COMP_POSITIVE) || (pctx->drawing_mode == RND_HID_COMP_POSITIVE_XOR))
		return "#FFFFFF";
	if (pctx->drawing_mode == RND_HID_COMP_NEGATIVE)
		return "#000000";
	return NULL;
}

void rnd_svg_set_color(rnd_svg_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color)
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

void rnd_svg_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

void rnd_svg_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width < RND_MM_TO_COORD(0.01) ? RND_MM_TO_COORD(0.01) : width;
}

void rnd_svg_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
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

static void draw_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t w, rnd_coord_t h, rnd_coord_t stroke)
{
	const char *clip_color = svg_clip_color(pctx, gc);

	indent(pctx, &pctx->snormal);
	rnd_append_printf(&pctx->snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
		x1, y1, w, h, stroke, svg_color(gc), CAPS(gc->cap));
	if (clip_color != NULL) {
		indent(pctx, &pctx->sclip);
		rnd_append_printf(&pctx->sclip, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, w, h, stroke, clip_color, CAPS(gc->cap));
	}
}

void rnd_svg_draw_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	fix_rect_coords();
	draw_rect(pctx, gc, x1, y1, x2-x1, y2-y1, gc->width);
}

static void draw_fill_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t w, rnd_coord_t h)
{
	const char *clip_color = svg_clip_color(pctx, gc);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[rnd_svg_photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, w, h, photo_palette[rnd_svg_photo_color].dark);
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, w, h, photo_palette[rnd_svg_photo_color].bright);
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<rect x=\"%mm\" y=\"%mm\" width=\"%mm\" height=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
			x1, y1, w, h, photo_palette[rnd_svg_photo_color].normal);
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

void rnd_svg_fill_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
	draw_fill_rect(pctx, gc, x1, y1, x2-x1, y2-y1);
}

static void pcb_line_draw(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	const char *clip_color = svg_clip_color(pctx, gc);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[rnd_svg_photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1-photo_offs, y1-photo_offs, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[rnd_svg_photo_color].bright, CAPS(gc->cap));
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
				x1+photo_offs, y1+photo_offs, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[rnd_svg_photo_color].dark, CAPS(gc->cap));
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<line x1=\"%mm\" y1=\"%mm\" x2=\"%mm\" y2=\"%mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\"/>\n",
			x1, y1, x2, y2, gc->width, photo_palette[rnd_svg_photo_color].normal, CAPS(gc->cap));
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

void rnd_svg_draw_line(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pctx->drawn_objs++;
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	pcb_line_draw(pctx, gc, x1, y1, x2, y2);
}

static void pcb_arc_draw(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t r, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t stroke, int large, int sweep)
{
	const char *clip_color = svg_clip_color(pctx, gc);
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs = photo_palette[rnd_svg_photo_color].offs;
		if (photo_offs != 0) {
			indent(pctx, &pctx->sbright);
			rnd_append_printf(&pctx->sbright, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1-photo_offs, y1-photo_offs, r, r, large, sweep, x2-photo_offs, y2-photo_offs, gc->width, photo_palette[rnd_svg_photo_color].bright, CAPS(gc->cap));
			indent(pctx, &pctx->sdark);
			rnd_append_printf(&pctx->sdark, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
				x1+photo_offs, y1+photo_offs, r, r, large, sweep, x2+photo_offs, y2+photo_offs, gc->width, photo_palette[rnd_svg_photo_color].dark, CAPS(gc->cap));
		}
		indent(pctx, &pctx->snormal);
		rnd_append_printf(&pctx->snormal, "<path d=\"M %.8mm %.8mm A %mm %mm 0 %d %d %mm %mm\" stroke-width=\"%mm\" stroke=\"%s\" stroke-linecap=\"%s\" fill=\"none\"/>\n",
			x1, y1, r, r, large, sweep, x2, y2, gc->width, photo_palette[rnd_svg_photo_color].normal, CAPS(gc->cap));
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

void rnd_svg_draw_arc(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_coord_t x1, y1, x2, y2, diff = 0, diff2, maxdiff;
	rnd_angle_t sa, ea;

	pctx->drawn_objs++;

 /* degenerate case: r=0 means a single dot */
	if ((width == 0) && (height == 0)) {
		pcb_line_draw(pctx, gc, cx, cy, cx, cy);
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
		rnd_svg_draw_arc(pctx, gc, cx, cy, width, height, 0, 180);
		rnd_svg_draw_arc(pctx, gc, cx, cy, width, height, 180, 180);
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

	pcb_arc_draw(pctx, gc, x1, y1, width, x2, y2, gc->width, (fabs(delta_angle) > 180), (delta_angle < 0.0));
}

static void draw_fill_circle(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_coord_t stroke)
{
	const char *clip_color = svg_clip_color(pctx, gc);

	pctx->drawn_objs++;

	if (pctx->photo_mode) {
		if (!pctx->drawing_hole) {
			rnd_coord_t photo_offs = photo_palette[rnd_svg_photo_color].offs;
			if ((!gc->drill) && (photo_offs != 0)) {
				indent(pctx, &pctx->sbright);
				rnd_append_printf(&pctx->sbright, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx-photo_offs, cy-photo_offs, r, stroke, photo_palette[rnd_svg_photo_color].bright);

				indent(pctx, &pctx->sdark);
				rnd_append_printf(&pctx->sdark, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
					cx+photo_offs, cy+photo_offs, r, stroke, photo_palette[rnd_svg_photo_color].dark);
			}
			indent(pctx, &pctx->snormal);
			rnd_append_printf(&pctx->snormal, "<circle cx=\"%mm\" cy=\"%mm\" r=\"%mm\" stroke-width=\"%mm\" fill=\"%s\" stroke=\"none\"/>\n",
				cx, cy, r, stroke, photo_palette[rnd_svg_photo_color].normal);
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

void rnd_svg_fill_circle(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	pctx->drawn_objs++;
	TRX(cx); TRY(cy);
	draw_fill_circle(pctx, gc, cx, cy, radius, 0);
}

static void draw_poly(rnd_svg_t *pctx, gds_t *s, rnd_hid_gc_t gc, int n_coords, rnd_coord_t * x, rnd_coord_t * y, rnd_coord_t dx, rnd_coord_t dy, const char *clr)
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

void rnd_svg_fill_polygon_offs(rnd_svg_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	const char *clip_color = svg_clip_color(pctx, gc);
	pctx->drawn_objs++;
	if (pctx->photo_mode) {
		rnd_coord_t photo_offs_x = photo_palette[rnd_svg_photo_color].offs, photo_offs_y = photo_palette[rnd_svg_photo_color].offs;
		if (photo_offs_x != 0) {
			if (pctx->flip)
				photo_offs_y = -photo_offs_y;
			draw_poly(pctx, &pctx->sbright, gc, n_coords, x, y, dx-photo_offs_x, dy-photo_offs_y, photo_palette[rnd_svg_photo_color].bright);
			draw_poly(pctx, &pctx->sdark, gc, n_coords, x, y, dx+photo_offs_x, dy+photo_offs_y, photo_palette[rnd_svg_photo_color].dark);
		}
		draw_poly(pctx, &pctx->snormal, gc, n_coords, x, y, dx, dy, photo_palette[rnd_svg_photo_color].normal);
	}
	else
		draw_poly(pctx, &pctx->snormal, gc, n_coords, x, y, dx, dy, svg_color(gc));

	if (clip_color != NULL)
		draw_poly(pctx, &pctx->sclip, gc, n_coords, x, y, dx, dy, clip_color);
}

void rnd_svg_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}
