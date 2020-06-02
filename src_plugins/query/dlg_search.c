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

#include <librnd/core/actions.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/event.h>
#include <librnd/core/error.h>
#include "query.h"
#include "board.h"

#define MAX_ROW 8
#define MAX_COL 4

static const char *search_acts[] = { "select", "unselect", "view", NULL };

#define NEW_EXPR_LAB "<edit me>"

/* for debugging: */
/*#define NEW_EXPR_LAB rnd_strdup_printf("%d:%d", row, col)*/

#include "dlg_search_tab.h"

typedef struct {
	int valid;
	const expr_wizard_t *expr;
	const char *op;
	char *right;
} search_expr_t;

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
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

#define WIZ(ctx) (ctx->dlg[ctx->wwizard].val.lng)

#include "dlg_search_edit.c"

static void free_expr(search_expr_t *e)
{
	free(e->right);
	memset(e, 0, sizeof(search_expr_t));
}

static void search_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	int r, c;
	search_ctx_t *ctx = caller_data;
	for(r = 0; r < MAX_ROW; r++)
		for(c = 0; c < MAX_COL; c++)
			free_expr(&ctx->expr[r][c]);
	free(ctx);
}

static void hspacer(search_ctx_t *ctx)
{
	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_END(ctx->dlg);
}

static void vspacer(search_ctx_t *ctx)
{
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_END(ctx->dlg);
}

static void update_vis(search_ctx_t *ctx)
{
	int c, r, wen;

	wen = WIZ(ctx);

	for(r = 0; r < MAX_ROW; r++) {
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wrowbox[r],!ctx->visible[r][0]);
		for(c = 0; c < MAX_COL; c++) {
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wexpr[r][c], !ctx->visible[r][c]);
			if (c > 0)
				rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wor[r][c], !((ctx->visible[r][c-1] && ctx->visible[r][c])));
			rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_del[r][c], wen);
			rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_edit[r][c], wen);
		}
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnew_or[r], !ctx->visible[r][0]);
		if (r > 0)
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wand[r], !((ctx->visible[r-1][0] && ctx->visible[r][0])));
		rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnew_or[r], wen);
	}
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wnew_and, wen);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wexpr_str, !wen);
}

/* look up row and col for a widget attr in a [row][col] widget idx array; returns 0 on success */
static int rc_lookup(search_ctx_t *ctx, int w[MAX_ROW][MAX_COL], rnd_hid_attribute_t *attr, int *row, int *col)
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
static int r_lookup(search_ctx_t *ctx, int w[MAX_ROW], rnd_hid_attribute_t *attr, int *row)
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
	rnd_hid_attr_val_t hv;
	gds_t buf;
	search_expr_t *e = &(ctx->expr[row][col]);

	if (e->valid) {
		gds_init(&buf);
		append_expr(&buf, e, '\n');
		hv.str = buf.array;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_lab[row][col], &hv);
		gds_uninit(&buf);
	}
	else {
		hv.str = NEW_EXPR_LAB;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_lab[row][col], &hv);
	}
}

static void search_recompile(search_ctx_t *ctx)
{
	rnd_hid_attr_val_t hv;
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
	hv.str = buf.array;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wexpr_str, &hv);
	gds_uninit(&buf);
}

static const char *qop_text[]         = {"==",       "!=",        ">=",         "<=",         ">",        "<",        "~"};
static const pcb_qry_nodetype_t qop[] = {PCBQ_OP_EQ, PCBQ_OP_NEQ, PCBQ_OP_GTEQ, PCBQ_OP_LTEQ, PCBQ_OP_GT, PCBQ_OP_LT, PCBQ_OP_MATCH};


static int search_decompile_(search_ctx_t *ctx, pcb_qry_node_t *nd, int dry, int row, int col)
{
	int n;
	const char *qopt = NULL;

	if (nd->type == PCBQ_EXPR_PROG)
		return search_decompile_(ctx, nd->data.children, dry, row, col);

	if (nd->type == PCBQ_ITER_CTX)
		return search_decompile_(ctx, nd->next, dry, row, col);

	if (nd->type == PCBQ_OP_AND) {
		if ((nd->parent == NULL) || ((nd->parent->type != PCBQ_OP_AND) && (nd->parent->type != PCBQ_EXPR_PROG))) /* first level of ops must be AND */
			return -1;
		if (search_decompile_(ctx, nd->data.children, dry, row, col) != 0)
			return -1;
		return search_decompile_(ctx, nd->data.children->next, dry, row+1, col);
	}

	if (nd->type == PCBQ_OP_OR) {
		if ((nd->parent == NULL) || ((nd->parent->type != PCBQ_OP_AND) && (nd->parent->type != PCBQ_OP_OR))) /* second level of ops must be OR */
			return -1;
		if (search_decompile_(ctx, nd->data.children, dry, row, col) != 0)
			return -1;
		return search_decompile_(ctx, nd->data.children->next, dry, row, col+1);
	}

	/* plain op */
	for(n = 0; n < sizeof(qop) / sizeof(qop[0]); n++) {
		if (qop[n] == nd->type) {
			qopt = qop_text[n];
			break;
		}
	}

	if (qopt == NULL)
		return -1; /* unknown binary op */

rnd_trace("decomp: %d (%s) at %d;%d\n", nd->type, qopt, row, col);
	return 0;
}

static void search_decompile(search_ctx_t *ctx)
{
	const char *script = ctx->dlg[ctx->wexpr_str].val.str;
	pcb_qry_node_t *root;

	if (script == NULL) return;
	while(isspace(*script)) script++;
	if (*script == '\0') return;

	root = pcb_query_compile(script);
	if (root == NULL) {
		rnd_message(RND_MSG_ERROR, "Syntax error compiling the script\nThe GUI will not be filled in by the script\n");
		return;
	}

	if (search_decompile_(ctx, root, 1, 0, 0) != 0) {
		rnd_message(RND_MSG_ERROR, "The script is too complex for the GUI\nThe GUI will not be filled in by the script\n");
		return;
	}

TODO("Clear the GUI");

	search_decompile_(ctx, root, 0, 0, 0);
}

static void search_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
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

static void search_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
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

static void search_enable_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	if (WIZ(ctx)) {
		search_decompile(ctx); /* fills in the GUI from the expression, if possible */
		search_recompile(ctx); /* overwrite the edit box just in case the user had something custom in it */
	}

	update_vis(ctx);
}

static void search_append_col_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
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
	rnd_message(RND_MSG_ERROR, "Too many expressions in the row, can not add more\n");
}

static void search_append_row_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
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
	rnd_message(RND_MSG_ERROR, "Too many expression rows, can not add more\n");
}

static void search_apply_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	if (ctx->dlg[ctx->wexpr_str].val.str != NULL)
		rnd_actionva(&PCB->hidlib, "query", search_acts[ctx->dlg[ctx->wact].val.lng], ctx->dlg[ctx->wexpr_str].val.str, NULL);
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
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	search_ctx_t *ctx = calloc(sizeof(search_ctx_t), 1);
	int row, col;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(ctx->dlg, "Query expression:");
		RND_DAD_STRING(ctx->dlg);
			RND_DAD_WIDTH_CHR(ctx->dlg, 64);
			ctx->wexpr_str = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Action on the results:");
			RND_DAD_ENUM(ctx->dlg, search_acts);
				ctx->wact = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_EXPFILL | RND_HATF_SCROLL);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "Enable the wizard:");
				RND_DAD_BOOL(ctx->dlg, "");
					ctx->wwizard = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_DEFAULT_NUM(ctx->dlg, 1);
					RND_DAD_CHANGE_CB(ctx->dlg, search_enable_cb);
			RND_DAD_END(ctx->dlg);
			for(row = 0; row < MAX_ROW; row++) {
				if (row > 0) {
					RND_DAD_BEGIN_HBOX(ctx->dlg);
						ctx->wand[row] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_HIDE);
						RND_DAD_LABEL(ctx->dlg, "AND");
					RND_DAD_END(ctx->dlg);
				}
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, /*RND_HATF_EXPFILL | */RND_HATF_HIDE);
					ctx->wrowbox[row] = RND_DAD_CURRENT(ctx->dlg);
					for(col = 0; col < MAX_COL; col++) {
						ctx->visible[row][col] = 0;
						if (col > 0) {
							RND_DAD_LABEL(ctx->dlg, "  OR  ");
								RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_HIDE);
								ctx->wor[row][col] = RND_DAD_CURRENT(ctx->dlg);
						}
						RND_DAD_BEGIN_HBOX(ctx->dlg);
							RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_TIGHT | RND_HATF_HIDE);
							ctx->wexpr[row][col] = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_LABEL(ctx->dlg, NEW_EXPR_LAB);
								ctx->wexpr_lab[row][col] = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_BEGIN_VBOX(ctx->dlg);
								RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_TIGHT);
								RND_DAD_PICBUTTON(ctx->dlg, icon_del);
									RND_DAD_HELP(ctx->dlg, "Remove expression");
									ctx->wexpr_del[row][col] = RND_DAD_CURRENT(ctx->dlg);
									RND_DAD_CHANGE_CB(ctx->dlg, search_del_cb);
								RND_DAD_PICBUTTON(ctx->dlg, icon_edit);
									RND_DAD_HELP(ctx->dlg, "Edit expression");
									ctx->wexpr_edit[row][col] = RND_DAD_CURRENT(ctx->dlg);
									RND_DAD_CHANGE_CB(ctx->dlg, search_edit_cb);
							RND_DAD_END(ctx->dlg);
						RND_DAD_END(ctx->dlg);
					}
					hspacer(ctx);
					RND_DAD_BUTTON(ctx->dlg, "append OR");
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_HIDE);
						ctx->wnew_or[row] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, search_append_col_cb);
				RND_DAD_END(ctx->dlg);
			}
			vspacer(ctx);
			RND_DAD_BUTTON(ctx->dlg, "append AND");
				ctx->wnew_and = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_CHANGE_CB(ctx->dlg, search_append_row_cb);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Apply");
				RND_DAD_CHANGE_CB(ctx->dlg, search_apply_cb);
				RND_DAD_HELP(ctx->dlg, "Execute the search expression\nusing the selected action");
			hspacer(ctx);
			RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_DEFSIZE(ctx->dlg, 300, 350);
	RND_DAD_NEW("search", ctx->dlg, "pcb-rnd search", ctx, rnd_false, search_close_cb);

	ctx->visible[0][0] = 1;
	update_vis(ctx);
	search_recompile(ctx);
}


const char pcb_acts_SearchDialog[] = "SearchDialog()\n";
const char pcb_acth_SearchDialog[] = "Open the log dialog.";
fgw_error_t pcb_act_SearchDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	search_window_create();
	RND_ACT_IRES(0);
	return 0;
}


