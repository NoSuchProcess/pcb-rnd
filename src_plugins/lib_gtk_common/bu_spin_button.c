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

/* Originally from gui-utils.c, was written by Bill Wilson and the functions
 * here are Copyright (C) 2004 by Bill Wilson.  Those functions were utility
 * functions which are taken from my other GPL'd projects gkrellm and
 * gstocks and are copied here for the Gtk PCB port.
 */

#include "config.h"

#include "bu_spin_button.h"
#include "compat.h"

void
ghid_spin_button(GtkWidget * box, GtkWidget ** spin_button, gfloat value,
								 gfloat low, gfloat high, gfloat step0, gfloat step1,
								 gint digits, gint width,
								 void (*cb_func) (GtkSpinButton *, gpointer), gpointer data, gboolean right_align, const gchar * string)
{
	GtkWidget *hbox = NULL, *label, *spin_but;
	GtkSpinButton *spin;
	GtkAdjustment *adj;

	if (string && box) {
		hbox = gtkc_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 2);
		box = hbox;
	}
	adj = (GtkAdjustment *) gtk_adjustment_new(value, low, high, step0, step1, 0.0);
	spin_but = gtk_spin_button_new(adj, 0.5, digits);
	if (spin_button)
		*spin_button = spin_but;
	if (width > 0)
		gtk_widget_set_size_request(spin_but, width, -1);
	spin = GTK_SPIN_BUTTON(spin_but);
	gtk_spin_button_set_numeric(spin, TRUE);
	if (data == NULL)
		data = (gpointer) spin;
	if (cb_func)
		g_signal_connect(G_OBJECT(spin_but), "value_changed", G_CALLBACK(cb_func), data);
	if (box) {
		if (right_align && string) {
			label = gtk_label_new(string);
			gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
			gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 2);
		}
		gtk_box_pack_start(GTK_BOX(box), spin_but, FALSE, FALSE, 2);
		if (!right_align && string) {
			label = gtk_label_new(string);
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
			gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 2);
		}
	}
}

