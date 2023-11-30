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

#include <librnd/core/conf.h>
#include <librnd/core/actions.h>
#include <librnd/plugins/lib_hid_common/dlg_pref.h>
#include "board.h"
#include "data.h"
#include "plug_io.h"
#include "event.h"
#include "undo.h"
#include "conf_core.h"

typedef struct {
	int wname, wthermscale, wtype, wloader;
} pref_board_t;

#define RND_EMPTY(a)           ((a) ? (a) : "")

#undef DEF_TABDATA
#define DEF_TABDATA pref_board_t *tabdata = PREF_TABDATA(ctx)

/* Actual board meta to dialog box */
static void pref_board_brd2dlg(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	pcb_board_t *pcb = (pcb_board_t *)dsg;
	DEF_TABDATA;

	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wname, str, RND_EMPTY(dsg->name));
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wthermscale, dbl, pcb->ThermScale);
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wtype, str, (pcb->is_footprint ? "footprint" : "PCB board"));
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, tabdata->wloader, str, pcb->Data->loader == NULL ? "unknown" : pcb->Data->loader->description);
}

/* Dialog box to actual board meta */
static void pref_board_dlg2brd(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	pcb_board_t *pcb = (pcb_board_t *)hl;
	int changed = 0;
	const char *newname, *oldname;
	double newtherm;
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;

	newname = RND_EMPTY(ctx->dlg[tabdata->wname].val.str);
	oldname = RND_EMPTY(hl->name);
	if (strcmp(oldname, newname) != 0) {
		free(hl->name);
		hl->name = rnd_strdup(newname);
		changed = 1;
		rnd_event(hl, RND_EVENT_DESIGN_FN_CHANGED, NULL); /* forces title update even if board ->Changed doesn't change */

	}

	newtherm = ctx->dlg[tabdata->wthermscale].val.dbl;
	if (pcb->ThermScale != newtherm) {
		pcb_board_chg_thermal_scale(newtherm, 1);
		pcb_undo_inc_serial();
		changed = 1;
	}

	if (changed) {
		pcb->Changed = 1;
		rnd_event(hl, RND_EVENT_DESIGN_META_CHANGED, NULL); /* always generate the event to make sure visible changes are flushed */
	}
}

static void pref_board_edit_attr(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	rnd_actionva(hl, "Propedit", "board", NULL);
}

void pcb_dlg_pref_board_create(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	pcb_board_t *pcb = (pcb_board_t *)dsg;
	DEF_TABDATA;

	RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
		RND_DAD_LABEL(ctx->dlg, "Board name");
		RND_DAD_STRING(ctx->dlg);
			tabdata->wname = RND_DAD_CURRENT(ctx->dlg);
			ctx->dlg[tabdata->wname].val.str = rnd_strdup(RND_EMPTY(dsg->name));
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Thermal scale");
		RND_DAD_REAL(ctx->dlg);
			tabdata->wthermscale = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_MINMAX(ctx->dlg, 0.01, 1.5);
			ctx->dlg[tabdata->wthermscale].val.dbl = -1;
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Type");
		RND_DAD_LABEL(ctx->dlg, "");
			tabdata->wtype = RND_DAD_CURRENT(ctx->dlg);
			ctx->dlg[tabdata->wtype].name = rnd_strdup((pcb->is_footprint ? "footprint" : "PCB board"));
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Loaded with");
		RND_DAD_LABEL(ctx->dlg, "");
			tabdata->wloader = RND_DAD_CURRENT(ctx->dlg);
			ctx->dlg[tabdata->wtype].name = rnd_strdup(pcb->Data->loader == NULL ? "unknown" : pcb->Data->loader->description);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		RND_DAD_LABEL(ctx->dlg, "Board attributes");
		RND_DAD_BUTTON(ctx->dlg, "Edit...");
			RND_DAD_CHANGE_CB(ctx->dlg, pref_board_edit_attr);

		RND_DAD_END(ctx->dlg);
}

static void pref_board_open(pref_ctx_t *ctx, rnd_design_t *dsg, const char *tabdatareq)
{
	pref_board_brd2dlg(ctx, dsg);
}

static const rnd_pref_tab_hook_t pref_board = {
	"Board meta", RND_PREFTAB_AUTO_FREE_DATA,
	pref_board_open, NULL,
	pcb_dlg_pref_board_create,
	pref_board_brd2dlg, pref_board_brd2dlg  /* board change, meta change */
};

static void pcb_dlg_pref_board_init(pref_ctx_t *ctx, int tab)
{
	PREF_INIT(ctx, &pref_board);
	PREF_TABDATA(ctx) = calloc(sizeof(pref_board_t), 1);
}
#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_board_init
