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

/* This file written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "conf_core.h"
#include "dlg_export.h"
#include "dlg_print.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "compat.h"

static GtkWidget *export_dialog = NULL;
static GtkWidget *export_top_win = NULL;


static void exporter_clicked_cb(GtkButton * button, pcb_hid_t * exporter)
{
	ghid_dialog_print(exporter, export_dialog, export_top_win);
}

void ghid_dialog_export(GtkWidget *top_window)
{
	GtkWidget *content_area;
	GtkWidget *vbox, *button;
	int i;
	pcb_hid_t **hids;
	gboolean no_exporter = TRUE;

	export_top_win = top_window;

	export_dialog = gtk_dialog_new_with_buttons(_("PCB Export Layout"),
																							GTK_WINDOW(top_window),
																							(GtkDialogFlags) (GTK_DIALOG_MODAL
																																|
																																GTK_DIALOG_DESTROY_WITH_PARENT),
																							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_wmclass(GTK_WINDOW(export_dialog), "PCB_Export", "PCB");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(export_dialog));

	vbox = gtkc_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);

	/*
	 * Iterate over all the export HID's and build up a dialog box that
	 * lets us choose which one we want to use.
	 * This way, any additions to the exporter HID's automatically are
	 * reflected in this dialog box.
	 */

	hids = pcb_hid_enumerate();
	for (i = 0; hids[i]; i++) {
		if (hids[i]->exporter) {
			no_exporter = FALSE;
			button = gtk_button_new_with_label(hids[i]->name);
			gtk_widget_set_tooltip_text(button, hids[i]->description);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(exporter_clicked_cb), hids[i]);
		}
	}

	if (no_exporter) {
		GtkWidget *label;
		label = gtk_label_new("No exporter found. Check your plugins!");
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}

	gtk_widget_show_all(export_dialog);
	gtk_dialog_run(GTK_DIALOG(export_dialog));

	if (export_dialog != NULL)
		gtk_widget_destroy(export_dialog);
	export_dialog = NULL;
}
