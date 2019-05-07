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
	int wleft, wop;
	const expr_wizard_op_t *last_op;
} srchedit_ctx_t;

static srchedit_ctx_t srchedit_ctx;

static void srchedit_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
/*	srchedit_ctx_t *ctx = caller_data;*/
}

/* the table on the left is static, needs to be filled in only once */
static void fill_in_left(srchedit_ctx_t *ctx)
{
	const expr_wizard_t *t;
	pcb_hid_attribute_t *attr;
	pcb_hid_row_t *r, *parent = NULL;
	char *cell[2];

	attr = &ctx->dlg[ctx->wleft];

	cell[1] = NULL;
	for(t = expr_tab; t->left_desc != NULL; t++) {
		if (t->left_var == NULL)
			parent = NULL;
		cell[0] = pcb_strdup(t->left_desc);
		if (parent != NULL)
			r = pcb_dad_tree_append_under(attr, parent, cell);
		else
			r = pcb_dad_tree_append(attr, NULL, cell);
		r->user_data = (void *)t;
		if (t->left_var == NULL)
			parent = r;
	}
}

static void srch_expr_set_ops(srchedit_ctx_t *ctx, const expr_wizard_op_t *op)
{
	pcb_hid_tree_t *tree;
	pcb_hid_attribute_t *attr;
	pcb_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;
	const char **o;

	if (op == ctx->last_op)
		return;

	attr = &ctx->dlg[ctx->wop];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all items */
	cell[1] = NULL;
	for(o = op->ops; *o != NULL; o++) {
		cell[0] = pcb_strdup_printf(*o);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wop, &hv);
		free(cursor_path);
	}
	ctx->last_op = op;
}

static void srch_expr_left_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	srchedit_ctx_t *ctx = tree->user_ctx;
	const expr_wizard_t *e;

	if (row == NULL)
		return;

	e = row->user_data;
	if (e->left_var == NULL) /* category */
		return;

	ctx->expr->expr = e;
	srch_expr_set_ops(ctx, e->op);
}


static void srchedit_window_create(search_expr_t *expr)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"OK", 0}, {NULL, 0}};
	srchedit_ctx_t *ctx = &srchedit_ctx;

	ctx->expr = expr;

	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL);
				ctx->wleft = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, srch_expr_left_cb);
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
				ctx->wop = PCB_DAD_CURRENT(ctx->dlg);
/*				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, srch_expr_op_cb);*/
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_DEFSIZE(ctx->dlg, 450, 450);
	PCB_DAD_NEW("search_expr", ctx->dlg, "pcb-rnd search expression", ctx, pcb_true, srchedit_close_cb);
	fill_in_left(ctx);
	srch_expr_set_ops(ctx, op_tab); /* just to get the initial tree widget width */
}


