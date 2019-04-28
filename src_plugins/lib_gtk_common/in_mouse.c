/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2017, Tibor 'Igor2' Palinkas
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port;
   refactored for pcb-rnd by Tibor 'Igor2' Palinkas */

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "in_mouse.h"

#include "actions.h"
#include "board.h"
#include "crosshair.h"
#include "conf_core.h"
#include "undo.h"
#include "tool.h"

#include "in_keyboard.h"
#include "bu_icons.h"

pcb_hid_cfg_mouse_t ghid_mouse;
int ghid_wheel_zoom = 0;

pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button)
{
	/* GDK numbers buttons from 1..5, there seems to be no symbolic names */
	return (PCB_MB_LEFT << (ev_button - 1));
}

static GdkCursorType old_cursor, cursor_override;

#define CUSTOM_CURSOR_CLOCKWISE		(GDK_LAST_CURSOR + 10)
#define CUSTOM_CURSOR_DRAG			  (GDK_LAST_CURSOR + 11)
#define CUSTOM_CURSOR_LOCK			  (GDK_LAST_CURSOR + 12)

#define ICON_X_HOT 8
#define ICON_Y_HOT 8


static GdkCursorType gport_set_cursor(pcb_gtk_mouse_t *ctx, GdkCursorType shape)
{
	GdkWindow *window;
	GdkCursorType old_shape = ctx->X_cursor_shape;

	if (ctx->drawing_area == NULL)
		return GDK_X_CURSOR;

	window = gtk_widget_get_window(ctx->drawing_area);

	if (ctx->X_cursor_shape == shape)
		return shape;

	/* check if window exists to prevent from fatal errors */
	if (window == NULL)
		return GDK_X_CURSOR;

	ctx->X_cursor_shape = shape;
	if (shape > GDK_LAST_CURSOR) {
		if (shape == CUSTOM_CURSOR_CLOCKWISE)
			ctx->X_cursor = gdk_cursor_new_from_pixbuf(gtk_widget_get_display(ctx->drawing_area), XC_clock_source, ICON_X_HOT, ICON_Y_HOT);
		else if (shape == CUSTOM_CURSOR_DRAG)
			ctx->X_cursor = gdk_cursor_new_from_pixbuf(gtk_widget_get_display(ctx->drawing_area), XC_hand_source, ICON_X_HOT, ICON_Y_HOT);
		else if (shape == CUSTOM_CURSOR_LOCK)
			ctx->X_cursor = gdk_cursor_new_from_pixbuf(gtk_widget_get_display(ctx->drawing_area), XC_lock_source, ICON_X_HOT, ICON_Y_HOT);
	}
	else
		ctx->X_cursor = gdk_cursor_new(shape);

	gdk_window_set_cursor(window, ctx->X_cursor);
	gdk_cursor_unref(ctx->X_cursor);

	return old_shape;
}

void ghid_point_cursor(pcb_gtk_mouse_t *ctx, pcb_bool grabbed)
{
	if (grabbed) {
		old_cursor = gport_set_cursor(ctx, GDK_DRAPED_BOX);
		cursor_override = GDK_DRAPED_BOX;
	}
	else {
		cursor_override = 0;
		ghid_mode_cursor(ctx, -1);
	}
}

void ghid_hand_cursor(pcb_gtk_mouse_t *ctx)
{
	old_cursor = gport_set_cursor(ctx, GDK_HAND2);
	cursor_override = GDK_HAND2;
}

void ghid_watch_cursor(pcb_gtk_mouse_t *ctx)
{
	GdkCursorType tmp;

	tmp = gport_set_cursor(ctx, GDK_WATCH);
	if (tmp != GDK_WATCH)
		old_cursor = tmp;
}

void ghid_mode_cursor(pcb_gtk_mouse_t *ctx, int mode)
{
	if (cursor_override != 0) {
		gport_set_cursor(ctx, cursor_override);
		return;
	}

	if (mode < 0) /* automatic */
		mode = conf_core.editor.mode;

	switch (mode) {
	case PCB_MODE_NO:
		gport_set_cursor(ctx, (GdkCursorType) CUSTOM_CURSOR_DRAG);
		break;

	case PCB_MODE_VIA:
		gport_set_cursor(ctx, GDK_ARROW);
		break;

	case PCB_MODE_LINE:
		gport_set_cursor(ctx, GDK_PENCIL);
		break;

	case PCB_MODE_ARC:
		gport_set_cursor(ctx, GDK_QUESTION_ARROW);
		break;

	case PCB_MODE_ARROW:
		gport_set_cursor(ctx, GDK_LEFT_PTR);
		break;

	case PCB_MODE_POLYGON:
	case PCB_MODE_POLYGON_HOLE:
		gport_set_cursor(ctx, GDK_SB_UP_ARROW);
		break;

	case PCB_MODE_PASTE_BUFFER:
		gport_set_cursor(ctx, GDK_HAND1);
		break;

	case PCB_MODE_TEXT:
		gport_set_cursor(ctx, GDK_XTERM);
		break;

	case PCB_MODE_RECTANGLE:
		gport_set_cursor(ctx, GDK_UL_ANGLE);
		break;

	case PCB_MODE_THERMAL:
		gport_set_cursor(ctx, GDK_IRON_CROSS);
		break;

	case PCB_MODE_REMOVE:
		gport_set_cursor(ctx, GDK_PIRATE);
		break;

	case PCB_MODE_ROTATE:
		if (ctx->com->shift_is_pressed())
			gport_set_cursor(ctx, (GdkCursorType) CUSTOM_CURSOR_CLOCKWISE);
		else
			gport_set_cursor(ctx, GDK_EXCHANGE);
		break;

	case PCB_MODE_COPY:
	case PCB_MODE_MOVE:
		gport_set_cursor(ctx, GDK_CROSSHAIR);
		break;

	case PCB_MODE_INSERT_POINT:
		gport_set_cursor(ctx, GDK_DOTBOX);
		break;

	case PCB_MODE_LOCK:
		gport_set_cursor(ctx, (GdkCursorType) CUSTOM_CURSOR_LOCK);
	}
}

void ghid_corner_cursor(pcb_gtk_mouse_t *ctx)
{
	GdkCursorType shape;

	if (pcb_crosshair.Y <= pcb_crosshair.AttachedBox.Point1.Y)
		shape = (pcb_crosshair.X >= pcb_crosshair.AttachedBox.Point1.X) ? GDK_UR_ANGLE : GDK_UL_ANGLE;
	else
		shape = (pcb_crosshair.X >= pcb_crosshair.AttachedBox.Point1.X) ? GDK_LR_ANGLE : GDK_LL_ANGLE;
	if (ctx->X_cursor_shape != shape)
		gport_set_cursor(ctx, shape);
}

void ghid_restore_cursor(pcb_gtk_mouse_t *ctx)
{
	cursor_override = 0;
	gport_set_cursor(ctx, old_cursor);
}

	/* =============================================================== */
typedef struct {
	GMainLoop *loop;
	pcb_gtk_common_t *com;
	gboolean got_location;
	gint last_press;
} loop_ctx_t;

static gboolean loop_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, loop_ctx_t *lctx)
{
	lctx->last_press = kev->keyval;
	return TRUE;
}


/*  If user hits a key instead of the mouse button, we'll abort unless
    it's the enter key (which accepts the current crosshair location).
 */
static gboolean loop_key_release_cb(GtkWidget *drawing_area, GdkEventKey *kev, loop_ctx_t *lctx)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		return TRUE;

	/* accept a key only after a press _and_ release to avoid interfering with
	   dialog boxes before and after the loop */
	if (ksym != lctx->last_press)
		return TRUE;

	switch (ksym) {
	case GDK_KEY_Return:					/* Accept cursor location */
		if (g_main_loop_is_running(lctx->loop))
			g_main_loop_quit(lctx->loop);
		break;

	default:											/* Abort */
		lctx->got_location = FALSE;
		if (g_main_loop_is_running(lctx->loop))
			g_main_loop_quit(lctx->loop);
		break;
	}
	return TRUE;
}

/*  User hit a mouse button in the Output drawing area, so quit the loop
    and the cursor values when the button was pressed will be used.
 */
static gboolean loop_button_press_cb(GtkWidget *drawing_area, GdkEventButton *ev, loop_ctx_t *lctx)
{
	if (g_main_loop_is_running(lctx->loop))
		g_main_loop_quit(lctx->loop);
	lctx->com->note_event_location(ev);
	return TRUE;
}

/*  Run a glib GMainLoop which intercepts key and mouse button events from
    the top level loop.  When a mouse or key is hit in the Output drawing
    area, quit the loop so the top level loop can continue and use the
    the mouse pointer coordinates at the time of the mouse button event.
 */
static gboolean run_get_location_loop(pcb_gtk_mouse_t *ctx, const gchar * message)
{
	static int getting_loc = 0;
	loop_ctx_t lctx;
	gulong button_handler, key_handler1, key_handler2;
	gint oldObjState, oldLineState, oldBoxState;

	/* Do not enter the loop recursively (ask for coord only once); also don't
	   ask for coord if the scrollwheel triggered the event, it may cause strange
	   GUI lockups when done outside of the drawing area
	 */
	if ((getting_loc) || (ghid_wheel_zoom))
		return pcb_false;

	getting_loc = 1;
	pcb_actionl("StatusSetText", message, NULL);

	oldObjState = pcb_crosshair.AttachedObject.State;
	oldLineState = pcb_crosshair.AttachedLine.State;
	oldBoxState = pcb_crosshair.AttachedBox.State;
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	ghid_hand_cursor(ctx);
	pcb_notify_crosshair_change(pcb_true);

	/*  Stop the top level GMainLoop from getting user input from keyboard
	   and mouse so we can install our own handlers here.  Also set the
	   control interface insensitive so all the user can do is hit a key
	   or mouse button in the Output drawing area.
	 */
	ctx->com->interface_input_signals_disconnect();
	ctx->com->interface_set_sensitive(FALSE);

	lctx.got_location = TRUE;   /* Will be unset by hitting most keys */
	button_handler =
		g_signal_connect(G_OBJECT(ctx->drawing_area), "button_press_event", G_CALLBACK(loop_button_press_cb), &lctx);
	key_handler1 = g_signal_connect(G_OBJECT(ctx->top_window), "key_press_event", G_CALLBACK(loop_key_press_cb), &lctx);
	key_handler2 = g_signal_connect(G_OBJECT(ctx->top_window), "key_release_event", G_CALLBACK(loop_key_release_cb), &lctx);

	lctx.loop = g_main_loop_new(NULL, FALSE);
	lctx.com = ctx->com;
	g_main_loop_run(lctx.loop);

	g_main_loop_unref(lctx.loop);

	g_signal_handler_disconnect(ctx->drawing_area, button_handler);
	g_signal_handler_disconnect(ctx->top_window, key_handler1);
	g_signal_handler_disconnect(ctx->top_window, key_handler2);

	ctx->com->interface_input_signals_connect();	/* return to normal */
	ctx->com->interface_set_sensitive(TRUE);

	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.State = oldObjState;
	pcb_crosshair.AttachedLine.State = oldLineState;
	pcb_crosshair.AttachedBox.State = oldBoxState;
	pcb_notify_crosshair_change(pcb_true);
	ghid_restore_cursor(ctx);

	pcb_actionl("StatusSetText", NULL);
	getting_loc = 0;
	return lctx.got_location;
}

gboolean ghid_get_user_xy(pcb_gtk_mouse_t *ctx, const char *msg)
{
	pcb_undo_save_serial(); /* will be restored on button release in action helper in core */
	return run_get_location_loop(ctx, msg);
}

/* Mouse scroll wheel events */
gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *data)
{
	pcb_gtk_mouse_t *ctx = data;
	ModifierKeysState mk;
	GdkModifierType state;
	int button;

	state = (GdkModifierType) (ev->state);
	mk = ghid_modifier_keys_state(widget, &state);

	/* X11 gtk hard codes buttons 4, 5, 6, 7 as below in
	 * gtk+/gdk/x11/gdkevents-x11.c:1121, but quartz and windows have
	 * special mouse scroll events, so this may conflict with a mouse
	 * who has buttons 4 - 7 that aren't the scroll wheel?
	 */
	switch (ev->direction) {
		case GDK_SCROLL_UP:    button = PCB_MB_SCROLL_UP; break;
		case GDK_SCROLL_DOWN:  button = PCB_MB_SCROLL_DOWN; break;
		case GDK_SCROLL_LEFT:  button = PCB_MB_SCROLL_LEFT; break;
		case GDK_SCROLL_RIGHT: button = PCB_MB_SCROLL_RIGHT; break;
		default: return FALSE;
	}

	ghid_wheel_zoom = 1;
	hid_cfg_mouse_action(&ghid_mouse, button | mk, ctx->com->command_entry_is_active());
	ghid_wheel_zoom = 0;

	return TRUE;
}

gboolean ghid_port_button_press_cb(GtkWidget *drawing_area, GdkEventButton *ev, gpointer data)
{
	ModifierKeysState mk;
	GdkModifierType state;
	GdkModifierType mask;
	pcb_gtk_mouse_t *ctx = data;

	/* Reject double and triple click events */
	if (ev->type != GDK_BUTTON_PRESS)
		return TRUE;

	ctx->com->note_event_location(ev);
	state = (GdkModifierType) (ev->state);
	mk = ghid_modifier_keys_state(drawing_area, &state);

	pcb_gtk_glob_mask = state;

	gdkc_window_get_pointer(drawing_area, NULL, NULL, &mask);

	hid_cfg_mouse_action(&ghid_mouse, ghid_mouse_button(ev->button) | mk, ctx->com->command_entry_is_active());

	ctx->com->port_button_press_main();

	return TRUE;
}

gboolean ghid_port_button_release_cb(GtkWidget *drawing_area, GdkEventButton *ev, gpointer data)
{
	ModifierKeysState mk;
	GdkModifierType state;
	pcb_gtk_mouse_t *ctx = data;

	ctx->com->note_event_location(ev);
	state = (GdkModifierType) (ev->state);
	mk = ghid_modifier_keys_state(drawing_area, &state);

	hid_cfg_mouse_action(&ghid_mouse, ghid_mouse_button(ev->button) | mk | PCB_M_Release, ctx->com->command_entry_is_active());

	ctx->com->port_button_release_main();
	return TRUE;
}

