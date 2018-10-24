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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Preferences dialog, general */

#include "dlg_pref.h"

static pref_confitem_t perf_topwin[] = {
	{"Alternate window layout to allow smaller horizontal size", "plugins/hid_gtk/compact_horizontal", 0, NULL},
	{"Alternate window layout to allow smaller vertical size", "plugins/hid_gtk/compact_vertical", 0, NULL},
	{NULL, NULL, 0}
};

static pref_confitem_t perf_backup[] = {
	{"Save unsaved layout to PCB.%i.save at exit", "editor/save_in_tmp", 0, NULL},
	{"Seconds between auto backups\n(set to zero to disable auto backups)", "rc/backup_interval", 0, NULL},
	{NULL, NULL, 0}
};

static void pref_general_dlg2conf(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	pcb_pref_dlg2conf_table(ctx, perf_topwin, attr);
	pcb_pref_dlg2conf_table(ctx, perf_backup, attr);
}

void pcb_dlg_pref_general_close(pref_ctx_t *ctx)
{
	pcb_pref_conflist_remove(ctx, perf_topwin);
	pcb_pref_conflist_remove(ctx, perf_backup);
}

void pcb_dlg_pref_general_create(pref_ctx_t *ctx)
{
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Top window layout");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
			pcb_pref_create_conftable(ctx, perf_topwin, pref_general_dlg2conf);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Backup");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
			pcb_pref_create_conftable(ctx, perf_backup, pref_general_dlg2conf);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}
