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

#include "bu_box.h"
#include "compat.h"

/* REMOVE: dlg_search */
GtkWidget *ghid_framed_vbox(GtkWidget *box, gchar *label, gint frame_border_width, gboolean frame_expand, gint vbox_pad, gint vbox_border_width)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new(label);
	gtk_container_set_border_width(GTK_CONTAINER(frame), frame_border_width);
	gtk_box_pack_start(GTK_BOX(box), frame, frame_expand, frame_expand, 0);
	vbox = gtkc_vbox_new(FALSE, vbox_pad);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), vbox_border_width);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	return vbox;
}

