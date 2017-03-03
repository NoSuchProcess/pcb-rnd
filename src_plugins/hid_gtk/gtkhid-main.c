#include "config.h"
#include "conf_core.h"

#include "glue_common.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "action_helper.h"
#include "crosshair.h"
#include "error.h"
#include "draw.h"
#include "data.h"
#include "gui.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_flags.h"
#include "hid_actions.h"
#include "plug_footprint.h"
#include "plug_io.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "layer.h"
#include "compat_nls.h"
#include "layer_vis.h"

#include "gtkhid-main.h"
#include "render.h"
#include "glue_conf.h"
#include "actions.h"

#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_common/act_fileio.h"
#include "../src_plugins/lib_gtk_common/act_print.h"
#include "../src_plugins/lib_gtk_common/bu_dwg_tooltip.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/bu_layer_selector.h"
#include "../src_plugins/lib_gtk_common/bu_menu.h"
#include "../src_plugins/lib_gtk_common/ui_zoompan.h"
#include "../src_plugins/lib_gtk_common/ui_crosshair.h"
#include "../src_plugins/lib_gtk_common/util_block_hook.h"
#include "../src_plugins/lib_gtk_common/util_timer.h"
#include "../src_plugins/lib_gtk_common/util_watch.h"
#include "../src_plugins/lib_gtk_common/util_listener.h"
#include "../src_plugins/lib_gtk_common/dlg_about.h"
#include "../src_plugins/lib_gtk_common/dlg_attribute.h"
#include "../src_plugins/lib_gtk_common/dlg_command.h"
#include "../src_plugins/lib_gtk_common/dlg_confirm.h"
#include "../src_plugins/lib_gtk_common/dlg_drc.h"
#include "../src_plugins/lib_gtk_common/dlg_export.h"
#include "../src_plugins/lib_gtk_common/dlg_file_chooser.h"
#include "../src_plugins/lib_gtk_common/dlg_input.h"
#include "../src_plugins/lib_gtk_common/dlg_log.h"
#include "../src_plugins/lib_gtk_common/dlg_message.h"
#include "../src_plugins/lib_gtk_common/dlg_netlist.h"
#include "../src_plugins/lib_gtk_common/dlg_print.h"
#include "../src_plugins/lib_gtk_common/dlg_progress.h"
#include "../src_plugins/lib_gtk_common/dlg_report.h"
#include "../src_plugins/lib_gtk_common/dlg_pinout.h"
#include "../src_plugins/lib_gtk_common/dlg_search.h"
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"
#include "../src_plugins/lib_gtk_common/dlg_library.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_common/colors.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/win_place.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

#warning TODO: make this a separate object
#include "actions.c"

#warning TODO: move most of this to render
gboolean ghid_port_drawing_area_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, void * out)
{
	static gboolean first_time_done;

	gport->view.canvas_width = ev->width;
	gport->view.canvas_height = ev->height;

	if (gport->pixmap)
		gdk_pixmap_unref(gport->pixmap);

	gport->pixmap = gdk_pixmap_new(gtk_widget_get_window(widget), gport->view.canvas_width, gport->view.canvas_height, -1);
	gport->drawable = gport->pixmap;

	if (!first_time_done) {
		gport->colormap = gtk_widget_get_colormap(gport->top_window);
		if (gdk_color_parse(conf_core.appearance.color.background, &gport->bg_color))
			gdk_color_alloc(gport->colormap, &gport->bg_color);
		else
			gdk_color_white(gport->colormap, &gport->bg_color);

		if (gdk_color_parse(conf_core.appearance.color.off_limit, &gport->offlimits_color))
			gdk_color_alloc(gport->colormap, &gport->offlimits_color);
		else
			gdk_color_white(gport->colormap, &gport->offlimits_color);
		first_time_done = TRUE;
		ghid_drawing_area_configure_hook(out);
		pcb_board_changed(0);
	}
	else {
		ghid_drawing_area_configure_hook(out);
	}

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	ghid_invalidate_all();
	return 0;
}

gint ghid_port_window_enter_cb(GtkWidget * widget, GdkEventCrossing * ev, void * out_)
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

gint ghid_port_window_leave_cb(GtkWidget * widget, GdkEventCrossing * ev, void * out_)
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

gint ghid_port_window_motion_cb(GtkWidget * widget, GdkEventMotion * ev, void * out_)
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


void ghid_do_export(pcb_hid_attr_val_t * options)
{
	gtkhid_begin();

	pcb_hid_cfg_keys_init(&ghid_keymap);
	ghid_keymap.translate_key = ghid_translate_key;
	ghid_keymap.key_name = ghid_key_name;
	ghid_keymap.auto_chr = 1;
	ghid_keymap.auto_tr = hid_cfg_key_default_trans;

	ghid_create_pcb_widgets(&ghidgui->topwin, gport->top_window);

	gport->mouse.drawing_area = ghidgui->topwin.drawing_area;
	gport->drawing_area = ghidgui->topwin.drawing_area;
	gport->mouse.top_window = ghidgui->common.top_window;

#warning TODO: move this to render init
	/* Mouse and key events will need to be intercepted when PCB needs a
	   |  location from the user.
	 */
	g_signal_connect(G_OBJECT(gport->drawing_area), "scroll_event", G_CALLBACK(ghid_port_window_mouse_scroll_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "motion_notify_event", G_CALLBACK(ghid_port_window_motion_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "configure_event", G_CALLBACK(ghid_port_drawing_area_configure_event_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "enter_notify_event", G_CALLBACK(ghid_port_window_enter_cb), gport);
	g_signal_connect(G_OBJECT(gport->drawing_area), "leave_notify_event", G_CALLBACK(ghid_port_window_leave_cb), gport);

	ghid_interface_input_signals_connect();

	/* These are needed to make sure the @layerpick and @layerview menus
	 * are properly initialized and synchronized with the current PCB.
	 */
	ghid_layer_buttons_update(&ghidgui->topwin);
	ghid_main_menu_install_route_style_selector
		(GHID_MAIN_MENU(ghidgui->topwin.menu.menu_bar), GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));

	if (conf_hid_gtk.plugins.hid_gtk.listen)
		pcb_gtk_create_listener();

	ghid_notify_gui_is_up();

	pcb_event(PCB_EVENT_GUI_INIT, NULL);

	gtk_main();
	pcb_hid_cfg_keys_uninit(&ghid_keymap);
	gtkhid_end();
}

void ghid_do_exit(pcb_hid_t * hid)
{
	gtk_main_quit();
}

static void ghid_get_view_size(pcb_coord_t * width, pcb_coord_t * height)
{
	*width = gport->view.width;
	*height = gport->view.height;
}

/* ------------------------------------------------------------ */

void ghid_calibrate(double xval, double yval)
{
	printf(_("ghid_calibrate() -- not implemented\n"));
}

void ghid_set_crosshair(int x, int y, int action)
{
	int offset_x, offset_y;

	if (gport->drawing_area == NULL)
		return;

	ghid_draw_grid_local(x, y);
	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	pcb_gtk_crosshair_set(x, y, action, offset_x, offset_y, &ghidgui->topwin.cps, &gport->view);
}


int ghid_confirm_dialog(const char *msg, ...)
{
	int res;
	va_list ap;
	va_start(ap, msg);
	res = pcb_gtk_dlg_confirm_open(ghid_port.top_window, msg, ap);
	va_end(ap);
	return res;
}

int ghid_close_confirm_dialog(void)
{
	return pcb_gtk_dlg_confirm_close(ghid_port.top_window);
}

void ghid_report_dialog(const char *title, const char *msg)
{
	pcb_gtk_dlg_report(ghid_port.top_window, title, msg, FALSE);
}

char *ghid_prompt_for(const char *msg, const char *default_string)
{
	char *grv, *rv;

	grv = pcb_gtk_dlg_input(msg, default_string, GTK_WINDOW(ghid_port.top_window));

	/* can't assume the caller will do g_free() on it */
	rv = pcb_strdup(grv);
	g_free(grv);
	return rv;
}

void ghid_show_item(void *item)
{
	ghid_pinout_window_show(&ghidgui->common, (pcb_element_t *)item);
}

void ghid_beep()
{
	gdk_beep();
}

int ghid_progress(int so_far, int total, const char *message)
{
	return pcb_gtk_dlg_progress(ghid_port.top_window, so_far, total, message);
}

static char *ghid_fileselect(const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, int flags)
{
	return pcb_gtk_fileselect(ghid_port.top_window, title, descr, default_file, default_ext, history_tag, flags);
}

/* Create a new menu by path */
static void pcb_gtk_menu_create_menu(pcb_gtk_menu_ctx_t *ctx, const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
	pcb_hid_cfg_create_menu(ghidgui->topwin.ghid_cfg, menu_path, action, mnemonic, accel, tip, cookie, ghid_create_menu_widget, ctx);
}

static int ghid_remove_menu(const char *menu_path)
{
	return pcb_hid_cfg_remove_menu(ghidgui->topwin.ghid_cfg, menu_path, ghid_remove_menu_widget, NULL);
}

void ghid_create_menu(const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
	pcb_gtk_menu_create_menu(&ghidgui->topwin.menu, menu_path, action, mnemonic, accel, tip, cookie);
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

static void ghid_attributes(const char *owner, pcb_attribute_list_t * attrs)
{
	pcb_gtk_dlg_attributes(ghid_port.top_window, owner, attrs);
}


static void ghid_drc_window_append_violation_glue(pcb_drc_violation_t *violation)
{
	ghid_drc_window_append_violation(&ghidgui->common, violation);
}

static int ghid_drc_window_throw_dialog_glue()
{
	return ghid_drc_window_throw_dialog(&ghidgui->common);
}

pcb_hid_drc_gui_t ghid_drc_gui = {
	1,  /* log_drc_overview */
	0,  /* log_drc_details */
	ghid_drc_window_reset_message,
	ghid_drc_window_append_violation_glue,
	ghid_drc_window_throw_dialog_glue,
};

/* ---------------------------------------------------------------------- */

static void PointCursor(pcb_bool grabbed)
{
	if (!ghidgui)
		return;

	if (grabbed > 0)
		ghid_point_cursor(&gport->mouse);
	else
		ghid_mode_cursor(&gport->mouse, conf_core.editor.mode);
}

/* ---------------------------------------------------------------------- */

static void RouteStylesChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui || !ghidgui->topwin.route_style_selector)
		return;

	pcb_gtk_route_style_sync
		(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector),
		 conf_core.design.line_thickness, conf_core.design.via_drilling_hole, conf_core.design.via_thickness,
		 conf_core.design.clearance);

	return;
}

/* ---------------------------------------------------------------------- */

static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!gtkhid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(PCB->Name);

	if (!gport->pixmap)
		return;

	if (ghidgui->topwin.route_style_selector) {
		pcb_gtk_route_style_empty(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
		make_route_style_buttons(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
	}
	RouteStylesChanged(0, 0, NULL);

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	pcb_gtk_zoom_view_fit(&gport->view);
	ghid_sync_with_new_layout(&ghidgui->topwin);
}


static void ghid_get_coords(const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
	pcb_gtk_get_coords(&gport->mouse, &gport->view, msg, x, y);
}

void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

/*
 * We will need these for finding the windows installation
 * directory.  Without that we can't find our fonts and
 * footprint libraries.
 */
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif

pcb_hid_t ghid_hid;

static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func) (conf_native_t *))
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		conf_hid_set_cb(n, ghid_conf_id, cbs);
	}
}

static void ghid_conf_regs()
{
	static conf_hid_callbacks_t cbs_refraction, cbs_direction, cbs_fullscreen, cbs_show_sside;

	init_conf_watch(&cbs_direction, "editor/all_direction_lines", ghid_confchg_all_direction_lines);
	init_conf_watch(&cbs_refraction, "editor/line_refraction", ghid_confchg_line_refraction);
	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", ghid_confchg_fullscreen);
	init_conf_watch(&cbs_show_sside, "editor/show_solder_side", ghid_confchg_flip);
}

void hid_hid_gtk_uninit()
{
	pcb_event_unbind_allcookie(ghid_cookie);
	conf_hid_unreg(ghid_cookie);
	conf_hid_unreg(ghid_menu_cookie);
}

static int ghid_attribute_dialog_(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title,
																	const char *descr)
{
	return ghid_attribute_dialog(ghid_port.top_window, attrs, n_attrs, results, title, descr);
}

pcb_hidval_t ghid_watch_file(int fd, unsigned int condition,
								void (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								pcb_hidval_t user_data)
{
	return pcb_gtk_watch_file(&ghidgui->common, fd, condition, func, user_data);
}

pcb_hidval_t ghid_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	return pcb_gtk_add_timer(&ghidgui->common, func, milliseconds, user_data);
}

static void GhidNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!gtkhid_active)
		return;

	pcb_gtk_netlist_changed(&ghidgui->common, user_data, argc, argv);
}

void ghid_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	pcb_gtk_logv(gtkhid_active, PCB_MSG_INFO, fmt, ap);
	va_end(ap);
}

void ghid_logv(enum pcb_message_level level, const char *fmt, va_list args)
{
	pcb_gtk_logv(gtkhid_active, level, fmt, args);
}

static void ghid_notify_save_pcb(const char *filename, pcb_bool done)
{
	pcb_gtk_tw_notify_save_pcb(&ghidgui->topwin, filename, done);
}

static void ghid_notify_filename_changed()
{
	pcb_gtk_tw_notify_filename_changed(&ghidgui->topwin);
}

static int ghid_usage(const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	conf_usage("plugins/hid_gtk", pcb_hid_usage_option);
	fprintf(stderr, "\nInvocation: pcb-rnd --gui gtk [options]\n");
	return 0;
}


static void ghid_gui_sync(void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	pcb_adjust_attached_objects();
	ghid_invalidate_all();
	ghidgui->common.window_set_name_label(PCB->Name);
	ghidgui->common.set_status_line_label();
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

	ghidgui->topwin.com = &ghidgui->common;
	ghidgui->common.top_window = window = gport->top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window), "pcb-rnd");

	wplc_place(WPLC_TOP, window);

	gtk_widget_show_all(gport->top_window);
}


pcb_uninit_t hid_hid_gtk_init()
{
#ifdef WIN32
	char *tmps;
	char *share_dir;
	char *loader_cache;
	FILE *loader_file;
#endif

#ifdef WIN32
	tmps = g_win32_get_package_installation_directory(PACKAGE "-" VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE
#define REST_OF_CACHE G_DIR_SEPARATOR_S "loaders.cache"
	share_dir = (char *) malloc(strlen(tmps) + strlen(REST_OF_PATH) + 1);
	sprintf(share_dir, "%s%s", tmps, REST_OF_PATH);

	/* Point to our gdk-pixbuf loader cache.  */
	loader_cache = (char *) malloc(strlen("bindir_todo12") + strlen(REST_OF_CACHE) + 1);
	sprintf(loader_cache, "%s%s", "bindir_todo12", REST_OF_CACHE);
	loader_file = fopen(loader_cache, "r");
	if (loader_file) {
		fclose(loader_file);
		g_setenv("GDK_PIXBUF_MODULE_FILE", loader_cache, TRUE);
	}

	free(tmps);
#undef REST_OF_PATH
	printf("\"Share\" installation path is \"%s\"\n", "share_dir_todo12");
#endif

	ghid_glue_common_init();

	memset(&ghid_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&ghid_hid);
	pcb_dhlp_draw_helpers_init(&ghid_hid);

	ghid_hid.struct_size = sizeof(pcb_hid_t);
	ghid_hid.name = "gtk";
	ghid_hid.description = "Gtk - The Gimp Toolkit";
	ghid_hid.gui = 1;
	ghid_hid.poly_after = 1;

	ghid_hid.do_export = ghid_do_export;
	ghid_hid.do_exit = ghid_do_exit;
	ghid_hid.parse_arguments = ghid_parse_arguments;
	ghid_hid.invalidate_lr = ghid_invalidate_lr;
	ghid_hid.invalidate_all = ghid_invalidate_all;
	ghid_hid.notify_crosshair_change = ghid_notify_crosshair_change;
	ghid_hid.notify_mark_change = ghid_notify_mark_change;
	ghid_hid.set_layer_group = ghid_set_layer_group;
	ghid_hid.make_gc = ghid_make_gc;
	ghid_hid.destroy_gc = ghid_destroy_gc;
	ghid_hid.use_mask = ghid_use_mask;
	ghid_hid.set_color = ghid_set_color;
	ghid_hid.set_line_cap = ghid_set_line_cap;
	ghid_hid.set_line_width = ghid_set_line_width;
	ghid_hid.set_draw_xor = ghid_set_draw_xor;
	ghid_hid.draw_line = ghid_draw_line;
	ghid_hid.draw_arc = ghid_draw_arc;
	ghid_hid.draw_rect = ghid_draw_rect;
	ghid_hid.fill_circle = ghid_fill_circle;
	ghid_hid.fill_polygon = ghid_fill_polygon;
	ghid_hid.fill_rect = ghid_fill_rect;

	ghid_hid.calibrate = ghid_calibrate;
	ghid_hid.shift_is_pressed = ghid_shift_is_pressed;
	ghid_hid.control_is_pressed = ghid_control_is_pressed;
	ghid_hid.mod1_is_pressed = ghid_mod1_is_pressed;
	ghid_hid.get_coords = ghid_get_coords;
	ghid_hid.get_view_size = ghid_get_view_size;
	ghid_hid.set_crosshair = ghid_set_crosshair;
	ghid_hid.add_timer = ghid_add_timer;
	ghid_hid.stop_timer = ghid_stop_timer;
	ghid_hid.watch_file = ghid_watch_file;
	ghid_hid.unwatch_file = pcb_gtk_unwatch_file;
	ghid_hid.add_block_hook = ghid_add_block_hook;
	ghid_hid.stop_block_hook = ghid_stop_block_hook;

	ghid_hid.log = ghid_log;
	ghid_hid.logv = ghid_logv;
	ghid_hid.confirm_dialog = ghid_confirm_dialog;
	ghid_hid.close_confirm_dialog = ghid_close_confirm_dialog;
	ghid_hid.report_dialog = ghid_report_dialog;
	ghid_hid.prompt_for = ghid_prompt_for;
	ghid_hid.fileselect = ghid_fileselect;
	ghid_hid.attribute_dialog = ghid_attribute_dialog_;
	ghid_hid.show_item = ghid_show_item;
	ghid_hid.beep = ghid_beep;
	ghid_hid.progress = ghid_progress;
	ghid_hid.drc_gui = &ghid_drc_gui;
	ghid_hid.edit_attributes = ghid_attributes;
	ghid_hid.point_cursor = PointCursor;

	ghid_hid.request_debug_draw = ghid_request_debug_draw;
	ghid_hid.flush_debug_draw = ghid_flush_debug_draw;
	ghid_hid.finish_debug_draw = ghid_finish_debug_draw;

	ghid_hid.notify_save_pcb = ghid_notify_save_pcb;
	ghid_hid.notify_filename_changed = ghid_notify_filename_changed;

	ghid_hid.propedit_start = ghid_propedit_start;
	ghid_hid.propedit_end = ghid_propedit_end;
	ghid_hid.propedit_add_stat = ghid_propedit_add_stat;
/*	ghid_hid.propedit_add_prop = ghid_propedit_add_prop;*/
/*	ghid_hid.propedit_add_value = ghid_propedit_add_value;*/

	pcb_gtk_conf_init();
	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(ghid_menu_cookie, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
	ghid_conf_regs();

	ghid_hid.create_menu = ghid_create_menu;
	ghid_hid.remove_menu = ghid_remove_menu;

	ghid_hid.usage = ghid_usage;

	pcb_hid_register_hid(&ghid_hid);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_hid_gtk, field,isarray,type_name,cpath,cname,desc,flags);
#include "../src_plugins/lib_gtk_config/hid_gtk_conf_fields.h"

	pcb_event_bind(PCB_EVENT_SAVE_PRE, ghid_conf_save_pre_wgeo, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_LOAD_POST, ghid_conf_load_post_wgeo, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, GhidNetlistChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, RouteStylesChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, ghid_cookie);

	return hid_hid_gtk_uninit;
}

#warning TODO: remove this
int gtkhid_active = 0;


void gtkhid_begin(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_cookie)
	PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_cookie)
	gtkhid_active = 1;
}

void gtkhid_end(void)
{
	pcb_hid_remove_actions_by_cookie(ghid_cookie);
	pcb_hid_remove_attributes_by_cookie(ghid_cookie);
	gtkhid_active = 0;
}
