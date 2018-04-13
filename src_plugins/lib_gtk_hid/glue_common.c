#include "config.h"

#include "glue_common.h"

#include "action_helper.h"

#include "gui.h"
#include "render.h"
#include "common.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

static void ghid_interface_set_sensitive(gboolean sensitive);


/*** drawing widget - output ***/

static void ghid_window_set_name_label(gchar *name)
{
	pcb_gtk_tw_window_set_name_label(&ghidgui->topwin, name);
}

static void ghid_set_status_line_label(void)
{
	if (!ghidgui->topwin.cmd.command_entry_status_line_active)
		pcb_gtk_status_line_update(ghidgui->topwin.status_line_label, conf_hid_gtk.plugins.hid_gtk.compact_horizontal);
}

void ghid_status_line_set_text(const gchar *text)
{
	if (!ghidgui->topwin.cmd.command_entry_status_line_active)
		pcb_gtk_status_line_set_text(ghidgui->topwin.status_line_label, text);
}

static void ghid_port_ranges_scale(void)
{
	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
}

static void ghid_layer_buttons_update(void)
{
	pcb_gtk_tw_layer_buttons_update(&ghidgui->topwin);
}

static void ghid_route_styles_edited_cb()
{
	pcb_gtk_tw_route_styles_edited_cb(&ghidgui->topwin);
}

static void ghid_port_ranges_changed(void)
{
	GtkAdjustment *h_adj, *v_adj;

	h_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.h_range));
	v_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.v_range));
	gport->view.x0 = gtk_adjustment_get_value(h_adj);
	gport->view.y0 = gtk_adjustment_get_value(v_adj);

	ghid_invalidate_all();
}

static void ghid_pan_common(void)
{
	ghidgui->topwin.adjustment_changed_holdoff = TRUE;
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.h_range), gport->view.x0);
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.v_range), gport->view.y0);
	ghidgui->topwin.adjustment_changed_holdoff = FALSE;

	ghid_port_ranges_changed();
}

void ghid_pack_mode_buttons(void)
{
	pcb_gtk_pack_mode_buttons(&ghidgui->topwin.mode_btn);
}

int ghid_command_entry_is_active(void)
{
	return ghidgui->topwin.cmd.command_entry_status_line_active;
}

static void command_pack_in_status_line(void)
{
	gtk_box_pack_start(GTK_BOX(ghidgui->topwin.status_line_hbox), ghidgui->topwin.cmd.command_combo_box, FALSE, FALSE, 0);
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

static void ghid_command_use_command_window_sync(void)
{
	command_use_command_window_sync(&ghidgui->topwin.cmd);
}

/*** input ***/

void ghid_status_update(void)
{
	ghidgui->common.set_status_line_label();
}

static void ghid_interface_set_sensitive(gboolean sensitive)
{
	pcb_gtk_tw_interface_set_sensitive(&ghidgui->topwin, sensitive);
}

static void ghid_port_button_press_main(void)
{
	ghid_invalidate_all();
	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	if (!gport->view.panning)
		g_idle_add(ghid_idle_cb, &ghidgui->topwin);
}

static void ghid_port_button_release_main(void)
{
	pcb_adjust_attached_objects();
	ghid_invalidate_all();

	ghid_window_set_name_label(PCB->Name);
	ghid_set_status_line_label();
	g_idle_add(ghid_idle_cb, &ghidgui->topwin);
}

static void ghid_mode_cursor_main(int mode)
{
	if (mode >= 0)
		ghid_point_cursor(&gport->mouse, 0); /* undo any override (e.g. left over from arrow cursor poitning to the end of a line) first */
	ghid_mode_cursor(&gport->mouse, mode);
}

/*** misc ***/
static void LayersChanged_cb(void)
{
	ghid_LayersChanged(0, 0, 0);
}

static void ghid_load_bg_image(void)
{
	GError *err = NULL;

	if (conf_hid_gtk.plugins.hid_gtk.bg_image)
		ghidgui->bg_pixbuf = gdk_pixbuf_new_from_file(conf_hid_gtk.plugins.hid_gtk.bg_image, &err);

	if (err) {
		g_error("%s", err->message);
		g_error_free(err);
	}
}

static gboolean lead_user_cb(gpointer data)
{
	pcb_lead_user_t *lead_user = data;
	pcb_coord_t step;
	double elapsed_time;

	/* Queue a redraw */
	ghid_invalidate_all();

	/* Update radius */
	elapsed_time = g_timer_elapsed(lead_user->timer, NULL);
	g_timer_start(lead_user->timer);

	step = PCB_MM_TO_COORD(LEAD_USER_VELOCITY * elapsed_time);
	if (lead_user->radius > step)
		lead_user->radius -= step;
	else
		lead_user->radius = PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);

	return TRUE;
}


void ghid_lead_user_to_location(pcb_coord_t x, pcb_coord_t y)
{
	pcb_lead_user_t *lead_user = &gport->lead_user;

	ghid_cancel_lead_user();

	lead_user->lead_user = pcb_true;
	lead_user->x = x;
	lead_user->y = y;
	lead_user->radius = PCB_MM_TO_COORD(LEAD_USER_INITIAL_RADIUS);
	lead_user->timeout = g_timeout_add(LEAD_USER_PERIOD, lead_user_cb, lead_user);
	lead_user->timer = g_timer_new();
}

void ghid_cancel_lead_user(void)
{
	pcb_lead_user_t *lead_user = &gport->lead_user;

	if (lead_user->timeout)
		g_source_remove(lead_user->timeout);

	if (lead_user->timer)
		g_timer_destroy(lead_user->timer);

	if (lead_user->lead_user)
		ghid_invalidate_all();

	lead_user->timeout = 0;
	lead_user->timer = NULL;
	lead_user->lead_user = pcb_false;
}

static void ghid_main_destroy(void *port)
{
	ghid_cancel_lead_user();
	ghidgui->common.shutdown_renderer(port);
	gtk_main_quit();
}

/*** init ***/

void ghid_glue_common_init(void)
{
	/* Set up the glue struct to lib_gtk_common */
	ghidgui->common.gport = &ghid_port;
	ghidgui->common.window_set_name_label = ghid_window_set_name_label;
	ghidgui->common.set_status_line_label = ghid_set_status_line_label;
	ghidgui->common.note_event_location = ghid_note_event_location;
	ghidgui->common.shift_is_pressed = ghid_shift_is_pressed;
	ghidgui->common.interface_input_signals_disconnect = ghid_interface_input_signals_disconnect;
	ghidgui->common.interface_input_signals_connect = ghid_interface_input_signals_connect;
	ghidgui->common.interface_set_sensitive = ghid_interface_set_sensitive;
	ghidgui->common.port_button_press_main = ghid_port_button_press_main;
	ghidgui->common.port_button_release_main = ghid_port_button_release_main;
	ghidgui->common.status_line_set_text = ghid_status_line_set_text;
	ghidgui->common.route_styles_edited_cb = ghid_route_styles_edited_cb;
	ghidgui->common.mode_cursor_main = ghid_mode_cursor_main;
	ghidgui->common.cancel_lead_user = ghid_cancel_lead_user;
	ghidgui->common.lead_user_to_location = ghid_lead_user_to_location;
	ghidgui->common.pan_common = ghid_pan_common;
	ghidgui->common.port_ranges_scale = ghid_port_ranges_scale;
	ghidgui->common.pack_mode_buttons = ghid_pack_mode_buttons;

	ghidgui->common.layer_buttons_update = ghid_layer_buttons_update;
	ghidgui->common.LayersChanged = LayersChanged_cb;
	ghidgui->common.command_entry_is_active = ghid_command_entry_is_active;
	ghidgui->common.command_use_command_window_sync = ghid_command_use_command_window_sync;
	ghidgui->common.load_bg_image = ghid_load_bg_image;
	ghidgui->common.main_destroy = ghid_main_destroy;
	ghidgui->common.port_ranges_changed = ghid_port_ranges_changed;

	ghidgui->topwin.cmd.com = &ghidgui->common;
	ghidgui->topwin.cmd.pack_in_status_line = command_pack_in_status_line;
	ghidgui->topwin.cmd.post_entry = command_post_entry;
	ghidgui->topwin.cmd.pre_entry = command_pre_entry;

	ghid_port.view.com = &ghidgui->common;
	ghid_port.mouse.com = &ghidgui->common;
}
