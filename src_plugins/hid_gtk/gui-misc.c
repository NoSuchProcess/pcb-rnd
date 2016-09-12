/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "global.h"
#include "crosshair.h"
#include "data.h"
#include "misc.h"
#include "action_helper.h"
#include "set.h"
#include "pcb-printf.h"
#include "misc_util.h"

#include "gui.h"
#include <gdk/gdkkeysyms.h>

const char *ghid_cookie = "gtk hid";

#define CUSTOM_CURSOR_CLOCKWISE		(GDK_LAST_CURSOR + 10)
#define CUSTOM_CURSOR_DRAG			(GDK_LAST_CURSOR + 11)
#define CUSTOM_CURSOR_LOCK			(GDK_LAST_CURSOR + 12)

#define ICON_X_HOT 8
#define ICON_Y_HOT 8


GdkPixmap *XC_clock_source, *XC_clock_mask, *XC_hand_source, *XC_hand_mask, *XC_lock_source, *XC_lock_mask;


static GdkCursorType oldCursor;

void ghid_status_line_set_text(const gchar * text)
{
	if (ghidgui->command_entry_status_line_active)
		return;

	ghid_label_set_markup(ghidgui->status_line_label, text);
}

void ghid_cursor_position_label_set_text(gchar * text)
{
	ghid_label_set_markup(ghidgui->cursor_position_absolute_label, text);
}

void ghid_cursor_position_relative_label_set_text(gchar * text)
{
	ghid_label_set_markup(ghidgui->cursor_position_relative_label, text);
}

static GdkCursorType gport_set_cursor(GdkCursorType shape)
{
	GdkWindow *window;
	GdkCursorType old_shape = gport->X_cursor_shape;
	GdkColor fg = { 0, 65535, 65535, 65535 };	/* white */
	GdkColor bg = { 0, 0, 0, 0 };	/* black */

	if (gport->drawing_area == NULL)
		return GDK_X_CURSOR;

	window = gtk_widget_get_window(gport->drawing_area);

	if (gport->X_cursor_shape == shape)
		return shape;

	/* check if window exists to prevent from fatal errors */
	if (window == NULL)
		return GDK_X_CURSOR;

	gport->X_cursor_shape = shape;
	if (shape > GDK_LAST_CURSOR) {
		if (shape == CUSTOM_CURSOR_CLOCKWISE)
			gport->X_cursor = gdk_cursor_new_from_pixmap(XC_clock_source, XC_clock_mask, &fg, &bg, ICON_X_HOT, ICON_Y_HOT);
		else if (shape == CUSTOM_CURSOR_DRAG)
			gport->X_cursor = gdk_cursor_new_from_pixmap(XC_hand_source, XC_hand_mask, &fg, &bg, ICON_X_HOT, ICON_Y_HOT);
		else if (shape == CUSTOM_CURSOR_LOCK)
			gport->X_cursor = gdk_cursor_new_from_pixmap(XC_lock_source, XC_lock_mask, &fg, &bg, ICON_X_HOT, ICON_Y_HOT);
	}
	else
		gport->X_cursor = gdk_cursor_new(shape);

	gdk_window_set_cursor(window, gport->X_cursor);
	gdk_cursor_unref(gport->X_cursor);

	return old_shape;
}

void ghid_point_cursor(void)
{
	oldCursor = gport_set_cursor(GDK_DRAPED_BOX);
}

void ghid_hand_cursor(void)
{
	oldCursor = gport_set_cursor(GDK_HAND2);
}

void ghid_watch_cursor(void)
{
	GdkCursorType tmp;

	tmp = gport_set_cursor(GDK_WATCH);
	if (tmp != GDK_WATCH)
		oldCursor = tmp;
}

void ghid_mode_cursor(int Mode)
{
	switch (Mode) {
	case PCB_MODE_NO:
		gport_set_cursor((GdkCursorType) CUSTOM_CURSOR_DRAG);
		break;

	case PCB_MODE_VIA:
		gport_set_cursor(GDK_ARROW);
		break;

	case PCB_MODE_LINE:
		gport_set_cursor(GDK_PENCIL);
		break;

	case PCB_MODE_ARC:
		gport_set_cursor(GDK_QUESTION_ARROW);
		break;

	case PCB_MODE_ARROW:
		gport_set_cursor(GDK_LEFT_PTR);
		break;

	case PCB_MODE_POLYGON:
	case PCB_MODE_POLYGON_HOLE:
		gport_set_cursor(GDK_SB_UP_ARROW);
		break;

	case PCB_MODE_PASTE_BUFFER:
		gport_set_cursor(GDK_HAND1);
		break;

	case PCB_MODE_TEXT:
		gport_set_cursor(GDK_XTERM);
		break;

	case PCB_MODE_RECTANGLE:
		gport_set_cursor(GDK_UL_ANGLE);
		break;

	case PCB_MODE_THERMAL:
		gport_set_cursor(GDK_IRON_CROSS);
		break;

	case PCB_MODE_REMOVE:
		gport_set_cursor(GDK_PIRATE);
		break;

	case PCB_MODE_ROTATE:
		if (ghid_shift_is_pressed())
			gport_set_cursor((GdkCursorType) CUSTOM_CURSOR_CLOCKWISE);
		else
			gport_set_cursor(GDK_EXCHANGE);
		break;

	case PCB_MODE_COPY:
	case PCB_MODE_MOVE:
		gport_set_cursor(GDK_CROSSHAIR);
		break;

	case PCB_MODE_INSERT_POINT:
		gport_set_cursor(GDK_DOTBOX);
		break;

	case PCB_MODE_LOCK:
		gport_set_cursor((GdkCursorType) CUSTOM_CURSOR_LOCK);
	}
}

void ghid_corner_cursor(void)
{
	GdkCursorType shape;

	if (Crosshair.Y <= Crosshair.AttachedBox.Point1.Y)
		shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ? GDK_UR_ANGLE : GDK_UL_ANGLE;
	else
		shape = (Crosshair.X >= Crosshair.AttachedBox.Point1.X) ? GDK_LR_ANGLE : GDK_LL_ANGLE;
	if (gport->X_cursor_shape != shape)
		gport_set_cursor(shape);
}

void ghid_restore_cursor(void)
{
	gport_set_cursor(oldCursor);
}



	/* =============================================================== */
static gboolean got_location;

	/* If user hits a key instead of the mouse button, we'll abort unless
	   |  it's the enter key (which accepts the current crosshair location).
	 */
static gboolean loop_key_press_cb(GtkWidget * drawing_area, GdkEventKey * kev, GMainLoop ** loop)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		return TRUE;

	switch (ksym) {
	case GDK_Return:							/* Accept cursor location */
		if (g_main_loop_is_running(*loop))
			g_main_loop_quit(*loop);
		break;

	default:											/* Abort */
		got_location = FALSE;
		if (g_main_loop_is_running(*loop))
			g_main_loop_quit(*loop);
		break;
	}
	return TRUE;
}

	/* User hit a mouse button in the Output drawing area, so quit the loop
	   |  and the cursor values when the button was pressed will be used.
	 */
static gboolean loop_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, GMainLoop ** loop)
{
	if (g_main_loop_is_running(*loop))
		g_main_loop_quit(*loop);
	ghid_note_event_location(ev);
	return TRUE;
}

int ghid_wheel_zoom = 0;
	/* Run a glib GMainLoop which intercepts key and mouse button events from
	   |  the top level loop.  When a mouse or key is hit in the Output drawing
	   |  area, quit the loop so the top level loop can continue and use the
	   |  the mouse pointer coordinates at the time of the mouse button event.
	 */
static gboolean run_get_location_loop(const gchar * message)
{
	static int getting_loc = 0;
	GMainLoop *loop;
	gulong button_handler, key_handler;
	gint oldObjState, oldLineState, oldBoxState;

	/* Do not enter the loop recursively (ask for coord only once); also don't
	   ask for coord if the scrollwheel triggered the event, it may cause strange
	   GUI lockups when done outside of the drawing area */
	if ((getting_loc) || (ghid_wheel_zoom))
		return false;

	getting_loc = 1;
	ghid_status_line_set_text(message);

	oldObjState = Crosshair.AttachedObject.State;
	oldLineState = Crosshair.AttachedLine.State;
	oldBoxState = Crosshair.AttachedBox.State;
	notify_crosshair_change(false);
	Crosshair.AttachedObject.State = STATE_FIRST;
	Crosshair.AttachedLine.State = STATE_FIRST;
	Crosshair.AttachedBox.State = STATE_FIRST;
	ghid_hand_cursor();
	notify_crosshair_change(true);

	/* Stop the top level GMainLoop from getting user input from keyboard
	   |  and mouse so we can install our own handlers here.  Also set the
	   |  control interface insensitive so all the user can do is hit a key
	   |  or mouse button in the Output drawing area.
	 */
	ghid_interface_input_signals_disconnect();
	ghid_interface_set_sensitive(FALSE);

	got_location = TRUE;					/* Will be unset by hitting most keys */
	button_handler =
		g_signal_connect(G_OBJECT(gport->drawing_area), "button_press_event", G_CALLBACK(loop_button_press_cb), &loop);
	key_handler = g_signal_connect(G_OBJECT(gport->top_window), "key_press_event", G_CALLBACK(loop_key_press_cb), &loop);

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_main_loop_unref(loop);

	g_signal_handler_disconnect(gport->drawing_area, button_handler);
	g_signal_handler_disconnect(gport->top_window, key_handler);

	ghid_interface_input_signals_connect();	/* return to normal */
	ghid_interface_set_sensitive(TRUE);

	notify_crosshair_change(false);
	Crosshair.AttachedObject.State = oldObjState;
	Crosshair.AttachedLine.State = oldLineState;
	Crosshair.AttachedBox.State = oldBoxState;
	notify_crosshair_change(true);
	ghid_restore_cursor();

	ghid_set_status_line_label();

	getting_loc = 0;
	return got_location;
}



/* ---------------------------------------------------------------------------*/
void ghid_get_user_xy(const char *msg)
{
	run_get_location_loop(msg);
}

	/* XXX The abort dialog isn't implemented yet in the Gtk port
	 */
void ghid_create_abort_dialog(char *msg)
{
}

gboolean ghid_check_abort(void)
{
	return FALSE;									/* Abort isn't implemented, so never abort */
}

void ghid_end_abort(void)
{
}

void ghid_get_pointer(int *x, int *y)
{
	gint xp, yp;

	gdk_window_get_pointer(gtk_widget_get_window(gport->drawing_area), &xp, &yp, NULL);
	if (x)
		*x = xp;
	if (y)
		*y = yp;
}

/* ---------------------------------------------------------------------------
 * output of status line
 */
void ghid_set_status_line_label(void)
{
	const gchar *flag = conf_core.editor.all_direction_lines
	    ? "all" : (PCB->Clipping == 0 ? "45" : (PCB->Clipping == 1 ? "45_/" : "45\\_"));
	    char *text = pcb_strdup_printf(_("%m+<b>view</b>=%s  "
	        "<b>grid</b>=%$mS  "
	        "%s%s  "
	        "<b>line</b>=%mS  "
	        "<b>via</b>=%mS (%mS)  %s"
	        "<b>clearance</b>=%mS  "
	        "<b>text</b>=%i%%  "
	        "<b>buffer</b>=#%i"),
	conf_core.editor.grid_unit->allow,
	conf_core.editor.show_solder_side ? _("solder") : _("component"),
	PCB->Grid,
	flag, conf_core.editor.rubber_band_mode ? ",R  " : "  ",
	conf_core.design.line_thickness,
	conf_core.design.via_thickness,
	conf_core.design.via_drilling_hole,
	conf_hid_gtk.plugins.hid_gtk.compact_horizontal ? "\n" : "",
	conf_core.design.clearance,
	conf_core.design.text_scale, conf_core.editor.buffer_number + 1);

	ghid_status_line_set_text(text);
	free(text);
}

/* ---------------------------------------------------------------------------
 * output of cursor position
 */
void ghid_set_cursor_position_labels(void)
{
	char *text, sep = ' ';
	if (conf_hid_gtk.plugins.hid_gtk.compact_vertical)
		sep = '\n';

	if (Marked.status) {
		Coord dx = Crosshair.X - Marked.X;
		Coord dy = Crosshair.Y - Marked.Y;
		Coord r = Distance(Crosshair.X, Crosshair.Y, Marked.X, Marked.Y);
		double a = atan2(dy, dx) * PCB_RAD_TO_DEG;


		text = pcb_strdup_printf(_("%m+r %-mS;%cphi %-.1f;%c%-mS %-mS"), conf_core.editor.grid_unit->allow, r, sep, a, sep, dx, dy);
		ghid_cursor_position_relative_label_set_text(text);
		free(text);
	}
	else {
		char text[64];
		sprintf(text, _("r __.__;%cphi __._;%c__.__ __.__"), sep, sep);
		ghid_cursor_position_relative_label_set_text(text);
	}


	text = pcb_strdup_printf("%m+%-mS%c%-mS", conf_core.editor.grid_unit->allow, Crosshair.X, sep, Crosshair.Y);
	ghid_cursor_position_label_set_text(text);
	free(text);
}
