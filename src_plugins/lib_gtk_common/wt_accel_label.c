/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>
#include "config.h"
#include "wt_accel_label.h"
#include "compat.h"

static GtkWidget *gtk_menu_or_checkmenu_item_new(int check, const char *label, const char *accel_label)
{
	GtkWidget *w;
	GtkWidget *hbox = gtkc_hbox_new(FALSE, 0);
	GtkWidget *spring = gtkc_hbox_new(FALSE, 0);
	GtkWidget *l = gtk_label_new(label);
	GtkWidget *accel = gtk_label_new(accel_label);

	if (check)
		w = gtk_check_menu_item_new();
	else
		w = gtk_menu_item_new();
	gtk_box_pack_start(GTK_BOX(hbox), l, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spring, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), accel, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(hbox));

	return w;
}

GtkWidget *pcb_gtk_menu_item_new(const char *label, const char *accel_label)
{
	return gtk_menu_or_checkmenu_item_new(FALSE, label, accel_label);
}

GtkWidget *pcb_gtk_checkmenu_item_new(const char *label, const char *accel_label)
{
	return gtk_menu_or_checkmenu_item_new(TRUE, label, accel_label);
}
