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
	search_expr_t se;
	int wleft, wop, wright[RIGHT_max];
	const expr_wizard_op_t *last_op;
	right_type last_rtype;
} srchedit_ctx_t;

static srchedit_ctx_t srchedit_ctx;

/* Set the right side of the expression from the non-enum value of an widget attr */
static void set_right(srchedit_ctx_t *ctx, pcb_hid_attribute_t *attr)
{
	free(ctx->se.right);
	ctx->se.right = NULL;

	switch(ctx->se.expr->rtype) {
		case RIGHT_STR:
			ctx->se.right = pcb_strdup(attr->default_val.str_value);
			break;
		case RIGHT_INT:
			ctx->se.right = pcb_strdup_printf("%d", attr->default_val.int_value);
			break;
		case RIGHT_DOUBLE:
			ctx->se.right = pcb_strdup_printf("%f", attr->default_val.real_value);
			break;
		case RIGHT_COORD:
			ctx->se.right = pcb_strdup_printf("%$mm", attr->default_val.coord_value);
			break;
		case RIGHT_CONST:
		case RIGHT_max:
			break;
	}
}

static void srch_expr_set_ops(srchedit_ctx_t *ctx, const expr_wizard_op_t *op, int click)
{
	pcb_hid_tree_t *tree;
	pcb_hid_attribute_t *attr;
	pcb_hid_row_t *r, *cur = NULL;
	char *cell[2], *cursor_path = NULL;
	const char **o;

	if (op == ctx->last_op)
		return;

	attr = &ctx->dlg[ctx->wop];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	if (click) {
		r = pcb_dad_tree_get_selected(attr);
		if (r != NULL)
			cursor_path = pcb_strdup(r->cell[0]);
	}

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all items */
	cell[1] = NULL;
	for(o = op->ops; *o != NULL; o++) {
		cell[0] = pcb_strdup_printf(*o);
		r = pcb_dad_tree_append(attr, NULL, cell);
		r->user_data = *o;
		if ((!click) && (ctx->se.op == *o))
			cur = r;
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wop, &hv);
		free(cursor_path);
	}
	if (cur != NULL)
		pcb_dad_tree_jumpto(attr, cur);
	ctx->last_op = op;
}

static void srch_expr_fill_in_right_const(srchedit_ctx_t *ctx, const search_expr_t *s)
{
	pcb_hid_tree_t *tree;
	pcb_hid_attribute_t *attr;
	char *cell[2];
	const char **o;

	attr = &ctx->dlg[ctx->wright[RIGHT_CONST]];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all items */
	cell[1] = NULL;
	for(o = right_const_tab[s->expr->rtype].ops; *o != NULL; o++) {
		cell[0] = pcb_strdup_printf(*o);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* set cursor to last known value */
	if ((s != NULL) && (s->right != NULL)) {
		pcb_hid_attr_val_t hv;
		hv.str_value = s->right;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wright[RIGHT_CONST], &hv);
	}
}

static void srch_expr_fill_in_right(srchedit_ctx_t *ctx, const search_expr_t *s)
{
	int n, empty = 0;
	pcb_hid_attr_val_t hv;

	if (s->expr->rtype == ctx->last_rtype)
		return;

	for(n = 0; n < RIGHT_max; n++)
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wright[n], 1);

	hv.str_value = ctx->se.right;
	if (hv.str_value == NULL) {
		hv.str_value = "";
		empty = 1;
	}

	switch(s->expr->rtype) {
		case RIGHT_STR:
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wright[s->expr->rtype], &hv);
			break;
		case RIGHT_INT:
			hv.int_value = strtol(hv.str_value, NULL, 10);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wright[s->expr->rtype], &hv);
			if (empty)
				set_right(ctx, &ctx->dlg[ctx->wright[s->expr->rtype]]);
			break;
		case RIGHT_DOUBLE:
			hv.real_value = strtod(hv.str_value, NULL);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wright[s->expr->rtype], &hv);
			if (empty)
				set_right(ctx, &ctx->dlg[ctx->wright[s->expr->rtype]]);
			break;
		case RIGHT_COORD:
			hv.coord_value = pcb_get_value_ex(hv.str_value, NULL, NULL, NULL, "mm", NULL);
			pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wright[s->expr->rtype], &hv);
			if (empty)
				set_right(ctx, &ctx->dlg[ctx->wright[s->expr->rtype]]);
			break;
		case RIGHT_CONST: srch_expr_fill_in_right_const(ctx, s); break;
		case RIGHT_max: break;
	}

	pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wright[s->expr->rtype], 0);
	ctx->last_rtype = s->expr->rtype;
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

	ctx->se.expr = e;
	srch_expr_set_ops(ctx, e->op, 1);
	srch_expr_fill_in_right(ctx, &ctx->se);
}

static void srch_expr_op_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	srchedit_ctx_t *ctx = tree->user_ctx;

	if (row != NULL)
		ctx->se.op = row->user_data;
	else
		ctx->se.op = NULL;
}

/* the table on the left is static, needs to be filled in only once;
   returns 1 if filled in op and right, else 0 */
static int fill_in_left(srchedit_ctx_t *ctx)
{
	const expr_wizard_t *t;
	pcb_hid_attribute_t *attr;
	pcb_hid_row_t *r, *parent = NULL, *cur = NULL;
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
		if (t == ctx->se.expr)
			cur = r;
	}

	if (cur != NULL) {
		pcb_dad_tree_jumpto(attr, cur);

		/* clear all cache fields so a second window open won't inhibit refreshes */
		ctx->last_op = NULL;
		ctx->last_rtype = RIGHT_max;

		srch_expr_set_ops(ctx, ctx->se.expr->op, 0);
		srch_expr_fill_in_right(ctx, &ctx->se);
		return 1;
	}
	return 0;
}

static void srchexpr_right_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	srchedit_ctx_t *ctx = caller_data;

	set_right(ctx, attr);
}

static void srch_expr_right_table_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	srchedit_ctx_t *ctx = tree->user_ctx;

	free(ctx->se.right);
	ctx->se.right = NULL;
	if (row != NULL)
		ctx->se.right = pcb_strdup(row->cell[0]);
}

static int srchedit_window(search_expr_t *expr)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"OK", 0}, {NULL, 0}};
	srchedit_ctx_t *ctx = &srchedit_ctx;
	int res;

	ctx->se = *expr;
	if (ctx->se.right != NULL)
		ctx->se.right = pcb_strdup(ctx->se.right);

	/* clear all cache fields so a second window open won't inhibit refreshes */
	ctx->last_op = NULL;
	ctx->last_rtype = RIGHT_max;

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
				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, srch_expr_op_cb);
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
				ctx->wright[RIGHT_STR] = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_CHANGE_CB(ctx->dlg, srchexpr_right_cb);

			PCB_DAD_INTEGER(ctx->dlg, "");
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
				ctx->wright[RIGHT_INT] = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, -(1UL<<30), (1UL<<30));
				PCB_DAD_CHANGE_CB(ctx->dlg, srchexpr_right_cb);

			PCB_DAD_REAL(ctx->dlg, "");
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
				ctx->wright[RIGHT_DOUBLE] = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, -(1UL<<31), (1UL<<31));
				PCB_DAD_CHANGE_CB(ctx->dlg, srchexpr_right_cb);

			PCB_DAD_COORD(ctx->dlg, "");
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
				ctx->wright[RIGHT_COORD] = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, -PCB_MAX_COORD, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(ctx->dlg, srchexpr_right_cb);

			PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
				ctx->wright[RIGHT_CONST] = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, srch_expr_right_table_cb);
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);

			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_DEFSIZE(ctx->dlg, 450, 450);
	PCB_DAD_NEW("search_expr", ctx->dlg, "pcb-rnd search expression", ctx, pcb_true, NULL);

	if (fill_in_left(ctx) != 1) {
		srch_expr_set_ops(ctx, op_tab, 1); /* just to get the initial tree widget width */
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wright[RIGHT_CONST], 0); /* just to get something harmless display on the right side after open */
	}

	res = PCB_DAD_RUN(ctx->dlg);
	if (res == 0) {
		free(expr->right);
		*expr = ctx->se;
		if (ctx->se.op != NULL)
			expr->valid = 1;
	}
	else
		free(ctx->se.right);

	PCB_DAD_FREE(ctx->dlg);
	return res;
}


