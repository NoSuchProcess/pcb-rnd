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
#include "conf_hid.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "error.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	search_expr_t *expr;
} srchedit_ctx_t;

static srchedit_ctx_t srchedit_ctx;

static void srchedit_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
/*	srchedit_ctx_t *ctx = caller_data;*/
}

static void srchedit_window_create(search_expr_t *expr)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"OK", 0}, {NULL, 0}};
	srchedit_ctx_t *ctx = &srchedit_ctx;

	ctx->expr = expr;

	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_TREE(ctx->dlg, 0, 1, NULL);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_TREE(ctx->dlg, 0, 1, NULL);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_DEFSIZE(ctx->dlg, 300, 300);
	PCB_DAD_NEW("search", ctx->dlg, "pcb-rnd search", ctx, pcb_true, srchedit_close_cb);
}


