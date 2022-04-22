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

/* Heavily based on the geda/pcb ps HID written by DJ Delorie */

#include "config.h"

#include <genht/htpp.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <librnd/core/hidlib.h>
#include <librnd/core/color.h>
#include <librnd/core/color_cache.h>
#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include <librnd/core/compat_misc.h>

#include <gd.h>

#define FROM_DRAW_PNG_C
#include "draw_png.h"

void rnd_png_init(rnd_png_t *pctx, rnd_hidlib_t *hidlib)
{
	memset(pctx, 0, sizeof(rnd_png_t));
	pctx->hidlib = hidlib;
	pctx->scale = 1;
	pctx->lastbrush = (gdImagePtr)((void *)-1);
	pctx->linewidth = -1;
}


#define SCALE(w)   ((int)rnd_round((w)/pctx->scale))
#define SCALE_X(x) ((int)rnd_round(((x) - pctx->x_shift)/pctx->scale))
#define SCALE_Y(y) ((int)rnd_round(((pctx->show_solder_side ? (pctx->hidlib->size_y-(y)) : (y)) - pctx->y_shift)/pctx->scale))
#define SWAP_IF_SOLDER(a,b) do { int c; if (pctx->show_solder_side) { c=a; a=b; b=c; }} while (0)

/* Used to detect non-trivial outlines */
#define NOT_EDGE_X(x) ((x) != 0 && (x) != pctx->hidlib->size_x)
#define NOT_EDGE_Y(y) ((y) != 0 && (y) != pctx->hidlib->size_y)
#define NOT_EDGE(x,y) (NOT_EDGE_X(x) || NOT_EDGE_Y(y))

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	void *me_pointer;
	rnd_cap_style_t cap;
	int width, r, g, b;
	rnd_png_color_struct_t *color;
	gdImagePtr brush;
	int is_erase;
} hid_gc_t;


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

void rnd_png_start(rnd_png_t *pctx)
{
	pctx->linewidth = -1;
	pctx->lastbrush = (gdImagePtr)((void *)-1);
	pctx->last_color_r = pctx->last_color_g = pctx->last_color_b = pctx->last_cap = -1;

	gdImageFilledRectangle(pctx->im, 0, 0, gdImageSX(pctx->im), gdImageSY(pctx->im), pctx->white->c);
}

void rnd_png_finish(rnd_png_t *pctx, FILE *f, const char *fmt)
{
	rnd_bool format_error = rnd_false;

	/* actually write out the image */

	if (fmt == NULL)
		format_error = rnd_true;
	else if (strcmp(fmt, RND_PNG_FMT_gif) == 0)
#ifdef PCB_HAVE_GDIMAGEGIF
		gdImageGif(pctx->im, f);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, RND_PNG_FMT_jpg) == 0)
#ifdef PCB_HAVE_GDIMAGEJPEG
		gdImageJpeg(pctx->im, f, -1);
#else
		format_error = rnd_true;
#endif
	else if (strcmp(fmt, RND_PNG_FMT_png) == 0)
#ifdef PCB_HAVE_GDIMAGEPNG
		gdImagePng(pctx->im, f);
#else
		format_error = rnd_true;
#endif
	else
		format_error = rnd_true;

	if (format_error)
		rnd_message(RND_MSG_ERROR, "rnd_png_finish(): Invalid graphic file format. This is a bug. Please report it.\n");
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
		pctx->h = pctx->hidlib->size_y;
		pctx->w = pctx->hidlib->size_x;
	}

	/* figure out the scale factor to fit in the specified PNG file size */
	if (dpi_in != 0) {
		pctx->dpi = dpi_in;
		if (pctx->dpi < 0) {
			rnd_message(RND_MSG_ERROR, "rnd_png_set_size(): dpi may not be < 0\n");
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
		rnd_message(RND_MSG_ERROR, "rnd_png_set_size(): xmax and ymax may not be < 0\n");
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
		pctx->w = rnd_round(pctx->w / pctx->scale);
		pctx->h = rnd_round(pctx->h / pctx->scale);
	}
	else if (pctx->xmax == 0 && pctx->ymax == 0) {
		rnd_message(RND_MSG_ERROR, "rnd_png_create(): you may not set both xmax, ymax, and xy-max to zero\n");
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
	pctx->white = (rnd_png_color_struct_t *)malloc(sizeof(rnd_png_color_struct_t));
	pctx->white->r = pctx->white->g = pctx->white->b = 255;
	if (use_alpha)
		pctx->white->a = 127;
	else
		pctx->white->a = 0;
	pctx->white->c = gdImageColorAllocateAlpha(pctx->im, pctx->white->r, pctx->white->g, pctx->white->b, pctx->white->a);
	if (pctx->white->c == RND_PNG_BADC) {
		rnd_message(RND_MSG_ERROR, "rnd_png_create(): gdImageColorAllocateAlpha() returned NULL. Aborting export.\n");
		return -1;
	}


	pctx->black = (rnd_png_color_struct_t *)malloc(sizeof(rnd_png_color_struct_t));
	pctx->black->r = pctx->black->g = pctx->black->b = pctx->black->a = 0;
	pctx->black->c = gdImageColorAllocate(pctx->im, pctx->black->r, pctx->black->g, pctx->black->b);
	if (pctx->black->c == RND_PNG_BADC) {
		rnd_message(RND_MSG_ERROR, "rnd_png_create(): gdImageColorAllocateAlpha() returned NULL. Aborting export.\n");
		return -1;
	}

	return 0;
}

static int rnd_png_me; /* placeholder for recongnizing a gc */
rnd_hid_gc_t rnd_png_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t)calloc(sizeof(hid_gc_t), 1);
	rv->me_pointer = &rnd_png_me;
	rv->cap = rnd_cap_round;
	rv->width = 1;
	rv->color = (rnd_png_color_struct_t *) malloc(sizeof(rnd_png_color_struct_t));
	rv->color->r = rv->color->g = rv->color->b = rv->color->a = 0;
	rv->color->c = 0;
	rv->is_erase = 0;
	return rv;
}

void rnd_png_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

void rnd_png_set_drawing_mode(rnd_png_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	static gdImagePtr dst_im;
	if ((direct) || (pctx->is_photo_drill)) /* photo drill is a special layer, no copositing on that */
		return;
	switch(op) {
		case RND_HID_COMP_RESET:

			/* the main pixel buffer; drawn with color */
			if (pctx->comp_im == NULL) {
				pctx->comp_im = gdImageCreate(gdImageSX(pctx->im), gdImageSY(pctx->im));
				if (!pctx->comp_im) {
					rnd_message(RND_MSG_ERROR, "rnd_png_set_drawing_mode(): gdImageCreate(%d, %d) returned NULL on pctx->comp_im. Corrupt export!\n", gdImageSY(pctx->im), gdImageSY(pctx->im));
					return;
				}
			}

			/* erase mask: for composite layers, tells whether the color pixel
			   should be combined back to the master image; white means combine back,
			   anything else means cut-out/forget/ignore that pixel */
			if (pctx->erase_im == NULL) {
				pctx->erase_im = gdImageCreate(gdImageSX(pctx->im), gdImageSY(pctx->im));
				if (!pctx->erase_im) {
					rnd_message(RND_MSG_ERROR, "rnd_png_set_drawing_mode(): gdImageCreate(%d, %d) returned NULL on pctx->erase_im. Corrupt export!\n", gdImageSY(pctx->im), gdImageSY(pctx->im));
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

void rnd_png_set_color(rnd_png_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color)
{
	rnd_png_color_struct_t *cc;

	if (pctx->im == NULL)
		return;

	if (color == NULL)
		color = rnd_color_red;

	if (rnd_color_is_drill(color) || pctx->is_photo_mech) {
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
		rnd_clrcache_init(&pctx->color_cache, sizeof(rnd_png_color_struct_t), NULL);
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
		if (gc->color->c == RND_PNG_BADC) {
			rnd_message(RND_MSG_ERROR, "rnd_png_set_color(): gdImageColorAllocate() returned NULL. Aborting export.\n");
			return;
		}
	}
	else {
		rnd_message(RND_MSG_ERROR, "rnd_png_set_color(): WE SHOULD NOT BE HERE!!!\n");
		gc->color = pctx->black;
	}
}

void rnd_png_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

void rnd_png_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}

void rnd_png_set_draw_xor(rnd_hid_gc_t gc, int xor_)
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

static void use_gc(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc)
{
	int need_brush = 0;
	rnd_hid_gc_t agc;
	gdImagePtr brp;

	pctx->png_drawn_objs++;

	agc = gc;
	if (pctx->unerase_override) {
		agc->r = -1;
		agc->g = -1;
		agc->b = -1;
	}
	else {
		agc->r = gc->color->r;
		agc->g = gc->color->g;
		agc->b = gc->color->b;
	}

	if (agc->me_pointer != &rnd_png_me) {
		rnd_message(RND_MSG_ERROR, "rnd_png use_gc(): GC from another HID passed to draw_png\n");
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
				rnd_message(RND_MSG_ERROR, "rnd_png use_gc(): gdImageCreate(%d, %d) returned NULL. Aborting export.\n", r, r);
				return;
			}

			bg = gdImageColorAllocate(agc->brush, 255, 255, 255);
			if (bg == RND_PNG_BADC) {
				rnd_message(RND_MSG_ERROR, "rnd png use_gc(): gdImageColorAllocate() returned NULL. Aborting export.\n");
				return;
			}
			if (pctx->unerase_override)
				fg = gdImageColorAllocateAlpha(agc->brush, 255, 255, 255, 0);
			else
				fg = gdImageColorAllocateAlpha(agc->brush, agc->r, agc->g, agc->b, 0);
			if (fg == RND_PNG_BADC) {
				rnd_message(RND_MSG_ERROR, "rnd_png use_gc(): gdImageColorAllocate() returned NULL. Aborting export.\n");
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


static void png_fill_rect_(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	use_gc(pctx, im, gc);
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

	gdImageFilledRectangle(im, SCALE_X(x1 - pctx->bloat), SCALE_Y(y1), SCALE_X(x2 + pctx->bloat) - 1, SCALE_Y(y2) - 1, pctx->unerase_override ? pctx->white->c : gc->color->c);
	pctx->have_outline |= pctx->doing_outline;
}

void rnd_png_fill_rect(rnd_png_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_fill_rect_(pctx, pctx->im, gc, x1, y1, x2, y2);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		pctx->unerase_override = 1;
		png_fill_rect_(pctx, pctx->erase_im, gc, x1, y1, x2, y2);
		pctx->unerase_override = 0;
	}
}

static void png_draw_line_(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	int x1o = 0, y1o = 0, x2o = 0, y2o = 0;
	if (x1 == x2 && y1 == y2 && !pctx->photo_mode) {
		rnd_coord_t w = gc->width / 2;
		if (gc->cap != rnd_cap_square)
			rnd_png_fill_circle(pctx, gc, x1, y1, w);
		else
			rnd_png_fill_rect(pctx, gc, x1 - w, y1 - w, x1 + w, y1 + w);
		return;
	}
	use_gc(pctx, im, gc);

	if (NOT_EDGE(x1, y1) || NOT_EDGE(x2, y2))
		pctx->have_outline |= pctx->doing_outline;
	
		/* libgd clipping bug - lines drawn along the bottom or right edges
		   need to be brought in by a pixel to make sure they are not clipped
		   by libgd - even tho they should be visible because of thickness, they
		   would not be because the center line is off the image */
	if (x1 == pctx->hidlib->size_x && x2 == pctx->hidlib->size_x) {
		x1o = -1;
		x2o = -1;
	}
	if (y1 == pctx->hidlib->size_y && y2 == pctx->hidlib->size_y) {
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

		if (pctx->unerase_override)
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

void rnd_png_draw_line(rnd_png_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	png_draw_line_(pctx, pctx->im, gc, x1, y1, x2, y2);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		pctx->unerase_override = 1;
		png_draw_line_(pctx, pctx->erase_im, gc, x1, y1, x2, y2);
		pctx->unerase_override = 0;
	}
}

static void png_draw_arc_(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	rnd_angle_t sa, ea;

	use_gc(pctx, im, gc);
	gdImageSetThickness(im, 0);
	pctx->linewidth = 0;

	/* zero angle arcs need special handling as gd will output either
	   nothing at all or a full circle when passed delta angle of 0 or 360. */
	if (delta_angle == 0) {
		rnd_coord_t x = (width * cos(start_angle * M_PI / 180));
		rnd_coord_t y = (width * sin(start_angle * M_PI / 180));
		x = cx - x;
		y = cy + y;
		rnd_png_fill_circle(pctx, gc, x, y, gc->width / 2);
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

	pctx->have_outline |= pctx->doing_outline;

#if 0
	printf("draw_arc %d,%d %dx%d %d..%d %d..%d\n", cx, cy, width, height, start_angle, delta_angle, sa, ea);
	printf("gdImageArc (%p, %d, %d, %d, %d, %d, %d, %d)\n",
				 (void *)im, SCALE_X(cx), SCALE_Y(cy), SCALE(width), SCALE(height), sa, ea, gc->color->c);
#endif
	gdImageArc(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * width), SCALE(2 * height), sa, ea, gdBrushed);
}

void rnd_png_draw_arc(rnd_png_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	png_draw_arc_(pctx, pctx->im, gc, cx, cy, width, height, start_angle, delta_angle);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		pctx->unerase_override = 1;
		png_draw_arc_(pctx, pctx->erase_im, gc, cx, cy, width, height, start_angle, delta_angle);
		pctx->unerase_override = 0;
	}
}


static void png_fill_circle_(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	rnd_coord_t my_bloat;

	use_gc(pctx, im, gc);

	if (gc->is_erase)
		my_bloat = -2 * pctx->bloat;
	else
		my_bloat = 2 * pctx->bloat;

	pctx->have_outline |= pctx->doing_outline;

	gdImageSetThickness(im, 0);
	pctx->linewidth = 0;
	gdImageFilledEllipse(im, SCALE_X(cx), SCALE_Y(cy), SCALE(2 * radius + my_bloat), SCALE(2 * radius + my_bloat), pctx->unerase_override ? pctx->white->c : gc->color->c);
}

void rnd_png_fill_circle(rnd_png_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	png_fill_circle_(pctx, pctx->im, gc, cx, cy, radius);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		pctx->unerase_override = 1;
		png_fill_circle_(pctx, pctx->erase_im, gc, cx, cy, radius);
		pctx->unerase_override = 0;
	}
}


static void png_fill_polygon_offs_(rnd_png_t *pctx, gdImagePtr im, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	gdPoint *points;

	points = (gdPoint *) malloc(n_coords * sizeof(gdPoint));
	if (points == NULL) {
		rnd_message(RND_MSG_ERROR, "png_fill_polygon(): malloc failed\n");
		abort();
	}

	use_gc(pctx, im, gc);
	for (i = 0; i < n_coords; i++) {
		if (NOT_EDGE(x[i], y[i]))
			pctx->have_outline |= pctx->doing_outline;
		points[i].x = SCALE_X(x[i]+dx);
		points[i].y = SCALE_Y(y[i]+dy);
	}
	gdImageSetThickness(im, 0);
	pctx->linewidth = 0;
	gdImageFilledPolygon(im, points, n_coords, pctx->unerase_override ? pctx->white->c : gc->color->c);
	free(points);
}

void rnd_png_fill_polygon_offs(rnd_png_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	png_fill_polygon_offs_(pctx, pctx->im, gc, n_coords, x, y, dx, dy);
	if ((pctx->im != pctx->erase_im) && (pctx->erase_im != NULL)) {
		pctx->unerase_override = 1;
		png_fill_polygon_offs_(pctx, pctx->erase_im, gc, n_coords, x, y, dx, dy);
		pctx->unerase_override = 0;
	}
}

