/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - font editor preview dlg
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <librnd/core/config.h>
#include <librnd/hid/hid_dad.h>
#include "board.h"
#include "draw.h"
#include "font.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int wprev;
	int active; /* already open - allow only one instance */
	unsigned char *sample;
	rnd_font_t font;
} fmprv_ctx_t;

fmprv_ctx_t fmprv_ctx;


/* these are coming from fontmode.c */
extern rnd_font_t *fontedit_src;
void editor2font(pcb_board_t *pcb, rnd_font_t *font, const rnd_font_t *orig_font);

static void fmprv_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	fmprv_ctx_t *ctx = caller_data;

	rnd_font_free(&ctx->font);
	RND_DAD_FREE(ctx->dlg);

	memset(ctx, 0, sizeof(fmprv_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void fmprv_pcb2preview_sample(fmprv_ctx_t *ctx)
{
	rnd_font_free(&ctx->font);
	editor2font(PCB, &ctx->font, fontedit_src);
#if PCB_WANT_FONT2
	rnd_font_copy_tables(&ctx->font, fontedit_src);
#endif

}

static void fmprv_pcb2preview_geo(fmprv_ctx_t *ctx)
{

}

static void fmprv_pcb2preview_entities(fmprv_ctx_t *ctx)
{

}

static void fmprv_pcb2preview_kerning(fmprv_ctx_t *ctx)
{

}

static void fmprv_pcb2preview(fmprv_ctx_t *ctx)
{
	if (fontedit_src == NULL) {
		rnd_message(RND_MSG_ERROR, "fmprv_pcb2preview(): No font editor running\n");
		return;
	}

	fmprv_pcb2preview_sample(ctx);
	fmprv_pcb2preview_geo(ctx);
	fmprv_pcb2preview_entities(ctx);
	fmprv_pcb2preview_kerning(ctx);

	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], NULL); /* redraw */
}

static void font_prv_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	fmprv_ctx_t *ctx = prv->user_ctx;
	rnd_xform_t xform = {0};
	pcb_draw_info_t info = {0};
#if PCB_WANT_FONT2
	rnd_font_render_opts_t opts = RND_FONT_HTAB | RND_FONT_ENTITY | RND_FONT_MULTI_LINE;
#else
	int opts = 0;
#endif

	info.xform = &xform;
	pcb_draw_setup_default_gui_xform(&xform);


	rnd_render->set_color(pcb_draw_out.fgGC, rnd_color_black);

	rnd_font_draw_string(&ctx->font, ctx->sample, 0, 0,
		1.0, 1.0, 0.0,
		opts, 0, 0, RND_FONT_TINY_ACCURATE, pcb_font_draw_atom, &info);
}

static void pcb_dlg_fontmode_preview(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static rnd_box_t prvbb = {0, 0, RND_MM_TO_COORD(40), RND_MM_TO_COORD(20)};
	static const char *tab_names[] = {"geometry", "entities", "kerning", NULL};

	if (fmprv_ctx.active)
		return; /* do not open another */

	if (fmprv_ctx.sample == NULL)
		fmprv_ctx.sample = (unsigned char *)rnd_strdup("Sample string\nin multiple\nlines.");

	RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg);
		RND_DAD_PREVIEW(fmprv_ctx.dlg, font_prv_expose_cb, NULL, NULL, NULL, &prvbb, 400, 200, &fmprv_ctx);
			fmprv_ctx.wprev = RND_DAD_CURRENT(fmprv_ctx.dlg);

		RND_DAD_BEGIN_HBOX(fmprv_ctx.dlg);
			RND_DAD_BUTTON(fmprv_ctx.dlg, "Edit sample text");
		RND_DAD_END(fmprv_ctx.dlg);

		RND_DAD_BEGIN_TABBED(fmprv_ctx.dlg, tab_names);
			RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* geometry */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_LABEL(fmprv_ctx.dlg, "geo...");
			RND_DAD_END(fmprv_ctx.dlg);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* entities */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_LABEL(fmprv_ctx.dlg, "entities...");
			RND_DAD_END(fmprv_ctx.dlg);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* kerning */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_LABEL(fmprv_ctx.dlg, "kerning...");
			RND_DAD_END(fmprv_ctx.dlg);
		RND_DAD_END(fmprv_ctx.dlg);

		RND_DAD_BUTTON_CLOSES(fmprv_ctx.dlg, clbtn);
	RND_DAD_END(fmprv_ctx.dlg);

	/* set up the context */
	fmprv_ctx.active = 1;

	RND_DAD_NEW("font_mode_preview", fmprv_ctx.dlg, "Font editor: preview", &fmprv_ctx, rnd_false, fmprv_close_cb);

	fmprv_pcb2preview(&fmprv_ctx);
}

const char pcb_acts_FontModePreview[] = "FontModePreview()\n";
const char pcb_acth_FontModePreview[] = "Open the font mode preview dialog that also helps editing font tables";
fgw_error_t pcb_act_FontModePreview(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (fontedit_src == NULL) {
		rnd_message(RND_MSG_ERROR, "FontModePreview() can be invoked only from the font editor\n");
		return -1;
	}
	pcb_dlg_fontmode_preview();
	return 0;
}
