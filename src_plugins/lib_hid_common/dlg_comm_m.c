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

/* Common dialogs: simple, modal dialogs and info bars.
   Even the core will run some of these, through a dispatcher (e.g.
   action). */

#define RND_DAD_CFG_NOLABEL 1

#include <librnd/config.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hidlib.h>
#include "xpm.h"
#include "dlg_comm_m.h"
#include "lib_hid_common.h"

static const char nope[] = "Do not use.";

static void prompt_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_dad_retovr_t **ro = attr->user_data;
	rnd_hid_dad_close(hid_ctx, *ro, 0);
}

fgw_error_t pcb_act_gui_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *label, *default_str = "";
	const char *pcb_acts_gui_PromptFor =  nope;
	char *title = NULL;
	int ws, ft = 0;
	rnd_hid_dad_buttons_t clbtn[] = {{"ok", 0}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	RND_ACT_CONVARG(1, FGW_STR, gui_PromptFor, label = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, gui_PromptFor, default_str = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, gui_PromptFor, title = argv[3].val.str);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_LABEL(dlg, label);
		RND_DAD_STRING(dlg);
			ws = RND_DAD_CURRENT(dlg);
			dlg[ws].val.str = rnd_strdup(default_str == NULL ? "" : default_str);
			RND_DAD_ENTER_CB(dlg, prompt_enter_cb);
			dlg[ws].user_data = &dlg_ret_override;
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	if (title == NULL) {
		title = rnd_concat(rnd_app_package, " user input", NULL);
		ft = 1;
	}
	RND_DAD_NEW("prompt_for", dlg, title, NULL, rnd_true, NULL);
	if (ft)
		free(title);

	if (RND_DAD_RUN(dlg) != 0) {
		RND_DAD_FREE(dlg);
		return -1;
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = rnd_strdup(dlg[ws].val.str);
	RND_DAD_FREE(dlg);

	return 0;
}


fgw_error_t pcb_act_gui_MessageBox(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *icon, *title, *label, *txt;
	const char *pcb_acts_gui_MessageBox = nope;
	const char **xpm;
	int n, ret;

	RND_DAD_DECL(dlg);

	RND_ACT_CONVARG(1, FGW_STR, gui_MessageBox, icon = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, gui_MessageBox, title = argv[2].val.str);
	RND_ACT_CONVARG(3, FGW_STR, gui_MessageBox, label = argv[3].val.str);

	RND_DAD_BEGIN_VBOX(dlg);
		/* icon and label */
		RND_DAD_BEGIN_HBOX(dlg);
			xpm = pcp_dlg_xpm_by_name(icon);
			if (xpm != NULL)
				RND_DAD_PICTURE(dlg, xpm);
			RND_DAD_LABEL(dlg, label);
		RND_DAD_END(dlg);

		/* close buttons */
		RND_DAD_BEGIN_HBOX(dlg);
			RND_DAD_BEGIN_HBOX(dlg);
				RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
			RND_DAD_END(dlg);
			for(n = 4; n < argc; n+=2) {
				RND_ACT_CONVARG(n+0, FGW_STR, gui_MessageBox, txt = argv[n+0].val.str);
				RND_ACT_CONVARG(n+1, FGW_INT, gui_MessageBox, ret = argv[n+1].val.nat_int);
				RND_DAD_BUTTON_CLOSE(dlg, txt, ret);
			}
		RND_DAD_END(dlg);

	RND_DAD_END(dlg);

	res->type = FGW_INT;
	RND_DAD_AUTORUN("message", dlg, title, NULL, res->val.nat_int);
	RND_DAD_FREE(dlg);

	return 0;
}

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wclr, wr, wg, wb;
	rnd_color_t clr;
} clrpick_t;

static int clamp(int c)
{
	if (c < 0)
		return 0;
	if (c > 255)
		return 255;
	return c;
}

static void color_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_attr_val_t val;
	clrpick_t *ctx = caller_data;

	rnd_color_load_int(&ctx->clr, clamp(ctx->dlg[ctx->wr].val.lng), clamp(ctx->dlg[ctx->wg].val.lng), clamp(ctx->dlg[ctx->wb].val.lng), 255);
	val.clr = ctx->clr;
	rnd_gui->attr_dlg_set_value(hid_ctx, ctx->wclr, &val);
}

fgw_error_t pcb_act_gui_FallbackColorPick(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sclr;
	const char *pcb_acts_gui_PromptFor =  nope;
	rnd_hid_dad_buttons_t clbtn[] = {{"ok", 0}, {"cancel", 1}, {NULL, 0}};
	clrpick_t ctx;
	rnd_hid_attr_val_t val;

	RND_ACT_CONVARG(1, FGW_STR, gui_PromptFor, sclr = argv[1].val.str);

	memset(&ctx, 0, sizeof(ctx));
	if (rnd_color_load_str(&ctx.clr, sclr) != 0)
		return -1;

	RND_DAD_BEGIN_VBOX(ctx.dlg);
		RND_DAD_COLOR(ctx.dlg);
			ctx.wclr = RND_DAD_CURRENT(ctx.dlg);
			RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_CLR_STATIC);
		RND_DAD_BEGIN_TABLE(ctx.dlg, 2);
			RND_DAD_LABEL(ctx.dlg, "red");
			RND_DAD_INTEGER(ctx.dlg);
				RND_DAD_MINMAX(ctx.dlg, 0, 255);
				RND_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wr = RND_DAD_CURRENT(ctx.dlg);
			RND_DAD_LABEL(ctx.dlg, "green");
			RND_DAD_INTEGER(ctx.dlg);
				RND_DAD_MINMAX(ctx.dlg, 0, 255);
				RND_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wg = RND_DAD_CURRENT(ctx.dlg);
			RND_DAD_LABEL(ctx.dlg, "blue");
			RND_DAD_INTEGER(ctx.dlg);
				RND_DAD_MINMAX(ctx.dlg, 0, 255);
				RND_DAD_CHANGE_CB(ctx.dlg, color_change_cb);
				ctx.wb = RND_DAD_CURRENT(ctx.dlg);
			RND_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
		RND_DAD_END(ctx.dlg);
	RND_DAD_END(ctx.dlg);

	RND_DAD_NEW("fallback_color_pick", ctx.dlg, "Change color", &ctx, rnd_true, NULL);


	val.lng = ctx.clr.r;
	rnd_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wr, &val);
	val.lng = ctx.clr.g;
	rnd_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wg, &val);
	val.lng = ctx.clr.b;
	rnd_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wb, &val);
	val.clr = ctx.clr;
	rnd_gui->attr_dlg_set_value(ctx.dlg_hid_ctx, ctx.wclr, &val);

	if (RND_DAD_RUN(ctx.dlg) != 0) {
		RND_DAD_FREE(ctx.dlg);
		return -1;
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = rnd_strdup_printf("#%02x%02x%02x", ctx.clr.r, ctx.clr.g, ctx.clr.b);
	RND_DAD_FREE(ctx.dlg);
	return 0;
}

fgw_error_t pcb_act_gui_MayOverwriteFile(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hidlib_t *hidlib;
	const char *fn;
	const char *pcb_acts_gui_MayOverwriteFile = nope;
	const char **xpm;
	int multi, wdontask;
	rnd_hid_dad_buttons_t clbtn_s[] = {{"yes", 1}, {"no", 0}, {NULL, 0}};
	rnd_hid_dad_buttons_t clbtn_m[] = {{"yes", 1}, {"yes to all", 2}, {"no", 0}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	if (!RND_HAVE_GUI_ATTR_DLG) {
		RND_ACT_IRES(0); /* no gui means auto-yes (for batch) */
		return 2;
	}

	if (dialogs_conf.plugins.dialogs.file_overwrite_dialog.dont_ask) {
		RND_ACT_IRES(0); /* "don't ask" means yes to all */
		return 2;
	}

	hidlib = RND_ACT_HIDLIB;
	RND_ACT_CONVARG(1, FGW_STR, gui_MayOverwriteFile, fn = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_INT, gui_MayOverwriteFile, multi = argv[2].val.nat_int);

	RND_DAD_BEGIN_VBOX(dlg);
		/* icon and label */
		RND_DAD_BEGIN_HBOX(dlg);
			xpm = pcp_dlg_xpm_by_name("warning");
			if (xpm != NULL)
				RND_DAD_PICTURE(dlg, xpm);

			RND_DAD_BEGIN_VBOX(dlg);
				RND_DAD_BEGIN_HBOX(dlg);
					RND_DAD_LABEL(dlg, "File ");
					RND_DAD_LABEL(dlg, fn);
					RND_DAD_LABEL(dlg, " already exists.");
				RND_DAD_END(dlg);

				RND_DAD_LABEL(dlg, "Do you want to overwrite that file?");

				RND_DAD_BEGIN_HBOX(dlg);
					RND_DAD_BOOL(dlg);
					wdontask = RND_DAD_CURRENT(dlg);
					RND_DAD_LABEL(dlg, "do not ask, always overwrite");
					RND_DAD_HELP(dlg, "saved in your user config under plugins/dialogs/file_overwrite_dialog/dont_ask");
				RND_DAD_END(dlg);
			RND_DAD_END(dlg);
		RND_DAD_END(dlg);

		if (multi)
			RND_DAD_BUTTON_CLOSES(dlg, clbtn_m);
		else
			RND_DAD_BUTTON_CLOSES(dlg, clbtn_s);
	RND_DAD_END(dlg);

	res->type = FGW_INT;
	RND_DAD_AUTORUN("MayOverwriteFile", dlg, "File overwrite question", NULL, res->val.nat_int);
	if (dlg[wdontask].val.lng) {
		rnd_conf_set(RND_CFR_USER, "plugins/dialogs/file_overwrite_dialog/dont_ask", 0, "1", RND_POL_OVERWRITE);
		if (rnd_conf_isdirty(RND_CFR_USER))
			rnd_conf_save_file(hidlib, NULL, NULL, RND_CFR_USER, NULL);
	}
	RND_DAD_FREE(dlg);

	return 0;
}
