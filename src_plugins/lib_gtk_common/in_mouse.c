/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

#include "actions.h"
#include "board.h"
#include "crosshair.h"
#include "conf_core.h"
#include "tool.h"

#define GVT_DONT_UNDEF
#include "in_mouse.h"
#include <genvector/genvector_impl.c>

#include "in_keyboard.h"
#include "glue_common.h"

pcb_hid_cfg_mouse_t ghid_mouse;
int ghid_wheel_zoom = 0;

pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button)
{
	/* GDK numbers buttons from 1..5, there seems to be no symbolic names */
	return (PCB_MB_LEFT << (ev_button - 1));
}

static GdkCursorType cursor_override;
static GdkCursor *cursor_override_X;

#define CUSTOM_CURSOR_CLOCKWISE		(GDK_LAST_CURSOR + 10)
#define CUSTOM_CURSOR_DRAG			  (GDK_LAST_CURSOR + 11)
#define CUSTOM_CURSOR_LOCK			  (GDK_LAST_CURSOR + 12)

#define ICON_X_HOT 8
#define ICON_Y_HOT 8

void ghid_watch_cursor(pcb_gtk_t *ctx)
{
	static GdkCursor *xc;

	cursor_override = GDK_WATCH;
	if (xc == NULL) xc = gdk_cursor_new(cursor_override);
	cursor_override_X = xc;
	ghid_mode_cursor(ctx);
}

static void ghid_hand_cursor(pcb_gtk_t *ctx)
{
	static GdkCursor *xc;

	cursor_override = GDK_HAND2;
	if (xc == NULL) xc = gdk_cursor_new(cursor_override);
	cursor_override_X = xc;
	ghid_mode_cursor(ctx);
}


void ghid_point_cursor(pcb_gtk_t *ctx, pcb_bool grabbed)
{
	if (grabbed) {
		static GdkCursor *xc;
		cursor_override = GDK_DRAPED_BOX;
		if (xc == NULL) xc = gdk_cursor_new(cursor_override);
		cursor_override_X = xc;
	}
	else
		cursor_override = 0;
	ghid_mode_cursor(ctx);
}


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
	pcb_gtk_note_event_location(ev);
	return TRUE;
}

/*  Run a glib GMainLoop which intercepts key and mouse button events from
    the top level loop.  When a mouse or key is hit in the Output drawing
    area, quit the loop so the top level loop can continue and use the
    the mouse pointer coordinates at the time of the mouse button event.
 */
static gboolean run_get_location_loop(pcb_gtk_t *ctx, const gchar * message)
{
	static int getting_loc = 0;
	loop_ctx_t lctx;
	gulong button_handler, key_handler1, key_handler2;
	void *chst;

	/* Do not enter the loop recursively (ask for coord only once); also don't
	   ask for coord if the scrollwheel triggered the event, it may cause strange
	   GUI lockups when done outside of the drawing area
	 */
	if ((getting_loc) || (ghid_wheel_zoom))
		return pcb_false;

	getting_loc = 1;
	pcb_actionl("StatusSetText", message, NULL);

	
	chst = pcb_hidlib_crosshair_suspend();
	ghid_hand_cursor(ctx);

	/*  Stop the top level GMainLoop from getting user input from keyboard
	   and mouse so we can install our own handlers here.  Also set the
	   control interface insensitive so all the user can do is hit a key
	   or mouse button in the Output drawing area.
	 */
	ctx->common.interface_input_signals_disconnect();
	ctx->common.interface_set_sensitive(FALSE);

	lctx.got_location = TRUE;   /* Will be unset by hitting most keys */

	button_handler =
		g_signal_connect(G_OBJECT(ctx->topwin.drawing_area), "button_press_event", G_CALLBACK(loop_button_press_cb), &lctx);
	key_handler1 = g_signal_connect(G_OBJECT(ctx->common.top_window), "key_press_event", G_CALLBACK(loop_key_press_cb), &lctx);
	key_handler2 = g_signal_connect(G_OBJECT(ctx->common.top_window), "key_release_event", G_CALLBACK(loop_key_release_cb), &lctx);

	lctx.loop = g_main_loop_new(NULL, FALSE);
	lctx.com = &ctx->common;
	g_main_loop_run(lctx.loop);

	g_main_loop_unref(lctx.loop);

	g_signal_handler_disconnect(ctx->topwin.drawing_area, button_handler);
	g_signal_handler_disconnect(ctx->common.top_window, key_handler1);
	g_signal_handler_disconnect(ctx->common.top_window, key_handler2);

	ctx->common.interface_input_signals_connect(); /* return to normal */
	ctx->common.interface_set_sensitive(TRUE);

	pcb_hidlib_crosshair_restore(chst);
	ghid_restore_cursor(ctx);

	pcb_actionl("StatusSetText", NULL);
	getting_loc = 0;
	return lctx.got_location;
}

gboolean ghid_get_user_xy(pcb_gtk_t *ctx, const char *msg)
{
	return run_get_location_loop(ctx, msg);
}

/* Mouse scroll wheel events */
gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *data)
{
	pcb_gtk_t *ctx = data;
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
	hid_cfg_mouse_action(&ghid_mouse, button | mk, ctx->topwin.cmd.command_entry_status_line_active);
	ghid_wheel_zoom = 0;

	return TRUE;
}

gboolean ghid_port_button_press_cb(GtkWidget *drawing_area, GdkEventButton *ev, gpointer data)
{
	ModifierKeysState mk;
	GdkModifierType state;
	GdkModifierType mask;
	pcb_gtk_t *ctx = data;

	/* Reject double and triple click events */
	if (ev->type != GDK_BUTTON_PRESS)
		return TRUE;

	pcb_gtk_note_event_location(ev);
	state = (GdkModifierType) (ev->state);
	mk = ghid_modifier_keys_state(drawing_area, &state);

	pcb_gtk_glob_mask = state;

	gdkc_window_get_pointer(drawing_area, NULL, NULL, &mask);

	hid_cfg_mouse_action(&ghid_mouse, ghid_mouse_button(ev->button) | mk, ctx->topwin.cmd.command_entry_status_line_active);

	ctx->common.port_button_press_main();

	return TRUE;
}

gboolean ghid_port_button_release_cb(GtkWidget *drawing_area, GdkEventButton *ev, gpointer data)
{
	ModifierKeysState mk;
	GdkModifierType state;
	pcb_gtk_t *ctx = data;

	pcb_gtk_note_event_location(ev);
	state = (GdkModifierType) (ev->state);
	mk = ghid_modifier_keys_state(drawing_area, &state);

	hid_cfg_mouse_action(&ghid_mouse, ghid_mouse_button(ev->button) | mk | PCB_M_Release, ctx->topwin.cmd.command_entry_status_line_active);

	ctx->common.port_button_release_main();
	return TRUE;
}

/* Allocates (with refco=1) an RGBA 24x24 GdkPixbuf from data, using mask;
   width and height should be smaller than 24. Use g_object_unref() to free
   the structure. */
static GdkPixbuf *pcb_gtk_cursor_from_xbm_data(const unsigned char *data, const unsigned char *mask, unsigned int width, unsigned int height)
{
	GdkPixbuf *dest;
	const unsigned char *src_data, *mask_data;
	unsigned char *dest_data;
	int dest_stride;
	unsigned char reg, reg_m;
	unsigned char data_b, mask_b;
	int bits;
	int x, y;

	g_return_val_if_fail((width < 25 && height < 25), NULL);

	/* Creates a new suitable GdkPixbuf structure for cursor. */
	dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 24, 24);
	dest_data = gdk_pixbuf_get_pixels(dest);
	dest_stride = gdk_pixbuf_get_rowstride(dest);

	src_data = data;
	mask_data = mask;

	/* Copy data + mask into GdkPixbuf RGBA pixels. */
	for (y = 0; y < height; y++) {
		bits = 0;
		for (x = 0; x < width; x++) {
			if (bits == 0) {
				reg = *src_data++;
				reg_m = *mask_data++;
				bits = 8;
			}
			data_b = (reg & 0x01) ? 255 : 0;
			reg >>= 1;
			mask_b = (reg_m & 0x01) ? 255 : 0;
			reg_m >>= 1;
			bits--;

			dest_data[x * 4 + 0] = data_b;
			dest_data[x * 4 + 1] = data_b;
			dest_data[x * 4 + 2] = data_b;
			dest_data[x * 4 + 3] = mask_b;
		}

		dest_data += dest_stride;
	}

	return dest;
}


typedef struct {
	const char *name;
	GdkCursorType shape;
} named_cursor_t;

static const named_cursor_t named_cursors[] = {
	{"question_arrow", GDK_QUESTION_ARROW},
	{"left_ptr", GDK_LEFT_PTR},
	{"hand", GDK_HAND1},
	{"crosshair", GDK_CROSSHAIR},
	{"dotbox", GDK_DOTBOX},
	{"pencil", GDK_PENCIL},
	{"up_arrow", GDK_SB_UP_ARROW},
	{"ul_angle", GDK_UL_ANGLE},
	{"pirate", GDK_PIRATE},
	{"xterm", GDK_XTERM},
	{"iron_cross", GDK_IRON_CROSS},
	{NULL, 0}
};

#define GHID_CURSOR_START (GDK_LAST_CURSOR+10)

void ghid_port_reg_mouse_cursor(pcb_gtk_t *ctx, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
	ghid_cursor_t *mc = vtmc_get(&ctx->mouse.cursor, idx, 1);
	if (pixel == NULL) {
		const named_cursor_t *c;

		mc->pb = NULL;
		if (name != NULL) {
			for(c = named_cursors; c->name != NULL; c++) {
				if (strcmp(c->name, name) == 0) {
					mc->shape = c->shape;
					mc->X_cursor = gdk_cursor_new(mc->shape);
					return;
				}
			}
			pcb_message(PCB_MSG_ERROR, "Failed to register named mouse cursor for tool: '%s' is unknown name\n", name);
		}
		mc->shape = GDK_LEFT_PTR; /* default */
		mc->X_cursor = gdk_cursor_new(mc->shape);
	}
	else {
		mc->shape = GHID_CURSOR_START + idx;
		mc->pb = pcb_gtk_cursor_from_xbm_data(pixel, mask, 16, 16);
		mc->X_cursor = gdk_cursor_new_from_pixbuf(gtk_widget_get_display(ctx->topwin.drawing_area), mc->pb, ICON_X_HOT, ICON_Y_HOT);
	}
}

void ghid_port_set_mouse_cursor(pcb_gtk_t *ctx, int idx)
{
	ghid_cursor_t *mc = vtmc_get(&ctx->mouse.cursor, idx, 0);
	GdkWindow *window;

	ctx->mouse.last_cursor_idx = idx;

	if (mc == NULL) {
		if (ctx->mouse.cursor.used > 0)
			pcb_message(PCB_MSG_ERROR, "Failed to set mouse cursor for unregistered tool %d\n", idx);
		return;
	}

	if (ctx->topwin.drawing_area == NULL)
		return;

	window = gtk_widget_get_window(ctx->topwin.drawing_area);

	/* check if window exists to prevent from fatal errors */
	if (window == NULL)
		return;

	if (cursor_override != 0) {
		ctx->mouse.X_cursor_shape = cursor_override;
		gdk_window_set_cursor(window, cursor_override_X);
		return;
	}

	if (ctx->mouse.X_cursor_shape == mc->shape)
		return;


	ctx->mouse.X_cursor_shape = mc->shape;
	ctx->mouse.X_cursor = mc->X_cursor;

	gdk_window_set_cursor(window, ctx->mouse.X_cursor);
}

void ghid_mode_cursor(pcb_gtk_t *ctx)
{
	ghid_port_set_mouse_cursor(ctx, ctx->mouse.last_cursor_idx);
}

void ghid_restore_cursor(pcb_gtk_t *ctx)
{
	cursor_override = 0;
	ghid_port_set_mouse_cursor(ctx, ctx->mouse.last_cursor_idx);
}

