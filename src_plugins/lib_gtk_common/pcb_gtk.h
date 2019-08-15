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
#include "conf.h"

typedef struct pcb_gtk_port_s  pcb_gtk_port_t;
typedef struct pcb_gtk_s pcb_gtk_t;
typedef struct pcb_gtk_mouse_s pcb_gtk_mouse_t;
typedef struct pcb_gtk_topwin_s pcb_gtk_topwin_t;
typedef struct pcb_gtk_impl_s pcb_gtk_impl_t;

extern pcb_gtk_t _ghidgui, *ghidgui;

#include <gtk/gtk.h>
#include "compat.h"

/* The HID using pcb_gtk_common needs to fill in this struct and pass it
   on to most of the calls. This is the only legal way pcb_gtk_common can
   back reference to the HID. This lets multiple HIDs use gtk_common code
   without linker errors. */
struct pcb_gtk_impl_s {
	void *gport;      /* Opaque pointer back to the HID's internal struct - used when common calls a HID function */

	/* rendering */
	void (*drawing_realize)(GtkWidget *w, void *gport);
	gboolean (*drawing_area_expose)(GtkWidget *w, pcb_gtk_expose_t *p, void *gport);
	void (*drawing_area_configure_hook)(void *);

	GtkWidget *(*new_drawing_widget)(pcb_gtk_impl_t *impl);
	void (*init_drawing_widget)(GtkWidget *widget, void *gport);
	gboolean (*preview_expose)(GtkWidget *widget, pcb_gtk_expose_t *p, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx); /* p == NULL when called from the code, not from a GUI expose event */
	void (*load_bg_image)(void);
	void (*init_renderer)(int *argc, char ***argv, void *port);
	void (*draw_grid_local)(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy);

	/* screen */
	void (*screen_update)(void);
	void (*shutdown_renderer)(void *port);

	pcb_bool (*map_color)(const pcb_color_t *inclr, pcb_gtk_color_t *color);
	const gchar *(*get_color_name)(pcb_gtk_color_t *color);

	void (*set_special_colors)(conf_native_t *cfg);
};

#include "../src_plugins/lib_gtk_common/ui_zoompan.h"

typedef struct {
	GdkCursorType shape;
	GdkCursor *X_cursor;
	GdkPixbuf *pb;
} ghid_cursor_t;

#define GVT(x) vtmc_ ## x
#define GVT_ELEM_TYPE ghid_cursor_t
#define GVT_SIZE_TYPE int
#define GVT_DOUBLING_THRS 256
#define GVT_START_SIZE 8
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

struct pcb_gtk_mouse_s {
	GdkCursor *X_cursor;          /* used X cursor */
	GdkCursorType X_cursor_shape; /* and its shape */
	vtmc_t cursor;
	int last_cursor_idx; /* tool index of the tool last selected */
};

#include "bu_menu.h"
#include "bu_command.h"

struct pcb_gtk_topwin_s {
	/* util/builder states */
	pcb_gtk_menu_ctx_t menu;
	pcb_hid_cfg_t *ghid_cfg;
	pcb_gtk_command_t cmd;

	/* own widgets */
	GtkWidget *drawing_area;
	GtkWidget *bottom_hbox;

	GtkWidget *top_hbox, *top_bar_background, *menu_hbox, *position_hbox, *menubar_toolbar_vbox;
	GtkWidget *left_toolbar;
	GtkWidget *layer_selector;
	GtkWidget *vbox_middle, *hpaned_middle;

	GtkWidget *h_range, *v_range;
	GObject *h_adjustment, *v_adjustment;

	/* own internal states */
	gboolean adjustment_changed_holdoff;
	gboolean small_label_markup;
	int active; /* 0 before init finishes */

	/* docking */
	GtkWidget *dockbox[PCB_HID_DOCK_max];
	gdl_list_t dock[PCB_HID_DOCK_max];
};

/* needed for a type in pcb_gtk_t - DO NOT ADD .h files that are not requred for the structs! */
#include "../src_plugins/lib_gtk_common/dlg_topwin.h"

#include "event.h"
#include "conf_hid.h"
#include "pcb_bool.h"

typedef struct pcb_gtk_pixmap_s {
	pcb_pixmap_t *pxm;        /* core-side pixmap (raw input image) */
	GdkPixbuf *image;         /* input image converted to gdk */
	int w, h;                 /* source image dimensions in ->image (for faster access) */

	/* backend/renderer cache */
	int h_scaled, w_scaled;  /* current scale of the chached image (for gdk) */
	union {
		GdkPixbuf *pb;         /* for gdk */
		unsigned long int lng; /* for opengl */
	} cache;
} pcb_gtk_pixmap_t;

/* The output viewport */
struct pcb_gtk_port_s {
	GtkWidget *top_window,        /* toplevel widget */
	          *drawing_area;      /* and its drawing area */

	pcb_bool drawing_allowed;     /* track if a drawing area is available for rendering */

	struct render_priv_s *render_priv;

	pcb_gtk_mouse_t *mouse;

	pcb_gtk_view_t view;
};

struct pcb_gtk_s {
	pcb_gtk_impl_t impl;
	pcb_gtk_port_t port;

	pcb_hidlib_t *hidlib;

	GtkWidget *wtop_window;
	GtkActionGroup *main_actions;

	pcb_gtk_topwin_t topwin;
	conf_hid_id_t conf_id;

	pcb_gtk_pixmap_t bg_pixmap;

	int hid_active; /* 1 if the currently running hid (pcb_gui) is us */
	int gui_is_up; /*1 if all parts of the gui is up and running */

	gulong button_press_handler, button_release_handler, key_press_handler[5], key_release_handler[5];

	pcb_gtk_mouse_t mouse;

	gdl_list_t previews; /* all widget lists */
};

#endif /* PCB_HID_GTK_GHID_GUI_H */
