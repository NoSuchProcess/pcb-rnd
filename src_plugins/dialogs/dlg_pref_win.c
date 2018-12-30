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
#include "conf.h"
#include "conf_core.h"
#include "dialogs_conf.h"

extern const conf_dialogs_t conf_dialogs;
extern void pcb_wplc_save_to_role(conf_role_t role);
extern int pcb_wplc_save_to_file(const char *fn);

static void pref_win_brd2dlg(pref_ctx_t *ctx)
{
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wmaster, int_value, conf_core.editor.auto_place);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wboard, int_value, conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_design);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wproject, int_value, conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_project);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->win.wuser, int_value, conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_user);
}

void pcb_dlg_pref_win_open(pref_ctx_t *ctx)
{
	pref_win_brd2dlg(ctx);
}

void pcb_dlg_pref_win_close(pref_ctx_t *ctx)
{
}

static void pref_win_master_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	conf_setf(ctx->role, "editor/auto_place", -1, "%d", attr->default_val.int_value);
	pref_win_brd2dlg(ctx);
}

static void pref_win_board_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_design", -1, "%d", attr->default_val.int_value);
	pref_win_brd2dlg(ctx);
}

static void pref_win_project_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_project", -1, "%d", attr->default_val.int_value);
	pref_win_brd2dlg(ctx);
}

static void pref_win_user_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	conf_setf(ctx->role, "plugins/dialogs/auto_save_window_geometry/to_user", -1, "%d", attr->default_val.int_value);
	pref_win_brd2dlg(ctx);
}


static void pref_win_board_now_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(CFR_USER);
}

static void pref_win_project_now_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(CFR_USER);
}

static void pref_win_user_now_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_wplc_save_to_role(CFR_USER);
}

static void pref_win_file_now_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	char *fname;

	fname = pcb_gui->fileselect("Save window geometry to...",
		"Pick a file for saving window geometry to.\n",
		"win_geo.lht", ".lht", "wingeo", HID_FILESELECT_MAY_NOT_EXIST);

	if (fname == NULL)
		return;

	if (pcb_wplc_save_to_file(fname) != NULL)
		pcb_message(PCB_MSG_ERROR, "Error saving window geometry to '%s'\n", fname);
}


void pcb_dlg_pref_win_create(pref_ctx_t *ctx)
{
	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Load window geometry and enable window placement:");
		PCB_DAD_BOOL(ctx->dlg, "");
			PCB_DAD_HELP(ctx->dlg, "When enabled, pcb-rnd will load window geometry from config files\nand try to resize and place windows accordingly.\nSizes can be saved once (golden arrangement)\nor at every exit (retrain last setup),\nsee below.");
			ctx->win.wmaster = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_master_cb);
	PCB_DAD_END(ctx->dlg);
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Save window geometry to...");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the design (board) file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_board_now_cb);
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->win.wboard = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_board_cb);
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the project file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_project_now_cb);
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->win.wproject = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_project_cb);
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the user config");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_user_now_cb);
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->win.wuser = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_user_cb);
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in a custom file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_win_file_now_cb);
			PCB_DAD_END(ctx->dlg);

		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}
