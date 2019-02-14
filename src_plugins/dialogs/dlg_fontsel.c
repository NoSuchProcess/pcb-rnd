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

#include "board.h"
#include "stub_draw.h"
#include "idpath.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wprev;
	
	unsigned active:1;
	unsigned alloced:1;

	pcb_idpath_t *txt_id;
	gdl_elem_t link;
} fontsel_ctx_t;

gdl_list_t fontsels;  /* object font selectors */
fontsel_ctx_t fontsel_brd;

static const char *fontsel_cookie = "dlg_fontsel";

static void fontsel_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	fontsel_ctx_t *ctx = caller_data;

	if (ctx->txt_id != NULL)
		pcb_idpath_destroy(ctx->txt_id);

	PCB_DAD_FREE(ctx->dlg);
	if (ctx->alloced) {
		gdl_remove(&fontsels, ctx, link);
		free(ctx);
	}
	else
		memset(ctx, 0, sizeof(fontsel_ctx_t));
}

void fontsel_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	fontsel_ctx_t *ctx = prv->user_ctx;

	if (ctx->txt_id != NULL) {
		pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj(ctx->pcb->Data, ctx->txt_id);
		if (txt != NULL)
			pcb_stub_draw_fontsel(gc, e, txt);
	}
	else
		pcb_stub_draw_fontsel(gc, e, NULL);
}

pcb_bool fontsel_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	fontsel_ctx_t *ctx = prv->user_ctx;

	if (ctx->txt_id != NULL) {
		pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj(ctx->pcb->Data, ctx->txt_id);
		if (txt == NULL)
			return 0;
		return pcb_stub_draw_fontsel_mouse_ev(kind, x, y, txt);
	}
	return pcb_stub_draw_fontsel_mouse_ev(kind, x, y, NULL);
}

void fontsel_free_cb(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx)
{
}

static void fontsel_preview_update(fontsel_ctx_t *ctx)
{
	pcb_hid_attr_val_t hv;

	if ((ctx == NULL) || (!ctx->active))
		return;

	hv.str_value = NULL;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}


static void btn_load_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("LoadFontFrom", NULL); /* modal, blocking */
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}

static void btn_replace_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	char file[1] = "", id[5];
	pcb_snprintf(id, sizeof(id), "%ld", conf_core.design.text_font_id);
	pcb_actionl("LoadFontFrom", file, id, NULL); /* modal, blocking */
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}

static void btn_remove_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	if (conf_core.design.text_font_id == 0) {
		pcb_message(PCB_MSG_ERROR, "Can not remove the default font.\n");
		return;
	}
	pcb_del_font(&PCB->fontkit, conf_core.design.text_font_id);
	conf_set(CFR_DESIGN, "design/text_font_id", 0, "0", POL_OVERWRITE);
	fontsel_preview_update((fontsel_ctx_t *)caller_data);
}


static void pcb_dlg_fontsel(pcb_board_t *pcb, int modal, int global, pcb_text_t *txt_obj)
{
	pcb_box_t vbox = {0, 0, PCB_MM_TO_COORD(55), PCB_MM_TO_COORD(55)};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	fontsel_ctx_t *c, *ctx = NULL;

	if (global) {
		if (fontsel_brd.active)
			return; /* do not open another */
		ctx = &fontsel_brd;
		ctx->alloced = 0;
	}
	else {
		for(c = gdl_first(&fontsels); c != NULL; c = gdl_next(&fontsels, c)) {
			pcb_text_t *txt = (pcb_text_t *)pcb_idpath2obj(c->pcb->Data, c->txt_id);
			if (txt == txt_obj) {
				pcb_message(PCB_MSG_ERROR, "There is already anm active fontedit dialog for that object,\nnot going to open a second dialog.\n");
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
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_PREVIEW(ctx->dlg, fontsel_expose_cb, fontsel_mouse_cb, fontsel_free_cb, &vbox, 200, 200, ctx);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Load font");
			PCB_DAD_CHANGE_CB(ctx->dlg, btn_load_cb);
			PCB_DAD_HELP(ctx->dlg, "Load new font from disk");
		PCB_DAD_BUTTON(ctx->dlg, "Replace font");
			PCB_DAD_CHANGE_CB(ctx->dlg, btn_replace_cb);
			PCB_DAD_HELP(ctx->dlg, "Replace currently selected font\nwith new font loaded from disk");
		PCB_DAD_BUTTON(ctx->dlg, "Remove font");
			PCB_DAD_CHANGE_CB(ctx->dlg, btn_remove_cb);
			PCB_DAD_HELP(ctx->dlg, "Remove currently selected font");
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	ctx->active = 1;
	PCB_DAD_NEW("fontsel", ctx->dlg, "Font selection", ctx, modal, fontsel_close_cb);
}

static const char pcb_acts_Fontsel[] = "Fontsel()\n";
static const char pcb_acth_Fontsel[] = "Open the font selection dialog";
static fgw_error_t pcb_act_Fontsel(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = NULL;
	int modal = 0, global = 1;
	pcb_text_t *txt_obj = NULL;

	if (argc > 2)
		PCB_ACT_FAIL(Fontsel);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Fontsel, op = argv[1].val.str);

	if (op != NULL) {
		if (pcb_strcasecmp(op, "Object") == 0) {
			pcb_coord_t x, y;
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_hid_get_coords("Select an Object", &x, &y, 0);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
				txt_obj = ptr2;
				modal = 0;
				global = 0;
			}
		}
		else
			PCB_ACT_FAIL(Fontsel);
	}
	pcb_dlg_fontsel(PCB, modal, global, txt_obj);
	return 0;
}

static void fontsel_changed_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (fontsel_brd.active)
		fontsel_preview_update(&fontsel_brd);
}


void pcb_dlg_fontsel_uninit(void)
{
	pcb_event_unbind_allcookie(fontsel_cookie);
}

void pcb_dlg_fontsel_init(void)
{
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, fontsel_changed_ev, NULL, fontsel_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, fontsel_changed_ev, NULL, fontsel_cookie);
}

