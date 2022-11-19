/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2021 Tibor 'Igor2' Palinkas
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

/* Preferences dialog, general tab */

#include <librnd/plugins/lib_hid_common/dlg_pref.h>
#include "board.h"

static pref_confitem_t perf_topwin[] = {
	{"Alternate window layout to\nallow smaller size", "appearance/compact", 0, NULL},
	{NULL, NULL, 0}
};

static pref_confitem_t perf_backup[] = {
	{"Save unsaved layout to\nPCB.%i.save at exit", "editor/save_in_tmp", 0, NULL},
	{"Seconds between auto backups\n(set to zero to disable auto backups)", "rc/backup_interval", 0, NULL},
	{NULL, NULL, 0}
};

static pref_confitem_t perf_cli[] = {
	{"Number of commands to\nremember in the\nhistory list", "plugins/hid_gtk/history_size", 0, NULL},
	{NULL, NULL, 0}
};

static void pref_general_dlg2conf(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_hidlib(hid_ctx);
	pref_ctx_t *ctx = caller_data;

	if (rnd_pref_dlg2conf_pre(hl, ctx) == NULL)
		return;

	rnd_pref_dlg2conf_table(ctx, perf_topwin, attr);
	rnd_pref_dlg2conf_table(ctx, perf_backup, attr);
	rnd_pref_dlg2conf_table(ctx, perf_cli, attr);

	rnd_pref_dlg2conf_post(hl, ctx);
}

void pcb_dlg_pref_general_close(pref_ctx_t *ctx)
{
	rnd_pref_conflist_remove(ctx, perf_topwin);
	rnd_pref_conflist_remove(ctx, perf_backup);
	rnd_pref_conflist_remove(ctx, perf_cli);
}

void pcb_dlg_pref_general_create(pref_ctx_t *ctx)
{
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Top window layout");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_pref_create_conftable(ctx, perf_topwin, pref_general_dlg2conf);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Backup");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_pref_create_conftable(ctx, perf_backup, pref_general_dlg2conf);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Command line entry");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_pref_create_conftable(ctx, perf_cli, pref_general_dlg2conf);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);
}

static const rnd_pref_tab_hook_t pref_general = {
	"General", RND_PREFTAB_AUTO_FREE_DATA | RND_PREFTAB_NEEDS_ROLE,
	NULL, pcb_dlg_pref_general_close,
	pcb_dlg_pref_general_create,
	NULL, NULL
};

static void pcb_dlg_pref_general_init(pref_ctx_t *ctx, int tab)
{
	PREF_INIT(ctx, &pref_general);
}
#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_general_init
