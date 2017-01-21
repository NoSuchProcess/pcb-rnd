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

#define GHID_TYPE_PINOUT_PREVIEW           (pcb_gtk_preview_get_type())
#define GHID_PINOUT_PREVIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_t))
#define GHID_PINOUT_PREVIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_class_t))
#define GHID_IS_PINOUT_PREVIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_PINOUT_PREVIEW))
#define GHID_PINOUT_PREVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_PINOUT_PREVIEW, pcb_gtk_preview_class_t))

typedef struct pcb_gtk_preview_class_s pcb_gtk_preview_class_t;
typedef struct pcb_gtk_preview_s pcb_gtk_preview_t;


struct pcb_gtk_preview_class_s {
	GtkDrawingAreaClass parent_class;
};

struct pcb_gtk_preview_s {
	GtkDrawingArea parent_instance;

	pcb_element_t element;					/* element data to display */
	gint x_max, y_max;
	gint w_pixels, h_pixels;			/* natural size of element preview */

	void *gport;
};


GType pcb_gtk_preview_get_type(void);

/* Query the natural size of a preview widget */
void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t * pinout, int *width, int *height);

GtkWidget *pcb_gtk_preview_pinout_new(void *gport, pcb_element_t *element);


#endif /* PCB_GTK_WT_REVIEW_H */
