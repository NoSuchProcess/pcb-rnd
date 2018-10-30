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

#include <string.h>
#include "bu_text_view.h"
//#include "compat.h"

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
