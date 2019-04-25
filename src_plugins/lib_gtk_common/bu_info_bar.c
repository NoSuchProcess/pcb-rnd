/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <gtk/gtk.h>

#include "bu_info_bar.h"

#include "board.h"
#include "plug_io.h"

static void info_bar_response_cb(GtkInfoBar * info_bar, gint response_id, pcb_gtk_info_bar_t *ibar)
{
	gtk_widget_destroy(ibar->info_bar);
	ibar->info_bar = NULL;

	if (response_id == GTK_RESPONSE_ACCEPT)
		pcb_revert_pcb();
}

void pcb_gtk_close_info_bar(pcb_gtk_info_bar_t *ibar)
{
	if (ibar->info_bar != NULL)
		gtk_widget_destroy(ibar->info_bar);
	ibar->info_bar = NULL;
}

void pcb_gtk_info_bar_file_extmod_prompt(pcb_gtk_info_bar_t *ibar, GtkWidget *vbox_middle)
{
	GtkWidget *button;
	GtkWidget *button_image;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *content_area;
	char *file_path_utf8;
	const char *secondary_text;
	char *markup;

	pcb_gtk_close_info_bar(ibar);

	ibar->info_bar = gtk_info_bar_new();

	button = gtk_info_bar_add_button(GTK_INFO_BAR(ibar->info_bar), "Reload", GTK_RESPONSE_ACCEPT);
	button_image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), button_image);

	gtk_info_bar_add_button(GTK_INFO_BAR(ibar->info_bar), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(ibar->info_bar), GTK_MESSAGE_WARNING);
	gtk_box_pack_start(GTK_BOX(vbox_middle), ibar->info_bar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(vbox_middle), ibar->info_bar, 0);


	g_signal_connect(ibar->info_bar, "response", G_CALLBACK(info_bar_response_cb), ibar);

	file_path_utf8 = g_filename_to_utf8(PCB->hidlib.filename, -1, NULL, NULL, NULL);

	secondary_text = PCB->Changed ? "Do you want to drop your changes and reload the file?" : "Do you want to reload the file?";

	markup = g_markup_printf_escaped("<b>The file %s has changed on disk</b>\n\n%s", file_path_utf8, secondary_text);
	g_free(file_path_utf8);

	content_area = gtk_info_bar_get_content_area(GTK_INFO_BAR(ibar->info_bar));

	icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(content_area), icon, FALSE, FALSE, 0);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 6);

	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);

	gtk_misc_set_alignment(GTK_MISC(label), 0., 0.5);

	gtk_widget_show_all(ibar->info_bar);
}

