#include "config.h"

#include "crosshair.h"

#include "gui.h"
#include "common.h"

#include "dlg_topwin.h"
#include "in_keyboard.h"
#include "hid_gtk_conf.h"

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

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
	   |  A g_main_loop will be running when pcb-rnd wants the user to select a
	   |  location or if command entry is needed in the status line.
	   |  During these times normal button/key presses are intercepted, either
	   |  by new signal handlers or the command_combo_box entry.
	 */

void ghid_interface_input_signals_connect(void)
{
	ghidgui->button_press_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_press_event", G_CALLBACK(ghid_port_button_press_cb), &gport->mouse);
	ghidgui->button_release_handler = g_signal_connect(G_OBJECT(gport->drawing_area), "button_release_event", G_CALLBACK(ghid_port_button_release_cb), &gport->mouse);
	kbd_input_signals_connect(0, gport->drawing_area);
	kbd_input_signals_connect(3, ghidgui->topwin.left_toolbar);
}

void ghid_interface_input_signals_disconnect(void)
{
	kbd_input_signals_disconnect(0, gport->drawing_area);
	kbd_input_signals_disconnect(3, ghidgui->topwin.left_toolbar);

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
