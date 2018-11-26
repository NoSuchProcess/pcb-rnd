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
	pcb_drc_violation_t drc;
	int alloced, active;
} drc_ctx_t;

drc_ctx_t drc_ctx;

static void drc_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	drc_ctx_t *ctx = caller_data;


	PCB_DAD_FREE(ctx->dlg);
	if (ctx->alloced)
		free(ctx);
}

static void pcb_dlg_drc(drc_ctx_t *ctx, const char *title)
{
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_LABEL(ctx->dlg, "drc");
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

	return 0;
}
