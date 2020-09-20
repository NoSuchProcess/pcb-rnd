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

#define TRX_(x) RND_MM_TO_COORD((x) * stroke.scale_x)
#define TRY_(y) RND_MM_TO_COORD((y) * stroke.scale_y)

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


static const char pcb_acts_LoadTtfGlyphs[] = "LoadTtfGlyphs(filename, srcglyps, [dstchars])";
static const char pcb_acth_LoadTtfGlyphs[] = "Loads glyphs from an outline ttf in the specified source range, optionally remapping them to dstchars range in the pcb-rnd font";
fgw_error_t pcb_act_LoadTtfGlyphs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *fn;
	pcb_ttf_t ctx = {0};
	pcb_ttf_stroke_t stroke = {0};
	int r;
	pcb_font_t *f = pcb_font(pcb, 0, 1);

	RND_ACT_CONVARG(1, FGW_STR, LoadTtfGlyphs, fn = argv[1].val.str);

	r = pcb_ttf_load(&ctx, fn);
	rnd_trace("ttf load; %d\n", r);

	stroke.funcs.move_to = str_move_to;
	stroke.funcs.line_to = str_line_to;
	stroke.funcs.conic_to = stroke_approx_conic_to;
	stroke.funcs.cubic_to = stroke_approx_cubic_to;
	
	stroke.init   = str_init;
	stroke.start  = str_start;
	stroke.finish = str_finish;
	stroke.uninit = str_uninit;

	stroke.scale_x = stroke.scale_y = 0.001;
	stroke.sym = &f->Symbol['A'];
	stroke.ttf = &ctx;

	stroke.want_poly = 1;

	pcb_font_free_symbol(stroke.sym);


	r = pcb_ttf_trace(&ctx, 'A', 'A', &stroke, 1);
	rnd_trace("ttf trace; %d\n", r);

	if (stroke.want_poly) {
		poly_flush(&stroke);
		poly_apply(&stroke);
	}

	stroke.sym->Valid = 1;
	stroke.sym->Width  = TRX_(ctx.face->glyph->advance.x);
	stroke.sym->Height = TRY_(ctx.face->ascender + ctx.face->descender);
	stroke.sym->Delta = RND_MIL_TO_COORD(12);

	pcb_ttf_unload(&ctx);
	RND_ACT_IRES(-1);
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
