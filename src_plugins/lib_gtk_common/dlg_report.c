/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include "dlg_report.h"
#include "compat.h"
#include "bu_box.h"

/*TODO: Fill, extand the scrolled window to fit during resize */
void pcb_gtk_dlg_report(GtkWidget * top_window, const gchar * title, const gchar * message)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *scrolled;
	GtkWidget *vbox, *vbox1;
	GtkWidget *label;
	const gchar *s;
	gint nlines;

	if (!message)
		return;
	dialog = gtk_dialog_new_with_buttons(title ? title : "PCB",
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	/*TODO: replace for GTK3 */
	gtk_window_set_wmclass(GTK_WINDOW(dialog), "PCB_Dialog", "PCB");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
	gtk_box_pack_start(GTK_BOX(content_area), vbox, FALSE, FALSE, 0);

	label = gtk_label_new(message);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	for (nlines = 0, s = message; *s; ++s)
		if (*s == '\n')
			++nlines;
	if (nlines > 20) {
		vbox1 = ghid_scrolled_vbox(vbox, &scrolled, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(scrolled, -1, 300);
		gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 0);
	}
	else
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);
}
