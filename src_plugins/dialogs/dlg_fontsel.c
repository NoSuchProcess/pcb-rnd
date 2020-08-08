/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>
#include "board.h"
#include "change.h"
#include "conf_core.h"
#include "event.h"
#include <librnd/core/hid_dad.h>
#include "stub_draw.h"
#include "idpath.h"
#include "search.h"
#include "dlg_fontsel.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wprev;
	
	unsigned active:1;
	unsigned alloced:1;

	pcb_idpath_t *txt_id;
	void *last_fobj;
	pcb_font_id_t last_fid;
	gdl_elem_t link;
} fontsel_ctx_t;

gdl_list_t fontsels;  /* object font selectors */
fontsel_ctx_t fontsel_brd;

static const char *fontsel_cookie = "dlg_fontsel";

static void fontsel_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	fontsel_ctx_t *ctx = caller_data;

	if (ctx->txt_id != NULL)
		pcb_idpath_destroy(ctx->txt_id);

	RND_DAD_FREE(ctx->dlg);
	if (ctx->alloced) {
		gdl_remove(&fontsels, ctx, link);
		free(ctx);
	}
	else
		memset(ctx, 0, sizeof(fontsel_ctx_t));
}

void fontsel_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	fontsel_ctx_t *ctx = prv->user_ctx;

	if (ctx->txt_id != NULL) {
		pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj_in(ctx->pcb->Data, ctx->txt_id);
		if (txt != NULL)
			pcb_stub_draw_fontsel(gc, e, txt);
		ctx->last_fobj = txt;
		ctx->last_fid = txt->fid;
	}
	else {
		pcb_stub_draw_fontsel(gc, e, NULL);
		ctx->last_fobj = NULL;
	}
}

rnd_bool fontsel_mouse_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	fontsel_ctx_t *ctx = prv->user_ctx;

	if (ctx->txt_id != NULL) {
		pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj_in(ctx->pcb->Data, ctx->txt_id);
		if (txt == NULL)
			return 0;
		return pcb_stub_draw_fontsel_mouse_ev(kind, x, y, txt);
	}
	return pcb_stub_draw_fontsel_mouse_ev(kind, x, y, NULL);
}

void fontsel_free_cb(rnd_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx)
{
}

static void fontsel_preview_update(fontsel_ctx_t *ctx)
{
	rnd_hid_attr_val_t hv;

	if ((ctx == NULL) || (!ctx->active))
		return;

	hv.str = NULL;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}


static void btn_load_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, "LoadFontFrom", NULL); /* modal, blocking */
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}

static void btn_replace_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char file[1] = "", id[5];
	rnd_snprintf(id, sizeof(id), "%ld", conf_core.design.text_font_id);
	rnd_actionva(&PCB->hidlib, "LoadFontFrom", file, id, NULL); /* modal, blocking */
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}

static void btn_remove_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	if (conf_core.design.text_font_id == 0) {
		rnd_message(RND_MSG_ERROR, "Can not remove the default font.\n");
		return;
	}
	pcb_del_font(&PCB->fontkit, conf_core.design.text_font_id);
	rnd_conf_set(RND_CFR_DESIGN, "design/text_font_id", 0, "0", RND_POL_OVERWRITE);
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}


static void pcb_dlg_fontsel(pcb_board_t *pcb, int modal, int global, pcb_text_t *txt_obj)
{
	rnd_box_t vbox = {0, 0, RND_MM_TO_COORD(55), RND_MM_TO_COORD(55)};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	fontsel_ctx_t *c, *ctx = NULL;

	if (global) {
		if (fontsel_brd.active)
			return; /* do not open another */
		ctx = &fontsel_brd;
		ctx->alloced = 0;
	}
	else {
		for(c = gdl_first(&fontsels); c != NULL; c = gdl_next(&fontsels, c)) {
			pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj_in(c->pcb->Data, c->txt_id);
			if (txt == txt_obj) {
				rnd_message(RND_MSG_ERROR, "There is already an active fontedit dialog for that object,\nnot going to open a second dialog.\n");
				return;
			}
		}
		ctx = calloc(sizeof(fontsel_ctx_t), 1);
		ctx->alloced = 1;
		gdl_insert(&fontsels, ctx, link);
	}

	ctx->pcb = pcb;
	if (txt_obj != NULL)
		ctx->txt_id = pcb_obj2idpath((pcb_any_obj_t *)txt_obj);
	else
		ctx->txt_id = NULL;
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_PREVIEW(ctx->dlg, fontsel_expose_cb, fontsel_mouse_cb, NULL, fontsel_free_cb, &vbox, 200, 200, ctx);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			ctx->wprev = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Load font");
			RND_DAD_CHANGE_CB(ctx->dlg, btn_load_cb);
			RND_DAD_HELP(ctx->dlg, "Load new font from disk");
		RND_DAD_BUTTON(ctx->dlg, "Replace font");
			RND_DAD_CHANGE_CB(ctx->dlg, btn_replace_cb);
			RND_DAD_HELP(ctx->dlg, "Replace currently selected font\nwith new font loaded from disk");
		RND_DAD_BUTTON(ctx->dlg, "Remove font");
			RND_DAD_CHANGE_CB(ctx->dlg, btn_remove_cb);
			RND_DAD_HELP(ctx->dlg, "Remove currently selected font");
		RND_DAD_END(ctx->dlg);
		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	ctx->active = 1;
	RND_DAD_DEFSIZE(ctx->dlg, 250, 200);
	RND_DAD_NEW("fontsel", ctx->dlg, "Font selection", ctx, modal, fontsel_close_cb);
}

const char pcb_acts_Fontsel[] = "Fontsel()\n";
const char pcb_acth_Fontsel[] = "Open the font selection dialog";
fgw_error_t pcb_act_Fontsel(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = NULL;
	int modal = 0, global = 1;
	pcb_text_t *txt_obj = NULL;

	if (argc > 2)
		RND_ACT_FAIL(Fontsel);

	RND_ACT_MAY_CONVARG(1, FGW_STR, Fontsel, op = argv[1].val.str);

	if (op != NULL) {
		if (rnd_strcasecmp(op, "Object") == 0) {
			rnd_coord_t x, y;
			int type;
			void *ptr1, *ptr2, *ptr3;
			rnd_hid_get_coords("Select an Object", &x, &y, 0);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
				txt_obj = ptr2;
				modal = 0;
				global = 0;
			}
		}
		else
			RND_ACT_FAIL(Fontsel);
	}
	pcb_dlg_fontsel(PCB, modal, global, txt_obj);
	return 0;
}

static void fontsel_mchanged_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	fontsel_ctx_t *c;

	if (fontsel_brd.active)
		fontsel_preview_update(&fontsel_brd);

	for(c = gdl_first(&fontsels); c != NULL; c = gdl_next(&fontsels, c))
		fontsel_preview_update(c);
}

static void fontsel_bchanged_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	fontsel_ctx_t *c, *next;
	rnd_dad_retovr_t retovr;

	if (fontsel_brd.active)
		fontsel_preview_update(&fontsel_brd);

	for(c = gdl_first(&fontsels); c != NULL; c = next) {
		next = gdl_next(&fontsels, c);
		rnd_hid_dad_close(c->dlg_hid_ctx, &retovr, 0);
	}
}

static int fontsel_timer_active = 0;
static rnd_hidval_t fontsel_timer;

static void fontsel_timer_cb(rnd_hidval_t user_data)
{
	fontsel_ctx_t *c;

	for(c = gdl_first(&fontsels); c != NULL; c = gdl_next(&fontsels, c)) {
		pcb_text_t *txt;

		if (c->txt_id == NULL)
			continue;

		txt = (pcb_text_t *)pcb_idpath2obj_in(c->pcb->Data, c->txt_id);
		if ((txt != c->last_fobj) || (txt != NULL && (txt->fid != c->last_fid)))
			fontsel_preview_update(c);
	}
	fontsel_timer = rnd_gui->add_timer(rnd_gui, fontsel_timer_cb, 500, fontsel_timer);
}

static void fontsel_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	fontsel_timer_cb(fontsel_timer);
	fontsel_timer_active = 1;
}

void pcb_dlg_fontsel_uninit(void)
{
	if ((fontsel_timer_active) && (rnd_gui != NULL) && (rnd_gui->stop_timer != NULL))
		rnd_gui->stop_timer(rnd_gui, fontsel_timer);
	rnd_event_unbind_allcookie(fontsel_cookie);
}

void pcb_dlg_fontsel_init(void)
{
	rnd_event_bind(RND_EVENT_BOARD_CHANGED, fontsel_bchanged_ev, NULL, fontsel_cookie);
	rnd_event_bind(RND_EVENT_BOARD_META_CHANGED, fontsel_mchanged_ev, NULL, fontsel_cookie);
	rnd_event_bind(PCB_EVENT_FONT_CHANGED, fontsel_mchanged_ev, NULL, fontsel_cookie);
	rnd_event_bind(RND_EVENT_GUI_INIT, fontsel_gui_init_ev, NULL, fontsel_cookie);
}

