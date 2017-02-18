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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_HID_GTK_GUI_DRC_WINDOW_H
#define PCB_HID_GTK_GUI_DRC_WINDOW_H

#include <gtk/gtk.h>

#define GHID_TYPE_DRC_VIOLATION           (ghid_drc_violation_get_type())
#define GHID_DRC_VIOLATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_DRC_VIOLATION, GhidDrcViolation))
#define GHID_DRC_VIOLATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_DRC_VIOLATION, GhidDrcViolationClass))
#define GHID_IS_DRC_VIOLATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_DRC_VIOLATION))
#define GHID_DRC_VIOLATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_DRC_VIOLATION, GhidDrcViolationClass))

typedef struct _GhidDrcViolationClass GhidDrcViolationClass;
typedef struct _GhidDrcViolation GhidDrcViolation;


struct _GhidDrcViolationClass {
	GObjectClass parent_class;
};

struct _GhidDrcViolation {
	GObject parent_instance;

	char *title;
	char *explanation;
	pcb_coord_t x_coord;
	pcb_coord_t y_coord;
	pcb_angle_t angle;
	pcb_bool have_measured;
	pcb_coord_t measured_value;
	pcb_coord_t required_value;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	GdkDrawable *pixmap;
};


GType ghid_drc_violation_get_type(void);

GhidDrcViolation *ghid_drc_violation_new(pcb_drc_violation_t * violation, GdkDrawable * pixmap);


#define GHID_TYPE_VIOLATION_RENDERER           (ghid_violation_renderer_get_type())
#define GHID_VIOLATION_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_TYPE_VIOLATION_RENDERER, GhidViolationRenderer))
#define GHID_VIOLATION_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GHID_TYPE_VIOLATION_RENDERER, GhidViolationRendererClass))
#define GHID_IS_VIOLATION_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_TYPE_VIOLATION_RENDERER))
#define GHID_VIOLATION_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GHID_TYPE_VIOLATION_RENDERER, GhidViolationRendererClass))

typedef struct _GhidViolationRendererClass GhidViolationRendererClass;
typedef struct _GhidViolationRenderer GhidViolationRenderer;


struct _GhidViolationRendererClass {
	GtkCellRendererTextClass parent_class;
};

struct _GhidViolationRenderer {
	GtkCellRendererText parent_instance;

	GhidDrcViolation *violation;
};


GType ghid_violation_renderer_get_type(void);

GtkCellRenderer *ghid_violation_renderer_new(void);

void ghid_drc_window_show(gboolean raise);
void ghid_drc_window_reset_message(void);
void ghid_drc_window_append_violation(pcb_drc_violation_t * violation);
void ghid_drc_window_append_messagev(const char *fmt, va_list va);
int ghid_drc_window_throw_dialog(void);

/* Temporary back reference to hid_gtk */
GdkPixmap *ghid_render_pixmap(int cx, int cy, double zoom, int width, int height, int depth);

#endif /* PCB_HID_GTK_GUI_DRC_WINDOW_H */
