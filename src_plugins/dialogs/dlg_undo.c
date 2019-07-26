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

#include <libuundo/uundo.h>
#include "event.h"
#include "undo.h"

const char *dlg_undo_cookie = "undo dialog";

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int wlist;
	long serial; /* last seen undo serial, for updating the dialog on change */
	int active; /* already open - allow only one instance */
} undo_ctx_t;

undo_ctx_t undo_ctx;

extern uundo_list_t pcb_uundo;

static void undo_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	undo_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(undo_ctx_t));
}

static void cb_undo(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_undo(pcb_true);
}

static void cb_redo(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_redo(pcb_true);
}

static void cb_clear(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_undo_clear_list(pcb_true);
}


static void undo_data2dlg(undo_ctx_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	uundo_item_t *i;
	char *payload, buff[8192], mark[2], ser[64];

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	mark[1] = '\0';
	cell[3] = NULL;
	for(i = pcb_uundo.head; i != NULL; i = i->next) {
		payload = "<unknown>";
		if (i->oper->item_print != NULL) {
			i->oper->item_print(i->udata, buff, sizeof(buff));
			payload = buff;
		}
		mark[0] = '\0';
		if ((i == pcb_uundo.head) && (i == pcb_uundo.tail))
			mark[0] = '*';
		else if (i == pcb_uundo.head)
			mark[0] = 'h';
		else if (i == pcb_uundo.tail)
			mark[0] = 't';
		sprintf(ser, "%ld", (long)i->serial);
		cell[0] = pcb_strdup(ser);
		cell[1] = pcb_strdup(mark);
		cell[2] = pcb_strdup(payload);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
}

static void pcb_dlg_undo(void)
{
	static const char *hdr[] = {"serial", "flg", "operation", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (undo_ctx.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(undo_ctx.dlg);
		PCB_DAD_COMPFLAG(undo_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TREE(undo_ctx.dlg, 3, 0, hdr);
			PCB_DAD_COMPFLAG(undo_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			undo_ctx.wlist = PCB_DAD_CURRENT(undo_ctx.dlg);

		PCB_DAD_BEGIN_HBOX(undo_ctx.dlg);
			PCB_DAD_BUTTON(undo_ctx.dlg, "Undo");
				PCB_DAD_CHANGE_CB(undo_ctx.dlg, cb_undo);
			PCB_DAD_BUTTON(undo_ctx.dlg, "Redo");
				PCB_DAD_CHANGE_CB(undo_ctx.dlg, cb_redo);
			PCB_DAD_BUTTON(undo_ctx.dlg, "Clear");
				PCB_DAD_CHANGE_CB(undo_ctx.dlg, cb_clear);
		PCB_DAD_END(undo_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(undo_ctx.dlg, clbtn);
	PCB_DAD_END(undo_ctx.dlg);

	/* set up the context */
	undo_ctx.active = 1;

	PCB_DAD_DEFSIZE(undo_ctx.dlg, 300, 400);
	PCB_DAD_NEW("undo", undo_ctx.dlg, "pcb-rnd undo list", &undo_ctx, pcb_false, undo_close_cb);
	undo_data2dlg(&undo_ctx);
}

static const char pcb_acts_UndoDialog[] = "UndoDialog()\n";
static const char pcb_acth_UndoDialog[] = "Open the undo dialog.";
static fgw_error_t pcb_act_UndoDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_undo();
	PCB_ACT_IRES(0);
	return 0;
}

/* update the dialog after an undo operation */
static void pcb_dlg_undo_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	undo_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	undo_data2dlg(ctx);
}

/* Check if the serial has changed and update the dialog if so */
static void pcb_dlg_undo_ev_chk(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	undo_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	if (ctx->serial != pcb_uundo.serial) {
		undo_data2dlg(ctx);
		ctx->serial = pcb_uundo.serial;
	}
}


static void pcb_dlg_undo_init(void)
{
	pcb_event_bind(PCB_EVENT_UNDO_POST, pcb_dlg_undo_ev, &undo_ctx, dlg_undo_cookie);
	pcb_event_bind(PCB_EVENT_USER_INPUT_POST, pcb_dlg_undo_ev_chk, &undo_ctx, dlg_undo_cookie);
}

static void pcb_dlg_undo_uninit(void)
{
	pcb_event_unbind_allcookie(dlg_undo_cookie);
}
