/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "conf_core.h"
#include "dlg_attribute.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_dad_tree.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "compat.h"
#include "bu_box.h"
#include "bu_check_button.h"
#include "bu_spin_button.h"
#include "wt_coord_entry.h"

#define PCB_OBJ_PROP "pcb-rnd_context"

typedef struct {
	pcb_hid_attribute_t *attrs;
	pcb_hid_attr_val_t *results;
	GtkWidget **wl;
	int n_attrs;
	void *caller_data;
	GtkWidget *dialog;
	int rc;
	pcb_hid_attr_val_t property[PCB_HATP_max];
	unsigned inhibit_valchg:1;
} attr_dlg_t;

#define change_cb(ctx, dst) \
	do { \
		if (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func != NULL) \
			ctx->property[PCB_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, dst); \
		if (dst->change_cb != NULL) \
			dst->change_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

static void set_flag_cb(GtkToggleButton *button, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);
	pcb_hid_attribute_t *dst = user_data;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.int_value = gtk_toggle_button_get_active(button);
	change_cb(ctx, dst);
}

static void intspinner_changed_cb(GtkSpinButton *spin_button, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(spin_button), PCB_OBJ_PROP);
	pcb_hid_attribute_t *dst = user_data;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;
	dst->default_val.int_value = gtk_spin_button_get_value(spin_button);
	change_cb(ctx, dst);
}

static void coordentry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);
	const char *s;
	pcb_coord_t crd;
	double crdd;
	const pcb_unit_t *u;
	pcb_bool succ;

	s = gtk_entry_get_text(entry);
	succ = pcb_get_value_unit(s, NULL, 1, &crdd, &u);

	if (!succ)
		return;

	dst->changed = 1;

	if (ctx->inhibit_valchg)
		return;

	crd = crdd;
	pcb_gtk_coord_entry_set_unit(GHID_COORD_ENTRY(entry), u);

	if ((dst->default_val.coord_value != crd) && (crd >= dst->min_val) && (crd <= dst->max_val)) {
		dst->default_val.coord_value = crd;
		change_cb(ctx, dst);
	}
}

static void dblspinner_changed_cb(GtkSpinButton *spin_button, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(spin_button), PCB_OBJ_PROP);
	pcb_hid_attribute_t *dst = user_data;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.real_value = gtk_spin_button_get_value(spin_button);
	change_cb(ctx, dst);
}

static void entry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	free((char *)dst->default_val.str_value);
	dst->default_val.str_value = pcb_strdup(gtk_entry_get_text(entry));
	change_cb(ctx, dst);
}

static void enum_changed_cb(GtkComboBox *combo_box, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(combo_box), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.int_value = gtk_combo_box_get_active(combo_box);
	change_cb(ctx, dst);
}

static void notebook_changed_cb(GtkNotebook *nb, GtkWidget *page, guint page_num, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(nb), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	/* Gets the index (starting from 0) of the current page in the notebook. If
	   the notebook has no pages, then -1 will be returned and no call-back occur. */
	if (gtk_notebook_get_current_page(nb) >= 0) {
		dst->default_val.int_value = page_num;
		change_cb(ctx, dst);
	}
}

static void button_changed_cb(GtkButton *button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	change_cb(ctx, dst);
}

typedef struct {
	void (*cb)(void *ctx, pcb_hid_attr_ev_t ev);
	void *ctx;
} resp_ctx_t;

static void ghid_attr_dlg_response_cb(GtkDialog *dialog, gint response_id, gpointer user_data)
{
	resp_ctx_t *ctx = (resp_ctx_t *)user_data;

	if ((ctx != NULL) && (ctx->cb != NULL)) {
		switch (response_id) {
		case GTK_RESPONSE_OK:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_OK);
			break;
		case GTK_RESPONSE_CANCEL:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_CANCEL);
			break;
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_DELETE_EVENT:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_WINCLOSE);
			break;
		}
	}
	free(ctx);
}


static GtkWidget *frame_scroll(GtkWidget *parent, pcb_hatt_compflags_t flags)
{
	GtkWidget *fr;

	if (flags & PCB_HATF_FRAME) {
		fr = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(parent), fr, FALSE, FALSE, 0);

		parent = gtkc_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(fr), parent);
	}
	if (flags & PCB_HATF_SCROLL) {
		fr = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(parent), fr, TRUE, TRUE, 0);
		parent = gtkc_hbox_new(FALSE, 0);
		gtkc_scrolled_window_add_with_viewport(fr, parent);
	}
	return parent;
}

static GtkTreeIter *ghid_treetable_add(pcb_hid_attribute_t *attr, GtkTreeStore *tstore, GtkTreeIter *par, pcb_hid_row_t *r, int prepend)
{
	int c;
	GtkTreeIter *curr = malloc(sizeof(GtkTreeIter));

	if (prepend)
		gtk_tree_store_prepend(tstore, curr, par);
	else
		gtk_tree_store_append(tstore, curr, par);

	for(c = 0; c < attr->pcb_hatt_table_cols; c++) {
		GValue v = G_VALUE_INIT;
		g_value_init(&v, G_TYPE_STRING);
		g_value_set_string(&v, r->cell[c]);
		gtk_tree_store_set_value(tstore, curr, c, &v);
	}

	/* remember the dad row in the hidden last cell */
	{
		GValue v = G_VALUE_INIT;
		g_value_init(&v, G_TYPE_POINTER);
		g_value_set_pointer(&v, r);
		gtk_tree_store_set_value(tstore, curr, c, &v);
	}

	r->hid_data = curr;
	return curr;
}

/* insert a subtree of a tree-table widget in a gtk table store reursively */
static void ghid_treetable_import(pcb_hid_attribute_t *attr, GtkTreeStore *tstore, gdl_list_t *lst, GtkTreeIter *par)
{
	pcb_hid_row_t *r;

	for(r = gdl_first(lst); r != NULL; r = gdl_next(lst, r)) {
		GtkTreeIter *curr = ghid_treetable_add(attr, tstore, par, r, 0);
		ghid_treetable_import(attr, tstore, &r->children, curr);
	}
}

static void ghid_treetable_insert_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *new_row)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *tt = ctx->wl[idx];
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tt));
	pcb_hid_row_t *par = pcb_dad_tree_parent_row((pcb_hid_tree_t *)attrib->enumerations, new_row);

#warning TODO: decide whether to insert or append
	ghid_treetable_add(attrib, GTK_TREE_STORE(model), (par == NULL ? NULL : par->hid_data), new_row, 1);
}

static void ghid_treetable_free_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	free(row->hid_data);
}

static void ghid_treetable_cursor(GtkWidget *tree, pcb_hid_attribute_t *attr)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	pcb_hid_row_t *r;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, attr->pcb_hatt_table_cols, &r, -1);
	pcb_trace("Select: %p %s\n", r, r->cell[0]);
}

typedef struct {
	enum {
		TB_TABLE,
		TB_TABBED
	} type;
	union {
		struct {
			int cols, rows;
			int col, row;
		} table;
		struct {
			const char **tablab;
		} tabbed;
	} val;
} ghid_attr_tb_t;

static int ghid_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, ghid_attr_tb_t *tb_st, int start_from, int add_labels)
{
	int j, i, n;
	GtkWidget *combo, *widget, *entry, *vbox1, *hbox, *bparent, *parent, *tbl;

	for (j = start_from; j < ctx->n_attrs; j++) {
		const pcb_unit_t *unit_list;
		if (ctx->attrs[j].help_text == ATTR_UNDOCUMENTED)
			continue;
		if (ctx->attrs[j].type == PCB_HATT_END)
			break;

		/* if we are filling a table, allocate parent boxes in row-major */
		if (tb_st != NULL) {
			switch(tb_st->type) {
				case TB_TABLE:
					parent = gtkc_vbox_new(FALSE, 0);
					gtkc_table_attach1(real_parent, parent, tb_st->val.table.row, tb_st->val.table.col);
					tb_st->val.table.col++;
					if (tb_st->val.table.col >= tb_st->val.table.cols) {
						tb_st->val.table.col = 0;
						tb_st->val.table.row++;
					}
					break;
				case TB_TABBED:
					/* Add a new notebook page with an empty vbox, using tab label present in enumerations. */
					parent = gtkc_vbox_new(FALSE, 4);
					if (*tb_st->val.tabbed.tablab) {
						widget = gtk_label_new(*tb_st->val.tabbed.tablab);
						tb_st->val.tabbed.tablab++;
					}
					else
						widget = NULL;
					gtk_notebook_append_page(GTK_NOTEBOOK(real_parent), parent, widget);
					break;
			}
		}
		else
			parent = real_parent;

		/* create the actual widget from attrs */
		switch (ctx->attrs[j].type) {
			case PCB_HATT_BEGIN_HBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), hbox, FALSE, FALSE, 0);
				ctx->wl[j] = hbox;
				j = ghid_attr_dlg_add(ctx, hbox, NULL, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_VBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
				vbox1 = gtkc_vbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), vbox1, FALSE, FALSE, 0);
				ctx->wl[j] = vbox1;
				j = ghid_attr_dlg_add(ctx, vbox1, NULL, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_TABLE:
				{
					ghid_attr_tb_t ts;
					bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
					ts.type = TB_TABLE;
					ts.val.table.cols = ctx->attrs[j].pcb_hatt_table_cols;
					ts.val.table.rows = pcb_hid_attrdlg_num_children(ctx->attrs, j+1, ctx->n_attrs) / ts.val.table.cols;
					ts.val.table.col = 0;
					ts.val.table.row = 0;
					tbl = gtkc_table_static(ts.val.table.rows, ts.val.table.cols, 1);
					gtk_box_pack_start(GTK_BOX(bparent), tbl, FALSE, FALSE, 0);
					ctx->wl[j] = tbl;
					j = ghid_attr_dlg_add(ctx, tbl, &ts, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				}
				break;

			case PCB_HATT_BEGIN_TABBED:
				{
					ghid_attr_tb_t ts;
					ts.type = TB_TABBED;
					ts.val.tabbed.tablab = ctx->attrs[j].enumerations;
					ctx->wl[j] = widget = gtk_notebook_new();
					if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LEFT_TAB)
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_LEFT);
					else
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_TOP);
					gtk_box_pack_start(GTK_BOX(parent), widget, FALSE, FALSE, 0);
					g_signal_connect(G_OBJECT(widget), "switch-page", G_CALLBACK(notebook_changed_cb), &(ctx->attrs[j]));
					g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
					j = ghid_attr_dlg_add(ctx, widget, &ts, j+1, 0);
				}
				break;

			case PCB_HATT_LABEL:
				ctx->wl[j] = widget = gtk_label_new(ctx->attrs[j].name);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);

				gtk_box_pack_start(GTK_BOX(parent), widget, FALSE, FALSE, 0);
				gtk_misc_set_alignment(GTK_MISC(widget), 0., 0.5);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				break;

			case PCB_HATT_INTEGER:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				/*
				 * FIXME
				 * need to pick the "digits" argument based on min/max
				 * values
				 */
				ghid_spin_button(hbox, &widget, ctx->attrs[j].default_val.int_value,
												 ctx->attrs[j].min_val, ctx->attrs[j].max_val, 1.0, 1.0, 0, 0,
												 intspinner_changed_cb, &(ctx->attrs[j]), FALSE, NULL);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_COORD:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = pcb_gtk_coord_entry_new(ctx->attrs[j].min_val, ctx->attrs[j].max_val,
																				ctx->attrs[j].default_val.coord_value, conf_core.editor.grid_unit, CE_SMALL);
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(coordentry_changed_cb), &(ctx->attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_REAL:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				/*
				 * FIXME
				 * need to pick the "digits" and step size argument more
				 * intelligently
				 */
				ghid_spin_button(hbox, &widget, ctx->attrs[j].default_val.real_value,
												 ctx->attrs[j].min_val, ctx->attrs[j].max_val, 0.01, 0.01, 3,
												 0, dblspinner_changed_cb, &(ctx->attrs[j]), FALSE, NULL);

				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_STRING:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_BOOL:
				/* put this in a check button */
				pcb_gtk_check_button_connected(parent, &widget,
                                       ctx->attrs[j].default_val.int_value,
                                       TRUE, FALSE, FALSE, 0, set_flag_cb, &(ctx->attrs[j]), (add_labels ? ctx->attrs[j].name : NULL));
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;
				break;

			case PCB_HATT_ENUM:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(combo), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				i = 0;
				while (ctx->attrs[j].enumerations[i]) {
					gtkc_combo_box_text_append_text(combo, ctx->attrs[j].enumerations[i]);
					i++;
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				break;

			case PCB_HATT_TREE:
				{
					int c;
					const char **colhdr;
					GtkWidget *view = gtk_tree_view_new();
					GtkTreeModel *model;
					GtkTreeStore *tstore;
					GType *types;
					GtkCellRenderer *renderer;
					GtkTreeSelection *selection;
					pcb_hid_tree_t *tree = (pcb_hid_tree_t *)ctx->attrs[j].enumerations;

					tree->insert_cb = ghid_treetable_insert_cb;
					tree->free_cb = ghid_treetable_free_cb;
					tree->hid_ctx = ctx;

					hbox = gtkc_hbox_new(FALSE, 4);
					gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

					/* create columns */
					types = malloc(sizeof(GType) * (ctx->attrs[j].pcb_hatt_table_cols+1));
					colhdr = tree->hdr;
					for(c = 0; c < ctx->attrs[j].pcb_hatt_table_cols; c++) {
						GtkTreeViewColumn *col = gtk_tree_view_column_new();
						if (tree->hdr != NULL) {
							gtk_tree_view_column_set_title(col, *colhdr == NULL ? "" : *colhdr);
							if (*colhdr != NULL)
								colhdr++;
						}
						gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
						renderer = gtk_cell_renderer_text_new();
						gtk_tree_view_column_pack_start(col, renderer, TRUE);
						gtk_tree_view_column_add_attribute(col, renderer, "text", c);
						types[c] = G_TYPE_STRING;
					}

					/* append a hidden row for storing the dad row pointer */
					types[c] = G_TYPE_POINTER;

					/* import existing data */
					tstore = gtk_tree_store_newv(ctx->attrs[j].pcb_hatt_table_cols+1, types);
					free(types);
					ghid_treetable_import(&ctx->attrs[j], tstore, &tree->rows, NULL);
					model = GTK_TREE_MODEL(tstore);
					gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
					g_object_unref(model); /* destroy model automatically with view */
					gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_NONE);

					g_object_set(view, "rules-hint", TRUE, "headers-visible", (tree->hdr != NULL));
					g_signal_connect(G_OBJECT(view), "cursor-changed", G_CALLBACK(ghid_treetable_cursor), &ctx->attrs[j]);
					selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
					gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

					gtk_widget_set_tooltip_text(view, ctx->attrs[j].help_text);
					gtk_box_pack_start(GTK_BOX(hbox), view, FALSE, FALSE, 0);
					g_object_set_data(G_OBJECT(view), PCB_OBJ_PROP, ctx);
					ctx->wl[j] = view;
				}
				break;

			case PCB_HATT_PATH:
				vbox1 = ghid_category_vbox(parent, (add_labels ? ctx->attrs[j].name : NULL), 4, 2, TRUE, TRUE);
				entry = gtk_entry_new();
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				gtk_box_pack_start(GTK_BOX(vbox1), entry, FALSE, FALSE, 0);
				gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));

				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				break;

			case PCB_HATT_UNIT:
				unit_list = pcb_units;
				n = pcb_get_n_units();

				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(combo), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				for (i = 0; i < n; ++i)
					gtkc_combo_box_text_append_text(combo, unit_list[i].in_suffix);
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;
			case PCB_HATT_BUTTON:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				ctx->wl[j] = gtk_button_new_with_label(ctx->attrs[j].default_val.str_value);
				gtk_box_pack_start(GTK_BOX(hbox), ctx->wl[j], FALSE, FALSE, 0);

				gtk_widget_set_tooltip_text(ctx->wl[j], ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "clicked", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), PCB_OBJ_PROP, ctx);

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			default:
				printf("ghid_attribute_dialog: unknown type of HID attribute\n");
				break;
		}
	}
	return j;
}

static int ghid_attr_dlg_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	int save;
	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;

	/* create the actual widget from attrs */
	switch (ctx->attrs[idx].type) {
		case PCB_HATT_BEGIN_HBOX:
		case PCB_HATT_BEGIN_VBOX:
		case PCB_HATT_BEGIN_TABLE:
		case PCB_HATT_END:
			goto error;

		case PCB_HATT_LABEL:
			{
				const char *txt = gtk_label_get_text(GTK_LABEL(ctx->wl[idx]));
				if (strcmp(txt, val->str_value) == 0)
					goto nochg;
				gtk_label_set_text(GTK_LABEL(ctx->wl[idx]), val->str_value);
			}
			break;

		case PCB_HATT_INTEGER:
			{
				double d = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->wl[idx]));
				if (val->int_value == (int)d)
					goto nochg;
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->wl[idx]), val->int_value);
			}
			break;

		case PCB_HATT_COORD:
			{
				pcb_coord_t crd = pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(ctx->wl[idx]));
				if (crd == val->coord_value)
					goto nochg;
				pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(ctx->wl[idx]), val->coord_value);
			}
			break;

		case PCB_HATT_REAL:
			{
				double d = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctx->wl[idx]));
				if (val->real_value == d)
					goto nochg;
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->wl[idx]), val->real_value);
			}
			break;

		case PCB_HATT_STRING:
		case PCB_HATT_PATH:
			{
				const char *s = gtk_entry_get_text(GTK_ENTRY(ctx->wl[idx]));
				if (strcmp(s, val->str_value) == 0)
					goto nochg;
				gtk_entry_set_text(GTK_ENTRY(ctx->wl[idx]), val->str_value);
			}
			break;

		case PCB_HATT_BOOL:
			{
				int chk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]));
				if (chk == val->int_value)
					goto nochg;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), val->int_value);
			}
			break;

		case PCB_HATT_ENUM:
			{
				int en = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->wl[idx]));
				if (en == val->int_value)
					goto nochg;
				gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->wl[idx]), val->int_value);
			}
			break;

		case PCB_HATT_UNIT:
			abort();
			break;

		case PCB_HATT_BUTTON:
			{
				const char *s = gtk_button_get_label(GTK_BUTTON(ctx->wl[idx]));
				if (strcmp(s, val->str_value) == 0)
					goto nochg;
				gtk_button_set_label(GTK_BUTTON(ctx->wl[idx]), val->str_value);
			}
			break;

		case PCB_HATT_BEGIN_TABBED:
			gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->wl[idx]), val->int_value);
			break;
	}

	ctx->inhibit_valchg = save;
	return 0;

	error:;
	ctx->inhibit_valchg = save;
	return -1;

	nochg:;
	ctx->inhibit_valchg = save;
	return 1;
}

void *ghid_attr_dlg_new(GtkWidget *top_window, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
{
	GtkWidget *content_area;
	GtkWidget *main_vbox, *vbox;
	attr_dlg_t *ctx;
	resp_ctx_t *resp_ctx = NULL;

	if (button_cb != NULL) {
		resp_ctx = malloc(sizeof(resp_ctx_t));
		resp_ctx->cb = button_cb;
		resp_ctx->ctx = caller_data;
	}

	ctx = calloc(sizeof(attr_dlg_t), 1);
	ctx->attrs = attrs;
	ctx->results = results;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;

	ctx->dialog = gtk_dialog_new_with_buttons(_(title),
																			 GTK_WINDOW(top_window),
																			 (GtkDialogFlags) ((modal?GTK_DIALOG_MODAL:0)
																												 | GTK_DIALOG_DESTROY_WITH_PARENT),
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_role(GTK_WINDOW(ctx->dialog), "PCB_attribute_editor");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
	g_signal_connect(ctx->dialog, "response", G_CALLBACK(ghid_attr_dlg_response_cb), resp_ctx);

	main_vbox = gtkc_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	gtk_container_add_with_properties(GTK_CONTAINER(content_area), main_vbox, "expand", TRUE, "fill", TRUE, NULL);

	if (!PCB_HATT_IS_COMPOSITE(attrs[0].type)) {
		vbox = ghid_category_vbox(main_vbox, descr != NULL ? descr : "", 4, 2, TRUE, TRUE);
		ghid_attr_dlg_add(ctx, vbox, NULL, 0, 1);
	}
	else
		ghid_attr_dlg_add(ctx, main_vbox, NULL, 0, (attrs[0].pcb_hatt_flags & PCB_HATF_LABEL));

	gtk_widget_show_all(ctx->dialog);

	return ctx;
}

int ghid_attr_dlg_run(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	if (gtk_dialog_run(GTK_DIALOG(ctx->dialog)) == GTK_RESPONSE_OK)
		ctx->rc = 0;
	else
		ctx->rc = 1;
	return ctx->rc;
}

void ghid_attr_dlg_free(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;

	if (ctx->rc == 0) { /* copy over the results */
		int i;
		for (i = 0; i < ctx->n_attrs; i++) {
			ctx->results[i] = ctx->attrs[i].default_val;
			if (PCB_HAT_IS_STR(ctx->attrs[i].type) && (ctx->results[i].str_value))
				ctx->results[i].str_value = pcb_strdup(ctx->results[i].str_value);
			else
				ctx->results[i].str_value = NULL;
		}
	}

	gtk_widget_destroy(ctx->dialog);
	free(ctx->wl);
}

void ghid_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((prop >= 0) && (prop < PCB_HATP_max))
		ctx->property[prop] = *val;
}


int ghid_attribute_dialog(GtkWidget * top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data)
{
	void *hid_ctx;
	int rc;

	hid_ctx = ghid_attr_dlg_new(top_window, attrs, n_attrs, results, title, descr, caller_data, pcb_true, NULL);
	rc = ghid_attr_dlg_run(hid_ctx);
	ghid_attr_dlg_free(hid_ctx);

	return rc;
}


int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	gtk_widget_set_sensitive(ctx->wl[idx], enabled);

	return 0;
}

int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (hide)
		gtk_widget_hide(ctx->wl[idx]);
	else
		gtk_widget_show(ctx->wl[idx]);

	return 0;
}

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;
	int res;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	res = ghid_attr_dlg_set(ctx, idx, val);

	if (res == 0) {
		ctx->attrs[idx].default_val = *val;
		return 0;
	}
	else if (res == 1)
		return 0; /* no change */

	return -1;
}
