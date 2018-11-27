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

#include "config.h"

#include "actions.h"
#include "drc.h"
#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_drc_list_t drc;
	int alloced, active;

	int wlist, wcount;

} drc_ctx_t;

drc_ctx_t drc_ctx;

static void drc_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	drc_ctx_t *ctx = caller_data;


	PCB_DAD_FREE(ctx->dlg);
	if (ctx->alloced)
		free(ctx);
}

void drc2dlg(drc_ctx_t *ctx)
{
	char tmp[32];
	
	sprintf(tmp, "%d", pcb_drc_list_length(&ctx->drc));
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wcount, str_value, tmp);
}

static void pcb_dlg_drc(drc_ctx_t *ctx, const char *title)
{
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Number of violations:");
			PCB_DAD_LABEL(ctx->dlg, "n/a");
			ctx->wcount = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL | PCB_HATF_FRAME | PCB_HATF_EXPFILL);
			ctx->wlist = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Copy entry");
			PCB_DAD_BUTTON(ctx->dlg, "Paste entry before");
			PCB_DAD_BUTTON(ctx->dlg, "Paste entry after");
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Refresh");
			PCB_DAD_BUTTON(ctx->dlg, "Remove entry");
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Close");
		PCB_DAD_END(ctx->dlg);

	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW(ctx->dlg, title, "", ctx, pcb_false, drc_close_cb);

	ctx->active = 1;
}

const char pcb_acts_DRC[] = "DRC()\n";
const char pcb_acth_DRC[] = "Execute drc checks";
fgw_error_t pcb_act_DRC(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	static drc_ctx_t ctx = {0};

	if (!ctx.active)
		pcb_dlg_drc(&ctx, "DRC violations");

	pcb_drc_all(&ctx.drc);
	drc2dlg(&ctx);

	return 0;
}
