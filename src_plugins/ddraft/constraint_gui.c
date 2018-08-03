/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin: trim objects
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "hid_dad.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} cnstgui_ctx_t;

cnstgui_ctx_t cnstgui_ctx;

static void cnstgui_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	cnstgui_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(cnstgui_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

int constraint_gui(void)
{
	const char *tab_names[] = {"line", "move", NULL};
	if (cnstgui_ctx.active)
		return 0; /* do not open another */

	PCB_DAD_BEGIN_TABBED(cnstgui_ctx.dlg, tab_names);

		/* line */
		PCB_DAD_BEGIN_VBOX(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "All direction lines (shall be on):");
				PCB_DAD_BOOL(cnstgui_ctx.dlg, "");
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed angles:");
				PCB_DAD_STRING(cnstgui_ctx.dlg);
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Angle modulo:");
				PCB_DAD_REAL(cnstgui_ctx.dlg, "");
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed lengthd:");
				PCB_DAD_STRING(cnstgui_ctx.dlg);
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Length modulo:");
				PCB_DAD_REAL(cnstgui_ctx.dlg, "");
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_BUTTON(cnstgui_ctx.dlg, "Reset");
				PCB_DAD_BUTTON(cnstgui_ctx.dlg, "perpendicular to");
				PCB_DAD_BUTTON(cnstgui_ctx.dlg, "parallel with");
			PCB_DAD_END(cnstgui_ctx.dlg);
		PCB_DAD_END(cnstgui_ctx.dlg);

		/* move */
		PCB_DAD_BEGIN_VBOX(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed angles:");
				PCB_DAD_STRING(cnstgui_ctx.dlg);
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Angle modulo:");
				PCB_DAD_REAL(cnstgui_ctx.dlg, "");
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Fixed lengthd:");
				PCB_DAD_STRING(cnstgui_ctx.dlg);
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_LABEL(cnstgui_ctx.dlg, "Length modulo:");
				PCB_DAD_REAL(cnstgui_ctx.dlg, "");
			PCB_DAD_END(cnstgui_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(cnstgui_ctx.dlg);
				PCB_DAD_BUTTON(cnstgui_ctx.dlg, "Reset");
			PCB_DAD_END(cnstgui_ctx.dlg);
		PCB_DAD_END(cnstgui_ctx.dlg);

	PCB_DAD_END(cnstgui_ctx.dlg);

	/* set up the context */
	cnstgui_ctx.active = 1;

	PCB_DAD_NEW(cnstgui_ctx.dlg, "Drawing constraints", "constraints", &cnstgui_ctx, pcb_false, cnstgui_close_cb);

	return 0;
}
