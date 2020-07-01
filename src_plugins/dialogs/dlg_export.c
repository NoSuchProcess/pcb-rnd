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

#include "config.h"
#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include "layer_vis.h"
#include "event.h"
#include <librnd/core/hid.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_unit.h>
#include <librnd/core/hid_init.h>
#include "dlg_export.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	int tabs, len;

	/* per exporter data */
	rnd_hid_t **hid;
	const char **tab_name;
	int **exp_attr;           /* widget IDs of the attributes holding the actual data; outer array is indexed by exporter, inner array is by exporter option index from 0*/
	int *button;              /* widget ID of the export button for a specific exporter */
	int *numo;                /* number of exporter options */
	rnd_export_opt_t **ea;    /* original export option arrays */
} export_ctx_t;

export_ctx_t export_ctx;

static rnd_hid_attr_val_t *get_results(export_ctx_t *export_ctx, int id)
{
	rnd_hid_attr_val_t *r;
	int *exp_attr, n, numo = export_ctx->numo[id];

	r = malloc(sizeof(rnd_hid_attr_val_t) * numo);

	exp_attr = export_ctx->exp_attr[id];
	for(n = 0; n < numo; n++) {
		int src = exp_attr[n];
		memcpy(&r[n], &(export_ctx->dlg[src].val), sizeof(rnd_hid_attr_val_t));
	}
	return r;
}

static void export_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	export_ctx_t *export_ctx = caller_data;
	int h, wid;
	int have_gui, currly = PCB_CURRLID(PCB);
	int save_l_ons[PCB_MAX_LAYER], save_g_ons[PCB_MAX_LAYERGRP];
	
	have_gui = (rnd_gui != NULL) && rnd_gui->gui;
	if (have_gui) {
		pcb_hid_save_and_show_layer_ons(save_l_ons);
		pcb_hid_save_and_show_layergrp_ons(save_g_ons);
	}

	wid = attr - export_ctx->dlg;
	for(h = 0; h < export_ctx->len; h++) {
		if (export_ctx->button[h] == wid) {
			rnd_hid_attr_val_t *results = get_results(export_ctx, h);
			rnd_event(&PCB->hidlib, RND_EVENT_EXPORT_SESSION_BEGIN, NULL);
			export_ctx->hid[h]->do_export(export_ctx->hid[h], results);
			rnd_event(&PCB->hidlib, RND_EVENT_EXPORT_SESSION_END, NULL);
			free(results);
			rnd_message(RND_MSG_INFO, "Export done using exporter: %s\n", export_ctx->hid[h]->name);
			goto done;
		}
	}

	rnd_message(RND_MSG_ERROR, "Internal error: can not find which exporter to call\n");
	done:;

	if (have_gui) {
		pcb_hid_restore_layer_ons(save_l_ons);
		pcb_hid_restore_layergrp_ons(save_g_ons);
		pcb_layervis_change_group_vis(&PCB->hidlib, currly, 1, 1);
		rnd_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
}

/* copy back the attribute values from the DAD dialog to exporter dialog so
   that values are preserved */
static void copy_attrs_back(export_ctx_t *ctx)
{
	int n, i;

	for(n = 0; n < ctx->len; n++) {
		int *exp_attr = export_ctx.exp_attr[n];
		int numo = export_ctx.numo[n];
		rnd_export_opt_t *opts = export_ctx.ea[n];

		for(i = 0; i < numo; i++)
			memcpy(&opts[i].default_val, &ctx->dlg[exp_attr[i]].val, sizeof(rnd_hid_attr_val_t));
	}
}

static void export_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	export_ctx_t *ctx = caller_data;
	int n;

	copy_attrs_back(ctx);

	RND_DAD_FREE(ctx->dlg);
	free(ctx->hid);
	free(ctx->tab_name);
	for(n = 0; n < export_ctx.len; n++)
		free(ctx->exp_attr[n]);
	free(ctx->exp_attr);
	free(ctx->button);
	free(ctx->numo);
	free(ctx->ea);
	memset(ctx, 0, sizeof(export_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_export(const char *title, int exporters, int printers)
{
	rnd_hid_t **hids;
	int n, i, *exp_attr;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (export_ctx.active)
		return; /* do not open another */

	hids = rnd_hid_enumerate();
	for(n = 0, export_ctx.len = 0; hids[n] != NULL; n++) {
		if (((exporters && hids[n]->exporter) || (printers && hids[n]->printer)) && (!hids[n]->hide_from_gui))
			export_ctx.len++;
	}

	if (export_ctx.len == 0) {
		rnd_message(RND_MSG_ERROR, "Can not export: there are no export plugins available\n");
		return;
	}

	export_ctx.tab_name = malloc(sizeof(char *) * (export_ctx.len+1));
	export_ctx.hid = malloc(sizeof(rnd_hid_t *) * (export_ctx.len));
	export_ctx.exp_attr = malloc(sizeof(int *) * (export_ctx.len));
	export_ctx.button = malloc(sizeof(int) * (export_ctx.len));
	export_ctx.numo = malloc(sizeof(int) * (export_ctx.len));
	export_ctx.ea = malloc(sizeof(rnd_hid_attribute_t *) * (export_ctx.len));

	for(i = n = 0; hids[n] != NULL; n++) {
		if (((exporters && hids[n]->exporter) || (printers && hids[n]->printer)) && (!hids[n]->hide_from_gui)) {
			export_ctx.tab_name[i] = hids[n]->name;
			export_ctx.hid[i] = hids[n];
			i++;
		}
	}

	export_ctx.tab_name[i] = NULL;

	RND_DAD_BEGIN_VBOX(export_ctx.dlg);
	RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABBED(export_ctx.dlg, export_ctx.tab_name);
			RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_LEFT_TAB|RND_HATF_EXPFILL);
			export_ctx.tabs = RND_DAD_CURRENT(export_ctx.dlg);
			for(n = 0; n < export_ctx.len; n++) {
				int numo;
				rnd_export_opt_t *opts = export_ctx.hid[n]->get_export_options(export_ctx.hid[n], &numo);
				export_ctx.numo[n] = numo;
				export_ctx.ea[n] = opts;
				if (numo < 1) {
					RND_DAD_LABEL(export_ctx.dlg, "Exporter unavailable for direct export");
					continue;
				}
				RND_DAD_BEGIN_VBOX(export_ctx.dlg);
					if (numo > 12)
						RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_SCROLL);
					export_ctx.exp_attr[n] = exp_attr = malloc(sizeof(int) * numo);
					for(i = 0; i < numo; i++) {
						RND_DAD_BEGIN_HBOX(export_ctx.dlg)
							
							switch(opts[i].type) {
								case RND_HATT_COORD:
									RND_DAD_COORD(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, opts[i].default_val.crd);
									break;
								case RND_HATT_INTEGER:
									RND_DAD_INTEGER(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, opts[i].default_val.lng);
									break;
								case RND_HATT_REAL:
									RND_DAD_REAL(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, opts[i].default_val.dbl);
									break;
								case RND_HATT_UNIT:
									RND_DAD_UNIT(export_ctx.dlg, RND_UNIT_METRIC | RND_UNIT_IMPERIAL);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, opts[i].default_val.lng);
									break;
								default:
									if (RND_HATT_IS_COMPOSITE(opts[i].type)) {
										rnd_hid_export_opt_func_t fnc = opts[i].default_val.func;
										if (fnc != NULL)
											fnc(RND_HIDEOF_DAD, &export_ctx, &opts[i]);
									}
									else
										RND_DAD_DUP_EXPOPT(export_ctx.dlg, &(opts[i]));
							}
							exp_attr[i] = RND_DAD_CURRENT(export_ctx.dlg);
							if (opts[i].name != NULL)
								RND_DAD_LABEL(export_ctx.dlg, opts[i].name);
						RND_DAD_END(export_ctx.dlg);
					}
					RND_DAD_LABEL(export_ctx.dlg, " "); /* ugly way of inserting some vertical spacing */
					RND_DAD_BEGIN_HBOX(export_ctx.dlg)
						RND_DAD_LABEL(export_ctx.dlg, "Apply attributes and export: ");
						RND_DAD_BUTTON(export_ctx.dlg, "Export!");
							export_ctx.button[n] = RND_DAD_CURRENT(export_ctx.dlg);
							RND_DAD_CHANGE_CB(export_ctx.dlg, export_cb);
					RND_DAD_END(export_ctx.dlg);
				RND_DAD_END(export_ctx.dlg);
			}
		RND_DAD_END(export_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(export_ctx.dlg, clbtn);
	RND_DAD_END(export_ctx.dlg);

	/* set up the context */
	export_ctx.active = 1;

	RND_DAD_NEW("export", export_ctx.dlg, title, &export_ctx, rnd_false, export_close_cb);
}

const char pcb_acts_ExportGUI[] = "ExportGUI()\n";
const char pcb_acth_ExportGUI[] = "Open the export dialog.";
fgw_error_t pcb_act_ExportGUI(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_export("Export to file", 1, 0);
	return 0;
}

const char pcb_acts_PrintGUI[] = "PrintGUI()\n";
const char pcb_acth_PrintGUI[] = "Open the print dialog.";
fgw_error_t pcb_act_PrintGUI(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_export("Print", 0, 1);
	return 0;
}
