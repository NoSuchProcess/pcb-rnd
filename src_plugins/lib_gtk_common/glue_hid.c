#include "config.h"
#include "glue_hid.h"

#include <locale.h>

#include "pcb_gtk.h"
#include "actions.h"
#include "glue_hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "coord_conv.h"

#include "in_keyboard.h"
#include "bu_dwg_tooltip.h"
#include "ui_crosshair.h"
#include "dlg_fileselect.h"
#include "dlg_attribute.h"
#include "dlg_attributes.h"
#include "util_listener.h"
#include "util_timer.h"
#include "util_watch.h"
#include "hid_gtk_conf.h"
#include "lib_gtk_config.h"
#include "glue_common.h"
#include "../src_plugins/lib_hid_common/menu_helper.h"

extern pcb_hid_cfg_keys_t ghid_keymap;

static gint ghid_port_window_enter_cb(GtkWidget *widget, GdkEventCrossing *ev, void *out_)
{
	pcb_gtk_port_t *out = out_;
	int force_update = 0;

	/* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* See comment in ghid_port_window_leave_cb() */

	if (ev->mode != GDK_CROSSING_NORMAL && ev->detail != GDK_NOTIFY_NONLINEAR)
		return FALSE;

	if (!ghidgui->topwin.cmd.command_entry_status_line_active) {
		out->view.has_entered = TRUE;
		force_update = 1; /* force a redraw for the crosshair */
		gtk_widget_grab_focus(out->drawing_area); /* Make sure drawing area has keyboard focus when we are in it. */
	}

	/* Following expression is true if a you open a menu from the menu bar,
	   move the mouse to the viewport and click on it. This closes the menu
	   and moves the pointer to the viewport without the pointer going over
	   the edge of the viewport */
	if (force_update || (ev->mode == GDK_CROSSING_UNGRAB && ev->detail == GDK_NOTIFY_NONLINEAR))
		ghidgui->impl.screen_update();
	return FALSE;
}

static gint ghid_port_window_leave_cb(GtkWidget *widget, GdkEventCrossing *ev, void *out_)
{
	pcb_gtk_port_t *out = out_;

	/* printf("leave mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* Window leave events can also be triggered because of focus grabs. Some
	   X applications occasionally grab the focus and so trigger this function.
	   At least GNOME's window manager is known to do this on every mouse click.
	   See http://bugzilla.gnome.org/show_bug.cgi?id=102209 */
	if (ev->mode != GDK_CROSSING_NORMAL)
		return FALSE;

	out->view.has_entered = FALSE;

	ghidgui->impl.screen_update();

	return FALSE;
}

static gboolean check_object_tooltips(pcb_gtk_port_t *out)
{
	return pcb_gtk_dwg_tooltip_check_object(out->drawing_area, out->view.crosshair_x, out->view.crosshair_y);
}

static gint ghid_port_window_motion_cb(GtkWidget *widget, GdkEventMotion *ev, void *out_)
{
	pcb_gtk_port_t *out = out_;
	gdouble dx, dy;
	static gint x_prev = -1, y_prev = -1;

	gdk_event_request_motions(ev);

	if (out->view.panning) {
		dx = ghidgui->port.view.coord_per_px * (x_prev - ev->x);
		dy = ghidgui->port.view.coord_per_px * (y_prev - ev->y);
		if (x_prev > 0)
			pcb_gtk_pan_view_rel(&ghidgui->port.view, dx, dy);
		x_prev = ev->x;
		y_prev = ev->y;
		return FALSE;
	}
	x_prev = y_prev = -1;
	pcb_gtk_note_event_location((GdkEventButton *)ev);

	pcb_gtk_dwg_tooltip_queue(out->drawing_area, (GSourceFunc)check_object_tooltips, out);

	return FALSE;
}

static void ghid_gui_inited(pcb_gtk_t *gctx, int main, int conf)
{
	static int im = 0, ic = 0, first = 1;
	if (main) im = 1;
	if (conf) ic = 1;

	if (im && ic && first) {
		first = 0;
		pcb_event(gctx->hidlib, PCB_EVENT_GUI_INIT, NULL);
		pcb_gtk_zoom_view_win_side(&gctx->port.view, 0, 0, gctx->hidlib->size_x, gctx->hidlib->size_y, 0);
	}
}

static gboolean ghid_port_drawing_area_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, void *out)
{
	ghidgui->port.view.canvas_width = ev->width;
	ghidgui->port.view.canvas_height = ev->height;

	ghidgui->impl.drawing_area_configure_hook(out);
	ghid_gui_inited(ghidgui, 0, 1);

	pcb_gtk_tw_ranges_scale(ghidgui);
	pcb_gui->invalidate_all(pcb_gui);
	return 0;
}


static void gtkhid_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	pcb_gtk_t *gctx = hid->hid_data;

	gctx->hid_active = 1;

	pcb_hid_cfg_keys_init(&ghid_keymap);
	ghid_keymap.translate_key = ghid_translate_key;
	ghid_keymap.key_name = ghid_key_name;
	ghid_keymap.auto_chr = 1;
	ghid_keymap.auto_tr = hid_cfg_key_default_trans;

	ghid_create_pcb_widgets(gctx, &gctx->topwin, gctx->port.top_window);

	/* assume pcb_gui is us */
	pcb_gui->hid_cfg = gctx->topwin.ghid_cfg;

	gctx->port.drawing_area = gctx->topwin.drawing_area;

TODO(": move this to render init")
	/* Mouse and key events will need to be intercepted when PCB needs a location from the user. */
	g_signal_connect(G_OBJECT(gctx->port.drawing_area), "scroll_event", G_CALLBACK(ghid_port_window_mouse_scroll_cb), gctx->port.mouse);
	g_signal_connect(G_OBJECT(gctx->port.drawing_area), "motion_notify_event", G_CALLBACK(ghid_port_window_motion_cb), &gctx->port);
	g_signal_connect(G_OBJECT(gctx->port.drawing_area), "configure_event", G_CALLBACK(ghid_port_drawing_area_configure_event_cb), &gctx->port);
	g_signal_connect(G_OBJECT(gctx->port.drawing_area), "enter_notify_event", G_CALLBACK(ghid_port_window_enter_cb), &gctx->port);
	g_signal_connect(G_OBJECT(gctx->port.drawing_area), "leave_notify_event", G_CALLBACK(ghid_port_window_leave_cb), &gctx->port);

	pcb_gtk_interface_input_signals_connect();

	if (conf_hid_gtk.plugins.hid_gtk.listen)
		pcb_gtk_create_listener();

	gctx->gui_is_up = 1;

	ghid_gui_inited(gctx, 1, 0);

	/* Make sure drawing area has keyboard focus so that keys are handled
	   while the mouse cursor is over the top window or children widgets,
	   before first entering the drawing area */
	gtk_widget_grab_focus(gctx->port.drawing_area);

	gtk_main();
	pcb_hid_cfg_keys_uninit(&ghid_keymap);

	gctx->hid_active = 0;
	gctx->gui_is_up = 0;
	hid->hid_cfg = NULL;
	hid->hid_data = NULL;
}

static void ghid_do_exit(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;

	/* Need to force-close the command entry first because it has its own main
	   loop that'd block the exit until the user closes the entry */
	ghid_cmd_close(&gctx->topwin.cmd);

	gtk_main_quit();
}

static void pcb_gtk_topwinplace(pcb_hidlib_t *hidlib, GtkWidget *dialog, const char *id)
{
	int plc[4] = {-1, -1, -1, -1};

	pcb_event(hidlib, PCB_EVENT_DAD_NEW_DIALOG, "psp", NULL, id, plc);

	if (pcbhl_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0))
			gtk_window_resize(GTK_WINDOW(dialog), plc[2], plc[3]);
		if ((plc[0] >= 0) && (plc[1] >= 0))
			gtk_window_move(GTK_WINDOW(dialog), plc[0], plc[1]);
	}
}

/* Create top level window for routines that will need top_window before ghid_create_pcb_widgets() is called. */
int gtkhid_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_gtk_t *gctx = hid->hid_data;
	GtkWidget *window;

	/* on windows we need to figure out the installation directory */
TODO("This needs to be done centrally, and should not use PCB_PACKAGE but pcbhl_app_*")
#ifdef WIN32
	char *tmps;
	char *libdir;
	tmps = g_win32_get_package_installation_directory(PCB_PACKAGE "-" PCB_VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PCB_PACKAGE  G_DIR_SEPARATOR_S "pcblib"
	libdir = (char *) malloc(strlen(tmps) + strlen(REST_OF_PATH) + 1);
	sprintf(libdir, "%s%s", tmps, REST_OF_PATH);
	free(tmps);

#undef REST_OF_PATH

#endif

	conf_parse_arguments("plugins/hid_gtk/", argc, argv);

	if (!gtk_init_check(argc, argv)) {
		fprintf(stderr, "gtk_init_check() fail - maybe $DISPLAY not set or X/GUI not accessible?\n");
		return 1; /* recoverable error - try another HID */
	}

	gctx->port.view.use_max_pcb = 1;
	gctx->port.view.coord_per_px = 300.0;
	pcb_pixel_slop = 300;

	gctx->impl.init_renderer(argc, argv, &gctx->port);
	gctx->wtop_window = window = gctx->port.top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	pcb_gtk_topwinplace(gctx->hidlib, window, "top");
	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	gtk_widget_show_all(gctx->port.top_window);
	return 0;
}

static void ghid_calibrate(pcb_hid_t *hid, double xval, double yval)
{
	printf("ghid_calibrate() -- not implemented\n");
}

static void ghid_set_crosshair(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int action)
{
	pcb_gtk_t *gctx = hid->hid_data;
	int offset_x, offset_y;

	if ((gctx->port.drawing_area == NULL) || (gctx->hidlib == NULL))
		return;

	gctx->impl.draw_grid_local(gctx->hidlib, x, y);
	gdk_window_get_origin(gtk_widget_get_window(gctx->port.drawing_area), &offset_x, &offset_y);
	pcb_gtk_crosshair_set(x, y, action, offset_x, offset_y, &gctx->port.view);
}

static void ghid_get_coords(pcb_hid_t *hid, const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_gtk_get_coords(gctx, &gctx->port.view, msg, x, y, force);
}

pcb_hidval_t ghid_add_timer(pcb_hid_t *hid, void (*func)(pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	return pcb_gtk_add_timer((pcb_gtk_t *)hid->hid_data, func, milliseconds, user_data);
}

static pcb_hidval_t ghid_watch_file(pcb_hid_t *hid, int fd, unsigned int condition,
	pcb_bool (*func)(pcb_hidval_t, int, unsigned int, pcb_hidval_t), pcb_hidval_t user_data)
{
	return pcb_gtk_watch_file((pcb_gtk_t *)hid->hid_data, fd, condition, func, user_data);
}

static char *ghid_fileselect(pcb_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub)
{
	return pcb_gtk_fileselect((pcb_gtk_t *)hid->hid_data, title, descr, default_file, default_ext, flt, history_tag, flags, sub);
}

static void *ghid_attr_dlg_new_(pcb_hid_t *hid, const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	return ghid_attr_dlg_new((pcb_gtk_t *)hid->hid_data, id, attrs, n_attrs, results, title, caller_data, modal, button_cb, defx, defy, minx, miny);
}

static void ghid_beep(pcb_hid_t *hid)
{
	gdk_beep();
}

static void ghid_attributes(pcb_hid_t *hid, const char *owner, pcb_attribute_list_t *attrs)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_gtk_dlg_attributes(gctx->port.top_window, owner, attrs);
}

static void PointCursor(pcb_hid_t *hid, pcb_bool grabbed)
{
	pcb_gtk_t *gctx = hid->hid_data;

	if (gctx == NULL)
		return;

	ghid_point_cursor(ghidgui, grabbed);
}

/* Create a new menu by path */
static int ghid_remove_menu(pcb_hid_t *hid, const char *menu_path)
{
	pcb_gtk_t *gctx = hid->hid_data;

	if (gctx->topwin.ghid_cfg == NULL)
		return -1;
	return pcb_hid_cfg_remove_menu(gctx->topwin.ghid_cfg, menu_path, ghid_remove_menu_widget, gctx->topwin.menu.menu_bar);
}

static int ghid_remove_menu_node(pcb_hid_t *hid, lht_node_t *node)
{
	pcb_gtk_t *gctx = hid->hid_data;
	return pcb_hid_cfg_remove_menu_node(gctx->topwin.ghid_cfg, node, ghid_remove_menu_widget, gctx->topwin.menu.menu_bar);
}

static void ghid_create_menu(pcb_hid_t *hid, const char *menu_path, const pcb_menu_prop_t *props)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_hid_cfg_create_menu(gctx->topwin.ghid_cfg, menu_path, props, ghid_create_menu_widget, &gctx->topwin.menu);
}

static void ghid_update_menu_checkbox(pcb_hid_t *hid, const char *cookie)
{
	pcb_gtk_t *gctx = hid->hid_data;
	if (gctx->hid_active)
		ghid_update_toggle_flags(&gctx->topwin, cookie);
}

pcb_hid_cfg_t *ghid_get_menu_cfg(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	if (!gctx->hid_active)
		return NULL;
	return gctx->topwin.ghid_cfg;
}

static int ghid_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	conf_usage("plugins/hid_gtk", pcb_hid_usage_option);
	fprintf(stderr, "\nInvocation:\n");
	fprintf(stderr, "  pcb-rnd --gui gtk2_gdk [options]\n");
	fprintf(stderr, "  pcb-rnd --gui gtk2_gl [options]\n");
	fprintf(stderr, "  (depending on which gtk plugin(s) are compiled and installed)\n");
	return 0;
}

static const char *ghid_command_entry(pcb_hid_t *hid, const char *ovr, int *cursor)
{
	pcb_gtk_t *gctx = hid->hid_data;
	return pcb_gtk_cmd_command_entry(&gctx->topwin.cmd, ovr, cursor);
}

static int ghid_clip_set(pcb_hid_t *hid, pcb_hid_clipfmt_t format, const void *data, size_t len)
{
	GtkClipboard *cbrd = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	switch(format) {
		case PCB_HID_CLIPFMT_TEXT:
			gtk_clipboard_set_text(cbrd, data, len);
			break;
	}
	return 0;
}



int ghid_clip_get(pcb_hid_t *hid, pcb_hid_clipfmt_t *format, void **data, size_t *len)
{
	GtkClipboard *cbrd = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	if (gtk_clipboard_wait_is_text_available(cbrd)) {
		gchar *txt = gtk_clipboard_wait_for_text(cbrd);
		*format = PCB_HID_CLIPFMT_TEXT;
		*data = txt;
		*len = strlen(txt) + 1;
		return 0;
	}

	return -1;
}

void ghid_clip_free(pcb_hid_t *hid, pcb_hid_clipfmt_t format, void *data, size_t len)
{
	switch(format) {
		case PCB_HID_CLIPFMT_TEXT:
			g_free(data);
			break;
	}
}

static void ghid_iterate(pcb_hid_t *hid)
{
	while(gtk_events_pending())
		gtk_main_iteration_do(0);
}

static double ghid_benchmark(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	int i = 0;
	time_t start, end;
	GdkDisplay *display;
	GdkWindow *window;

	window = gtk_widget_get_window(gctx->port.drawing_area);
	display = gtk_widget_get_display(gctx->port.drawing_area);

	gdk_display_sync(display);
	time(&start);
	do {
		pcb_gui->invalidate_all(pcb_gui);
		gdk_window_process_updates(window, FALSE);
		time(&end);
		i++;
	}
	while (end - start < 10);

	return i/10.0;
}

static int ghid_dock_enter(pcb_hid_t *hid, pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id)
{
	pcb_gtk_t *gctx = hid->hid_data;
	return pcb_gtk_tw_dock_enter(&gctx->topwin, sub, where, id);
}

static void ghid_dock_leave(pcb_hid_t *hid, pcb_hid_dad_subdialog_t *sub)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_gtk_tw_dock_leave(&gctx->topwin, sub);
}

static void ghid_zoom_win(pcb_hid_t *hid, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_bool set_crosshair)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_gtk_zoom_view_win_side(&gctx->port.view, x1, y1, x2, y2, set_crosshair);
}

static void ghid_zoom(pcb_hid_t *hid, pcb_coord_t center_x, pcb_coord_t center_y, double factor, int relative)
{
	pcb_gtk_t *gctx = hid->hid_data;
	if (relative)
		pcb_gtk_zoom_view_rel(&gctx->port.view, center_x, center_y, factor);
	else
		pcb_gtk_zoom_view_abs(&gctx->port.view, center_x, center_y, factor);
}

static void ghid_pan(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int relative)
{
	pcb_gtk_t *gctx = hid->hid_data;
	if (relative)
		pcb_gtk_pan_view_rel(&gctx->port.view, x, y);
	else
		pcb_gtk_pan_view_abs(&gctx->port.view, x, y, gctx->port.view.canvas_width/2.0, gctx->port.view.canvas_height/2.0);
}

static void ghid_pan_mode(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, pcb_bool mode)
{
	pcb_gtk_t *gctx = hid->hid_data;
	gctx->port.view.panning = mode;
}

static void ghid_view_get(pcb_hid_t *hid, pcb_box_t *viewbox)
{
	pcb_gtk_t *gctx = hid->hid_data;
	viewbox->X1 = gctx->port.view.x0;
	viewbox->Y1 = gctx->port.view.y0;
	viewbox->X2 = pcb_round((double)gctx->port.view.x0 + (double)gctx->port.view.canvas_width * gctx->port.view.coord_per_px);
	viewbox->Y2 = pcb_round((double)gctx->port.view.y0 + (double)gctx->port.view.canvas_height * gctx->port.view.coord_per_px);
}

static void ghid_open_command(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	ghid_handle_user_command(&gctx->topwin.cmd, TRUE);
}

static int ghid_open_popup(pcb_hid_t *hid, const char *menupath)
{
	pcb_gtk_t *gctx = hid->hid_data;
	GtkWidget *menu = NULL;
	lht_node_t *menu_node = pcb_hid_cfg_get_menu(gctx->topwin.ghid_cfg, menupath);

	if (menu_node == NULL)
		return 1;

	menu = pcb_gtk_menu_widget(menu_node);
	if (!GTK_IS_MENU(menu)) {
		pcb_message(PCB_MSG_ERROR, "The specified popup menu \"%s\" has not been defined.\n", menupath);
		return 1;
	}

	gtk_widget_grab_focus(gctx->port.drawing_area);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	gtk_window_set_transient_for(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(menu))), GTK_WINDOW(gtk_widget_get_toplevel(gctx->port.drawing_area)));
	return 0;
}

static void ghid_set_hidlib(pcb_hid_t *hid, pcb_hidlib_t *hidlib)
{
	pcb_gtk_t *gctx = hid->hid_data;

	if (gctx == NULL)
		return;

	gctx->hidlib = hidlib;

	if(!gctx->hid_active)
		return;

	if (hidlib == NULL)
		return;

	if (!gctx->port.drawing_allowed)
		return;

	pcb_gtk_tw_ranges_scale(gctx);
	pcb_gtk_zoom_view_win_side(&gctx->port.view, 0, 0, hidlib->size_x, hidlib->size_y, 0);
}

static void ghid_reg_mouse_cursor(pcb_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
	ghid_port_reg_mouse_cursor((pcb_gtk_t *)hid->hid_data, idx, name, pixel, mask);
}

static void ghid_set_mouse_cursor(pcb_hid_t *hid, int idx)
{
	ghid_port_set_mouse_cursor((pcb_gtk_t *)hid->hid_data, idx);
}

static void ghid_set_top_title(pcb_hid_t *hid, const char *title)
{
	pcb_gtk_t *gctx = hid->hid_data;
	pcb_gtk_tw_set_title(&gctx->topwin, title);
}

static void ghid_busy(pcb_hid_t *hid, pcb_bool busy)
{
	pcb_gtk_t *gctx = hid->hid_data;
	if ((gctx == NULL) || (!gctx->hid_active))
		return;
	if (busy)
		ghid_watch_cursor(gctx);
	else
		ghid_restore_cursor(gctx);
}

static int ghid_shift_is_pressed(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	pcb_gtk_port_t *out = &ghidgui->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

#ifdef PCB_WORKAROUND_GTK_SHIFT
	/* On some systems the above query fails and we need to return the last known state instead */
	return pcb_gtk_glob_mask & GDK_SHIFT_MASK;
#else
	return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
#endif
}

static int ghid_control_is_pressed(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	pcb_gtk_port_t *out = &gctx->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

#ifdef PCB_WORKAROUND_GTK_CTRL
	/* On some systems the above query fails and we need to return the last known state instead */
	return pcb_gtk_glob_mask & GDK_CONTROL_MASK;
#else
	return (mask & GDK_CONTROL_MASK) ? TRUE : FALSE;
#endif
}

static int ghid_mod1_is_pressed(pcb_hid_t *hid)
{
	pcb_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	pcb_gtk_port_t *out = &gctx->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);
#ifdef __APPLE__
	return (mask & (1 << 13)) ? TRUE : FALSE;	/* The option key is not MOD1, although it should be... */
#else
	return (mask & GDK_MOD1_MASK) ? TRUE : FALSE;
#endif
}


void ghid_glue_hid_init(pcb_hid_t *dst)
{
	memset(dst, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(dst);

	dst->struct_size = sizeof(pcb_hid_t);
	dst->gui = 1;
	dst->heavy_term_layer_ind = 1;
	dst->allow_dad_before_init = 1;

	dst->do_export = gtkhid_do_export;
	dst->do_exit = ghid_do_exit;
	dst->iterate = ghid_iterate;
	dst->parse_arguments = gtkhid_parse_arguments;

	dst->calibrate = ghid_calibrate;
	dst->shift_is_pressed = ghid_shift_is_pressed;
	dst->control_is_pressed = ghid_control_is_pressed;
	dst->mod1_is_pressed = ghid_mod1_is_pressed;
	dst->get_coords = ghid_get_coords;
	dst->set_crosshair = ghid_set_crosshair;
	dst->add_timer = ghid_add_timer;
	dst->stop_timer = ghid_stop_timer;
	dst->watch_file = ghid_watch_file;
	dst->unwatch_file = pcb_gtk_unwatch_file;

	dst->fileselect = ghid_fileselect;
	dst->attr_dlg_new = ghid_attr_dlg_new_;
	dst->attr_dlg_run = ghid_attr_dlg_run;
	dst->attr_dlg_raise = ghid_attr_dlg_raise;
	dst->attr_dlg_free = ghid_attr_dlg_free;
	dst->attr_dlg_property = ghid_attr_dlg_property;
	dst->attr_dlg_widget_state = ghid_attr_dlg_widget_state;
	dst->attr_dlg_widget_hide = ghid_attr_dlg_widget_hide;
	dst->attr_dlg_set_value = ghid_attr_dlg_set_value;
	dst->attr_dlg_set_help = ghid_attr_dlg_set_help;
	dst->supports_dad_text_markup = 1;

	dst->dock_enter = ghid_dock_enter;
	dst->dock_leave = ghid_dock_leave;

	dst->beep = ghid_beep;
	dst->edit_attributes = ghid_attributes;
	dst->point_cursor = PointCursor;
	dst->benchmark = ghid_benchmark;

	dst->command_entry = ghid_command_entry;

	dst->create_menu = ghid_create_menu;
	dst->remove_menu = ghid_remove_menu;
	dst->remove_menu_node = ghid_remove_menu_node;
	dst->update_menu_checkbox = ghid_update_menu_checkbox;
	dst->get_menu_cfg = ghid_get_menu_cfg;

	dst->clip_set  = ghid_clip_set;
	dst->clip_get  = ghid_clip_get;
	dst->clip_free = ghid_clip_free;

	dst->zoom_win = ghid_zoom_win;
	dst->zoom = ghid_zoom;
	dst->pan = ghid_pan;
	dst->pan_mode = ghid_pan_mode;
	dst->view_get = ghid_view_get;
	dst->open_command = ghid_open_command;
	dst->open_popup = ghid_open_popup;
	dst->reg_mouse_cursor = ghid_reg_mouse_cursor;
	dst->set_mouse_cursor = ghid_set_mouse_cursor;
	dst->set_top_title = ghid_set_top_title;
	dst->busy = ghid_busy;

	dst->set_hidlib = ghid_set_hidlib;

	dst->key_state = &ghid_keymap;

	dst->usage = ghid_usage;

	dst->hid_data = ghidgui;
}
