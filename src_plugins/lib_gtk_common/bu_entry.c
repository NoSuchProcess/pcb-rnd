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

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "bu_entry.h"
#include "conf_core.h"

#include "compat.h"

void ghid_coord_entry(GtkWidget * box, GtkWidget ** coord_entry, pcb_coord_t value,
											pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
											const pcb_unit_t * u, gint width,
											void (*cb_func) (pcb_gtk_coord_entry_t *, gpointer),
											gpointer data, const gchar * string_pre, const gchar * string_post)
{
	GtkWidget *hbox = NULL, *label, *entry_widget;
	pcb_gtk_coord_entry_t *entry;

	if (u == NULL)
		u = conf_core.editor.grid_unit;

	if ((string_pre || string_post) && box) {
		hbox = gtkc_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 2);
		box = hbox;
	}

	entry_widget = pcb_gtk_coord_entry_new(low, high, value, u, step_size);
	if (coord_entry)
		*coord_entry = entry_widget;
	if (width > 0)
		gtk_widget_set_size_request(entry_widget, width, -1);
	entry = GHID_COORD_ENTRY(entry_widget);
	if (data == NULL)
		data = (gpointer) entry;
	if (cb_func)
		g_signal_connect(G_OBJECT(entry_widget), "value_changed", G_CALLBACK(cb_func), data);
	if (box) {
		if (string_pre) {
			label = gtk_label_new(string_pre);
			gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
		}
		gtk_box_pack_start(GTK_BOX(box), entry_widget, FALSE, FALSE, 2);
		if (string_post) {
			label = gtk_label_new(string_post);
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
			gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 2);
		}
	}
}

