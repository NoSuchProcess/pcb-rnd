#include "config.h"
#include "conf_core.h"

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
#include "gui-top-window.h"
#include "render.h"

#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_common/act_fileio.h"
#include "../src_plugins/lib_gtk_common/act_print.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
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
#include "../src_plugins/lib_gtk_common/dlg_library.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_common/colors.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";

void ghid_do_export(pcb_hid_attr_val_t * options)
{
	gtkhid_begin();

	pcb_hid_cfg_keys_init(&ghid_keymap);
	ghid_keymap.translate_key = ghid_translate_key;
	ghid_keymap.key_name = ghid_key_name;
	ghid_keymap.auto_chr = 1;
	ghid_keymap.auto_tr = hid_cfg_key_default_trans;

	ghid_create_pcb_widgets(&ghidgui->topwin, gport->top_window);

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

void ghid_pan_common(void)
{
	ghidgui->topwin.adjustment_changed_holdoff = TRUE;
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.h_range), gport->view.x0);
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.v_range), gport->view.y0);
	ghidgui->topwin.adjustment_changed_holdoff = FALSE;

	ghid_port_ranges_changed(&ghidgui->topwin);
}

void ghid_port_button_press_main(void)
{
	ghid_invalidate_all();
	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	if (!gport->view.panning)
		g_idle_add(ghid_idle_cb, NULL);
}

void ghid_port_button_release_main(void)
{
	pcb_adjust_attached_objects();
	ghid_invalidate_all();

	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	g_idle_add(ghid_idle_cb, NULL);
}

void ghid_mode_cursor_main(int mode)
{
	ghid_mode_cursor(&gport->mouse, mode);
}


/* ------------------------------------------------------------ */

void ghid_calibrate(double xval, double yval)
{
	printf(_("ghid_calibrate() -- not implemented\n"));
}

static int ghid_gui_is_up = 0;

void ghid_notify_gui_is_up(void)
{
	ghid_gui_is_up = 1;
}

int ghid_shift_is_pressed()
{
	GdkModifierType mask;
	GHidPort *out = &ghid_port;

	if (!ghid_gui_is_up)
		return 0;

	gdk_window_get_pointer(gtk_widget_get_window(out->drawing_area), NULL, NULL, &mask);

#ifdef PCB_WORKAROUND_GTK_SHIFT
	/* On some systems the above query fails and we need to return the last known state instead */
	return pcb_gtk_glob_mask & GDK_SHIFT_MASK;
#else
	return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
#endif
}

int ghid_control_is_pressed()
{
	GdkModifierType mask;
	GHidPort *out = &ghid_port;

	if (!ghid_gui_is_up)
		return 0;

	gdk_window_get_pointer(gtk_widget_get_window(out->drawing_area), NULL, NULL, &mask);

#ifdef PCB_WORKAROUND_GTK_CTRL
	/* On some systems the above query fails and we need to return the last known state instead */
	return pcb_gtk_glob_mask & GDK_CONTROL_MASK;
#else
	return (mask & GDK_CONTROL_MASK) ? TRUE : FALSE;
#endif
}

int ghid_mod1_is_pressed()
{
	GdkModifierType mask;
	GHidPort *out = &ghid_port;

	if (!ghid_gui_is_up)
		return 0;

	gdk_window_get_pointer(gtk_widget_get_window(out->drawing_area), NULL, NULL, &mask);
#ifdef __APPLE__
	return (mask & (1 << 13)) ? TRUE : FALSE;	/* The option key is not MOD1, although it should be... */
#else
	return (mask & GDK_MOD1_MASK) ? TRUE : FALSE;
#endif
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

/* ------------------------------------------------------------
 *
 * Actions specific to the GTK HID follow from here
 *
 */

/* ------------------------------------------------------------ */
static const char about_syntax[] = "About()";

static const char about_help[] = N_("Tell the user about this version of PCB.");

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int About(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_gtk_dlg_about(gport->top_window);
	return 0;
}

/* ------------------------------------------------------------ */
static const char getxy_syntax[] = "GetXY()";

static const char getxy_help[] = N_("Get a coordinate.");

/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */

static int GetXY(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

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
		ghid_window_set_name_label(PCB->Name);

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

/* ---------------------------------------------------------------------- */

static int LayerGroupsChanged(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	printf(_("LayerGroupsChanged -- not implemented\n"));
	return 0;
}

/* ---------------------------------------------------------------------- */

static int Command(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	ghid_handle_user_command(&ghidgui->cmd, TRUE);
	return 0;
}

/* ------------------------------------------------------------ */

int pcb_gtk_act_print_(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_print(gport->top_window, argc, argv, x, y);
}

/* ------------------------------------------------------------ */

static int ExportGUI(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{

	/* check if layout is empty */
	if (!pcb_data_is_empty(PCB->Data)) {
		ghid_dialog_export(ghid_port.top_window);
	}
	else
		pcb_gui->log(_("Can't export empty layout"));

	return 0;
}

/* ------------------------------------------------------------ */

static int Benchmark(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i = 0;
	time_t start, end;
	GdkDisplay *display;

	display = gdk_drawable_get_display(gport->drawable);

	gdk_display_sync(display);
	time(&start);
	do {
		ghid_invalidate_all();
		gdk_window_process_updates(gtk_widget_get_window(gport->drawing_area), FALSE);
		time(&end);
		i++;
	}
	while (end - start < 10);

	printf(_("%g redraws per second\n"), i / 10.0);

	return 0;
}

/* ------------------------------------------------------------ */

static const char dowindows_syntax[] =
	"DoWindows(1|2|3|4|5|6|7,[false])\n" "DoWindows(Layout|Library|Log|Netlist|Preferences|DRC,[false])";

static const char dowindows_help[] = N_("Open various GUI windows. With false, do not raise the window (no focus stealing).");

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@item 5
@itemx Preferences
Open the preferences window.

@item 6
@itemx DRC
Open the DRC violations window.

@item 7
@itemx Search
Open the advanced search window.

@end table

%end-doc */

static int DoWindows(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *a = argc >= 1 ? argv[0] : "";
	gboolean raise = TRUE;

	if (argc >= 2) {
		char c = tolower((argv[1])[0]);
		if ((c == 'n') || (c == 'f') || (c == '0'))
			raise = FALSE;
	}

	if (strcmp(a, "1") == 0 || pcb_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || pcb_strcasecmp(a, "Library") == 0) {
		pcb_gtk_library_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		pcb_gtk_dlg_log_show(raise);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		pcb_gtk_dlg_netlist_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "5") == 0 || pcb_strcasecmp(a, "Preferences") == 0) {
		ghid_config_window_show(&ghidgui->common);
	}
	else if (strcmp(a, "6") == 0 || pcb_strcasecmp(a, "DRC") == 0) {
		ghid_drc_window_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "7") == 0 || pcb_strcasecmp(a, "search") == 0) {
		ghid_search_window_show(gport->top_window, raise);
	}
	else {
		PCB_AFAIL(dowindows);
	}

	return 0;
}

/* ------------------------------------------------------------ */
static const char setunits_syntax[] = "SetUnits(mm|mil)";

static const char setunits_help[] = N_("Set the default measurement units.");

/* %start-doc actions SetUnits

@table @code

@item mil
Sets the display units to mils (1/1000 inch).

@item mm
Sets the display units to millimeters.

@end table

%end-doc */

static int SetUnits(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const pcb_unit_t *new_unit;
	if (argc == 0)
		return 0;

	new_unit = get_unit_struct(argv[0]);
	if (new_unit != NULL && new_unit->allow != PCB_UNIT_NO_PRINT) {
		conf_set(CFR_DESIGN, "editor/grid_unit", -1, argv[0], POL_OVERWRITE);
		pcb_attrib_put(PCB, "PCB::grid::unit", argv[0]);
	}

	ghid_handle_units_changed(&ghidgui->topwin);

	ghid_set_status_line_label();

	/* FIXME ?
	 * lesstif_sizes_reset ();
	 * lesstif_styles_update_values ();
	 */
	return 0;
}

/* ------------------------------------------------------------ */
static const char popup_syntax[] = "Popup(MenuName, [Button])";

static const char popup_help[] =
N_("Bring up the popup menu specified by @code{MenuName}.\n"
	 "If called by a mouse event then the mouse button number\n" "must be specified as the optional second argument.");

/* %start-doc actions Popup

This just pops up the specified menu.  The menu must have been defined
in the popups subtree in the menu lht file.

%end-doc */
static int Popup(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	GtkMenu *menu = NULL;
	char name[256];

	if (argc != 1 && argc != 2)
		PCB_AFAIL(popup);

	if (strlen(argv[0]) < sizeof(name) - 32) {
		lht_node_t *menu_node;
		sprintf(name, "/popups/%s", argv[0]);
		menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);
		if (menu_node != NULL)
			menu = menu_node->user_data;
	}

	if (!GTK_IS_MENU(menu)) {
		pcb_message(PCB_MSG_ERROR, _("The specified popup menu \"%s\" has not been defined.\n"), argv[0]);
		return 1;
	}
	else {
		ghidgui->topwin.in_popup = TRUE;
		gtk_widget_grab_focus(ghid_port.drawing_area);
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}
	return 0;
}

/* ------------------------------------------------------------ */
static const char savewingeo_syntax[] = "SaveWindowGeometry()";

static const char savewingeo_help[] = N_("Saves window geometry in the config.\n");

static int SaveWinGeo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	ghid_wgeo_save(1, 0);
	return 0;
}


/* ------------------------------------------------------------ */
static void ghid_Busy(void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghid_watch_cursor(&gport->mouse);
}

static int Zoom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_zoom(&gport->view, argc, argv, x, y);
}

int Center(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int offset_x, offset_y, pointer_x, pointer_y;
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	pcb_gtk_act_center(&gport->view, argc, argv, x, y, offset_x, offset_y, &pointer_x, &pointer_y);
	gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);
	return 0;
}

static int SwapSides(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_swap_sides(&gport->view, argc, argv, x, y);
}

static int ScrollAction(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_scroll(&gport->view, argc, argv, x, y);
}

static int PanAction(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_pan(&gport->view, argc, argv, x, y);
}

static int GhidNetlistShow(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_netlistshow(&ghidgui->common, argc, argv, x, y);
}

static int GhidNetlistPresent(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_netlistpresent(&ghidgui->common, argc, argv, x, y);
}

static void ghid_get_coords(const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
	pcb_gtk_get_coords(&gport->mouse, &gport->view, msg, x, y);
}

int act_load(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_load(ghid_port.top_window, argc, argv, x, y);
}

int act_save(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_save(ghid_port.top_window, argc, argv, x, y);
}

int act_importgui(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_act_importgui(ghid_port.top_window, argc, argv, x, y);
}

void ghid_status_line_set_text(const gchar *text)
{
	if (!ghidgui->cmd.command_entry_status_line_active)
		ghid_status_line_set_text_(ghidgui->topwin.status_line_label, text);
}

void ghid_set_status_line_label(void)
{
	if (!ghidgui->cmd.command_entry_status_line_active) \
		ghid_set_status_line_label_(ghidgui->topwin.status_line_label, conf_hid_gtk.plugins.hid_gtk.compact_horizontal); \
}

void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

pcb_hid_action_t ghid_main_action_list[] = {
	{"About", 0, About, about_help, about_syntax}
	,
	{"Benchmark", 0, Benchmark}
	,
	{"Center", N_("Click on a location to center"), Center, pcb_acth_center, pcb_acts_center}
	,
	{"Command", 0, Command}
	,
	{"DoWindows", 0, DoWindows, dowindows_help, dowindows_syntax}
	,
	{"ExportGUI", 0, ExportGUI}
	,
	{"GetXY", "", GetXY, getxy_help, getxy_syntax}
	,
	{"ImportGUI", 0, act_importgui, pcb_gtk_acth_importgui, pcb_gtk_acts_importgui}
	,
	{"LayerGroupsChanged", 0, LayerGroupsChanged}
	,
	{"Load", 0, act_load }
	,
	{"LogShowOnAppend", 0, pcb_gtk_act_logshowonappend, pcb_gtk_acth_logshowonappend, pcb_gtk_acts_logshowonappend}
	,
	{"NetlistShow", 0, GhidNetlistShow, pcb_gtk_acth_netlistshow, pcb_gtk_acts_netlistshow}
	,
	{"NetlistPresent", 0, GhidNetlistPresent, pcb_gtk_acth_netlistpresent, pcb_gtk_acts_netlistpresent}
	,
	{"Pan", 0, PanAction, pcb_acth_pan, pcb_acts_pan}
	,
	{"Popup", 0, Popup, popup_help, popup_syntax}
	,
	{"Print", 0, pcb_gtk_act_print_, pcb_gtk_acth_print, pcb_gtk_acts_print}
	,
	{"PrintCalibrate", 0, pcb_gtk_act_printcalibrate, pcb_gtk_acth_printcalibrate, pcb_gtk_acts_printcalibrate}
	,
	{"Save", 0, act_save, pcb_gtk_acth_save, pcb_gtk_acts_save}
	,
	{"SaveWindowGeometry", 0, SaveWinGeo, savewingeo_help, savewingeo_syntax}
	,
	{"Scroll", N_("Click on a place to scroll"), ScrollAction, pcb_acth_scroll, pcb_acts_scroll}
	,
	{"SetUnits", 0, SetUnits, setunits_help, setunits_syntax}
	,
	{"SwapSides", 0, SwapSides, pcb_acth_swapsides, pcb_acts_swapsides}
	,
	{"Zoom", N_("Click on zoom focus"), Zoom, pcb_acth_zoom, pcb_acts_zoom}
};

PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_cookie)
#include "dolists.h"
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

int ghid_command_entry_is_active(void)
{
	return ghidgui->cmd.command_entry_status_line_active;
}

static void command_pack_in_status_line(void)
{
	gtk_box_pack_start(GTK_BOX(ghidgui->topwin.status_line_hbox), ghidgui->cmd.command_combo_box, FALSE, FALSE, 0);
}

static void ghid_interface_set_sensitive(gboolean sensitive)
{
	pcb_gtk_tw_interface_set_sensitive(&ghidgui->topwin, sensitive);
}

static void command_post_entry(void)
{
	ghid_interface_input_signals_connect();
	ghid_interface_set_sensitive(TRUE);
	ghid_install_accel_groups(GTK_WINDOW(gport->top_window), &ghidgui->topwin);
	gtk_widget_grab_focus(gport->drawing_area);
}

static void command_pre_entry(void)
{
	ghid_remove_accel_groups(GTK_WINDOW(gport->top_window), &ghidgui->topwin);
	ghid_interface_input_signals_disconnect();
	ghid_interface_set_sensitive(FALSE);
}

	/* If conf_hid_gtk.plugins.hid_gtk.use_command_window toggles, the config code calls
	   |  this to ensure the command_combo_box is set up for living in the
	   |  right place.
	 */
static void command_use_command_window_sync(pcb_gtk_command_t *ctx)
{
	/* The combo box will be NULL and not living anywhere until the
	   |  first command entry.
	 */
	if (!ctx->command_combo_box)
		return;

	if (conf_hid_gtk.plugins.hid_gtk.use_command_window)
		gtk_container_remove(GTK_CONTAINER(ghidgui->topwin.status_line_hbox), ctx->command_combo_box);
	else {
		/* Destroy the window (if it's up) which floats the command_combo_box
		   |  so we can pack it back into the status line hbox.  If the window
		   |  wasn't up, the command_combo_box was already floating.
		 */
		command_window_close_cb(ctx);
		gtk_widget_hide(ctx->command_combo_box);
		command_pack_in_status_line();
	}
}

void ghid_command_use_command_window_sync(void)
{
	command_use_command_window_sync(&ghidgui->cmd);
}

static void LayersChanged_cb(void)
{
	ghid_LayersChanged(0, 0, 0);
}

void ghid_pack_mode_buttons(void)
{
	pcb_gtk_pack_mode_buttons(&ghidgui->topwin.mode_btn);
}

static void ghid_notify_save_pcb(const char *filename, pcb_bool done)
{
	pcb_gtk_tw_notify_save_pcb(&ghidgui->topwin, filename, done);
}

static void ghid_notify_filename_changed()
{
	pcb_gtk_tw_notify_filename_changed(&ghidgui->topwin);
}

static void ghid_port_ranges_scale(void)
{
	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
}

void ghid_window_set_name_label(gchar *name)
{
	pcb_gtk_tw_window_set_name_label(&ghidgui->topwin, name);
}

static void ghid_layer_buttons_color_update(void)
{
	pcb_gtk_tw_layer_buttons_color_update(&ghidgui->topwin);
}

static void ghid_route_styles_edited_cb()
{
	pcb_gtk_tw_route_styles_edited_cb(&ghidgui->topwin);
}

static void ghid_gui_sync(void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	pcb_adjust_attached_objects();
	ghid_invalidate_all();
	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
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

	/* Set up the glue struct to lib_gtk_common */
	ghidgui->common.gport = &ghid_port;
	ghidgui->common.render_pixmap = ghid_render_pixmap;
	ghidgui->common.init_drawing_widget = ghid_init_drawing_widget;
	ghidgui->common.preview_expose = ghid_preview_expose;
	ghidgui->common.window_set_name_label = ghid_window_set_name_label;
	ghidgui->common.set_status_line_label = ghid_set_status_line_label;
	ghidgui->common.note_event_location = ghid_note_event_location;
	ghidgui->common.idle_cb = ghid_idle_cb;
	ghidgui->common.shift_is_pressed = ghid_shift_is_pressed;
	ghidgui->common.interface_input_signals_disconnect = ghid_interface_input_signals_disconnect;
	ghidgui->common.interface_input_signals_connect = ghid_interface_input_signals_connect;
	ghidgui->common.interface_set_sensitive = ghid_interface_set_sensitive;
	ghidgui->common.port_button_press_main = ghid_port_button_press_main;
	ghidgui->common.port_button_release_main = ghid_port_button_release_main;
	ghidgui->common.status_line_set_text = ghid_status_line_set_text;
	ghidgui->common.route_styles_edited_cb = ghid_route_styles_edited_cb;
	ghidgui->common.mode_cursor_main = ghid_mode_cursor_main;
	ghidgui->common.invalidate_all = ghid_invalidate_all;
	ghidgui->common.cancel_lead_user = ghid_cancel_lead_user;
	ghidgui->common.lead_user_to_location = ghid_lead_user_to_location;
	ghidgui->common.pan_common = ghid_pan_common;
	ghidgui->common.port_ranges_scale = ghid_port_ranges_scale;
	ghidgui->common.preview_draw = ghid_preview_draw;
	ghidgui->common.pack_mode_buttons = ghid_pack_mode_buttons;
	ghidgui->common.get_color_name = ghid_get_color_name;
	ghidgui->common.map_color_string = ghid_map_color_string;
	ghidgui->common.set_special_colors = ghid_set_special_colors;
	ghidgui->common.layer_buttons_color_update = ghid_layer_buttons_color_update;
	ghidgui->common.LayersChanged = LayersChanged_cb;
	ghidgui->common.command_entry_is_active = ghid_command_entry_is_active;
	ghidgui->common.command_use_command_window_sync = ghid_command_use_command_window_sync;

	ghidgui->cmd.com = &ghidgui->common;
	ghidgui->cmd.pack_in_status_line = command_pack_in_status_line;
	ghidgui->cmd.post_entry = command_post_entry;
	ghidgui->cmd.pre_entry = command_pre_entry;


	ghid_port.view.com = &ghidgui->common;
	ghid_port.mouse.com = &ghidgui->common;


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

int gtkhid_active = 0;

void gtkhid_begin(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_cookie)
	PCB_REGISTER_ACTIONS(gtk_topwindow_action_list, ghid_cookie)
	PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_cookie)
	gtkhid_active = 1;
}

void gtkhid_end(void)
{
	pcb_hid_remove_actions_by_cookie(ghid_cookie);
	pcb_hid_remove_attributes_by_cookie(ghid_cookie);
	gtkhid_active = 0;
}
