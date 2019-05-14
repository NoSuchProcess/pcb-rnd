/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "board.h"
#include "hid.h"
#include "hid_dad.h"
#include "hid_init.h"
#include "dlg_library.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} library_ctx_t;

library_ctx_t library_ctx;

static void library_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;

	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(library_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_library(void)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (library_ctx.active)
		return; /* do not open another */


	PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
		PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BUTTON_CLOSES(library_ctx.dlg, clbtn);
	PCB_DAD_END(library_ctx.dlg);

	/* set up the context */
	library_ctx.active = 1;

	PCB_DAD_NEW("library", library_ctx.dlg, "pcb-rnd Footprint Library", &library_ctx, pcb_false, library_close_cb);
}

const char pcb_acts_LibraryDialog[] = "libraryDialog()\n";
const char pcb_acth_LibraryDialog[] = "Open the library dialog.";
fgw_error_t pcb_act_LibraryDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_library();
	return 0;
}
