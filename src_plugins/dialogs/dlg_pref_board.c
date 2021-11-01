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

/* Preferences dialog, board tab */

#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "board.h"
#include "conf_core.h"

#define RND_EMPTY(a)           ((a) ? (a) : "")

/* Actual board meta to dialog box */
static void pref_board_brd2dlg(pref_ctx_t *ctx)
{
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wname, str, RND_EMPTY(PCB->hidlib.name));
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wthermscale, dbl, PCB->ThermScale);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wtype, str, (PCB->is_footprint ? "footprint" : "PCB board"));
}

/* Dialog box to actual board meta */
static void pref_board_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int changed = 0;
	const char *newname, *oldname;
	double newtherm;
	pref_ctx_t *ctx = caller_data;

	newname = RND_EMPTY(ctx->dlg[ctx->board.wname].val.str);
	oldname = RND_EMPTY(PCB->hidlib.name);
	if (strcmp(oldname, newname) != 0) {
		free(PCB->hidlib.name);
		PCB->hidlib.name = rnd_strdup(newname);
		changed = 1;
	}

	newtherm = ctx->dlg[ctx->board.wthermscale].val.dbl;
	if (PCB->ThermScale != newtherm) {
		PCB->ThermScale = newtherm;
		changed = 1;
	}

	if (changed) {
		PCB->Changed = 1;
		rnd_event(&PCB->hidlib, RND_EVENT_BOARD_META_CHANGED, NULL); /* always generate the event to make sure visible changes are flushed */
	}
}

static void pref_board_edit_attr(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, "Propedit", "board", NULL);
}

void pcb_dlg_pref_board_create(pref_ctx_t *ctx)
{
	RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
		RND_DAD_LABEL(ctx->dlg, "Board name");
		RND_DAD_STRING(ctx->dlg);
			ctx->board.wname = RND_DAD_CURRENT(ctx->dlg);
			ctx->dlg[ctx->board.wname].val.str = rnd_strdup(RND_EMPTY(PCB->hidlib.name));
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Thermal scale");
		RND_DAD_REAL(ctx->dlg);
			ctx->board.wthermscale = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_MINMAX(ctx->dlg, 0.01, 100);
			ctx->dlg[ctx->board.wthermscale].val.dbl = PCB->ThermScale;
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Type");
		RND_DAD_LABEL(ctx->dlg, "");
			ctx->board.wtype = RND_DAD_CURRENT(ctx->dlg);
			ctx->dlg[ctx->board.wtype].name = rnd_strdup((PCB->is_footprint ? "footprint" : "PCB board"));
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Board attributes");
		RND_DAD_BUTTON(ctx->dlg, "Edit...");
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_edit_attr);
		RND_DAD_END(ctx->dlg);
}

static const Rnd_pref_tab_hook_t pref_board = {
	"Board meta", Rnd_PREFTAB_AUTO_FREE_DATA,
	NULL, NULL,
	pcb_dlg_pref_board_create,
	pref_board_brd2dlg, pref_board_brd2dlg  /* board change, meta change */
};

static void pcb_dlg_pref_board_init(pref_ctx_t *ctx, int tab)
{
	PREF_INIT(ctx, &pref_board);
	rnd_trace("INIT pref board tab %d\n", tab);
}
#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_board_init
