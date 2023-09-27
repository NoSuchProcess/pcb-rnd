/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf glyph import
 *  pcb-rnd Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "conf_core.h"

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_menu.h>

#include "ttf_load.h"
#include "str_approx.h"

#include <librnd/poly/polygon1_gen.h>
#include <librnd/poly/self_isc.h>

static const char *ttf_cookie = "ttf importer";

#include "menu_internal.c"

#ifdef PCB_WANT_FONT2
#	define MAX_SIMPLE_POLY_POINTS RND_FONT2_MAX_SIMPLE_POLY_POINTS
#else
#	define MAX_SIMPLE_POLY_POINTS 256
#endif

static void str_init(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke init\n");
}

static void str_start(pcb_ttf_stroke_t *s, int chr)
{
	rnd_trace("stroke start\n");
}

static void str_finish(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke finish\n");
}

static void str_uninit(pcb_ttf_stroke_t *s)
{
	rnd_trace("stroke uninit\n");
}

#define TRX(x) RND_MM_TO_COORD((x) * str->scale_x + str->dx)
#define TRY(y) RND_MM_TO_COORD((str->ttf->face->height - (y) - str->ttf->face->ascender - str->ttf->face->descender) * str->scale_y + str->dy)

#define TRX_(x) RND_MM_TO_COORD((x) * stroke->scale_x)
#define TRY_(y) RND_MM_TO_COORD((y) * stroke->scale_y)

static void poly_create(pcb_ttf_stroke_t *str, rnd_pline_t *pl, int is_neg)
{
	rnd_polyarea_t *pa;

	if (pl->Count < 3)
		return;

	pa = rnd_polyarea_create();
	rnd_polyarea_contour_include(pa, pl);
	vtp0_append((is_neg ? &str->poly_neg : &str->poly_pos), pa);
rnd_trace("poly append: %d [%f] on %s\n", pl->Count, pl->area/1000000000.0, is_neg ? "neg" : "pos");
}

static void poly_flush(pcb_ttf_stroke_t *str)
{
	int is_neg = 0;

	if (!str->want_poly || (str->contour == NULL))
		return;

	rnd_poly_contour_pre(str->contour, rnd_true);
	if (str->contour->Flags.orient != RND_PLF_DIR) {
		rnd_poly_contour_inv(str->contour);
		is_neg = 1;
	}

	if (rnd_pline_is_selfint(str->contour)) {
		int n;
		vtp0_t pls;
		vtp0_init(&pls);
		rnd_pline_split_selfint(str->contour, &pls);
		for(n = 0; n < pls.used; n++) {
			rnd_pline_t *pln = (rnd_pline_t *)pls.array[n];
			poly_create(str, pln, is_neg);
		}
		vtp0_uninit(&pls);
		rnd_poly_contour_del(&str->contour);
	}
	else
		poly_create(str, str->contour, is_neg);

	str->contour = NULL;
}

static void ttf_poly_emit_small(rnd_pline_t *pl, void *ctx)
{
	long n;
	rnd_vnode_t *v;
	pcb_ttf_stroke_t *str = ctx;
	rnd_glyph_poly_t *p = rnd_font_new_poly_in_glyph(str->glyph, pl->Count);
	rnd_coord_t *px = &p->pts.array[0], *py = &p->pts.array[pl->Count];


rnd_trace("  emit small: %d\n", pl->Count);

	for(n = 0, v = pl->head; n < pl->Count; n++, v = v->next, px++, py++) {
		*px = v->point[0];
		*py = v->point[1];
	}
}

static void ttf_poly_emit_pa(rnd_polyarea_t *pa, void *ctx);

/* Emit a polyline converting it into a simplepoly; simplepoly has a limitation
   on max number of points; when pl is larger than that, slice it in halves
   recursively until the pieces are small enough */
static void ttf_poly_emit(rnd_pline_t *pl, void *ctx)
{
	if (pl->Count >= MAX_SIMPLE_POLY_POINTS) {
		rnd_polyarea_t *pa_in = rnd_polyarea_create(), *pa1, *pa2, *half;
		rnd_coord_t bbsx, bbsy;

rnd_trace(" emit: count %ld too large, need to split\n", pl->Count);

		/* pa_in is our input polyarea that has pl */
		rnd_poly_contour_copy(&pa_in->contours, pl);

		/* draw a rectangle that spans half the bounding box, cutting perpendicular
		   to the longer edge */
		bbsx = pl->xmax - pl->xmin;
		bbsy = pl->ymax - pl->ymin;
		if (bbsy >= bbsx) {
			rnd_coord_t margin = bbsy/16;
			half = rnd_poly_from_rect(pl->xmin-margin, pl->xmax+margin, pl->ymin-margin, (pl->ymin + pl->ymax)/2);
		}
		else {
			rnd_coord_t margin = bbsx/16;
			half = rnd_poly_from_rect(pl->xmin-margin, (pl->xmin + pl->xmax)/2, pl->ymin-margin, pl->ymax+margin);
		}

		/* calculate each half */
		if (rnd_polyarea_boolean(pa_in, half, &pa2, RND_PBO_SUB) == 0) {
rnd_trace(" emit sub\n");
			ttf_poly_emit_pa(pa2, ctx);
			rnd_polyarea_free(&pa2);
		}
		else
			rnd_message(RND_MSG_ERROR, "ttf_poly_emit(): failed to cut large poly in half (sub)\n");

		if (rnd_polyarea_boolean(pa_in, half, &pa1, RND_PBO_ISECT) == 0) {
rnd_trace(" emit isc\n");
			ttf_poly_emit_pa(pa1, ctx);
			rnd_polyarea_free(&pa1);
		}
		else
			rnd_message(RND_MSG_ERROR, "ttf_poly_emit(): failed to cut large poly in half (isect)\n");


		rnd_polyarea_free(&pa_in);
		rnd_polyarea_free(&half);
	}
	else
		ttf_poly_emit_small(pl, ctx);
}

/* Emit all plines of a polyarea; recurse back to the large-poly-slicer because
   a half polyline can still be too large for a simplepoly */
static void ttf_poly_emit_pa(rnd_polyarea_t *pa_in, void *ctx)
{
	rnd_polyarea_t *pa = pa_in;
	rnd_pline_t *pl;

	do {
		for(pl = pa->contours; pl != NULL; pl = pl->next)
			ttf_poly_emit(pl, ctx);
		pa = pa->f;
	} while(pa != pa_in);
}


static void poly_apply(pcb_ttf_stroke_t *str)
{
	int p, n;
	rnd_trace("poly apply:\n");
	for(p = 0; p < str->poly_pos.used; p++) {
		rnd_polyarea_t *pap = str->poly_pos.array[p];
		for(n = 0; n < str->poly_neg.used; n++) {
			rnd_polyarea_t *res, *pan = str->poly_neg.array[n];
			if (pan == NULL) continue;
			if (rnd_poly_contour_in_contour(pap->contours, pan->contours)) {
				str->poly_neg.array[n] = NULL;
				rnd_polyarea_boolean_free(pap, pan, &res, RND_PBO_SUB);
				if (res != NULL) {
					str->poly_pos.array[p] = pap = res;
					str->poly_neg.array[n] = NULL; /* already freed, do not reuse */
				}
			}
		}
	}

	/* dice and free positive poly areas */
	for(p = 0; p < str->poly_pos.used; p++)
		rnd_polyarea_no_holes_dicer(str->poly_pos.array[p], -RND_MAX_COORD, -RND_MAX_COORD, +RND_MAX_COORD, +RND_MAX_COORD, ttf_poly_emit, str);

	/* free remaining (unused) negative areas */
	for(n = 0; n < str->poly_neg.used; n++) {
		rnd_polyarea_t *pan = str->poly_neg.array[n];
		if (pan != NULL)
			rnd_polyarea_free(&pan);
	}

	vtp0_uninit(&str->poly_pos);
	vtp0_uninit(&str->poly_neg);
	rnd_trace("(end)\n");
}

static int str_move_to(const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *str = s_;
	rnd_trace(" move %f;%f %ld;%ld\n", str->x, str->y, to->x, to->y);
	poly_flush(str);
	str->x = to->x;
	str->y = to->y;
	return 0;
}

static int str_line_to(const FT_Vector *to, void *s_)
{
	pcb_ttf_stroke_t *str = s_;
	rnd_trace(" line %f;%f %ld;%ld\n", str->x, str->y, to->x, to->y);
	if (str->want_poly) {
		rnd_vector_t v;


		if (str->contour == NULL) {
			v[0] = TRX(str->x); v[1] = TRY(str->y);
			str->contour = rnd_poly_contour_new(v);
		}
		v[0] = TRX(to->x); v[1] = TRY(to->y);
		rnd_poly_vertex_include(str->contour->head->prev, rnd_poly_node_create(v));
	}
	else {
		rnd_font_new_line_in_glyph(str->glyph,
			TRX(str->x), TRY(str->y),
			TRX(to->x),  TRY(to->y),
			1);
	}
	str->x = to->x;
	str->y = to->y;
	return 0;
}

static int conv_char_desc(const char *s, const char **end)
{
	/* explicit '' */
	if (s[0] == '\'') {
		if (s[2] == '\'') {
			*end = s+3;
			return s[1];
		}
		*end = s+2;
		return -1;
	}

	/* unicode &# code point, hex or dec */
	if ((s[0] == '&') && (s[1] == '#')) {
		if (s[2] == 'x')
			return strtol(s+3, (char **)end, 16);
		return strtol(s+2, (char **)end, 10);
	}

	/* unicode U+ code point, hex */
	if ((s[0] == 'U') && (s[1] == '+'))
		return strtol(s+2, (char **)end, 16);

	/* assume plain character */
	*end = s+1;
	return s[0];
}

static int conv_coord_pair(const char *s, double *x, double *y)
{
	char *end;
	*x = strtod(s, &end);
	if (*end == '\0') {
		*y = *x;
		return 0;
	}
	if ((*end != ',') && (*end != ';') && (*end != 'x') && (*end != '*') && (*end != '/'))
		return -1;
	*y = strtod(end+1, &end);
	if (*end == '\0')
		return 0;
	return -1;
}

static int ttf_import(pcb_board_t *pcb, pcb_ttf_t *ctx, pcb_ttf_stroke_t *stroke, int src_from, int src_to, int dst)
{
	int r, src, ret = 0;
	rnd_font_t *f = pcb_font(pcb, conf_core.design.text_font_id, 1);


	stroke->funcs.move_to = str_move_to;
	stroke->funcs.line_to = str_line_to;
	stroke->funcs.conic_to = stroke_approx_conic_to;
	stroke->funcs.cubic_to = stroke_approx_cubic_to;

	stroke->init   = str_init;
	stroke->start  = str_start;
	stroke->finish = str_finish;
	stroke->uninit = str_uninit;

	stroke->ttf = ctx;

	for(src = src_from; (src <= src_to) && (dst < (PCB_MAX_FONTPOSITION+1)); src++,dst++) {
		rnd_trace("face: %d -> %d\n", src, dst);
		stroke->glyph = &f->glyph[dst];

		rnd_font_free_glyph(stroke->glyph);

		r = pcb_ttf_trace(ctx, src, src, stroke, 1);
		if (r != 0)
			ret = -1;

		if (stroke->want_poly) {
			poly_flush(stroke);
			poly_apply(stroke);
		}

		stroke->glyph->valid = 1;
		stroke->glyph->width  = TRX_(ctx->face->glyph->advance.x);
		stroke->glyph->height = TRY_(ctx->face->ascender + ctx->face->descender);
		stroke->glyph->xdelta = RND_MIL_TO_COORD(12);
	}

	return ret;
}

static const char pcb_acts_LoadTtfGlyphs[] = "LoadTtfGlyphs(filename, srcglyps, [dstchars], [outline|polygon], [scale], [offset])";
static const char pcb_acth_LoadTtfGlyphs[] = "Loads glyphs from an outline ttf in the specified source range, optionally remapping them to dstchars range in the pcb-rnd font";
/* DOC: loadttfglyphs.html */
fgw_error_t pcb_act_LoadTtfGlyphs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *fn, *ssrc, *smode = "polygon", *sdst = NULL, *sscale = "0.001", *soffs = "0", *end = NULL;
	pcb_ttf_t ctx = {0};
	pcb_ttf_stroke_t stroke = {0};
	int r, ret = 0, dst, src_from, src_to;

	RND_ACT_CONVARG(1, FGW_STR, LoadTtfGlyphs, fn = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, LoadTtfGlyphs, ssrc = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LoadTtfGlyphs, smode = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, LoadTtfGlyphs, sdst = argv[4].val.str);
	RND_ACT_MAY_CONVARG(5, FGW_STR, LoadTtfGlyphs, sscale = argv[5].val.str);
	RND_ACT_MAY_CONVARG(6, FGW_STR, LoadTtfGlyphs, soffs = argv[6].val.str);

	r = pcb_ttf_load(&ctx, fn);
	if (r != 0) {
		RND_ACT_IRES(-1);
		return 0;
	}
	rnd_trace("ttf load; %d\n", r);

	src_from = conv_char_desc(ssrc, &end);
	if ((end[0] == '.') && (end[1] == '.'))
		src_to = conv_char_desc(end+2, &end);
	else
		src_to = src_from;

	if ((end[0] != '\0') || (src_from < 0) || (src_to < 0)) {
		rnd_message(RND_MSG_ERROR, "LoadTtfGlyphs(): invalid source character: %s\n", ssrc);
		RND_ACT_IRES(-1);
		return 0;
	}

	if (sdst != NULL) {
		dst = conv_char_desc(sdst, &end);
		if ((end[0] != '\0') || (dst < 0) || (dst > 255)) {
			rnd_message(RND_MSG_ERROR, "LoadTtfGlyphs(): invalid destination character: %s\n", sdst);
			RND_ACT_IRES(-1);
			return 0;
		}
	}
	else
		dst = src_from;

	if (strncmp(smode, "poly", 4) == 0)
		stroke.want_poly = 1;
	else if (strcmp(smode, "outline") == 0)
		stroke.want_poly = 0;
	else {
		rnd_message(RND_MSG_ERROR, "LoadTtfGlyphs(): invalid mode: %s\n", smode);
		RND_ACT_IRES(-1);
		return 0;
	}

	if (conv_coord_pair(sscale, &stroke.scale_x, &stroke.scale_y) != 0) {
		rnd_message(RND_MSG_ERROR, "LoadTtfGlyphs(): invalid scale: %s\n", sscale);
		RND_ACT_IRES(-1);
		return 0;
	}
	
	if (conv_coord_pair(soffs,  &stroke.dx, &stroke.dy) != 0) {
		rnd_message(RND_MSG_ERROR, "LoadTtfGlyphs(): invalid offs: %s\n", sscale);
		RND_ACT_IRES(-1);
		return 0;
	}

	ret = ttf_import(pcb, &ctx, &stroke, src_from, src_to, dst);

rnd_trace(" xform: %f;%f %f;%f\n", stroke.scale_x, stroke.scale_y, stroke.dx, stroke.dy);

	pcb_ttf_unload(&ctx);
	RND_ACT_IRES(ret);
	return 0;
}

/*** dialog ***/
typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active;
	pcb_ttf_t ttf;
	int loaded; /* ttf loaded */
	int wmsg, wfont, wsrc, wdst, wrend, wscale, wox, woy, wimport, wprv;
	int timer_active;
	rnd_hidval_t timer;
} ttfgui_ctx_t;

ttfgui_ctx_t ttfgui_ctx;

static void ttfgui_unload(ttfgui_ctx_t *ctx)
{
	if (!ctx->loaded) return;

	pcb_ttf_unload(&ctx->ttf);
	ctx->loaded = 0;
}

static void ttfgui_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	ttfgui_ctx_t *ctx = caller_data;

	ttfgui_unload(ctx);
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(ttfgui_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void ttf_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
/*	ttfgui_ctx_t *ctx = prv->user_ctx;*/
	char s[17];
	int x, y, v;

	rnd_render->set_color(gc, rnd_color_black);

	s[16] = '\0';
	v = 0;
	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++)
			s[x] = v++;
		pcb_text_draw_string_simple(NULL, s, RND_MM_TO_COORD(0), RND_MM_TO_COORD(y*2), 1.0, 1.0, 0.0, 0, 0, 0, 0, 0);
	}
}

static void load_src_dst(ttfgui_ctx_t *ctx, const char *ssrc, const char *sdst)
{
	rnd_hid_attr_val_t hv;

	hv.str = ssrc;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wsrc, &hv);

	hv.str = sdst;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdst, &hv);
}

static void load_all_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	load_src_dst(caller_data, "&#32..&#126", "&#32");
}

static void load_low_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	load_src_dst(caller_data, "a..z", "a");
}

static void load_up_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	load_src_dst(caller_data, "A..Z", "A");
}

static void load_num_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	load_src_dst(caller_data, "0..9", "0");
}

static void font_change_timer_cb(rnd_hidval_t user_data)
{
	ttfgui_ctx_t *ctx = user_data.ptr;
	if (ctx->active) {
		int r;
		const char *fn = ctx->dlg[ctx->wfont].val.str;
		char *tmp;
		rnd_hid_attr_val_t hv;

		ttfgui_unload(ctx);
		r = pcb_ttf_load(&ctx->ttf, fn);
		if (r == 0) {
			ctx->loaded = 1;
			tmp = rnd_strdup_printf("Loaded %s", fn);
			rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wimport, 1);
		}
		else {
			tmp = rnd_strdup_printf("ERROR: failed to load %s", fn);
			rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wimport, 0);
		}

		hv.str = tmp;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wmsg, &hv);

		free(tmp);
	}
	ctx->timer_active = 0;
}

static void font_change_timer(ttfgui_ctx_t *ctx, int period)
{
	rnd_hidval_t hv;

	if (ctx->timer_active && (rnd_gui->stop_timer != NULL))
		rnd_gui->stop_timer(rnd_gui, ctx->timer);

	hv.ptr = ctx;
	ctx->timer = rnd_gui->add_timer(rnd_gui, font_change_timer_cb, period, hv);
	ctx->timer_active = 1;
}

static void font_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	font_change_timer(caller_data, 750);
}

static void font_browse_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fn;
	ttfgui_ctx_t *ctx = caller_data;
		rnd_hid_attr_val_t hv;

	fn = rnd_hid_fileselect(rnd_gui, "Import ttf file", "Select a ttf file (or other font file that libfreetype can load) for importing glyphs from",
		NULL, "ttf", NULL, "import_ttf", RND_HID_FSD_READ, NULL);

	if (fn == NULL)
		return;

	hv.str = fn;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfont, &hv);
	free(fn);
	font_change_timer(caller_data, 100);
}

static void rezoom(ttfgui_ctx_t *ctx)
{
	rnd_box_t bbox;
	bbox.X1 = 0;
	bbox.Y1 = 0;
	bbox.X2 = RND_MM_TO_COORD(32);
	bbox.Y2 = RND_MM_TO_COORD(32);
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprv], &bbox);
}

static void import_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int src_from, src_to, dst, ret;
	pcb_ttf_stroke_t stroke = {0};
	const char *end;
	ttfgui_ctx_t *ctx = caller_data;

	if ((ctx->dlg[ctx->wsrc].val.str == NULL) || (ctx->dlg[ctx->wdst].val.str == NULL)) {
		rnd_message(RND_MSG_ERROR, "missing from/to ranges for the character mapping\n");
		return;
	}

	src_from = conv_char_desc(ctx->dlg[ctx->wsrc].val.str, &end);
	if ((end[0] == '.') && (end[1] == '.'))
		src_to = conv_char_desc(end+2, &end);
	else
		src_to = src_from;
	if ((end[0] != '\0') || (src_from < 0) || (src_to < 0)) {
		rnd_message(RND_MSG_ERROR, "invalid source character\n");
		return;
	}

	dst = conv_char_desc(ctx->dlg[ctx->wdst].val.str, &end);
	if ((end[0] != '\0') || (dst < 0) || (dst > 255)) {
		rnd_message(RND_MSG_ERROR, "invalid destination character\n");
		return;
	}

	stroke.want_poly = (ctx->dlg[ctx->wrend].val.lng == 0);

	stroke.scale_x = stroke.scale_y = ctx->dlg[ctx->wscale].val.dbl;
	stroke.dx = ctx->dlg[ctx->wox].val.dbl;
	stroke.dy = ctx->dlg[ctx->woy].val.dbl;

	ret = ttf_import(PCB, &ctx->ttf, &stroke, src_from, src_to, dst);
	if (ret != 0)
		rnd_message(RND_MSG_ERROR, "ttf import failed - make sure your character range settings are good\n");

	/* redraw */
	rnd_gui->invalidate_all(rnd_gui);
	rezoom(ctx);
}


static const char pcb_acts_LoadTtf[] = "LoadTtf()";
static const char pcb_acth_LoadTtf[] = "Presents a GUI dialog for interactively loading glyphs from from a ttf file";
fgw_error_t pcb_act_LoadTtf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static const char *rend_names[] =  {"polygon", "outline", NULL};
	ttfgui_ctx_t *ctx = &ttfgui_ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	RND_ACT_IRES(0);
	if (ctx->active)
		return 0; /* do not open another */

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

			RND_DAD_BEGIN_HPANE(ctx->dlg, "left-right");
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

				/* left */
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_BEGIN_HBOX(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, "Font:");
						RND_DAD_STRING(ctx->dlg);
							ctx->wfont = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_WIDTH_CHR(ctx->dlg, 32);
							RND_DAD_CHANGE_CB(ctx->dlg, font_change_cb);
						RND_DAD_BUTTON(ctx->dlg, "Browse");
							RND_DAD_CHANGE_CB(ctx->dlg, font_browse_cb);
					RND_DAD_END(ctx->dlg);
					RND_DAD_LABEL(ctx->dlg, "");
						ctx->wmsg = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
						RND_DAD_LABEL(ctx->dlg, "Source character(s):");
						RND_DAD_STRING(ctx->dlg);
							ctx->wsrc = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, "Destination start:");
						RND_DAD_STRING(ctx->dlg);
							ctx->wdst = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, "Rendering:");
						RND_DAD_ENUM(ctx->dlg, rend_names);
							ctx->wrend = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, "Scale:");
						RND_DAD_REAL(ctx->dlg);
							ctx->wscale = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_DEFAULT_NUM(ctx->dlg, 0.001);
						RND_DAD_LABEL(ctx->dlg, "X offset:");
						RND_DAD_REAL(ctx->dlg);
							ctx->wox = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_DEFAULT_NUM(ctx->dlg, 0);
						RND_DAD_LABEL(ctx->dlg, "Y offset:");
						RND_DAD_REAL(ctx->dlg);
							ctx->woy = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_DEFAULT_NUM(ctx->dlg, 0);
					RND_DAD_END(ctx->dlg);

				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
					RND_DAD_BUTTON(ctx->dlg, "All");
						RND_DAD_CHANGE_CB(ctx->dlg, load_all_cb);
					RND_DAD_BUTTON(ctx->dlg, "Lowercase");
						RND_DAD_CHANGE_CB(ctx->dlg, load_low_cb);
					RND_DAD_BUTTON(ctx->dlg, "Uppercase");
						RND_DAD_CHANGE_CB(ctx->dlg, load_up_cb);
					RND_DAD_BUTTON(ctx->dlg, "Number");
						RND_DAD_CHANGE_CB(ctx->dlg, load_num_cb);
					RND_DAD_BEGIN_VBOX(ctx->dlg);
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
					RND_DAD_END(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Import!");
						ctx->wimport = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, import_cb);
				RND_DAD_END(ctx->dlg);
				RND_DAD_END(ctx->dlg);

				/* right */
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_PREVIEW(ctx->dlg, ttf_expose, NULL, NULL, NULL, NULL, 300, 300, ctx);
					ctx->wprv = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_END(ctx->dlg);

			RND_DAD_END(ctx->dlg);
		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	/* set up the context */
	ctx->active = 1;

	RND_DAD_NEW("load_ttf", ctx->dlg, "Load glyphs from ttf font", ctx, rnd_false, ttfgui_close_cb);

	rezoom(ctx);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wimport, 0); /* disable import button before a file name is specified */

	return 0;
}

rnd_action_t ttf_action_list[] = {
	{"LoadTtfGlyphs", pcb_act_LoadTtfGlyphs, pcb_acth_LoadTtfGlyphs, pcb_acts_LoadTtfGlyphs},
	{"LoadTtf", pcb_act_LoadTtf, pcb_acth_LoadTtf, pcb_acts_LoadTtf}
};

int pplg_check_ver_import_ttf(int ver_needed) { return 0; }

void pplg_uninit_import_ttf(void)
{
	rnd_hid_menu_unload(rnd_gui, ttf_cookie);
	rnd_remove_actions_by_cookie(ttf_cookie);
}

int pplg_init_import_ttf(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(ttf_action_list, ttf_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, ttf_cookie, 50, NULL, 0, import_ttf_menu, "plugin: import_ttf");
	return 0;
}
