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
 *
 */

#ifndef PCB_GTK_DLG_DRC_H
#define PCB_GTK_DLG_DRC_H

#include <gtk/gtk.h>
#include "glue.h"

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
};

typedef struct pcb_gtk_drcwin_s {
	GtkWidget *drc_window;
	GtkWidget *drc_vbox;
	int num_violations;
	
} pcb_gtk_drcwin_t;

GType ghid_drc_violation_get_type(void);

GhidDrcViolation *ghid_drc_violation_new(pcb_drc_violation_t * violation);

/*TODO: split in dlg_drc_cr.h */
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
	pcb_gtk_common_t *common;
};

GType ghid_violation_renderer_get_type(void);

GtkCellRenderer *ghid_violation_renderer_new(pcb_gtk_common_t *common);

void pcb_gtk_drcwin_init(pcb_gtk_drcwin_t *drcwin);
void ghid_drc_window_show(pcb_gtk_drcwin_t *drcwin, gboolean raise);
void ghid_drc_window_reset_message(pcb_gtk_drcwin_t *drcwin);
void ghid_drc_window_append_violation(pcb_gtk_drcwin_t *drcwin, pcb_gtk_common_t *common, pcb_drc_violation_t * violation);
void ghid_drc_window_append_messagev(const char *fmt, va_list va);
int ghid_drc_window_throw_dialog(pcb_gtk_drcwin_t *drcwin);

#endif /* PCB_GTK_DLG_DRC_H */
