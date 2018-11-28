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
#include "../src_plugins/lib_gtk_common/dlg_confirm.h"
#include "../src_plugins/lib_gtk_common/dlg_input.h"
#include "../src_plugins/lib_gtk_common/dlg_log.h"
#include "../src_plugins/lib_gtk_common/dlg_file_chooser.h"
#include "../src_plugins/lib_gtk_common/dlg_report.h"
#include "../src_plugins/lib_gtk_common/dlg_progress.h"
#include "../src_plugins/lib_gtk_common/dlg_attribute.h"
#include "../src_plugins/lib_gtk_common/dlg_attributes.h"
#include "../src_plugins/lib_gtk_common/util_listener.h"
#include "../src_plugins/lib_gtk_common/util_timer.h"
#include "../src_plugins/lib_gtk_common/util_watch.h"
#include "../src_plugins/lib_gtk_common/util_block_hook.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"

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

	/* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* See comment in ghid_port_window_leave_cb() */

	if (ev->mode != GDK_CROSSING_NORMAL && ev->detail != GDK_NOTIFY_NONLINEAR) {
		return FALSE;
	}

	if (!ghidgui->topwin.cmd.command_entry_status_line_active) {
		out->view.has_entered = TRUE;
		/* Make sure drawing area has keyboard focus when we are in it.
		 */
		gtk_widget_grab_focus(out->drawing_area);
	}
	ghidgui->topwin.in_popup = FALSE;

	/* Following expression is true if a you open a menu from the menu bar,
	 * move the mouse to the viewport and click on it. This closes the menu
	 * and moves the pointer to the viewport without the pointer going over
	 * the edge of the viewport */
	if (ev->mode == GDK_CROSSING_UNGRAB && ev->detail == GDK_NOTIFY_NONLINEAR) {
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

#warning TODO: move this to render init
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

	ghid_config_init();

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

	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	wplc_place(WPLC_TOP, window);

	gtk_widget_show_all(gport->top_window);
	return 0;
}

static void ghid_calibrate(double xval, double yval)
{
	printf(_("ghid_calibrate() -- not implemented\n"));
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

static void ghid_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	pcb_gtk_logv(ghidgui->hid_active, PCB_MSG_INFO, fmt, ap);
	va_end(ap);
}

static void ghid_logv(enum pcb_message_level level, const char *fmt, va_list args)
{
	pcb_gtk_logv(ghidgui->hid_active, level, fmt, args);
}

static int ghid_confirm_dialog(const char *msg, ...)
{
	int res;
	va_list ap;
	va_start(ap, msg);
	res = pcb_gtk_dlg_confirm_open(ghid_port.top_window, msg, ap);
	va_end(ap);
	return res;
}

static int ghid_close_confirm_dialog(void)
{
	return pcb_gtk_dlg_confirm_close(ghid_port.top_window);
}

static void ghid_report_dialog(const char *title, const char *msg)
{
	pcb_gtk_dlg_report(ghid_port.top_window, title, msg, FALSE);
}

static char *ghid_prompt_for(const char *msg, const char *default_string)
{
	char *grv, *rv;

	grv = pcb_gtk_dlg_input(msg, default_string, GTK_WINDOW(ghid_port.top_window));

	if (grv == NULL)
		return NULL;

	/* can't assume the caller will do g_free() on it */
	rv = pcb_strdup(grv);
	g_free(grv);
	return rv;
}

static char *ghid_fileselect(const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, int flags)
{
	return pcb_gtk_fileselect(ghid_port.top_window, title, descr, default_file, default_ext, history_tag, flags);
}

static int ghid_attribute_dialog_(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data)
{
	return ghid_attribute_dialog(&ghidgui->common, attrs, n_attrs, results, title, descr, caller_data);
}

static void *ghid_attr_dlg_new_(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
{
	return ghid_attr_dlg_new(&ghidgui->common, attrs, n_attrs, results, title, descr, caller_data, modal, button_cb);
}

static void ghid_beep()
{
	gdk_beep();
}

static int ghid_progress(int so_far, int total, const char *message)
{
	return pcb_gtk_dlg_progress(ghid_port.top_window, so_far, total, message);
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

static int ghid_propedit_start(void *pe, int num_props, const char *(*query) (void *pe, const char *cmd, const char *key, const char *val, int idx))
{
	ghidgui->propedit_widget = pcb_gtk_dlg_propedit_create(&ghidgui->propedit_dlg, &ghidgui->common);
	ghidgui->propedit_dlg.propedit_query = query;
	ghidgui->propedit_dlg.propedit_pe = pe;
	return 0;
}

static void ghid_propedit_end(void *pe)
{
	if (gtk_dialog_run(GTK_DIALOG(ghidgui->propedit_widget)) == GTK_RESPONSE_OK) {
	}
	gtk_widget_destroy(ghidgui->propedit_widget);
}

static void ghid_propedit_add_stat(void *pe, const char *propname, void *propctx, const char *most_common, const char *min, const char *max, const char *avg)
{
	pcb_gtk_dlg_propedit_prop_add(&ghidgui->propedit_dlg, propname, most_common, min, max, avg);
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

void ghid_glue_hid_init(pcb_hid_t *dst)
{
	memset(dst, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(dst);
	pcb_dhlp_draw_helpers_init(dst);

	dst->struct_size = sizeof(pcb_hid_t);
	dst->gui = 1;
	dst->heavy_term_layer_ind = 1;

	dst->do_export = gtkhid_do_export;
	dst->do_exit = ghid_do_exit;
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
	dst->add_block_hook = ghid_add_block_hook;
	dst->stop_block_hook = ghid_stop_block_hook;

	dst->log = ghid_log;
	dst->logv = ghid_logv;
	dst->confirm_dialog = ghid_confirm_dialog;
	dst->close_confirm_dialog = ghid_close_confirm_dialog;
	dst->report_dialog = ghid_report_dialog;
	dst->prompt_for = ghid_prompt_for;
	dst->fileselect = ghid_fileselect;
	dst->attribute_dialog = ghid_attribute_dialog_;
	dst->attr_dlg_new = ghid_attr_dlg_new_;
	dst->attr_dlg_run = ghid_attr_dlg_run;
	dst->attr_dlg_free = ghid_attr_dlg_free;
	dst->attr_dlg_property = ghid_attr_dlg_property;
	dst->attr_dlg_widget_state = ghid_attr_dlg_widget_state;
	dst->attr_dlg_widget_hide = ghid_attr_dlg_widget_hide;
	dst->attr_dlg_set_value = ghid_attr_dlg_set_value;
	dst->beep = ghid_beep;
	dst->progress = ghid_progress;
	dst->edit_attributes = ghid_attributes;
	dst->point_cursor = PointCursor;

	dst->notify_save_pcb = ghid_notify_save_pcb;
	dst->notify_filename_changed = ghid_notify_filename_changed;

	dst->propedit_start = ghid_propedit_start;
	dst->propedit_end = ghid_propedit_end;
	dst->propedit_add_stat = ghid_propedit_add_stat;
/*	dst->propedit_add_prop = ghid_propedit_add_prop;*/
/*	dst->propedit_add_value = ghid_propedit_add_value;*/
	dst->command_entry = ghid_command_entry;

	dst->create_menu = ghid_create_menu;
	dst->remove_menu = ghid_remove_menu;
	dst->remove_menu_node = ghid_remove_menu_node;
	dst->update_menu_checkbox = ghid_update_menu_checkbox;

	dst->usage = ghid_usage;
}
