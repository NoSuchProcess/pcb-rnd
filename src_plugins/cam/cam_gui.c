/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin: GUI dialog
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
} cam_dlg_t;

cam_dlg_t foo_ctx;

static void cam_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	cam_dlg_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(cam_dlg_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int cam_gui(const char *arg)
{
	cam_dlg_t *ctx = calloc(sizeof(cam_dlg_t), 1);
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(ctx->dlg);

			PCB_DAD_BEGIN_VBOX(ctx->dlg); /* left */
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_VBOX(ctx->dlg); /* right */
				PCB_DAD_BEGIN_VPANE(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg); /* top */
					PCB_DAD_END(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg); /* bottom */
					PCB_DAD_END(ctx->dlg);
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW("cam", ctx->dlg, "CAM export", ctx, pcb_false, cam_close_cb);

	return 0;
}
