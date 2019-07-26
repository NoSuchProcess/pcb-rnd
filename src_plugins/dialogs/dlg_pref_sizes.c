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
#include "conf.h"
#include "conf_core.h"

/* Actual board size to dialog box */
static void pref_sizes_brd2dlg(pref_ctx_t *ctx)
{
	if (ctx->sizes.lock)
		return;
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wwidth, crd, PCB->hidlib.size_x);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wheight, crd, PCB->hidlib.size_y);
}

/* Dialog box to actual board size */
static void pref_sizes_dlg2brd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;

	ctx->sizes.lock++;
	if ((PCB->hidlib.size_x != ctx->dlg[ctx->sizes.wwidth].val.crd) || (PCB->hidlib.size_y != ctx->dlg[ctx->sizes.wheight].val.crd))
		pcb_board_resize(ctx->dlg[ctx->sizes.wwidth].val.crd, ctx->dlg[ctx->sizes.wheight].val.crd);
	ctx->sizes.lock--;
}

static pref_confitem_t drc_sizes[] = {
	{"Minimum copper spacing", "design/bloat", 0, NULL},
	{"Minimum copper width", "design/min_wid", 0, NULL},
	{"Minimum copper overlap", "design/shrink", 0, NULL},
	{"Minimum silk width", "design/min_slk", 0, NULL},
	{"Minimum drill diameter", "design/min_drill", 0, NULL},
	{"Minimum annular ring", "design/min_ring", 0, NULL},
	{NULL, NULL, 0}
};

static void pref_sizes_drc_dlg2conf(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	pcb_pref_dlg2conf_table(ctx, drc_sizes, attr);
}

static void pref_isle_brd2dlg(conf_native_t *cfg, int arr_idx)
{
	if ((pref_ctx.sizes.lock) || (!pref_ctx.active))
		return;
	PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.sizes.wisle, dbl, conf_core.design.poly_isle_area / 1000000.0);
}

static void pref_isle_dlg2brd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	double v = ctx->dlg[ctx->sizes.wisle].val.dbl * 1000000.0;

	ctx->sizes.lock++;
	conf_setf(ctx->role, "design/poly_isle_area", -1, "%f", v);
	ctx->sizes.lock--;
}

void pcb_dlg_pref_sizes_close(pref_ctx_t *ctx)
{
	pcb_pref_conflist_remove(ctx, drc_sizes);
}

void pcb_dlg_pref_sizes_create(pref_ctx_t *ctx)
{
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Board size");
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Width=");
			PCB_DAD_COORD(ctx->dlg, "");
				ctx->sizes.wwidth = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, PCB_MM_TO_COORD(1), PCB_MAX_COORD);
				PCB_DAD_DEFAULT_NUM(ctx->dlg, PCB->hidlib.size_x);
				PCB_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
			PCB_DAD_LABEL(ctx->dlg, "Height=");
			PCB_DAD_COORD(ctx->dlg, "");
				ctx->sizes.wheight = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, PCB_MM_TO_COORD(1), PCB_MAX_COORD);
				PCB_DAD_DEFAULT_NUM(ctx->dlg, PCB->hidlib.size_y);
				PCB_DAD_CHANGE_CB(ctx->dlg, pref_sizes_dlg2brd);
		PCB_DAD_END(ctx->dlg);

	PCB_DAD_END(ctx->dlg);
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "DRC sizes");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
			pcb_pref_create_conftable(ctx, drc_sizes, pref_sizes_drc_dlg2conf);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "misc sizes");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
			PCB_DAD_LABEL(ctx->dlg, "polygon isle minimum size\n[square um]");
			PCB_DAD_REAL(ctx->dlg, "");
				ctx->sizes.wisle = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, 0, PCB_MAX_COORD);
				ctx->dlg[ctx->sizes.wisle].val.dbl = (conf_core.design.poly_isle_area / 1000000.0);
				PCB_DAD_CHANGE_CB(ctx->dlg, pref_isle_dlg2brd);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}

void pcb_dlg_pref_sizes_init(pref_ctx_t *ctx)
{
	static conf_hid_callbacks_t cbs_isle;
	conf_native_t *cn = conf_get_field("design/poly_isle_area");

	if (cn != NULL) {
		memset(&cbs_isle, 0, sizeof(conf_hid_callbacks_t));
		cbs_isle.val_change_post = pref_isle_brd2dlg;
		conf_hid_set_cb(cn, pref_hid, &cbs_isle);
	}
}
