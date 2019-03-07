/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Alain Vigne
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
 *
 */

/* This file originally from the PCB Gtk port by Bill Wilson. It has
 * since been combined with modified code from the gEDA project:
 *
 * gschem/src/ghid_library_window.c, checked out by Peter Clifton
 * from gEDA/gaf commit 72581a91da08c9d69593c24756144fc18940992e
 * on 3rd Jan, 2008.
 *
 * gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2007 Ales Hvezda
 * Copyright (C) 1998-2007 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include <ctype.h>
#include <assert.h>
#include "dlg_library.h"
#include "conf_core.h"

#include "buffer.h"
#include "data.h"
#include "plug_footprint.h"
#include "compat_misc.h"
#include "actions.h"
#include "draw.h"

#include <gdk/gdkkeysyms.h>

#include "bu_box.h"
#include "compat.h"
#include "wt_preview.h"
#include "win_place.h"
#include "tool.h"
#include "event.h"

#include "hid_gtk_conf.h"
#include "dlg_library_param.h"

/* how often should the parametric footprint redrawn while the parameters are
   changing - in millisec */
#define PARAM_REFRESH_RATE_MS 500

static GtkWidget *library_window;

/* Lag between user modifications in the filter entry and the
   actual evaluation/GUI update in ms. Helps reducing the frequency of
   updates while the user is typing. */
#define LIBRARY_FILTER_INTERVAL 200

/* GtkWidget "configure_event" signal emitted when the size, position or stacking of the widget's window has changed. */
static gint library_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	pcb_event(PCB_EVENT_DAD_NEW_GEO, "psiiii", NULL, "library",
		(int)ev->x, (int)ev->y, (int)ev->width, (int)ev->height);

	return FALSE;
}

enum {
	MENU_NAME_COLUMN,  /* Text to display in the tree */
	MENU_ENTRY_COLUMN, /* Pointer to the library_t */
	N_MENU_COLUMNS
};

static void library_window_callback_response(GtkDialog * dialog, gint response_id, gpointer user_data)
{
	switch (response_id) {
	case GTK_RESPONSE_CLOSE:
	case GTK_RESPONSE_DELETE_EVENT:
		gtk_widget_destroy(GTK_WIDGET(library_window));
		library_window = NULL;
		break;

	default:
		/* Do nothing, in case there's another handler function which
		   can handle the response ID received. */
		break;
	}
}

static GObjectClass *library_window_parent_class = NULL;

/* Return whether a treeview item should be visible.
   This is the function applies the filter on footprint names. */
static gboolean lib_model_filter_visible_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	pcb_gtk_library_t *lib_window = (pcb_gtk_library_t *) data;
	const gchar *compname;
	gchar *compname_upper, *text_upper, *pattern;
	const gchar *text_;
	char text[1024];
	gboolean ret;
	char *p, *tags;
	int len, is_para = 0;

	g_assert(GHID_IS_LIBRARY_WINDOW(data));

	text_ = gtk_entry_get_text(lib_window->entry_filter);
	if (strcmp(text_, "") == 0) {
		return TRUE;
	}

/* TODO: do these only once.... */
	p = strchr(text_, '(');
	if (p != NULL) {
		len = p - text_;
		is_para = 1;
	}
	else
		len = sizeof(text) - 1;

	strncpy(text, text_, len);
	text[len] = '\0';

	/* apply tags on non-parametrics */
	if (!is_para) {
		tags = strchr(text, ' ');
		if (tags != NULL) {
			*tags = '\0';
			tags++;
			while (isspace(*tags))
				tags++;
			if (*tags == '\0')
				tags = NULL;
		}
	}
	else
		tags = NULL;

	/* If this is a source, only display it if it has children that
	 * match */
	if (gtk_tree_model_iter_has_child(model, iter)) {
		GtkTreeIter iter2;

		gtk_tree_model_iter_children(model, &iter2, iter);
		ret = FALSE;
		do {
			if (lib_model_filter_visible_func(model, &iter2, data)) {
				ret = TRUE;
				break;
			}
		}
		while (gtk_tree_model_iter_next(model, &iter2));
	}
	else {
		gtk_tree_model_get(model, iter, MENU_NAME_COLUMN, &compname, -1);
		/* Do a case insensitive comparison, converting the strings
		   to uppercase */
		compname_upper = g_ascii_strup(compname, -1);
		text_upper = g_ascii_strup(text, -1);
		if (is_para)
			pattern = g_strconcat("*", text_upper, "(*", NULL);
		else
			pattern = g_strconcat("*", text_upper, "*", NULL);

		ret = g_pattern_match_simple(pattern, compname_upper);

		if ((tags != NULL) && ret) {
			pcb_fplibrary_t *entry = NULL;
			gtk_tree_model_get(model, iter, MENU_ENTRY_COLUMN, &entry, -1);
			if ((entry != NULL) && (entry->type == LIB_FOOTPRINT) && (entry->data.fp.tags != NULL)) {
				char *next, *tag;
				int found;
				void **t;
				const void *need;

				for (tag = tags; tag != NULL; tag = next) {
					next = strpbrk(tag, " \t\r\n");
					if (next != NULL) {
						*next = '\0';
						next++;
						while (isspace(*next))
							next++;
					}
					need = pcb_fp_tag(tag, 0);
/*					pcb_trace("TAG: '%s' %p\n", tag, (void *) need);*/
					if (need == NULL) {
						ret = FALSE;
						break;
					}
					found = 0;
					for (t = entry->data.fp.tags; *t != NULL; t++) {
						if (*t == need) {
							found = 1;
							break;
						}
					}
					if (!found) {
						ret = FALSE;
						break;
					}
				}
			}
			else
				ret = FALSE;
		}

		g_free(compname_upper);
		g_free(text_upper);
		g_free(pattern);
	}

	return ret;
}

static void library_window_preview_refresh(pcb_gtk_library_t * library_window, const char *name, pcb_fplibrary_t * entry)
{
	GString *pt;
	char *fullp;

	/*  -1 flags this is an element file part and the file path is in
	   entry->AllocateMemory.
	 */
	if (name == NULL) {
		if ((entry == NULL) || (entry->type != LIB_FOOTPRINT))
			return;
		fullp = entry->data.fp.loc_info;
	}
	pcb_tool_select_by_id(PCB_MODE_ARROW);
	if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, name == NULL ? fullp : name, NULL))
		pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);

	/* update the preview with new symbol data */
	if (PCB_PASTEBUFFER->Data != NULL) {
		if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0) {
			pcb_subc_t *sc = pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc);
			g_object_set(library_window->preview, "draw_data", sc, NULL);
			if (sc != NULL) {
				pcb_box_t bbox;
				pcb_data_bbox(&bbox, sc->data, 0);
				pcb_gtk_preview_zoomto(PCB_GTK_PREVIEW(library_window->preview), &bbox);
			}
		}
		else
			g_object_set(library_window->preview, "draw_data", NULL, NULL);
	}
	else
		g_object_set(library_window->preview, "draw_data", NULL, NULL);

	/* update the text */
	pt = g_string_new("Tags:");
	if ((entry != NULL) && (entry->type == LIB_FOOTPRINT) && (entry->data.fp.tags != NULL)) {
		void **t;

		for (t = entry->data.fp.tags; *t != NULL; t++) {
			const char *name = pcb_fp_tagname(*t);
			if (name != NULL) {
				g_string_append(pt, "\n  ");
				g_string_append(pt, name);
			}
		}
		g_string_append(pt, "\nLocation:\n ");
		g_string_append(pt, entry->data.fp.loc_info);
		g_string_append(pt, "\n");
	}
	gtk_label_set_text(GTK_LABEL(library_window->preview_text), g_string_free(pt, FALSE));
}

/* Update the preview and filter text from the parametric attribute dialog */
static gboolean lib_param_chg_delayed(pcb_gtk_library_param_cb_ctx_t *ctx)
{
	char *cmd = pcb_gtk_library_param_snapshot(ctx);

	gtk_entry_set_text(ctx->library_window->entry_filter, cmd);
	library_window_preview_refresh(ctx->library_window, cmd, ctx->entry);
	free(cmd);
	ctx->library_window->param_timer = 0;
	return FALSE; /* Turns timer off */
}

static void lib_param_del_timer(pcb_gtk_library_t *library_window)
{
	if (library_window->param_timer != 0)
		g_source_remove(library_window->param_timer);
	library_window->param_timer = 0;
}

static void lib_param_chg(pcb_gtk_library_param_cb_ctx_t *ctx)
{
	if (ctx->library_window->param_timer == 0)
		ctx->library_window->param_timer = g_timeout_add(PARAM_REFRESH_RATE_MS, (GSourceFunc)lib_param_chg_delayed, ctx);
}

/* Called when a tree view row is selected. If the selection is a footprint,
   update the preview. */
static void library_window_callback_tree_selection_changed(GtkTreeSelection * selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	pcb_gtk_library_t *library_window = (pcb_gtk_library_t *) user_data;
	char *name = NULL;
	pcb_fplibrary_t *entry = NULL;
	int norefresh = 0;

	lib_param_del_timer(library_window);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, MENU_ENTRY_COLUMN, &entry, -1);

	if (entry == NULL)
		return;

	if ((entry->type == LIB_FOOTPRINT) && (entry->data.fp.type == PCB_FP_PARAMETRIC)) {
		const char *in_para = gtk_entry_get_text(library_window->entry_filter);
		char *orig = pcb_strdup(in_para);
		name = pcb_gtk_library_param_ui(library_window->com, library_window, entry, in_para, lib_param_chg);
		lib_param_del_timer(library_window);
		if (name == NULL) {
			gtk_entry_set_text(library_window->entry_filter, orig);
			if (library_window->filter_timeout != 0)
				g_source_remove(library_window->filter_timeout); /* block the above edit from collapsing the tree */
			library_window->filter_timeout = 0;
			gtk_tree_model_filter_refilter((GtkTreeModelFilter *) model);

			/* cancel means we should skip refreshing the preview with the invalid
			   text we may have in the filter; also dismiss the preview and clear
			   the buffer: we may have a valid parametric footprint output in there
			   that happened before the cancel */
			norefresh = 1;
			g_object_set(library_window->preview, "draw_data", NULL, NULL);
			pcb_actionl("PasteBuffer", "clear", NULL);
		}
		else
			gtk_entry_set_text(library_window->entry_filter, name);
		free(orig);
	}
	if (!norefresh)
		library_window_preview_refresh(library_window, name, entry);
	free(name);
}

static void library_window_callback_edit_button_clicked(GtkButton *button, void *library_window)
{
	pcb_gtk_library_t *lw = library_window;
	library_window_callback_tree_selection_changed(lw->selection, lw);
}


/* Requests re-evaluation of the filter. The timeout this callback is
   attached to is removed after the function. */
static gboolean library_window_filter_timeout(gpointer data)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(data);
	GtkTreeModel *model;

	/* resets the source id in library_window */
	library_window->filter_timeout = 0;

	model = gtk_tree_view_get_model(library_window->libtreeview);

	if (model != NULL) {
		const gchar *text = gtk_entry_get_text(library_window->entry_filter);
		gtk_tree_model_filter_refilter((GtkTreeModelFilter *) model);
		if (strcmp(text, "") != 0) {
			/* filter text not-empty */
			gtk_tree_view_expand_all(library_window->libtreeview);

			/* parametric footprints need to be refreshed on edit */
			if (strchr(text, ')') != NULL)
				library_window_preview_refresh(library_window, text, NULL);

		}
		else {
			/* filter text is empty, collapse expanded tree */
			gtk_tree_view_collapse_all(library_window->libtreeview);
		}
	}

	/* return FALSE to remove the source */
	return FALSE;
}

/* Manage the sensitivity of the clear button, request an update of the
   footprint list by re-evaluating filter at every changes. */
static void library_window_callback_filter_entry_changed(GtkEditable * editable, gpointer lib_win)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(lib_win);
	GtkWidget *button;
	gboolean sensitive;

	/* turns button off if filter entry is empty; turns it on otherwise */
	button = GTK_WIDGET(library_window->button_clear);
	sensitive = (strcmp(gtk_entry_get_text(library_window->entry_filter), "") != 0);
	gtk_widget_set_sensitive(button, sensitive);

	/* Cancel any pending update of the footprint list filter */
	if (library_window->filter_timeout != 0)
		g_source_remove(library_window->filter_timeout);

	/* Schedule an update of the footprint list filter in LIBRARY_FILTER_INTERVAL milliseconds */
	library_window->filter_timeout = g_timeout_add(LIBRARY_FILTER_INTERVAL, library_window_filter_timeout, library_window);

}

/* Click on the [filter] clear button. Resets the filter entry,
   indirectly causing re-evaluation/GUI update. */
static void library_window_callback_filter_button_clicked(GtkButton * button, gpointer lib_win)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(lib_win);

	/* clears text in text entry for filter */
	gtk_entry_set_text(library_window->entry_filter, "");

}

/* Creates a tree model from the library tree starting at parent. */
static GtkTreeModel *create_lib_tree_model_recurse(GtkTreeStore * tree, pcb_gtk_library_t * library_window,
																									 pcb_fplibrary_t * parent, GtkTreeIter * iter_parent)
{
	GtkTreeIter p_iter;
	pcb_fplibrary_t *menu;
	int n;

	for (menu = parent->data.dir.children.array, n = 0; n < parent->data.dir.children.used; n++, menu++) {
		gtk_tree_store_append(tree, &p_iter, iter_parent);
		gtk_tree_store_set(tree, &p_iter, MENU_NAME_COLUMN, menu->name, MENU_ENTRY_COLUMN, menu, -1);
		if (menu->type == LIB_DIR)
			create_lib_tree_model_recurse(tree, library_window, menu, &p_iter);
	}

	return (GtkTreeModel *) tree;
}

static GtkTreeModel *create_lib_tree_model(pcb_gtk_library_t * library_window)
{
	GtkTreeStore *tree = gtk_tree_store_new(N_MENU_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
	return create_lib_tree_model_recurse(tree, library_window, &pcb_library, NULL);
}

/* User requested refresh of the tree content */
static void library_window_callback_refresh_library(GtkButton * button, gpointer lib_win)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(lib_win);
	GtkTreeModel *model;
	pcb_fplibrary_t *entry = NULL;
	GtkTreeIter iter;

	lib_param_del_timer(library_window);

	if (gtk_tree_selection_get_selected(library_window->selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, MENU_ENTRY_COLUMN, &entry, -1);

		if (entry == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid selection\n");
			return;
		}

		if (entry->type != LIB_DIR) {
			pcb_message(PCB_MSG_ERROR, "Library path is not a top level directory - please select a top level dir first\n");
			return;
		}

		if (entry->data.dir.backend == NULL) {
			pcb_message(PCB_MSG_ERROR, "Library path is not a top level directory of a fp_ plugin - please select a top level dir first\n");
			return;
		}
	}

	if (pcb_fp_rehash(entry) != 0) {
		pcb_message(PCB_MSG_ERROR, "Failed to rehash library\n");
		return;
	}

	/* Refresh the "Library" view */
	model = (GtkTreeModel *)
		g_object_new(GTK_TYPE_TREE_MODEL_FILTER, "child-model", create_lib_tree_model(library_window), "virtual-root", NULL, NULL);

	gtk_tree_model_filter_set_visible_func((GtkTreeModelFilter *) model, lib_model_filter_visible_func, library_window, NULL);

	gtk_tree_view_set_model(library_window->libtreeview, model);
}

/* Activation (e.g. double-clicking) of a footprint row. As a convenience
to the user, GTK provides Shift-Arrow Left, Right to expand or
contract any node with children. */
static void tree_row_activated(GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer lib_win)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	if (!gtk_tree_model_iter_has_child(model, &iter)) {
		library_window_callback_tree_selection_changed(gtk_tree_view_get_selection(tree_view), lib_win);
		return;
	}

	if (gtk_tree_view_row_expanded(tree_view, path))
		gtk_tree_view_collapse_row(tree_view, path);
	else
		gtk_tree_view_expand_row(tree_view, path, FALSE);
}

/* Key pressed activation handler: CTRL-C -> copy footprint name to clipboard;
   Enter -> row-activate. */
static gboolean treeview_key_press_cb(GtkTreeView * tree_view, GdkEventKey * event, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	pcb_bool key_handled, arrow_key, enter_key, force_activate = pcb_false;
	GtkClipboard *clipboard;
	const gchar *compname;
	guint default_mod_mask = gtk_accelerator_get_default_mod_mask();

	arrow_key = ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_KP_Up)
		|| (event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_KP_Down)
		|| (event->keyval == GDK_KEY_KP_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Up)
		|| (event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_Page_Up)
		|| (event->keyval == GDK_KEY_KP_Home) || (event->keyval == GDK_KEY_KP_End)
		|| (event->keyval == GDK_KEY_Home) || (event->keyval == GDK_KEY_End));
	enter_key = (event->keyval == GDK_KEY_Return) || (event->keyval == GDK_KEY_KP_Enter);
	key_handled = (enter_key || arrow_key);

	/* Handle ctrl+c and ctrl+C: copy current name to clipboard */
	if (((event->state & default_mod_mask) == GDK_CONTROL_MASK) && ((event->keyval == GDK_KEY_c) || (event->keyval == GDK_KEY_C))) {
		selection = gtk_tree_view_get_selection(tree_view);
		g_return_val_if_fail(selection != NULL, TRUE);

		if (!gtk_tree_selection_get_selected(selection, &model, &iter))
			return TRUE;

		gtk_tree_model_get(model, &iter, MENU_NAME_COLUMN, &compname, -1);

		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		g_return_val_if_fail(clipboard != NULL, TRUE);

		gtk_clipboard_set_text(clipboard, compname, -1);

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
		pcb_fplibrary_t *entry = NULL;

		gtk_tree_model_get(model, &iter, MENU_ENTRY_COLUMN, &entry, -1);

		if ((entry != NULL) && (entry->type == LIB_FOOTPRINT) && (entry->data.fp.type != PCB_FP_PARAMETRIC))
			force_activate = pcb_true;
	}

	/* Handle 'Enter' key and arrow keys as "activate" on plain footprints */
	if (enter_key || force_activate) {
		path = gtk_tree_model_get_path(model, &iter);
		if (path != NULL) {
			tree_row_activated(tree_view, path, NULL, user_data);
		}
		gtk_tree_path_free(path);
	}

	return TRUE;
}

/* Just handle the double-click to be equivalent to "row-activated" signal */
static gboolean treeview_button_press_cb(GtkWidget  * widget, GdkEvent * ev, gpointer user_data)
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
			tree_row_activated(tv, path, NULL, user_data);
		}
	}

	return FALSE;
}

static gboolean treeview_button_release_cb(GtkWidget  * widget, GdkEvent * ev, gpointer user_data)
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
			tree_row_activated(tv, path, NULL, user_data);
	}

	return FALSE;
}


static GtkWidget *create_lib_treeview(pcb_gtk_library_t * library_window)
{
	GtkWidget *libtreeview, *vbox, *scrolled_win, *label, *hbox, *entry, *button;
	GtkTreeModel *child_model, *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* -- library selection view -- */

	/* vertical box for footprint selection and search entry */
	vbox = gtkc_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	child_model = create_lib_tree_model(library_window);
	model = (GtkTreeModel *) g_object_new(GTK_TYPE_TREE_MODEL_FILTER, "child-model", child_model, "virtual-root", NULL, NULL);

	scrolled_win = GTK_WIDGET(g_object_new(GTK_TYPE_SCROLLED_WINDOW,
																				 /* GtkScrolledWindow */
																				 "hscrollbar-policy",
																				 GTK_POLICY_AUTOMATIC,
																				 "vscrollbar-policy", GTK_POLICY_ALWAYS, "shadow-type", GTK_SHADOW_ETCHED_IN, NULL));
	/* create the treeview */
	libtreeview = GTK_WIDGET(g_object_new(GTK_TYPE_TREE_VIEW,
																				/* GtkTreeView */
																				"model", model, "rules-hint", TRUE, "headers-visible", FALSE, NULL));

	g_signal_connect(libtreeview, "button-press-event", G_CALLBACK(treeview_button_press_cb), library_window);
	g_signal_connect(libtreeview, "button-release-event", G_CALLBACK(treeview_button_release_cb), library_window);

	g_signal_connect(libtreeview, "key-press-event", G_CALLBACK(treeview_key_press_cb), library_window);

	library_window->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(libtreeview));
	gtk_tree_selection_set_mode(library_window->selection, GTK_SELECTION_SINGLE);

	/* insert a column to treeview for library/symbol name */
	renderer = GTK_CELL_RENDERER(g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
																						/* GtkCellRendererText */
																						"editable", FALSE, NULL));
	column = GTK_TREE_VIEW_COLUMN(g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
																						 /* GtkTreeViewColumn */
																						 "title", "Components", NULL));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", MENU_NAME_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(libtreeview), column);

	/* add the treeview to the scrolled window */
	gtk_container_add(GTK_CONTAINER(scrolled_win), libtreeview);
	/* set directory/footprint treeview of library_window */
	library_window->libtreeview = GTK_TREE_VIEW(libtreeview);

	/* add the scrolled window for directories to the vertical box */
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);


	/* -- filter area -- */
	hbox = gtkc_hbox_new(FALSE, 3);

	/* create the entry label */
	label = GTK_WIDGET(g_object_new(GTK_TYPE_LABEL,
																	/* GtkMisc */
																	"xalign", 0.0,
																	/* GtkLabel */
																	"label", "Filter:", NULL));
	/* add the search label to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* create the text entry for filter in footprints */
	entry = GTK_WIDGET(g_object_new(GTK_TYPE_ENTRY,
																	/* GtkEntry */
																	"text", "", NULL));
	g_signal_connect(entry, "changed", G_CALLBACK(library_window_callback_filter_entry_changed), library_window);

	/* now that that we have an entry, set the filter func of model */
	gtk_tree_model_filter_set_visible_func((GtkTreeModelFilter *) model, lib_model_filter_visible_func, library_window, NULL);

	/* add the filter entry to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	/* set filter entry of library_window */
	library_window->entry_filter = GTK_ENTRY(entry);
	/* and init the event source for footprint filter */
	library_window->filter_timeout = 0;

	/* EDIT */
	button = GTK_WIDGET(g_object_new(GTK_TYPE_BUTTON,
																	 /* GtkWidget */
																	 "sensitive", TRUE,
																	 /* GtkButton */
																	 "relief", GTK_RELIEF_NONE, NULL));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name(GTK_STOCK_EDIT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_edit_button_clicked), library_window);
	/* add the clear button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	/* set clear button of library_window */
	library_window->button_edit = GTK_BUTTON(button);
	gtk_widget_set_tooltip_text(button, "Edit parameters of a\nparametric footprint");

	/* CLEAR */
	button = GTK_WIDGET(g_object_new(GTK_TYPE_BUTTON,
																	 /* GtkWidget */
																	 "sensitive", FALSE,
																	 /* GtkButton */
																	 "relief", GTK_RELIEF_NONE, NULL));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("edit-clear", GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_filter_button_clicked), library_window);
	/* add the clear button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	/* set clear button of library_window */
	library_window->button_clear = GTK_BUTTON(button);
	gtk_widget_set_tooltip_text(button, "Clear the filter entry");

	/* create the refresh button */
	button = GTK_WIDGET(g_object_new(GTK_TYPE_BUTTON,
																	 /* GtkWidget */
																	 "sensitive", TRUE,
																	 /* GtkButton */
																	 "relief", GTK_RELIEF_NONE, NULL));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_SMALL_TOOLBAR));
	/* add the refresh button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_refresh_library), library_window);
	gtk_widget_set_tooltip_text(button, "Refresh the library window tree\n(GUI-only refresh)");

	/* add the filter area to the vertical box */
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	library_window->libtreeview = GTK_TREE_VIEW(libtreeview);

	return vbox;
}

TODO(": figure how this can be passed to the constructor without a global var")
static pcb_gtk_common_t *lwcom;

static void pinout_expose(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_subc_t *sc = e->draw_data;

	if (sc != NULL) {
		int orig_po = pcb_draw_force_termlab;
		pcb_draw_force_termlab = pcb_true;
 		pcb_subc_draw_preview(sc, &e->view);
		pcb_draw_force_termlab = orig_po;
	}
}


static GObject *library_window_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_params)
{
	GObject *object;
	pcb_gtk_library_t *lib_window;
	GtkWidget *content_area;
	GtkWidget *hpaned, *notebook;
	GtkWidget *libview;
	GtkWidget *preview, *preview_text;
	GtkWidget *frame;

	/* chain up to constructor of parent class */
	object = G_OBJECT_CLASS(library_window_parent_class)->constructor(type, n_construct_properties, construct_params);
	lib_window = GHID_LIBRARY_WINDOW(object);
	lib_window->param_timer = 0;
	lib_window->com = lwcom;

	/* dialog initialization */
	g_object_set(object,
							 /* GtkWindow */
							 "type", GTK_WINDOW_TOPLEVEL,
							 "title", "Select Footprint...",
							 "default-height", 300, "default-width", 400, "modal", FALSE, "window-position", GTK_WIN_POS_NONE,
							 /* GtkDialog */
							 NULL);
	g_object_set(gtk_dialog_get_content_area(GTK_DIALOG(lib_window)), "homogeneous", FALSE, NULL);

	/* horizontal pane containing selection and preview */
	hpaned = gtkc_hpaned_new();
	gtk_container_set_border_width(GTK_CONTAINER(hpaned), 5);
	lib_window->hpaned = hpaned;

	/* notebook for library views */
	notebook = GTK_WIDGET(g_object_new(GTK_TYPE_NOTEBOOK, "show-tabs", FALSE, NULL));
	lib_window->viewtabs = GTK_NOTEBOOK(notebook);

	libview = create_lib_treeview(lib_window);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), libview, gtk_label_new("Libraries"));

	/* include the vertical box in horizontal box */
	gtk_paned_pack1(GTK_PANED(hpaned), notebook, TRUE, FALSE);


	/* -- preview area -- */
	frame = GTK_WIDGET(g_object_new(GTK_TYPE_FRAME,
																	/* GtkFrame */
																	"label", "Preview", NULL));

	preview = pcb_gtk_preview_new(lwcom, lwcom->init_drawing_widget, lwcom->preview_expose, pinout_expose, NULL, NULL);
	gtk_widget_set_size_request(preview, 150, 150);

	preview_text = gtk_label_new("");

	{
		GtkWidget *vbox = gtkc_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), preview, TRUE, TRUE, 0);
		gtk_box_pack_end(GTK_BOX(vbox), preview_text, FALSE, FALSE, 0);

		gtk_container_add(GTK_CONTAINER(frame), vbox);
	}

	/* set preview of library window */
	lib_window->preview = preview;
	lib_window->preview_text = preview_text;

	gtk_paned_pack2(GTK_PANED(hpaned), frame, FALSE, FALSE);

	/* add the hpaned to the dialog content area */
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(lib_window));
	gtk_box_pack_start(GTK_BOX(content_area), hpaned, TRUE, TRUE, 0);
	gtk_widget_show_all(hpaned);

	/* now add buttons in the action area */
	gtk_dialog_add_buttons(GTK_DIALOG(lib_window),
												 /*  - close button */
												 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);

	return object;
}

static void library_window_finalize(GObject * object)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(object);

	if (library_window->filter_timeout != 0) {
		g_source_remove(library_window->filter_timeout);
		library_window->filter_timeout = 0;
	}

	lib_param_del_timer(library_window);

	G_OBJECT_CLASS(library_window_parent_class)->finalize(object);
}

static void library_window_class_init(pcb_gtk_library_class_t * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->constructor = library_window_constructor;
	gobject_class->finalize = library_window_finalize;

	library_window_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
}

/* API */

GType pcb_gtk_library_get_type()
{
	static GType library_window_type = 0;

	if (!library_window_type) {
		static const GTypeInfo library_window_info = {
			sizeof(pcb_gtk_library_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) library_window_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_library_t),
			0,												/* n_preallocs */
			NULL											/* instance_init */
		};

		library_window_type = g_type_register_static(GTK_TYPE_DIALOG, "GhidLibraryWindow", &library_window_info, (GTypeFlags) 0);
	}

	return library_window_type;
}

void pcb_gtk_library_create(pcb_gtk_common_t *com)
{
	GtkWidget *current_tab, *entry_filter;
	GtkNotebook *notebook;

	if (library_window)
		return;

	lwcom = com;
	library_window = (GtkWidget *) g_object_new(GHID_TYPE_LIBRARY_WINDOW, NULL);

	pcb_gtk_winplace(library_window, "library");

	g_signal_connect(GTK_DIALOG(library_window), "response", G_CALLBACK(library_window_callback_response), NULL);
	g_signal_connect(library_window, "configure_event", G_CALLBACK(library_window_configure_event_cb), NULL);

	gtk_window_set_title(GTK_WINDOW(library_window), "pcb-rnd Library");
	gtk_window_set_role(GTK_WINDOW(library_window), "PCB_Library");
	gtk_window_set_transient_for(GTK_WINDOW(library_window), GTK_WINDOW(com->top_window));

	gtk_editable_select_region(GTK_EDITABLE(GHID_LIBRARY_WINDOW(library_window)->entry_filter), 0, -1);

	/* Set the focus to the filter entry only if it is in the current
	   displayed tab */
	notebook = GTK_NOTEBOOK(GHID_LIBRARY_WINDOW(library_window)->viewtabs);
	current_tab = gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook));
	entry_filter = GTK_WIDGET(GHID_LIBRARY_WINDOW(library_window)->entry_filter);
	if (gtk_widget_is_ancestor(entry_filter, current_tab)) {
		gtk_widget_grab_focus(entry_filter);
	}
	lwcom = NULL;
}

void pcb_gtk_library_show(pcb_gtk_common_t *com, gboolean raise)
{
	pcb_gtk_library_create(com);
	gtk_widget_show_all(library_window);
	if (raise)
		gtk_window_present(GTK_WINDOW(library_window));
}
