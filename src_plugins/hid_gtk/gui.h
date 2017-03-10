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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef PCB_HID_GTK_GHID_H
#define PCB_HID_GTK_GHID_H

typedef struct GHidPort_s  GHidPort;

#include "hid.h"

#include <gtk/gtk.h>

/* needed for a type in GhidGui - DO NOT ADD .h files that are not requred for the structs! */
#include "../src_plugins/lib_gtk_common/ui_zoompan.h"
#include "../src_plugins/lib_gtk_common/dlg_propedit.h"
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/glue.h"

#include "board.h"
#include "event.h"
#include "conf_hid.h"
#include "render.h"

typedef struct GhidGui_s {
	GtkActionGroup *main_actions;

	pcb_gtk_topwin_t topwin;
	pcb_gtk_common_t common;
	conf_hid_id_t conf_id;

	GdkPixbuf *bg_pixbuf; /* -> renderer */

	pcb_gtk_dlg_propedit_t propedit_dlg;
	GtkWidget *propedit_widget;

	int hid_active; /* 1 if the currently running hid (pcb_gui) is us */
	int gui_is_up; /*1 if all parts of the gui is up and running */

	gulong button_press_handler, button_release_handler, key_press_handler, key_release_handler;
} GhidGui;

extern GhidGui _ghidgui, *ghidgui;


/* The output viewport */
struct GHidPort_s {
	GtkWidget *top_window,				/* toplevel widget              */
	 *drawing_area;								/* and its drawing area */
	GdkPixmap *pixmap, *mask;
	GdkDrawable *drawable;				/* Current drawable for drawing routines */

	struct render_priv *render_priv;

	GdkColor bg_color, offlimits_color, grid_color;

	GdkColormap *colormap;

	pcb_gtk_mouse_t mouse;

	pcb_gtk_view_t view;

	pcb_lead_user_t lead_user;
};

extern GHidPort ghid_port, *gport;

/* ***  */
void ghid_range_control(GtkWidget * box, GtkWidget ** scale_res,
												gboolean horizontal, GtkPositionType pos,
												gboolean set_draw_value, gint digits,
												gboolean pack_start, gboolean expand, gboolean fill,
												guint pad, gfloat value, gfloat low, gfloat high,
												gfloat step0, gfloat step1, void (*cb_func) (), gpointer data);

pcb_lib_menu_t *ghid_get_net_from_node_name(const gchar * name, gboolean);
void ghid_netlist_highlight_node(const gchar * name);

extern pcb_hid_t ghid_hid;

static inline void ghid_invalidate_all(void) { ghid_hid.invalidate_all(); }
static inline void ghid_screen_update(void) { ghidgui->common.screen_update(); }
static inline void ghid_shutdown_renderer(void *port) { ghidgui->common.shutdown_renderer(port); }
static inline void ghid_drawing_area_configure_hook(void *out) { ghidgui->common.drawing_area_configure_hook(out); }

/* Coordinate conversions */
#include "compat_misc.h"
#include "conf_core.h"

/* Px converts view->pcb, Vx converts pcb->view */
static inline int Vx(pcb_coord_t x)
{
	double rv;
	if (conf_core.editor.view.flip_x)
		rv = (PCB->MaxWidth - x - gport->view.x0) / gport->view.coord_per_px + 0.5;
	else
		rv = (x - gport->view.x0) / gport->view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vy(pcb_coord_t y)
{
	double rv;
	if (conf_core.editor.view.flip_y)
		rv = (PCB->MaxHeight - y - gport->view.y0) / gport->view.coord_per_px + 0.5;
	else
		rv = (y - gport->view.y0) / gport->view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vz(pcb_coord_t z)
{
	return pcb_round((double)z / gport->view.coord_per_px + 0.5);
}

static inline double Vxd(pcb_coord_t x)
{
	double rv;
	if (conf_core.editor.view.flip_x)
		rv = (PCB->MaxWidth - x - gport->view.x0) / gport->view.coord_per_px;
	else
		rv = (x - gport->view.x0) / gport->view.coord_per_px;
	return rv;
}

static inline double Vyd(pcb_coord_t y)
{
	double rv;
	if (conf_core.editor.view.flip_y)
		rv = (PCB->MaxHeight - y - gport->view.y0) / gport->view.coord_per_px;
	else
		rv = (y - gport->view.y0) / gport->view.coord_per_px;
	return rv;
}

static inline double Vzd(pcb_coord_t z)
{
	return (double)z / gport->view.coord_per_px;
}

static inline pcb_coord_t Px(int x)
{
	pcb_coord_t rv = x * gport->view.coord_per_px + gport->view.x0;
	if (conf_core.editor.view.flip_x)
		rv = PCB->MaxWidth - (x * gport->view.coord_per_px + gport->view.x0);
	return rv;
}

static inline pcb_coord_t Py(int y)
{
	pcb_coord_t rv = y * gport->view.coord_per_px + gport->view.y0;
	if (conf_core.editor.view.flip_y)
		rv = PCB->MaxHeight - (y * gport->view.coord_per_px + gport->view.y0);
	return rv;
}

static inline pcb_coord_t Pz(int z)
{
	return (z * gport->view.coord_per_px);
}

extern const char *ghid_cookie;
extern const char *ghid_menu_cookie;

void hid_gtk_wgeo_update(void);

static inline void ghid_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy) { ghidgui->common.draw_grid_local(cx, cy); }

#endif /* PCB_HID_GTK_GHID_GUI_H */
