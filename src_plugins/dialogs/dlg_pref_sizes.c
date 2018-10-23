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

/* Preferences dialog, sizes tab */

#include "dlg_pref_sizes.h"

/* Actual board size to dialog box */
static void pref_sizes_brd2dlg(pref_ctx_t *ctx)
{
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wwidth, coord_value, PCB->MaxWidth);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->sizes.wheight, coord_value, PCB->MaxHeight);
}

/* Dialog box to actual board size */
static void pref_sizes_dlg2brd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	pcb_board_resize(ctx->dlg[ctx->sizes.wwidth].default_val.coord_value, ctx->dlg[ctx->sizes.wheight].default_val.coord_value);
}


#define DLG_PREF_SIZES_DAD \
	PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); \
		PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_FRAME); \
		PCB_DAD_LABEL(pref_ctx.dlg, "Board size"); \
		PCB_DAD_BEGIN_HBOX(pref_ctx.dlg); \
			PCB_DAD_LABEL(pref_ctx.dlg, "Width="); \
			PCB_DAD_COORD(pref_ctx.dlg, ""); \
				pref_ctx.sizes.wwidth = PCB_DAD_CURRENT(pref_ctx.dlg); \
				PCB_DAD_MINMAX(pref_ctx.dlg, PCB_MM_TO_COORD(1), PCB_MAX_COORD); \
				PCB_DAD_DEFAULT(pref_ctx.dlg, PCB->MaxWidth); \
				PCB_DAD_CHANGE_CB(pref_ctx.dlg, pref_sizes_dlg2brd); \
			PCB_DAD_LABEL(pref_ctx.dlg, "Height="); \
			PCB_DAD_COORD(pref_ctx.dlg, ""); \
				pref_ctx.sizes.wheight = PCB_DAD_CURRENT(pref_ctx.dlg); \
				PCB_DAD_MINMAX(pref_ctx.dlg, PCB_MM_TO_COORD(1), PCB_MAX_COORD); \
				PCB_DAD_DEFAULT(pref_ctx.dlg, PCB->MaxHeight); \
				PCB_DAD_CHANGE_CB(pref_ctx.dlg, pref_sizes_dlg2brd); \
		PCB_DAD_END(pref_ctx.dlg); \
	PCB_DAD_END(pref_ctx.dlg);

