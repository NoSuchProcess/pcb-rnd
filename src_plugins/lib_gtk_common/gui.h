/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/glue.h"

#include "event.h"
#include "conf_hid.h"
#include "pcb_bool.h"

struct GhidGui_s {
	GtkActionGroup *main_actions;

	pcb_gtk_topwin_t topwin;
	pcb_gtk_common_t common;
	conf_hid_id_t conf_id;

	GdkPixbuf *bg_pixbuf; /* -> renderer */

	int hid_active; /* 1 if the currently running hid (pcb_gui) is us */
	int gui_is_up; /*1 if all parts of the gui is up and running */

	gulong button_press_handler, button_release_handler, key_press_handler[5], key_release_handler[5];
};

/* The output viewport */
struct GHidPort_s {
	GtkWidget *top_window,				/* toplevel widget              */
	 *drawing_area;								/* and its drawing area */

	pcb_bool drawing_allowed;     /* track if a drawing area is available for rendering */

	struct render_priv_s *render_priv;

	pcb_gtk_mouse_t mouse;

	pcb_gtk_view_t view;
};

extern GHidPort ghid_port, *gport;

#endif /* PCB_HID_GTK_GHID_GUI_H */
