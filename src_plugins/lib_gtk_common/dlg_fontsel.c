/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include "dlg_fontsel.h"
#include "compat.h"
#include "bu_box.h"

void pcb_gtk_dlg_fontsel(GtkWidget *top_window, pcb_font_t *fnt, int modal)
{
	GtkWidget *w;
	GtkDialog *dialog;
	GtkWidget *content_area;
	GtkWidget *vbox;

	w = gtk_dialog_new_with_buttons("PCB - font selector",
																	GTK_WINDOW(top_window),
																	GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);
	dialog = GTK_DIALOG(w);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CLOSE);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_window_set_role(GTK_WINDOW(w), "PCB_Dialog");

	content_area = gtk_dialog_get_content_area(dialog);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);

	gtk_widget_show_all(w);
	gtk_window_set_modal(GTK_WINDOW(dialog), modal);
}
