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

#include <genvector/gds_char.h>

#include "actions.h"
#include "conf_hid.h"
#include "hid_dad.h"
#include "event.h"
#include "error.h"

#define MAX_ROW 8
#define MAX_COL 4

static const char *search_acts[] = { "select", "unselect", NULL };

#define NEW_EXPR_LAB "<edit me>"

/* for debugging: */
/*#define NEW_EXPR_LAB pcb_strdup_printf("%d:%d", row, col)*/

#include "dlg_search_tab.h"

typedef struct {
	int valid;
	const expr_wizard_t *expr;
	const char *op;
	char *right;
} search_expr_t;

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int wexpr_str, wwizard, wact;
	int wrowbox[MAX_ROW];
	int wexpr[MAX_ROW][MAX_COL]; /* expression framed box */
	int wexpr_lab[MAX_ROW][MAX_COL]; /* the label within the expression box */
	int wexpr_del[MAX_ROW][MAX_COL], wexpr_edit[MAX_ROW][MAX_COL]; /* expression buttons */
	int wor[MAX_ROW][MAX_COL]; /* before the current col */
	int wand[MAX_ROW]; /* before the current row */
	int wnew_or[MAX_ROW], wnew_and;
	int visible[MAX_ROW][MAX_COL];
	search_expr_t expr[MAX_ROW][MAX_COL];
} search_ctx_t;

#define WIZ(ctx) (ctx->dlg[ctx->wwizard].default_val.int_value)

#include "dlg_search_edit.c"

static void search_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	search_ctx_t *ctx = caller_data;
}

static void hspacer(search_ctx_t *ctx)
{
	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_END(ctx->dlg);
}

static void vspacer(search_ctx_t *ctx)
{
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_END(ctx->dlg);
}

static void update_vis(search_ctx_t *ctx)
{
	int c, r, wen;

	wen = WIZ(ctx);

	for(r = 0; r < MAX_ROW; r++) {
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wrowbox[r],!ctx->visible[r][0]);
		for(c = 0; c < MAX_COL; c++) {
			pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wexpr[r][c], !ctx->visible[r][c]);
			if (c > 0)
				pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wor[r][c], !((ctx->visible[r][c-1] && ctx->visible[r][c])));
			pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_del[r][c], wen);
			pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_edit[r][c], wen);
		}
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnew_or[r], !ctx->visible[r][0]);
		if (r > 0)
			pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wand[r], !((ctx->visible[r-1][0] && ctx->visible[r][0])));
		pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnew_or[r], wen);
	}
	pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnew_and, wen);
	pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_str, !wen);
}

/* look up row and col for a widget attr in a [row][col] widget idx array; returns 0 on success */
static int rc_lookup(search_ctx_t *ctx, int w[MAX_ROW][MAX_COL], pcb_hid_attribute_t *attr, int *row, int *col)
{
	int r, c, idx = attr - ctx->dlg;
	for(r = 0; r < MAX_ROW; r++) {
		for(c = 0; c < MAX_COL; c++) {
			if (w[r][c] == idx) {
				*row = r;
				*col = c;
				return 0;
			}
		}
	}
	return -1;
}

/* look up row for a widget attr in a [row] widget idx array; returns 0 on success */
static int r_lookup(search_ctx_t *ctx, int w[MAX_ROW], pcb_hid_attribute_t *attr, int *row)
{
	int r, idx = attr - ctx->dlg;
	for(r = 0; r < MAX_ROW; r++) {
		if (w[r] == idx) {
			*row = r;
			return 0;
		}
	}
	return -1;
}

static void free_expr(search_expr_t *e)
{
	free(e->right);
	memset(e, 0, sizeof(search_expr_t));
}

static void append_expr(gds_t *dst, search_expr_t *e, int sepchar)
{
	gds_append_str(dst, e->expr->left_var);
	gds_append(dst, sepchar);
	gds_append_str(dst, e->op);
	gds_append(dst, sepchar);
	gds_append_str(dst, e->right);
}

static void redraw_expr(search_ctx_t *ctx, int row, int col)
{
	pcb_hid_attr_val_t hv;
	gds_t buf;
	search_expr_t *e = &(ctx->expr[row][col]);

	if (e->valid) {
		gds_init(&buf);
		append_expr(&buf, e, '\n');
		hv.str_value = buf.array;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_lab[row][col], &hv);
		gds_uninit(&buf);
	}
	else {
		hv.str_value = NEW_EXPR_LAB;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_lab[row][col], &hv);
	}
}

static void search_recompile(search_ctx_t *ctx)
{
	pcb_hid_attr_val_t hv;
	gds_t buf;
	int row, col;

	gds_init(&buf);
	for(row = 0; row < MAX_ROW; row++) {
		if (!ctx->visible[row][0] || !ctx->expr[row][0].valid)
			continue;
		if (row > 0)
			gds_append_str(&buf, " && ");
		gds_append(&buf, '(');
		for(col = 0; col < MAX_COL; col++) {
			if (!ctx->visible[row][col] || !ctx->expr[row][col].valid)
				continue;
			if (col > 0)
				gds_append_str(&buf, " || ");
			gds_append(&buf, '(');
			append_expr(&buf, &(ctx->expr[row][col]), ' ');
			gds_append(&buf, ')');
		}
		gds_append(&buf, ')');
	}
	hv.str_value = buf.array;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_str, &hv);
	gds_uninit(&buf);
}

static void search_del_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row, col;

	if (rc_lookup(ctx, ctx->wexpr_del, attr, &row, &col) != 0)
		return;

	free_expr(&(ctx->expr[row][col]));
	for(;col < MAX_COL-1; col++) {
		if (!ctx->visible[row][col+1]) {
			ctx->visible[row][col] = 0;
			memset(&ctx->expr[row][col], 0, sizeof(search_expr_t));
			break;
		}
		ctx->expr[row][col] = ctx->expr[row][col+1];
		redraw_expr(ctx, row, col);
	}
	update_vis(ctx);
	search_recompile(ctx);
}

static void search_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row, col;
	search_expr_t *e;

	if (rc_lookup(ctx, ctx->wexpr_edit, attr, &row, &col) != 0)
		return;

	e = &(ctx->expr[row][col]);
	if (srchedit_window(e) == 0) {
		redraw_expr(ctx, row, col);
		search_recompile(ctx);
	}
}

static void search_enable_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	if (WIZ(ctx))
		search_recompile(ctx); /* overwrite the edit box just in case the user had something custom in it */
	update_vis(ctx);
}

static void search_append_col_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row, col;

	if (r_lookup(ctx, ctx->wnew_or, attr, &row) != 0)
		return;

	for(col = 0; col < MAX_COL; col++) {
		if (!ctx->visible[row][col]) {
			ctx->visible[row][col] = 1;
			redraw_expr(ctx, row, col);
			update_vis(ctx);
			search_recompile(ctx);
			return;
		}
	}
	pcb_message(PCB_MSG_ERROR, "Too many expressions in the row, can not add more\n");
}

static void search_append_row_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row;

	for(row = 0; row < MAX_ROW; row++) {
		if (!ctx->visible[row][0]) {
			ctx->visible[row][0] = 1;
			redraw_expr(ctx, row, 0);
			update_vis(ctx);
			search_recompile(ctx);
			return;
		}
	}
	pcb_message(PCB_MSG_ERROR, "Too many expression rows, can not add more\n");
}

static void search_apply_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	if (ctx->dlg[ctx->wexpr_str].default_val.str_value != NULL)
		pcb_actionl("query", search_acts[ctx->dlg[ctx->wact].default_val.int_value], ctx->dlg[ctx->wexpr_str].default_val.str_value, NULL);
}


static const char *icon_del[] = {
"5 5 2 1",
" 	c None",
"+	c #FF0000",
"+   +",
" + + ",
"  +  ",
" + + ",
"+   +",
};

static const char *icon_edit[] = {
"5 5 2 1",
" 	c None",
"+	c #000000",
"++++ ",
"+    ",
"+++  ",
"+    ",
"++++ ",
};

static void search_window_create(void)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	search_ctx_t *ctx = calloc(sizeof(search_ctx_t), 1);
	int row, col;

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(ctx->dlg, "Query expression:");
		PCB_DAD_STRING(ctx->dlg);
			ctx->wexpr_str = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Action on the results:");
			PCB_DAD_ENUM(ctx->dlg, search_acts);
				ctx->wact = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME | PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "Enable the wizard:");
				PCB_DAD_BOOL(ctx->dlg, "");
					ctx->wwizard = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_DEFAULT_NUM(ctx->dlg, 1);
					PCB_DAD_CHANGE_CB(ctx->dlg, search_enable_cb);
			PCB_DAD_END(ctx->dlg);
			for(row = 0; row < MAX_ROW; row++) {
				if (row > 0) {
					PCB_DAD_BEGIN_HBOX(ctx->dlg);
						ctx->wand[row] = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_HIDE);
						PCB_DAD_LABEL(ctx->dlg, "AND");
					PCB_DAD_END(ctx->dlg);
				}
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, /*PCB_HATF_EXPFILL | */PCB_HATF_HIDE);
					ctx->wrowbox[row] = PCB_DAD_CURRENT(ctx->dlg);
					for(col = 0; col < MAX_COL; col++) {
						ctx->visible[row][col] = 0;
						if (col > 0) {
							PCB_DAD_LABEL(ctx->dlg, "  OR  ");
								PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
								ctx->wor[row][col] = PCB_DAD_CURRENT(ctx->dlg);
						}
						PCB_DAD_BEGIN_HBOX(ctx->dlg);
							PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME | PCB_HATF_TIGHT | PCB_HATF_HIDE);
							ctx->wexpr[row][col] = PCB_DAD_CURRENT(ctx->dlg);
							PCB_DAD_LABEL(ctx->dlg, NEW_EXPR_LAB);
								ctx->wexpr_lab[row][col] = PCB_DAD_CURRENT(ctx->dlg);
							PCB_DAD_BEGIN_VBOX(ctx->dlg);
								PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_TIGHT);
								PCB_DAD_PICBUTTON(ctx->dlg, icon_del);
									PCB_DAD_HELP(ctx->dlg, "Remove expression");
									ctx->wexpr_del[row][col] = PCB_DAD_CURRENT(ctx->dlg);
									PCB_DAD_CHANGE_CB(ctx->dlg, search_del_cb);
								PCB_DAD_PICBUTTON(ctx->dlg, icon_edit);
									PCB_DAD_HELP(ctx->dlg, "Edit expression");
									ctx->wexpr_edit[row][col] = PCB_DAD_CURRENT(ctx->dlg);
									PCB_DAD_CHANGE_CB(ctx->dlg, search_edit_cb);
							PCB_DAD_END(ctx->dlg);
						PCB_DAD_END(ctx->dlg);
					}
					hspacer(ctx);
					PCB_DAD_BUTTON(ctx->dlg, "append OR");
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_HIDE);
						ctx->wnew_or[row] = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_CHANGE_CB(ctx->dlg, search_append_col_cb);
				PCB_DAD_END(ctx->dlg);
			}
			vspacer(ctx);
			PCB_DAD_BUTTON(ctx->dlg, "append AND");
				ctx->wnew_and = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_CHANGE_CB(ctx->dlg, search_append_row_cb);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Apply");
				PCB_DAD_CHANGE_CB(ctx->dlg, search_apply_cb);
				PCB_DAD_HELP(ctx->dlg, "Execute the search expression\nusing the selected action");
			hspacer(ctx);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_DEFSIZE(ctx->dlg, 300, 350);
	PCB_DAD_NEW("search", ctx->dlg, "pcb-rnd search", ctx, pcb_false, search_close_cb);

	ctx->visible[0][0] = 1;
	update_vis(ctx);
	search_recompile(ctx);
}


const char pcb_acts_SearchDialog[] = "SearchDialog()\n";
const char pcb_acth_SearchDialog[] = "Open the log dialog.";
fgw_error_t pcb_act_SearchDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	search_window_create();
	PCB_ACT_IRES(0);
	return 0;
}


