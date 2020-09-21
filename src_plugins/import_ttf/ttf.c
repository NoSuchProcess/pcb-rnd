/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf glyph import
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hid.h>

#include "ttf_load.h"
#include "str_approx.h"

#include <librnd/poly/polygon1_gen.h>

static const char *ttf_cookie = "ttf importer";

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

static void poly_flush(pcb_ttf_stroke_t *str)
{
	int is_neg = 0;
	rnd_polyarea_t *pa;

	if (!str->want_poly || (str->contour == NULL))
		return;

	rnd_poly_contour_pre(str->contour, rnd_true);
	if (str->contour->Flags.orient != RND_PLF_DIR) {
		rnd_poly_contour_inv(str->contour);
		is_neg = 1;
	}

	pa = rnd_polyarea_create();
	rnd_polyarea_contour_include(pa, str->contour);
	vtp0_append((is_neg ? &str->poly_neg : &str->poly_pos), pa);
rnd_trace("poly append: %d [%f] on %s\n", str->contour->Count, str->contour->area/1000000000.0, is_neg ? "neg" : "pos");
	str->contour = NULL;
}

static void ttf_poly_emit(rnd_pline_t *pl, void *ctx)
{
	long n;
	rnd_vnode_t *v;
	pcb_ttf_stroke_t *str = ctx;
	pcb_poly_t *p = pcb_font_new_poly_in_sym(str->sym, pl->Count);

rnd_trace(" emit: %d\n", pl->Count);
	for(n = 0, v = pl->head; n < pl->Count; n++, v = v->next) {
		p->Points[n].X = v->point[0];
		p->Points[n].Y = v->point[1];
	}
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
				str->poly_pos.array[n] = NULL;
				rnd_polyarea_boolean_free(pap, pan, &res, RND_PBO_SUB);
				pap = res;
				str->poly_neg.array[n] = NULL; /* already freed, do not reuse */
			}
		}
		str->poly_pos.array[p] = pap; /* may have changed during the poly bool ops */
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
		pcb_font_new_line_in_sym(str->sym,
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
	pcb_font_t *f = pcb_font(pcb, conf_core.design.text_font_id, 1);


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
		stroke->sym = &f->Symbol[dst];

		pcb_font_free_symbol(stroke->sym);

		r = pcb_ttf_trace(ctx, src, src, stroke, 1);
		if (r != 0)
			ret = -1;

		if (stroke->want_poly) {
			poly_flush(stroke);
			poly_apply(stroke);
		}

		stroke->sym->Valid = 1;
		stroke->sym->Width  = TRX_(ctx->face->glyph->advance.x);
		stroke->sym->Height = TRY_(ctx->face->ascender + ctx->face->descender);
		stroke->sym->Delta = RND_MIL_TO_COORD(12);
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

rnd_action_t ttf_action_list[] = {
	{"LoadTtfGlyphs", pcb_act_LoadTtfGlyphs, pcb_acth_LoadTtfGlyphs, pcb_acts_LoadTtfGlyphs}
};

int pplg_check_ver_import_ttf(int ver_needed) { return 0; }

void pplg_uninit_import_ttf(void)
{
	rnd_remove_actions_by_cookie(ttf_cookie);
}

int pplg_init_import_ttf(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(ttf_action_list, ttf_cookie)
	return 0;
}
