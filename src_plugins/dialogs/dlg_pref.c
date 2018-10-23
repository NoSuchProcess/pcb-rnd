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

/* The preferences dialog, invoked by the Preferences() action */

#include <genvector/gds_char.h>
#include "build_run.h"
#include "pcb-printf.h"
#include "dlg_pref.h"

#include "dlg_pref_sizes.c"

pref_ctx_t pref_ctx;
static const char *pref_cookie = "preferences dialog";

void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_conflist_t *item)
{
	PCB_DAD_LABEL(ctx->dlg, item->label);
	PCB_DAD_COORD(ctx->dlg, "");
		item->wid = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_MINMAX(ctx->dlg, PCB_MM_TO_COORD(1), PCB_MAX_COORD);
/*		PCB_DAD_DEFAULT(pref_ctx.dlg, PCB->MaxHeight);*/
/*		PCB_DAD_CHANGE_CB(pref_ctx.dlg, pref_sizes_dlg2brd);*/
}

void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_conflist_t *list)
{
	pref_conflist_t *c;
	for(c = list; c->confpath != NULL; c++)
		pcb_pref_create_conf_item(ctx, c);
}


static void pref_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pref_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}


static void pcb_dlg_pref(void)
{
	const char *tabs[] = { "General", "Window", "Sizes & DRC",  "Library", "Layers", "Colors", "Config tree", NULL };

	if (pref_ctx.active)
		return;

	PCB_DAD_BEGIN_TABBED(pref_ctx.dlg, tabs);
		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* General */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Window */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Sizes & DRC */
			DLG_PREF_SIZES_DAD;
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Library */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Layers */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Colors */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Config tree */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

	PCB_DAD_END(pref_ctx.dlg);

	/* set up the context */
	pref_ctx.active = 1;

	PCB_DAD_NEW(pref_ctx.dlg, "pcb-rnd preferences", "", &pref_ctx, pcb_false, pref_close_cb);
}

static void pref_ev_board_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
}

static void pref_ev_board_meta_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
}

static void dlg_pref_init(void)
{
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pref_ev_board_changed, &pref_ctx, pref_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, pref_ev_board_meta_changed, &pref_ctx, pref_cookie);
}

static void dlg_pref_uninit(void)
{
	pcb_event_unbind_allcookie(pref_cookie);
}

static const char pcb_acts_Preferences[] = "Preferences()\n";
static const char pcb_acth_Preferences[] = "Present the preferences dialog";
static fgw_error_t pcb_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_pref();
	PCB_ACT_IRES(0);
	return 0;
}
