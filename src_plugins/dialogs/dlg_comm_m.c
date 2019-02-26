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

/* Common dialogs: simple, modal dialogs for all parts of the code, not just
   the GUI HIDs. Even the core will run some of these, through a dispatcher. */

#include "config.h"
#include "actions.h"
#include "hid_dad.h"
#include "xpm.h"

static const char nope[] = "Do not use.";

static void prompt_enter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_close(hid_ctx, attr->user_data, 0);
}

fgw_error_t pcb_act_gui_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *label, *default_str = "", *title = "pcb-rnd user input";
	const char *pcb_acts_gui_PromptFor =  nope;
	int ws;
	pcb_hid_dad_buttons_t clbtn[] = {{"ok", 0}, {NULL, 0}};
	PCB_DAD_DECL(dlg);

	PCB_ACT_CONVARG(1, FGW_STR, gui_PromptFor, label = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, gui_PromptFor, default_str = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, gui_PromptFor, title = argv[3].val.str);

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_LABEL(dlg, label);
		PCB_DAD_STRING(dlg);
			ws = PCB_DAD_CURRENT(dlg);
			dlg[ws].default_val.str_value = pcb_strdup(default_str);
			PCB_DAD_ENTER_CB(dlg, prompt_enter_cb);
			dlg[ws].user_data = &dlg_ret_override;
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW("prompt_for", dlg, title, NULL, pcb_true, NULL);
	if (PCB_DAD_RUN(dlg) != 0) {
		PCB_DAD_FREE(dlg);
		return -1;
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = pcb_strdup(dlg[ws].default_val.str_value);
	PCB_DAD_FREE(dlg);

	return 0;
}


fgw_error_t pcb_act_gui_MessageBox(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *icon, *title, *label, *txt;
	const char *pcb_acts_gui_MessageBox = nope;
	const char **xpm;
	int n, ret;

	PCB_DAD_DECL(dlg);

	PCB_ACT_CONVARG(1, FGW_STR, gui_MessageBox, icon = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, gui_MessageBox, title = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, gui_MessageBox, label = argv[3].val.str);

	PCB_DAD_BEGIN_VBOX(dlg);
		/* icon and label */
		PCB_DAD_BEGIN_HBOX(dlg);
			xpm = pcp_dlg_xpm_by_name(icon);
			if (xpm != NULL)
				PCB_DAD_PICTURE(dlg, xpm);
			PCB_DAD_LABEL(dlg, label);
		PCB_DAD_END(dlg);

		/* close buttons */
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BEGIN_HBOX(dlg);
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(dlg);
			for(n = 4; n < argc; n+=2) {
				PCB_ACT_CONVARG(n+0, FGW_STR, gui_MessageBox, txt = argv[n+0].val.str);
				PCB_ACT_CONVARG(n+1, FGW_INT, gui_MessageBox, ret = argv[n+1].val.nat_int);
				PCB_DAD_BUTTON_CLOSE(dlg, txt, ret);
			}
		PCB_DAD_END(dlg);

	PCB_DAD_END(dlg);

	res->type = FGW_INT;
	PCB_DAD_AUTORUN("message", dlg, title, NULL, res->val.nat_int);
	PCB_DAD_FREE(dlg);

	return 0;
}

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int wclr, wr, wg, wb;
	pcb_color_t clr;
} clrpick_t;

static int clamp(int c)
{
	if (c < 0)
		return 0;
	if (c > 255)
		return 255;
	return c;
}

static void color_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_attr_val_t val;
	clrpick_t *ctx = caller_data;

	pcb_color_load_int(&ctx->clr, clamp(ctx->dlg[ctx->wr].default_val.int_value), clamp(ctx->dlg[ctx->wg].default_val.int_value), clamp(ctx->dlg[ctx->wb].default_val.int_value), 255);
	val.clr_value = ctx->clr;
	pcb_gui->attr_dlg_set_value(hid_ctx, ctx->wclr, &val);
}

fgw_error_t pcb_act_gui_FallbackColorPick(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sclr;
	const char *pcb_acts_gui_PromptFor =  nope;
	pcb_hid_dad_buttons_t clbtn[] = {{"ok", 0}, {"cancel", 1}, {NULL, 0}};
	clrpick_t ctx;
	pcb_hid_attr_val_t val;

	PCB_ACT_CONVARG(1, FGW_STR, gui_PromptFor, sclr = argv[1].val.str);

	memset(&ctx, 0, sizeof(ctx));
	if (pcb_color_load_str(&ctx.clr, sclr) != 0)
		return -1;

	PCB_DAD_BEGIN_VBOX(ctx.dlg);
		PCB_DAD_COLOR(ctx.dlg);
			ctx.wclr = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_COMPFLAG(ctx.dlg, PCB_HATF_CLR_STATIC);
		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_LABEL(ctx.dlg, "red");
			PCB_DAD_INTEGER(ctx.dlg, "");
				PCB_DAD_MINMAX(ctx.dlg, 0, 255);
				PCB_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wr = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, "green");
			PCB_DAD_INTEGER(ctx.dlg, "");
				PCB_DAD_MINMAX(ctx.dlg, 0, 255);
				PCB_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wg = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_LABEL(ctx.dlg, "blue");
			PCB_DAD_INTEGER(ctx.dlg, "");
				PCB_DAD_MINMAX(ctx.dlg, 0, 255);
				PCB_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wb = PCB_DAD_CURRENT(ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		PCB_DAD_END(ctx.dlg);
	PCB_DAD_END(ctx.dlg);

	PCB_DAD_NEW("fallback_color_pick", ctx.dlg, "Change color", &ctx, pcb_true, NULL);


	val.int_value = ctx.clr.r;
	pcb_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wr, &val);
	val.int_value = ctx.clr.g;
	pcb_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wg, &val);
	val.int_value = ctx.clr.b;
	pcb_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wb, &val);
	val.clr_value = ctx.clr;
	pcb_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wclr, &val);

	if (PCB_DAD_RUN(ctx.dlg) != 0) {
		PCB_DAD_FREE(ctx.dlg);
		return -1;
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = pcb_strdup_printf("#%02x%02x%02x", ctx.clr.r, ctx.clr.g, ctx.clr.b);
	PCB_DAD_FREE(ctx.dlg);
	return 0;
}

