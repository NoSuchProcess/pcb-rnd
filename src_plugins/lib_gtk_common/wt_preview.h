/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file was originally written by Peter Clifton
   then got a major refactoring by Tibor 'Igor2' Palinkas in pcb-rnd
*/

#ifndef PCB_GTK_WT_REVIEW_H
#define PCB_GTK_WT_REVIEW_H

#include <gtk/gtk.h>
#include "obj_elem.h"
#include "hid.h"
#include "layer.h"
#include "ui_zoompan.h"
#include "glue.h"

#define GHID_TYPE_PINOUT_PREVIEW           (pcb_gtk_preview_get_type())
#define GHID_PINOUT_PREVIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_t))
#define GHID_PINOUT_PREVIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_class_t))
#define GHID_IS_PINOUT_PREVIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_PINOUT_PREVIEW))
#define GHID_PINOUT_PREVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_class_t))

/** The _preview_ widget Class. */
typedef struct pcb_gtk_preview_class_s pcb_gtk_preview_class_t;
/** A _preview_ widget. */
typedef struct pcb_gtk_preview_s pcb_gtk_preview_t;

/** The _preview_ widget Class data. */
struct pcb_gtk_preview_class_s {
	GtkDrawingAreaClass parent_class;
};

/* Forward declarations ... event functions */
/** \todo understand where is this initialized ? */
typedef void (*pcb_gtk_init_drawing_widget_t) (GtkWidget * widget, void *port);
/** Expose event, set as a property. */
typedef gboolean(*pcb_gtk_preview_expose_t) (GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall,
																						 const pcb_hid_expose_ctx_t * ctx);
/** \todo How this is initialized ? Where is it going ? */
typedef pcb_bool(*pcb_gtk_preview_mouse_ev_t) (void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);

/** Selects the kind of preview. */
typedef enum pcb_gtk_preview_kind_e {
	PCB_GTK_PREVIEW_INVALID,
	PCB_GTK_PREVIEW_PINOUT,	/**<- render a single element     */
	PCB_GTK_PREVIEW_LAYER,	/**<- render a specific layer     */

	PCB_GTK_PREVIEW_kind_max
} pcb_gtk_preview_kind_t;

/** The _preview_ widget data. */
struct pcb_gtk_preview_s {
	GtkDrawingArea parent_instance;

	pcb_hid_expose_ctx_t expose_data;
	pcb_gtk_view_t view;

	pcb_coord_t x_max, y_max;			/* for the element preview only       */
	gint win_w, win_h;
	gint w_pixels, h_pixels;			/* natural size of element preview    */

	void *gport;
	pcb_gtk_init_drawing_widget_t init_drawing_widget;
	pcb_gtk_preview_expose_t expose;
	pcb_gtk_preview_kind_t kind;
	pcb_gtk_preview_mouse_ev_t mouse_cb;
	pcb_hid_expose_t overlay_draw_cb;
	pcb_coord_t grabx, graby;

	pcb_element_t element;

	pcb_gtk_common_t *com;
};

/** Retrieves \ref pcb_gtk_preview_t 's GType identifier.
    Upon first call, this registers the pcb_gtk_preview_t in the GType system.
    Subsequently it returns the saved value from its first execution.

    \return     the GType identifier associated with \ref pcb_gtk_preview_t.
 */
GType pcb_gtk_preview_get_type(void);

/** Queries the natural size of a preview widget */
void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t * pinout, int *width, int *height);

/** Creates and returns a new freshly-allocated \ref pcb_gtk_preview_t widget.
    \param  init_widget       ?
    \param  expose            Drawing event call-back function ?
    \param  element           ?
\todo rename to pcb_gtk_preview_new */
GtkWidget *pcb_gtk_preview_pinout_new(pcb_gtk_common_t *com,
																			pcb_gtk_init_drawing_widget_t init_widget,
																			pcb_gtk_preview_expose_t expose, pcb_element_t * element);

/** Creates and returns a new freshly-allocated \ref pcb_gtk_preview_t widget,
    using \p layer... for What ?
 */
GtkWidget *pcb_gtk_preview_layer_new(pcb_gtk_common_t *com,
																		 pcb_gtk_init_drawing_widget_t init_widget,
																		 pcb_gtk_preview_expose_t expose, pcb_layer_id_t layer);

#endif /* PCB_GTK_WT_REVIEW_H */
