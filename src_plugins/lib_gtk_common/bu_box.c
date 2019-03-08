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

/*TODO: The 2 following functions could be merged if an additional 
 *  gboolean pack_start   parameter is added */
GtkWidget *ghid_framed_vbox(GtkWidget * box, gchar * label, gint frame_border_width,
														gboolean frame_expand, gint vbox_pad, gint vbox_border_width)
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

GtkWidget *ghid_framed_vbox_end(GtkWidget * box, gchar * label, gint frame_border_width,
																gboolean frame_expand, gint vbox_pad, gint vbox_border_width)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new(label);
	gtk_container_set_border_width(GTK_CONTAINER(frame), frame_border_width);
	gtk_box_pack_end(GTK_BOX(box), frame, frame_expand, frame_expand, 0);
	vbox = gtkc_vbox_new(FALSE, vbox_pad);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), vbox_border_width);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	return vbox;
}

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header,
															gint header_pad, gint box_pad, gboolean pack_start, gboolean bottom_pad)
{
	GtkWidget *vbox, *vbox1, *hbox, *label;
	gchar *s;

	vbox = gtkc_vbox_new(FALSE, 0);
	if (pack_start)
		gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);
	else
		gtk_box_pack_end(GTK_BOX(box), vbox, FALSE, FALSE, 0);

	if (category_header) {
		label = gtk_label_new(NULL);
		s = g_strconcat("<span weight=\"bold\">", category_header, "</span>", NULL);
		gtk_label_set_markup(GTK_LABEL(label), s);
		/*TODO: Deprecated in GTK3. Use gtk_widget_set_[h|v]align () functions ? */
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, header_pad);
		g_free(s);
	}

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("     ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	vbox1 = gtkc_vbox_new(FALSE, box_pad);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 0);

	if (bottom_pad) {
		label = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}
	return vbox1;
}

GtkTreeSelection *ghid_scrolled_selection(GtkTreeView * treeview, GtkWidget * box,
																					GtkSelectionMode s_mode,
																					GtkPolicyType h_policy, GtkPolicyType v_policy,
																					void (*func_cb) (GtkTreeSelection *, gpointer), gpointer data)
{
	GtkTreeSelection *selection;
	GtkWidget *scrolled;

	if (!box || !treeview)
		return NULL;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(treeview));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), h_policy, v_policy);
	selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(selection, s_mode);
	if (func_cb)
		g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(func_cb), data);
	return selection;
}
