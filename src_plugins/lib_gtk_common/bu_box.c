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

#include "config.h"

#include "bu_box.h"

GtkWidget *ghid_framed_vbox(GtkWidget * box, gchar * label, gint frame_border_width,
														gboolean frame_expand, gint vbox_pad, gint vbox_border_width)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new(label);
	gtk_container_set_border_width(GTK_CONTAINER(frame), frame_border_width);
	gtk_box_pack_start(GTK_BOX(box), frame, frame_expand, frame_expand, 0);
	vbox = gtk_vbox_new(FALSE, vbox_pad);
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
	vbox = gtk_vbox_new(FALSE, vbox_pad);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), vbox_border_width);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	return vbox;
}

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header,
															gint header_pad, gint box_pad, gboolean pack_start, gboolean bottom_pad)
{
	GtkWidget *vbox, *vbox1, *hbox, *label;
	gchar *s;

	vbox = gtk_vbox_new(FALSE, 0);
	if (pack_start)
		gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);
	else
		gtk_box_pack_end(GTK_BOX(box), vbox, FALSE, FALSE, 0);

	if (category_header) {
		label = gtk_label_new(NULL);
		s = g_strconcat("<span weight=\"bold\">", category_header, "</span>", NULL);
		gtk_label_set_markup(GTK_LABEL(label), s);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, header_pad);
		g_free(s);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("     ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	vbox1 = gtk_vbox_new(FALSE, box_pad);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 0);

	if (bottom_pad) {
		label = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}
	return vbox1;
}

GtkWidget *ghid_scrolled_vbox(GtkWidget * box, GtkWidget ** scr, GtkPolicyType h_policy, GtkPolicyType v_policy)
{
	GtkWidget *scrolled, *vbox;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), h_policy, v_policy);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), vbox);
	if (scr)
		*scr = scrolled;
	return vbox;
}

GtkWidget *ghid_scrolled_text_view(GtkWidget * box, GtkWidget ** scr, GtkPolicyType h_policy, GtkPolicyType v_policy)
{
	GtkWidget *scrolled, *view;
	GtkTextBuffer *buffer;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), h_policy, v_policy);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

	view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_create_tag(buffer, "heading", "weight", PANGO_WEIGHT_BOLD, "size", 14 * PANGO_SCALE, NULL);
	gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, "center", "justification", GTK_JUSTIFY_CENTER, NULL);
	gtk_text_buffer_create_tag(buffer, "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_buffer_create_tag(buffer, "red", "foreground", "#aa0000", NULL);
	gtk_text_buffer_create_tag(buffer, "green", "foreground", "#00aa00", NULL);
	gtk_text_buffer_create_tag(buffer, "blue", "foreground", "#0000aa", NULL);

	gtk_container_add(GTK_CONTAINER(scrolled), view);
	if (scr)
		*scr = scrolled;
	return view;
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
