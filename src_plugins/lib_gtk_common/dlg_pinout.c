/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "dlg_pinout.h"

#include "conf_core.h"
#include "copy.h"
#include "data.h"
#include "draw.h"
#include "move.h"
#include "rotate.h"
#include "macro.h"

#include "wt_preview.h"
#include "win_place.h"

#include "compat.h"

static void pinout_close_cb(GtkWidget * widget, GtkWidget * top_window)
{
	gtk_widget_destroy(top_window);
}

void ghid_pinout_window_show(pcb_gtk_common_t *com, pcb_subc_t *subc)
{
	GtkWidget *button, *vbox, *hbox, *preview, *top_window;
	gchar *title;
	int width, height;

	if (!subc)
		return;

	title = g_strdup_printf("%s [%s,%s]",
		PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "value")), PCB_UNKNOWN(subc->refdes), PCB_UNKNOWN(pcb_attribute_get(&subc->Attributes, "value")));

	top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(top_window), title);
	g_free(title);
	gtk_window_set_role(GTK_WINDOW(top_window), "PCB_Pinout");
	gtk_container_set_border_width(GTK_CONTAINER(top_window), 4);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(top_window), vbox);


	preview = pcb_gtk_preview_pinout_new(com, com->init_drawing_widget, com->preview_expose, (pcb_any_obj_t *)subc);

	gtk_box_pack_start(GTK_BOX(vbox), preview, TRUE, TRUE, 0);

	pcb_gtk_preview_get_natsize(PCB_GTK_PREVIEW(preview), &width, &height);

	/* do not force window size too large */
	if (width > 200) width = 200;
	if (height > 200) height = 200;

	gtk_widget_set_size_request(top_window, width, height);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pinout_close_cb), top_window);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	gtk_widget_realize(top_window);
	wplc_place(WPLC_PINOUT, top_window);
	gtk_widget_show_all(top_window);
}
