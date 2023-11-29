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
#include <librnd/hid/hid_dad_tree.h>
#include "board.h"
#include "draw.h"
#include "font.h"

#define RND_TIMED_CHG_TIMEOUT 1000
#ifdef PCB_WANT_FONT2
#include <librnd/plugins/lib_hid_common/timed_chg.h>
#endif

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int wprev, wpend;
#ifdef PCB_WANT_FONT2
	int wbaseline, wtab_width, wline_height, wentt, wkernt;
	unsigned geo_changed:1;
	unsigned ent_changed:1;
	unsigned kern_changed:1;
	rnd_timed_chg_t timed_refresh;
#endif
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

#ifdef PCB_WANT_FONT2
	rnd_timed_chg_cancel(&ctx->timed_refresh);
#endif
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
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wentt];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r;
	htsi_entry_t *e;
	char *cursor_path = NULL;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->path);

	rnd_dad_tree_clear(tree);

	for(e = htsi_first(&fontedit_src->entity_tbl); e != NULL; e = htsi_next(&fontedit_src->entity_tbl, e)) {
		char *cell[3];
		cell[0] = rnd_strdup(e->key);
		cell[1] = rnd_strdup_printf("%d", e->value);
		cell[2] = NULL;
		rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wentt, &hv);
		free(cursor_path);
	}
}

/* Use the &number format for these */
#define glyph_non_printable(n) ((n <= 32) || (n > 126) || (n == '&') || (n == '#') || (n == '{') || (n == '}') || (n == '/') || (n == ':') || (n == ';') || (n == '=') || (n == '\\') || (n == '\'') || (n == '`'))

static int kern_key_cmp(const void *E1, const void *E2)
{
	const htkc_entry_t **e1 = E1, **e2 = E2;
	if ((*e1)->key.left == (*e2)->key.left)
		return (*e1)->key.right > (*e2)->key.right ? +1 : -1;
	return (*e1)->key.left > (*e2)->key.left ? +1 : -1;
}

static void fmprv_pcb2preview_kerning(fmprv_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wkernt];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r;
	htkc_entry_t *e;
	char *cursor_path = NULL;
	vtp0_t sorted = {0};
	long n;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->path);

	rnd_dad_tree_clear(tree);

	for(e = htkc_first(&fontedit_src->kerning_tbl); e != NULL; e = htkc_next(&fontedit_src->kerning_tbl, e))
		vtp0_append(&sorted, e);

	if (sorted.used > 0)
		qsort(sorted.array, sorted.used, sizeof(htkc_entry_t *), kern_key_cmp);

	for(n = 0; n < sorted.used; n++) {
		char *cell[3];
		gds_t tmp = {0};

		e = sorted.array[n];
		if (glyph_non_printable(e->key.left))
			rnd_append_printf(&tmp, "&%02x-", e->key.left);
		else
			rnd_append_printf(&tmp, "%c-", e->key.left);

		if (glyph_non_printable(e->key.right))
			rnd_append_printf(&tmp, "&%02x", e->key.right);
		else
			rnd_append_printf(&tmp, "%c", e->key.right);

		cell[0] = tmp.array;
		cell[1] = rnd_strdup_printf("%.09$mH", e->value);
		cell[2] = NULL;
		rnd_dad_tree_append(attr, NULL, cell);
	}

	vtp0_uninit(&sorted);

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wkernt, &hv);
		free(cursor_path);
	}

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
#ifdef PCB_WANT_FONT2
	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 0);
	rnd_timed_chg_schedule(&ctx->timed_refresh);
#endif
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

#ifdef PCB_WANT_FONT2
typedef struct {
	char *name;
	rnd_coord_t val;
	int is_crd;
} edit2_t;

RND_INLINE int edit2(edit2_t *ed2)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"Ok", 0}, {NULL, 0}};
	int res, wname, wval;
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(dlg, 2);
			RND_DAD_LABEL(dlg, "key:");
			RND_DAD_STRING(dlg);
				if (ed2->name != NULL) {
					RND_DAD_DEFAULT_PTR(dlg, ed2->name);
					ed2->name = NULL;
				}
				wname = RND_DAD_CURRENT(dlg);
			RND_DAD_LABEL(dlg, "value:");
			if (ed2->is_crd) {
				RND_DAD_COORD(dlg);
				RND_DAD_DEFAULT_NUM(dlg, ed2->val);
				wval = RND_DAD_CURRENT(dlg);
			}
			else {
				RND_DAD_INTEGER(dlg);
				RND_DAD_DEFAULT_NUM(dlg, ed2->val);
				wval = RND_DAD_CURRENT(dlg);
			}
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("font_mode_table_edit", dlg, "Font editor: table entry", NULL, rnd_true, NULL);
	res = RND_DAD_RUN(dlg);

	/* copy result */
	if (res == 0) {
		ed2->val = ed2->is_crd ? dlg[wval].val.crd : dlg[wval].val.lng;
		ed2->name = (char *)dlg[wname].val.str; /* taking over ownership */
		dlg[wname].val.str = NULL;
	}
	else
		ed2->val = 0;

	RND_DAD_FREE(dlg);

	return res;
}


RND_INLINE void edit2_ent(fmprv_ctx_t *ctx, edit2_t ed2, const char *orig_name)
{
	htsi_entry_t *e;

	if ((edit2(&ed2) != 0) || (ed2.name == NULL) || (*ed2.name == '\0'))
		return;

	if (!fontedit_src->entity_tbl_valid) {
		htsi_init(&fontedit_src->entity_tbl, strhash, strkeyeq);
		fontedit_src->entity_tbl_valid = 1;
	}

	/* renamed: remove old entry */
	if ((orig_name != NULL) && (strcmp(ed2.name, orig_name) != 0)) {
		e = htsi_popentry(&fontedit_src->entity_tbl, orig_name);
		free(e->key);
	}

	e = htsi_getentry(&fontedit_src->entity_tbl, ed2.name);
	if (e != NULL) { /* adding an entry that's already in - modify existing */
		free(ed2.name);
		e->value = ed2.val;
	}
	else /* adding a new entry */
		htsi_insert(&fontedit_src->entity_tbl, ed2.name, ed2.val);

	fmprv_pcb2preview(ctx);
}


static void ent_add_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	fmprv_ctx_t *ctx = caller_data;
	edit2_t ed2 = {0};
	edit2_ent(ctx, ed2, NULL);
}

static void ent_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	fmprv_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wentt];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);
	edit2_t ed2;

	if (r == NULL) {
		rnd_message(RND_MSG_ERROR, "Select a row first\n");
		return;
	}


	ed2.name = rnd_strdup(r->cell[0]);
	ed2.val = strtol(r->cell[1], NULL, 10);
	ed2.is_crd = 0;

	edit2_ent(ctx, ed2, r->cell[0]);
}

static int load_kern_key(const char *start, const char *end, char **end_out)
{
	/* single character cases, left or right */
	if (end == start+1) {
		if (end_out != NULL)
			*end_out = (char *)end;
		return *start;
	}
	if ((end == NULL) && (start[0] != '\0') && ((start[1] == '\0') || isspace(start[1]))) {
		if (end_out != NULL)
			*end_out = (char *)start+1;
		return *start;
	}

	if (*start == '&') {
		char *e;
		long val = strtol(start+1, &e, 10);
		if (end != NULL) {
			if (e != end) {
				if (end_out != NULL)
					*end_out = NULL;
				return 0; /* non-numeric suffix on left side */
			}
		}
		else {
			if (end_out != NULL)
				*end_out = e;
			if (isspace(*e)) {
				/* this is okay: multiple pairs are entered */
			}
			else if (*e != '\0') {
				if (end_out != NULL)
					*end_out = NULL;
				return 0; /* non-numeric suffix on right side */
			}
		}
		if ((val < 1) || (val > 254))
			return 0;
		return val;
	}

	return 0; /* unknown format */
}

RND_INLINE void edit2_kern(fmprv_ctx_t *ctx, edit2_t ed2, const char *orig_name)
{
	htkc_entry_t *e;
	htkc_key_t key;
	char *sep, *end, *curr;

	if ((edit2(&ed2) != 0) || (ed2.name == NULL) || (*ed2.name == '\0'))
		return;

	for(curr = ed2.name; curr != NULL; curr = end) {

		while(isspace(*curr)) curr++;
		if (*curr == '\0') break;

		sep = strchr(curr+1, '-'); /* +1 so if '-' is the left char it is preserved */
		if (sep == NULL) {
			rnd_message(RND_MSG_ERROR, "Key needs to be in the form of character pair, e.g. A-V\n");
			return;
		}

		key.left = load_kern_key(curr, sep, NULL);
		key.right = load_kern_key(sep+1, NULL, &end);

		/* renamed: remove old entry */
		if ((orig_name != NULL) && (strcmp(ed2.name, orig_name) != 0))
			htkc_popentry(&fontedit_src->kerning_tbl, key);

		if (!fontedit_src->kerning_tbl_valid) {
			htkc_init(&fontedit_src->kerning_tbl, htkc_keyhash, htkc_keyeq);
			fontedit_src->kerning_tbl_valid = 1;
		}

		e = htkc_getentry(&fontedit_src->kerning_tbl, key);
		if (e != NULL) /* adding an entry that's already in - modify existing */
			e->value = ed2.val;
		else /* adding a new entry */
			htkc_insert(&fontedit_src->kerning_tbl, key, ed2.val);
	}

	free(ed2.name);
	fmprv_pcb2preview(ctx);
}


static void kern_add_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	fmprv_ctx_t *ctx = caller_data;
	edit2_t ed2 = {0};
	ed2.is_crd = 1;
	edit2_kern(ctx, ed2, NULL);
}

static void kern_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	fmprv_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wkernt];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);
	edit2_t ed2;
	rnd_bool succ;

	if (r == NULL) {
		rnd_message(RND_MSG_ERROR, "Select a row first\n");
		return;
	}


	ed2.name = rnd_strdup(r->cell[0]);
	ed2.val = rnd_get_value(r->cell[1], NULL, NULL, &succ);
	if (!succ)
		rnd_message(RND_MSG_ERROR, "invalid coord value '%s' (is the proper unit appended?)\n", r->cell[1]);
	ed2.is_crd = 1;

	edit2_kern(ctx, ed2, r->cell[0]);
}
#endif


static void sample_text_changed_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *atxt)
{
	fmprv_ctx_t *ctx = caller_data;
	rnd_hid_text_t *txt = atxt->wdata;

	free(ctx->sample);
	ctx->sample = (unsigned char *)txt->hid_get_text(atxt, hid_ctx); /* allocated */
	fmprv_pcb2preview(ctx);
}

static void change_sample_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	fmprv_ctx_t *ctx = caller_data;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int wtxt;
	rnd_hid_attribute_t *atxt;
	rnd_hid_text_t *txt;
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_TEXT(dlg, NULL);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			RND_DAD_CHANGE_CB(dlg, sample_text_changed_cb);
			wtxt = RND_DAD_CURRENT(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("font_mode_preview_text_edit", dlg, "Font editor: preview text", ctx, rnd_true, NULL);

	atxt = &dlg[wtxt];
	txt = atxt->wdata;
	txt->hid_set_text(atxt, dlg_hid_ctx, RND_HID_TEXT_REPLACE, (const char *)ctx->sample);

	RND_DAD_RUN(dlg);
	RND_DAD_FREE(dlg);
}


static void pcb_dlg_fontmode_preview(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static rnd_box_t prvbb = {0, 0, RND_MM_TO_COORD(40), RND_MM_TO_COORD(10)};
	static const char *tab_names[] = {"geometry", "entities", "kerning", NULL};
	static const char *ent_hdr[]   = {"entity name", "glyph index", NULL};
	static const char *kern_hdr[]  = {"character pair", "displacement", NULL};

	if (fmprv_ctx.active)
		return; /* do not open another */

	if (fmprv_ctx.sample == NULL)
		fmprv_ctx.sample = (unsigned char *)rnd_strdup("Sample string\nin multiple\nlines.");

#ifdef PCB_WANT_FONT2
	rnd_timed_chg_init(&fmprv_ctx.timed_refresh, fmprv_pcb2preview_timed, &fmprv_ctx);
#endif

	RND_DAD_BEGIN_VPANE(fmprv_ctx.dlg, "vert_main");
		RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg);
			RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_PREVIEW(fmprv_ctx.dlg, font_prv_expose_cb, NULL, NULL, NULL, &prvbb, 400, 100, &fmprv_ctx);
				fmprv_ctx.wprev = RND_DAD_CURRENT(fmprv_ctx.dlg);

			RND_DAD_BEGIN_HBOX(fmprv_ctx.dlg);
				RND_DAD_BUTTON(fmprv_ctx.dlg, "Edit sample text");
					RND_DAD_CHANGE_CB(fmprv_ctx.dlg, change_sample_cb);
				RND_DAD_LABEL(fmprv_ctx.dlg, "(pending refresh)");
					fmprv_ctx.wpend = RND_DAD_CURRENT(fmprv_ctx.dlg);
			RND_DAD_END(fmprv_ctx.dlg);
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
					RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					fmprv_ctx.wentt = RND_DAD_CURRENT(fmprv_ctx.dlg);

				RND_DAD_BEGIN_HBOX(fmprv_ctx.dlg);
					RND_DAD_BUTTON(fmprv_ctx.dlg, "Add");
						RND_DAD_CHANGE_CB(fmprv_ctx.dlg, ent_add_cb);
					RND_DAD_BUTTON(fmprv_ctx.dlg, "Edit");
						RND_DAD_CHANGE_CB(fmprv_ctx.dlg, ent_edit_cb);
				RND_DAD_END(fmprv_ctx.dlg);

				RND_DAD_LABEL(fmprv_ctx.dlg, "(Key format: case sensitive entity name without the &; wrapping)");
#endif
			RND_DAD_END(fmprv_ctx.dlg);

			RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg); /* kerning */
				RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
#ifndef PCB_WANT_FONT2
				RND_DAD_LABEL(fmprv_ctx.dlg, "Not supported in font v1");
#else
				RND_DAD_TREE(fmprv_ctx.dlg, 2, 0, kern_hdr);
					RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					fmprv_ctx.wkernt = RND_DAD_CURRENT(fmprv_ctx.dlg);
				RND_DAD_BEGIN_HBOX(fmprv_ctx.dlg);
					RND_DAD_BUTTON(fmprv_ctx.dlg, "Add");
						RND_DAD_CHANGE_CB(fmprv_ctx.dlg, kern_add_cb);
					RND_DAD_BUTTON(fmprv_ctx.dlg, "Edit");
						RND_DAD_CHANGE_CB(fmprv_ctx.dlg, kern_edit_cb);
					RND_DAD_LABEL(fmprv_ctx.dlg, "(Key format: char1-char2, e.g. A-V or &6b-V or &6b-&a1 in &hh hex glyph\nindex form; multiple keys: space separated list like a-b c-d e-f)");
				RND_DAD_END(fmprv_ctx.dlg);
#endif
			RND_DAD_END(fmprv_ctx.dlg);
		RND_DAD_END(fmprv_ctx.dlg);

		RND_DAD_BUTTON_CLOSES(fmprv_ctx.dlg, clbtn);
	RND_DAD_END(fmprv_ctx.dlg);

	/* set up the context */
	fmprv_ctx.active = 1;

	RND_DAD_NEW("font_mode_preview", fmprv_ctx.dlg, "Font editor: preview", &fmprv_ctx, rnd_false, fmprv_close_cb);

#ifdef PCB_WANT_FONT2
	fmprv_ctx.geo_changed = fmprv_ctx.ent_changed = fmprv_ctx.kern_changed = 1;
#endif
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
