/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* advanced search dialog */


#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <genlist/gendlist.h>
#include "gui.h"
#include "create.h"
#include "compat_misc.h"
#include "ghid-search.h"
#include "win_place.h"

typedef struct expr1_s expr1_t;

struct expr1_s {
	GtkWidget *and;        /* only if not the first row */

	GtkWidget *hbox;       /* only if first in row*/
	GtkWidget *remove_row; /* only if first in row*/
	GtkWidget *append_col; /* only if first in row*/
	GtkWidget *spc;        /* only if first in row*/

	GtkWidget *remove, *remove_button;
	GtkWidget *content;    /* the actual expression */
	GtkWidget *or;         /* only if not the first in the row */

	gdl_elem_t next_or;   /* --> */
	gdl_elem_t next_and;  /* v */
	gdl_list_t ors;       /* only in the first; the row-first is not on this list, so it collects the subseqent siblings only */

	expr1_t *row;         /* only if not the first */

	gulong sig_remove_row;
	gulong sig_append_col;
};

typedef struct {
	GtkWidget *window;
	GtkWidget *expr;          /* manual expression entry */
	GtkWidget *action;        /* what-to-do combo box */
	GtkWidget *wizard_enable; /* checkbox */
	GtkWidget *wizard_vbox;
	GtkWidget *new_row;

	gdl_list_t wizard;       /* of expr1_t */
} ghid_search_dialog_t;

static ghid_search_dialog_t sdlg;

static void new_col_cb(GtkWidget *button, void *data);
static void remove_row_cb(GtkWidget *button, void *data);
static void remove_expr_cb(GtkWidget *button, void *data);
static void edit_expr_cb(GtkWidget *button, void *data);
static void expr_wizard_dialog(expr1_t *e);

static void build_expr1(expr1_t *e, GtkWidget *parent_box)
{
	e->content = gtk_button_new_with_label("<expr>");
	gtk_button_set_image(GTK_BUTTON(e->content), gtk_image_new_from_icon_name("gtk-new", GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(parent_box), e->content, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(e->content, "Edit search expression");
	g_signal_connect(e->content, "clicked", G_CALLBACK(edit_expr_cb), e);

	e->remove = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent_box), e->remove, FALSE, FALSE, 0);

	e->remove_button = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(e->remove_button), gtk_image_new_from_icon_name("gtk-delete", GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(e->remove), e->remove_button, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(e->remove_button, "Remove this expression");
	g_signal_connect(e->remove_button, "clicked", G_CALLBACK(remove_expr_cb), e);
}

/* e is not part of any list by the time of the call */
static void destroy_expr1(expr1_t *e)
{
#	define destroy(w) if (w != NULL) gtk_widget_destroy(w)
	destroy(e->and);
	destroy(e->spc);
	destroy(e->remove_row);
	destroy(e->append_col);
	destroy(e->remove);
	destroy(e->content);
	destroy(e->or);
	destroy(e->hbox);
	free(e);
#	undef destroy
}

static expr1_t *append_row()
{
	expr1_t *e = calloc(sizeof(expr1_t), 1);

	if (gdl_first(&sdlg.wizard) != NULL) {
		e->and = gtk_label_new("AND");
		gtk_misc_set_alignment(GTK_MISC(e->and), -1, 0.);
		gtk_box_pack_start(GTK_BOX(sdlg.wizard_vbox), e->and, FALSE, FALSE, 0);
	}

	e->hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sdlg.wizard_vbox), e->hbox, FALSE, FALSE, 0);

	e->remove_row = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(e->remove_row), gtk_image_new_from_icon_name("gtk-delete", GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_box_pack_start(GTK_BOX(e->hbox), e->remove_row, FALSE, FALSE, 0);
	e->sig_remove_row = g_signal_connect(e->remove_row, "clicked", G_CALLBACK(remove_row_cb), e);
	gtk_widget_set_tooltip_text(e->remove_row, "Remove this row of expressions");


	e->append_col = gtk_button_new_with_label("");
	gtk_button_set_image(GTK_BUTTON(e->append_col), gtk_image_new_from_icon_name("gtk-add", GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_box_pack_start(GTK_BOX(e->hbox), e->append_col, FALSE, FALSE, 0);
	e->sig_append_col = g_signal_connect(e->append_col, "clicked", G_CALLBACK(new_col_cb), e);
	gtk_widget_set_tooltip_text(e->append_col, "Append an expression to this row with OR");

	e->spc = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(e->hbox), e->spc, FALSE, FALSE, 10);

	build_expr1(e, e->hbox);

	gdl_append(&sdlg.wizard, e, next_and);
	return e;
}

static expr1_t *append_col(expr1_t *row)
{
	expr1_t *e = calloc(sizeof(expr1_t), 1);

	e->or = gtk_label_new(" OR ");
	gtk_misc_set_alignment(GTK_MISC(e->or), -1, 1);
	gtk_box_pack_start(GTK_BOX(row->hbox), e->or, FALSE, FALSE, 0);

	build_expr1(e, row->hbox);

	gdl_append(&row->ors, e, next_or);
	e->row = row;
	return e;
}

static void remove_row(expr1_t *row)
{
	expr1_t *o;
	gdl_remove(&sdlg.wizard, row, next_and);
	for(o = gdl_first(&row->ors); o != NULL; o = gdl_next(&row->ors, o))
		destroy_expr1(o);
	destroy_expr1(row);

	/* the new first widget must not have an AND "preface" */
	o = gdl_first(&sdlg.wizard);
	if ((o != NULL) && (o->and != NULL)) {
		gtk_widget_destroy(o->and);
		o->and = NULL;
	}
}

static void remove_expr(expr1_t *e)
{
	if (e->row == NULL) {
		/* first item in a row */
		expr1_t *o, *o2 = gdl_first(&e->ors);
		if (o2 != NULL) {
			/* there are subsequent items in the row - have to make the first of them the new head */
			gdl_remove(&e->ors, o2, next_or);
			gdl_insert_before(&sdlg.wizard, e, o2, next_and);
			gdl_remove(&sdlg.wizard, e, next_and);
#			define inherit(dst, src, fld)  dst->fld = src->fld; src->fld = NULL;
			inherit(o2, e, and);
			inherit(o2, e, hbox);
			inherit(o2, e, remove_row);
			inherit(o2, e, append_col);
			inherit(o2, e, spc);
#			undef inherit
			/* move the list, reparenting it */
			memcpy(&o2->ors, &e->ors, sizeof(e->ors));
			for(o = gdl_first(&o2->ors); o != NULL; o = gdl_next(&o2->ors, o))
				o->next_or.parent = &o2->ors;

			o2->row = NULL;
			memset(&e->ors, 0, sizeof(e->ors));
			if (o2->or != NULL) {
				gtk_widget_destroy(o2->or);
				o2->or = NULL;
			}

			/* reconnect row signals to the new head */
			g_signal_handler_disconnect(o2->remove_row, e->sig_remove_row);
			g_signal_handler_disconnect(o2->append_col, e->sig_append_col);

			o2->sig_remove_row = g_signal_connect(o2->remove_row, "clicked", G_CALLBACK(remove_row_cb), o2);
			o2->sig_append_col = g_signal_connect(o2->append_col, "clicked", G_CALLBACK(new_col_cb), o2);
		}
		else {
			/* only item of the row */
			remove_row(e);
			return;
		}
	}
	else
		gdl_remove(&e->row->ors, e, next_or);
	destroy_expr1(e);
}


/* button callbacks */
static void new_row_cb(GtkWidget *button, void *data)
{
	append_row();
	gtk_widget_show_all(sdlg.window);
}

static void remove_row_cb(GtkWidget *button, void *data)
{
	expr1_t *row = (expr1_t *)data;
	remove_row(row);
	gtk_widget_show_all(sdlg.window);
}

static void remove_expr_cb(GtkWidget *button, void *data)
{
	remove_expr((expr1_t *)data);
	gtk_widget_show_all(sdlg.window);
}

static void edit_expr_cb(GtkWidget *button, void *data)
{
	expr_wizard_dialog((expr1_t *)data);
}

static void new_col_cb(GtkWidget *button, void *data)
{
	expr1_t *row = (expr1_t *)data;
	append_col(row);
	gtk_widget_show_all(sdlg.window);
}


/* Run the expression wizard dialog box */

static struct {
	GtkWidget *entry_left, *entry_right;
	GtkWidget *tr_left, *tr_op, *tr_right;
	GtkListStore *md_left;
} expr_wizard_dlg;

static GType model_op[2] = { G_TYPE_STRING, G_TYPE_POINTER };

typedef struct expr_wizard_op_s expr_wizard_op_t;
struct expr_wizard_op_s {
	const char **ops;
	GtkListStore *model;
};

typedef struct expr_wizard_s expr_wizard_t;
struct expr_wizard_s {
	const char *left_var;
	const char *left_desc;
	const expr_wizard_op_t *ops;
};

enum {
	OPS_ANY,
	OPS_EQ,
};

const char *ops_any[] = {"==", "!=", ">=", "<=", ">", "<", NULL};
const char *ops_eq[]  = {"==", "!=", NULL};

expr_wizard_op_t op_tab[] = {
	{ops_any, NULL},
	{ops_eq, NULL},
	{NULL, NULL}
};

static const expr_wizard_t expr_tab[] = {
	{"@.id",        "object ID",             &op_tab[OPS_ANY]},
	{"@.type",      "object type",           &op_tab[OPS_EQ]},
	{"@.bbox.x1",   "bounding box X1",       &op_tab[OPS_ANY]},
	{"@.bbox.y1",   "bounding box Y1",       &op_tab[OPS_ANY]},
	{"@.bbox.x2",   "bounding box X2",       &op_tab[OPS_ANY]},
	{"@.bbox.y2",   "bounding box Y2",       &op_tab[OPS_ANY]},
	{"@.bbox.w",    "bounding box width",    &op_tab[OPS_ANY]},
	{"@.bbox.h",    "bounding box height",   &op_tab[OPS_ANY]},
	{NULL, NULL}
};

static void expr_wizard_init_model()
{
	const expr_wizard_t *w;
	expr_wizard_op_t *o;

	if (expr_wizard_dlg.md_left != NULL)
		return;

	for(o = op_tab; o->ops != NULL; o++) {
		const char **s;
		o->model = gtk_list_store_newv(1, model_op);
		for(s = o->ops; *s != NULL; s++)
			gtk_list_store_insert_with_values(o->model, NULL, -1,  0, *s,  -1);
	}

	expr_wizard_dlg.md_left = gtk_list_store_newv(2, model_op);
	for(w = expr_tab; w->left_var != NULL; w++)
		gtk_list_store_insert_with_values(expr_wizard_dlg.md_left, NULL, -1,  0, w->left_desc,  1,w,  -1);
}

static void left_chg_cb(GtkTreeView *t, gpointer *data)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	const expr_wizard_t *w;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(t));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, 1, &w, -1);
	gtk_tree_view_set_model(GTK_TREE_VIEW(expr_wizard_dlg.tr_op), GTK_TREE_MODEL(w->ops->model));
}

static void expr_wizard_dialog(expr1_t *e)
{
	GtkWidget *dialog, *vbox, *hbox;
	GtkCellRenderer *renderer;
	gboolean response;

	expr_wizard_init_model();

	/* Create the dialog */
	dialog = gtk_dialog_new_with_buttons("Expression wizard",
	                                     GTK_WINDOW(gport->top_window),
	                                     GTK_DIALOG_MODAL,
	                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);

	/* left */
	vbox = gtk_vbox_new(FALSE, 4);
	expr_wizard_dlg.tr_left = gtk_tree_view_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(expr_wizard_dlg.tr_left), -1, "variable", renderer, "text", 0, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(expr_wizard_dlg.tr_left), GTK_TREE_MODEL(expr_wizard_dlg.md_left));
	g_signal_connect(G_OBJECT(expr_wizard_dlg.tr_left), "cursor-changed", G_CALLBACK(left_chg_cb), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), expr_wizard_dlg.tr_left, FALSE, TRUE, 4);



	expr_wizard_dlg.entry_left = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), expr_wizard_dlg.entry_left, FALSE, TRUE, 4);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 4);

	/* operator */
	vbox = gtk_vbox_new(FALSE, 4);

	expr_wizard_dlg.tr_op = gtk_tree_view_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(expr_wizard_dlg.tr_op), -1, "op", renderer, "text", 0, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(expr_wizard_dlg.tr_op), GTK_TREE_MODEL(op_tab[OPS_ANY].model));
	gtk_box_pack_start(GTK_BOX(vbox), expr_wizard_dlg.tr_op, FALSE, FALSE, 4);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

	/* right */
	vbox = gtk_vbox_new(FALSE, 4);

	expr_wizard_dlg.tr_right = gtk_tree_view_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(expr_wizard_dlg.tr_right), -1, "constant", renderer, "text", 0, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(expr_wizard_dlg.tr_right), GTK_TREE_MODEL(expr_wizard_dlg.md_left));
	gtk_box_pack_start(GTK_BOX(vbox), expr_wizard_dlg.tr_right, FALSE, TRUE, 4);

	expr_wizard_dlg.entry_right = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), expr_wizard_dlg.entry_right, FALSE, TRUE, 4);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 4);

	/* pack content */
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
	gtk_widget_show_all(dialog);

	/* Run the dialog */
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_OK) {
	}
	gtk_widget_destroy(dialog);
}

/* Advanced search window creation and administration */
static void ghid_search_window_create()
{
	GtkWidget *vbox_win, *hbox, *lab, *vbox;
	GtkWidget *content_area, *top_window = gport->top_window;
	const char *actions[] = { "select", "unselect", NULL };
	const char **s;

	/* make sure the list is empty */
	memset(&sdlg.wizard, 0, sizeof(sdlg.wizard));

	sdlg.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sdlg.window = gtk_dialog_new_with_buttons(_("Advanced search"),
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(sdlg.window));

	vbox_win = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), vbox_win);

	lab = gtk_label_new("Query expression:");
	gtk_box_pack_start(GTK_BOX(vbox_win), lab, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lab), -1, 0.);

/* expr entry */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_win), hbox, TRUE, TRUE, 0);


	sdlg.expr = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), sdlg.expr, TRUE, TRUE, 0);

	sdlg.action = gtk_combo_box_new_text();
	gtk_widget_set_tooltip_text(sdlg.action, "Do this with any object matching the query expression");
/*	g_signal_connect(G_OBJECT(sdlg.action), "changed", G_CALLBACK(action_changed_cb), NULL);*/
	for(s = actions; *s != NULL; s++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(sdlg.action), *s);
	gtk_box_pack_start(GTK_BOX(hbox), sdlg.action, TRUE, TRUE, 0);

/* */
	vbox = ghid_framed_vbox(vbox_win, "wizard", 1, TRUE, 4, 1);
	sdlg.wizard_enable = gtk_check_button_new_with_label("Enable wizard");
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.wizard_enable, TRUE, TRUE, 0);

	sdlg.wizard_vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.wizard_vbox, TRUE, TRUE, 4);

	sdlg.new_row = gtk_button_new_with_label("Add new row");
	g_signal_connect(sdlg.new_row, "clicked", G_CALLBACK(new_row_cb), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.new_row, TRUE, TRUE, 0);
	gtk_button_set_image(GTK_BUTTON(sdlg.new_row), gtk_image_new_from_icon_name("gtk-new", GTK_ICON_SIZE_MENU));
	gtk_widget_set_tooltip_text(sdlg.new_row, "Append a row of expressions to the query with AND");


/* Add one row of wizard to save a click in the most common case */
	append_row();

	gtk_widget_realize(sdlg.window);
}

void ghid_search_window_show(gboolean raise)
{
	ghid_search_window_create();
	gtk_widget_show_all(sdlg.window);
	wplc_place(WPLC_SEARCH, sdlg.window);
	if (raise)
		gtk_window_present(GTK_WINDOW(sdlg.window));
}
