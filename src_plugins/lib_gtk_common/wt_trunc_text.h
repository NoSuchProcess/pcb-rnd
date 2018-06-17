/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Alain Vigne
 *
 *  This module is subject to the GNU GPL as described below.
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_GTK_TRUNC_TEXT_H
#define PCB_GTK_TRUNC_TEXT_H

#include <gtk/gtk.h>
typedef struct _GtkTrunctext GtkTrunctext;
typedef struct _GtkTrunctextClass GtkTrunctextClass;
GtkTrunctext *gtk_trunctext_new(const gchar *str);

#include "lib_gtk_common/compat.h"
/* To be included in compat.h */
#ifdef PCB_GTK3
static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	return gtk_trunctext_new(str);
}
#else
/* GTK2 */
static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	return gtk_label_new(str);
}
#endif

/* GtkTrunctext is a subclass of GtkLabel */
#define TYPE_GTK_TRUNCTEXT (gtk_trunctext_get_type ())
#define GTK_TRUNCTEXT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GTK_TRUNCTEXT, GtkTrunctext))
#define GTK_TRUNCTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GTK_TRUNCTEXT, GtkTrunctextClass))
#define IS_GTK_TRUNCTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GTK_TRUNCTEXT))
#define IS_GTK_TRUNCTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GTK_TRUNCTEXT))
#define GTK_TRUNCTEXT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GTK_TRUNCTEXT, GtkTrunctextClass))

GType gtk_trunctext_get_type(void) G_GNUC_CONST;

GtkTrunctext *gtk_trunctext_construct(GType object_type, const gchar *str);

enum {
	GTK_TRUNCTEXT_0_PROPERTY,
	GTK_TRUNCTEXT_NUM_PROPERTIES
};
static GParamSpec *gtk_trunctext_properties[GTK_TRUNCTEXT_NUM_PROPERTIES];

struct _GtkTrunctext {
	GtkLabel parent_instance;
};

struct _GtkTrunctextClass {
	GtkLabelClass parent_class;
};

#endif
