/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2016, 2017, 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <librnd/core/pixmap.h>

#include "glue_common.h"

#include <librnd/core/conf.h>
#include "pcb_gtk.h"
#include <librnd/core/hidlib.h>
#include "dlg_topwin.h"
#include "hid_gtk_conf.h"
#include "in_keyboard.h"
#include "wt_preview.h"

pcb_gtk_t _ghidgui, *ghidgui = &_ghidgui;

/*** win32 workarounds ***/

#ifdef __WIN32__

#include <librnd/core/hid_init.h>
#include <librnd/core/compat_fs.h>

static void ghid_win32_init(void)
{
	char *cache, *cmd, *s;
	/* set up gdk pixmap modules - without this XPMs won't be loaded */
	cache = pcb_concat(pcb_w32_cachedir, "\\gdk-pixmap-loaders.cache", NULL);
	pcb_setenv("GDK_PIXBUF_MODULE_FILE", cache, 1);

	for(s = cache; *s != '\0'; s++)
		if (*s == '\\')
			*s = '/';
	if (!pcb_file_readable(cache)) {
		cmd = pcb_concat(pcb_w32_bindir, "\\gdk-pixbuf-query-loaders --update-cache", NULL);
		fprintf(stderr, "pcb-rnd: updating gdk loader cache: '%s'...\n", cache);
		system(cmd);
		free(cmd);
	}
	free(cache);
}
#else
static void ghid_win32_init(void) {} /* no-op on non-win32 */
#endif


/*** config ***/

static const char *cookie_menu = "gtk hid menu";

static void ghid_confchg_fullscreen(conf_native_t *cfg, int arr_idx)
{
	if (ghidgui->hid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg, int arr_idx)
{
	if ((ghidgui->hid_active) && (ghidgui->hidlib != NULL))
		ghid_update_toggle_flags(ghidgui->hidlib, &ghidgui->topwin, NULL);
}

static void ghid_confchg_cli(conf_native_t *cfg, int arr_idx)
{
	ghid_command_update_prompt(&ghidgui->topwin.cmd);
}

static void ghid_confchg_spec_color(conf_native_t *cfg, int arr_idx)
{
	if (!ghidgui->hid_active)
		return;

	if (ghidgui->impl.set_special_colors != NULL)
		ghidgui->impl.set_special_colors(cfg);
}



static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func)(conf_native_t *, int))
{
	conf_native_t *n = pcb_conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		pcb_conf_hid_set_cb(n, ghidgui->conf_id, cbs);
	}
}

static void ghid_conf_regs(const char *cookie)
{
	static conf_hid_callbacks_t cbs_fullscreen, cbs_cli[2], cbs_color[3];

	ghidgui->conf_id = pcb_conf_hid_reg(cookie, NULL);

	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", ghid_confchg_fullscreen);

	init_conf_watch(&cbs_cli[0], "rc/cli_prompt", ghid_confchg_cli);
	init_conf_watch(&cbs_cli[1], "rc/cli_backend", ghid_confchg_cli);

	init_conf_watch(&cbs_color[0], "appearance/color/background", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[1], "appearance/color/off_limit", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[2], "appearance/color/grid", ghid_confchg_spec_color);

	ghidgui->topwin.menu.ghid_menuconf_id = pcb_conf_hid_reg(cookie_menu, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
}


/*** drawing widget - output ***/

/* Do scrollbar scaling based on current port drawing area size and
   overall PCB board size. */
void pcb_gtk_tw_ranges_scale(pcb_gtk_t *gctx)
{
	pcb_gtk_topwin_t *tw = &gctx->topwin;
	pcb_gtk_view_t *view = &gctx->port.view;

	/* Update the scrollbars with PCB units. So Scale the current drawing area
	   size in pixels to PCB units and that will be the page size for the Gtk adjustment. */
	pcb_gtk_zoom_post(view);

	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->h_range)), view->width, gctx->hidlib->size_x);
	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->v_range)), view->height, gctx->hidlib->size_y);
}

void pcb_gtk_port_ranges_changed(void)
{
	GtkAdjustment *h_adj, *v_adj;

	h_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.h_range));
	v_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.v_range));
	ghidgui->port.view.x0 = gtk_adjustment_get_value(h_adj);
	ghidgui->port.view.y0 = gtk_adjustment_get_value(v_adj);

	pcb_gui->invalidate_all(pcb_gui);
}

void pcb_gtk_pan_common(void)
{
	ghidgui->topwin.adjustment_changed_holdoff = TRUE;
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.h_range), ghidgui->port.view.x0);
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.v_range), ghidgui->port.view.y0);
	ghidgui->topwin.adjustment_changed_holdoff = FALSE;

	pcb_gtk_port_ranges_changed();
}

static void command_post_entry(void)
{
#if PCB_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	pcb_gtk_interface_input_signals_connect();
#endif
	pcb_gtk_interface_set_sensitive(TRUE);
	ghid_install_accel_groups(GTK_WINDOW(ghidgui->port.top_window), &ghidgui->topwin);
	gtk_widget_grab_focus(ghidgui->port.drawing_area);
}

static void command_pre_entry(void)
{
	ghid_remove_accel_groups(GTK_WINDOW(ghidgui->port.top_window), &ghidgui->topwin);
#if PCB_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	pcb_gtk_interface_input_signals_disconnect();
#endif
	pcb_gtk_interface_set_sensitive(FALSE);
}

/*** input ***/

void pcb_gtk_interface_set_sensitive(gboolean sensitive)
{
	pcb_gtk_tw_interface_set_sensitive(&ghidgui->topwin, sensitive);
}

void pcb_gtk_mode_cursor_main(void)
{
	ghid_mode_cursor(ghidgui);
}

static void kbd_input_signals_connect(int idx, void *obj)
{
	ghidgui->key_press_handler[idx] = g_signal_connect(G_OBJECT(obj), "key_press_event", G_CALLBACK(ghid_port_key_press_cb), ghidgui);
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
   Cursor and motion events still need to be handled by the top level
   loop, so don't connect/reconnect these.
   A g_main_loop will be running when pcb-rnd wants the user to select a
   location or if command entry is needed in the status line.
   During these times normal button/key presses are intercepted, either
   by new signal handlers or the command_combo_box entry. */
void pcb_gtk_interface_input_signals_connect(void)
{
	ghidgui->button_press_handler = g_signal_connect(G_OBJECT(ghidgui->port.drawing_area), "button_press_event", G_CALLBACK(ghid_port_button_press_cb), ghidgui);
	ghidgui->button_release_handler = g_signal_connect(G_OBJECT(ghidgui->port.drawing_area), "button_release_event", G_CALLBACK(ghid_port_button_release_cb), ghidgui);
	kbd_input_signals_connect(0, ghidgui->port.drawing_area);
	kbd_input_signals_connect(3, ghidgui->topwin.left_toolbar);
}

void pcb_gtk_interface_input_signals_disconnect(void)
{
	kbd_input_signals_disconnect(0, ghidgui->port.drawing_area);
	kbd_input_signals_disconnect(3, ghidgui->topwin.left_toolbar);

	if (ghidgui->button_press_handler != 0)
		g_signal_handler_disconnect(G_OBJECT(ghidgui->port.drawing_area), ghidgui->button_press_handler);

	if (ghidgui->button_release_handler != 0)
		g_signal_handler_disconnect(ghidgui->port.drawing_area, ghidgui->button_release_handler);

	ghidgui->button_press_handler = ghidgui->button_release_handler = 0;
}

/*** misc ***/

/* import a core pixmap into a gdk pixmap */
void ghid_init_pixmap_(pcb_gtk_pixmap_t *gpm)
{
	int rowstd, nch, x, y;
	unsigned char *dst, *dst_row, *src = gpm->pxm->p;

	gpm->image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, gpm->pxm->has_transp, 8, gpm->pxm->sx, gpm->pxm->sy);
	dst_row = gdk_pixbuf_get_pixels(gpm->image);
	rowstd = gdk_pixbuf_get_rowstride(gpm->image);
	nch = gdk_pixbuf_get_n_channels(gpm->image);
	assert((nch == 3) || (nch == 4));
	for(y = 0; y < gpm->pxm->sy; y++,dst_row += rowstd) {
		dst = dst_row;
		for(x = 0; x < gpm->pxm->sx; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			if (gpm->pxm->has_transp) {
				if ((src[0] == gpm->pxm->tr) && (src[1] == gpm->pxm->tg) && (src[2] == gpm->pxm->tb))
					dst[3] = 0;
				else
					dst[3] = 255;
			}
			dst += nch;
			src += 3;
		}
	}
}

static void ghid_load_bg_image(void)
{
	static pcb_pixmap_t pxm;

	ghidgui->bg_pixmap.pxm = NULL;
	ghidgui->bg_pixmap.image = NULL;
	if (pcb_conf_hid_gtk.plugins.hid_gtk.bg_image != NULL) {
		if (pcb_pixmap_load(ghidgui->hidlib, &pxm, pcb_conf_hid_gtk.plugins.hid_gtk.bg_image) != 0) {
			pcb_message(PCB_MSG_ERROR, "Failed to load pixmap %s for background image\n", pcb_conf_hid_gtk.plugins.hid_gtk.bg_image);
			return;
		}
		ghidgui->bg_pixmap.pxm = &pxm;
		ghid_init_pixmap_(&ghidgui->bg_pixmap);
	}
}

void ghid_draw_area_update(pcb_gtk_port_t *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtkc_widget_get_window(port->drawing_area), rect, FALSE);
}

void pcb_gtk_previews_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	pcb_box_t screen;
	screen.X1 = left; screen.X2 = right;
	screen.Y1 = top; screen.Y2 = bottom;
	pcb_gtk_preview_invalidate(ghidgui, &screen);
}

void pcb_gtk_previews_invalidate_all(void)
{
	pcb_gtk_preview_invalidate(ghidgui, NULL);
}


void pcb_gtk_note_event_location(GdkEventButton *ev)
{
	gint event_x, event_y;

	if (!ev) {
		gdkc_window_get_pointer(ghidgui->port.drawing_area, &event_x, &event_y, NULL);
	}
	else {
		event_x = ev->x;
		event_y = ev->y;
	}

	pcb_gtk_coords_event2pcb(&ghidgui->port.view, event_x, event_y, &ghidgui->port.view.pcb_x, &ghidgui->port.view.pcb_y);

	pcb_hidcore_crosshair_move_to(ghidgui->port.view.ctx->hidlib, ghidgui->port.view.pcb_x, ghidgui->port.view.pcb_y, 1);
}

/*** init ***/
void ghid_glue_common_uninit(const char *cookie)
{
	pcb_event_unbind_allcookie(cookie);
	pcb_conf_hid_unreg(cookie);
	pcb_conf_hid_unreg(cookie_menu);
}

void ghid_glue_common_init(const char *cookie)
{
	ghid_win32_init();

	/* Set up the glue struct to lib_gtk_common */
	ghidgui->impl.gport = &ghidgui->port;

	ghidgui->impl.load_bg_image = ghid_load_bg_image;

	ghidgui->topwin.cmd.post_entry = command_post_entry;
	ghidgui->topwin.cmd.pre_entry = command_pre_entry;

	ghidgui->port.view.ctx = ghidgui;
	ghidgui->port.mouse = &ghidgui->mouse;

	ghid_conf_regs(cookie);
}

