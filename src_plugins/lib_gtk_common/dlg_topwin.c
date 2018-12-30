/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "dlg_topwin.h"
#include "conf_core.h"

#include "board.h"
#include "crosshair.h"
#include "pcb-printf.h"
#include "actions.h"
#include "compat_nls.h"
#include "compat_misc.h"

#include "compat.h"
#include "bu_box.h"
#include "bu_status_line.h"
#include "bu_menu.h"
#include "bu_icons.h"
#include "bu_info_bar.h"
#include "dlg_route_style.h"
#include "util_str.h"
#include "util_listener.h"
#include "in_mouse.h"
#include "in_keyboard.h"
#include "wt_layersel.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "win_place.h"

/* sync the menu checkboxes with actual pcb state */
void ghid_update_toggle_flags(pcb_gtk_topwin_t *tw, const char *cookie)
{
	ghid_main_menu_update_toggle_state(GHID_MAIN_MENU(tw->menu.menu_bar), menu_toggle_update_cb);
}

static void h_adjustment_changed_cb(GtkAdjustment *adj, pcb_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	tw->com->port_ranges_changed();
}

static void v_adjustment_changed_cb(GtkAdjustment *adj, pcb_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	tw->com->port_ranges_changed();
}

	/* Save size of top window changes so PCB can restart at its size at exit.
	 */
static gint top_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, void *gport)
{
	pcb_event(PCB_EVENT_DAD_NEW_GEO, "psiiii", NULL, "top",
		(int)ev->x, (int)ev->y, (int)ev->width, (int)ev->height);

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
	char *text = pcb_strdup_printf("<b>%s</b>", conf_core.editor.grid_unit->in_suffix);
	ghid_set_cursor_position_labels(&tw->cps, conf_core.appearance.compact);
	gtk_label_set_markup(GTK_LABEL(tw->cps.grid_units_label), text);
	free(text);
}

gboolean ghid_idle_cb(void *topwin)
{
	pcb_gtk_topwin_t *tw = topwin;
	if (conf_core.editor.mode == PCB_MODE_NO)
		pcb_tool_select_by_id(PCB_MODE_ARROW);
	tw->com->mode_cursor_main(conf_core.editor.mode);
	if (tw->mode_btn.settings_mode != conf_core.editor.mode) {
		ghid_mode_buttons_update();
	}
	tw->mode_btn.settings_mode = conf_core.editor.mode;
	return FALSE;
}

gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, pcb_gtk_topwin_t *tw)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		tw->com->note_event_location(NULL);

	pcb_tool_adjust_attached_objects();
	tw->com->invalidate_all();
	g_idle_add(ghid_idle_cb, tw);
	return FALSE;
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

	tw->com->window_set_name_label(PCB->Name);
	tw->com->set_status_line_label();
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
	tw->com->window_set_name_label(PCB->Name);
}

void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_add_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
	gtk_window_add_accel_group(window, pcb_gtk_route_style_get_accel_group(GHID_ROUTE_STYLE(tw->route_style_selector)));
}

void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_remove_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
	gtk_window_remove_accel_group(window, pcb_gtk_route_style_get_accel_group(GHID_ROUTE_STYLE(tw->route_style_selector)));
}

/* Refreshes the window title bar and sets the PCB name to the
 * window title bar or to a seperate label
 */
void pcb_gtk_tw_window_set_name_label(pcb_gtk_topwin_t *tw, gchar *name)
{
	gchar *str;
	gchar *filename;

	/* This happens if we're calling an exporter from the command line */
	if (!tw->active)
		return;

TODO(": use some gds here to speed things up")

	pcb_gtk_g_strdup(&(tw->name_label_string), name);
	if (!tw->name_label_string || !*tw->name_label_string)
		tw->name_label_string = g_strdup(_("Unnamed"));

	if (!PCB->Filename || !*PCB->Filename)
		filename = g_strdup(_("<board with no file name or format>"));
	else
		filename = g_strdup(PCB->Filename);

	str = g_strdup_printf("%s%s (%s) - %s - pcb-rnd", PCB->Changed ? "*" : "", tw->name_label_string, filename, PCB->is_footprint ? "footprint" : "board");
	gtk_window_set_title(GTK_WINDOW(tw->com->top_window), str);
	g_free(str);
	g_free(filename);
}

void pcb_gtk_tw_layer_buttons_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layersel_update(tw->com, &tw->layersel);
}

void pcb_gtk_tw_layer_vis_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layersel_vis_update(&tw->layersel);
}

/* Called when user clicks OK on route style dialog */
void pcb_gtk_tw_route_styles_edited_cb(pcb_gtk_topwin_t *tw)
{
TODO(": generate a route styles changed event")
}

/*
 * ---------------------------------------------------------------
 * Top window
 * ---------------------------------------------------------------
 */
static gint delete_chart_cb(GtkWidget *widget, GdkEvent *event, void *data)
{
	pcb_action("Quit");

	/*
	 * Return TRUE to keep our app running.  A FALSE here would let the
	 * delete signal continue on and kill our program.
	 */
	return TRUE;
}

static void destroy_chart_cb(GtkWidget * widget, pcb_gtk_topwin_t *tw)
{
	tw->com->main_destroy(tw->com->gport);
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

/* XPM */
static const char * FullScreen_xpm[] = {
"8 8 2 1",
" 	c None",
".	c #729FCF",
"        ",
" ...... ",
" .      ",
" . ...  ",
" . .    ",
" . .    ",
" .      ",
"        "};

/* XPM */
static const char * resize_grip_xpm[] = {
"17 17 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #9E9A91",
"                .",
"               .+",
"              .++",
"             .++ ",
"            .++  ",
"           .++  .",
"          .++  .+",
"         .++  .++",
"        .++  .++ ",
"       .++  .++  ",
"      .++  .++  .",
"     .++  .++  .+",
"    .++  .++  .++",
"   .++  .++  .++ ",
"  .++  .++  .++  ",
" .++  .++  .++   ",
".++  .++  .++    "};

/* Embed an XPM image in a button, and make it display as small as possible 
 *   Returns: a new image button. When freeing the object, the image needs to be freed 
 *            as well, using :
 *            g_object_unref(gtk_button_get_image(GTK_BUTTON(button)); g_object_unref(button);
 */
static GtkWidget *create_image_button_from_xpm_data(const char **xpm_data)
{
	GtkWidget *button;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	const char *css_class = "minimum_size_button";
	char *css_descr;

	button = gtk_button_new();
	pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_data);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	gtk_button_set_image(GTK_BUTTON(button), image);

	css_descr = pcb_strdup_printf(".%s {min-width:0; min-height:0;}\n", css_class);
	gtkc_widget_add_class_style(button, css_class, css_descr);
	free(css_descr);

	return button;
}

/* Just calls the scroll bars preferred size, when drawing area is resized */
static void drawing_area_size_allocate_cb(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	gint min;
	pcb_gtk_topwin_t *tw = user_data;

	gtkc_widget_get_preferred_height(GTK_WIDGET(tw->v_range), &min, NULL);
	gtkc_widget_get_preferred_height(GTK_WIDGET(gtk_widget_get_parent(tw->h_range)), &min, NULL);
	gtkc_widget_get_preferred_height(GTK_WIDGET(tw->status_line_hbox), &min, NULL);
}

static gboolean drawing_area_enter_cb(GtkWidget *w, pcb_gtk_expose_t *p, void *user_data)
{
	pcb_gtk_topwin_t *tw = user_data;
	tw->com->invalidate_all();
	return FALSE;
}

static gboolean resize_grip_button_press(GtkWidget *area, GdkEventButton *event, gpointer user_data)
{
	if (event->type != GDK_BUTTON_PRESS)
		return TRUE;

	switch (event->button) {
		case 1:
			gtk_window_begin_resize_drag(GTK_WINDOW(gtk_widget_get_toplevel(area)),
				GDK_WINDOW_EDGE_SOUTH_EAST,
				event->button, event->x_root, event->y_root, event->time);
			break;

		case 2:
			gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(area)),
					event->button, event->x_root, event->y_root, event->time);
			break;
	}

	return TRUE;
}

/*
 * Create the top_window contents.  The config settings should be loaded
 * before this is called.
 */
static void ghid_build_pcb_top_window(pcb_gtk_topwin_t *tw)
{
	GtkWidget *vbox_main, *hbox_middle, *hbox;
	GtkWidget *vbox, *frame, *hbox_scroll, *fullscreen_btn;
	GtkWidget *label;
	GtkWidget *resize_grip_vbox;
	GtkWidget *resize_grip;
	GdkPixbuf *resize_grip_pixbuf;
	GtkWidget *resize_grip_image;

	vbox_main = gtkc_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tw->com->top_window), vbox_main);

	/* -- Top control bar */
	tw->top_bar_background = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(vbox_main), tw->top_bar_background, FALSE, FALSE, 0);

	tw->top_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tw->top_bar_background), tw->top_hbox);

	/*
	 * menu_hbox will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	tw->menu_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->top_hbox), tw->menu_hbox, FALSE, FALSE, 0);

	tw->menubar_toolbar_vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->menu_hbox), tw->menubar_toolbar_vbox, FALSE, FALSE, 0);

	/* Build layer menus */
	tw->layer_selector = pcb_gtk_layersel_build(tw->com, &tw->layersel);

	/* Build main menu */
	tw->menu.menu_bar = ghid_load_menus(&tw->menu, &tw->ghid_cfg);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->menu.menu_bar, FALSE, FALSE, 0);

	pcb_gtk_make_mode_buttons_and_toolbar(tw->com, &tw->mode_btn);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->mode_btn.mode_toolbar_vbox, FALSE, FALSE, 0);

	tw->position_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(tw->top_hbox), tw->position_hbox, FALSE, FALSE, 0);

	make_cursor_position_labels(tw->position_hbox, &tw->cps);

	hbox_middle = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_middle, TRUE, TRUE, 0);

	fix_topbar_theming(tw); /* Must be called after toolbar is created */

	/* -- Left control bar */
	/*
	 * This box will be made insensitive when the gui needs
	 * a modal button GetLocation button press.
	 */
	tw->left_toolbar = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->left_toolbar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(tw->left_toolbar), tw->layer_selector, TRUE, TRUE, 0);

	/* tw->mode_btn.mode_buttons_frame was created above in the call to
	 * make_mode_buttons_and_toolbar (...);
	 */
	gtk_box_pack_start(GTK_BOX(tw->left_toolbar), tw->mode_btn.mode_buttons_frame, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(tw->left_toolbar), frame, FALSE, FALSE, 0);
	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
	tw->route_style_selector = pcb_gtk_route_style_new(tw->com);
	make_route_style_buttons(GHID_ROUTE_STYLE(tw->route_style_selector));
	gtk_box_pack_start(GTK_BOX(hbox), tw->route_style_selector, FALSE, FALSE, 0);

	tw->vbox_middle = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->vbox_middle, TRUE, TRUE, 0);

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hbox, TRUE, TRUE, 0);

	/* -- The PCB layout output drawing area */

	tw->drawing_area = tw->com->new_drawing_widget(tw->com);
	g_signal_connect(G_OBJECT(tw->drawing_area), "realize", G_CALLBACK(tw->com->drawing_realize), tw->com->gport);
	tw->com->init_drawing_widget(tw->drawing_area, tw->com->gport);

	gtk_widget_add_events(tw->drawing_area, GDK_EXPOSURE_MASK
												| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
												| GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK
												| GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
												| GDK_SCROLL_MASK
												| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	/*
	 * This is required to get the drawing_area key-press-event.  Also the
	 * enter and button press callbacks grab focus to be sure we have it
	 * when in the drawing_area.
	 */
	gtk_widget_set_can_focus(tw->drawing_area, TRUE);

	gtk_box_pack_start(GTK_BOX(hbox), tw->drawing_area, TRUE, TRUE, 0);

	tw->v_adjustment = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));
	tw->v_range = gtk_vscrollbar_new(GTK_ADJUSTMENT(tw->v_adjustment));

	gtk_box_pack_start(GTK_BOX(hbox), tw->v_range, FALSE, FALSE, 0);


	g_signal_connect(G_OBJECT(tw->v_adjustment), "value_changed", G_CALLBACK(v_adjustment_changed_cb), tw);

	tw->h_adjustment = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));

	hbox_scroll = gtkc_hbox_new(FALSE, 0);
	tw->h_range = gtk_hscrollbar_new(GTK_ADJUSTMENT(tw->h_adjustment));
	fullscreen_btn = create_image_button_from_xpm_data(FullScreen_xpm);
	g_signal_connect(G_OBJECT(fullscreen_btn), "clicked", G_CALLBACK(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), tw->h_range, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_scroll), fullscreen_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hbox_scroll, FALSE, FALSE, 0);


	g_signal_connect(G_OBJECT(tw->h_adjustment), "value_changed", G_CALLBACK(h_adjustment_changed_cb), tw);

	/* -- The bottom status line label */
	tw->status_line_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), tw->status_line_hbox, FALSE, FALSE, 0);

	label = pcb_gtk_status_line_label_new();

	tw->status_line_label = label;
	gtk_box_pack_start(GTK_BOX(tw->status_line_hbox), label, FALSE, FALSE, 0);

	resize_grip_vbox = gtkc_vbox_new(FALSE, 0);
	resize_grip = gtk_event_box_new();
	resize_grip_pixbuf = gdk_pixbuf_new_from_xpm_data(resize_grip_xpm);
	resize_grip_image = gtk_image_new_from_pixbuf(resize_grip_pixbuf);
	g_object_unref(resize_grip_pixbuf);
	gtk_container_add(GTK_CONTAINER(resize_grip), resize_grip_image);
	gtk_widget_add_events(resize_grip, GDK_BUTTON_PRESS_MASK);
	gtk_widget_set_tooltip_text(resize_grip, "Left-click to resize the main window\nMid-click to move the window");
	g_signal_connect(resize_grip, "button_press_event", G_CALLBACK(resize_grip_button_press), NULL);
	gtk_box_pack_end(GTK_BOX(resize_grip_vbox), resize_grip, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(tw->status_line_hbox), resize_grip_vbox, FALSE, FALSE, 0);

	/* Depending on user setting, the command_combo_box may get packed into
	   |  the status_line_hbox, but it will happen on demand the first time
	   |  the user does a command entry.
	 */

	g_signal_connect(G_OBJECT(tw->drawing_area), "size-allocate", G_CALLBACK(drawing_area_size_allocate_cb), tw);
	g_signal_connect(G_OBJECT(tw->drawing_area), "enter-notify-event", G_CALLBACK(drawing_area_enter_cb), tw);
	g_signal_connect(G_OBJECT(tw->com->top_window), "configure_event", G_CALLBACK(top_window_configure_event_cb), tw->com->gport);
	g_signal_connect(tw->com->top_window, "enter-notify-event", G_CALLBACK(top_window_enter_cb), tw);

	g_signal_connect(G_OBJECT(tw->com->top_window), "delete_event", G_CALLBACK(delete_chart_cb), tw->com->gport);
	g_signal_connect(G_OBJECT(tw->com->top_window), "destroy", G_CALLBACK(destroy_chart_cb), tw);

	gtk_widget_show_all(tw->com->top_window);
	tw->com->pack_mode_buttons();

	ghid_fullscreen_apply(tw);
	tw->active = 1;
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
	tw->com->load_bg_image();

	ghid_build_pcb_top_window(tw);
	ghid_install_accel_groups(GTK_WINDOW(tw->com->top_window), tw);
	ghid_update_toggle_flags(tw, NULL);

	pcb_gtk_icons_init(GTK_WINDOW(tw->com->top_window));
	pcb_tool_select_by_id(PCB_MODE_ARROW);
	ghid_mode_buttons_update();
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
