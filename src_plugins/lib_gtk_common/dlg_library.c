/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Alain Vigne
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

#include "config.h"
#include <ctype.h>
#include <assert.h>
#include "dlg_library.h"
#include "conf_core.h"

#include "buffer.h"
#include "data.h"
#include "const.h"
#include "plug_footprint.h"
#include "compat_nls.h"

#include <gdk/gdkkeysyms.h>

#include "bu_box.h"
#include "compat.h"
#include "wt_preview.h"
#include "win_place.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "dlg_library_param.h"

/** how often should the parametric footprint redrawn while the parameters are
    changing - in milisec */
#define PARAM_REFRESH_RATE_MS 100

/** \todo Open an empty file. Launch the Netlist window ... then : \n <tt>
(pcb-rnd:14394): Gtk-CRITICAL **: IA__gtk_widget_show_all: assertion 'GTK_IS_WIDGET (widget)' failed \n
(pcb-rnd:14394): Gtk-CRITICAL **: IA__gtk_tree_view_set_model: assertion 'GTK_IS_TREE_VIEW (tree_view)' failed \n
Error: can't update netlist window: there is no netlist loaded. \n
(pcb-rnd:14394): Gtk-CRITICAL **: IA__gtk_window_present_with_time: assertion 'GTK_IS_WINDOW (window)' failed </tt>
*/

/** \file   wt_library.c
    \brief  Implementation of \ref pcb_gtk_library_t widget.
 */

static GtkWidget *library_window;

/** The time interval between request and actual filtering.

    This constant is the time-lag between user modifications in the
    filter entry and the actual evaluation of the filter which
    ultimately update the display. It helps reduce the frequency of
    evaluation of the filter as user types.

    Unit is milliseconds.
 */
#define LIBRARY_FILTER_INTERVAL 200

/** */
static gint library_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	wplc_config_event(widget, &hid_gtk_wgeo.library_x, &hid_gtk_wgeo.library_y, &hid_gtk_wgeo.library_width,
										&hid_gtk_wgeo.library_height);
	return FALSE;
}

/** */
enum {
	MENU_NAME_COLUMN,							/* Text to display in the tree     */
	MENU_ENTRY_COLUMN,						/* Pointer to the library_t */
	N_MENU_COLUMNS
};


/** Processes the response returned by the library dialog.

    This function handles the response \p arg1 of the library \p dialog .

    Parameter \p user_data is a pointer on the relevant toplevel structure.

    \param dialog        The library dialog.
    \param arg1          The response ID.
    \param user_data     pointer to ??FIXME??
 */
static void library_window_callback_response(GtkDialog * dialog, gint arg1, gpointer user_data)
{
	switch (arg1) {
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

/** Determines visibility of items of the library treeview.
    This is the function used to filter entries of the footprint
    selection tree.

    \param model    The current selection in the treeview.
    \param iter     An iterator on a footprint or folder in the tree.
    \param data     The library dialog.
    \returns        `TRUE` if item should be visible, `FALSE` otherwise.
 */
static gboolean lib_model_filter_visible_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	pcb_gtk_library_t *library_window = (pcb_gtk_library_t *) data;
	const gchar *compname;
	gchar *compname_upper, *text_upper, *pattern;
	const gchar *text_;
	char text[1024];
	gboolean ret;
	char *p, *tags;
	int len, is_para = 0;

	g_assert(GHID_IS_LIBRARY_WINDOW(data));

	text_ = gtk_entry_get_text(library_window->entry_filter);
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

/** Handles activation (e.g. double-clicking) of a component row.
    As a convenience to the user `,` expand `/` contract any node with children.

    \param tree_view     The component treeview.
    \param path          The GtkTreePath to the activated row.
    \param column        The GtkTreeViewColumn in which the activation occurre
    \param user_data     The component selection dialog.
 */
static void tree_row_activated(GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	if (!gtk_tree_model_iter_has_child(model, &iter))
		return;

	if (gtk_tree_view_row_expanded(tree_view, path))
		gtk_tree_view_collapse_row(tree_view, path);
	else
		gtk_tree_view_expand_row(tree_view, path, FALSE);
}

/** Handles CTRL-C keypress in the TreeView.

    Keypress activation handler:
    If CTRL-C is pressed, copy footprint name into the clipboard.

    \param tree_view     The component treeview.
    \param event         The GdkEventKey with keypress info.
    \param user_data     Not used.
    \return              `TRUE` if CTRL-C event was handled, `FALSE` otherwise.
 */
static gboolean tree_row_key_pressed(GtkTreeView * tree_view, GdkEventKey * event, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkClipboard *clipboard;
	const gchar *compname;
	guint default_mod_mask = gtk_accelerator_get_default_mod_mask();

	/* Handle both lower- and uppercase `c' */
	if (((event->state & default_mod_mask) != GDK_CONTROL_MASK)
			|| ((event->keyval != GDK_c) && (event->keyval != GDK_C)))
		return FALSE;

	selection = gtk_tree_view_get_selection(tree_view);
	g_return_val_if_fail(selection != NULL, TRUE);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return TRUE;

	gtk_tree_model_get(model, &iter, MENU_NAME_COLUMN, &compname, -1);

	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	g_return_val_if_fail(clipboard != NULL, TRUE);

	gtk_clipboard_set_text(clipboard, compname, -1);

	return TRUE;
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
	pcb_crosshair_set_mode(PCB_MODE_ARROW);
	if (pcb_element_load_to_buffer(PCB_PASTEBUFFER, name == NULL ? fullp : name))
		pcb_crosshair_set_mode(PCB_MODE_PASTE_BUFFER);

	/* update the preview with new symbol data */
	if ((PCB_PASTEBUFFER->Data != NULL) && (elementlist_length(&PCB_PASTEBUFFER->Data->Element) != 0))
		g_object_set(library_window->preview, "element-data", elementlist_first(&PCB_PASTEBUFFER->Data->Element), NULL);
	else {
		g_object_set(library_window->preview, "element-data", NULL, NULL);
	}

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

/** Update the preview and filter text from the parametric attribute dialog */
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

/** Handles changes in the treeview selection.

    This is the callback function that is called every time the user
    select a row the library treeview of the dialog.

    If the selection is not a selection of a footprint, it does
    nothing. Otherwise it updates the preview and Element data.

    \param selection     The current selection in the treeview.
    \param user_data     The library dialog.
 */
static void library_window_callback_tree_selection_changed(GtkTreeSelection * selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	pcb_gtk_library_t *library_window = (pcb_gtk_library_t *) user_data;
	char *name = NULL;
	pcb_fplibrary_t *entry = NULL;

	lib_param_del_timer(library_window);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, MENU_ENTRY_COLUMN, &entry, -1);

	if (entry == NULL)
		return;

	if ((entry->type == LIB_FOOTPRINT) && (entry->data.fp.type == PCB_FP_PARAMETRIC)) {
		const char *in_para = gtk_entry_get_text(library_window->entry_filter);
		name = pcb_gtk_library_param_ui(library_window, entry, in_para, lib_param_chg);
		lib_param_del_timer(library_window);
		if (name == NULL) {
#warning TODO: refresh the display with empty - also for the above returns!
			return;
		}
		gtk_entry_set_text(library_window->entry_filter, name);
	}
	library_window_preview_refresh(library_window, name, entry);
	free(name);
}

static void library_window_callback_edit_button_clicked(GtkButton *button, void *library_window)
{
	pcb_gtk_library_t *lw = library_window;
	library_window_callback_tree_selection_changed(lw->selection, lw);
}


/** Requests re-evaluation of the filter.

    This is the timeout function for the filtering of footprint
    in the tree of the dialog.

    The timeout this callback is attached to is removed after the
    function.

    \param data     The library dialog.
    \returns        `FALSE` to remove the timeout.
 */
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

/** Callback function for the changed signal of the filter entry.

    This function monitors changes in the entry filter of the dialog.

    It specifically manages the sensitivity of the clear button of the
    entry depending on its contents. It also requests an update of the
    footprint list by re-evaluating filter at every changes.

    \param editable      The filter text entry.
    \param user_data     The library dialog.
 */
static void library_window_callback_filter_entry_changed(GtkEditable * editable, gpointer user_data)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(user_data);
	GtkWidget *button;
	gboolean sensitive;

	/* turns button off if filter entry is empty */
	/* turns it on otherwise */
	button = GTK_WIDGET(library_window->button_clear);
	sensitive = (strcmp(gtk_entry_get_text(library_window->entry_filter), "") != 0);
	gtk_widget_set_sensitive(button, sensitive);

	/* Cancel any pending update of the footprint list filter */
	if (library_window->filter_timeout != 0)
		g_source_remove(library_window->filter_timeout);

	/* Schedule an update of the footprint list filter in
	 * LIBRARY_FILTER_INTERVAL milliseconds */
	library_window->filter_timeout = g_timeout_add(LIBRARY_FILTER_INTERVAL, library_window_filter_timeout, library_window);

}

/** Handles a click on the clear button.

    This is the callback function called every time the user press the
    clear button associated with the filter.

    It resets the filter entry, indirectly causing re-evaluation
    of the filter on the list of symbols to update the display.

    \param editable      The filter text entry.
    \param user_data     The library dialog.
 */
static void library_window_callback_filter_button_clicked(GtkButton * button, gpointer user_data)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(user_data);

	/* clears text in text entry for filter */
	gtk_entry_set_text(library_window->entry_filter, "");

}

/** Creates the tree model for the "Library" view.

    Creates a tree where the branches are the available library
    sources and the leaves are the footprints.
 */
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

/* \brief On-demand refresh of the footprint library.
 * \par Function Description
 * Requests a rescan of the footprint library in order to pick up any
 * new signals, and then updates the library window.
 */
static void library_window_callback_refresh_library(GtkButton * button, gpointer user_data)
{
	pcb_gtk_library_t *library_window = GHID_LIBRARY_WINDOW(user_data);
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
			pcb_message(PCB_MSG_ERROR, "Library path is not a directory\n");
			return;
		}

		if (entry->data.dir.backend == NULL) {
			pcb_message(PCB_MSG_ERROR, "Library path is not a top level directory of a fp_ plugin\n");
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

/** Creates the treeview for the "Library" view */
static GtkWidget *create_lib_treeview(pcb_gtk_library_t * library_window)
{
	GtkWidget *libtreeview, *vbox, *scrolled_win, *label, *hbox, *entry, *button;
	GtkTreeModel *child_model, *model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* -- library selection view -- */

	/* vertical box for footprint selection and search entry */
	/* GTK3 issue there ? */
	vbox = GTK_WIDGET(g_object_new(GTK_TYPE_VBOX,
																 /* GtkContainer */
																 "border-width", 5,
																 /* GtkBox */
																 "homogeneous", FALSE, "spacing", 5, NULL));

	child_model = create_lib_tree_model(library_window);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(child_model), MENU_NAME_COLUMN, GTK_SORT_ASCENDING);
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

	g_signal_connect(libtreeview, "row-activated", G_CALLBACK(tree_row_activated), NULL);

	g_signal_connect(libtreeview, "key-press-event", G_CALLBACK(tree_row_key_pressed), NULL);

	/* connect callback to selection */
	library_window->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(libtreeview));
	gtk_tree_selection_set_mode(library_window->selection, GTK_SELECTION_SINGLE);
	g_signal_connect(library_window->selection, "changed", G_CALLBACK(library_window_callback_tree_selection_changed), library_window);

	/* insert a column to treeview for library/symbol name */
	renderer = GTK_CELL_RENDERER(g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
																						/* GtkCellRendererText */
																						"editable", FALSE, NULL));
	column = GTK_TREE_VIEW_COLUMN(g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
																						 /* GtkTreeViewColumn */
																						 "title", _("Components"), NULL));
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
	hbox = GTK_WIDGET(g_object_new(GTK_TYPE_HBOX,
																 /* GtkBox */
																 "homogeneous", FALSE, "spacing", 3, NULL));

	/* create the entry label */
	label = GTK_WIDGET(g_object_new(GTK_TYPE_LABEL,
																	/* GtkMisc */
																	"xalign", 0.0,
																	/* GtkLabel */
																	"label", _("Filter:"), NULL));
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
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_edit_button_clicked), library_window);
	/* add the clear button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	/* set clear button of library_window */
	library_window->button_edit = GTK_BUTTON(button);

	/* CLEAR */
	button = GTK_WIDGET(g_object_new(GTK_TYPE_BUTTON,
																	 /* GtkWidget */
																	 "sensitive", FALSE,
																	 /* GtkButton */
																	 "relief", GTK_RELIEF_NONE, NULL));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_SMALL_TOOLBAR));
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_filter_button_clicked), library_window);
	/* add the clear button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	/* set clear button of library_window */
	library_window->button_clear = GTK_BUTTON(button);


	/* create the refresh button */
	button = GTK_WIDGET(g_object_new(GTK_TYPE_BUTTON,
																	 /* GtkWidget */
																	 "sensitive", TRUE,
																	 /* GtkButton */
																	 "relief", GTK_RELIEF_NONE, NULL));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR));
	/* add the refresh button to the filter area */
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(library_window_callback_refresh_library), library_window);

	/* add the filter area to the vertical box */
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	library_window->libtreeview = GTK_TREE_VIEW(libtreeview);

	return vbox;
}

#warning TODO: figure how this can be passed to the constructor without a global var
static pcb_gtk_common_t *lwcom;

static GObject *library_window_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_params)
{
	GObject *object;
	pcb_gtk_library_t *library_window;
	GtkWidget *content_area;
	GtkWidget *hpaned, *notebook;
	GtkWidget *libview;
	GtkWidget *preview, *preview_text;
	GtkWidget *alignment, *frame;

	/* chain up to constructor of parent class */
	object = G_OBJECT_CLASS(library_window_parent_class)->constructor(type, n_construct_properties, construct_params);
	library_window = GHID_LIBRARY_WINDOW(object);
	library_window->param_timer = 0;

	/* dialog initialization */
	g_object_set(object,
							 /* GtkWindow */
							 "type", GTK_WINDOW_TOPLEVEL,
							 "title", _("Select Footprint..."),
							 "default-height", 300, "default-width", 400, "modal", FALSE, "window-position", GTK_WIN_POS_NONE,
							 /* GtkDialog */
							 "has-separator", TRUE, NULL);
	g_object_set(gtk_dialog_get_content_area(GTK_DIALOG(library_window)), "homogeneous", FALSE, NULL);

	/* horizontal pane containing selection and preview */
	hpaned = GTK_WIDGET(g_object_new(GTK_TYPE_HPANED,
																	 /* GtkContainer */
																	 "border-width", 5, NULL));
	library_window->hpaned = hpaned;

	/* notebook for library views */
	notebook = GTK_WIDGET(g_object_new(GTK_TYPE_NOTEBOOK, "show-tabs", FALSE, NULL));
	library_window->viewtabs = GTK_NOTEBOOK(notebook);

	libview = create_lib_treeview(library_window);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), libview, gtk_label_new(_("Libraries")));

	/* include the vertical box in horizontal box */
	gtk_paned_pack1(GTK_PANED(hpaned), notebook, TRUE, FALSE);


	/* -- preview area -- */
	frame = GTK_WIDGET(g_object_new(GTK_TYPE_FRAME,
																	/* GtkFrame */
																	"label", _("Preview"), NULL));
	alignment = GTK_WIDGET(g_object_new(GTK_TYPE_ALIGNMENT,
																			/* GtkAlignment */
																			"left-padding", 5,
																			"right-padding", 5,
																			"top-padding", 5,
																			"bottom-padding", 5, "xscale", 1.0, "yscale", 1.0, "xalign", 0.5, "yalign", 0.5, NULL));

	preview = pcb_gtk_preview_new(lwcom, lwcom->init_drawing_widget, lwcom->preview_expose, NULL);
	gtk_widget_set_size_request(preview, 150, 150);

	preview_text = gtk_label_new("");

	{
		GtkWidget *vbox = gtkc_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), preview, TRUE, TRUE, 0);
		gtk_box_pack_end(GTK_BOX(vbox), preview_text, FALSE, FALSE, 0);

		gtk_container_add(GTK_CONTAINER(alignment), vbox);
		gtk_container_add(GTK_CONTAINER(frame), alignment);
	}

	/* set preview of library_window */
	library_window->preview = preview;
	library_window->preview_text = preview_text;

	gtk_paned_pack2(GTK_PANED(hpaned), frame, FALSE, FALSE);

	/* add the hpaned to the dialog content area */
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(library_window));
	gtk_box_pack_start(GTK_BOX(content_area), hpaned, TRUE, TRUE, 0);
	gtk_widget_show_all(hpaned);

	/* now add buttons in the action area */
	gtk_dialog_add_buttons(GTK_DIALOG(library_window),
												 /*  - close button */
												 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);

	return object;
}

/** */
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

/** */
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

	g_signal_connect(library_window, "response", G_CALLBACK(library_window_callback_response), NULL);
	g_signal_connect(G_OBJECT(library_window), "configure_event", G_CALLBACK(library_window_configure_event_cb), NULL);
	gtk_window_set_default_size(GTK_WINDOW(library_window), hid_gtk_wgeo.library_width, hid_gtk_wgeo.library_height);

	gtk_window_set_title(GTK_WINDOW(library_window), _("pcb-rnd Library"));
	gtk_window_set_role(GTK_WINDOW(library_window), "PCB_Library");

	wplc_place(WPLC_LIBRARY, library_window);

	gtk_widget_realize(library_window);

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
