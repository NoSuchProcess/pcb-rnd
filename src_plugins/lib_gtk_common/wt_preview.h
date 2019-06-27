/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *
 */

/* This file was originally written by Peter Clifton
   then got a major refactoring by Tibor 'Igor2' Palinkas and Alain in pcb-rnd
*/

#ifndef PCB_GTK_WT_REVIEW_H
#define PCB_GTK_WT_REVIEW_H

#include <gtk/gtk.h>
#include <genlist/gendlist.h>
#include "hid.h"
#include "ui_zoompan.h"
#include "glue.h"
#include "compat.h"

#define PCB_GTK_TYPE_PREVIEW           (pcb_gtk_preview_get_type())
#define PCB_GTK_PREVIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), PCB_GTK_TYPE_PREVIEW, pcb_gtk_preview_t))
#define PCB_GTK_PREVIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  PCB_GTK_TYPE_PREVIEW, pcb_gtk_preview_class_t))
#define PCB_GTK_IS_PREVIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PCB_GTK_TYPE_PREVIEW))
#define PCB_GTK_PREVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  PCB_GTK_TYPE_PREVIEW, pcb_gtk_preview_class_t))

typedef struct pcb_gtk_preview_class_s pcb_gtk_preview_class_t;
typedef struct pcb_gtk_preview_s pcb_gtk_preview_t;

struct pcb_gtk_preview_class_s {
	GtkDrawingAreaClass parent_class;
};

typedef void (*pcb_gtk_init_drawing_widget_t)(GtkWidget *widget, void *port);
typedef void (*pcb_gtk_preview_config_t)(pcb_gtk_preview_t *gp, GtkWidget *widget);
typedef gboolean(*pcb_gtk_preview_expose_t)(GtkWidget *widget, pcb_gtk_expose_t *ev, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx);
typedef pcb_bool(*pcb_gtk_preview_mouse_ev_t)(void *widget, void *draw_data, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);

struct pcb_gtk_preview_s {
	GtkDrawingArea parent_instance;

	pcb_hid_expose_ctx_t expose_data;
	pcb_gtk_view_t view;

	pcb_coord_t x_min, y_min, x_max, y_max;   /* for the obj preview: bounding box */
	gint w_pixels, h_pixels;                  /* natural size of object preview */
	gint win_w, win_h;

	pcb_coord_t xoffs, yoffs; /* difference between desired x0;y0 and the actual window's top left coords */

	void *gport;
	pcb_gtk_init_drawing_widget_t init_drawing_widget;
	pcb_gtk_preview_config_t config_cb;
	pcb_gtk_preview_expose_t expose;
	pcb_gtk_preview_mouse_ev_t mouse_cb;
	pcb_hid_expose_t overlay_draw_cb;
	pcb_coord_t grabx, graby;
	time_t grabt;
	long grabmot;

	pcb_any_obj_t *obj; /* object being displayed in the preview */

	pcb_gtk_common_t *com;
	gdl_elem_t link; /* in the list of all previews in ->com->previews */
	unsigned redraw_with_board:1;
	unsigned redrawing:1;
};

GType pcb_gtk_preview_get_type(void);

/* Queries the natural size of a preview widget */
void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t *preview, int *width, int *height);

GtkWidget *pcb_gtk_preview_new(pcb_gtk_common_t *com, pcb_gtk_init_drawing_widget_t init_widget,
																			pcb_gtk_preview_expose_t expose, pcb_hid_expose_t dialog_draw, pcb_gtk_preview_config_t config, void *draw_data);

void pcb_gtk_preview_zoomto(pcb_gtk_preview_t *preview, const pcb_box_t *data_view);

/* invalidate (redraw) all preview widgets whose current view overlaps with
   the screen box; if screen is NULL, redraw all */
void pcb_gtk_preview_invalidate(pcb_gtk_common_t *com, const pcb_box_t *screen);

#endif /* PCB_GTK_WT_REVIEW_H */
