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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* This widget was for the gtk3 port - gtk2 does not use it at the moment
   so it is not compiled. A gtk4 or gtkN port may need it. */

#include "wt_trunc_text.h"

static gpointer gtk_trunctext_parent_class = NULL;

GtkTrunctext *gtk_trunctext_construct(GType object_type, const gchar *str)
{
	GtkTrunctext *self = NULL;
	self = (GtkTrunctext *)g_object_new(object_type, NULL);

	if (str && *str)
		gtk_label_set_text(GTK_LABEL(self), str);

	return self;
}

GtkTrunctext *gtk_trunctext_new(const gchar *str)
{
	return gtk_trunctext_construct(TYPE_GTK_TRUNCTEXT, str);
}

static void gtk_trunctext_get_preferred_width(GtkWidget *widget, gint *minimum_size, gint *natural_size)
{
	*minimum_size = 1;
	*natural_size = 1;
}

static void gtk_trunctext_get_preferred_width_for_height(GtkWidget *widget, gint height, gint *minimum_width, gint *natural_width)
{
	*minimum_width = 1;
	*natural_width = 1;
}

static void gtk_trunctext_get_preferred_height_and_baseline_for_width(GtkWidget *widget, gint width, gint *minimum_height, gint *natural_height, gint *minimum_baseline, gint *natural_baseline)
{
	*minimum_height = 1;
	*natural_height = 1;
	*minimum_baseline = 0;
	*natural_baseline = 0;
}

static void gtk_trunctext_class_init(GtkTrunctextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	widget_class->get_preferred_width = gtk_trunctext_get_preferred_width;
	widget_class->get_preferred_height = gtk_trunctext_get_preferred_width;
	widget_class->get_preferred_width_for_height = gtk_trunctext_get_preferred_width_for_height;
	widget_class->get_preferred_height_for_width = gtk_trunctext_get_preferred_width_for_height;
	widget_class->get_preferred_height_and_baseline_for_width = gtk_trunctext_get_preferred_height_and_baseline_for_width;

	gtk_trunctext_parent_class = g_type_class_peek_parent(klass);
}

static void gtk_trunctext_instance_init(GtkTrunctext *self)
{
}

GType gtk_trunctext_get_type(void)
{
	static volatile gsize gtk_trunctext_type_id__volatile = 0;

	if (g_once_init_enter(&gtk_trunctext_type_id__volatile)) {
		static const GTypeInfo g_define_type_info =
			{ sizeof(GtkTrunctextClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gtk_trunctext_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof(GtkTrunctext), 0, (GInstanceInitFunc) gtk_trunctext_instance_init, NULL };
		GType gtk_trunctext_type_id;

		gtk_trunctext_type_id = g_type_register_static(gtk_label_get_type(), "GtkTrunctext", &g_define_type_info, 0);
		g_once_init_leave(&gtk_trunctext_type_id__volatile, gtk_trunctext_type_id);
	}

	return gtk_trunctext_type_id__volatile;
}
