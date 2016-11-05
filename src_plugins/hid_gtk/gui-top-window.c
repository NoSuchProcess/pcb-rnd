/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* #define DEBUG_MENUS */

#ifdef DAN_FIXME
TODO:

-what about stuff like this:

	/* Set to ! because ActionDisplay toggles it */
conf_core.editor.draw_grid = !gtk_toggle_action_get_active(action);
hid_actionl("Display", "Grid", "", NULL);
ghid_set_status_line_label();


I NEED TO DO THE STATUS LINE THING.for example shift - alt - v to change the
	via size.NOte the status line label does not get updated properly until a zoom in / out.
#endif
/* This file was originally written by Bill Wilson for the PCB Gtk
 * port.  It was later heavily modified by Dan McMahill to provide
 * user customized menus.
*/
/* gui-top-window.c
|  This handles creation of the top level window and all its widgets.
|  events for the Output.drawing_area widget are handled in a separate
|  file gui-output-events.c
|
|  Some caveats with menu shorcut keys:  Some keys are trapped out by Gtk
|  and can't be used as shortcuts (eg. '|', TAB, etc).  For these cases
|  we have our own shortcut table and capture the keys and send the events
|  there in ghid_port_key_press_cb().
*/
#include "config.h"
#include "conf_core.h"

#include <locale.h>
#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"
#include "gtkhid.h"
#include "gui.h"
#include "hid.h"
#include "hid_cfg.h"
#include "hid_cfg_action.h"
#include "action_helper.h"
#include "buffer.h"
#include "change.h"
#include "copy.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "plug_io.h"
#include "find.h"
#include "insert.h"
#include "layer.h"
#include "move.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "event.h"
#include "free_atexit.h"
#include "paths.h"
#include "gui-icons-mode-buttons.data"
#include "gui-icons-misc.data"
#include "win_place.h"
#include "hid_attrib.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "route_style.h"
#include "compat_nls.h"
#include "obj_line.h"

static pcb_bool ignore_layer_update;

static GtkWidget *ghid_load_menus(void);

GhidGui _ghidgui, *ghidgui = &_ghidgui;

GHidPort ghid_port, *gport;

hid_cfg_t *ghid_cfg = NULL;
hid_cfg_mouse_t ghid_mouse;
hid_cfg_keys_t ghid_keymap;

/*! \brief callback for ghid_main_menu_update_toggle_state () */
void menu_toggle_update_cb(GtkAction * act, const char *tflag, const char *aflag)
{
	if (tflag != NULL) {
		int v = hid_get_flag(tflag);
		if (v < 0) {
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), 0);
			gtk_action_set_sensitive(act, 0);
		}
		else
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), ! !v);
	}
	if (aflag != NULL) {
		int v = hid_get_flag(aflag);
		gtk_action_set_sensitive(act, ! !v);
	}
}

/*! \brief sync the menu checkboxes with actual pcb state */
void ghid_update_toggle_flags()
{
	ghid_main_menu_update_toggle_state(GHID_MAIN_MENU(ghidgui->menu_bar), menu_toggle_update_cb);
}

static void h_adjustment_changed_cb(GtkAdjustment * adj, GhidGui * g)
{
	if (g->adjustment_changed_holdoff)
		return;

	ghid_port_ranges_changed();
}

static void v_adjustment_changed_cb(GtkAdjustment * adj, GhidGui * g)
{
	if (g->adjustment_changed_holdoff)
		return;

	ghid_port_ranges_changed();
}

	/* Save size of top window changes so PCB can restart at its size at exit.
	 */
static gint top_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, GHidPort * port)
{
	wplc_config_event(widget, &hid_gtk_wgeo.top_x, &hid_gtk_wgeo.top_y, &hid_gtk_wgeo.top_width, &hid_gtk_wgeo.top_height);

	return FALSE;
}

static void info_bar_response_cb(GtkInfoBar * info_bar, gint response_id, GhidGui * _gui)
{
	gtk_widget_destroy(_gui->info_bar);
	_gui->info_bar = NULL;

	if (response_id == GTK_RESPONSE_ACCEPT)
		RevertPCB();
}

static void close_file_modified_externally_prompt(void)
{
	if (ghidgui->info_bar != NULL)
		gtk_widget_destroy(ghidgui->info_bar);
	ghidgui->info_bar = NULL;
}

static void show_file_modified_externally_prompt(void)
{
	GtkWidget *button;
	GtkWidget *button_image;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *content_area;
	char *file_path_utf8;
	const char *secondary_text;
	char *markup;

	close_file_modified_externally_prompt();

	ghidgui->info_bar = gtk_info_bar_new();

	button = gtk_info_bar_add_button(GTK_INFO_BAR(ghidgui->info_bar), _("Reload"), GTK_RESPONSE_ACCEPT);
	button_image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), button_image);

	gtk_info_bar_add_button(GTK_INFO_BAR(ghidgui->info_bar), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(ghidgui->info_bar), GTK_MESSAGE_WARNING);
	gtk_box_pack_start(GTK_BOX(ghidgui->vbox_middle), ghidgui->info_bar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(ghidgui->vbox_middle), ghidgui->info_bar, 0);


	g_signal_connect(ghidgui->info_bar, "response", G_CALLBACK(info_bar_response_cb), ghidgui);

	file_path_utf8 = g_filename_to_utf8(PCB->Filename, -1, NULL, NULL, NULL);

	secondary_text = PCB->Changed ? "Do you want to drop your changes and reload the file?" : "Do you want to reload the file?";

	markup = g_markup_printf_escaped(_("<b>The file %s has changed on disk</b>\n\n%s"), file_path_utf8, secondary_text);
	g_free(file_path_utf8);

	content_area = gtk_info_bar_get_content_area(GTK_INFO_BAR(ghidgui->info_bar));

	icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(content_area), icon, FALSE, FALSE, 0);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 6);

	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);

	gtk_misc_set_alignment(GTK_MISC(label), 0., 0.5);

	gtk_widget_show_all(ghidgui->info_bar);
}

static pcb_bool check_externally_modified(void)
{
	GFile *file;
	GFileInfo *info;
	GTimeVal timeval;

	/* Treat zero time as a flag to indicate we've not got an mtime yet */
	if (PCB->Filename == NULL || (ghidgui->our_mtime.tv_sec == 0 && ghidgui->our_mtime.tv_usec == 0))
		return pcb_false;

	file = g_file_new_for_path(PCB->Filename);
	info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref(file);

	if (info == NULL || !g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		return pcb_false;

	g_file_info_get_modification_time(info, &timeval);	/*&ghidgui->last_seen_mtime); */
	g_object_unref(info);

	/* Ignore when the file on disk is the same age as when we last looked */
	if (timeval.tv_sec == ghidgui->last_seen_mtime.tv_sec && timeval.tv_usec == ghidgui->last_seen_mtime.tv_usec)
		return pcb_false;

	ghidgui->last_seen_mtime = timeval;

	return (ghidgui->last_seen_mtime.tv_sec > ghidgui->our_mtime.tv_sec) ||
		(ghidgui->last_seen_mtime.tv_sec == ghidgui->our_mtime.tv_sec &&
		 ghidgui->last_seen_mtime.tv_usec > ghidgui->our_mtime.tv_usec);
}

static gboolean top_window_enter_cb(GtkWidget * widget, GdkEvent * event, GHidPort * port)
{
	if (check_externally_modified())
		show_file_modified_externally_prompt();

	return FALSE;
}

/*! \brief Menu action callback function
 *  \par Function Description
 *  This is the main menu callback function.  The callback receives
 *  the original lihata action node pointer HID actions to be
 *  executed.
 *
 *  \param [in]   The action that was activated
 *  \param [in]   The related menu lht action node
 */

static void ghid_menu_cb(GtkAction * action, const lht_node_t * node)
{
	if (action == NULL || node == NULL)
		return;

	hid_cfg_action(node);

	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	AdjustAttachedObjects();
	ghid_invalidate_all();
	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
}

static void update_board_mtime_from_disk(void)
{
	GFile *file;
	GFileInfo *info;

	ghidgui->our_mtime.tv_sec = 0;
	ghidgui->our_mtime.tv_usec = 0;
	ghidgui->last_seen_mtime = ghidgui->our_mtime;

	if (PCB->Filename == NULL)
		return;

	file = g_file_new_for_path(PCB->Filename);
	info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref(file);

	if (info == NULL || !g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		return;

	g_file_info_get_modification_time(info, &ghidgui->our_mtime);
	g_object_unref(info);

	ghidgui->last_seen_mtime = ghidgui->our_mtime;
}

	/* Sync toggle states that were saved with the layout and notify the
	   |  config code to update Settings values it manages.
	 */
void ghid_sync_with_new_layout(void)
{
	if (vtroutestyle_len(&PCB->RouteStyle) > 0) {
		pcb_use_route_style(&PCB->RouteStyle.array[0]);
		ghid_route_style_selector_select_style(GHID_ROUTE_STYLE_SELECTOR(ghidgui->route_style_selector), &PCB->RouteStyle.array[0]);
	}

	ghid_config_handle_units_changed();

	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	close_file_modified_externally_prompt();
	update_board_mtime_from_disk();
}

void ghid_notify_save_pcb(const char *filename, pcb_bool done)
{
	/* Do nothing if it is not the active PCB file that is being saved.
	 */
	if (PCB->Filename == NULL || strcmp(filename, PCB->Filename) != 0)
		return;

	if (done)
		update_board_mtime_from_disk();
}

void ghid_notify_filename_changed(void)
{
	/* Pick up the mtime of the new PCB file */
	update_board_mtime_from_disk();
	ghid_window_set_name_label(PCB->Name);
}

/* ---------------------------------------------------------------------------
 *
 * layer_process()
 *
 * Takes the index into the layers and produces the text string for
 * the layer and if the layer is currently visible or not.  This is
 * used by a couple of functions.
 *
 */
static void layer_process(const gchar ** color_string, const char **text, int *set, int i)
{
	int tmp;
	const char *tmps;
	const gchar *tmpc;

	/* cheap hack to let users pass in NULL for either text or set if
	 * they don't care about the result
	 */

	if (color_string == NULL)
		color_string = &tmpc;

	if (text == NULL)
		text = &tmps;

	if (set == NULL)
		set = &tmp;

	switch (i) {
	case LAYER_BUTTON_SILK:
		*color_string = conf_core.appearance.color.element;
		*text = _("silk");
		*set = PCB->ElementOn;
		break;
	case LAYER_BUTTON_RATS:
		*color_string = conf_core.appearance.color.rat;
		*text = _("rat lines");
		*set = PCB->RatOn;
		break;
	case LAYER_BUTTON_PINS:
		*color_string = conf_core.appearance.color.pin;
		*text = _("pins/pads");
		*set = PCB->PinOn;
		break;
	case LAYER_BUTTON_VIAS:
		*color_string = conf_core.appearance.color.via;
		*text = _("vias");
		*set = PCB->ViaOn;
		break;
	case LAYER_BUTTON_FARSIDE:
		*color_string = conf_core.appearance.color.invisible_objects;
		*text = _("far side");
		*set = PCB->InvisibleObjectsOn;
		break;
	case LAYER_BUTTON_MASK:
		*color_string = conf_core.appearance.color.mask;
		*text = _("solder mask");
		*set = conf_core.editor.show_mask;
		break;
	default:											/* layers */
		*color_string = conf_core.appearance.color.layer[i];
		*text = (char *) UNKNOWN(PCB->Data->Layer[i].Name);
		*set = PCB->Data->Layer[i].On;
		break;
	}
}

/*! \brief Callback for GHidLayerSelector layer selection */
static void layer_selector_select_callback(GHidLayerSelector * ls, int layer, gpointer d)
{
	ignore_layer_update = pcb_true;
	/* Select Layer */
	PCB->SilkActive = (layer == LAYER_BUTTON_SILK);
	PCB->RatDraw = (layer == LAYER_BUTTON_RATS);
	if (layer == LAYER_BUTTON_SILK) {
		PCB->ElementOn = pcb_true;
		hid_action("LayersChanged");
	}
	else if (layer == LAYER_BUTTON_RATS) {
		PCB->RatOn = pcb_true;
		hid_action("LayersChanged");
	}
	else if (layer < max_copper_layer)
		ChangeGroupVisibility(layer, TRUE, pcb_true);

	ignore_layer_update = pcb_false;

	ghid_invalidate_all();
}

/*! \brief Callback for GHidLayerSelector layer toggling */
static void layer_selector_toggle_callback(GHidLayerSelector * ls, int layer, gpointer d)
{
	gboolean redraw = FALSE;
	gboolean active;
	layer_process(NULL, NULL, &active, layer);

	active = !active;
	ignore_layer_update = pcb_true;
	switch (layer) {
	case LAYER_BUTTON_SILK:
		PCB->ElementOn = active;
		PCB->Data->SILKLAYER.On = PCB->ElementOn;
		PCB->Data->BACKSILKLAYER.On = PCB->ElementOn;
		redraw = 1;
		break;
	case LAYER_BUTTON_RATS:
		PCB->RatOn = active;
		redraw = 1;
		break;
	case LAYER_BUTTON_PINS:
		PCB->PinOn = active;
		redraw |= (elementlist_length(&PCB->Data->Element) != 0);
		break;
	case LAYER_BUTTON_VIAS:
		PCB->ViaOn = active;
		redraw |= (pinlist_length(&PCB->Data->Via) != 0);
		break;
	case LAYER_BUTTON_FARSIDE:
		PCB->InvisibleObjectsOn = active;
		PCB->Data->BACKSILKLAYER.On = (active && PCB->ElementOn);
		redraw = TRUE;
		break;
	case LAYER_BUTTON_MASK:
		if (active)
			conf_set_editor(show_mask, 1);
		else
			conf_set_editor(show_mask, 0);
		redraw = TRUE;
		break;
	default:
		/* Flip the visibility */
		ChangeGroupVisibility(layer, active, pcb_false);
		redraw = TRUE;
		break;
	}

	/* Select the next visible layer. (If there is none, this will
	 * select the currently-selected layer, triggering the selection
	 * callback, which will turn the visibility on.) This way we
	 * will never have an invisible layer selected.
	 */
	if (!active)
		ghid_layer_selector_select_next_visible(ls);

	ignore_layer_update = pcb_false;

	if (redraw)
		ghid_invalidate_all();
}

/*! \brief Install menu bar and accelerator groups */
void ghid_install_accel_groups(GtkWindow * window, GhidGui * gui)
{
	gtk_window_add_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(gui->menu_bar)));
	gtk_window_add_accel_group(window, ghid_layer_selector_get_accel_group(GHID_LAYER_SELECTOR(gui->layer_selector)));
	gtk_window_add_accel_group
		(window, ghid_route_style_selector_get_accel_group(GHID_ROUTE_STYLE_SELECTOR(gui->route_style_selector)));
}

/*! \brief Remove menu bar and accelerator groups */
void ghid_remove_accel_groups(GtkWindow * window, GhidGui * gui)
{
	gtk_window_remove_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(gui->menu_bar)));
	gtk_window_remove_accel_group(window, ghid_layer_selector_get_accel_group(GHID_LAYER_SELECTOR(gui->layer_selector)));
	gtk_window_remove_accel_group
		(window, ghid_route_style_selector_get_accel_group(GHID_ROUTE_STYLE_SELECTOR(gui->route_style_selector)));
}

/* Refreshes the window title bar and sets the PCB name to the
 * window title bar or to a seperate label
 */
void ghid_window_set_name_label(gchar * name)
{
	gchar *str;
	gchar *filename;

	/* FIXME -- should this happen?  It does... */
	/* This happens if we're calling an exporter from the command line */
	if (ghidgui == NULL)
		return;

	dup_string(&(ghidgui->name_label_string), name);
	if (!ghidgui->name_label_string || !*ghidgui->name_label_string)
		ghidgui->name_label_string = g_strdup(_("Unnamed"));

	if (!PCB->Filename || !*PCB->Filename)
		filename = g_strdup(_("Unsaved.pcb"));
	else
		filename = g_strdup(PCB->Filename);

	str = g_strdup_printf("%s%s (%s) - pcb-rnd", PCB->Changed ? "*" : "", ghidgui->name_label_string, filename);
	gtk_window_set_title(GTK_WINDOW(gport->top_window), str);
	g_free(str);
	g_free(filename);
}

static void grid_units_button_cb(GtkWidget * widget, gpointer data)
{
	/* Button only toggles between mm and mil */
	if (conf_core.editor.grid_unit == get_unit_struct("mm"))
		hid_actionl("SetUnits", "mil", NULL);
	else
		hid_actionl("SetUnits", "mm", NULL);
}

/*
 * The two following callbacks are used to keep the absolute
 * and relative cursor labels from growing and shrinking as you
 * move the cursor around.
 */
static void absolute_label_size_req_cb(GtkWidget * widget, GtkRequisition * req, gpointer data)
{

	static gint w = 0;
	if (req->width > w)
		w = req->width;
	else
		req->width = w;
}

static void relative_label_size_req_cb(GtkWidget * widget, GtkRequisition * req, gpointer data)
{

	static gint w = 0;
	if (req->width > w)
		w = req->width;
	else
		req->width = w;
}

static void make_cursor_position_labels(GtkWidget * hbox, GHidPort * port)
{
	GtkWidget *frame, *label;

	/* The grid units button next to the cursor position labels.
	 */
	ghidgui->grid_units_button = gtk_button_new();
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), conf_core.editor.grid_unit->in_suffix);
	ghidgui->grid_units_label = label;
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(ghidgui->grid_units_button), label);
	gtk_box_pack_end(GTK_BOX(hbox), ghidgui->grid_units_button, FALSE, TRUE, 0);
	g_signal_connect(ghidgui->grid_units_button, "clicked", G_CALLBACK(grid_units_button_cb), NULL);

	/* The absolute cursor position label
	 */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

	label = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(frame), label);
	ghidgui->cursor_position_absolute_label = label;
	g_signal_connect(G_OBJECT(label), "size-request", G_CALLBACK(absolute_label_size_req_cb), NULL);


	/* The relative cursor position label
	 */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	label = gtk_label_new(" __.__  __.__ ");
	gtk_container_add(GTK_CONTAINER(frame), label);
	ghidgui->cursor_position_relative_label = label;
	g_signal_connect(G_OBJECT(label), "size-request", G_CALLBACK(relative_label_size_req_cb), NULL);

}

/* \brief Add "virtual layers" to a layer selector */
static void make_virtual_layer_buttons(GtkWidget * layer_selector)
{
	GHidLayerSelector *layersel = GHID_LAYER_SELECTOR(layer_selector);
	const gchar *text;
	const gchar *color_string;
	gboolean active;

	layer_process(&color_string, &text, &active, LAYER_BUTTON_SILK);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_SILK, text, color_string, active, TRUE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_RATS);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_RATS, text, color_string, active, TRUE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_PINS);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_PINS, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_VIAS);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_VIAS, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_FARSIDE);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_FARSIDE, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_MASK);
	ghid_layer_selector_add_layer(layersel, LAYER_BUTTON_MASK, text, color_string, active, FALSE);
}

/*! \brief callback for ghid_layer_selector_update_colors */
const gchar *get_layer_color(gint layer)
{
	const gchar *rv;
	layer_process(&rv, NULL, NULL, layer);
	return rv;
}

/*! \brief Update a layer selector's color scheme */
void ghid_layer_buttons_color_update(void)
{
	ghid_layer_selector_update_colors(GHID_LAYER_SELECTOR(ghidgui->layer_selector), get_layer_color);
	pcb_colors_from_settings(PCB);
}

/*! \brief Populate a layer selector with all layers Gtk is aware of */
static void make_layer_buttons(GtkWidget * layersel)
{
	gint i;
	const gchar *text;
	const gchar *color_string;
	gboolean active = TRUE;

	for (i = 0; i < max_copper_layer; ++i) {
		layer_process(&color_string, &text, &active, i);
		ghid_layer_selector_add_layer(GHID_LAYER_SELECTOR(layersel), i, text, color_string, active, TRUE);
	}
}


/*! \brief callback for ghid_layer_selector_delete_layers */
gboolean get_layer_delete(gint layer)
{
	return layer >= max_copper_layer;
}

/*! \brief Synchronize layer selector widget with current PCB state
 *  \par Function Description
 *  Called when user toggles layer visibility or changes drawing layer,
 *  or when layer visibility is changed programatically.
 */
void ghid_layer_buttons_update(void)
{
	gint layer;

	if (ignore_layer_update)
		return;

	ghid_layer_selector_delete_layers(GHID_LAYER_SELECTOR(ghidgui->layer_selector), get_layer_delete);
	make_layer_buttons(ghidgui->layer_selector);
	make_virtual_layer_buttons(ghidgui->layer_selector);
	ghid_main_menu_install_layer_selector(GHID_MAIN_MENU(ghidgui->menu_bar), GHID_LAYER_SELECTOR(ghidgui->layer_selector));

	/* Sync selected layer with PCB's state */
	if (PCB->RatDraw)
		layer = LAYER_BUTTON_RATS;
	else if (PCB->SilkActive)
		layer = LAYER_BUTTON_SILK;
	else
		layer = LayerStack[0];

	ghid_layer_selector_select_layer(GHID_LAYER_SELECTOR(ghidgui->layer_selector), layer);
}

/*! \brief Called when user clicks OK on route style dialog */
static void route_styles_edited_cb(GHidRouteStyleSelector * rss, gboolean save, gpointer data)
{
	conf_setf(CFR_DESIGN, "design/routes", -1, "%s", make_route_string(&PCB->RouteStyle));
	if (save)
		conf_setf(CFR_USER, "design/routes", -1, "%s", make_route_string(&PCB->RouteStyle));
	ghid_main_menu_install_route_style_selector
		(GHID_MAIN_MENU(ghidgui->menu_bar), GHID_ROUTE_STYLE_SELECTOR(ghidgui->route_style_selector));
}

/*! \brief Called when a route style is selected */
static void route_style_changed_cb(GHidRouteStyleSelector * rss, RouteStyleType * rst, gpointer data)
{
	pcb_use_route_style(rst);
	ghid_set_status_line_label();
}

/*! \brief Configure the route style selector */
void make_route_style_buttons(GHidRouteStyleSelector * rss)
{
	int i;

	/* Make sure the <custom> item is added */
	ghid_route_style_selector_add_route_style(rss, NULL);

	for (i = 0; i < vtroutestyle_len(&PCB->RouteStyle); ++i)
		ghid_route_style_selector_add_route_style(rss, &PCB->RouteStyle.array[i]);
	g_signal_connect(G_OBJECT(rss), "select_style", G_CALLBACK(route_style_changed_cb), NULL);
	g_signal_connect(G_OBJECT(rss), "style_edited", G_CALLBACK(route_styles_edited_cb), NULL);
}

/*
 *  ---------------------------------------------------------------
 * Mode buttons
 */
typedef struct {
	GtkWidget *button;
	GtkWidget *toolbar_button;
	guint button_cb_id;
	guint toolbar_button_cb_id;
	const gchar *name;
	gint mode;
	const gchar **xpm;
} ModeButton;


static ModeButton mode_buttons[] = {
	{NULL, NULL, 0, 0, "via", PCB_MODE_VIA, via},
	{NULL, NULL, 0, 0, "line", PCB_MODE_LINE, line},
	{NULL, NULL, 0, 0, "arc", PCB_MODE_ARC, arc},
	{NULL, NULL, 0, 0, "text", PCB_MODE_TEXT, text},
	{NULL, NULL, 0, 0, "rectangle", PCB_MODE_RECTANGLE, rect},
	{NULL, NULL, 0, 0, "polygon", PCB_MODE_POLYGON, poly},
	{NULL, NULL, 0, 0, "polygonhole", PCB_MODE_POLYGON_HOLE, polyhole},
	{NULL, NULL, 0, 0, "buffer", PCB_MODE_PASTE_BUFFER, buf},
	{NULL, NULL, 0, 0, "remove", PCB_MODE_REMOVE, del},
	{NULL, NULL, 0, 0, "rotate", PCB_MODE_ROTATE, rot},
	{NULL, NULL, 0, 0, "insertPoint", PCB_MODE_INSERT_POINT, ins},
	{NULL, NULL, 0, 0, "thermal", PCB_MODE_THERMAL, thrm},
	{NULL, NULL, 0, 0, "select", PCB_MODE_ARROW, sel},
	{NULL, NULL, 0, 0, "lock", PCB_MODE_LOCK, lock}
};

static gint n_mode_buttons = G_N_ELEMENTS(mode_buttons);

static void do_set_mode(int mode)
{
	SetMode(mode);
	ghid_mode_cursor(mode);
	ghidgui->settings_mode = mode;
}

static void mode_toolbar_button_toggled_cb(GtkToggleButton * button, ModeButton * mb)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (mb->button != NULL) {
		g_signal_handler_block(mb->button, mb->button_cb_id);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->button), active);
		g_signal_handler_unblock(mb->button, mb->button_cb_id);
	}

	if (active)
		do_set_mode(mb->mode);
}

static void mode_button_toggled_cb(GtkWidget * widget, ModeButton * mb)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (mb->toolbar_button != NULL) {
		g_signal_handler_block(mb->toolbar_button, mb->toolbar_button_cb_id);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->toolbar_button), active);
		g_signal_handler_unblock(mb->toolbar_button, mb->toolbar_button_cb_id);
	}

	if (active)
		do_set_mode(mb->mode);
}

void ghid_mode_buttons_update(void)
{
	ModeButton *mb;
	gint i;

	for (i = 0; i < n_mode_buttons; ++i) {
		mb = &mode_buttons[i];
		if (conf_core.editor.mode == mb->mode) {
			g_signal_handler_block(mb->button, mb->button_cb_id);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->button), TRUE);
			g_signal_handler_unblock(mb->button, mb->button_cb_id);

			g_signal_handler_block(mb->toolbar_button, mb->toolbar_button_cb_id);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->toolbar_button), TRUE);
			g_signal_handler_unblock(mb->toolbar_button, mb->toolbar_button_cb_id);
			break;
		}
	}
}

void ghid_pack_mode_buttons(void)
{
	if (conf_hid_gtk.plugins.hid_gtk.compact_vertical) {
		gtk_widget_hide(ghidgui->mode_buttons_frame);
		gtk_widget_show_all(ghidgui->mode_toolbar);
	}
	else {
		gtk_widget_hide(ghidgui->mode_toolbar);
		gtk_widget_show_all(ghidgui->mode_buttons_frame);
	}
}

static void make_mode_buttons_and_toolbar(GtkWidget ** mode_frame, GtkWidget ** mode_toolbar, GtkWidget ** mode_toolbar_vbox)
{
	GtkToolItem *tool_item;
	GtkWidget *vbox, *hbox = NULL;
	GtkWidget *pad_hbox, *pad_vbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GSList *group = NULL;
	GSList *toolbar_group = NULL;
	ModeButton *mb;
	int n_mb, i, tb_width = 0;

	*mode_toolbar = gtk_toolbar_new();

	*mode_frame = gtk_frame_new(NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(*mode_frame), vbox);

	for (i = 0; i < n_mode_buttons; ++i) {
		mb = &mode_buttons[i];

		/* Create tool button for mode frame */
		mb->button = gtk_radio_button_new(group);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(mb->button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mb->button), FALSE);

		/* Create tool button for toolbar */
		mb->toolbar_button = gtk_radio_button_new(toolbar_group);
		toolbar_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(mb->toolbar_button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mb->toolbar_button), FALSE);

		/* Pack mode-frame button into the frame */
		n_mb = conf_hid_gtk.plugins.hid_gtk.n_mode_button_columns;
		if ((n_mb < 1) || (n_mb > 10))
			n_mb = 3;
		if ((i % n_mb) == 0) {
			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		}
		gtk_box_pack_start(GTK_BOX(hbox), mb->button, FALSE, FALSE, 0);

		/* Create a container for the toolbar button and add that */
		tool_item = gtk_tool_item_new();
		gtk_container_add(GTK_CONTAINER(tool_item), mb->toolbar_button);
		gtk_toolbar_insert(GTK_TOOLBAR(*mode_toolbar), tool_item, -1);

		/* Load the image for the button, create GtkImage widgets for both
		 * the grid button and the toolbar button, then pack into the buttons
		 */
		pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) mb->xpm);
		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_container_add(GTK_CONTAINER(mb->button), image);
		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_container_add(GTK_CONTAINER(mb->toolbar_button), image);
		g_object_unref(pixbuf);
		tb_width += image->requisition.width;

		if (strcmp(mb->name, "select") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->button), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->toolbar_button), TRUE);
		}

		mb->button_cb_id = g_signal_connect(mb->button, "toggled", G_CALLBACK(mode_button_toggled_cb), mb);
		mb->toolbar_button_cb_id = g_signal_connect(mb->toolbar_button, "toggled", G_CALLBACK(mode_toolbar_button_toggled_cb), mb);
	}

	*mode_toolbar_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(*mode_toolbar_vbox), *mode_toolbar, FALSE, FALSE, 0);

	/* Pack an empty, wide hbox right below the toolbar and make it as wide
	   as the calculated width of the toolbar with some tuning. Toolbar icons
	   disappear if the container hbox is not wide enough. Without this hack
	   the width would be determined by the menu bar, and that could be short
	   if the user changes the menu layout. */
	pad_hbox = gtk_hbox_new(FALSE, 0);
	pad_vbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pad_hbox), pad_vbox, FALSE, FALSE, tb_width * 3 / 4);
	gtk_box_pack_start(GTK_BOX(*mode_toolbar_vbox), pad_hbox, FALSE, FALSE, 0);
}


/*
 * ---------------------------------------------------------------
 * Top window
 * ---------------------------------------------------------------
 */

static gint delete_chart_cb(GtkWidget * widget, GdkEvent * event, GHidPort * port)
{
	if (ghid_entry_loop != NULL)
		g_main_loop_quit(ghid_entry_loop);

	hid_action("Quit");

	/*
	 * Return TRUE to keep our app running.  A FALSE here would let the
	 * delete signal continue on and kill our program.
	 */
	return TRUE;
}

static void destroy_chart_cb(GtkWidget * widget, GHidPort * port)
{
	ghid_shutdown_renderer(port);
	gtk_main_quit();
}

static void get_widget_styles(GtkStyle ** menu_bar_style, GtkStyle ** tool_button_style, GtkStyle ** tool_button_label_style)
{
	GtkWidget *tool_button;
	GtkWidget *tool_button_label;
	GtkToolItem *tool_item;

	/* Build a tool item to extract the theme's styling for a toolbar button with text */
	tool_item = gtk_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(ghidgui->mode_toolbar), tool_item, 0);
	tool_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(tool_item), tool_button);
	tool_button_label = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(tool_button), tool_button_label);

	/* Grab the various styles we need */
	gtk_widget_ensure_style(ghidgui->menu_bar);
	*menu_bar_style = gtk_widget_get_style(ghidgui->menu_bar);

	gtk_widget_ensure_style(tool_button);
	*tool_button_style = gtk_widget_get_style(tool_button);

	gtk_widget_ensure_style(tool_button_label);
	*tool_button_label_style = gtk_widget_get_style(tool_button_label);

	gtk_widget_destroy(GTK_WIDGET(tool_item));
}

static void do_fix_topbar_theming(void)
{
	GtkWidget *rel_pos_frame;
	GtkWidget *abs_pos_frame;
	GtkStyle *menu_bar_style;
	GtkStyle *tool_button_style;
	GtkStyle *tool_button_label_style;

	get_widget_styles(&menu_bar_style, &tool_button_style, &tool_button_label_style);

	/* Style the top bar background as if it were all a menu bar */
	gtk_widget_set_style(ghidgui->top_bar_background, menu_bar_style);

	/* Style the cursor position labels using the menu bar style as well.
	 * If this turns out to cause problems with certain gtk themes, we may
	 * need to grab the GtkStyle associated with an actual menu item to
	 * get a text color to render with.
	 */
	gtk_widget_set_style(ghidgui->cursor_position_relative_label, menu_bar_style);
	gtk_widget_set_style(ghidgui->cursor_position_absolute_label, menu_bar_style);

	/* Style the units button as if it were a toolbar button - hopefully
	 * this isn't too ugly sitting on a background themed as a menu bar.
	 * It is unlikely any theme defines colours for a GtkButton sitting on
	 * a menu bar.
	 */
	rel_pos_frame = gtk_widget_get_parent(ghidgui->cursor_position_relative_label);
	abs_pos_frame = gtk_widget_get_parent(ghidgui->cursor_position_absolute_label);
	gtk_widget_set_style(rel_pos_frame, menu_bar_style);
	gtk_widget_set_style(abs_pos_frame, menu_bar_style);
	gtk_widget_set_style(ghidgui->grid_units_button, tool_button_style);
	gtk_widget_set_style(ghidgui->grid_units_label, tool_button_label_style);
}

/* Attempt to produce a conststent style for our extra menu-bar items by
 * copying aspects from the menu bar style set by the user's GTK theme.
 * Setup signal handlers to update our efforts if the user changes their
 * theme whilst we are running.
 */
static void fix_topbar_theming(void)
{
	GtkSettings *settings;

	do_fix_topbar_theming();

	settings = gtk_widget_get_settings(ghidgui->top_bar_background);
	g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(do_fix_topbar_theming), NULL);
	g_signal_connect(settings, "notify::gtk-font-name", G_CALLBACK(do_fix_topbar_theming), NULL);
}

static void fullscreen_cb(GtkButton *btn, void *data)
{
	conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !conf_core.editor.fullscreen, POL_OVERWRITE);
}

/*
 * Create the top_window contents.  The config settings should be loaded
 * before this is called.
 */
static void ghid_build_pcb_top_window(void)
{
	GtkWidget *window;
	GtkWidget *vbox_main, *hbox_middle, *hbox;
	GtkWidget *vbox, *frame, *hbox_scroll, *fullscreen_btn;
	GtkWidget *label;
	GHidPort *port = &ghid_port;
	GtkWidget *scrolled;

	window = gport->top_window;

	vbox_main = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox_main);

	/* -- Top control bar */
	ghidgui->top_bar_background = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(vbox_main), ghidgui->top_bar_background, FALSE, FALSE, 0);

	ghidgui->top_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ghidgui->top_bar_background), ghidgui->top_hbox);

	/*
	 * menu_hbox will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	ghidgui->menu_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ghidgui->top_hbox), ghidgui->menu_hbox, FALSE, FALSE, 0);

	ghidgui->menubar_toolbar_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ghidgui->menu_hbox), ghidgui->menubar_toolbar_vbox, FALSE, FALSE, 0);

	/* Build layer menus */
	ghidgui->layer_selector = ghid_layer_selector_new();
	make_layer_buttons(ghidgui->layer_selector);
	make_virtual_layer_buttons(ghidgui->layer_selector);
	g_signal_connect(G_OBJECT(ghidgui->layer_selector), "select_layer", G_CALLBACK(layer_selector_select_callback), NULL);
	g_signal_connect(G_OBJECT(ghidgui->layer_selector), "toggle_layer", G_CALLBACK(layer_selector_toggle_callback), NULL);
	/* Build main menu */
	ghidgui->menu_bar = ghid_load_menus();
	gtk_box_pack_start(GTK_BOX(ghidgui->menubar_toolbar_vbox), ghidgui->menu_bar, FALSE, FALSE, 0);

	make_mode_buttons_and_toolbar(&ghidgui->mode_buttons_frame, &ghidgui->mode_toolbar, &ghidgui->mode_toolbar_vbox);
	gtk_box_pack_start(GTK_BOX(ghidgui->menubar_toolbar_vbox), ghidgui->mode_toolbar_vbox, FALSE, FALSE, 0);

	ghidgui->position_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(ghidgui->top_hbox), ghidgui->position_hbox, FALSE, FALSE, 0);

	make_cursor_position_labels(ghidgui->position_hbox, port);

	hbox_middle = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_middle, TRUE, TRUE, 0);

	fix_topbar_theming();					/* Must be called after toolbar is created */

	/* -- Left control bar */
	/*
	 * This box will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	ghidgui->left_toolbar = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), ghidgui->left_toolbar, FALSE, FALSE, 0);

	vbox = ghid_scrolled_vbox(ghidgui->left_toolbar, &scrolled, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), ghidgui->layer_selector, FALSE, FALSE, 0);

	/* ghidgui->mode_buttons_frame was created above in the call to
	 * make_mode_buttons_and_toolbar (...);
	 */
	gtk_box_pack_start(GTK_BOX(ghidgui->left_toolbar), ghidgui->mode_buttons_frame, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(ghidgui->left_toolbar), frame, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
	ghidgui->route_style_selector = ghid_route_style_selector_new();
	make_route_style_buttons(GHID_ROUTE_STYLE_SELECTOR(ghidgui->route_style_selector));
	gtk_box_pack_start(GTK_BOX(hbox), ghidgui->route_style_selector, FALSE, FALSE, 0);

	ghidgui->vbox_middle = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), ghidgui->vbox_middle, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ghidgui->vbox_middle), hbox, TRUE, TRUE, 0);

	/* -- The PCB layout output drawing area */

	gport->drawing_area = gtk_drawing_area_new();
	ghid_init_drawing_widget(gport->drawing_area, gport);

	gtk_widget_add_events(gport->drawing_area, GDK_EXPOSURE_MASK
												| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
												| GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
												| GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
												| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	/*
	 * This is required to get the drawing_area key-press-event.  Also the
	 * enter and button press callbacks grab focus to be sure we have it
	 * when in the drawing_area.
	 */
	gtk_widget_set_can_focus(gport->drawing_area, TRUE);

	gtk_box_pack_start(GTK_BOX(hbox), gport->drawing_area, TRUE, TRUE, 0);

	ghidgui->v_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);
	ghidgui->v_range = gtk_vscrollbar_new(GTK_ADJUSTMENT(ghidgui->v_adjustment));

	gtk_box_pack_start(GTK_BOX(hbox), ghidgui->v_range, FALSE, FALSE, 0);



	g_signal_connect(G_OBJECT(ghidgui->v_adjustment), "value_changed", G_CALLBACK(v_adjustment_changed_cb), ghidgui);

	ghidgui->h_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);

	hbox_scroll = gtk_hbox_new(FALSE, 0);
	ghidgui->h_range = gtk_hscrollbar_new(GTK_ADJUSTMENT(ghidgui->h_adjustment));
	fullscreen_btn = gtk_button_new_with_label("FS");
	g_signal_connect(GTK_OBJECT(fullscreen_btn), "clicked", G_CALLBACK(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), ghidgui->h_range, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), fullscreen_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ghidgui->vbox_middle), hbox_scroll, FALSE, FALSE, 0);


	g_signal_connect(G_OBJECT(ghidgui->h_adjustment), "value_changed", G_CALLBACK(h_adjustment_changed_cb), ghidgui);

	/* -- The bottom status line label */
	ghidgui->status_line_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ghidgui->vbox_middle), ghidgui->status_line_hbox, FALSE, FALSE, 0);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(ghidgui->status_line_hbox), label, FALSE, FALSE, 0);
	ghidgui->status_line_label = label;

	/* Depending on user setting, the command_combo_box may get packed into
	   |  the status_line_hbox, but it will happen on demand the first time
	   |  the user does a command entry.
	 */

	g_signal_connect(G_OBJECT(gport->drawing_area), "realize", G_CALLBACK(ghid_port_drawing_realize_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "expose_event", G_CALLBACK(ghid_drawing_area_expose_cb), port);
	g_signal_connect(G_OBJECT(gport->top_window), "configure_event", G_CALLBACK(top_window_configure_event_cb), port);
	g_signal_connect(gport->top_window, "enter-notify-event", G_CALLBACK(top_window_enter_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "configure_event",
									 G_CALLBACK(ghid_port_drawing_area_configure_event_cb), port);


	/* Mouse and key events will need to be intercepted when PCB needs a
	   |  location from the user.
	 */

	ghid_interface_input_signals_connect();

	g_signal_connect(G_OBJECT(gport->drawing_area), "scroll_event", G_CALLBACK(ghid_port_window_mouse_scroll_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "enter_notify_event", G_CALLBACK(ghid_port_window_enter_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "leave_notify_event", G_CALLBACK(ghid_port_window_leave_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "motion_notify_event", G_CALLBACK(ghid_port_window_motion_cb), port);



	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_chart_cb), port);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy_chart_cb), port);

	ghidgui->creating = FALSE;

	gtk_widget_show_all(gport->top_window);
	ghid_pack_mode_buttons();
	gdk_window_set_back_pixmap(gtk_widget_get_window(gport->drawing_area), NULL, FALSE);

	ghid_fullscreen_apply();
}


	/* Connect and disconnect just the signals a g_main_loop() will need.
	   |  Cursor and motion events still need to be handled by the top level
	   |  loop, so don't connect/reconnect these.
	   |  A g_main_loop will be running when PCB wants the user to select a
	   |  location or if command entry is needed in the status line hbox.
	   |  During these times normal button/key presses are intercepted, either
	   |  by new signal handlers or the command_combo_box entry.
	 */
static gulong button_press_handler, button_release_handler, key_press_handler, key_release_handler;

void ghid_interface_input_signals_connect(void)
{
	button_press_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "button_press_event", G_CALLBACK(ghid_port_button_press_cb), NULL);

	button_release_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "button_release_event", G_CALLBACK(ghid_port_button_release_cb), NULL);

	key_press_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "key_press_event", G_CALLBACK(ghid_port_key_press_cb), NULL);

	key_release_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "key_release_event", G_CALLBACK(ghid_port_key_release_cb), NULL);
}

void ghid_interface_input_signals_disconnect(void)
{
	if (button_press_handler)
		g_signal_handler_disconnect(gport->drawing_area, button_press_handler);

	if (button_release_handler)
		g_signal_handler_disconnect(gport->drawing_area, button_release_handler);

	if (key_press_handler)
		g_signal_handler_disconnect(gport->drawing_area, key_press_handler);

	if (key_release_handler)
		g_signal_handler_disconnect(gport->drawing_area, key_release_handler);

	button_press_handler = button_release_handler = 0;
	key_press_handler = key_release_handler = 0;

}


	/* We'll set the interface insensitive when a g_main_loop is running so the
	   |  Gtk menus and buttons don't respond and interfere with the special entry
	   |  the user needs to be doing.
	 */
void ghid_interface_set_sensitive(gboolean sensitive)
{
	gtk_widget_set_sensitive(ghidgui->left_toolbar, sensitive);
	gtk_widget_set_sensitive(ghidgui->menu_hbox, sensitive);
}


/* ----------------------------------------------------------------------
 * initializes icon pixmap and also cursor bit maps
 */
static void ghid_init_icons(GHidPort * port)
{
	GdkWindow *window = gtk_widget_get_window(gport->top_window);

	XC_clock_source = gdk_bitmap_create_from_data(window, (char *) rotateIcon_bits, rotateIcon_width, rotateIcon_height);
	XC_clock_mask = gdk_bitmap_create_from_data(window, (char *) rotateMask_bits, rotateMask_width, rotateMask_height);

	XC_hand_source = gdk_bitmap_create_from_data(window, (char *) handIcon_bits, handIcon_width, handIcon_height);
	XC_hand_mask = gdk_bitmap_create_from_data(window, (char *) handMask_bits, handMask_width, handMask_height);

	XC_lock_source = gdk_bitmap_create_from_data(window, (char *) lockIcon_bits, lockIcon_width, lockIcon_height);
	XC_lock_mask = gdk_bitmap_create_from_data(window, (char *) lockMask_bits, lockMask_width, lockMask_height);
}

void ghid_create_pcb_widgets(void)
{
	GHidPort *port = &ghid_port;
	GError *err = NULL;

	if (conf_hid_gtk.plugins.hid_gtk.bg_image)
		ghidgui->bg_pixbuf = gdk_pixbuf_new_from_file(conf_hid_gtk.plugins.hid_gtk.bg_image, &err);
	if (err) {
		g_error("%s", err->message);
		g_error_free(err);
	}
	ghid_build_pcb_top_window();
	ghid_install_accel_groups(GTK_WINDOW(port->top_window), ghidgui);
	ghid_update_toggle_flags();

	ghid_init_icons(port);
	SetMode(PCB_MODE_ARROW);
	ghid_mode_buttons_update();
}

static gboolean ghid_listener_cb(GIOChannel * source, GIOCondition condition, gpointer data)
{
	GIOStatus status;
	gchar *str;
	gsize len;
	gsize term;
	GError *err = NULL;


	if (condition & G_IO_HUP) {
		gui->log("Read end of pipe died!\n");
		return FALSE;
	}

	if (condition == G_IO_IN) {
		status = g_io_channel_read_line(source, &str, &len, &term, &err);
		switch (status) {
		case G_IO_STATUS_NORMAL:
			hid_parse_actions(str);
			g_free(str);
			break;

		case G_IO_STATUS_ERROR:
			gui->log("ERROR status from g_io_channel_read_line\n");
			return FALSE;
			break;

		case G_IO_STATUS_EOF:
			gui->log("Input pipe returned EOF.  The --listen option is \n" "probably not running anymore in this session.\n");
			return FALSE;
			break;

		case G_IO_STATUS_AGAIN:
			gui->log("AGAIN status from g_io_channel_read_line\n");
			return FALSE;
			break;

		default:
			fprintf(stderr, "ERROR:  unhandled case in ghid_listener_cb\n");
			return FALSE;
			break;
		}

	}
	else
		fprintf(stderr, "Unknown condition in ghid_listener_cb\n");

	return TRUE;
}

static void ghid_create_listener(void)
{
	GIOChannel *channel;
	int fd = fileno(stdin);

	channel = g_io_channel_unix_new(fd);
	g_io_add_watch(channel, G_IO_IN, ghid_listener_cb, NULL);
}


/* ------------------------------------------------------------ */
int ghid_usage(const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	conf_usage("plugins/hid_gtk", hid_usage_option);
	fprintf(stderr, "\nInvocation: pcb-rnd --gui gtk [options]\n");
	return 0;
}

	/* Create top level window for routines that will need top_window
	   |  before ghid_create_pcb_widgets() is called.
	 */
void ghid_parse_arguments(int *argc, char ***argv)
{
	GtkWidget *window;
	GdkPixbuf *icon;

	ghid_config_init();

	/* on windows we need to figure out the installation directory */
#ifdef WIN32
	char *tmps;
	char *libdir;
	tmps = g_win32_get_package_installation_directory(PACKAGE "-" VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE  G_DIR_SEPARATOR_S "pcblib"
	libdir = (char *) malloc(strlen(tmps) + strlen(REST_OF_PATH) + 1);
	sprintf(libdir, "%s%s", tmps, REST_OF_PATH);
	free(tmps);

#undef REST_OF_PATH

#endif

#if defined (DEBUG)
	{
		int i;
		for (i = 0; i < *argc; i++)
			printf("ghid_parse_arguments():  *argv[%d] = \"%s\"\n", i, (*argv)[i]);
	}
#endif

	/* Threads aren't used in PCB, but this call would go here.
	 */
	/* g_thread_init (NULL); */

#if defined (ENABLE_NLS)
	/* Do our own setlocale() stufff since we want to override LC_NUMERIC
	 */
	gtk_set_locale();
	setlocale(LC_NUMERIC, "C");		/* use decimal point instead of comma */
#endif

	conf_parse_arguments("plugins/hid_gtk/", argc, argv);

	/*
	 * Prevent gtk_init() and gtk_init_check() from automatically
	 * calling setlocale (LC_ALL, "") which would undo LC_NUMERIC if ENABLE_NLS
	 * We also don't want locale set if no ENABLE_NLS to keep "C" LC_NUMERIC.
	 */
	gtk_disable_setlocale();
	gtk_init(argc, argv);


	gport = &ghid_port;
	gport->view.coord_per_px = 300.0;
	pixel_slop = 300;

	ghid_init_renderer(argc, argv, gport);

#ifdef ENABLE_NLS
#ifdef LOCALEDIR
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	icon = gdk_pixbuf_new_from_xpm_data((const gchar **) icon_bits);
	gtk_window_set_default_icon(icon);

	window = gport->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	wplc_place(WPLC_TOP, window);

	gtk_widget_show_all(gport->top_window);
	ghidgui->creating = TRUE;
}

static unsigned short int ghid_translate_key(const char *desc, int len)
{
	guint key;

	if (strcasecmp(desc, "enter") == 0) desc = "Return";

	key = gdk_keyval_from_name(desc);
	if (key > 0xffff) {
		Message(PCB_MSG_DEFAULT, "Ignoring invalid/exotic key sym: '%s'\n", desc);
		return 0;
	}
	return key;
}

int ghid_key_name(unsigned short int key_char, char *out, int out_len)
{
	char *name = gdk_keyval_name(key_char);
	if (name == NULL)
		return -1;
	strncpy(out, name, out_len);
	out[out_len-1] = '\0';
	return 0;
}

void ghid_do_export(HID_Attr_Val * options)
{
	gtkhid_begin();

	hid_cfg_keys_init(&ghid_keymap);
	ghid_keymap.translate_key = ghid_translate_key;
	ghid_keymap.key_name = ghid_key_name;
	ghid_keymap.auto_chr = 1;
	ghid_keymap.auto_tr = hid_cfg_key_default_trans;

	ghid_create_pcb_widgets();

	/* These are needed to make sure the @layerpick and @layerview menus
	 * are properly initialized and synchronized with the current PCB.
	 */
	ghid_layer_buttons_update();
	ghid_main_menu_install_route_style_selector
		(GHID_MAIN_MENU(ghidgui->menu_bar), GHID_ROUTE_STYLE_SELECTOR(ghidgui->route_style_selector));

	if (conf_hid_gtk.plugins.hid_gtk.listen)
		ghid_create_listener();

	ghid_notify_gui_is_up();

	event(EVENT_GUI_INIT, NULL);

	gtk_main();
	hid_cfg_keys_uninit(&ghid_keymap);
	gtkhid_end();
}

void ghid_do_exit(HID *hid)
{
	gtk_main_quit();
}

void ghid_fullscreen_apply(void)
{
	if (conf_core.editor.fullscreen) {
		gtk_widget_hide(ghidgui->left_toolbar);
		gtk_widget_hide(ghidgui->top_hbox);
		gtk_widget_hide(ghidgui->status_line_hbox);
	}
	else {
		gtk_widget_show(ghidgui->left_toolbar);
		gtk_widget_show(ghidgui->top_hbox);
		gtk_widget_show(ghidgui->status_line_hbox);
	}
}

/*! \brief callback for */
static gboolean get_layer_visible_cb(int id)
{
	int visible;
	layer_process(NULL, NULL, &visible, id);
	return visible;
}

gint LayersChanged(int argc, const char **argv, Coord x, Coord y)
{
	if (!ghidgui || !ghidgui->menu_bar)
		return 0;

	ghid_config_groups_changed();
	ghid_layer_buttons_update();
	ghid_layer_selector_show_layers(GHID_LAYER_SELECTOR(ghidgui->layer_selector), get_layer_visible_cb);

	/* FIXME - if a layer is moved it should retain its color.  But layers
	   |  currently can't do that because color info is not saved in the
	   |  pcb file.  So this makes a moved layer change its color to reflect
	   |  the way it will be when the pcb is reloaded.
	 */
	pcb_colors_from_settings(PCB);
	return 0;
}

static const char toggleview_syntax[] =
	"ToggleView(1..MAXLAYER)\n" "ToggleView(layername)\n" "ToggleView(Silk|Rats|Pins|Vias|Mask|BackSide)";

static const char toggleview_help[] = "Toggle the visibility of the specified layer or layer group.";

/* %start-doc actions ToggleView

If you pass an integer, that layer is specified by index (the first
layer is @code{1}, etc).  If you pass a layer name, that layer is
specified by name.  When a layer is specified, the visibility of the
layer group containing that layer is toggled.

If you pass a special layer name, the visibility of those components
(silk, rats, etc) is toggled.  Note that if you have a layer named
the same as a special layer, the layer is chosen over the special layer.

%end-doc */

static int ToggleView(int argc, const char **argv, Coord x, Coord y)
{
	int i, l;

#ifdef DEBUG_MENUS
	puts("Starting ToggleView().");
#endif

	if (argc == 0) {
		AFAIL(toggleview);
	}
	if (isdigit((int) argv[0][0])) {
		l = atoi(argv[0]) - 1;
	}
	else if (strcmp(argv[0], "Silk") == 0)
		l = LAYER_BUTTON_SILK;
	else if (strcmp(argv[0], "Rats") == 0)
		l = LAYER_BUTTON_RATS;
	else if (strcmp(argv[0], "Pins") == 0)
		l = LAYER_BUTTON_PINS;
	else if (strcmp(argv[0], "Vias") == 0)
		l = LAYER_BUTTON_VIAS;
	else if (strcmp(argv[0], "Mask") == 0)
		l = LAYER_BUTTON_MASK;
	else if (strcmp(argv[0], "BackSide") == 0)
		l = LAYER_BUTTON_FARSIDE;
	else {
		l = -1;
		for (i = 0; i < max_copper_layer + 2; i++)
			if (strcmp(argv[0], PCB->Data->Layer[i].Name) == 0) {
				l = i;
				break;
			}
		if (l == -1) {
			AFAIL(toggleview);
		}

	}

	/* Now that we've figured out which toggle button ought to control
	 * this layer, simply hit the button and let the pre-existing code deal
	 */
	ghid_layer_selector_toggle_layer(GHID_LAYER_SELECTOR(ghidgui->layer_selector), l);
	return 0;
}

static const char selectlayer_syntax[] = "SelectLayer(1..MAXLAYER|Silk|Rats)";

static const char selectlayer_help[] = "Select which layer is the current layer.";

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

static int SelectLayer(int argc, const char **argv, Coord x, Coord y)
{
	int newl;
	if (argc == 0)
		AFAIL(selectlayer);

	if (strcasecmp(argv[0], "silk") == 0)
		newl = LAYER_BUTTON_SILK;
	else if (strcasecmp(argv[0], "rats") == 0)
		newl = LAYER_BUTTON_RATS;
	else
		newl = atoi(argv[0]) - 1;

#ifdef DEBUG_MENUS
	printf("SelectLayer():  newl = %d\n", newl);
#endif

	/* Now that we've figured out which radio button ought to select
	 * this layer, simply hit the button and let the pre-existing code deal
	 */
	ghid_layer_selector_select_layer(GHID_LAYER_SELECTOR(ghidgui->layer_selector), newl);

	return 0;
}


HID_Action gtk_topwindow_action_list[] = {
	{"LayersChanged", 0, LayersChanged,
	 layerschanged_help, layerschanged_syntax}
	,
	{"SelectLayer", 0, SelectLayer,
	 selectlayer_help, selectlayer_syntax}
	,
	{"ToggleView", 0, ToggleView,
	 toggleview_help, toggleview_syntax}
};

REGISTER_ACTIONS(gtk_topwindow_action_list, ghid_cookie)

static GtkWidget *ghid_load_menus(void)
{
	const lht_node_t *mr;
	GtkWidget *menu_bar = NULL;
	extern const char *hid_gtk_menu_default;

	ghid_cfg = hid_cfg_load("gtk", 0, hid_gtk_menu_default);
	if (ghid_cfg == NULL) {
		Message(PCB_MSG_DEFAULT, "FATAL: can't load the gtk menu res either from file or from hardwired default.");
		abort();
	}

	mr = hid_cfg_get_menu(ghid_cfg, "/main_menu");
	if (mr != NULL) {
		menu_bar = ghid_main_menu_new(G_CALLBACK(ghid_menu_cb));
		ghid_main_menu_add_node(GHID_MAIN_MENU(menu_bar), mr);
	}

	mr = hid_cfg_get_menu(ghid_cfg, "/popups");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for(n = mr->data.list.first; n != NULL; n = n->next)
				ghid_main_menu_add_popup_node(GHID_MAIN_MENU(menu_bar), n);
		}
		else
			hid_cfg_error(mr, "/popups should be a list");
	}

#ifdef DEBUG_MENUS
	puts("Finished loading menus.");
#endif

	mr = hid_cfg_get_menu(ghid_cfg, "/mouse");
	if (hid_cfg_mouse_init(ghid_cfg, &ghid_mouse) != 0)
		Message(PCB_MSG_DEFAULT, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	return menu_bar;
}

/* ------------------------------------------------------------ */

static const char adjuststyle_syntax[] = "AdjustStyle()\n";

static const char adjuststyle_help[] = "Open the window which allows editing of the route styles.";

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int AdjustStyle(int argc, const char **argv, Coord x, Coord y)
{
	if (argc > 1)
		AFAIL(adjuststyle);

	ghid_route_style_selector_edit_dialog(GHID_ROUTE_STYLE_SELECTOR(ghidgui->route_style_selector));
	return 0;
}

/* ------------------------------------------------------------ */

static const char editlayergroups_syntax[] = "EditLayerGroups()\n";

static const char editlayergroups_help[] = "Open the preferences window which allows editing of the layer groups.";

/* %start-doc actions EditLayerGroups

Opens the preferences window which is where the layer groups
are edited.  This action is primarily provides to provide menu
lht compatibility with the lesstif HID.

%end-doc */

static int EditLayerGroups(int argc, const char **argv, Coord x, Coord y)
{

	if (argc != 0)
		AFAIL(editlayergroups);

	hid_actionl("DoWindows", "Preferences", NULL);

	return 0;
}

/* ------------------------------------------------------------ */

HID_Action ghid_menu_action_list[] = {
	{"AdjustStyle", 0, AdjustStyle, adjuststyle_help, adjuststyle_syntax}
	,
	{"EditLayerGroups", 0, EditLayerGroups, editlayergroups_help, editlayergroups_syntax}
};

REGISTER_ACTIONS(ghid_menu_action_list, ghid_cookie)
