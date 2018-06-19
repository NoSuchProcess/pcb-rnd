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

#include "build_run.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} about_ctx_t;

about_ctx_t about_ctx;

static void about_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	about_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(about_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}


static void pcb_dlg_about(void)
{
	if (about_ctx.active)
		return;

	PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
	PCB_DAD_LABEL(about_ctx.dlg, "About...");
	PCB_DAD_END(about_ctx.dlg);

	/* set up the context */
	about_ctx.active = 1;

	/* this is the modal version - please consider using the non-modal version */
	PCB_DAD_NEW(about_ctx.dlg, "EDIT THIS", "EDIT THIS", &about_ctx, pcb_false, about_close_cb);
}

static const char pcb_acts_About[] = "About()\n";
static const char pcb_acth_About[] = "Present the about box";
static int pcb_act_About(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_dlg_about();
	return 0;
	PCB_OLD_ACT_END;
}
