/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/*! \file <wt_layer_selector.c>
 *  \brief Implementation of pcb_gtk_layer_selector_t widget
 *  \par Description
 *  This widget is the layer selector on the left side of the Gtk
 *  GUI. It also builds the relevant sections of the menu for layer
 *  selection and visibility toggling, and keeps these in sync.
 */

#include "config.h"

#include "wt_layer_selector.h"

#include "pcb-printf.h"
#include "hid_actions.h"

#include "wt_layer_selector_cr.h"


#define INITIAL_ACTION_MAX	40

/* Forward dec'ls */
struct _layer;
static void ghid_layer_selector_finalize(GObject * object);
static void menu_pick_cb(GtkRadioAction * action, struct _layer *ldata);

/*! \brief Signals exposed by the widget */
enum {
	SELECT_LAYER_SIGNAL,
	TOGGLE_LAYER_SIGNAL,
	LAST_SIGNAL
};

/*! \brief Columns used for internal data store */
enum {
	STRUCT_COL,
	USER_ID_COL,
	VISIBLE_COL,
	COLOR_COL,
	TEXT_COL,
	FONT_COL,
	ACTIVATABLE_COL,
	SEPARATOR_COL,
	GROUP_COL,
	N_COLS
};

static GtkTreeView *ghid_layer_selector_parent_class;
static guint ghid_layer_selector_signals[LAST_SIGNAL] = { 0 };

struct pcb_gtk_layer_selector_s {
	GtkTreeView parent;

	GtkListStore *list_store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *visibility_column;

	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;

	GSList *radio_group;
	int n_actions;

	gboolean accel_available[20];

	gboolean last_activatable;

	gulong selection_changed_sig_id;
};

struct pcb_gtk_layer_selector_class_s {
	GtkTreeViewClass parent_class;

	void (*select_layer) (pcb_gtk_layer_selector_t *, gint);
	void (*toggle_layer) (pcb_gtk_layer_selector_t *, gint);
};

struct _layer {
	gint accel_index;							/* Index into ls->accel_available */
	GtkWidget *pick_item;
	GtkWidget *view_item;
	GtkToggleAction *view_action;
	GtkRadioAction *pick_action;
	GtkTreeRowReference *rref;
};

/*! \brief Deletes the action and accelerator from a layer */
static void free_ldata(pcb_gtk_layer_selector_t * ls, struct _layer *ldata)
{
	if (ldata->pick_action) {
		gtk_action_disconnect_accelerator(GTK_ACTION(ldata->pick_action));
		gtk_action_group_remove_action(ls->action_group, GTK_ACTION(ldata->pick_action));
/* TODO: make this work without wrecking the radio action group
 *           g_object_unref (G_OBJECT (ldata->pick_action)); 
 *                   */
	}
	if (ldata->view_action) {
		gtk_action_disconnect_accelerator(GTK_ACTION(ldata->view_action));
		gtk_action_group_remove_action(ls->action_group, GTK_ACTION(ldata->view_action));
		g_object_unref(G_OBJECT(ldata->view_action));
	}
	gtk_tree_row_reference_free(ldata->rref);
	if (ldata->accel_index >= 0)
		ls->accel_available[ldata->accel_index] = TRUE;
	g_free(ldata);

}

/*! \brief internal set-visibility function -- emits no signals */
static void set_visibility(pcb_gtk_layer_selector_t * ls, GtkTreeIter * iter, struct _layer *ldata, gboolean state)
{
	gtk_list_store_set(ls->list_store, iter, VISIBLE_COL, state, -1);

	if ((ldata) && (ldata->view_item)) {
		gtk_action_block_activate(GTK_ACTION(ldata->view_action));
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ldata->view_item), state);
		gtk_action_unblock_activate(GTK_ACTION(ldata->view_action));
	}
}

/*! \brief Flips the visibility state of a given layer 
 *  \par Function Description
 *  Changes the internal toggle state and menu checkbox state
 *  of the layer pointed to by iter. Emits a toggle-layer signal.
 *
 *  \param [in] ls    The selector to be acted on
 *  \param [in] iter  A GtkTreeIter pointed at the relevant layer
 *  \param [in] emit  Whether or not to emit a signal
 */
static void toggle_visibility(pcb_gtk_layer_selector_t * ls, GtkTreeIter * iter, gboolean emit)
{
	gint user_id;
	struct _layer *ldata;
	gboolean toggle;
	gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), iter, USER_ID_COL, &user_id, VISIBLE_COL, &toggle, STRUCT_COL, &ldata, -1);
	set_visibility(ls, iter, ldata, !toggle);
	if (emit)
		g_signal_emit(ls, ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL], 0, user_id);
}

/*! \brief Decides if a GtkListStore entry is a layer or separator */
static gboolean tree_view_separator_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean ret_val;
	gtk_tree_model_get(model, iter, SEPARATOR_COL, &ret_val, -1);
	return ret_val;
}

/*! \brief Decides if a GtkListStore entry is a group */
static gboolean tree_view_group_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean ret_val;
	gtk_tree_model_get(model, iter, GROUP_COL, &ret_val, -1);
	return ret_val;
}

/*! \brief Decides if a GtkListStore entry may be selected */
static gboolean tree_selection_func(GtkTreeSelection * selection,
																		GtkTreeModel * model, GtkTreePath * path, gboolean selected, gpointer data)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gboolean activatable;
		gtk_tree_model_get(model, &iter, ACTIVATABLE_COL, &activatable, -1);
		return activatable;
	}

	return FALSE;
}

/* SIGNAL HANDLERS */
/*! \brief Callback for mouse-click: toggle visibility */
static gboolean button_press_cb(pcb_gtk_layer_selector_t * ls, GdkEventButton * event)
{
	/* Handle visibility independently to prevent changing the active
	 *  layer, which will happen if we let this event propagate.  */
	GtkTreeViewColumn *column;
	GtkTreePath *path;

	/* Ignore the synthetic presses caused by double and tripple clicks, and
	 * also ignore all but left-clicks
	 */
	if (event->type != GDK_BUTTON_PRESS)
		return TRUE;

	if (event->button == 3)
		return pcb_hid_actionl("Popup", "layer", NULL);

	if ((event->button == 1) == (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(ls), event->x, event->y, &path, &column, NULL, NULL))) {
		GtkTreeIter iter;
		gboolean activatable, separator, group;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(ls->list_store), &iter, path);
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, ACTIVATABLE_COL, &activatable, SEPARATOR_COL, &separator, GROUP_COL, &group, -1);
		/* Toggle visibility for non-activatable layers no matter
		 *  where you click. */
		if (!separator && (column == ls->visibility_column || !activatable)) {
			toggle_visibility(ls, &iter, TRUE);
			return TRUE;
		}
		return FALSE;
	}

	/* unhandled button */
	return TRUE;
}

/*! \brief Callback for layer selection change: sync menu, emit signal */
static void selection_changed_cb(GtkTreeSelection * selection, pcb_gtk_layer_selector_t * ls)
{
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		gint user_id;
		struct _layer *ldata;
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, STRUCT_COL, &ldata, USER_ID_COL, &user_id, -1);

		if (ldata && ldata->pick_action) {
			gtk_action_block_activate(GTK_ACTION(ldata->pick_action));
			gtk_radio_action_set_current_value(ldata->pick_action, user_id);
			gtk_action_unblock_activate(GTK_ACTION(ldata->pick_action));
		}
		g_signal_emit(ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL], 0, user_id);
	}
}

/*! \brief Callback for menu actions: sync layer selection list, emit signal */
static void menu_view_cb(GtkToggleAction * action, struct _layer *ldata)
{
	pcb_gtk_layer_selector_t *ls;
	GtkTreeModel *model = gtk_tree_row_reference_get_model(ldata->rref);
	GtkTreePath *path = gtk_tree_row_reference_get_path(ldata->rref);
	gboolean state = gtk_toggle_action_get_active(action);
	GtkTreeIter iter;
	gint user_id;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, VISIBLE_COL, state, -1);
	gtk_tree_model_get(model, &iter, USER_ID_COL, &user_id, -1);

	ls = g_object_get_data(G_OBJECT(model), "layer-selector");
	g_signal_emit(ls, ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL], 0, user_id);
}

/*! \brief Callback for menu actions: sync layer selection list, emit signal */
static void menu_pick_cb(GtkRadioAction * action, struct _layer *ldata)
{
	/* We only care about the activation signal (as opposed to deactivation).
	 * A row we are /deactivating/ might not even exist anymore! */
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
		pcb_gtk_layer_selector_t *ls;
		GtkTreeModel *model = gtk_tree_row_reference_get_model(ldata->rref);
		GtkTreePath *path = gtk_tree_row_reference_get_path(ldata->rref);
		GtkTreeIter iter;
		gint user_id;

		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, USER_ID_COL, &user_id, -1);

		ls = g_object_get_data(G_OBJECT(model), "layer-selector");
		g_signal_handler_block(ls->selection, ls->selection_changed_sig_id);
		gtk_tree_selection_select_path(ls->selection, path);
		g_signal_handler_unblock(ls->selection, ls->selection_changed_sig_id);
		g_signal_emit(ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL], 0, user_id);
	}
}

/* CONSTRUCTOR */
static void ghid_layer_selector_init(pcb_gtk_layer_selector_t * ls)
{
	/* Hookup signal handlers */
}

static void ghid_layer_selector_class_init(pcb_gtk_layer_selector_class_t * klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	ghid_layer_selector_signals[SELECT_LAYER_SIGNAL] =
		g_signal_new("select-layer",
								 G_TYPE_FROM_CLASS(klass),
								 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
								 G_STRUCT_OFFSET(pcb_gtk_layer_selector_class_t, select_layer),
								 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
	ghid_layer_selector_signals[TOGGLE_LAYER_SIGNAL] =
		g_signal_new("toggle-layer",
								 G_TYPE_FROM_CLASS(klass),
								 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
								 G_STRUCT_OFFSET(pcb_gtk_layer_selector_class_t, toggle_layer),
								 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

	object_class->finalize = ghid_layer_selector_finalize;
}

/*! \brief Cleans up object before garbage collection */
static void ghid_layer_selector_finalize(GObject * object)
{
	GtkTreeIter iter;
	pcb_gtk_layer_selector_t *ls = (pcb_gtk_layer_selector_t *) object;

	g_object_unref(ls->accel_group);
	g_object_unref(ls->action_group);

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	do {
		struct _layer *ldata;
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, STRUCT_COL, &ldata, -1);
		free_ldata(ls, ldata);
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));

	G_OBJECT_CLASS(ghid_layer_selector_parent_class)->finalize(object);
}

/* PUBLIC FUNCTIONS */
GType pcb_gtk_layer_selector_get_type(void)
{
	static GType ls_type = 0;

	if (!ls_type) {
		const GTypeInfo ls_info = {
			sizeof(pcb_gtk_layer_selector_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_layer_selector_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_layer_selector_t),
			0,												/* n_preallocs */
			(GInstanceInitFunc) ghid_layer_selector_init,
		};

		ls_type = g_type_register_static(GTK_TYPE_TREE_VIEW, "pcb_gtk_layer_selector_t", &ls_info, 0);
	}

	return ls_type;
}

GtkWidget *pcb_gtk_layer_selector_new(void)
{
	int i;
	GtkCellRenderer *renderer1 = ghid_cell_renderer_visibility_new();
	GtkCellRenderer *renderer2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *opacity_col = gtk_tree_view_column_new_with_attributes("", renderer1,
																																						"active", VISIBLE_COL,
																																						"color", COLOR_COL, NULL);
	GtkTreeViewColumn *name_col = gtk_tree_view_column_new_with_attributes("", renderer2,
																																				 "text", TEXT_COL,
																																				 "font", FONT_COL,
																																				 NULL);

	pcb_gtk_layer_selector_t *ls = g_object_new(GHID_LAYER_SELECTOR_TYPE, NULL);

	/* action index, active, color, text, font, is_separator */
	ls->list_store = gtk_list_store_new(N_COLS, G_TYPE_POINTER, G_TYPE_INT,
																			G_TYPE_BOOLEAN, G_TYPE_STRING,
																			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(ls), opacity_col, -1);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(ls), name_col, -1);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ls), GTK_TREE_MODEL(ls->list_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ls), FALSE);

	ls->last_activatable = TRUE;
	ls->visibility_column = opacity_col;
	ls->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ls));
	ls->accel_group = gtk_accel_group_new();
	ls->action_group = gtk_action_group_new("LayerSelector");
	ls->n_actions = 0;
	for (i = 0; i < 20; ++i)
		ls->accel_available[i] = TRUE;

	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(ls), tree_view_separator_func, NULL, NULL);
	gtk_tree_selection_set_select_function(ls->selection, tree_selection_func, NULL, NULL);
	gtk_tree_selection_set_mode(ls->selection, GTK_SELECTION_BROWSE);

	g_object_set_data(G_OBJECT(ls->list_store), "layer-selector", ls);
	g_signal_connect(ls, "button_press_event", G_CALLBACK(button_press_cb), NULL);
	ls->selection_changed_sig_id = g_signal_connect(ls->selection, "changed", G_CALLBACK(selection_changed_cb), ls);

	g_object_ref(ls->accel_group);

	return GTK_WIDGET(ls);
}

void pcb_gtk_layer_selector_add_layer(pcb_gtk_layer_selector_t * ls,
																			gint user_id,
																			const gchar * name, const gchar * color_string, gboolean visible, gboolean activatable, gboolean is_group)
{
	struct _layer *new_layer = NULL;
	gchar *pname, *vname;
	gboolean new_iter = TRUE;
	gboolean last_activatable = TRUE;
	GtkTreePath *path;
	GtkTreeIter iter;
	int i;

	/* Look for existing layer with this ID */
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter))
		do {
			gboolean is_sep, active;
			gint read_id;
			gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store),
												 &iter, USER_ID_COL, &read_id, SEPARATOR_COL, &is_sep, ACTIVATABLE_COL, &active, -1);
			if (!is_sep) {
				last_activatable = active;
				if (read_id == user_id) {
					new_iter = FALSE;
					break;
				}
			}
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));

	/* Handle separator addition */
	if (new_iter) {
		if (activatable != last_activatable) {
			/* Add separator between activatable/non-activatable boundaries */
			gtk_list_store_append(ls->list_store, &iter);
			gtk_list_store_set(ls->list_store, &iter, STRUCT_COL, NULL, SEPARATOR_COL, TRUE, -1);
		}
		/* Create new layer */
		new_layer = malloc(sizeof(*new_layer));
		gtk_list_store_append(ls->list_store, &iter);
		gtk_list_store_set(ls->list_store, &iter,
											 STRUCT_COL, new_layer,
											 USER_ID_COL, user_id,
											 VISIBLE_COL, visible,
											 COLOR_COL, color_string,
											 TEXT_COL, name,
											 FONT_COL, activatable ? NULL : "Italic",
											 ACTIVATABLE_COL, activatable,
											 SEPARATOR_COL, FALSE,
											 GROUP_COL, is_group,
											 -1);
	}
	else {
		/* If the row exists, we clear out its ldata to create
		 * a new action, accelerator and menu item. */
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, STRUCT_COL, &new_layer, -1);
		free_ldata(ls, new_layer);
		new_layer = malloc(sizeof(*new_layer));

		gtk_list_store_set(ls->list_store, &iter,
											 STRUCT_COL, new_layer,
											 VISIBLE_COL, visible,
											 COLOR_COL, color_string,
											 TEXT_COL, name,
											 FONT_COL, activatable ? NULL : "Italic",
											 ACTIVATABLE_COL, activatable,
											 GROUP_COL, is_group,
											 -1);
	}

	/* -- Setup new actions -- */
	vname = g_strdup_printf("LayerView%d", ls->n_actions);
	pname = g_strdup_printf("LayerPick%d", ls->n_actions);

	/* Create row reference for actions */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(ls->list_store), &iter);
	new_layer->rref = gtk_tree_row_reference_new(GTK_TREE_MODEL(ls->list_store), path);
	gtk_tree_path_free(path);

	/* Create selection action */
	if (activatable) {
		new_layer->pick_action = gtk_radio_action_new(pname, name, NULL, NULL, user_id);
		gtk_radio_action_set_group(new_layer->pick_action, ls->radio_group);
		ls->radio_group = gtk_radio_action_get_group(new_layer->pick_action);
	}
	else
		new_layer->pick_action = NULL;

	/* Create visibility action */
	new_layer->view_action = gtk_toggle_action_new(vname, name, NULL, NULL);
	gtk_toggle_action_set_active(new_layer->view_action, visible);

	/* Determine keyboard accelerators */
	for (i = 0; i < 20; ++i)
		if (ls->accel_available[i])
			break;
	if (i < 20) {
		/* Map 1-0 to actions 1-10 (with '0' meaning 10) */
		gchar *accel1 = g_strdup_printf("%s%d",
																		i < 10 ? "" : "<Alt>",
																		(i + 1) % 10);
		gchar *accel2 = g_strdup_printf("<Ctrl>%s%d",
																		i < 10 ? "" : "<Alt>",
																		(i + 1) % 10);

		if (activatable) {
			GtkAction *action = GTK_ACTION(new_layer->pick_action);
			gtk_action_set_accel_group(action, ls->accel_group);
			gtk_action_group_add_action_with_accel(ls->action_group, action, accel1);
			gtk_action_connect_accelerator(action);
			g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(menu_pick_cb), new_layer);
		}
		gtk_action_set_accel_group(GTK_ACTION(new_layer->view_action), ls->accel_group);
		gtk_action_group_add_action_with_accel(ls->action_group, GTK_ACTION(new_layer->view_action), accel2);
		gtk_action_connect_accelerator(GTK_ACTION(new_layer->view_action));
		g_signal_connect(G_OBJECT(new_layer->view_action), "activate", G_CALLBACK(menu_view_cb), new_layer);

		ls->accel_available[i] = FALSE;
		new_layer->accel_index = i;
		g_free(accel2);
		g_free(accel1);
	}
	else {
		new_layer->accel_index = -1;
	}
	/* finalize new layer struct */
	new_layer->pick_item = new_layer->view_item = NULL;

	/* cleanup */
	g_free(vname);
	g_free(pname);

	ls->n_actions++;
}

gint pcb_gtk_layer_selector_install_pick_items(pcb_gtk_layer_selector_t * ls, GtkMenuShell * shell, gint pos)
{
	GtkTreeIter iter;
	int n = 0;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	do {
		struct _layer *ldata;
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, STRUCT_COL, &ldata, -1);
		if (ldata && ldata->pick_action) {
			GtkAction *action = GTK_ACTION(ldata->pick_action);
			ldata->pick_item = gtk_action_create_menu_item(action);
			gtk_menu_shell_insert(shell, ldata->pick_item, pos + n);
			++n;
		}
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));

	return n;
}

gint pcb_gtk_layer_selector_install_view_items(pcb_gtk_layer_selector_t * ls, GtkMenuShell * shell, gint pos)
{
	GtkTreeIter iter;
	int n = 0;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	do {
		struct _layer *ldata;
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, STRUCT_COL, &ldata, -1);
		if (ldata && ldata->view_action) {
			GtkAction *action = GTK_ACTION(ldata->view_action);
			ldata->view_item = gtk_action_create_menu_item(action);
			gtk_menu_shell_insert(shell, ldata->view_item, pos + n);
			++n;
		}
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));

	return n;
}

GtkAccelGroup *pcb_gtk_layer_selector_get_accel_group(pcb_gtk_layer_selector_t * ls)
{
	return ls->accel_group;
}

/*! \brief used internally */
static gboolean toggle_foreach_func(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	gint id;
	pcb_gtk_layer_selector_t *ls = g_object_get_data(G_OBJECT(model),
																									 "layer-selector");

	gtk_tree_model_get(model, iter, USER_ID_COL, &id, -1);
	if (id == *(gint *) data) {
		toggle_visibility(ls, iter, TRUE);
		return TRUE;
	}
	return FALSE;
}

void pcb_gtk_layer_selector_toggle_layer(pcb_gtk_layer_selector_t * ls, gint user_id)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(ls->list_store), toggle_foreach_func, &user_id);
}

/*! \brief used internally */
static gboolean select_foreach_func(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	gint id;
	pcb_gtk_layer_selector_t *ls = g_object_get_data(G_OBJECT(model),
																									 "layer-selector");

	gtk_tree_model_get(model, iter, USER_ID_COL, &id, -1);
	if (id == *(gint *) data) {
		gtk_tree_selection_select_path(ls->selection, path);
		return TRUE;
	}
	return FALSE;
}

void pcb_gtk_layer_selector_select_layer(pcb_gtk_layer_selector_t * ls, gint user_id)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(ls->list_store), select_foreach_func, &user_id);
}

gboolean pcb_gtk_layer_selector_select_next_visible(pcb_gtk_layer_selector_t * ls)
{
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(ls->selection, NULL, &iter)) {
		/* Scan forward, looking for selectable iter */
		do {
			gboolean visible, activatable;
			gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, VISIBLE_COL, &visible, ACTIVATABLE_COL, &activatable, -1);
			if (visible && activatable) {
				gtk_tree_selection_select_iter(ls->selection, &iter);
				return TRUE;
			}
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));
		/* Move iter to start, and repeat. */
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
		do {
			gboolean visible, activatable;
			gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, VISIBLE_COL, &visible, ACTIVATABLE_COL, &activatable, -1);
			if (visible && activatable) {
				gtk_tree_selection_select_iter(ls->selection, &iter);
				return TRUE;
			}
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));
		/* Failing this, just emit a selected signal on the original layer. */
		selection_changed_cb(ls->selection, ls);
	}
	/* If we get here, nothing is selectable, so fail. */
	return FALSE;
}

void pcb_gtk_layer_selector_update_colors(pcb_gtk_layer_selector_t * ls, const gchar * (*callback) (int user_id))
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	do {
		gint user_id;
		const gchar *new_color;
		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store), &iter, USER_ID_COL, &user_id, -1);
		new_color = callback(user_id);
		if (new_color != NULL)
			gtk_list_store_set(ls->list_store, &iter, COLOR_COL, new_color, -1);
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));
}

void pcb_gtk_layer_selector_delete_layers(pcb_gtk_layer_selector_t * ls, gboolean(*callback) (int user_id))
{
	GtkTreeIter iter, last_iter;

	gboolean iter_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	while (iter_valid) {
		struct _layer *ldata;
		gboolean sep, was_sep = FALSE;
		gint user_id;

		/* Find next iter to delete */
		while (iter_valid) {
			gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store),
												 &iter, USER_ID_COL, &user_id, STRUCT_COL, &ldata, SEPARATOR_COL, &sep, -1);
			if (!sep && callback(user_id))
				break;

			/* save iter in case it's a bad separator */
			was_sep = sep;
			last_iter = iter;
			/* iterate */
			iter_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter);
		}

		if (iter_valid) {
			/* remove preceeding separator */
			if (was_sep)
				gtk_list_store_remove(ls->list_store, &last_iter);

					/*** remove row ***/
			iter_valid = gtk_list_store_remove(ls->list_store, &iter);
			free_ldata(ls, ldata);
		}
		last_iter = iter;
	}
}

void pcb_gtk_layer_selector_show_layers(pcb_gtk_layer_selector_t * ls, gboolean(*callback) (int user_id))
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls->list_store), &iter);
	do {
		struct _layer *ldata;
		gboolean sep, is_grp;
		gint user_id;

		gtk_tree_model_get(GTK_TREE_MODEL(ls->list_store),
											 &iter, USER_ID_COL, &user_id, STRUCT_COL, &ldata, SEPARATOR_COL, &sep, GROUP_COL, &is_grp, -1);
		if ((!sep) && (!is_grp))
			set_visibility(ls, &iter, ldata, callback(user_id));
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ls->list_store), &iter));
}
