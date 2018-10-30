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

#include "bu_check_button.h"

void pcb_gtk_check_button_connected(GtkWidget * box,
																		GtkWidget ** button,
																		gboolean active,
																		gboolean pack_start,
																		gboolean expand,
																		gboolean fill,
																		gint pad,
																		void (*cb_func) (GtkToggleButton *, gpointer), gpointer data, const gchar * string)
{
	GtkWidget *b;

	if (string != NULL)
		b = gtk_check_button_new_with_label(string);
	else
		b = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), active);
	if (box && pack_start)
		gtk_box_pack_start(GTK_BOX(box), b, expand, fill, pad);
	else if (box && !pack_start)
		gtk_box_pack_end(GTK_BOX(box), b, expand, fill, pad);

	if (cb_func)
		g_signal_connect(b, "clicked", G_CALLBACK(cb_func), data);
	if (button)
		*button = b;
}
