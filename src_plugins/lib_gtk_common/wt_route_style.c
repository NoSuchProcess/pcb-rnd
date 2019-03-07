/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Implementation of pcb_gtk_route_style_t widget. Opens a dialog upon "Edit" */

#include "config.h"
#include "conf_core.h"

#include <gdk/gdkkeysyms.h>

#include "dlg_route_style.h"

#include "pcb-printf.h"
#include "board.h"

#include "bu_status_line.h"


static gint action_count;

static GtkVBox *pcb_gtk_route_style_parent_class;
guint pcb_gtk_route_style_signals_id[STYLE_LAST_SIGNAL] = { 0 };


TODO(": this should be in core")
void pcb_gtk_route_style_copy(int idx)
{
	pcb_route_style_t *drst;

	if ((idx < 0) || (idx >= vtroutestyle_len(&PCB->RouteStyle)))
		return;
	drst = PCB->RouteStyle.array + idx;
	pcb_custom_route_style.Thick = drst->Thick;
	pcb_custom_route_style.Clearance = drst->Clearance;
	pcb_custom_route_style.Diameter = drst->Diameter;
	pcb_custom_route_style.Hole = drst->Hole;
}

/* Opens the Edit dialog */
static void edit_button_cb(GtkButton * btn, gpointer rss_)
{
	pcb_gtk_route_style_t *rss = rss_;
	pcb_gtk_route_style_edit_dialog(rss->com, rss);
}

/* user selection of a route style */
static void radio_select_cb(GtkToggleAction * action, gpointer data)
{
	pcb_gtk_route_style_t *rss = (pcb_gtk_route_style_t *) data;

	rss->active_style = g_object_get_data(G_OBJECT(action), "route-style");
	if (gtk_toggle_action_get_active(action))
		g_signal_emit(rss, pcb_gtk_route_style_signals_id[SELECT_STYLE_SIGNAL], 0, rss->active_style->rst);
}

/* CONSTRUCTOR */
static void ghid_route_style_init(pcb_gtk_route_style_t * rss)
{
}

/* Clean up object before garbage collection */
static void ghid_route_style_finalize(GObject * object)
{
	pcb_gtk_route_style_t *rss = (pcb_gtk_route_style_t *) object;

	pcb_gtk_route_style_empty(rss);

	g_object_unref(rss->accel_group);
	g_object_unref(rss->action_group);

	G_OBJECT_CLASS(pcb_gtk_route_style_parent_class)->finalize(object);
}

static void ghid_route_style_class_init(pcb_gtk_route_style_class_t * klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	pcb_gtk_route_style_parent_class = g_type_class_peek_parent(klass);

	pcb_gtk_route_style_signals_id[SELECT_STYLE_SIGNAL] =
		g_signal_new("select-style",
								 G_TYPE_FROM_CLASS(klass),
								 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
								 G_STRUCT_OFFSET(pcb_gtk_route_style_class_t, select_style),
								 NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
	pcb_gtk_route_style_signals_id[STYLE_EDITED_SIGNAL] =
		g_signal_new("style-edited",
								 G_TYPE_FROM_CLASS(klass),
								 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
								 G_STRUCT_OFFSET(pcb_gtk_route_style_class_t, style_edited),
								 NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	object_class->finalize = ghid_route_style_finalize;
}


/* PUBLIC FUNCTIONS */

GType pcb_gtk_route_style_get_type(void)
{
	static GType rss_type = 0;

	if (!rss_type) {
		const GTypeInfo rss_info = {
			sizeof(pcb_gtk_route_style_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_route_style_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_route_style_t),
			0,												/* n_preallocs */
			(GInstanceInitFunc) ghid_route_style_init,
		};

		rss_type = g_type_register_static(GTK_TYPE_VBOX, "pcb_gtk_route_style_t", &rss_info, 0);
	}

	return rss_type;
}

GtkWidget *pcb_gtk_route_style_new(pcb_gtk_common_t *com)
{
	pcb_gtk_route_style_t *rss = g_object_new(GHID_ROUTE_STYLE_TYPE, NULL);

	rss->com = com;
	rss->active_style = NULL;
	rss->action_radio_group = NULL;
	rss->button_radio_group = NULL;
	rss->model = gtk_list_store_new(STYLE_N_COLS, G_TYPE_STRING, G_TYPE_POINTER);

	rss->accel_group = gtk_accel_group_new();
	rss->action_group = gtk_action_group_new("RouteStyleSelector");

	/* Create edit button */
	rss->edit_button = gtk_button_new_with_label("Route Styles");
	g_signal_connect(G_OBJECT(rss->edit_button), "clicked", G_CALLBACK(edit_button_cb), rss);
	gtk_box_pack_start(GTK_BOX(rss), rss->edit_button, FALSE, FALSE, 0);

	return GTK_WIDGET(rss);
}

pcb_gtk_obj_route_style_t *pcb_gtk_route_style_add_route_style(pcb_gtk_route_style_t * rss, pcb_route_style_t * data, int hide)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *action_name = g_strdup_printf("RouteStyle%d", action_count);
	pcb_gtk_obj_route_style_t *new_style = g_malloc(sizeof(*new_style));

	/* Key the route style data with the pcb_route_style_t it controls */
	new_style->hidden = hide;
	new_style->rst = data;
	new_style->action = gtk_radio_action_new(action_name, data->name, NULL, NULL, action_count);
	gtk_radio_action_set_group(new_style->action, rss->action_radio_group);
	rss->action_radio_group = gtk_radio_action_get_group(new_style->action);
	new_style->button = gtk_radio_button_new(rss->button_radio_group);
	rss->button_radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(new_style->button));
	gtk_activatable_set_related_action(GTK_ACTIVATABLE(new_style->button), GTK_ACTION(new_style->action));

	gtk_list_store_append(rss->model, &iter);
	gtk_list_store_set(rss->model, &iter, STYLE_TEXT_COL, data->name, STYLE_DATA_COL, new_style, -1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(rss->model), &iter);
	new_style->rref = gtk_tree_row_reference_new(GTK_TREE_MODEL(rss->model), path);
	gtk_tree_path_free(path);

	/* Setup accelerator */
	if (action_count < 12) {
		gchar *accel = g_strdup_printf("<Ctrl>F%d", action_count + 1);
		gtk_action_set_accel_group(GTK_ACTION(new_style->action), rss->accel_group);
		gtk_action_group_add_action_with_accel(rss->action_group, GTK_ACTION(new_style->action), accel);
		g_free(accel);
	}

	/* Hookup and install radio button */
	g_object_set_data(G_OBJECT(new_style->action), "route-style", new_style);
	new_style->sig_id = g_signal_connect(G_OBJECT(new_style->action), "activate", G_CALLBACK(radio_select_cb), rss);
	gtk_box_pack_start(GTK_BOX(rss), new_style->button, FALSE, FALSE, 0);

	g_free(action_name);
	++action_count;

	if (hide)
		gtk_widget_hide(new_style->button);

	return new_style;
}

/* Adds a PCB route style to the widget. The route style object passed
   will be updated directly. */
static void add_route_style_with_hidden_check(pcb_gtk_route_style_t * rss, pcb_route_style_t * data)
{
	if (!rss->hidden_button) {
		if (*pcb_custom_route_style.name == '\0') {
			memset(&pcb_custom_route_style, 0, sizeof(pcb_custom_route_style));
			strcpy(pcb_custom_route_style.name, "<custom>");
			pcb_gtk_route_style_copy(0);
		}
		pcb_gtk_route_style_add_route_style(rss, &pcb_custom_route_style, 1);
		rss->hidden_button = 1;
	}
	if (data != NULL)
		pcb_gtk_route_style_add_route_style(rss, data, 0);
}

gboolean pcb_gtk_route_style_select_style(pcb_gtk_route_style_t * rss, pcb_route_style_t * rst)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(rss->model), &iter);

	do {
		pcb_gtk_obj_route_style_t *style;

		gtk_tree_model_get(GTK_TREE_MODEL(rss->model), &iter, STYLE_DATA_COL, &style, -1);
		if ((style != NULL) && (style->rst == rst)) {
			g_signal_handler_block(G_OBJECT(style->action), style->sig_id);
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(style->action), TRUE);
			g_signal_handler_unblock(G_OBJECT(style->action), style->sig_id);
			rss->active_style = style;
			g_signal_emit(rss, pcb_gtk_route_style_signals_id[SELECT_STYLE_SIGNAL], 0, rss->active_style->rst);
			return TRUE;
		}
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(rss->model), &iter));

	return FALSE;
}

GtkAccelGroup *pcb_gtk_route_style_get_accel_group(pcb_gtk_route_style_t * rss)
{
	return rss->accel_group;
}

void pcb_gtk_route_style_sync(pcb_gtk_route_style_t * rss,
															pcb_coord_t Thick, pcb_coord_t Hole, pcb_coord_t Diameter, pcb_coord_t Clearance)
{
	GtkTreeIter iter;
	int target, n;

	/* Always update the label - even if there's no style, the current settings need to show */
	rss->com->set_status_line_label();

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(rss->model), &iter))
		return;

	target = pcb_route_style_lookup(&PCB->RouteStyle, Thick, Diameter, Hole, Clearance, NULL);
	if (target == -1) {
		pcb_gtk_obj_route_style_t *style;

		/* None of the styles matched: select the hidden custom button */
		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(rss->model), &iter))
			return;

		gtk_tree_model_get(GTK_TREE_MODEL(rss->model), &iter, STYLE_DATA_COL, &style, -1);
		g_signal_handler_block(G_OBJECT(style->action), style->sig_id);
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(style->action), TRUE);
		g_signal_handler_unblock(G_OBJECT(style->action), style->sig_id);
		rss->active_style = style;

		return;
	}

	/* need to select the style with index stored in target */
	n = -1;
	do {
		pcb_gtk_obj_route_style_t *style;
		gtk_tree_model_get(GTK_TREE_MODEL(rss->model), &iter, STYLE_DATA_COL, &style, -1);
		if (n == target) {
			g_signal_handler_block(G_OBJECT(style->action), style->sig_id);
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(style->action), TRUE);
			g_signal_handler_unblock(G_OBJECT(style->action), style->sig_id);
			rss->active_style = style;
			break;
		}
		n++;
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(rss->model), &iter));
}

void pcb_gtk_route_style_empty(pcb_gtk_route_style_t * rss)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(rss->model), &iter)) {
		do {
			pcb_gtk_obj_route_style_t *rsdata;
			gtk_tree_model_get(GTK_TREE_MODEL(rss->model), &iter, STYLE_DATA_COL, &rsdata, -1);
			if (rsdata != NULL) {
				if (rsdata->action) {
					gtk_action_disconnect_accelerator(GTK_ACTION(rsdata->action));
					gtk_action_group_remove_action(rss->action_group, GTK_ACTION(rsdata->action));
					g_object_unref(G_OBJECT(rsdata->action));
				}
				if (rsdata->button)
					gtk_widget_destroy(GTK_WIDGET(rsdata->button));;
				gtk_tree_row_reference_free(rsdata->rref);
				free(rsdata);
			}
		} while (gtk_list_store_remove(rss->model, &iter));
	}

	/* ugly hack: get the menu updated */
	rss->com->route_styles_edited_cb();

	rss->action_radio_group = NULL;
	rss->button_radio_group = NULL;
	rss->hidden_button = 0;
}

/* Called when a route style is selected. */
static void route_style_changed_cb(pcb_gtk_route_style_t * rss, pcb_route_style_t * rst, gpointer data)
{
	pcb_use_route_style(rst);
	rss->com->set_status_line_label();
}


static void route_styles_edited_cb(pcb_gtk_route_style_t *rss, gboolean save, gpointer data)
{
	pcb_gtk_common_t *com = data;
	com->route_styles_edited_cb();
}

void make_route_style_buttons(pcb_gtk_route_style_t * rss)
{
	int i;

	/* Make sure the <custom> item is added */
	add_route_style_with_hidden_check(rss, NULL);

	for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); ++i)
		add_route_style_with_hidden_check(rss, &PCB->RouteStyle.array[i]);
	g_signal_connect(G_OBJECT(rss), "select_style", G_CALLBACK(route_style_changed_cb), NULL);
	g_signal_connect(G_OBJECT(rss), "style_edited", G_CALLBACK(route_styles_edited_cb), rss->com);

	/* ugly hack: get the menu updated */
	rss->com->route_styles_edited_cb();
}
