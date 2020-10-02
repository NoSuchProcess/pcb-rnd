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

/* Preferences dialog, sizes tab */

#include "board.h"
#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include "drc.h"

/* Actual board size to dialog box */
static void pref_sizes_brd2dlg(pref_ctx_t *ctx)
{
	if (ctx->sizes.lock)
		return;
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wwidth, crd, PCB->hidlib.size_x);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wheight, crd, PCB->hidlib.size_y);
}

/* Dialog box to actual board size */
static void pref_sizes_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;

	ctx->sizes.lock++;
	if ((PCB->hidlib.size_x != ctx->dlg[ctx->sizes.wwidth].val.crd) || (PCB->hidlib.size_y != ctx->dlg[ctx->sizes.wheight].val.crd))
		pcb_board_resize(ctx->dlg[ctx->sizes.wwidth].val.crd, ctx->dlg[ctx->sizes.wheight].val.crd, 0);
	ctx->sizes.lock--;
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

	if (pref_dlg2conf_pre(ctx) == NULL)
		return;

	pcb_pref_dlg2conf_table(ctx, limit_sizes, attr);

	pref_dlg2conf_post(ctx);
}

static void pref_isle_brd2dlg(rnd_conf_native_t *cfg, int arr_idx)
{
	if ((pref_ctx.sizes.lock) || (!pref_ctx.active))
		return;
	RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.sizes.wisle, dbl, conf_core.design.poly_isle_area / 1000000.0);
}

static void pref_isle_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	double v = ctx->dlg[ctx->sizes.wisle].val.dbl * 1000000.0;

	ctx->sizes.lock++;
	rnd_conf_setf(ctx->role, "design/poly_isle_area", -1, "%f", v);
	ctx->sizes.lock--;
}

void pcb_dlg_pref_sizes_close(pref_ctx_t *ctx)
{
	pcb_pref_conflist_remove(ctx, limit_sizes);
}

void pcb_dlg_pref_sizes_create(pref_ctx_t *ctx)
{
	pcb_drc_impl_t *di;
	int drcs;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Board size");
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Width=");
			RND_DAD_COORD(ctx->dlg);
				ctx->sizes.wwidth = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, RND_MM_TO_COORD(1), RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, PCB->hidlib.size_x);
				RND_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
			RND_DAD_LABEL(ctx->dlg, "Height=");
			RND_DAD_COORD(ctx->dlg);
				ctx->sizes.wheight = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, RND_MM_TO_COORD(1), RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, PCB->hidlib.size_y);
				RND_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
		RND_DAD_LABEL(ctx->dlg, "Limiting sizes (not DRC)");
		RND_DAD_LABEL(ctx->dlg, "(Used when the code needs to figure the absolute global smallest value)");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			pcb_pref_create_conftable(ctx, limit_sizes, pref_sizes_limit_dlg2conf);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			RND_DAD_LABEL(ctx->dlg, "polygon isle minimum size\n[square um]");
			RND_DAD_REAL(ctx->dlg);
				ctx->sizes.wisle = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, 0, RND_MAX_COORD);
				ctx->dlg[ctx->sizes.wisle].val.dbl = (conf_core.design.poly_isle_area / 1000000.0);
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

void pcb_dlg_pref_sizes_init(pref_ctx_t *ctx)
{
	static rnd_conf_hid_callbacks_t cbs_isle;
	rnd_conf_native_t *cn = rnd_conf_get_field("design/poly_isle_area");

	if (cn != NULL) {
		memset(&cbs_isle, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_isle.val_change_post = pref_isle_brd2dlg;
		rnd_conf_hid_set_cb(cn, pref_hid, &cbs_isle);
	}
}
