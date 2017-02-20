/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2004 Ales V. Hvezda
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef PCB_GTK_LIBRARY_WINDOW_H
#define PCB_GTK_LIBRARY_WINDOW_H

#include <gtk/gtk.h>
#include "hid.h"

#define GHID_TYPE_LIBRARY_WINDOW           (pcb_gtk_library_get_type())
#define GHID_LIBRARY_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_LIBRARY_WINDOW, pcb_gtk_library_t))
#define GHID_LIBRARY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_LIBRARY_WINDOW, pcb_gtk_library_class_t))
#define GHID_IS_LIBRARY_WINDOW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_LIBRARY_WINDOW))
#define GHID_GET_LIBRARY_WINDOW_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_LIBRARY, pcb_gtk_library_class_t))

/** The _library_ widget Class. A subclass of GtkDialogClass. */
typedef struct pcb_gtk_library_class_s pcb_gtk_library_class_t;
/** The _library_ widget. */
typedef struct pcb_gtk_library_s pcb_gtk_library_t;

/** The widget Class data. */
struct pcb_gtk_library_class_s {
	GtkDialogClass parent_class;
};

/** The widget data. */
struct pcb_gtk_library_s {
	GtkDialog parent_instance;

	GtkWidget *hpaned;
	GtkTreeView *libtreeview;
	GtkNotebook *viewtabs;
	GtkWidget *preview;
	GtkWidget *preview_text;
	GtkEntry *entry_filter;
	GtkButton *button_clear;
	guint filter_timeout;

	void *gport;
};

/** \return     the GType identifier associated with \ref pcb_gtk_library_t. */
GType pcb_gtk_library_get_type(void);

/** Creates the _library_ dialog if it is not already created.
    It does not show the dialog ; use \ref pcb_gtk_library_show () for that. */
void pcb_gtk_library_create(void *gport);

/** Shows the _library_ dialog, creating it if it is not already created, and
    presents it to the user (brings it to the front with focus), depending on 
    \p raise.
    \param gport    Temporary ?
    \param raise    if `TRUE`, presents the window.
 */
void pcb_gtk_library_show(void *gport, gboolean raise);

/* Temporary back references to hid_gtk */
gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall,
														 const pcb_hid_expose_ctx_t * ctx);
void ghid_init_drawing_widget(GtkWidget * widget, void *gport);

#endif /* PCB_GTK_LIBRARY_WINDOW_H */
