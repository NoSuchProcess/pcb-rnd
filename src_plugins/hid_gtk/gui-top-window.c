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
#define _POSIX_SOURCE
#include "config.h"
#include "gui-top-window.h"
#include "conf_core.h"
#include <locale.h>
#include "data.h"
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
#include "search.h"
#include "select.h"
#include "undo.h"
#include "event.h"
#include "free_atexit.h"
#include "paths.h"
#include "hid_attrib.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "route_style.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "obj_line.h"
#include "layer_vis.h"
#include "gtkhid-main.h"

#include "../src_plugins/lib_gtk_common/bu_box.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/bu_layer_selector.h"
#include "../src_plugins/lib_gtk_common/bu_menu.h"
#include "../src_plugins/lib_gtk_common/bu_icons.h"
#include "../src_plugins/lib_gtk_common/bu_info_bar.h"
#include "../src_plugins/lib_gtk_common/dlg_route_style.h"
#include "../src_plugins/lib_gtk_common/dlg_fontsel.h"
#include "../src_plugins/lib_gtk_common/util_str.h"
#include "../src_plugins/lib_gtk_common/util_listener.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_common/wt_layer_selector.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#warning TODO: remove these:
#include "gui.h"
GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

/*! \brief sync the menu checkboxes with actual pcb state */
void ghid_update_toggle_flags(pcb_gtk_topwin_t *tw)
{
	ghid_main_menu_update_toggle_state(GHID_MAIN_MENU(tw->menu.menu_bar), menu_toggle_update_cb);
}

static void h_adjustment_changed_cb(GtkAdjustment *adj, pcb_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	ghid_port_ranges_changed(tw);
}

static void v_adjustment_changed_cb(GtkAdjustment *adj, pcb_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	ghid_port_ranges_changed(tw);
}

	/* Save size of top window changes so PCB can restart at its size at exit.
	 */
static gint top_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, GHidPort * port)
{
	wplc_config_event(widget, &hid_gtk_wgeo.top_x, &hid_gtk_wgeo.top_y, &hid_gtk_wgeo.top_width, &hid_gtk_wgeo.top_height);

	return FALSE;
}

static gboolean top_window_enter_cb(GtkWidget * widget, GdkEvent * event, pcb_gtk_topwin_t *tw)
{
	if (check_externally_modified(&tw->ext_chg))
		pcb_gtk_info_bar_file_extmod_prompt(&tw->ibar, tw->vbox_middle);

	return FALSE;
}


void ghid_handle_units_changed(pcb_gtk_topwin_t *tw)
{
	char *text = pcb_strdup_printf("<b>%s</b>",
																 conf_core.editor.grid_unit->in_suffix);
	ghid_set_cursor_position_labels(&tw->cps, conf_hid_gtk.plugins.hid_gtk.compact_vertical);
	gtk_label_set_markup(GTK_LABEL(tw->cps.grid_units_label), text);
	free(text);
	ghid_config_handle_units_changed(gport);
}


	/* Sync toggle states that were saved with the layout and notify the
	   |  config code to update Settings values it manages.
	 */
void ghid_sync_with_new_layout(pcb_gtk_topwin_t *tw)
{
	if (vtroutestyle_len(&PCB->RouteStyle) > 0) {
		pcb_use_route_style(&PCB->RouteStyle.array[0]);
		pcb_gtk_route_style_select_style(GHID_ROUTE_STYLE(tw->route_style_selector), &PCB->RouteStyle.array[0]);
	}

	ghid_handle_units_changed(tw);

	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	pcb_gtk_close_info_bar(&tw->ibar);
	update_board_mtime_from_disk(&tw->ext_chg);
}

void pcb_gtk_tw_notify_save_pcb(pcb_gtk_topwin_t *tw, const char *filename, pcb_bool done)
{
	/* Do nothing if it is not the active PCB file that is being saved.
	 */
	if (PCB->Filename == NULL || strcmp(filename, PCB->Filename) != 0)
		return;

	if (done)
		update_board_mtime_from_disk(&tw->ext_chg);
}

void pcb_gtk_tw_notify_filename_changed(pcb_gtk_topwin_t *tw)
{
	/* Pick up the mtime of the new PCB file */
	update_board_mtime_from_disk(&tw->ext_chg);
	ghid_window_set_name_label(PCB->Name);
}

/*! \brief Install menu bar and accelerator groups */
void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_add_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
	gtk_window_add_accel_group(window, pcb_gtk_layer_selector_get_accel_group(GHID_LAYER_SELECTOR(tw->layer_selector)));
	gtk_window_add_accel_group(window, pcb_gtk_route_style_get_accel_group(GHID_ROUTE_STYLE(tw->route_style_selector)));
}

/*! \brief Remove menu bar and accelerator groups */
void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_remove_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
	gtk_window_remove_accel_group(window, pcb_gtk_layer_selector_get_accel_group(GHID_LAYER_SELECTOR(tw->layer_selector)));
	gtk_window_remove_accel_group(window, pcb_gtk_route_style_get_accel_group(GHID_ROUTE_STYLE(tw->route_style_selector)));
}

/* Refreshes the window title bar and sets the PCB name to the
 * window title bar or to a seperate label
 */
void pcb_gtk_tw_window_set_name_label(pcb_gtk_topwin_t *tw, gchar *name)
{
	gchar *str;
	gchar *filename;

	/* FIXME -- should this happen?  It does... */
	/* This happens if we're calling an exporter from the command line */
	if (ghidgui == NULL)
		return;

#warning TODO: use some gds here to speed things up

	pcb_gtk_g_strdup(&(tw->name_label_string), name);
	if (!tw->name_label_string || !*tw->name_label_string)
		tw->name_label_string = g_strdup(_("Unnamed"));

	if (!PCB->Filename || !*PCB->Filename)
		filename = g_strdup(_("<board with no file name or format>"));
	else
		filename = g_strdup(PCB->Filename);

	str = g_strdup_printf("%s%s (%s) - pcb-rnd", PCB->Changed ? "*" : "", tw->name_label_string, filename);
	gtk_window_set_title(GTK_WINDOW(gport->top_window), str);
	g_free(str);
	g_free(filename);
}


/*! \brief callback for pcb_gtk_layer_selector_update_colors */
const gchar *get_layer_color(gint layer)
{
	const gchar *rv;
	layer_process(&rv, NULL, NULL, layer);
	return rv;
}

/*! \brief Update a layer selector's color scheme */
void pcb_gtk_tw_layer_buttons_color_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layer_selector_update_colors(GHID_LAYER_SELECTOR(tw->layer_selector), get_layer_color);
	pcb_colors_from_settings(PCB);
}

void ghid_layer_buttons_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layer_buttons_update(tw->layer_selector, GHID_MAIN_MENU(tw->menu.menu_bar));
}

/*! \brief Called when user clicks OK on route style dialog */
void pcb_gtk_tw_route_styles_edited_cb(pcb_gtk_topwin_t *tw)
{
	ghid_main_menu_install_route_style_selector(GHID_MAIN_MENU(tw->menu.menu_bar), GHID_ROUTE_STYLE(tw->route_style_selector));
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

	pcb_hid_action("Quit");

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

static void get_widget_styles(pcb_gtk_topwin_t *tw, GtkStyle ** menu_bar_style, GtkStyle ** tool_button_style, GtkStyle ** tool_button_label_style)
{
	GtkWidget *tool_button;
	GtkWidget *tool_button_label;
	GtkToolItem *tool_item;

	/* Build a tool item to extract the theme's styling for a toolbar button with text */
	tool_item = gtk_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tw->mode_btn.mode_toolbar), tool_item, 0);
	tool_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(tool_item), tool_button);
	tool_button_label = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(tool_button), tool_button_label);

	/* Grab the various styles we need */
	gtk_widget_ensure_style(tw->menu.menu_bar);
	*menu_bar_style = gtk_widget_get_style(tw->menu.menu_bar);

	gtk_widget_ensure_style(tool_button);
	*tool_button_style = gtk_widget_get_style(tool_button);

	gtk_widget_ensure_style(tool_button_label);
	*tool_button_label_style = gtk_widget_get_style(tool_button_label);

	gtk_widget_destroy(GTK_WIDGET(tool_item));
}

static void do_fix_topbar_theming(pcb_gtk_topwin_t *tw)
{
	GtkWidget *rel_pos_frame;
	GtkWidget *abs_pos_frame;
	GtkStyle *menu_bar_style;
	GtkStyle *tool_button_style;
	GtkStyle *tool_button_label_style;

	get_widget_styles(tw, &menu_bar_style, &tool_button_style, &tool_button_label_style);

	/* Style the top bar background as if it were all a menu bar */
	gtk_widget_set_style(tw->top_bar_background, menu_bar_style);

	/* Style the cursor position labels using the menu bar style as well.
	 * If this turns out to cause problems with certain gtk themes, we may
	 * need to grab the GtkStyle associated with an actual menu item to
	 * get a text color to render with.
	 */
	gtk_widget_set_style(tw->cps.cursor_position_relative_label, menu_bar_style);
	gtk_widget_set_style(tw->cps.cursor_position_absolute_label, menu_bar_style);

	/* Style the units button as if it were a toolbar button - hopefully
	 * this isn't too ugly sitting on a background themed as a menu bar.
	 * It is unlikely any theme defines colours for a GtkButton sitting on
	 * a menu bar.
	 */
	rel_pos_frame = gtk_widget_get_parent(tw->cps.cursor_position_relative_label);
	abs_pos_frame = gtk_widget_get_parent(tw->cps.cursor_position_absolute_label);
	gtk_widget_set_style(rel_pos_frame, menu_bar_style);
	gtk_widget_set_style(abs_pos_frame, menu_bar_style);
	gtk_widget_set_style(tw->cps.grid_units_button, tool_button_style);
	gtk_widget_set_style(tw->cps.grid_units_label, tool_button_label_style);
}

/* Attempt to produce a conststent style for our extra menu-bar items by
 * copying aspects from the menu bar style set by the user's GTK theme.
 * Setup signal handlers to update our efforts if the user changes their
 * theme whilst we are running.
 */
static void fix_topbar_theming(pcb_gtk_topwin_t *tw)
{
	GtkSettings *settings;

	do_fix_topbar_theming(tw);

	settings = gtk_widget_get_settings(tw->top_bar_background);
	g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(do_fix_topbar_theming), NULL);
	g_signal_connect(settings, "notify::gtk-font-name", G_CALLBACK(do_fix_topbar_theming), NULL);
}

static void fullscreen_cb(GtkButton * btn, void *data)
{
	conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !conf_core.editor.fullscreen, POL_OVERWRITE);
}

/*
 * Create the top_window contents.  The config settings should be loaded
 * before this is called.
 */
static void ghid_build_pcb_top_window(pcb_gtk_topwin_t *tw, GtkWidget *in_top_window)
{
	GtkWidget *window;
	GtkWidget *vbox_main, *hbox_middle, *hbox;
	GtkWidget *vbox, *frame, *hbox_scroll, *fullscreen_btn;
	GtkWidget *label;
	GHidPort *port = &ghid_port;
	GtkWidget *scrolled;

	window = in_top_window;

	vbox_main = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox_main);

	/* -- Top control bar */
	tw->top_bar_background = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(vbox_main), tw->top_bar_background, FALSE, FALSE, 0);

	tw->top_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tw->top_bar_background), tw->top_hbox);

	/*
	 * menu_hbox will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	tw->menu_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->top_hbox), tw->menu_hbox, FALSE, FALSE, 0);

	tw->menubar_toolbar_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->menu_hbox), tw->menubar_toolbar_vbox, FALSE, FALSE, 0);

	/* Build layer menus */
	tw->layer_selector = pcb_gtk_layer_selector_new();
	make_layer_buttons(tw->layer_selector);
	make_virtual_layer_buttons(tw->layer_selector);
	g_signal_connect(G_OBJECT(tw->layer_selector), "select_layer", G_CALLBACK(layer_selector_select_callback), &ghidgui->common);
	g_signal_connect(G_OBJECT(tw->layer_selector), "toggle_layer", G_CALLBACK(layer_selector_toggle_callback), &ghidgui->common);
	/* Build main menu */
	tw->menu.menu_bar = ghid_load_menus(&tw->menu, &tw->ghid_cfg);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->menu.menu_bar, FALSE, FALSE, 0);

	pcb_gtk_make_mode_buttons_and_toolbar(&ghidgui->common, &tw->mode_btn);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->mode_btn.mode_toolbar_vbox, FALSE, FALSE, 0);

	tw->position_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(tw->top_hbox), tw->position_hbox, FALSE, FALSE, 0);

	make_cursor_position_labels(tw->position_hbox, &tw->cps);

	hbox_middle = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_middle, TRUE, TRUE, 0);

	fix_topbar_theming(tw); /* Must be called after toolbar is created */

	/* -- Left control bar */
	/*
	 * This box will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	tw->left_toolbar = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->left_toolbar, FALSE, FALSE, 0);

	vbox = ghid_scrolled_vbox(tw->left_toolbar, &scrolled, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), tw->layer_selector, FALSE, FALSE, 0);

	/* tw->mode_btn.mode_buttons_frame was created above in the call to
	 * make_mode_buttons_and_toolbar (...);
	 */
	gtk_box_pack_start(GTK_BOX(tw->left_toolbar), tw->mode_btn.mode_buttons_frame, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(tw->left_toolbar), frame, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
	tw->route_style_selector = pcb_gtk_route_style_new(&ghidgui->common);
	make_route_style_buttons(GHID_ROUTE_STYLE(tw->route_style_selector));
	gtk_box_pack_start(GTK_BOX(hbox), tw->route_style_selector, FALSE, FALSE, 0);

	tw->vbox_middle = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->vbox_middle, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hbox, TRUE, TRUE, 0);

	/* -- The PCB layout output drawing area */

	gport->drawing_area = gtk_drawing_area_new();
	ghid_init_drawing_widget(gport->drawing_area, gport);

	gport->mouse.drawing_area = gport->drawing_area;
	gport->mouse.top_window = gport->top_window;

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

	tw->v_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);
	tw->v_range = gtk_vscrollbar_new(GTK_ADJUSTMENT(tw->v_adjustment));

	gtk_box_pack_start(GTK_BOX(hbox), tw->v_range, FALSE, FALSE, 0);



	g_signal_connect(G_OBJECT(tw->v_adjustment), "value_changed", G_CALLBACK(v_adjustment_changed_cb), tw);

	tw->h_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0);

	hbox_scroll = gtk_hbox_new(FALSE, 0);
	tw->h_range = gtk_hscrollbar_new(GTK_ADJUSTMENT(tw->h_adjustment));
	fullscreen_btn = gtk_button_new_with_label("FS");
	g_signal_connect(GTK_OBJECT(fullscreen_btn), "clicked", G_CALLBACK(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), tw->h_range, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), fullscreen_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hbox_scroll, FALSE, FALSE, 0);


	g_signal_connect(G_OBJECT(tw->h_adjustment), "value_changed", G_CALLBACK(h_adjustment_changed_cb), tw);

	/* -- The bottom status line label */
	tw->status_line_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), tw->status_line_hbox, FALSE, FALSE, 0);

	label = pcb_gtk_build_status_line_label();

	tw->status_line_label = label;
	gtk_box_pack_start(GTK_BOX(tw->status_line_hbox), label, FALSE, FALSE, 0);

	/* Depending on user setting, the command_combo_box may get packed into
	   |  the status_line_hbox, but it will happen on demand the first time
	   |  the user does a command entry.
	 */

	g_signal_connect(G_OBJECT(gport->drawing_area), "realize", G_CALLBACK(ghid_port_drawing_realize_cb), port);
	g_signal_connect(G_OBJECT(gport->drawing_area), "expose_event", G_CALLBACK(ghid_drawing_area_expose_cb), port);
	g_signal_connect(G_OBJECT(gport->top_window), "configure_event", G_CALLBACK(top_window_configure_event_cb), port);
	g_signal_connect(gport->top_window, "enter-notify-event", G_CALLBACK(top_window_enter_cb), tw);
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

	ghidgui->topwin.creating = FALSE;

	gtk_widget_show_all(gport->top_window);
	ghid_pack_mode_buttons();
	gdk_window_set_back_pixmap(gtk_widget_get_window(gport->drawing_area), NULL, FALSE);

	ghid_fullscreen_apply(tw);
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
	button_press_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_press_event", G_CALLBACK(ghid_port_button_press_cb), &gport->mouse);
	button_release_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_release_event", G_CALLBACK(ghid_port_button_release_cb), &gport->mouse);

	key_press_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "key_press_event", G_CALLBACK(ghid_port_key_press_cb), &ghid_port.view);

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
void pcb_gtk_tw_interface_set_sensitive(pcb_gtk_topwin_t *tw, gboolean sensitive)
{
	gtk_widget_set_sensitive(tw->left_toolbar, sensitive);
	gtk_widget_set_sensitive(tw->menu_hbox, sensitive);
}

void ghid_create_pcb_widgets(pcb_gtk_topwin_t *tw, GtkWidget *in_top_window)
{
	GHidPort *port = &ghid_port;
	GError *err = NULL;

	if (conf_hid_gtk.plugins.hid_gtk.bg_image)
		ghidgui->bg_pixbuf = gdk_pixbuf_new_from_file(conf_hid_gtk.plugins.hid_gtk.bg_image, &err);
	if (err) {
		g_error("%s", err->message);
		g_error_free(err);
	}
	ghid_build_pcb_top_window(tw, in_top_window);
	ghid_install_accel_groups(GTK_WINDOW(port->top_window), tw);
	ghid_update_toggle_flags(tw);

	pcb_gtk_icons_init(gtk_widget_get_window(port->top_window));
	pcb_crosshair_set_mode(PCB_MODE_ARROW);
	ghid_mode_buttons_update();
}

/* ------------------------------------------------------------ */
int ghid_usage(const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	conf_usage("plugins/hid_gtk", pcb_hid_usage_option);
	fprintf(stderr, "\nInvocation: pcb-rnd --gui gtk [options]\n");
	return 0;
}

	/* Create top level window for routines that will need top_window
	   |  before ghid_create_pcb_widgets() is called.
	 */
void ghid_parse_arguments(int *argc, char ***argv)
{
	GtkWidget *window;

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
	pcb_pixel_slop = 300;

	ghid_init_renderer(argc, argv, gport);

#ifdef ENABLE_NLS
#ifdef LOCALEDIR
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain(PACKAGE);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	ghidgui->common.top_window = window = gport->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	wplc_place(WPLC_TOP, window);

	gtk_widget_show_all(gport->top_window);
	ghidgui->topwin.creating = TRUE;
}

void ghid_fullscreen_apply(pcb_gtk_topwin_t *tw)
{
	if (conf_core.editor.fullscreen) {
		gtk_widget_hide(tw->left_toolbar);
		gtk_widget_hide(tw->top_hbox);
		gtk_widget_hide(tw->status_line_hbox);
	}
	else {
		gtk_widget_show(tw->left_toolbar);
		gtk_widget_show(tw->top_hbox);
		gtk_widget_show(tw->status_line_hbox);
	}
}

/*! \brief callback for */
static gboolean get_layer_visible_cb(int id)
{
	int visible;
	layer_process(NULL, NULL, &visible, id);
	return visible;
}

void ghid_LayersChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui || !ghidgui->topwin.menu.menu_bar || PCB == NULL)
		return;

	ghid_layer_buttons_update(&ghidgui->topwin);
	pcb_gtk_layer_selector_show_layers(GHID_LAYER_SELECTOR(ghidgui->topwin.layer_selector), get_layer_visible_cb);

	/* FIXME - if a layer is moved it should retain its color.  But layers
	   |  currently can't do that because color info is not saved in the
	   |  pcb file.  So this makes a moved layer change its color to reflect
	   |  the way it will be when the pcb is reloaded.
	 */
	pcb_colors_from_settings(PCB);
	return;
}

int SelectLayer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_SelectLayer(ghidgui->topwin.layer_selector, argc, argv, x, y);
}

int ToggleView(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_ToggleView(ghidgui->topwin.layer_selector, argc, argv, x, y);
}

pcb_hid_action_t gtk_topwindow_action_list[] = {
	{"SelectLayer", 0, SelectLayer,
	 selectlayer_help, selectlayer_syntax}
	,
	{"ToggleView", 0, ToggleView,
	 toggleview_help, toggleview_syntax}
};

PCB_REGISTER_ACTIONS(gtk_topwindow_action_list, ghid_cookie)

/* ------------------------------------------------------------ */

static const char adjuststyle_syntax[] = "AdjustStyle()\n";

static const char adjuststyle_help[] = "Open the window which allows editing of the route styles.";

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int AdjustStyle(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc > 1)
		PCB_AFAIL(adjuststyle);

  pcb_gtk_route_style_edit_dialog(&ghidgui->common, GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
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

static int EditLayerGroups(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{

	if (argc != 0)
		PCB_AFAIL(editlayergroups);

#warning TODO: extend the DoWindows action so it opens the right preferences tab

	pcb_hid_actionl("DoWindows", "Preferences", NULL);

	return 0;
}


static const char pcb_acts_fontsel[] = "FontSel()\n";
static const char pcb_acth_fontsel[] = "Select the font to draw new text with.";
static int pcb_act_fontsel(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc > 1)
		PCB_ACT_FAIL(fontsel);

	if (argc > 0) {
		if (pcb_strcasecmp(argv[0], "Object") == 0) {
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_gui->get_coords(_("Select an Object"), &x, &y);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
/*				pcb_undo_save_serial();*/
				pcb_gtk_dlg_fontsel(&ghidgui->common, ptr1, ptr2, type, 1);
			}
		}
		else
			PCB_ACT_FAIL(fontsel);
	}
	else
		pcb_gtk_dlg_fontsel(&ghidgui->common, NULL, NULL, 0, 0);
	return 0;
}

/* ------------------------------------------------------------ */

pcb_hid_action_t ghid_menu_action_list[] = {
	{"AdjustStyle", 0, AdjustStyle, adjuststyle_help, adjuststyle_syntax}
	,
	{"EditLayerGroups", 0, EditLayerGroups, editlayergroups_help, editlayergroups_syntax}
	,
	{"fontsel", 0, pcb_act_fontsel, pcb_acth_fontsel, pcb_acts_fontsel}
};

PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_cookie)
