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
 */

/* This module, gui-utils.c, was written by Bill Wilson and the functions
 * here are Copyright (C) 2004 by Bill Wilson.  These functions are utility
 * functions which are taken from my other GPL'd projects gkrellm and
 * gstocks and are copied here for the Gtk PCB port.
 */

#include "config.h"
#include "conf_core.h"

#include "gui.h"
#include <gdk/gdkkeysyms.h>


void ghid_draw_area_update(GHidPort * port, GdkRectangle * rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

const gchar *ghid_entry_get_text(GtkWidget * entry)
{
	const gchar *s = "";

	if (entry)
		s = gtk_entry_get_text(GTK_ENTRY(entry));
	while (*s == ' ' || *s == '\t')
		++s;
	return s;
}

void
ghid_coord_entry(GtkWidget * box, GtkWidget ** coord_entry, pcb_coord_t value,
								 pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size, const pcb_unit_t *u,
								 gint width, void (*cb_func) (GHidCoordEntry *, gpointer), gpointer data, const gchar * string_pre, const gchar * string_post)
{
	GtkWidget *hbox = NULL, *label, *entry_widget;
	GHidCoordEntry *entry;

	if (u == NULL)
		u = conf_core.editor.grid_unit;

	if ((string_pre || string_post) && box) {
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 2);
		box = hbox;
	}

	entry_widget = ghid_coord_entry_new(low, high, value, u, step_size);
	if (coord_entry)
		*coord_entry = entry_widget;
	if (width > 0)
		gtk_widget_set_size_request(entry_widget, width, -1);
	entry = GHID_COORD_ENTRY(entry_widget);
	if (data == NULL)
		data = (gpointer) entry;
	if (cb_func)
		g_signal_connect(G_OBJECT(entry_widget), "value_changed", G_CALLBACK(cb_func), data);
	if (box) {
		if (string_pre) {
			label = gtk_label_new(string_pre);
			gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
		}
		gtk_box_pack_start(GTK_BOX(box), entry_widget, FALSE, FALSE, 2);
		if (string_post) {
			label = gtk_label_new(string_post);
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
			gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 2);
		}
	}
}

void
ghid_table_coord_entry(GtkWidget * table, gint row, gint column,
											 GtkWidget ** coord_entry, pcb_coord_t value,
											 pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
											 gint width, void (*cb_func) (GHidCoordEntry *, gpointer),
											 gpointer data, gboolean right_align, const gchar * string)
{
	GtkWidget *label, *entry_widget;
	GHidCoordEntry *entry;

	if (!table)
		return;

	entry_widget = ghid_coord_entry_new(low, high, value, conf_core.editor.grid_unit, step_size);
	if (coord_entry)
		*coord_entry = entry_widget;
	if (width > 0)
		gtk_widget_set_size_request(entry_widget, width, -1);
	entry = GHID_COORD_ENTRY(entry_widget);
	if (data == NULL)
		data = (gpointer) entry;
	if (cb_func)
		g_signal_connect(G_OBJECT(entry), "value_changed", G_CALLBACK(cb_func), data);

	if (right_align) {
		gtk_table_attach_defaults(GTK_TABLE(table), entry_widget, column + 1, column + 2, row, row + 1);
		if (string) {
			label = gtk_label_new(string);
			gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
			gtk_table_attach_defaults(GTK_TABLE(table), label, column, column + 1, row, row + 1);
		}
	}
	else {
		gtk_table_attach_defaults(GTK_TABLE(table), entry_widget, column, column + 1, row, row + 1);
		if (string) {
			label = gtk_label_new(string);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_table_attach_defaults(GTK_TABLE(table), label, column + 1, column + 2, row, row + 1);
		}
	}
}

void ghid_label_set_markup(GtkWidget * label, const gchar * text)
{
	if (label)
		gtk_label_set_markup(GTK_LABEL(label), text ? text : "");
}

static void text_view_append(GtkWidget * view, const gchar * s)
{
	GtkTextIter iter;
	GtkTextBuffer *buffer;
	GtkTextMark *mark;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	/* gtk_text_iter_forward_to_end(&iter); */

	if (strncmp(s, "<b>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "bold", NULL);
	else if (strncmp(s, "<i>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "italic", NULL);
	else if (strncmp(s, "<h>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "heading", NULL);
	else if (strncmp(s, "<c>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "center", NULL);
	else if (strncmp(s, "<R>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "red", NULL);
	else if (strncmp(s, "<G>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "green", NULL);
	else if (strncmp(s, "<B>", 3) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 3, -1, "blue", NULL);
	else if (strncmp(s, "<ul>", 4) == 0)
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, s + 4, -1, "underline", NULL);
	else
		gtk_text_buffer_insert(buffer, &iter, s, -1);

	mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(view), mark, 0, TRUE, 0.0, 1.0);
	gtk_text_buffer_delete_mark(buffer, mark);
}

void ghid_text_view_append(GtkWidget * view, const gchar * string)
{
	static gchar *tag;
	const gchar *s;

	s = string;
	if (*s == '<' && ((*(s + 2) == '>' && !*(s + 3)) || (*(s + 3) == '>' && !*(s + 4)))) {
		tag = g_strdup(s);
		return;
	}

	if (tag) {
		char *concatenation;
		concatenation = g_strconcat(tag, string, NULL);
		text_view_append(view, concatenation);
		g_free(concatenation);
		g_free(tag);
		tag = NULL;
	}
	else
		text_view_append(view, string);
}
