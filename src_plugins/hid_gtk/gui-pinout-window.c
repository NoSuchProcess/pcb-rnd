/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "gui.h"
#include "win_place.h"

#include "copy.h"
#include "data.h"
#include "draw.h"
#include "move.h"
#include "rotate.h"

#include "../src_plugins/lib_gtk_common/wt_preview.h"

static void pinout_close_cb(GtkWidget * widget, GtkWidget * top_window)
{
	gtk_widget_destroy(top_window);
}


void ghid_pinout_window_show(GHidPort * out, pcb_element_t * element)
{
	GtkWidget *button, *vbox, *hbox, *preview, *top_window;
	gchar *title;
	int width, height;

	if (!element)
		return;
	title = g_strdup_printf("%s [%s,%s]",
													PCB_UNKNOWN(PCB_ELEM_NAME_DESCRIPTION(element)), PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), PCB_UNKNOWN(PCB_ELEM_NAME_VALUE(element)));

	top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(top_window), title);
	g_free(title);
	gtk_window_set_wmclass(GTK_WINDOW(top_window), "PCB_Pinout", "PCB");
	gtk_container_set_border_width(GTK_CONTAINER(top_window), 4);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(top_window), vbox);


	preview = pcb_gtk_preview_pinout_new(gport, ghid_init_drawing_widget, element);

	gtk_box_pack_start(GTK_BOX(vbox), preview, TRUE, TRUE, 0);

	pcb_gtk_preview_get_natsize(GHID_PINOUT_PREVIEW(preview), &width, &height);

	gtk_window_set_default_size(GTK_WINDOW(top_window), width + 50, height + 50);

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
