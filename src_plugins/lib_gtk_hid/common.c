#include "config.h"

#include "crosshair.h"
#include "conf_core.h"

#include "gui.h"
#include "common.h"

#include "../src_plugins/lib_gtk_common/wt_layersel.h"
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

/* Do scrollbar scaling based on current port drawing area size and
   |  overall PCB board size.
 */
void pcb_gtk_tw_ranges_scale(pcb_gtk_topwin_t *tw)
{
	/* Update the scrollbars with PCB units.  So Scale the current
	   |  drawing area size in pixels to PCB units and that will be
	   |  the page size for the Gtk adjustment.
	 */
	pcb_gtk_zoom_post(&gport->view);

	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->h_range)), gport->view.width, PCB->MaxWidth);
	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->v_range)), gport->view.height, PCB->MaxHeight);
}

void ghid_note_event_location(GdkEventButton *ev)
{
	gint event_x, event_y;

	if (!ev) {
		gdkc_window_get_pointer(ghid_port.drawing_area, &event_x, &event_y, NULL);
	}
	else {
		event_x = ev->x;
		event_y = ev->y;
	}

	pcb_gtk_coords_event2pcb(&gport->view, event_x, event_y, &gport->view.pcb_x, &gport->view.pcb_y);

	pcb_event_move_crosshair(gport->view.pcb_x, gport->view.pcb_y);
	ghid_set_cursor_position_labels(&ghidgui->topwin.cps, conf_core.appearance.compact);
}

static void kbd_input_signals_connect(int idx, void *obj)
{
	ghidgui->key_press_handler[idx] = g_signal_connect(G_OBJECT(obj), "key_press_event", G_CALLBACK(ghid_port_key_press_cb), &ghid_port.view);
	ghidgui->key_release_handler[idx] = g_signal_connect(G_OBJECT(obj), "key_release_event", G_CALLBACK(ghid_port_key_release_cb), &ghidgui->topwin);
}

static void kbd_input_signals_disconnect(int idx, void *obj)
{
	if (ghidgui->key_press_handler[idx] != 0) {
		g_signal_handler_disconnect(G_OBJECT(obj), ghidgui->key_press_handler[idx]);
		ghidgui->key_press_handler[idx] = 0;
	}
	if (ghidgui->key_release_handler[idx] != 0) {
		g_signal_handler_disconnect(G_OBJECT(obj), ghidgui->key_release_handler[idx]);
		ghidgui->key_release_handler[idx] = 0;
	}
}

	/* Connect and disconnect just the signals a g_main_loop() will need.
	   |  Cursor and motion events still need to be handled by the top level
	   |  loop, so don't connect/reconnect these.
	   |  A g_main_loop will be running when PCB wants the user to select a
	   |  location or if command entry is needed in the status line hbox.
	   |  During these times normal button/key presses are intercepted, either
	   |  by new signal handlers or the command_combo_box entry.
	 */

void ghid_interface_input_signals_connect(void)
{
	ghidgui->button_press_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_press_event", G_CALLBACK(ghid_port_button_press_cb), &gport->mouse);
	ghidgui->button_release_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_release_event", G_CALLBACK(ghid_port_button_release_cb), &gport->mouse);
	kbd_input_signals_connect(0, gport->drawing_area);
	kbd_input_signals_connect(1, ghidgui->topwin.layer_selector);
	kbd_input_signals_connect(2, ghidgui->topwin.route_style_selector);
	kbd_input_signals_connect(3, ghidgui->topwin.left_toolbar);
	kbd_input_signals_connect(4, ghidgui->topwin.cps.grid_units_button);
}

void ghid_interface_input_signals_disconnect(void)
{
	kbd_input_signals_disconnect(0, gport->drawing_area);
	kbd_input_signals_disconnect(1, ghidgui->topwin.layer_selector);
	kbd_input_signals_disconnect(2, ghidgui->topwin.route_style_selector);
	kbd_input_signals_disconnect(3, ghidgui->topwin.left_toolbar);
	kbd_input_signals_disconnect(4, ghidgui->topwin.cps.grid_units_button);

	if (ghidgui->button_press_handler != 0)
		g_signal_handler_disconnect(G_OBJECT(gport->drawing_area), ghidgui->button_press_handler);

	if (ghidgui->button_release_handler != 0)
		g_signal_handler_disconnect(gport->drawing_area, ghidgui->button_release_handler);

	ghidgui->button_press_handler = ghidgui->button_release_handler = 0;
}

int ghid_shift_is_pressed()
{
	GdkModifierType mask;
	GHidPort *out = &ghid_port;

	if (!ghidgui->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

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

	if (!ghidgui->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

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

	if (!ghidgui->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);
#ifdef __APPLE__
	return (mask & (1 << 13)) ? TRUE : FALSE;	/* The option key is not MOD1, although it should be... */
#else
	return (mask & GDK_MOD1_MASK) ? TRUE : FALSE;
#endif
}

void ghid_LayersChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui || !ghidgui->topwin.active || PCB == NULL || ghidgui->topwin.layersel.running)
		return;

	pcb_gtk_tw_layer_buttons_update(&ghidgui->topwin);

	return;
}

void ghid_LayervisChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui || !ghidgui->topwin.active || PCB == NULL || ghidgui->topwin.layersel.running)
		return;

	pcb_gtk_tw_layer_vis_update(&ghidgui->topwin);
	return;
}
