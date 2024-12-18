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

/* Preferences dialog, sizes tab */

#include "board.h"
#include <librnd/core/conf.h>
#include <librnd/plugins/lib_hid_common/dlg_pref.h>
#include "conf_core.h"
#include "drc.h"

typedef struct {
	int wwidth, wheight;
	int wisle;
	int lock; /* a change in on the dialog box causes a change on the board but this shouldn't in turn casue a changein the dialog */
} pref_sizes_t;

#undef DEF_TABDATA
#define DEF_TABDATA pref_sizes_t *tabdata = PREF_TABDATA(ctx)

/* Actual board size to dialog box */
static void pref_sizes_brd2dlg(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;

	if (tabdata->lock)
		return;
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wwidth, crd, rnd_dwg_get_size_x(dsg));
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wheight, crd, rnd_dwg_get_size_y(dsg));
}

/* Dialog box to actual board size */
static void pref_sizes_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;

	tabdata->lock++;
	if ((rnd_dwg_get_size_x(&PCB->hidlib) != ctx->dlg[tabdata->wwidth].val.crd) || (rnd_dwg_get_size_y(&PCB->hidlib) != ctx->dlg[tabdata->wheight].val.crd)) {
		rnd_coord_t x1 = PCB->hidlib.dwg.X1, y1 = PCB->hidlib.dwg.Y1;
		rnd_coord_t x2 = PCB->hidlib.dwg.X1 + ctx->dlg[tabdata->wwidth].val.crd, y2 = PCB->hidlib.dwg.Y1 + ctx->dlg[tabdata->wheight].val.crd;
		pcb_board_resize(x1, y1, x2, y2, 1);
		pcb_undo_inc_serial();
	}
	tabdata->lock--;
}

static void drc_rules_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, attr->user_data, NULL);
}

static pref_confitem_t limit_sizes[] = {
	{"Minimum copper spacing", "design/bloat", 0, NULL},
	{"Minimum copper width", "design/min_wid", 0, NULL},
	{"Minimum silk width", "design/min_slk", 0, NULL},
	{NULL, NULL, 0}
};

static void pref_sizes_limit_dlg2conf(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;

	if (rnd_pref_dlg2conf_pre(&PCB->hidlib, ctx) == NULL)
		return;

	rnd_pref_dlg2conf_table(ctx, limit_sizes, attr);

	rnd_pref_dlg2conf_post(&PCB->hidlib, ctx);
}

static void pref_isle_brd2dlg(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	pref_ctx_t *ctx = rnd_pref_get_ctx(&PCB->hidlib);
	DEF_TABDATA;
	
	if ((tabdata->lock) || (!ctx->active))
		return;
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wisle, dbl, conf_core.design.poly_isle_area / 1000000.0);
}

static void pref_isle_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	double v = ctx->dlg[tabdata->wisle].val.dbl * 1000000.0;

	tabdata->lock++;
	rnd_conf_setf(ctx->role, "design/poly_isle_area", -1, "%f", v);
	tabdata->lock--;
}

void pcb_dlg_pref_sizes_close(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	rnd_pref_conflist_remove(ctx, limit_sizes);
}

void pcb_dlg_pref_sizes_create(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;
	pcb_drc_impl_t *di;
	int drcs;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Board size");
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Width=");
			RND_DAD_COORD(ctx->dlg);
				tabdata->wwidth = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, RND_MM_TO_COORD(1), RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, rnd_dwg_get_size_x(&PCB->hidlib));
				RND_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
			RND_DAD_LABEL(ctx->dlg, "Height=");
			RND_DAD_COORD(ctx->dlg);
				tabdata->wheight = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, RND_MM_TO_COORD(1), RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, rnd_dwg_get_size_y(&PCB->hidlib));
				RND_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Limiting sizes (not DRC)");
		RND_DAD_LABEL(ctx->dlg, "(Used when the code needs to figure the absolute global smallest value)");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_pref_create_conftable(ctx, limit_sizes, pref_sizes_limit_dlg2conf);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			RND_DAD_LABEL(ctx->dlg, "polygon isle minimum size\nfor MorphPolygon(); [square um]");
			RND_DAD_REAL(ctx->dlg);
				tabdata->wisle = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, 0, RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, (conf_core.design.poly_isle_area / 1000000.0));
				RND_DAD_CHANGE_CB(ctx->dlg, pref_isle_dlg2brd);
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Configure DRC rules:");
		for(di = gdl_first(&pcb_drc_impls), drcs = 0; di != NULL; di = di->link.next, drcs++) {
			char *ra = rnd_strdup(di->list_rules_action); /* need to strdup it just in case the plugin is unloaded while the preferences dialog is open */
			vtp0_append(&ctx->auto_free, ra);
			RND_DAD_BUTTON(ctx->dlg, di->name);
				RND_DAD_HELP(ctx->dlg, di->desc);
				RND_DAD_SET_ATTR_FIELD(ctx->dlg, user_data, ra);
				RND_DAD_CHANGE_CB(ctx->dlg, drc_rules_cb);
		}
		if (drcs == 0)
			RND_DAD_LABEL(ctx->dlg, "ERROR: no DRC plugins available");
	RND_DAD_END(ctx->dlg);
}

static const rnd_pref_tab_hook_t pref_sizes = {
	"Sizes & DRC", RND_PREFTAB_AUTO_FREE_DATA | RND_PREFTAB_NEEDS_ROLE,
	NULL, pcb_dlg_pref_sizes_close,
	pcb_dlg_pref_sizes_create,
	pref_sizes_brd2dlg, pref_sizes_brd2dlg  /* board change, meta change */
};

static void pcb_dlg_pref_sizes_init(pref_ctx_t *ctx, int tab)
{
	static rnd_conf_hid_callbacks_t cbs_isle;
	rnd_conf_native_t *cn = rnd_conf_get_field("design/poly_isle_area");

	PREF_INIT(ctx, &pref_sizes);
	PREF_TABDATA(ctx) = calloc(sizeof(pref_sizes_t), 1);

	if (cn != NULL) {
		memset(&cbs_isle, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_isle.val_change_post = pref_isle_brd2dlg;
		rnd_conf_hid_set_cb(cn, pref_hid, &cbs_isle);
	}
}

#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_sizes_init
