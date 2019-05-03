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
#include "hidlib_conf.h"

#include "board.h"
#include "crosshair.h"
#include "pcb-printf.h"
#include "actions.h"
#include "compat_misc.h"

#include "compat.h"
#include "bu_menu.h"
#include "bu_icons.h"
#include "dlg_attribute.h"
#include "util_listener.h"
#include "in_mouse.h"
#include "in_keyboard.h"
#include "wt_layersel.h"
#include "tool.h"
#include "lib_gtk_config.h"
#include "hid_gtk_conf.h"
#include "win_place.h"

/*** docking code (dynamic parts) ***/
static int pcb_gtk_dock_poke(pcb_hid_dad_subdialog_t *sub, const char *cmd, pcb_event_arg_t *res, int argc, pcb_event_arg_t *argv)
{
	return -1;
}

typedef struct {
	void *hid_ctx;
	GtkWidget *frame;
	pcb_gtk_topwin_t *tw;
	pcb_hid_dock_t where;
} docked_t;

GdkColor clr_orange = {0,  0xffff, 0xaaaa, 0x3333};

static int dock_is_vert[PCB_HID_DOCK_max]   = {0, 0, 0, 1, 0, 1}; /* Update this if pcb_hid_dock_t changes */
static int dock_has_frame[PCB_HID_DOCK_max] = {0, 0, 0, 1, 0, 0}; /* Update this if pcb_hid_dock_t changes */
static GdkColor *dock_color[PCB_HID_DOCK_max] = {NULL, NULL, &clr_orange, NULL, NULL, NULL}; /* force change color when docked */
int pcb_gtk_tw_dock_enter(pcb_gtk_topwin_t *tw, pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id)
{
	docked_t *docked;
	GtkWidget *hvbox;

	docked = calloc(sizeof(docked_t), 1);
	docked->where = where;

	if (dock_is_vert[where])
		hvbox = gtkc_vbox_new(FALSE, 0);
	else
		hvbox = gtkc_hbox_new(TRUE, 0);

	if (dock_has_frame[where]) {
		docked->frame = gtk_frame_new(id);
		gtk_container_add(GTK_CONTAINER(docked->frame), hvbox);
	}
	else
		docked->frame = hvbox;

	gtk_box_pack_end(GTK_BOX(tw->dockbox[where]), docked->frame, TRUE, TRUE, 0);
	gtk_widget_show_all(docked->frame);

	sub->parent_poke = pcb_gtk_dock_poke;
	sub->dlg_hid_ctx = docked->hid_ctx = ghid_attr_sub_new(tw->com, hvbox, sub->dlg, sub->dlg_len, sub);
	docked->tw = tw;
	sub->parent_ctx = docked;

	gdl_append(&tw->dock[where], sub, link);

	if (dock_color[where] != NULL)
		pcb_gtk_dad_fixcolor(sub->dlg_hid_ctx, dock_color[where]);

	return 0;
}

void pcb_gtk_tw_dock_leave(pcb_gtk_topwin_t *tw, pcb_hid_dad_subdialog_t *sub)
{
	docked_t *docked = sub->parent_ctx;
	gtk_widget_destroy(docked->frame);
	gdl_remove(&tw->dock[docked->where], sub, link);
	free(docked);
	PCB_DAD_FREE(sub->dlg);
}

/*** static top window code ***/
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

/* Save size of top window changes so PCB can restart at its size at exit. */
static gint top_window_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, void *tw_)
{
	pcb_gtk_topwin_t *tw = tw_;
	return pcb_gtk_winplace_cfg(tw->com->hidlib, widget, NULL, "top");
}

gboolean ghid_idle_cb(void *topwin)
{
	pcb_gtk_topwin_t *tw = topwin;
	if (conf_core.editor.mode == PCB_MODE_NO)
		pcb_tool_select_by_id(PCB_MODE_ARROW);
	tw->com->mode_cursor_main(conf_core.editor.mode);
	return FALSE;
}

gboolean ghid_port_key_release_cb(GtkWidget *drawing_area, GdkEventKey *kev, pcb_gtk_topwin_t *tw)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		tw->com->note_event_location(NULL);

	pcb_tool_adjust_attached_objects();
	tw->com->invalidate_all(tw->com->hidlib);
	g_idle_add(ghid_idle_cb, tw);
	return FALSE;
}


/* Sync toggle states that were saved with the layout and notify the
   config code to update Settings values it manages. */
void ghid_sync_with_new_layout(pcb_gtk_topwin_t *tw)
{
	tw->com->window_set_name_label(tw->com->hidlib->name);
	update_board_mtime_from_disk(&tw->ext_chg);
}

void pcb_gtk_tw_notify_save_pcb(pcb_gtk_topwin_t *tw, const char *filename, pcb_bool done)
{
	/* Do nothing if it is not the active PCB file that is being saved. */
	if (tw->com->hidlib->filename == NULL || strcmp(filename, tw->com->hidlib->filename) != 0)
		return;

	if (done)
		update_board_mtime_from_disk(&tw->ext_chg);
}

void pcb_gtk_tw_notify_filename_changed(pcb_gtk_topwin_t *tw)
{
	/* Pick up the mtime of the new PCB file */
	update_board_mtime_from_disk(&tw->ext_chg);
	tw->com->window_set_name_label(tw->com->hidlib->name);
}

void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_add_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
}

void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw)
{
	gtk_window_remove_accel_group(window, ghid_main_menu_get_accel_group(GHID_MAIN_MENU(tw->menu.menu_bar)));
}

/* Refreshes the window title bar and sets the PCB name to the
   window title bar or to a seperate label */
void pcb_gtk_tw_window_set_name_label(pcb_gtk_topwin_t *tw, const char *name)
{
	const char *filename;
	char tmp[512];

	/* This happens if we're calling an exporter from the command line */
	if (!tw->active)
		return;

	if ((name == NULL) || (*name == '\0'))
		name = "Unnamed";

	if (!tw->com->hidlib->filename || !*tw->com->hidlib->filename)
		filename = "<board with no file name or format>";
	else
		filename = tw->com->hidlib->filename;

	pcb_snprintf(tmp, sizeof(tmp), "%s%s (%s) - %s - pcb-rnd", PCB->Changed ? "*" : "", name, filename, PCB->is_footprint ? "footprint" : "board");
	gtk_window_set_title(GTK_WINDOW(tw->com->top_window), tmp);
}

void pcb_gtk_tw_layer_buttons_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layersel_update(tw->com, &tw->layersel);
}

void pcb_gtk_tw_layer_vis_update(pcb_gtk_topwin_t *tw)
{
	pcb_gtk_layersel_vis_update(&tw->layersel);
}

/*** Top window ***/
static gint delete_chart_cb(GtkWidget *widget, GdkEvent *event, void *data)
{
	pcb_action("Quit");

	/* Return TRUE to keep our app running.  A FALSE here would let the
	   delete signal continue on and kill our program. */
	return TRUE;
}

static void destroy_chart_cb(GtkWidget *widget, pcb_gtk_topwin_t *tw)
{
	tw->com->main_destroy(tw->com->gport);
}

static void get_widget_styles(pcb_gtk_topwin_t *tw, GtkStyle **menu_bar_style)
{
	/* Grab the various styles we need */
	gtk_widget_ensure_style(tw->menu.menu_bar);
	*menu_bar_style = gtk_widget_get_style(tw->menu.menu_bar);
}

static void do_fix_topbar_theming(pcb_gtk_topwin_t *tw)
{
	GtkStyle *menu_bar_style;

	get_widget_styles(tw, &menu_bar_style);

	/* Style the top bar background as if it were all a menu bar */
	gtk_widget_set_style(tw->top_bar_background, menu_bar_style);
}

/* Attempt to produce a conststent style for our extra menu-bar items by
   copying aspects from the menu bar style set by the user's GTK theme.
   Setup signal handlers to update our efforts if the user changes their
   theme whilst we are running. */
static void fix_topbar_theming(pcb_gtk_topwin_t *tw)
{
	GtkSettings *settings;

	do_fix_topbar_theming(tw);

	settings = gtk_widget_get_settings(tw->top_bar_background);
	g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(do_fix_topbar_theming), NULL);
	g_signal_connect(settings, "notify::gtk-font-name", G_CALLBACK(do_fix_topbar_theming), NULL);
}

static void fullscreen_cb(GtkButton *btn, void *data)
{
	conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !pcbhl_conf.editor.fullscreen, POL_OVERWRITE);
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
	gtkc_widget_get_preferred_height(GTK_WIDGET(tw->bottom_hbox), &min, NULL);
}

static gboolean drawing_area_enter_cb(GtkWidget *w, pcb_gtk_expose_t *p, void *user_data)
{
	pcb_gtk_topwin_t *tw = user_data;
	tw->com->invalidate_all(tw->com->hidlib);
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

void ghid_topwin_hide_status(void *ctx, int show)
{
	pcb_gtk_topwin_t *tw = ctx;

	if (show)
		gtk_widget_show(tw->dockbox[PCB_HID_DOCK_BOTTOM]);
	else
		gtk_widget_hide(tw->dockbox[PCB_HID_DOCK_BOTTOM]);
}

/* Create the top_window contents.  The config settings should be loaded
   before this is called. */
static void ghid_build_pcb_top_window(pcb_gtk_topwin_t *tw)
{
	GtkWidget *vbox_main, *hbox_middle, *hbox, *hboxi, *evb;
	GtkWidget *hbox_scroll, *fullscreen_btn;
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

	/* menu_hbox will be made insensitive when the gui needs
	   a modal button GetLocation button press. */
	tw->menu_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->top_hbox), tw->menu_hbox, FALSE, FALSE, 0);

	tw->menubar_toolbar_vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->menu_hbox), tw->menubar_toolbar_vbox, FALSE, FALSE, 0);

	/* Build layer menus */
	tw->layer_selector = pcb_gtk_layersel_build(tw->com, &tw->layersel);

	/* Build main menu */
	tw->menu.menu_bar = ghid_load_menus(&tw->menu, &tw->ghid_cfg);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->menu.menu_bar, FALSE, FALSE, 0);

	tw->dockbox[PCB_HID_DOCK_TOP_LEFT] = gtkc_hbox_new(TRUE, 2);
	gtk_box_pack_start(GTK_BOX(tw->menubar_toolbar_vbox), tw->dockbox[PCB_HID_DOCK_TOP_LEFT], FALSE, FALSE, 0);

	tw->position_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(tw->top_hbox), tw->position_hbox, FALSE, FALSE, 0);

	tw->dockbox[PCB_HID_DOCK_TOP_RIGHT] = gtkc_vbox_new(FALSE, 8);
	gtk_box_pack_end(GTK_BOX(GTK_BOX(tw->position_hbox)), tw->dockbox[PCB_HID_DOCK_TOP_RIGHT], FALSE, FALSE, 0);

	hbox_middle = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_middle, TRUE, TRUE, 0);

	fix_topbar_theming(tw); /* Must be called after toolbar is created */

	/* -- Left control bar */
	/* This box will be made insensitive when the gui needs
	 * a modal button GetLocation button press. */
	tw->left_toolbar = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->left_toolbar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(tw->left_toolbar), tw->layer_selector, TRUE, TRUE, 0);
	tw->dockbox[PCB_HID_DOCK_LEFT] = gtkc_vbox_new(FALSE, 8);
	gtk_box_pack_end(GTK_BOX(GTK_BOX(tw->left_toolbar)), tw->dockbox[PCB_HID_DOCK_LEFT], FALSE, FALSE, 0);

	/* -- main content */
	tw->vbox_middle = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_middle), tw->vbox_middle, TRUE, TRUE, 0);


	/* -- The PCB layout output drawing area */

	/* info bar: hboxi->event_box->hbox2:
	    hboxi is for the layout (horizontal fill)
	    the event box is neeed for background color
      vbox is tw->dockbox[PCB_HID_DOCK_TOP_INFOBAR] where DAD widgets are packed */
	hboxi = gtkc_hbox_new(TRUE, 0);
	tw->dockbox[PCB_HID_DOCK_TOP_INFOBAR] = gtkc_vbox_new(TRUE, 0);
	evb = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(evb), tw->dockbox[PCB_HID_DOCK_TOP_INFOBAR]);
	gtk_box_pack_start(GTK_BOX(hboxi), evb, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hboxi, FALSE, FALSE, 0);

	if (dock_color[PCB_HID_DOCK_TOP_INFOBAR] != NULL)
		gtk_widget_modify_bg(evb, GTK_STATE_NORMAL, dock_color[PCB_HID_DOCK_TOP_INFOBAR]);

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), hbox, TRUE, TRUE, 0);

	/* drawing area */
	tw->drawing_area = tw->com->new_drawing_widget(tw->com);
	g_signal_connect(G_OBJECT(tw->drawing_area), "realize", G_CALLBACK(tw->com->drawing_realize), tw->com->gport);
	tw->com->init_drawing_widget(tw->drawing_area, tw->com->gport);

	gtk_widget_add_events(tw->drawing_area,
		GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
		| GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_RELEASE_MASK
		| GDK_KEY_PRESS_MASK | GDK_SCROLL_MASK | GDK_FOCUS_CHANGE_MASK
		| GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	/* This is required to get the drawing_area key-press-event.  Also the
	 * enter and button press callbacks grab focus to be sure we have it
	 * when in the drawing_area. */
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
	tw->bottom_hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tw->vbox_middle), tw->bottom_hbox, FALSE, FALSE, 0);

	tw->dockbox[PCB_HID_DOCK_BOTTOM] = gtkc_hbox_new(TRUE, 2);
	gtk_box_pack_start(GTK_BOX(tw->bottom_hbox), tw->dockbox[PCB_HID_DOCK_BOTTOM], FALSE, FALSE, 0);

	tw->cmd.prompt_label = gtk_label_new("action:");
	gtk_box_pack_start(GTK_BOX(tw->bottom_hbox), tw->cmd.prompt_label, FALSE, FALSE, 0);
	ghid_command_combo_box_entry_create(&tw->cmd, ghid_topwin_hide_status, tw);
	gtk_box_pack_start(GTK_BOX(tw->bottom_hbox), tw->cmd.command_combo_box, FALSE, FALSE, 0);

	/* resize grip: rightmost widget in the status line hbox */
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
	gtk_box_pack_end(GTK_BOX(tw->bottom_hbox), resize_grip_vbox, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(tw->drawing_area), "size-allocate", G_CALLBACK(drawing_area_size_allocate_cb), tw);
	g_signal_connect(G_OBJECT(tw->drawing_area), "enter-notify-event", G_CALLBACK(drawing_area_enter_cb), tw);
	g_signal_connect(G_OBJECT(tw->com->top_window), "configure_event", G_CALLBACK(top_window_configure_event_cb), tw);

	g_signal_connect(G_OBJECT(tw->com->top_window), "delete_event", G_CALLBACK(delete_chart_cb), tw->com->gport);
	g_signal_connect(G_OBJECT(tw->com->top_window), "destroy", G_CALLBACK(destroy_chart_cb), tw);

	gtk_widget_show_all(tw->com->top_window);

	ghid_fullscreen_apply(tw);
	tw->active = 1;

	gtk_widget_hide(tw->cmd.command_combo_box);
	gtk_widget_hide(tw->cmd.prompt_label);
}

/* We'll set the interface insensitive when a g_main_loop is running so the
   Gtk menus and buttons don't respond and interfere with the special entry
   the user needs to be doing. */
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
}

void ghid_fullscreen_apply(pcb_gtk_topwin_t *tw)
{
	if (pcbhl_conf.editor.fullscreen) {
		gtk_widget_hide(tw->left_toolbar);
		gtk_widget_hide(tw->top_hbox);
		if (!tw->cmd.command_entry_status_line_active)
			gtk_widget_hide(tw->bottom_hbox);
	}
	else {
		gtk_widget_show(tw->left_toolbar);
		gtk_widget_show(tw->top_hbox);
		gtk_widget_show(tw->bottom_hbox);
	}
}
