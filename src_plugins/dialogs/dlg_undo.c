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

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int whatever;
} undo_ctx_t;

undo_ctx_t undo_ctx;

static void undo_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	undo_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(undo_ctx_t));
}

static void pcb_dlg_undo(void)
{
	static const char *hdr[] = {"flg", "serial", "operation", "payload", NULL};
	if (undo_ctx.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(undo_ctx.dlg);
		PCB_DAD_TREE(undo_ctx.dlg, 4, 0, hdr);
			PCB_DAD_COMPFLAG(undo_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);

		PCB_DAD_BEGIN_HBOX(undo_ctx.dlg);
			PCB_DAD_COMPFLAG(undo_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BUTTON(undo_ctx.dlg, "Undo");
			PCB_DAD_BUTTON(undo_ctx.dlg, "Redo");
			PCB_DAD_BUTTON(undo_ctx.dlg, "Clear");
		PCB_DAD_END(undo_ctx.dlg);
	PCB_DAD_END(undo_ctx.dlg);

	/* set up the context */
	undo_ctx.active = 1;

	PCB_DAD_NEW(undo_ctx.dlg, "pcb-rnd undo list", "", &undo_ctx, pcb_false, undo_close_cb);
}

static const char pcb_acts_UndoDialog[] = "UndoDialog()\n";
static const char pcb_acth_UndoDialog[] = "Open the undo dialog.";
static fgw_error_t pcb_act_UndoDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_undo();
	PCB_ACT_IRES(0);
	return 0;
}
