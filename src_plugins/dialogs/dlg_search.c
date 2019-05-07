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
#include "event.h"
#include "error.h"

#define MAX_ROW 8
#define MAX_COL 4

/*#define NEW_EXPR_LAB "<edit me>"*/

/* for debugging: */
#define NEW_EXPR_LAB pcb_strdup_printf("%d:%d", row, col)

#include "dlg_search_tab.h"

typedef struct {
	const expr_wizard_t *expr;
	pcb_hid_attr_val_t right;
} search_expr_t;

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
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
	int c, r;

	for(r = 0; r < MAX_ROW; r++) {
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wrowbox[r],!ctx->visible[r][0]);
		for(c = 0; c < MAX_COL; c++) {
			pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wexpr[r][c], !ctx->visible[r][c]);
			if (c > 0)
				pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wor[r][c], !((ctx->visible[r][c-1] && ctx->visible[r][c])));
		}
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnew_or[r], !ctx->visible[r][0]);
		if (r > 0)
			pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wand[r], !((ctx->visible[r-1][0] && ctx->visible[r][0])));
	}
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

static void search_del_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row, col;

	if (rc_lookup(ctx, ctx->wexpr_del, attr, &row, &col) != 0)
		return;
	printf("del at %d %d\n", row, col);
}

static void search_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	search_ctx_t *ctx = caller_data;
	int row, col;

	if (rc_lookup(ctx, ctx->wexpr_edit, attr, &row, &col) != 0)
		return;
	printf("edit at %d %d\n", row, col);
	srchedit_window_create(&(ctx->expr[row][col]));
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
	static const char *acts[] = { "select", "unselect", NULL };
	search_ctx_t *ctx = calloc(sizeof(search_ctx_t), 1);
	int row, col;

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(ctx->dlg, "Query expression:");
		PCB_DAD_STRING(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Action on the results:");
			PCB_DAD_ENUM(ctx->dlg, acts);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME | PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "Enable the wizard:");
				PCB_DAD_BOOL(ctx->dlg, "");
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
				PCB_DAD_END(ctx->dlg);
			}
			vspacer(ctx);
			PCB_DAD_BUTTON(ctx->dlg, "append AND");
				ctx->wnew_and = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_DEFSIZE(ctx->dlg, 300, 350);
	PCB_DAD_NEW("search", ctx->dlg, "pcb-rnd search", ctx, pcb_false, search_close_cb);

	ctx->visible[0][0] = 1;
	ctx->visible[0][1] = 1;
	ctx->visible[1][0] = 1;
	update_vis(ctx);
}


const char pcb_acts_SearchDialog[] = "SearchDialog()\n";
const char pcb_acth_SearchDialog[] = "Open the log dialog.";
fgw_error_t pcb_act_SearchDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	search_window_create();
	PCB_ACT_IRES(0);
	return 0;
}


