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
#include "bu_cursor_pos.h"
#include "conf_core.h"
#include "actions.h"
#include "crosshair.h"
#include "misc_util.h"
#include "math_helper.h"

static void grid_units_button_cb(GtkWidget * widget, gpointer data)
{
	/* Button only toggles between mm and mil */
	if (conf_core.editor.grid_unit == get_unit_struct("mm"))
		pcb_actionl("SetUnits", "mil", NULL);
	else
		pcb_actionl("SetUnits", "mm", NULL);
}

/*
 * The two following callbacks are used to keep the absolute
 * and relative cursor labels from growing and shrinking as you
 * move the cursor around.
 */
static void absolute_label_size_req_cb(GtkWidget * widget, GdkRectangle * req, gpointer data)
{

	static gint w = 0;
	if (req->width > w)
		w = req->width;
	else
		req->width = w;
}

static void relative_label_size_req_cb(GtkWidget * widget, GdkRectangle * req, gpointer data)
{

	static gint w = 0;
	if (req->width > w)
		w = req->width;
	else
		req->width = w;
}


void make_cursor_position_labels(GtkWidget *hbox, pcb_gtk_cursor_pos_t *cps)
{
	GtkWidget *frame, *label;

	/* The grid units button next to the cursor position labels.
	 */
	cps->grid_units_button = gtk_button_new();
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), conf_core.editor.grid_unit->in_suffix);
	cps->grid_units_label = label;
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(cps->grid_units_button), label);
	gtk_box_pack_end(GTK_BOX(hbox), cps->grid_units_button, FALSE, TRUE, 0);
	g_signal_connect(cps->grid_units_button, "clicked", G_CALLBACK(grid_units_button_cb), NULL);

	/* The absolute cursor position label
	 */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

	/* Using a text will reserve a default width to the widget : */
	label = gtk_label_new(" ___.__ ");
	gtk_container_add(GTK_CONTAINER(frame), label);
	cps->cursor_position_absolute_label = label;
	g_signal_connect(G_OBJECT(label), "size-allocate", G_CALLBACK(absolute_label_size_req_cb), NULL);


	/* The relative cursor position label
	 */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	label = gtk_label_new(" __.__  __.__ ");
	gtk_container_add(GTK_CONTAINER(frame), label);
	cps->cursor_position_relative_label = label;
	g_signal_connect(G_OBJECT(label), "size-allocate", G_CALLBACK(relative_label_size_req_cb), NULL);
}

void ghid_cursor_position_label_set_text(pcb_gtk_cursor_pos_t *cps, gchar * text)
{
	if (cps->cursor_position_absolute_label != NULL)
		gtk_label_set_markup(GTK_LABEL(cps->cursor_position_absolute_label), text ? text : "");
}

void ghid_cursor_position_relative_label_set_text(pcb_gtk_cursor_pos_t *cps, gchar * text)
{
	if (cps->cursor_position_relative_label != NULL)
		gtk_label_set_markup(GTK_LABEL(cps->cursor_position_relative_label), text ? text : "");
}

void ghid_set_cursor_position_labels(pcb_gtk_cursor_pos_t *cps, int compact)
{
	char *text, sep = ' ';
	if (compact)
		sep = '\n';

	if (pcb_marked.status) {
		pcb_coord_t dx = pcb_crosshair.X - pcb_marked.X;
		pcb_coord_t dy = pcb_crosshair.Y - pcb_marked.Y;
		pcb_coord_t r = pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, pcb_marked.X, pcb_marked.Y);
		double a = atan2(dy, dx) * PCB_RAD_TO_DEG;


		text = pcb_strdup_printf("%m+r %-mS;%cphi %-.1f;%c%-mS %-mS", conf_core.editor.grid_unit->allow, r, sep, a, sep, dx, dy);
		ghid_cursor_position_relative_label_set_text(cps, text);
		free(text);
	}
	else {
		char text[64];
		sprintf(text, "r __.__;%cphi __._;%c__.__ __.__", sep, sep);
		ghid_cursor_position_relative_label_set_text(cps, text);
	}

	text = pcb_strdup_printf("%m+%-mS%c%-mS", conf_core.editor.grid_unit->allow, pcb_crosshair.X, sep, pcb_crosshair.Y);
	ghid_cursor_position_label_set_text(cps, text);
	free(text);
}

