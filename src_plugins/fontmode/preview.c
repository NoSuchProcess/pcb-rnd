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
#include <genht/hash.h>
#include <librnd/hid/hid_dad.h>
#include "board.h"
#include "draw.h"
#include "font.h"

#define RND_TIMED_CHG_TIMEOUT 1000
#include <librnd/plugins/lib_hid_common/timed_chg.h>

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int wprev, wpend;
#ifdef PCB_WANT_FONT2
	int wbaseline, wtab_width, wline_height, wentt, wkernt;
	unsigned geo_changed:1;
	unsigned ent_changed:1;
	unsigned kern_changed:1;
#endif
	rnd_timed_chg_t timed_refresh;
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

	rnd_timed_chg_cancel(&ctx->timed_refresh);
	rnd_font_free(&ctx->font);
	RND_DAD_FREE(ctx->dlg);

	memset(ctx, 0, sizeof(fmprv_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void fmprv_pcb2preview_sample(fmprv_ctx_t *ctx)
{
	rnd_font_free(&ctx->font);
	memset(&ctx->font, 0, sizeof(ctx->font)); /* clear any cache */
	editor2font(PCB, &ctx->font, fontedit_src);
#ifdef PCB_WANT_FONT2
	ctx->font.baseline = fontedit_src->baseline;
	ctx->font.line_height = fontedit_src->line_height;
	ctx->font.tab_width = fontedit_src->tab_width;
	rnd_font_copy_tables(&ctx->font, fontedit_src);
#endif

}

#ifdef PCB_WANT_FONT2
static void fmprv_pcb2preview_geo(fmprv_ctx_t *ctx)
{
	rnd_hid_attr_val_t hv;

	hv.crd = fontedit_src->baseline;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wbaseline, &hv);

	hv.crd = fontedit_src->tab_width;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtab_width, &hv);

	hv.crd = fontedit_src->line_height;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wline_height, &hv);
}

static void fmprv_pcb2preview_entities(fmprv_ctx_t *ctx)
{

}

static void fmprv_pcb2preview_kerning(fmprv_ctx_t *ctx)
{

}
#endif


static void fmprv_pcb2preview(fmprv_ctx_t *ctx)
{
	if (fontedit_src == NULL) {
		rnd_message(RND_MSG_ERROR, "fmprv_pcb2preview(): No font editor running\n");
		return;
	}

	fmprv_pcb2preview_sample(ctx);

#ifdef PCB_WANT_FONT2
	if (ctx->geo_changed)
		fmprv_pcb2preview_geo(ctx);
	if (ctx->ent_changed)
		fmprv_pcb2preview_entities(ctx);
	if (ctx->kern_changed)
		fmprv_pcb2preview_kerning(ctx);

#endif

	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 1);
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], NULL); /* redraw */
}

static void fmprv_pcb2preview_timed(void *ctx)
{
	fmprv_pcb2preview(ctx);
}


static void font_prv_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	fmprv_ctx_t *ctx = prv->user_ctx;
	rnd_xform_t xform = {0};
	pcb_draw_info_t info = {0};
#ifdef PCB_WANT_FONT2
	rnd_font_render_opts_t opts = RND_FONT_HTAB | RND_FONT_ENTITY | RND_FONT_MULTI_LINE;
#else
	int opts = 0;
#endif

	info.xform = &xform;
	pcb_draw_setup_default_gui_xform(&xform);


	rnd_render->set_color(pcb_draw_out.fgGC, rnd_color_black);

#ifdef PCB_WANT_FONT2
	rnd_font_draw_string(&ctx->font, ctx->sample, 0, 0,
		1.0, 1.0, 0.0,
		opts, 0, 0, RND_FONT_TINY_ACCURATE, pcb_font_draw_atom, &info);
#else
	rnd_font_draw_string(&ctx->font, ctx->sample, 0, 0,
		1.0, 1.0, 0.0,
		opts, 0, 0, 0, RND_FONT_TINY_ACCURATE, pcb_font_draw_atom, &info);
#endif
}

static void timed_refresh(fmprv_ctx_t *ctx)
{
	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 0);
	rnd_timed_chg_schedule(&ctx->timed_refresh);
}

static void refresh_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
#ifdef PCB_WANT_FONT2
	fmprv_ctx_t *ctx = caller_data;
	int widx = attr - ctx->dlg;

	if (widx == ctx->wbaseline) {
		ctx->geo_changed = 1;
		fontedit_src->baseline = attr->val.crd;
	}
	else if (widx == ctx->wline_height) {
		ctx->geo_changed = 1;
		fontedit_src->line_height = attr->val.crd;
	}
	else if (widx == ctx->wtab_width) {
		ctx->geo_changed = 1;
		fontedit_src->tab_width = attr->val.crd;
	}
	else
		return;

	timed_refresh(ctx);
#endif
}

static void pcb_dlg_fontmode_preview(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static rnd_box_t prvbb = {0, 0, RND_MM_TO_COORD(40), RND_MM_TO_COORD(20)};
	static const char *tab_names[] = {"geometry", "entities", "kerning", NULL};
	static const char *ent_hdr[]   = {"entity name", "glyph index", NULL};
	static const char *kern_hdr[]  = {"character pair", "displacement", NULL};

	if (fmprv_ctx.active)
		return; /* do not open another */

	if (fmprv_ctx.sample == NULL)
		fmprv_ctx.sample = (unsigned char *)rnd_strdup("Sample string\nin multiple\nlines.");

	rnd_timed_chg_init(&fmprv_ctx.timed_refresh, fmprv_pcb2preview_timed, &fmprv_ctx);

	RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg);
		RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_PREVIEW(fmprv_ctx.dlg, font_prv_expose_cb, NULL, NULL, NULL, &prvbb, 400, 200, &fmprv_ctx);
			fmprv_ctx.wprev = RND_DAD_CURRENT(fmprv_ctx.dlg);

		RND_DAD_BEGIN_HBOX(fmprv_ctx.dlg);
			RND_DAD_BUTTON(fmprv_ctx.dlg, "Edit sample text");
			RND_DAD_LABEL(fmprv_ctx.dlg, "(pending refresh)");
				fmprv_ctx.wpend = RND_DAD_CURRENT(fmprv_ctx.dlg);
		RND_DAD_END(fmprv_ctx.dlg);

		RND_DAD_BEGIN_TABBED(fmprv_ctx.dlg, tab_names);
			RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);

			RND_DAD_BEGIN_TABLE(fmprv_ctx.dlg, 2); /* geometry */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
#ifndef PCB_WANT_FONT2
				RND_DAD_LABEL(fmprv_ctx.dlg, "Not supported in font v1");
#else
				RND_DAD_LABEL(fmprv_ctx.dlg, "Baseline:");
				RND_DAD_COORD(fmprv_ctx.dlg);
					fmprv_ctx.wbaseline = RND_DAD_CURRENT(fmprv_ctx.dlg);
					RND_DAD_CHANGE_CB(fmprv_ctx.dlg, refresh_cb);
				RND_DAD_LABEL(fmprv_ctx.dlg, "Line height:");
				RND_DAD_COORD(fmprv_ctx.dlg);
					fmprv_ctx.wline_height = RND_DAD_CURRENT(fmprv_ctx.dlg);
					RND_DAD_CHANGE_CB(fmprv_ctx.dlg, refresh_cb);
				RND_DAD_LABEL(fmprv_ctx.dlg, "Tab width:");
				RND_DAD_COORD(fmprv_ctx.dlg);
					fmprv_ctx.wtab_width = RND_DAD_CURRENT(fmprv_ctx.dlg);
					RND_DAD_CHANGE_CB(fmprv_ctx.dlg, refresh_cb);
#endif
			RND_DAD_END(fmprv_ctx.dlg);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* entities */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
#ifndef PCB_WANT_FONT2
				RND_DAD_LABEL(fmprv_ctx.dlg, "Not supported in font v1");
#else
				RND_DAD_TREE(fmprv_ctx.dlg, 2, 0, ent_hdr);
					fmprv_ctx.wentt = RND_DAD_CURRENT(fmprv_ctx.dlg);
#endif
			RND_DAD_END(fmprv_ctx.dlg);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* kerning */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
#ifndef PCB_WANT_FONT2
				RND_DAD_LABEL(fmprv_ctx.dlg, "Not supported in font v1");
#else
				RND_DAD_TREE(fmprv_ctx.dlg, 2, 0, kern_hdr);
					fmprv_ctx.wkernt = RND_DAD_CURRENT(fmprv_ctx.dlg);
#endif
			RND_DAD_END(fmprv_ctx.dlg);
		RND_DAD_END(fmprv_ctx.dlg);

		RND_DAD_BUTTON_CLOSES(fmprv_ctx.dlg, clbtn);
	RND_DAD_END(fmprv_ctx.dlg);

	/* set up the context */
	fmprv_ctx.active = 1;

	RND_DAD_NEW("font_mode_preview", fmprv_ctx.dlg, "Font editor: preview", &fmprv_ctx, rnd_false, fmprv_close_cb);

	fmprv_ctx.geo_changed = fmprv_ctx.ent_changed = fmprv_ctx.kern_changed = 1;
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
