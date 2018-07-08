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

#include "hid.h"
#include "hid_init.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	pcb_hid_t **hid;
	const char **tab_name;
	int *first_attr;
	int tabs, len;
} export_ctx_t;

export_ctx_t export_ctx;

static void export_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	export_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx->hid);
	free(ctx->tab_name);
	free(ctx->first_attr);
	memset(ctx, 0, sizeof(export_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_export(void)
{
	pcb_hid_t **hids;
	int n, i;

	if (export_ctx.active)
		return; /* do not open another */

	hids = pcb_hid_enumerate();
	for(n = 0, export_ctx.len = 0; hids[n] != NULL; n++) {
		if (!hids[n]->exporter)
			continue;
		export_ctx.len++;
	}

	export_ctx.tab_name = malloc(sizeof(char *) * (export_ctx.len+1));
	export_ctx.hid = malloc(sizeof(pcb_hid_t *) * (export_ctx.len));
	export_ctx.first_attr = malloc(sizeof(int) * (export_ctx.len));

	for(i = n = 0; hids[n] != NULL; n++) {
		if (!hids[n]->exporter)
			continue;
		export_ctx.tab_name[i] = hids[n]->name;
		export_ctx.hid[i] = hids[n];
		i++;
	}

	export_ctx.tab_name[i] = NULL;

	PCB_DAD_BEGIN_TABBED(export_ctx.dlg, export_ctx.tab_name);
		PCB_DAD_COMPFLAG(export_ctx.dlg, PCB_HATF_LEFT_TAB);
		export_ctx.tabs = PCB_DAD_CURRENT(export_ctx.dlg);
		for(n = 0; n < export_ctx.len; n++) {
			int numa;
			pcb_hid_attribute_t *attrs = export_ctx.hid[n]->get_export_options(&numa);
			PCB_DAD_BEGIN_VBOX(export_ctx.dlg);
				PCB_DAD_COMPFLAG(export_ctx.dlg, PCB_HATF_LABEL);
				for(i = 0; i < numa; i++) {
					if (i == 0)
						export_ctx.first_attr[n] = PCB_DAD_CURRENT(export_ctx.dlg);
					PCB_DAD_DUP_ATTR(export_ctx.dlg, attrs+i);
				}
			PCB_DAD_END(export_ctx.dlg);
		}
	PCB_DAD_END(export_ctx.dlg);

	/* set up the context */
	export_ctx.active = 1;

	PCB_DAD_NEW(export_ctx.dlg, "EDIT THIS", "EDIT THIS", &export_ctx, pcb_false, export_close_cb);
}

static const char pcb_acts_ExportGUI2[] = "ExportGUI2()\n";
static const char pcb_acth_ExportGUI2[] = "Open the export dialog.";
static fgw_error_t pcb_act_ExportGUI2(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_export();
	return 0;
}
