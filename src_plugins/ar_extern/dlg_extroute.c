/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process: dialog box
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hid_dad.h>

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int whatever;
} ar_ctx_t;

static ar_ctx_t ar_ctx;

static void ar_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	ar_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(ar_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void extroute_gui(pcb_board_t *pcb)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (ar_ctx.active)
		return; /* do not open another */

	printf("GUI!\n");
	extroute_query_conf(pcb);

	RND_DAD_BEGIN_VBOX(ar_ctx.dlg);
		RND_DAD_LABEL(ar_ctx.dlg, "woops\n");

		
		RND_DAD_BEGIN_HBOX(ar_ctx.dlg);
			RND_DAD_BUTTON(ar_ctx.dlg, "Route");
			RND_DAD_BUTTON(ar_ctx.dlg, "Re-route");
			RND_DAD_BUTTON(ar_ctx.dlg, "Save conf");
			RND_DAD_BUTTON(ar_ctx.dlg, "Load conf");
			RND_DAD_BEGIN_VBOX(ar_ctx.dlg);
				RND_DAD_COMPFLAG(ar_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ar_ctx.dlg);
			RND_DAD_BUTTON_CLOSES(ar_ctx.dlg, clbtn);
		RND_DAD_END(ar_ctx.dlg);
	RND_DAD_END(ar_ctx.dlg);

	/* set up the context */
	ar_ctx.active = 1;

	RND_DAD_NEW("external_autorouter", ar_ctx.dlg, "External autorouter", &ar_ctx, rnd_false, ar_close_cb);
}
