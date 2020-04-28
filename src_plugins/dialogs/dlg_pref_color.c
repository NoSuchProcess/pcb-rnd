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

/* Preferences dialog, color tab */

#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"

static void pref_color_brd2dlg(pref_ctx_t *ctx)
{
	conf_native_t *nat;
	int n;

	if (ctx->color.wlayer != NULL) {
		nat = pcb_conf_get_field("appearance/color/layer");
		for (n = 0; n < nat->used; n++)
			PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->color.wlayer[n], clr, nat->val.color[n]);
	}

	for(n = 0; n < ctx->color.ngen; n++) {
		int w = ctx->color.wgen[n];
		const char *path = ctx->dlg[w].user_data;
		nat = pcb_conf_get_field(path);
		if (nat != NULL)
			PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, w, clr, nat->val.color[0]);
	}
}


void pcb_dlg_pref_color_open(pref_ctx_t *ctx)
{
	pref_color_brd2dlg(ctx);
}

void pcb_dlg_pref_color_close(pref_ctx_t *ctx)
{
	int n;

	for(n = 0; n < ctx->color.ngen; n++) {
		int w = ctx->color.wgen[n];
		free(ctx->dlg[w].user_data);
	}

	free(ctx->color.wgen);
	free(ctx->color.wlayer);
}

static void pref_color_gen_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	const char *path = attr->user_data;
	pcb_conf_setf(ctx->role, path, -1, "%s", attr->val.clr.str);
	pcb_gui->invalidate_all(pcb_gui);
}

static void pref_color_layer_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	int idx = (int *)attr->user_data - ctx->color.wlayer;
	pcb_conf_setf(ctx->role, "appearance/color/layer", idx, "%s", attr->val.clr.str);
}


void pcb_dlg_pref_color_create(pref_ctx_t *ctx)
{
	static const char *tabs[] = { "Generic colors", "Default layer colors", NULL };
	conf_native_t *nat;
	htsp_entry_t *e;
	int n, pl, w;
	const char *path_prefix = "appearance/color";


	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_TABBED(ctx->dlg, tabs);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_LEFT_TAB);

		PCB_DAD_BEGIN_VBOX(ctx->dlg); /* generic */
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			pl = strlen(path_prefix);

			ctx->color.ngen = 0;
			conf_fields_foreach(e) {
				nat = e->value;
				if ((strncmp(e->key, path_prefix, pl) == 0) && (nat->type == CFN_COLOR) && (nat->array_size == 1))
					ctx->color.ngen++;
			}
			ctx->color.wgen = calloc(sizeof(int), ctx->color.ngen);

			n = 0;
			conf_fields_foreach(e) {
				nat = e->value;
				if ((strncmp(e->key, path_prefix, pl) == 0) && (nat->type == CFN_COLOR) && (nat->array_size == 1)) {
					PCB_DAD_BEGIN_HBOX(ctx->dlg);
						PCB_DAD_BEGIN_VBOX(ctx->dlg);
							PCB_DAD_COLOR(ctx->dlg);
								ctx->color.wgen[n] = w = PCB_DAD_CURRENT(ctx->dlg);
								ctx->dlg[w].user_data = rnd_strdup(e->key);
								PCB_DAD_CHANGE_CB(ctx->dlg, pref_color_gen_cb);
						PCB_DAD_END(ctx->dlg);
						PCB_DAD_LABEL(ctx->dlg, nat->description);
						n++;
					PCB_DAD_END(ctx->dlg);
				}
			}
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_VBOX(ctx->dlg); /* layer */
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			nat = pcb_conf_get_field("appearance/color/layer");
			if (nat->type == CFN_COLOR) {
				PCB_DAD_LABEL(ctx->dlg, "NOTE: these colors are used only\nwhen creating new layers.");
				ctx->color.wlayer = calloc(sizeof(int), nat->used);
				PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
				for (n = 0; n < nat->used; n++) {
					char tmp[32];
						PCB_DAD_COLOR(ctx->dlg);
							ctx->color.wlayer[n] = w = PCB_DAD_CURRENT(ctx->dlg);
							ctx->dlg[w].user_data = &ctx->color.wlayer[n];
							PCB_DAD_CHANGE_CB(ctx->dlg, pref_color_layer_cb);
						sprintf(tmp, "Layer %d", n);
						PCB_DAD_LABEL(ctx->dlg, tmp);
				}
				PCB_DAD_END(ctx->dlg);
			}
			else {
				ctx->color.wlayer = NULL;
				PCB_DAD_LABEL(ctx->dlg, "Broken internal configuration:\nno layer colors");
			}
		PCB_DAD_END(ctx->dlg);

	PCB_DAD_END(ctx->dlg);
}
