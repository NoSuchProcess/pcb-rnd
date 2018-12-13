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
#include "conf.h"
#include "conf_core.h"

static void pref_color_brd2dlg(pref_ctx_t *ctx)
{
}

static void pref_color_dlg2brd(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
}

void pcb_dlg_pref_color_close(pref_ctx_t *ctx)
{
	free(ctx->color.wgen);
	free(ctx->color.wlayer);
}

void pcb_dlg_pref_color_create(pref_ctx_t *ctx)
{
	static const char *tabs[] = { "Generic colors", "Default layer colors", NULL };
	ctx->color.wgen = calloc(sizeof(int), 32);
	ctx->color.wlayer = calloc(sizeof(int), 32);

	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_TABBED(ctx->dlg, tabs);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_LEFT_TAB);

		PCB_DAD_BEGIN_VBOX(ctx->dlg); /* generic */
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_VBOX(ctx->dlg); /* layer */
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
		PCB_DAD_END(ctx->dlg);

	PCB_DAD_END(ctx->dlg);
}
