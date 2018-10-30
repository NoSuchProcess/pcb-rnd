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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "dlg_input.h"
#include "compat.h"

gchar *pcb_gtk_dlg_input(const char *prompt, const char *initial, GtkWindow * parent)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *vbox, *label, *entry;
	gchar *string;
	gboolean response;
	/*GHidPort *out = &ghid_port; */

	dialog = gtk_dialog_new_with_buttons("PCB User Input",
																			 /*GTK_WINDOW(out->top_window), */
																			 parent,
																			 GTK_DIALOG_MODAL,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	/* Change to gtkc_vbox_new here */
	vbox = gtkc_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), prompt ? prompt : "Enter something");

	entry = gtk_entry_new();
	if (initial)
		gtk_entry_set_text(GTK_ENTRY(entry), initial);

	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response != GTK_RESPONSE_OK)
		string = NULL;
	else
		string = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

	gtk_widget_destroy(dialog);
	return string;
}
