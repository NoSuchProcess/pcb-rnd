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

#include "glue_common.h"

#include "conf.h"
#include "gui.h"
#include "common.h"
#include "hidlib.h"
#include "dlg_topwin.h"
#include "hid_gtk_conf.h"
#include "wt_preview.h"

static void ghid_interface_set_sensitive(gboolean sensitive);

/*** win32 workarounds ***/

/* Needed for finding the windows installation directory. Without that
   we can't find our fonts and footprint libraries. */
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif

static void ghid_win32_init(void)
{
TODO("Should not use PCB_PACKAGE but pcbhl_app_*");
#ifdef WIN32
	char *tmps;
	char *share_dir;
	char *loader_cache;
	FILE *loader_file;

	tmps = g_win32_get_package_installation_directory(PCB_PACKAGE "-" PCB_VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PCB_PACKAGE
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
}


/*** config ***/

static const char *cookie_menu = "gtk hid menu";

static void ghid_confchg_fullscreen(conf_native_t *cfg, int arr_idx)
{
	if (ghidgui->hid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg, int arr_idx)
{
	if (ghidgui->hid_active)
		ghid_update_toggle_flags(&ghidgui->topwin, NULL);
}

static void ghid_confchg_cli(conf_native_t *cfg, int arr_idx)
{
	ghid_command_update_prompt(&ghidgui->topwin.cmd);
}

static void ghid_confchg_spec_color(conf_native_t *cfg, int arr_idx)
{
	if (!ghidgui->hid_active)
		return;

	if (ghidgui->common.set_special_colors != NULL)
		ghidgui->common.set_special_colors(cfg);
}



static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func)(conf_native_t *, int))
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		conf_hid_set_cb(n, ghidgui->conf_id, cbs);
	}
}

static void ghid_conf_regs(const char *cookie)
{
	static conf_hid_callbacks_t cbs_fullscreen, cbs_cli[2], cbs_color[3];

	ghidgui->conf_id = conf_hid_reg(cookie, NULL);

	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", ghid_confchg_fullscreen);

	init_conf_watch(&cbs_cli[0], "rc/cli_prompt", ghid_confchg_cli);
	init_conf_watch(&cbs_cli[1], "rc/cli_backend", ghid_confchg_cli);

	init_conf_watch(&cbs_color[0], "appearance/color/background", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[1], "appearance/color/off_limit", ghid_confchg_spec_color);
	init_conf_watch(&cbs_color[2], "appearance/color/grid", ghid_confchg_spec_color);

	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(cookie_menu, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
}


/*** drawing widget - output ***/

static void ghid_port_ranges_scale(void)
{
	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
}

static void ghid_port_ranges_changed(void)
{
	GtkAdjustment *h_adj, *v_adj;

	h_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.h_range));
	v_adj = gtk_range_get_adjustment(GTK_RANGE(ghidgui->topwin.v_range));
	gport->view.x0 = gtk_adjustment_get_value(h_adj);
	gport->view.y0 = gtk_adjustment_get_value(v_adj);

	pcb_gui->invalidate_all(ghidgui->common.hidlib);
}

static void ghid_pan_common(void)
{
	ghidgui->topwin.adjustment_changed_holdoff = TRUE;
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.h_range), gport->view.x0);
	gtk_range_set_value(GTK_RANGE(ghidgui->topwin.v_range), gport->view.y0);
	ghidgui->topwin.adjustment_changed_holdoff = FALSE;

	ghid_port_ranges_changed();
}

int ghid_command_entry_is_active(void)
{
	return ghidgui->topwin.cmd.command_entry_status_line_active;
}

static void command_post_entry(void)
{
#if PCB_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	ghid_interface_input_signals_connect();
#endif
	ghid_interface_set_sensitive(TRUE);
	ghid_install_accel_groups(GTK_WINDOW(gport->top_window), &ghidgui->topwin);
	gtk_widget_grab_focus(gport->drawing_area);
}

static void command_pre_entry(void)
{
	ghid_remove_accel_groups(GTK_WINDOW(gport->top_window), &ghidgui->topwin);
#if PCB_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	ghid_interface_input_signals_disconnect();
#endif
	ghid_interface_set_sensitive(FALSE);
}

/*** input ***/

static void ghid_interface_set_sensitive(gboolean sensitive)
{
	pcb_gtk_tw_interface_set_sensitive(&ghidgui->topwin, sensitive);
}

static void ghid_port_button_press_main(void)
{
	pcb_gui->invalidate_all(ghidgui->common.hidlib);
	if (!gport->view.panning)
		g_idle_add(ghid_idle_cb, &ghidgui->topwin);
}

static void ghid_port_button_release_main(void)
{
	pcb_hidlib_adjust_attached_objects();
	pcb_gui->invalidate_all(ghidgui->common.hidlib);

	g_idle_add(ghid_idle_cb, &ghidgui->topwin);
}

static void ghid_mode_cursor_main(void)
{
	ghid_mode_cursor(&gport->mouse);
}

/*** misc ***/
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

static void ghid_main_destroy(void *port)
{
	ghidgui->common.shutdown_renderer(port);
	gtk_main_quit();
}

void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

void pcb_gtk_previews_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	pcb_box_t screen;
	screen.X1 = left; screen.X2 = right;
	screen.Y1 = top; screen.Y2 = bottom;
	pcb_gtk_preview_invalidate(&ghidgui->common, &screen);
}

void pcb_gtk_previews_invalidate_all(void)
{
	pcb_gtk_preview_invalidate(&ghidgui->common, NULL);
}


/*** init ***/
void ghid_glue_common_uninit(const char *cookie)
{
	pcb_event_unbind_allcookie(cookie);
	conf_hid_unreg(cookie);
	conf_hid_unreg(cookie_menu);
}

void ghid_glue_common_init(const char *cookie)
{
	ghid_win32_init();

	/* Set up the glue struct to lib_gtk_common */
	ghidgui->common.gport = &ghid_port;
	ghidgui->common.note_event_location = ghid_note_event_location;
	ghidgui->common.shift_is_pressed = ghid_shift_is_pressed;
	ghidgui->common.interface_input_signals_disconnect = ghid_interface_input_signals_disconnect;
	ghidgui->common.interface_input_signals_connect = ghid_interface_input_signals_connect;
	ghidgui->common.interface_set_sensitive = ghid_interface_set_sensitive;
	ghidgui->common.port_button_press_main = ghid_port_button_press_main;
	ghidgui->common.port_button_release_main = ghid_port_button_release_main;
	ghidgui->common.mode_cursor_main = ghid_mode_cursor_main;
	ghidgui->common.pan_common = ghid_pan_common;
	ghidgui->common.port_ranges_scale = ghid_port_ranges_scale;

	ghidgui->common.command_entry_is_active = ghid_command_entry_is_active;
	ghidgui->common.load_bg_image = ghid_load_bg_image;
	ghidgui->common.main_destroy = ghid_main_destroy;
	ghidgui->common.port_ranges_changed = ghid_port_ranges_changed;

	ghidgui->topwin.cmd.com = &ghidgui->common;
	ghidgui->topwin.cmd.post_entry = command_post_entry;
	ghidgui->topwin.cmd.pre_entry = command_pre_entry;

	ghid_port.view.com = &ghidgui->common;
	ghid_port.mouse.com = &ghidgui->common;

	ghid_conf_regs(cookie);
}

