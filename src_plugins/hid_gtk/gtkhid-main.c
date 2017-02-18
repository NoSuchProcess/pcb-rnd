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
#include "ghid-main-menu.h"
#include "gui-drc-window.h"

/* AV: Care to circular includes !!!? */
#include "../src_plugins/lib_gtk_common/act_fileio.h"
#include "../src_plugins/lib_gtk_common/act_print.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/ui_zoompan.h"
#include "../src_plugins/lib_gtk_common/util_block_hook.h"
#include "../src_plugins/lib_gtk_common/util_timer.h"
#include "../src_plugins/lib_gtk_common/util_watch.h"
#include "../src_plugins/lib_gtk_common/dlg_about.h"
#include "../src_plugins/lib_gtk_common/dlg_attribute.h"
#include "../src_plugins/lib_gtk_common/dlg_confirm.h"
#include "../src_plugins/lib_gtk_common/dlg_export.h"
#include "../src_plugins/lib_gtk_common/dlg_file_chooser.h"
#include "../src_plugins/lib_gtk_common/dlg_input.h"
#include "../src_plugins/lib_gtk_common/dlg_message.h"
#include "../src_plugins/lib_gtk_common/dlg_print.h"
#include "../src_plugins/lib_gtk_common/dlg_progress.h"
#include "../src_plugins/lib_gtk_common/dlg_report.h"
#include "../src_plugins/lib_gtk_common/dlg_pinout.h"
#include "../src_plugins/lib_gtk_common/dlg_search.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";

conf_hid_id_t ghid_menuconf_id = -1;
GdkModifierType ghid_glob_mask;

static void ghid_get_view_size(pcb_coord_t * width, pcb_coord_t * height)
{
	*width = gport->view.width;
	*height = gport->view.height;
}

void ghid_pan_common(void)
{
	ghidgui->adjustment_changed_holdoff = TRUE;
	gtk_range_set_value(GTK_RANGE(ghidgui->h_range), gport->view.x0);
	gtk_range_set_value(GTK_RANGE(ghidgui->v_range), gport->view.y0);
	ghidgui->adjustment_changed_holdoff = FALSE;

	ghid_port_ranges_changed();
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
	return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
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
	return ghid_glob_mask & GDK_CONTROL_MASK;
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
	GdkDisplay *display;
	GdkScreen *screen;
	int offset_x, offset_y;
	int widget_x, widget_y;
	int pointer_x, pointer_y;
	pcb_coord_t pcb_x, pcb_y;

	ghid_draw_grid_local(x, y);

	if (gport->view.crosshair_x != x || gport->view.crosshair_y != y) {
		ghid_set_cursor_position_labels(&ghidgui->cps, conf_hid_gtk.plugins.hid_gtk.compact_vertical);
		gport->view.crosshair_x = x;
		gport->view.crosshair_y = y;

		/* FIXME - does this trigger the idle_proc stuff?  It is in the
		 * lesstif HID.  Maybe something is needed here?
		 *
		 * need_idle_proc ();
		 */
	}

	if (action != HID_SC_PAN_VIEWPORT && action != HID_SC_WARP_POINTER)
		return;

	/* Find out where the drawing area is on the screen. gdk_display_get_pointer
	 * and gdk_display_warp_pointer work relative to the whole display, whilst
	 * our coordinates are relative to the drawing area origin.
	 */
	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	display = gdk_display_get_default();

	switch (action) {
	case HID_SC_PAN_VIEWPORT:
		/* Pan the board in the viewport so that the crosshair (who's location
		 * relative on the board was set above) lands where the pointer is.
		 * We pass the request to pan a particular point on the board to a
		 * given widget coordinate of the viewport into the rendering code
		 */

		/* Find out where the pointer is relative to the display */
		gdk_display_get_pointer(display, NULL, &pointer_x, &pointer_y, NULL);

		widget_x = pointer_x - offset_x;
		widget_y = pointer_y - offset_y;

		pcb_gtk_coords_event2pcb(&gport->view, widget_x, widget_y, &pcb_x, &pcb_y);
		pcb_gtk_pan_view_abs(&gport->view, pcb_x, pcb_y, widget_x, widget_y);

		/* Just in case we couldn't pan the board the whole way,
		 * we warp the pointer to where the crosshair DID land.
		 */
		/* Fall through */

	case HID_SC_WARP_POINTER:
		screen = gdk_display_get_default_screen(display);

		pcb_gtk_coords_pcb2event(&gport->view, x, y, &widget_x, &widget_y);

		pointer_x = offset_x + widget_x;
		pointer_y = offset_y + widget_y;

		gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);

		break;
	}
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
	ghid_pinout_window_show(&ghid_port, (pcb_element_t *) item);
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


static int ghid_propedit_start(void *pe, int num_props,
															 const char *(*query) (void *pe, const char *cmd, const char *key, const char *val, int idx))
{

	ghidgui->propedit_widget = pcb_gtk_dlg_propedit_create(&ghidgui->propedit_dlg, gport->top_window);
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

static void ghid_propedit_add_stat(void *pe, const char *propname, void *propctx, const char *most_common, const char *min,
																	 const char *max, const char *avg)
{
	pcb_gtk_dlg_propedit_prop_add(&ghidgui->propedit_dlg, propname, most_common, min, max, avg);
}

static void ghid_attributes(const char *owner, pcb_attribute_list_t * attrs)
{
	pcb_gtk_dlg_attributes(ghid_port.top_window, owner, attrs);
}

pcb_hid_drc_gui_t ghid_drc_gui = {
	1,														/* log_drc_overview */
	0,														/* log_drc_details */
	ghid_drc_window_reset_message,
	ghid_drc_window_append_violation,
	ghid_drc_window_throw_dialog,
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
	if (!ghidgui || !ghidgui->route_style_selector)
		return;

	pcb_gtk_route_style_sync
		(GHID_ROUTE_STYLE(ghidgui->route_style_selector),
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

	if (ghidgui->route_style_selector) {
		pcb_gtk_route_style_empty(GHID_ROUTE_STYLE(ghidgui->route_style_selector));
		make_route_style_buttons(GHID_ROUTE_STYLE(ghidgui->route_style_selector));
	}
	RouteStylesChanged(0, 0, NULL);

	ghid_port_ranges_scale();
	pcb_gtk_zoom_view_fit(&gport->view);
	ghid_sync_with_new_layout();
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
	ghid_handle_user_command(TRUE);
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
		ghid_library_window_show(gport, raise);
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		ghid_log_window_show(raise);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		ghid_netlist_window_show(gport, raise);
	}
	else if (strcmp(a, "5") == 0 || pcb_strcasecmp(a, "Preferences") == 0) {
		ghid_config_window_show(gport);
	}
	else if (strcmp(a, "6") == 0 || pcb_strcasecmp(a, "DRC") == 0) {
		ghid_drc_window_show(raise);
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

#warning TODO: move this to a header
extern void ghid_handle_units_changed(void);

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

	ghid_handle_units_changed();

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
		menu_node = pcb_hid_cfg_get_menu(ghid_cfg, name);
		if (menu_node != NULL)
			menu = menu_node->user_data;
	}

	if (!GTK_IS_MENU(menu)) {
		pcb_message(PCB_MSG_ERROR, _("The specified popup menu \"%s\" has not been defined.\n"), argv[0]);
		return 1;
	}
	else {
		ghidgui->in_popup = TRUE;
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
	if (!ghidgui->command_entry_status_line_active)
		ghid_status_line_set_text_(ghidgui->status_line_label, text);
}

void ghid_set_status_line_label(void)
{
	if (!ghidgui->command_entry_status_line_active) \
		ghid_set_status_line_label_(ghidgui->status_line_label, conf_hid_gtk.plugins.hid_gtk.compact_horizontal); \
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

		 static void init_conf_watch(conf_hid_callbacks_t * cbs, const char *path, void (*func) (conf_native_t *))
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

void GhidNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[]);

static int ghid_attribute_dialog_(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title,
																	const char *descr)
{
	return ghid_attribute_dialog(ghid_port.top_window, attrs, n_attrs, results, title, descr);
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
	ghid_hid.unwatch_file = ghid_unwatch_file;
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
	ghid_menuconf_id = conf_hid_reg(ghid_menu_cookie, NULL);
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

	return hid_hid_gtk_uninit;
}

int gtkhid_active = 0;

void gtkhid_begin(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_cookie)
		PCB_REGISTER_ACTIONS(ghid_netlist_action_list, ghid_cookie)
		PCB_REGISTER_ACTIONS(ghid_log_action_list, ghid_cookie)
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
