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

/* Preferences dialog, color tab */

#include <librnd/core/conf.h>
#include <librnd/plugins/lib_hid_common/dlg_pref.h>
#include "conf_core.h"

typedef struct {
	int *wgen, *wlayer;
	int ngen;
} pref_color_t;

#undef DEF_TABDATA
#define DEF_TABDATA pref_color_t *tabdata = PREF_TABDATA(ctx)

static void pref_color_brd2dlg(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;
	rnd_conf_native_t *nat;
	int n;

	if (tabdata->wlayer != NULL) {
		nat = rnd_conf_get_field("appearance/color/layer");
		for (n = 0; n < nat->used; n++)
			RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wlayer[n], clr, nat->val.color[n]);
	}

	for(n = 0; n < tabdata->ngen; n++) {
		int w = tabdata->wgen[n];
		const char *path = ctx->dlg[w].user_data;
		nat = rnd_conf_get_field(path);
		if (nat != NULL)
			RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, w, clr, nat->val.color[0]);
	}
}


void pcb_dlg_pref_color_open(pref_ctx_t *ctx, rnd_design_t *dsg, const char *tabdatareq)
{
	pref_color_brd2dlg(ctx, dsg);
}

void pcb_dlg_pref_color_close(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;
	int n;

	for(n = 0; n < tabdata->ngen; n++) {
		int w = tabdata->wgen[n];
		free(ctx->dlg[w].user_data);
	}

	free(tabdata->wgen);
	free(tabdata->wlayer);
}

static void pref_color_gen_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	pref_ctx_t *ctx = caller_data;
	const char *path = attr->user_data;

	if (rnd_pref_dlg2conf_pre(hl, ctx) == NULL)
		return;

	rnd_conf_setf(ctx->role, path, -1, "%s", attr->val.clr.str);

	rnd_pref_dlg2conf_post(hl, ctx);

	rnd_gui->invalidate_all(rnd_gui);
}

static void pref_color_layer_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	int idx = (int *)attr->user_data - tabdata->wlayer;

	if (rnd_pref_dlg2conf_pre(hl, ctx) == NULL)
		return;

	rnd_conf_setf(ctx->role, "appearance/color/layer", idx, "%s", attr->val.clr.str);

	rnd_pref_dlg2conf_post(hl, ctx);
}


void pcb_dlg_pref_color_create(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	static const char *tabs[] = { "Generic colors", "Default layer colors", NULL };
	rnd_conf_native_t *nat;
	htsp_entry_t *e;
	int n, pl, w;
	const char *path_prefix = "appearance/color";
	DEF_TABDATA;


	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_BEGIN_TABBED(ctx->dlg, tabs);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_LEFT_TAB);

		RND_DAD_BEGIN_VBOX(ctx->dlg); /* generic */
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			pl = strlen(path_prefix);

			tabdata->ngen = 0;
			rnd_conf_fields_foreach(e) {
				nat = e->value;
				if ((strncmp(e->key, path_prefix, pl) == 0) && (nat->type == RND_CFN_COLOR) && (nat->array_size == 1))
					tabdata->ngen++;
			}
			tabdata->wgen = calloc(sizeof(int), tabdata->ngen);

			n = 0;
			rnd_conf_fields_foreach(e) {
				nat = e->value;
				if ((strncmp(e->key, path_prefix, pl) == 0) && (nat->type == RND_CFN_COLOR) && (nat->array_size == 1)) {
					RND_DAD_BEGIN_HBOX(ctx->dlg);
						RND_DAD_BEGIN_VBOX(ctx->dlg);
							RND_DAD_COLOR(ctx->dlg);
								tabdata->wgen[n] = w = RND_DAD_CURRENT(ctx->dlg);
								ctx->dlg[w].user_data = rnd_strdup(e->key);
								RND_DAD_CHANGE_CB(ctx->dlg, pref_color_gen_cb);
						RND_DAD_END(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, nat->description);
						n++;
					RND_DAD_END(ctx->dlg);
				}
			}
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_VBOX(ctx->dlg); /* layer */
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			nat = rnd_conf_get_field("appearance/color/layer");
			if (nat->type == RND_CFN_COLOR) {
				RND_DAD_LABEL(ctx->dlg, "NOTE: these colors are used only\nwhen creating new layers.");
				tabdata->wlayer = calloc(sizeof(int), nat->used);
				RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
				for (n = 0; n < nat->used; n++) {
					char tmp[32];
						RND_DAD_COLOR(ctx->dlg);
							tabdata->wlayer[n] = w = RND_DAD_CURRENT(ctx->dlg);
							ctx->dlg[w].user_data = &tabdata->wlayer[n];
							RND_DAD_CHANGE_CB(ctx->dlg, pref_color_layer_cb);
						sprintf(tmp, "Layer %d", n);
						RND_DAD_LABEL(ctx->dlg, tmp);
				}
				RND_DAD_END(ctx->dlg);
			}
			else {
				tabdata->wlayer = NULL;
				RND_DAD_LABEL(ctx->dlg, "Broken internal configuration:\nno layer colors");
			}
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);
}

static const rnd_pref_tab_hook_t pref_color = {
	"Colors", RND_PREFTAB_AUTO_FREE_DATA | RND_PREFTAB_NEEDS_ROLE,
	pcb_dlg_pref_color_open, pcb_dlg_pref_color_close,
	pcb_dlg_pref_color_create,
	pref_color_brd2dlg, pref_color_brd2dlg  /* board change, meta change */
};

static void pcb_dlg_pref_color_init(pref_ctx_t *ctx, int tab)
{
	PREF_INIT(ctx, &pref_color);
	PREF_TABDATA(ctx) = calloc(sizeof(pref_color_t), 1);
}
#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_color_init
