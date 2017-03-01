/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "hid_cfg.h"
#include "gui-top-window.h"

#include <gdk/gdkkeysyms.h>

#include "action_helper.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "layer.h"
#include "find.h"
#include "search.h"
#include "rats.h"
#include "gtkhid-main.h"
#include "gui-top-window.h"

#include "../src_plugins/lib_gtk_common/bu_dwg_tooltip.h"
#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#warning TODO: remove
#include "gui.h"


void ghid_port_ranges_changed(pcb_gtk_topwin_t *tw)
{
	GtkAdjustment *h_adj, *v_adj;

	h_adj = gtk_range_get_adjustment(GTK_RANGE(tw->h_range));
	v_adj = gtk_range_get_adjustment(GTK_RANGE(tw->v_range));
	gport->view.x0 = gtk_adjustment_get_value(h_adj);
	gport->view.y0 = gtk_adjustment_get_value(v_adj);

	ghid_invalidate_all();
}

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

void ghid_note_event_location(GdkEventButton * ev)
{
	gint event_x, event_y;

	if (!ev) {
		gdk_window_get_pointer(gtk_widget_get_window(ghid_port.drawing_area), &event_x, &event_y, NULL);
	}
	else {
		event_x = ev->x;
		event_y = ev->y;
	}

	pcb_gtk_coords_event2pcb(&gport->view, event_x, event_y, &gport->view.pcb_x, &gport->view.pcb_y);

	pcb_event_move_crosshair(gport->view.pcb_x, gport->view.pcb_y);
	ghid_set_cursor_position_labels(&ghidgui->cps, conf_hid_gtk.plugins.hid_gtk.compact_vertical);
}

#warning TODO: move this to common
gboolean ghid_idle_cb(gpointer data)
{
	if (conf_core.editor.mode == PCB_MODE_NO)
		pcb_crosshair_set_mode(PCB_MODE_ARROW);
	ghid_mode_cursor(&gport->mouse, conf_core.editor.mode);
	if (ghidgui->topwin.mode_btn.settings_mode != conf_core.editor.mode) {
		ghid_mode_buttons_update();
	}
	ghidgui->topwin.mode_btn.settings_mode = conf_core.editor.mode;
	return FALSE;
}

gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, gpointer data)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		ghid_note_event_location(NULL);

	pcb_adjust_attached_objects();
	ghid_invalidate_all();
	g_idle_add(ghid_idle_cb, NULL);
	return FALSE;
}

gboolean ghid_port_drawing_area_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, GHidPort * out)
{
	static gboolean first_time_done;

	gport->view.canvas_width = ev->width;
	gport->view.canvas_height = ev->height;

	if (gport->pixmap)
		gdk_pixmap_unref(gport->pixmap);

	gport->pixmap = gdk_pixmap_new(gtk_widget_get_window(widget), gport->view.canvas_width, gport->view.canvas_height, -1);
	gport->drawable = gport->pixmap;

	if (!first_time_done) {
		gport->colormap = gtk_widget_get_colormap(gport->top_window);
		if (gdk_color_parse(conf_core.appearance.color.background, &gport->bg_color))
			gdk_color_alloc(gport->colormap, &gport->bg_color);
		else
			gdk_color_white(gport->colormap, &gport->bg_color);

		if (gdk_color_parse(conf_core.appearance.color.off_limit, &gport->offlimits_color))
			gdk_color_alloc(gport->colormap, &gport->offlimits_color);
		else
			gdk_color_white(gport->colormap, &gport->offlimits_color);
		first_time_done = TRUE;
		ghid_drawing_area_configure_hook(out);
		pcb_board_changed(0);
	}
	else {
		ghid_drawing_area_configure_hook(out);
	}

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	ghid_invalidate_all();
	return 0;
}

static gboolean check_object_tooltips(GHidPort *out)
{
	return pcb_gtk_dwg_tooltip_check_object(out->drawing_area, out->view.crosshair_x, out->view.crosshair_y);
}

gint ghid_port_window_motion_cb(GtkWidget * widget, GdkEventMotion * ev, GHidPort * out)
{
	gdouble dx, dy;
	static gint x_prev = -1, y_prev = -1;

	gdk_event_request_motions(ev);

	if (out->view.panning) {
		dx = gport->view.coord_per_px * (x_prev - ev->x);
		dy = gport->view.coord_per_px * (y_prev - ev->y);
		if (x_prev > 0)
			pcb_gtk_pan_view_rel(&gport->view, dx, dy);
		x_prev = ev->x;
		y_prev = ev->y;
		return FALSE;
	}
	x_prev = y_prev = -1;
	ghid_note_event_location((GdkEventButton *) ev);

	pcb_gtk_dwg_tooltip_queue(out->drawing_area, (GSourceFunc)check_object_tooltips, out);

	return FALSE;
}

gint ghid_port_window_enter_cb(GtkWidget * widget, GdkEventCrossing * ev, GHidPort * out)
{
	/* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* See comment in ghid_port_window_leave_cb() */

	if (ev->mode != GDK_CROSSING_NORMAL && ev->detail != GDK_NOTIFY_NONLINEAR) {
		return FALSE;
	}

	if (!ghidgui->cmd.command_entry_status_line_active) {
		out->view.has_entered = TRUE;
		/* Make sure drawing area has keyboard focus when we are in it.
		 */
		gtk_widget_grab_focus(out->drawing_area);
	}
	ghidgui->in_popup = FALSE;

	/* Following expression is true if a you open a menu from the menu bar,
	 * move the mouse to the viewport and click on it. This closes the menu
	 * and moves the pointer to the viewport without the pointer going over
	 * the edge of the viewport */
	if (ev->mode == GDK_CROSSING_UNGRAB && ev->detail == GDK_NOTIFY_NONLINEAR) {
		ghid_screen_update();
	}
	return FALSE;
}

gint ghid_port_window_leave_cb(GtkWidget * widget, GdkEventCrossing * ev, GHidPort * out)
{
	/* printf("leave mode: %d detail: %d\n", ev->mode, ev->detail); */

	/* Window leave events can also be triggered because of focus grabs. Some
	 * X applications occasionally grab the focus and so trigger this function.
	 * At least GNOME's window manager is known to do this on every mouse click.
	 *
	 * See http://bugzilla.gnome.org/show_bug.cgi?id=102209
	 */

	if (ev->mode != GDK_CROSSING_NORMAL) {
		return FALSE;
	}

	out->view.has_entered = FALSE;

	ghid_screen_update();

	return FALSE;
}


void ghid_confchg_line_refraction(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_all_direction_lines(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_flip(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_fullscreen(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_update_toggle_flags(&ghidgui->topwin);
}
