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

/* Preferences dialog, window geometry tab */

#include "dlg_pref.h"
#include "conf.h"
#include "conf_core.h"

static void pref_win_brd2dlg(pref_ctx_t *ctx)
{
}

void pcb_dlg_pref_win_open(pref_ctx_t *ctx)
{
	pref_win_brd2dlg(ctx);
}

void pcb_dlg_pref_win_close(pref_ctx_t *ctx)
{
}


void pcb_dlg_pref_win_create(pref_ctx_t *ctx)
{
	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Load window geometry and enable window placement:");
		PCB_DAD_BOOL(ctx->dlg, "");
			PCB_DAD_HELP(ctx->dlg, "When enabled, pcb-rnd will load window geometry from config files\nand try to resize and place windows accordingly.\nSizes can be saved once (golden arrangement)\nor at every exit (retrain last setup),\nsee below.");
	PCB_DAD_END(ctx->dlg);
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_LABEL(ctx->dlg, "Save window geometry to...");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the design (board) file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the project file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in the user config");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
				PCB_DAD_LABEL(ctx->dlg, "before close:");
				PCB_DAD_BOOL(ctx->dlg, "");
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "... in a custom file");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "now");
			PCB_DAD_END(ctx->dlg);

		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}
