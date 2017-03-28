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

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "bu_notebook.h"
#include "bu_box.h"

#include "compat.h"

GtkWidget *ghid_notebook_page(GtkWidget * tabs, const char *name, gint pad, gint border)
{
	GtkWidget *label;
	GtkWidget *vbox;

	vbox = gtkc_vbox_new(FALSE, pad);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), border);

	label = gtk_label_new(name);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), vbox, label);

	return vbox;
}

GtkWidget *ghid_framed_notebook_page(GtkWidget * tabs, const char *name, gint border,
																		 gint frame_border, gint vbox_pad, gint vbox_border)
{
	GtkWidget *vbox;

	vbox = ghid_notebook_page(tabs, name, 0, border);
	vbox = ghid_framed_vbox(vbox, NULL, frame_border, TRUE, vbox_pad, vbox_border);
	return vbox;
}
