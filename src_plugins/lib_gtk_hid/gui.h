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

#include "hid.h"

typedef struct GHidPort_s  GHidPort;
typedef struct GhidGui_s GhidGui;
extern GhidGui _ghidgui, *ghidgui;

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

struct GhidGui_s {
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
};

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

#endif /* PCB_HID_GTK_GHID_GUI_H */
