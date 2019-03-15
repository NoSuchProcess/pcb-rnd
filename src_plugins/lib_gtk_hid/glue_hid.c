#include "config.h"
#include "glue_hid.h"

#include "gui.h"
#include "actions.h"
#include "glue_hid.h"
#include "render.h"
#include "common.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_draw_helpers.h"
#include "coord_conv.h"

#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_common/bu_dwg_tooltip.h"
#include "../src_plugins/lib_gtk_common/ui_crosshair.h"
#include "../src_plugins/lib_gtk_common/dlg_fileselect.h"
#include "../src_plugins/lib_gtk_common/dlg_attribute.h"
#include "../src_plugins/lib_gtk_common/dlg_attributes.h"
#include "../src_plugins/lib_gtk_common/util_listener.h"
#include "../src_plugins/lib_gtk_common/util_timer.h"
#include "../src_plugins/lib_gtk_common/util_watch.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

void gtkhid_begin(void)
{
	pcb_gtk_action_reg();
	ghidgui->hid_active = 1;
}

void gtkhid_end(void)
{
	pcb_gtk_action_unreg();
	ghidgui->hid_active = 0;
}

static inline void ghid_screen_update(void) { ghidgui->common.screen_update(); }

static gint ghid_port_window_enter_cb(GtkWidget * widget, GdkEventCrossing * ev, void * out_)
{
	GHidPort *out = out_;
	int force_update = 0;

	/* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* See comment in ghid_port_window_leave_cb() */

	if (ev->mode != GDK_CROSSING_NORMAL && ev->detail != GDK_NOTIFY_NONLINEAR) {
		return FALSE;
	}

	if (!ghidgui->topwin.cmd.command_entry_status_line_active) {
		out->view.has_entered = TRUE;
		force_update = 1; /* force a redraw for the crosshair */
		/* Make sure drawing area has keyboard focus when we are in it.
		 */
		gtk_widget_grab_focus(out->drawing_area);
	}
	ghidgui->topwin.in_popup = FALSE;

	/* Following expression is true if a you open a menu from the menu bar,
	 * move the mouse to the viewport and click on it. This closes the menu
	 * and moves the pointer to the viewport without the pointer going over
	 * the edge of the viewport */
	if (force_update || (ev->mode == GDK_CROSSING_UNGRAB && ev->detail == GDK_NOTIFY_NONLINEAR)) {
		ghid_screen_update();
	}
	return FALSE;
}

static gint ghid_port_window_leave_cb(GtkWidget * widget, GdkEventCrossing * ev, void * out_)
{
	GHidPort *out = out_;

	/* printf("leave mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* Window leave events can also be triggered because of focus grabs. Some
	 * X applications occasionally grab the focus and so trigger this function.
	 * At least GNOME's window manager is known to do this on every mouse click.
	 *
	 * See http://bugzilla.gnome.org/show_bug.cgi?id=102209
	 */

	if (ev->mode != GDK_CROSSING_NORMAL) {
		return FALSE;
	}

	out->view.has_entered = FALSE;

	ghid_screen_update();

	return FALSE;
}

static gboolean check_object_tooltips(GHidPort *out)
{
	return pcb_gtk_dwg_tooltip_check_object(out->drawing_area, out->view.crosshair_x, out->view.crosshair_y);
}

static gint ghid_port_window_motion_cb(GtkWidget * widget, GdkEventMotion * ev, void * out_)
{
	GHidPort *out = out_;
	gdouble dx, dy;
	static gint x_prev = -1, y_prev = -1;

	gdk_event_request_motions(ev);

	if (out->view.panning) {
		dx = gport->view.coord_per_px * (x_prev - ev->x);
		dy = gport->view.coord_per_px * (y_prev - ev->y);
		if (x_prev > 0)
			pcb_gtk_pan_view_rel(&gport->view, dx, dy);
		x_prev = ev->x;
		y_prev = ev->y;
		return FALSE;
	}
	x_prev = y_prev = -1;
	ghid_note_event_location((GdkEventButton *) ev);

	pcb_gtk_dwg_tooltip_queue(out->drawing_area, (GSourceFunc)check_object_tooltips, out);

	return FALSE;
}

static gboolean ghid_port_drawing_area_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, void * out)
{
	static gboolean first_time_done;

	gport->view.canvas_width = ev->width;
	gport->view.canvas_height = ev->height;

	ghid_drawing_area_configure_hook(out);
	if (!first_time_done) {
		first_time_done = TRUE;
		pcb_board_changed(0);
	}

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	ghid_invalidate_all();
	return 0;
}


void gtkhid_do_export(pcb_hid_attr_val_t *options)
{
	gtkhid_begin();

	pcb_hid_cfg_keys_init(&ghid_keymap);
	ghid_keymap.translate_key = ghid_translate_key;
	ghid_keymap.key_name = ghid_key_name;
	ghid_keymap.auto_chr = 1;
	ghid_keymap.auto_tr = hid_cfg_key_default_trans;

	ghid_create_pcb_widgets(&ghidgui->topwin, gport->top_window);

	/* assume pcb_gui is us */
	pcb_gui->hid_cfg = ghidgui->topwin.ghid_cfg;

	gport->mouse.drawing_area = ghidgui->topwin.drawing_area;
	gport->drawing_area = ghidgui->topwin.drawing_area;
	gport->mouse.top_window = ghidgui->common.top_window;

TODO(": move this to render init")
	/* Mouse and key events will need to be intercepted when PCB needs a
	   |  location from the user.
	 */
	g_signal_connect(G_OBJECT(gport->drawing_area), "scroll_event", G_CALLBACK(ghid_port_window_mouse_scroll_cb), &gport->mouse);
	g_signal_connect(G_OBJECT(gport->drawing_area), "motion_notify_event", G_CALLBACK(ghid_port_window_motion_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "configure_event", G_CALLBACK(ghid_port_drawing_area_configure_event_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "enter_notify_event", G_CALLBACK(ghid_port_window_enter_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "leave_notify_event", G_CALLBACK(ghid_port_window_leave_cb), gport);

	ghid_interface_input_signals_connect();

	pcb_gtk_tw_layer_buttons_update(&ghidgui->topwin);

	if (conf_hid_gtk.plugins.hid_gtk.listen)
		pcb_gtk_create_listener();

	ghidgui->gui_is_up = 1;

	pcb_event(PCB_EVENT_GUI_INIT, NULL);

	/* Make sure drawing area has keyboard focus so that keys are handled
	   while the mouse cursor is over the top window or children widgets,
	   before first entering the drawing area */
	gtk_widget_grab_focus(gport->drawing_area);

	gtk_main();
	pcb_hid_cfg_keys_uninit(&ghid_keymap);
	gtkhid_end();

	ghidgui->gui_is_up = 0;
	pcb_gui->hid_cfg = NULL;
}

static void ghid_do_exit(pcb_hid_t * hid)
{
	/* Need to force-close the command entry first because it has its own main
	   loop that'd block the exit until the user closes the entry */
	ghid_cmd_close(&ghidgui->topwin.cmd);

	gtk_main_quit();
}

	/* Create top level window for routines that will need top_window
	   |  before ghid_create_pcb_widgets() is called.
	 */
int gtkhid_parse_arguments(int *argc, char ***argv)
{
	GtkWidget *window;

	/* on windows we need to figure out the installation directory */
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

	conf_parse_arguments("plugins/hid_gtk/", argc, argv);

	/*
	 * Prevent gtk_init() and gtk_init_check() from automatically
	 * calling setlocale (LC_ALL, "") which would undo LC_NUMERIC if ENABLE_NLS
	 * We also don't want locale set if no ENABLE_NLS to keep "C" LC_NUMERIC.
	 */
	gtk_disable_setlocale();

	if (!gtk_init_check(argc, argv)) {
		fprintf(stderr, "gtk_init_check() fail - maybe $DISPLAY not set or X/GUI not accessible?\n");
		return 1; /* recoverable error - try another HID */
	}


	gport = &ghid_port;
	gport->view.use_max_pcb = 1;
	gport->view.coord_per_px = 300.0;
	pcb_pixel_slop = 300;

	ghidgui->common.init_renderer(argc, argv, gport);

#ifdef ENABLE_NLS
#ifdef LOCALEDIR
	bindtextdomain(PCB_PACKAGE, LOCALEDIR);
#endif
	textdomain(PCB_PACKAGE);
	bind_textdomain_codeset(PCB_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	ghidgui->topwin.com = &ghidgui->common;
	ghidgui->common.top_window = window = gport->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	pcb_gtk_winplace(window, "top");
	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	gtk_widget_show_all(gport->top_window);
	return 0;
}

static void ghid_calibrate(double xval, double yval)
{
	printf("ghid_calibrate() -- not implemented\n");
}

static void ghid_set_crosshair(pcb_coord_t x, pcb_coord_t y, int action)
{
	int offset_x, offset_y;

	if (gport->drawing_area == NULL)
		return;

	ghidgui->common.draw_grid_local(x, y);
	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	pcb_gtk_crosshair_set(x, y, action, offset_x, offset_y, &ghidgui->topwin.cps, &gport->view);
}

static void ghid_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
	pcb_gtk_get_coords(&gport->mouse, &gport->view, msg, x, y, force);
}

static void ghid_get_view_size(pcb_coord_t * width, pcb_coord_t * height)
{
	*width = gport->view.width;
	*height = gport->view.height;
}

pcb_hidval_t ghid_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	return pcb_gtk_add_timer(&ghidgui->common, func, milliseconds, user_data);
}

static pcb_hidval_t ghid_watch_file(int fd, unsigned int condition,
								pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								pcb_hidval_t user_data)
{
	return pcb_gtk_watch_file(&ghidgui->common, fd, condition, func, user_data);
}

static char *ghid_fileselect(const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub)
{
	return pcb_gtk_fileselect(&ghidgui->common, title, descr, default_file, default_ext, flt, history_tag, flags, sub);
}

static void *ghid_attr_dlg_new_(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy)
{
	return ghid_attr_dlg_new(&ghidgui->common, id, attrs, n_attrs, results, title, caller_data, modal, button_cb, defx, defy);
}

static void ghid_beep()
{
	gdk_beep();
}

static void ghid_attributes(const char *owner, pcb_attribute_list_t * attrs)
{
	pcb_gtk_dlg_attributes(ghid_port.top_window, owner, attrs);
}

static void PointCursor(pcb_bool grabbed)
{
	if (!ghidgui)
		return;

	ghid_point_cursor(&gport->mouse, grabbed);
}

static void ghid_notify_save_pcb(const char *filename, pcb_bool done)
{
	pcb_gtk_tw_notify_save_pcb(&ghidgui->topwin, filename, done);
}

static void ghid_notify_filename_changed()
{
	pcb_gtk_tw_notify_filename_changed(&ghidgui->topwin);
}

/* Create a new menu by path */
static int ghid_remove_menu(const char *menu_path)
{
	return pcb_hid_cfg_remove_menu(ghidgui->topwin.ghid_cfg, menu_path, ghid_remove_menu_widget, ghidgui->topwin.menu.menu_bar);
}

static int ghid_remove_menu_node(lht_node_t *node)
{
	return pcb_hid_cfg_remove_menu_node(ghidgui->topwin.ghid_cfg, node, ghid_remove_menu_widget, ghidgui->topwin.menu.menu_bar);
}

static void ghid_create_menu(const char *menu_path, const pcb_menu_prop_t *props)
{
	pcb_hid_cfg_create_menu(ghidgui->topwin.ghid_cfg, menu_path, props, ghid_create_menu_widget, &ghidgui->topwin.menu);
}

static void ghid_update_menu_checkbox(const char *cookie)
{
	if (ghidgui->hid_active)
		ghid_update_toggle_flags(&ghidgui->topwin, cookie);
}

static int ghid_usage(const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	conf_usage("plugins/hid_gtk", pcb_hid_usage_option);
	fprintf(stderr, "\nInvocation:\n");
	fprintf(stderr, "  pcb-rnd --gui gtk2_gdk [options]\n");
	fprintf(stderr, "  pcb-rnd --gui gtk2_gl [options]\n");
	fprintf(stderr, "  pcb-rnd --gui gtk3_cairo [options]\n");
	fprintf(stderr, "  (depending on which gtk plugin(s) are compiled and installed)\n");
	return 0;
}

static const char *ghid_command_entry(const char *ovr, int *cursor)
{
	return pcb_gtk_cmd_command_entry(&ghidgui->topwin.cmd, ovr, cursor);
}

static int ghid_clip_set(pcb_hid_clipfmt_t format, const void *data, size_t len)
{
	GtkClipboard *cbrd = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

	switch(format) {
		case PCB_HID_CLIPFMT_TEXT:
			gtk_clipboard_set_text(cbrd, data, len);
			break;
	}
	return 0;
}



int ghid_clip_get(pcb_hid_clipfmt_t *format, void **data, size_t *len)
{
	GtkClipboard *cbrd = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

	if (gtk_clipboard_wait_is_text_available(cbrd)) {
		gchar *txt = gtk_clipboard_wait_for_text(cbrd);
		*format = PCB_HID_CLIPFMT_TEXT;
		*data = txt;
		*len = strlen(txt) + 1;
		return 0;
	}

	return -1;
}

void ghid_clip_free(pcb_hid_clipfmt_t format, void *data, size_t len)
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

static double ghid_benchmark(void)
{
	int i = 0;
	time_t start, end;
	GdkDisplay *display;
	GdkWindow *window;

	window = gtk_widget_get_window(gport->drawing_area);
	display = gtk_widget_get_display(gport->drawing_area);

	gdk_display_sync(display);
	time(&start);
	do {
		ghid_invalidate_all();
		gdk_window_process_updates(window, FALSE);
		time(&end);
		i++;
	}
	while (end - start < 10);

	return i/10.0;
}

void ghid_glue_hid_init(pcb_hid_t *dst)
{
	memset(dst, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(dst);
	pcb_dhlp_draw_helpers_init(dst);

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
	dst->get_view_size = ghid_get_view_size;
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

	dst->beep = ghid_beep;
	dst->edit_attributes = ghid_attributes;
	dst->point_cursor = PointCursor;
	dst->benchmark = ghid_benchmark;

	dst->notify_save_pcb = ghid_notify_save_pcb;
	dst->notify_filename_changed = ghid_notify_filename_changed;

	dst->command_entry = ghid_command_entry;

	dst->create_menu = ghid_create_menu;
	dst->remove_menu = ghid_remove_menu;
	dst->remove_menu_node = ghid_remove_menu_node;
	dst->update_menu_checkbox = ghid_update_menu_checkbox;

	dst->clip_set  = ghid_clip_set;
	dst->clip_get  = ghid_clip_get;
	dst->clip_free = ghid_clip_free;

	dst->usage = ghid_usage;
}
