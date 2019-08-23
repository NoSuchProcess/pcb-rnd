/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Alain Vigne
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <gdk/gdkkeysyms.h>

static GtkTreeIter *ghid_tree_table_add(pcb_hid_attribute_t *attr, GtkTreeStore *tstore, GtkTreeIter *par, pcb_hid_row_t *r, int prepend, GtkTreeIter *sibling)
{
	int c;
	GtkTreeIter *curr = malloc(sizeof(GtkTreeIter));

	if (sibling == NULL) {
		if (prepend)
			gtk_tree_store_prepend(tstore, curr, par);
		else
			gtk_tree_store_append(tstore, curr, par);
	}
	else {
		if (prepend)
			gtk_tree_store_insert_before(tstore, curr, par, sibling);
		else
			gtk_tree_store_insert_after(tstore, curr, par, sibling);
	}

	for(c = 0; c < attr->pcb_hatt_table_cols; c++) {
		GValue v = G_VALUE_INIT;
		g_value_init(&v, G_TYPE_STRING);
		if (c < r->cols)
			g_value_set_string(&v, r->cell[c]);
		else
			g_value_set_string(&v, "");
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
static void ghid_tree_table_import(pcb_hid_attribute_t *attr, GtkTreeStore *tstore, gdl_list_t *lst, GtkTreeIter *par)
{
	pcb_hid_row_t *r;

	for(r = gdl_first(lst); r != NULL; r = gdl_next(lst, r)) {
		GtkTreeIter *curr = ghid_tree_table_add(attr, tstore, par, r, 0, NULL);
		ghid_tree_table_import(attr, tstore, &r->children, curr);
	}
}

/* Return the tree-store data model, which is the child of the filter model
   attached to the tree widget */
GtkTreeModel *ghid_tree_table_get_model(attr_dlg_t *ctx, pcb_hid_attribute_t *attrib, int filter)
{
	int idx = attrib - ctx->attrs;
	GtkWidget *tt = ctx->wl[idx];
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tt));

	if (filter)
		return model;

	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
	return model;
}

static void ghid_tree_table_insert_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *new_row)
{
	attr_dlg_t *ctx = hid_ctx;
	pcb_hid_row_t *sibling, *par = pcb_dad_tree_parent_row(attrib->wdata, new_row);
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attrib, 0);
	GtkTreeIter *sibiter, *pariter;
	int prepnd;

	sibling = gdl_prev(new_row->link.parent, new_row);
	if (sibling == NULL) {
		sibling = gdl_next(new_row->link.parent, new_row);
		prepnd = 1;
	}
	else
		prepnd = 0;

	pariter = par == NULL ? NULL : par->hid_data;
	sibiter = sibling == NULL ? NULL : sibling->hid_data;
	ghid_tree_table_add(attrib, GTK_TREE_STORE(model), pariter, new_row, prepnd, sibiter);
}

static void ghid_tree_table_remove_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	attr_dlg_t *ctx = hid_ctx;
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attrib, 0);
	gtk_tree_store_remove(GTK_TREE_STORE(model), (GtkTreeIter *)row->hid_data);
	row->hid_data = NULL;
}

static void ghid_tree_table_modify_cb(pcb_hid_attribute_t *attr, void *hid_ctx, pcb_hid_row_t *row, int col)
{
	attr_dlg_t *ctx = hid_ctx;
	GtkTreeIter *iter = row->hid_data;
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attr, 0);
	GValue v = G_VALUE_INIT;

	g_value_init(&v, G_TYPE_STRING);

	if (col < 0) {
		for(col = 0; col < attr->pcb_hatt_table_cols; col++) {
			g_value_set_string(&v, row->cell[col]);
			gtk_tree_store_set_value(GTK_TREE_STORE(model), iter, col, &v);
		}
	}
	else {
		g_value_set_string(&v, row->cell[col]);
		gtk_tree_store_set_value(GTK_TREE_STORE(model), iter, col, &v);
	}
}

static void ghid_tree_table_free_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	free(row->hid_data);
}

static void ghid_tree_table_update_hide(pcb_hid_attribute_t *attrib, void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attrib, 1);
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));
}


static pcb_hid_row_t *ghid_tree_table_get_selected(pcb_hid_attribute_t *attrib, void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *tt = ctx->wl[idx];
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	pcb_hid_row_t *r;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tt));
	if (tsel == NULL)
		return NULL;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return NULL;

	gtk_tree_model_get(tm, &iter, attrib->pcb_hatt_table_cols, &r, -1);
	return r;
}

static void ghid_tree_table_cursor(GtkWidget *widget, pcb_hid_attribute_t *attr)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(widget), PCB_OBJ_PROP);
	pcb_hid_row_t *r = ghid_tree_table_get_selected(attr, ctx);
	pcb_hid_tree_t *tree = attr->wdata;

	attr->changed = 1;
	if (ctx->inhibit_valchg)
		return;
	if (r != NULL)
		attr->val.str = r->path;
	else
		attr->val.str = NULL;
	change_cb(ctx, attr);
	if (tree->user_selected_cb != NULL)
		tree->user_selected_cb(attr, ctx, r);
}

static gboolean tree_table_filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, void *user_data)
{
	pcb_hid_attribute_t *attr = user_data;
	pcb_hid_row_t *r = NULL;

	gtk_tree_model_get(model, iter, attr->pcb_hatt_table_cols, &r, -1);

	if (r == NULL)
		return TRUE; /* should not happen; when it does, it's a bug - better make the row visible */

	return !r->hide;
}

/* Activation (e.g. double-clicking) of a footprint row. As a convenience
to the user, GTK provides Shift-Arrow Left, Right to expand or
contract any node with children. */
static void tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, pcb_hid_attribute_t *attr)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	if (gtk_tree_view_row_expanded(tree_view, path))
		gtk_tree_view_collapse_row(tree_view, path);
	else
		gtk_tree_view_expand_row(tree_view, path, FALSE);
}

/* Key pressed activation handler: CTRL-C -> copy footprint name to clipboard;
   Enter -> row-activate. */
static gboolean ghid_tree_table_key_press_cb(GtkTreeView *tree_view, GdkEventKey *event, pcb_hid_attribute_t *attr)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	pcb_bool key_handled, arrow_key, enter_key, force_activate = pcb_false;
	GtkClipboard *clipboard;
	guint default_mod_mask = gtk_accelerator_get_default_mod_mask();

	arrow_key = ((event->keyval == PCB_GTK_KEY(Up)) || (event->keyval == PCB_GTK_KEY(KP_Up))
		|| (event->keyval == PCB_GTK_KEY(Down)) || (event->keyval == PCB_GTK_KEY(KP_Down))
		|| (event->keyval == PCB_GTK_KEY(KP_Page_Down)) || (event->keyval == PCB_GTK_KEY(KP_Page_Up))
		|| (event->keyval == PCB_GTK_KEY(Page_Down)) || (event->keyval == PCB_GTK_KEY(Page_Up))
		|| (event->keyval == PCB_GTK_KEY(KP_Home)) || (event->keyval == PCB_GTK_KEY(KP_End))
		|| (event->keyval == PCB_GTK_KEY(Home)) || (event->keyval == PCB_GTK_KEY(End)));
	enter_key = (event->keyval == PCB_GTK_KEY(Return)) || (event->keyval == PCB_GTK_KEY(KP_Enter));
	key_handled = (enter_key || arrow_key);

	/* Handle ctrl+c and ctrl+C: copy current name to clipboard */
	if (((event->state & default_mod_mask) == GDK_CONTROL_MASK) && ((event->keyval == PCB_GTK_KEY(c)) || (event->keyval == PCB_GTK_KEY(C)))) {
		pcb_hid_tree_t *tree = attr->wdata;
		pcb_hid_row_t *r;
		const char *cliptext;

		selection = gtk_tree_view_get_selection(tree_view);
		g_return_val_if_fail(selection != NULL, TRUE);

		if (!gtk_tree_selection_get_selected(selection, &model, &iter))
			return TRUE;

		gtk_tree_model_get(model, &iter, attr->pcb_hatt_table_cols, &r, -1);
		if (r == NULL)
			return TRUE;

		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		g_return_val_if_fail(clipboard != NULL, TRUE);

		if (tree->user_copy_to_clip_cb != NULL)
			cliptext = tree->user_copy_to_clip_cb(attr, tree->hid_wdata, r);
		else
			cliptext = r->cell[0];

		gtk_clipboard_set_text(clipboard, cliptext, -1);

		return FALSE;
	}

	if (!key_handled)
		return FALSE;

	/* If arrows (up or down), let GTK process the selection change. Then activate the new selected row. */
	if (arrow_key) {
		GtkWidgetClass *class = GTK_WIDGET_GET_CLASS(tree_view);

		class->key_press_event(GTK_WIDGET(tree_view), event);
	}

	selection = gtk_tree_view_get_selection(tree_view);
	g_return_val_if_fail(selection != NULL, TRUE);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return TRUE;

	/* arrow key should activate the row only if it's a leaf (real footprint), for
	   display, but shouldn't open/close levels visited or pop up the parametric
	   footprint dialog */
	if (arrow_key) {
		pcb_hid_row_t *r;

		gtk_tree_model_get(model, &iter, attr->pcb_hatt_table_cols, &r, -1);
		if (r != NULL) {
			pcb_hid_tree_t *tree = attr->wdata;
			if (tree->user_browse_activate_cb != NULL) { /* let the user callbakc decide */
				force_activate = tree->user_browse_activate_cb(attr, tree->hid_wdata, r);
			}
			else { /* automatic decision: only leaf nodes activate */
				if (gdl_first(&r->children) == NULL)
					force_activate = pcb_true;
			}
		}
	}

	/* Handle 'Enter' key and arrow keys as "activate" on plain footprints */
	if (enter_key || force_activate) {
		path = gtk_tree_model_get_path(model, &iter);
		if (path != NULL) {
			tree_row_activated(tree_view, path, NULL, attr);
		}
		gtk_tree_path_free(path);
	}

	return TRUE;
}

/* Handle the double-click to be equivalent to "row-activated" signal */
static gboolean ghid_tree_table_button_press_cb(GtkWidget *widget, GdkEvent *ev, pcb_hid_attribute_t *attr)
{
	GtkTreeView *tv = GTK_TREE_VIEW(widget);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	if (ev->button.type == GDK_2BUTTON_PRESS) {
		model = gtk_tree_view_get_model(tv);
		gtk_tree_view_get_path_at_pos(tv, ev->button.x, ev->button.y, &path, NULL, NULL, NULL);
		if (path != NULL) {
			gtk_tree_model_get_iter(model, &iter, path);
			tree_row_activated(tv, path, NULL, attr);
		}
	}

	return FALSE;
}

static gboolean ghid_tree_table_button_release_cb(GtkWidget *widget, GdkEvent *ev, pcb_hid_attribute_t *attr)
{
	GtkTreeView *tv = GTK_TREE_VIEW(widget);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	model = gtk_tree_view_get_model(tv);
	gtk_tree_view_get_path_at_pos(tv, ev->button.x, ev->button.y, &path, NULL, NULL, NULL);
	if (path != NULL) {
		gtk_tree_model_get_iter(model, &iter, path);
		/* Do not activate the row if LEFT-click on a "parent category" row. */
		if (ev->button.button != 1 || !gtk_tree_model_iter_has_child(model, &iter))
			tree_row_activated(tv, path, NULL, attr);
	}

	return FALSE;
}

static int ghid_tree_table_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	GtkWidget *tt = ctx->wl[idx];
	pcb_hid_attribute_t *attr = &ctx->attrs[idx];
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attr, 0);
	GtkTreePath *path;
	pcb_hid_tree_t *tree = attr->wdata;
	pcb_hid_row_t *r;
	const char *s = val->str;

	if ((s == NULL) || (*s == '\0')) {
TODO("DAD: remove cursor - is it possible with gtk?")
		return 0;
	}

	while(*s == '/') s++;
	r = htsp_get(&tree->paths, s);
	if (r == NULL)
		return -1;

	path = gtk_tree_model_get_path(model, r->hid_data);
	if (path == NULL)
		return -1;

	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tt), path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(tt), path, NULL, FALSE);
	return 0;
}

void ghid_tree_table_jumpto_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *tt = ctx->wl[idx];
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attrib, 0);
	GtkTreePath *path;

	if (row == NULL) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tt), NULL, NULL, FALSE);
		return;
	}

	path = gtk_tree_model_get_path(model, row->hid_data);
	if (path == NULL) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tt), NULL, NULL, FALSE);
		return;
	}

	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tt), path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(tt), path, NULL, FALSE);
}

void ghid_tree_table_expcoll_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row, int expanded)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *tt = ctx->wl[idx];
	GtkTreeModel *model = ghid_tree_table_get_model(ctx, attrib, 0);
	GtkTreePath *path;

	if (row == NULL)
		return;

	path = gtk_tree_model_get_path(model, row->hid_data);
	if (path == NULL)
		return;

	if (expanded) {
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tt), path);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(tt), path, 0);
	}
	else
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(tt), path);
}


static GtkWidget *ghid_tree_table_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	int c;
	const char **colhdr;
	GtkWidget *bparent, *view = gtk_tree_view_new();
	GtkTreeModel *model;
	GtkTreeStore *tstore;
	GType *types;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	pcb_hid_tree_t *tree = attr->wdata;

	tree->hid_insert_cb = ghid_tree_table_insert_cb;
	tree->hid_remove_cb = ghid_tree_table_remove_cb;
	tree->hid_modify_cb = ghid_tree_table_modify_cb;
	tree->hid_jumpto_cb = ghid_tree_table_jumpto_cb;
	tree->hid_expcoll_cb = ghid_tree_table_expcoll_cb;
	tree->hid_free_cb = ghid_tree_table_free_cb;
	tree->hid_get_selected_cb = ghid_tree_table_get_selected;
	tree->hid_update_hide_cb = ghid_tree_table_update_hide;
	tree->hid_wdata = ctx;

	/* create columns */
	types = malloc(sizeof(GType) * (attr->pcb_hatt_table_cols+1));
	colhdr = tree->hdr;
	for(c = 0; c < attr->pcb_hatt_table_cols; c++) {
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
	tstore = gtk_tree_store_newv(attr->pcb_hatt_table_cols+1, types);
	free(types);
	ghid_tree_table_import(attr, tstore, &tree->rows, NULL);

	model = GTK_TREE_MODEL(tstore);
	model = (GtkTreeModel *)g_object_new(GTK_TYPE_TREE_MODEL_FILTER, "child-model", model, "virtual-root", NULL, NULL);
	gtk_tree_model_filter_set_visible_func((GtkTreeModelFilter *) model, tree_table_filter_visible_func, attr, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model); /* destroy model automatically with view */
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_NONE);

	g_object_set(view, "rules-hint", TRUE, "headers-visible", (tree->hdr != NULL), NULL);
	g_signal_connect(G_OBJECT(view), "cursor-changed", G_CALLBACK(ghid_tree_table_cursor), attr);
	g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(ghid_tree_table_button_press_cb), attr);
	g_signal_connect(G_OBJECT(view), "button-release-event", G_CALLBACK(ghid_tree_table_button_release_cb), attr);
	g_signal_connect(G_OBJECT(view), "key-press-event", G_CALLBACK(ghid_tree_table_key_press_cb), attr);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	gtk_widget_set_tooltip_text(view, attr->help_text);
	bparent = frame_scroll(parent, attr->pcb_hatt_flags, &ctx->wltop[j]);
	gtk_box_pack_start(GTK_BOX(bparent), view, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(view), PCB_OBJ_PROP, ctx);
	return view;
}

static void ghid_tree_pre_free(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, int j)
{
	GtkWidget *tt = ctx->wl[j];
	/* make sure the model filter functions are not called back in an async call
	   when we already free'd attr */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tt), NULL);
}

