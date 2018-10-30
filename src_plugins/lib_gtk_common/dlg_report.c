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

#include "dlg_report.h"
#include "compat.h"
#include "bu_box.h"

void pcb_gtk_dlg_report(GtkWidget * top_window, const gchar * title, const gchar * message, gboolean modal)
{
	GtkWidget *w;
	GtkDialog *dialog;
	GtkWidget *content_area;
	GtkWidget *scrolled;
	GtkWidget *vbox, *vbox1;
	GtkWidget *label;
	GtkTextView *text_view;
	GtkTextBuffer *buffer;
	const gchar *s;
	gint nlines;

	if (!message)
		return;
	w = gtk_dialog_new_with_buttons(title ? title : "PCB",
																	GTK_WINDOW(top_window),
																	GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);
	dialog = GTK_DIALOG(w);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CLOSE);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

	g_signal_connect_swapped(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), G_OBJECT(dialog));
	gtk_window_set_role(GTK_WINDOW(w), "PCB_Dialog");

	content_area = gtk_dialog_get_content_area(dialog);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);

	label = gtk_text_view_new();
	text_view = GTK_TEXT_VIEW(label);
	buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_view_set_cursor_visible(text_view, FALSE);
	gtk_text_view_set_editable(text_view, FALSE);
	gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_NONE);
	/* The message should be NULL terminated */
	gtk_text_buffer_set_text(buffer, message, -1);

	for (nlines = 0, s = message; *s; ++s)
		if (*s == '\n')
			++nlines;
	if (nlines > 20) {
		vbox1 = ghid_scrolled_vbox(vbox, &scrolled, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
		gtk_widget_set_size_request(scrolled, -1, 300);
		gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 0);
	}
	else
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(w);
	gtk_window_set_modal(GTK_WINDOW(dialog), modal);
}
