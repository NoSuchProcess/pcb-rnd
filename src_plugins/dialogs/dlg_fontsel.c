/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wprev;
	int active;

	pcb_text_t *old_txt;
} fontsel_ctx_t;

fontsel_ctx_t fontsel_ctx;

static void fontsel_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	fontsel_ctx_t *ctx = caller_data;

	*pcb_stub_draw_fontsel_text_obj = ctx->old_txt;

	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(fontsel_ctx_t));
}

void fontsel_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_stub_draw_fontsel(gc, e);
}

pcb_bool fontsel_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_stub_draw_fontsel_mouse_ev(kind, x, y);
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


static void pcb_dlg_fontsel(pcb_board_t *pcb)
{
	pcb_box_t vbox = {0, 0, PCB_MM_TO_COORD(55), PCB_MM_TO_COORD(55)};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (fontsel_ctx.active)
		return; /* do not open another */

	fontsel_ctx.pcb = pcb;
	PCB_DAD_BEGIN_VBOX(fontsel_ctx.dlg);
		PCB_DAD_COMPFLAG(fontsel_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_PREVIEW(fontsel_ctx.dlg, fontsel_expose_cb, fontsel_mouse_cb, fontsel_free_cb, &vbox, 200, 200, &fontsel_ctx);
			PCB_DAD_COMPFLAG(fontsel_ctx.dlg, PCB_HATF_EXPFILL);
			fontsel_ctx.wprev = PCB_DAD_CURRENT(fontsel_ctx.dlg);
		PCB_DAD_BEGIN_HBOX(fontsel_ctx.dlg);
		PCB_DAD_BUTTON(fontsel_ctx.dlg, "Load font");
			PCB_DAD_CHANGE_CB(fontsel_ctx.dlg, btn_load_cb);
			PCB_DAD_HELP(fontsel_ctx.dlg, "Load new font from disk");
		PCB_DAD_BUTTON(fontsel_ctx.dlg, "Replace font");
			PCB_DAD_CHANGE_CB(fontsel_ctx.dlg, btn_replace_cb);
			PCB_DAD_HELP(fontsel_ctx.dlg, "Replace currently selected font\nwith new font loaded from disk");
		PCB_DAD_BUTTON(fontsel_ctx.dlg, "Remove font");
			PCB_DAD_CHANGE_CB(fontsel_ctx.dlg, btn_remove_cb);
			PCB_DAD_HELP(fontsel_ctx.dlg, "Remove currently selected font");
		PCB_DAD_END(fontsel_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(fontsel_ctx.dlg, clbtn);
	PCB_DAD_END(fontsel_ctx.dlg);

	fontsel_ctx.active = 1;
	PCB_DAD_NEW("fontsel", fontsel_ctx.dlg, "Font selection", &fontsel_ctx, pcb_true, fontsel_close_cb);
}

static const char pcb_acts_Fontsel[] = "Fontsel()\n";
static const char pcb_acth_Fontsel[] = "Open the font selection dialog";
static fgw_error_t pcb_act_Fontsel(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op = NULL;
	if (argc > 2)
		PCB_ACT_FAIL(Fontsel);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Fontsel, op = argv[1].val.str);

	fontsel_ctx.old_txt = *pcb_stub_draw_fontsel_text_obj;
	*pcb_stub_draw_fontsel_text_obj = NULL;

	if (op != NULL) {
		if (pcb_strcasecmp(op, "Object") == 0) {
			pcb_coord_t x, y;
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_hid_get_coords("Select an Object", &x, &y, 0);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID)
				*pcb_stub_draw_fontsel_text_obj = ptr2;
		}
		else
			PCB_ACT_FAIL(Fontsel);
	}
	pcb_dlg_fontsel(PCB);
	return 0;
}
