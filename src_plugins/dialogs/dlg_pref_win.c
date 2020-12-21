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

/* Preferences dialog, window geometry tab */

#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/plugins/lib_hid_common/dialogs_conf.h>
#include <librnd/plugins/lib_hid_common/place.h>

extern conf_dialogs_t dialogs_conf;

static void pref_win_brd2dlg(pref_ctx_t *ctx)
{
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wmaster, lng, rnd_conf.editor.auto_place);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wboard, lng, dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_design);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wproject, lng, dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_project);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wuser, lng, dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_user);
}

void pcb_dlg_pref_win_open(pref_ctx_t *ctx)
{
	pref_win_brd2dlg(ctx);
}

void pcb_dlg_pref_win_close(pref_ctx_t *ctx)
{
}

static void pref_win_master_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	rnd_conf_setf(ctx->role, "editor/auto_place", -1, "%d", attr->val.lng);
	pref_win_brd2dlg(ctx);
}

static void pref_win_board_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	rnd_conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_design", -1, "%d", attr->val.lng);
	pref_win_brd2dlg(ctx);
}

static void pref_win_project_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;

	if (pref_dlg2conf_pre(ctx) == NULL)
		return;

	rnd_conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_project", -1, "%d", attr->val.lng);

	pref_dlg2conf_post(ctx);

	pref_win_brd2dlg(ctx);
}

static void pref_win_user_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	rnd_conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_user", -1, "%d", attr->val.lng);
	pref_win_brd2dlg(ctx);
}


static void pref_win_board_now_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(rnd_gui->get_dad_hidlib(hid_ctx), RND_CFR_USER);
}

static void pref_win_project_now_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(rnd_gui->get_dad_hidlib(hid_ctx), RND_CFR_USER);
}

static void pref_win_user_now_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(rnd_gui->get_dad_hidlib(hid_ctx), RND_CFR_USER);
}

static void pref_win_file_now_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fname;

	fname = rnd_gui->fileselect(rnd_gui, "Save window geometry to...",
		"Pick a file for saving window geometry to.\n",
		"win_geo.lht", ".lht", NULL, "wingeo", RND_HID_FSD_MAY_NOT_EXIST, NULL);

	if (fname == NULL)
		return;

	if (pcb_wplc_save_to_file(rnd_gui->get_dad_hidlib(hid_ctx), fname) != 0)
		rnd_message(RND_MSG_ERROR, "Error saving window geometry to '%s'\n", fname);
}


void pcb_dlg_pref_win_create(pref_ctx_t *ctx)
{
	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Load window geometry and enable window placement:");
		RND_DAD_BOOL(ctx->dlg);
			RND_DAD_HELP(ctx->dlg, "When enabled, pcb-rnd will load window geometry from config files\nand try to resize and place windows accordingly.\nSizes can be saved once (golden arrangement)\nor at every exit (retrain last setup),\nsee below.");
			ctx->win.wmaster = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_win_master_cb);
	RND_DAD_END(ctx->dlg);
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Save window geometry to...");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);

			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "... in the design (board) file");
			RND_DAD_END(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "now");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_board_now_cb);
				RND_DAD_LABEL(ctx->dlg, "before close:");
				RND_DAD_BOOL(ctx->dlg);
					ctx->win.wboard = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_board_cb);
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "... in the project file");
			RND_DAD_END(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "now");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_project_now_cb);
				RND_DAD_LABEL(ctx->dlg, "before close:");
				RND_DAD_BOOL(ctx->dlg);
					ctx->win.wproject = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_project_cb);
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "... in the user config");
			RND_DAD_END(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "now");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_user_now_cb);
				RND_DAD_LABEL(ctx->dlg, "before close:");
				RND_DAD_BOOL(ctx->dlg);
					ctx->win.wuser = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_user_cb);
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "... in a custom file");
			RND_DAD_END(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "now");
					RND_DAD_CHANGE_CB(ctx->dlg, pref_win_file_now_cb);
			RND_DAD_END(ctx->dlg);

		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);
}
