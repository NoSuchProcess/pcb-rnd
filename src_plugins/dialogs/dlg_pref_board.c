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

/* Preferences dialog, board tab */

#include "dlg_pref.h"
#include "conf.h"
#include "conf_core.h"

#define PCB_EMPTY(a)           ((a) ? (a) : "")

/* Actual board meta to dialog box */
static void pref_board_brd2dlg(pref_ctx_t *ctx)
{
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wname, str_value, PCB_EMPTY(PCB->Name));
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wthermscale, real_value, PCB->ThermScale);
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->board.wtype, str_value, (PCB->is_footprint ? "footprint" : "PCB board"));
}

/* Dialog box to actual board meta */
static void pref_board_dlg2brd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int changed = 0;
	const char *newname, *oldname;
	double newtherm;
	pref_ctx_t *ctx = caller_data;

	newname = PCB_EMPTY(ctx->dlg[ctx->board.wname].default_val.str_value);
	oldname = PCB_EMPTY(PCB->Name);
	if (strcmp(oldname, newname) != 0) {
		free(PCB->Name);
		PCB->Name = pcb_strdup(newname);
		changed = 1;
	}

	newtherm = ctx->dlg[ctx->board.wthermscale].default_val.real_value;
	if (PCB->ThermScale != newtherm) {
		PCB->ThermScale = newtherm;
		changed = 1;
	}

	if (changed)
		pcb_board_set_changed_flag(1);
}

static void pref_board_edit_attr(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("Propedit", "board", NULL);
}


void pcb_dlg_pref_board_close(pref_ctx_t *ctx)
{
	pcb_pref_conflist_remove(ctx, drc_sizes);
}

void pcb_dlg_pref_board_create(pref_ctx_t *ctx)
{
	PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
		PCB_DAD_LABEL(ctx->dlg, "Board name");
		PCB_DAD_STRING(ctx->dlg);
			ctx->board.wname = PCB_DAD_CURRENT(ctx->dlg);
			ctx->dlg[ctx->board.wname].default_val.str_value = pcb_strdup(PCB_EMPTY(PCB->Name));
			PCB_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		PCB_DAD_LABEL(ctx->dlg, "Thermal scale");
		PCB_DAD_REAL(ctx->dlg, "");
			ctx->board.wthermscale = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_MINMAX(ctx->dlg, 0.01, 100);
			ctx->dlg[ctx->board.wthermscale].default_val.real_value = PCB->ThermScale;
			PCB_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		PCB_DAD_LABEL(ctx->dlg, "Type");
		PCB_DAD_LABEL(ctx->dlg, "");
			ctx->board.wtype = PCB_DAD_CURRENT(ctx->dlg);
			ctx->dlg[ctx->board.wtype].name = (PCB->is_footprint ? "footprint" : "PCB board");
			PCB_DAD_CHANGE_CB(ctx->dlg, pref_board_dlg2brd);
		PCB_DAD_LABEL(ctx->dlg, "Board attributes");
		PCB_DAD_BUTTON(ctx->dlg, "Edit...");
			PCB_DAD_CHANGE_CB(ctx->dlg, pref_board_edit_attr);
		PCB_DAD_END(ctx->dlg);
}
