/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998 Thomas Nau
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

/*
 * This file written by Bill Wilson for the PCB Gtk port
 */

#include "config.h"
#include "dlg_netlist.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "event.h"
#include "error.h"
#include "macro.h"
#include "find.h"
#include "rats.h"
#include "remove.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "actions.h"
#include "compat_nls.h"

#include "util_str.h"
#include "win_place.h"
#include "bu_text_view.h"
#include "bu_box.h"
#include "bu_check_button.h"

#include "compat.h"

#include "hid_gtk_conf.h"

#define NET_HIERARCHY_SEPARATOR "/"

static GtkWidget *netlist_window;
static GtkWidget *disable_all_button;

static GtkTreeModel *node_model;
static GtkTreeView *node_treeview;
static GtkTreeSelection *node_selection;

static pcb_bool selection_holdoff;

static pcb_lib_menu_t *selected_net;
static pcb_lib_menu_t *node_selected_net;


/*  The Netlist window displays all the layout nets in a left treeview.
    When one of the nets is selected, all of its nodes (or connections)
    will be displayed in a right treeview.  If a "Select on layout" button
    is pressed, the net that is selected in the left treeview will be
    drawn selected on the layout.

    Gtk separates the data model from the view in its treeview widgets so
    here we maintain two data models.  The net data model has pointers to
    all the nets in the layout and the node data model keeps pointers to all
    the nodes for the currently selected net.  By updating the data models
    the net and node gtk treeviews handle displaying the results.

    The netlist window code has a public interface providing hooks so PCB
    code can control the net and node treeviews:

    ghid_get_net_from_node_name gchar *node_name, pcb_bool enabled_only)
      Given a node name (eg C101-1), walk through the nets in the net
      data model and search each net for the given node_name.  If found
      and enabled_only is true, make the net treeview scroll to and
      highlight (select) the found net.  Return the found net.

    ghid_netlist_highlight_node()
      Given some PCB internal pointers (not really a good gui api here)
      look up a node name determined by the pointers and highlight the node
      in the node treeview.  By using ghid_get_net_from_node_name() to
      look up the node, the net the node belongs to will also be
      highlighted in the net treeview.

    pcb_gtk_dlg_netlist_update(pcb_bool init_nodes)
      PCB calls this to tell the gui netlist code the layout net has
      changed and the gui data structures (net and optionally node data
      models) should be rebuilt.

    See doc/developer/old_netlist.txt for the core data model.

*/

static pcb_lib_entry_t *node_get_node_from_name(pcb_gtk_common_t *com, gchar *node_name, pcb_lib_menu_t **node_net);

enum {
	NODE_NAME_COLUMN,							/* Name to show in the treeview         */
	NODE_LIBRARY_COLUMN,					/* Pointer to this node (pcb_lib_entry_t)      */
	N_NODE_COLUMNS
};

/* Given a net in the netlist (a pcb_lib_menu_t) put all the Entry[]
   names (the nodes) into a newly created node tree model. */
static GtkTreeModel *node_model_create(pcb_lib_menu_t * menu)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new(N_NODE_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);

	if (menu == NULL)
		return GTK_TREE_MODEL(store);

	PCB_ENTRY_LOOP(menu);
	{
		if (!entry->ListEntry)
			continue;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, NODE_NAME_COLUMN, entry->ListEntry, NODE_LIBRARY_COLUMN, entry, -1);
	}
	PCB_END_LOOP;

	return GTK_TREE_MODEL(store);
}

/* When there's a new node to display in the node treeview, call this.
   Create a new model containing the nodes of the given net, insert
   the model into the treeview and unref the old model. */
static void node_model_update(pcb_lib_menu_t * menu)
{
	GtkTreeModel *model;

	if (menu == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error: can't update netlist window: there is no netlist loaded.\n");
		return;
	}

	model = node_model;
	node_model = node_model_create(menu);
	gtk_tree_view_set_model(node_treeview, node_model);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(node_model), NODE_NAME_COLUMN, GTK_SORT_ASCENDING);

	/*  We could be using gtk_list_store_clear() on the same model, but it's
	   just as easy that we've created a new one and here unref the old one.
	 */
	if (model)
		g_object_unref(G_OBJECT(model));
}

static void toggle_pin_selected(pcb_lib_entry_t * entry)
{
	pcb_connection_t conn;

	if (!pcb_rat_seek_pad(entry, &conn, pcb_false))
		return;

	pcb_undo_add_obj_to_flag(conn.obj);
	PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, conn.obj);
	pcb_draw_obj((pcb_any_obj_t *)conn.obj);
}

/* Callback when the user clicks on a PCB node in the right node treeview. */
static void node_selection_changed_cb(GtkTreeSelection * selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	pcb_lib_menu_t *node_net;
	pcb_lib_entry_t *node;
	pcb_connection_t conn;
	pcb_coord_t x = -1, y;
	static gchar *node_name;
	pcb_gtk_common_t *com = data;

	if (selection_holdoff)				/* PCB is highlighting, user is not selecting */
		return;

	/*  Toggle off the previous selection.  Look up node_name to make sure
	   it still exists.  This toggling can get out of sync if a node is
	   toggled selected, then the net that includes the node is selected
	   then unselected.
	 */
	if ((node = node_get_node_from_name(com, node_name, &node_net)) != NULL) {
		/*  If net node belongs to has been highlighted/unhighighed, toggling
		   if off here will get our on/off toggling out of sync.
		 */
		if (node_net == node_selected_net) {
			toggle_pin_selected(node);
			com->cancel_lead_user();
		}
		g_free(node_name);
		node_name = NULL;
	}

	/* Get the selected treeview row.
	 */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		if (node)
			com->invalidate_all();
		return;
	}

	/*  From the treeview row, extract the node pointer stored there and
	   we've got a pointer to the pcb_lib_entry_t (node) the row
	   represents.
	 */
	gtk_tree_model_get(model, &iter, NODE_LIBRARY_COLUMN, &node, -1);

	pcb_gtk_g_strdup(&node_name, node->ListEntry);
	node_selected_net = selected_net;

	/* Now just toggle a select of the node on the layout     */
	toggle_pin_selected(node);
	pcb_undo_inc_serial();

	/* And lead the user to the location */
	if (pcb_rat_seek_pad(node, &conn, pcb_false))
		pcb_obj_center(conn.obj, &x, &y);
	
	if (x >= 0) {
		pcb_gui->set_crosshair(x, y, 0);
		com->lead_user_to_location(x, y);
	}
}

/* -------- The net (pcb_lib_menu_t) data model ----------
 */
/* TODO: the enable and disable all nets.  Can't seem to get how that's
   |  supposed to work, but it'll take updating the NET_ENABLED_COLUMN in
   |  the net_model.  Probably it should be made into a gpointer and make
   |  a text renderer for it and just write a '*' or a ' ' similar to the
   |  the Xt PCB scheme.  Or better, since it's an "all nets" function, just
   |  have a "Disable all nets" toggle button and don't mess with the
   |  model/treeview at all.
*/
enum {
	NET_ENABLED_COLUMN,						/* If enabled will be ' ', if disable '*'       */
	NET_NAME_COLUMN,							/* Name to show in the treeview */
	NET_LIBRARY_COLUMN,						/* Pointer to this net (pcb_lib_menu_t)        */
	N_NET_COLUMNS
};

static GtkTreeModel *net_model = NULL;
static GtkTreeView *net_treeview;

static pcb_bool loading_new_netlist;

static GtkTreeModel *net_model_create(void)
{
	GtkTreeModel *model;
	GtkTreeStore *store;
	GtkTreeIter new_iter;
	GtkTreeIter parent_iter;
	GtkTreeIter *parent_ptr;
	GtkTreePath *path;
	GtkTreeRowReference *row_ref;
	GHashTable *prefix_hash;
	char *display_name;
	char *hash_string;
	char **join_array;
	char **path_segments;
	int path_depth;
	int try_depth;

	store = gtk_tree_store_new(N_NET_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	model = GTK_TREE_MODEL(store);

	/* Hash table stores GtkTreeRowReference for given path prefixes */
	prefix_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)
																			gtk_tree_row_reference_free);

	PCB_MENU_LOOP(&PCB->NetlistLib[PCB_NETLIST_EDITED]);
	{
		if (!menu->Name)
			continue;

		if (loading_new_netlist)
			menu->flag = TRUE;

		parent_ptr = NULL;

		path_segments = g_strsplit(menu->Name, NET_HIERARCHY_SEPARATOR, 0);
		path_depth = g_strv_length(path_segments);

		for (try_depth = path_depth - 1; try_depth > 0; try_depth--) {
			join_array = g_new0(char *, try_depth + 1);
			memcpy(join_array, path_segments, sizeof(char *) * try_depth);

			/* See if this net's parent node is in the hash table */
			hash_string = g_strjoinv(NET_HIERARCHY_SEPARATOR, join_array);
			g_free(join_array);

			row_ref = (GtkTreeRowReference *) g_hash_table_lookup(prefix_hash, hash_string);
			g_free(hash_string);

			/* If we didn't find the path at this level, keep looping */
			if (row_ref == NULL)
				continue;

			path = gtk_tree_row_reference_get_path(row_ref);
			gtk_tree_model_get_iter(model, &parent_iter, path);
			parent_ptr = &parent_iter;
			break;
		}

		/* NB: parent_ptr may still be NULL if we reached the toplevel */

		/* Now walk up the desired path, adding the nodes */

		for (; try_depth < path_depth - 1; try_depth++) {
			display_name = g_strconcat(path_segments[try_depth], NET_HIERARCHY_SEPARATOR, NULL);
			gtk_tree_store_append(store, &new_iter, parent_ptr);
			gtk_tree_store_set(store, &new_iter, NET_ENABLED_COLUMN, "", NET_NAME_COLUMN, display_name, NET_LIBRARY_COLUMN, NULL, -1);
			g_free(display_name);

			path = gtk_tree_model_get_path(model, &new_iter);
			row_ref = gtk_tree_row_reference_new(model, path);
			parent_iter = new_iter;
			parent_ptr = &parent_iter;

			join_array = g_new0(char *, try_depth + 2);
			memcpy(join_array, path_segments, sizeof(char *) * (try_depth + 1));

			hash_string = g_strjoinv(NET_HIERARCHY_SEPARATOR, join_array);
			g_free(join_array);

			/* Insert those node in the hash table */
			g_hash_table_insert(prefix_hash, hash_string, row_ref);
			/* Don't free hash_string, it is now oened by the hash table */
		}

		gtk_tree_store_append(store, &new_iter, parent_ptr);
		gtk_tree_store_set(store, &new_iter,
											 NET_ENABLED_COLUMN, menu->flag ? "" : "*",
											 NET_NAME_COLUMN, path_segments[path_depth - 1], NET_LIBRARY_COLUMN, menu, -1);
		g_strfreev(path_segments);
	}
	PCB_END_LOOP;

	g_hash_table_destroy(prefix_hash);

	return model;
}

/* Called when the user double clicks on a net in the left treeview. */
static void net_selection_double_click_cb(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *str;
	pcb_lib_menu_t *menu;

	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path)) {

		/* Expand / contract nodes with children */
		if (gtk_tree_model_iter_has_child(model, &iter)) {
			if (gtk_tree_view_row_expanded(treeview, path))
				gtk_tree_view_collapse_row(treeview, path);
			else
				gtk_tree_view_expand_row(treeview, path, FALSE);
			return;
		}

		/* Get the current enabled string and toggle it between "" and "*"
		 */
		gtk_tree_model_get(model, &iter, NET_ENABLED_COLUMN, &str, -1);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter, NET_ENABLED_COLUMN, !strcmp(str, "*") ? "" : "*", -1);
		/* set/clear the flag which says the net is enabled or disabled */
		gtk_tree_model_get(model, &iter, NET_LIBRARY_COLUMN, &menu, -1);
		menu->flag = strcmp(str, "*") == 0 ? 1 : 0;
		g_free(str);
	}
}

/* Called when the user clicks on a net in the left treeview. */
static void net_selection_changed_cb(GtkTreeSelection * selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	pcb_lib_menu_t *net;

	if (selection_holdoff)				/* PCB is highlighting, user is not selecting */
		return;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		selected_net = NULL;

		return;
	}

	/*  Get a pointer, net, to the pcb_lib_menu_t of the newly selected
	   netlist row, and create a new node model from the net entries
	   and insert that model into the node view.  Delete old entry model.
	 */
	gtk_tree_model_get(model, &iter, NET_LIBRARY_COLUMN, &net, -1);
	node_model_update(net);

	selected_net = net;
}

static void netlist_disable_all_cb(GtkToggleButton * button, gpointer data)
{
	GtkTreeIter iter;
	pcb_bool active = gtk_toggle_button_get_active(button);
	pcb_lib_menu_t *menu;

	/*  Get each net iter and change the NET_ENABLED_COLUMN to a "*" or ""
	   to flag it as disabled or enabled based on toggle button state.
	 */
	if (gtk_tree_model_get_iter_first(net_model, &iter))
		do {
			gtk_tree_store_set(GTK_TREE_STORE(net_model), &iter, NET_ENABLED_COLUMN, active ? "*" : "", -1);
			/* set/clear the flag which says the net is enabled or disabled */
			gtk_tree_model_get(net_model, &iter, NET_LIBRARY_COLUMN, &menu, -1);
			menu->flag = active ? 0 : 1;
		}
		while (gtk_tree_model_iter_next(net_model, &iter));
}

/* Select on the layout the current net treeview selection */
static void netlist_select_cb(GtkWidget * widget, gpointer data)
{
	char *name = NULL;

	if (!selected_net)
		return;

	name = selected_net->Name + 2;
	if (data == (void *)1)
		pcb_actionl("netlist", "select", name, NULL);
	else
		pcb_actionl("netlist", "unselect", name, NULL);
}

static void netlist_find_cb(GtkWidget * widget, gpointer data)
{
	char *name = NULL;

	if (!selected_net)
		return;

	name = selected_net->Name + 2;
	pcb_data_clear_flag(PCB->Data, PCB_FLAG_FOUND, 0, 1);
	pcb_actionl("netlist", "find", name, NULL);
}

TODO("padstack: this seems to be duplicate in lesstif")
static void netlist_rip_up_cb(GtkWidget * widget, gpointer data)
{

	if (!selected_net)
		return;
	netlist_find_cb(widget, data);

	PCB_LINE_VISIBLE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
			pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_VISIBLE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc))
			pcb_remove_object(PCB_OBJ_ARC, layer, arc, arc);
	}
	PCB_ENDALL_LOOP;

	if (PCB->pstk_on)
		PCB_PADSTACK_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, padstack) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack))
				pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
		}
		PCB_END_LOOP;
}

typedef struct {
	pcb_lib_entry_t *ret_val;
	pcb_lib_menu_t *node_net;
	const gchar *node_name;
	pcb_bool found;
} node_get_node_from_name_state;

static pcb_bool node_get_node_from_name_helper(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	pcb_lib_menu_t *net;
	pcb_lib_entry_t *node;
	node_get_node_from_name_state *state = data;

	gtk_tree_model_get(net_model, iter, NET_LIBRARY_COLUMN, &net, -1);
	/* Ignore non-nets (category headers) */
	if (net == NULL)
		return FALSE;

	/* Look for the node name in this net. */
	for (node = net->Entry; node - net->Entry < net->EntryN; node++)
		if (node->ListEntry && !strcmp(state->node_name, node->ListEntry)) {
			state->node_net = net;
			state->ret_val = node;
			/* stop iterating */
			state->found = TRUE;
			return TRUE;
		}
	return FALSE;
}

/* ---------- Manage the GUI treeview of the data models ----------- */
static gint netlist_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	pcb_event(PCB_EVENT_DAD_NEW_GEO, "psiiii", NULL, "netlist",
		(int)ev->x, (int)ev->y, (int)ev->width, (int)ev->height);
	return FALSE;
}

static void netlist_close_cb(GtkWidget * widget, gpointer data)
{
	pcb_gtk_common_t *com = data;

	gtk_widget_destroy(netlist_window);
	selected_net = NULL;
	netlist_window = NULL;

	/* For now, we are the only consumer of this API, so we can just do this */
	com->cancel_lead_user();
}

static void netlist_destroy_cb(GtkWidget * widget, void *data)
{
	selected_net = NULL;
	netlist_window = NULL;
}

static void ghid_netlist_window_create(pcb_gtk_common_t *com)
{
	GtkWidget *vbox, *hbox, *button, *label, *sep;
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* No point in putting up the window if no netlist is loaded.
	 */
	if (!PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN)
		return;

	if (netlist_window)
		return;

	netlist_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	pcb_gtk_winplace(netlist_window, "netlist");
	g_signal_connect(G_OBJECT(netlist_window), "destroy", G_CALLBACK(netlist_destroy_cb), NULL);
	gtk_window_set_title(GTK_WINDOW(netlist_window), _("pcb-rnd Netlist"));
	gtk_window_set_role(GTK_WINDOW(netlist_window), "PCB_Netlist");
	g_signal_connect(G_OBJECT(netlist_window), "configure_event", G_CALLBACK(netlist_window_configure_event_cb), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(netlist_window), 2);

	vbox = gtkc_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(netlist_window), vbox);
	hbox = gtkc_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);


	model = net_model_create();
	treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
	net_model = model;
	net_treeview = treeview;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(net_model), NET_NAME_COLUMN, GTK_SORT_ASCENDING);

	gtk_tree_view_set_rules_hint(treeview, FALSE);
	g_object_set(treeview, "enable-tree-lines", TRUE, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _(" "), renderer, "text", NET_ENABLED_COLUMN, NULL);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Net Name"), renderer, "text", NET_NAME_COLUMN, NULL);
	gtk_tree_view_insert_column(treeview, column, -1);
	gtk_tree_view_set_expander_column(treeview, column);

	/* TODO: dont expand all, but record expanded states when window is
	   |  destroyed and restore state here.
	 */
	gtk_tree_view_expand_all(treeview);

	selection = ghid_scrolled_selection(treeview, hbox,
																			GTK_SELECTION_SINGLE,
																			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC, net_selection_changed_cb, NULL);

	/* Connect to the double click event.
	 */
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(net_selection_double_click_cb), NULL);



	/* Create the elements treeview and wait for a callback to populate it.
	 */
	treeview = GTK_TREE_VIEW(gtk_tree_view_new());
	node_treeview = treeview;

	gtk_tree_view_set_rules_hint(treeview, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeview, -1, _("Nodes"), renderer, "text", NODE_NAME_COLUMN, NULL);

	selection = ghid_scrolled_selection(treeview, hbox,
																			GTK_SELECTION_SINGLE,
																			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC, node_selection_changed_cb, com);
	node_selection = selection;

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Operations on selected 'Net Name':"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 4);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

	button = gtk_button_new_with_label(_("Select"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(netlist_select_cb), GINT_TO_POINTER(1));

	button = gtk_button_new_with_label(_("Unselect"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(netlist_select_cb), GINT_TO_POINTER(0));

	button = gtk_button_new_with_label(_("Find"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(netlist_find_cb), GINT_TO_POINTER(0));

	button = gtk_button_new_with_label(_("Rip Up"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(netlist_rip_up_cb), GINT_TO_POINTER(0));

	pcb_gtk_check_button_connected(vbox, &disable_all_button, FALSE, TRUE, FALSE,
																 FALSE, 0, netlist_disable_all_cb, NULL, _("Disable all nets for adding rats"));

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 3);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(netlist_close_cb), com);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	gtk_widget_realize(netlist_window);
}

/* A forward declaration is needed here... Could it be avoided ? */
static pcb_lib_entry_t *node_get_node_from_name(pcb_gtk_common_t *com, gchar *node_name, pcb_lib_menu_t **node_net)
{
	node_get_node_from_name_state state;

	if (!node_name)
		return NULL;

	/*  Have to force the netlist window created because we need the treeview
	   models constructed to do the search.
	 */
	ghid_netlist_window_create(com);

	/*  Now walk through node entries of each net in the net model looking for
	   the node_name.
	 */
	state.found = 0;
	state.node_name = node_name;
	gtk_tree_model_foreach(net_model, node_get_node_from_name_helper, &state);
	if (state.found) {
		if (node_net)
			*node_net = state.node_net;
		return state.ret_val;
	}
	return NULL;
}

void pcb_gtk_dlg_netlist_show(pcb_gtk_common_t *com, pcb_bool raise)
{
	ghid_netlist_window_create(com);
	gtk_widget_show_all(netlist_window);
	pcb_gtk_dlg_netlist_update(com, TRUE);
	if (raise)
		gtk_window_present(GTK_WINDOW(netlist_window));
}

struct ggnfnn_task {
	pcb_bool enabled_only;
	const gchar *node_name;
	pcb_lib_menu_t *found_net;
	GtkTreeIter iter;
};

static pcb_bool hunt_named_node(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	struct ggnfnn_task *task = (struct ggnfnn_task *) data;
	pcb_lib_menu_t *net;
	pcb_lib_entry_t *node;
	gchar *str;
	gint j;
	pcb_bool is_disabled;

	/* We only want to inspect leaf nodes in the tree */
	if (gtk_tree_model_iter_has_child(model, iter))
		return FALSE;

	gtk_tree_model_get(model, iter, NET_LIBRARY_COLUMN, &net, -1);
	gtk_tree_model_get(model, iter, NET_ENABLED_COLUMN, &str, -1);
	is_disabled = !strcmp(str, "*");
	g_free(str);

	/* Don't check net nodes of disabled nets. */
	if (task->enabled_only && is_disabled)
		return FALSE;

	/* Look for the node name in this net. */
	for (j = net->EntryN, node = net->Entry; j; j--, node++)
		if (node->ListEntry && !strcmp(task->node_name, node->ListEntry)) {
			task->found_net = net;
			task->iter = *iter;
			return TRUE;
		}

	return FALSE;
}

/* API */

pcb_lib_menu_t *ghid_get_net_from_node_name(pcb_gtk_common_t *com, const gchar * node_name, pcb_bool enabled_only)
{
	GtkTreePath *path;
	struct ggnfnn_task task;

	if (!node_name)
		return NULL;

	/*  Have to force the netlist window created because we need the treeview
	   models constructed so we can find the pcb_lib_menu_t pointer the
	   caller wants.
	 */
	ghid_netlist_window_create(com);

	/* If no netlist is loaded the window doesn't appear. */
	if (netlist_window == NULL)
		return NULL;

	task.enabled_only = enabled_only;
	task.node_name = node_name;
	task.found_net = NULL;

	/*  Now walk through node entries of each net in the net model looking for
	   the node_name.
	 */
	gtk_tree_model_foreach(net_model, hunt_named_node, &task);

	/*  We are asked to highlight the found net if enabled_only is TRUE.
	   Set holdoff TRUE since this is just a highlight and user is not
	   expecting normal select action to happen?  Or should the node
	   treeview also get updated?  Original PCB code just tries to highlight.
	 */
	if (task.found_net && enabled_only) {
		selection_holdoff = TRUE;
		path = gtk_tree_model_get_path(net_model, &task.iter);
		gtk_tree_view_scroll_to_cell(net_treeview, path, NULL, TRUE, 0.5, 0.5);
		gtk_tree_selection_select_path(gtk_tree_view_get_selection(net_treeview), path);
		selection_holdoff = FALSE;
	}
	return task.found_net;
}

/* PCB LookupConnection code in find.c calls this if it wants a node
   and its net highlighted. */
void ghid_netlist_highlight_node(pcb_gtk_common_t *com, const gchar *node_name)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	pcb_lib_menu_t *net;
	gchar *name;

	if (!node_name)
		return;

	if ((net = ghid_get_net_from_node_name(com, node_name, TRUE)) == NULL)
		return;

	/*  We've found the net containing the node, so update the node treeview
	   to contain the nodes from the net.  Then we have to find the node
	   in the new node model so we can highlight it.
	 */
	node_model_update(net);

	if (gtk_tree_model_get_iter_first(node_model, &iter))
		do {
			gtk_tree_model_get(node_model, &iter, NODE_NAME_COLUMN, &name, -1);

			if (!strcmp(node_name, name)) {	/* found it, so highlight it */
				selection_holdoff = TRUE;
				selected_net = net;
				path = gtk_tree_model_get_path(node_model, &iter);
				gtk_tree_view_scroll_to_cell(node_treeview, path, NULL, TRUE, 0.5, 0.5);
				gtk_tree_selection_select_path(gtk_tree_view_get_selection(node_treeview), path);
				selection_holdoff = FALSE;
			}
			g_free(name);
		}
		while (gtk_tree_model_iter_next(node_model, &iter));
}

void pcb_gtk_dlg_netlist_update(pcb_gtk_common_t *com, pcb_bool init_nodes)
{
	GtkTreeModel *model;

	/* Make sure there is something to update */
	ghid_netlist_window_create(com);

	model = net_model;
	net_model = net_model_create();
	gtk_tree_view_set_model(net_treeview, net_model);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(net_model), NET_NAME_COLUMN, GTK_SORT_ASCENDING);
	if (model) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		g_object_unref(model);
	}

	selected_net = NULL;

	/* XXX Check if the select callback does this for us */
	if (init_nodes)
		node_model_update((&PCB->NetlistLib[PCB_NETLIST_EDITED])->Menu);
}

/* Actions ? */

void pcb_gtk_netlist_changed(pcb_gtk_common_t *com, void *user_data, int argc, pcb_event_arg_t argv[])
{
	loading_new_netlist = TRUE;
	pcb_gtk_dlg_netlist_update(com, TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_all_button), FALSE);
	loading_new_netlist = FALSE;
}

const char pcb_gtk_acts_netlistshow[] = "NetlistShow(pinname|netname)";
const char pcb_gtk_acth_netlistshow[] = "Selects the given pinname or netname in the netlist window. Does not show the window if it isn't already shown.";
fgw_error_t pcb_gtk_act_netlistshow(pcb_gtk_common_t *com, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op;
	const char *pcb_acts_netlistshow = pcb_gtk_acts_netlistshow;

	PCB_ACT_CONVARG(1, FGW_STR, netlistshow, op = argv[1].val.str);
	ghid_netlist_window_create(com);
	if (op != NULL)
		ghid_netlist_highlight_node(com, op);
	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_gtk_acts_netlistpresent[] = "NetlistPresent()";
const char pcb_gtk_acth_netlistpresent[] = "Presents the netlist window.";
fgw_error_t pcb_gtk_act_netlistpresent(pcb_gtk_common_t *com, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_gtk_dlg_netlist_show(com, TRUE);
	PCB_ACT_IRES(0);
	return 0;
}
