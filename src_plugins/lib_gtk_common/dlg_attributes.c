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

/* This file written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "dlg_attributes.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"

#include "compat.h"

typedef struct {
	GtkWidget *del;
	GtkWidget *w_name;
	GtkWidget *w_value;
} AttrRow;

static AttrRow *attr_row = 0;
static int attr_num_rows = 0;
static int attr_max_rows = 0;
static pcb_attribute_list_t *attributes_list;
static GtkWidget *attributes_dialog, *attr_table;

static void attributes_delete_callback(GtkWidget * w, void *v);

#define GA_RESPONSE_REVERT	1
#define GA_RESPONSE_NEW		2

static void ghid_attr_set_table_size()
{
	gtk_table_resize(GTK_TABLE(attr_table), attr_num_rows > 0 ? attr_num_rows : 1, 3);
}

static void ghid_attributes_need_rows(int new_max)
{
	if (attr_max_rows < new_max) {
		if (attr_row)
			attr_row = (AttrRow *) realloc(attr_row, new_max * sizeof(AttrRow));
		else
			attr_row = (AttrRow *) malloc(new_max * sizeof(AttrRow));
	}
	while (attr_max_rows < new_max) {
		/* add [attr_max_rows] */
		attr_row[attr_max_rows].del = gtk_button_new_with_label("del");
		gtk_table_attach(GTK_TABLE(attr_table), attr_row[attr_max_rows].del,
										 0, 1, attr_max_rows, attr_max_rows + 1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
		g_signal_connect(G_OBJECT(attr_row[attr_max_rows].del), "clicked",
										 G_CALLBACK(attributes_delete_callback), GINT_TO_POINTER(attr_max_rows));

		attr_row[attr_max_rows].w_name = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(attr_table), attr_row[attr_max_rows].w_name,
										 1, 2, attr_max_rows, attr_max_rows + 1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		attr_row[attr_max_rows].w_value = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(attr_table), attr_row[attr_max_rows].w_value,
										 2, 3, attr_max_rows, attr_max_rows + 1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

		attr_max_rows++;
	}

	/* Manage any previously unused rows we now need to show.  */
	while (attr_num_rows < new_max) {
		/* manage attr_num_rows */
		gtk_widget_show(attr_row[attr_num_rows].del);
		gtk_widget_show(attr_row[attr_num_rows].w_name);
		gtk_widget_show(attr_row[attr_num_rows].w_value);
		attr_num_rows++;
	}
}

static void ghid_attributes_revert()
{
	int i;

	ghid_attributes_need_rows(attributes_list->Number);

	/* Unmanage any previously used rows we don't need.  */
	while (attr_num_rows > attributes_list->Number) {
		attr_num_rows--;
		gtk_widget_hide(attr_row[attr_num_rows].del);
		gtk_widget_hide(attr_row[attr_num_rows].w_name);
		gtk_widget_hide(attr_row[attr_num_rows].w_value);
	}

	/* Fill in values */
	for (i = 0; i < attributes_list->Number; i++) {
		/* create row [i] */
		gtk_entry_set_text(GTK_ENTRY(attr_row[i].w_name), attributes_list->List[i].name);
		gtk_entry_set_text(GTK_ENTRY(attr_row[i].w_value), attributes_list->List[i].value);
	}
	ghid_attr_set_table_size();
}

static void attributes_delete_callback(GtkWidget * w, void *v)
{
	int i, n;

	n = GPOINTER_TO_INT(v);

	for (i = n; i < attr_num_rows - 1; i++) {
		gtk_entry_set_text(GTK_ENTRY(attr_row[i].w_name), gtk_entry_get_text(GTK_ENTRY(attr_row[i + 1].w_name)));
		gtk_entry_set_text(GTK_ENTRY(attr_row[i].w_value), gtk_entry_get_text(GTK_ENTRY(attr_row[i + 1].w_value)));
	}
	attr_num_rows--;

	gtk_widget_hide(attr_row[attr_num_rows].del);
	gtk_widget_hide(attr_row[attr_num_rows].w_name);
	gtk_widget_hide(attr_row[attr_num_rows].w_value);

	ghid_attr_set_table_size();
}

void pcb_gtk_dlg_attributes(GtkWidget *top_window, const char *owner, pcb_attribute_list_t * attrs)
{
	GtkWidget *content_area;
	int response;

	attributes_list = attrs;

	attr_max_rows = 0;
	attr_num_rows = 0;

	attributes_dialog = gtk_dialog_new_with_buttons(owner,
																									GTK_WINDOW(top_window),
																									GTK_DIALOG_MODAL,
																									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																									"Revert", GA_RESPONSE_REVERT,
																									"New", GA_RESPONSE_NEW, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	attr_table = gtk_table_new(attrs->Number, 3, 0);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(attributes_dialog));
	gtk_box_pack_start(GTK_BOX(content_area), attr_table, FALSE, FALSE, 0);

	gtk_widget_show(attr_table);

	ghid_attributes_revert();

	while (1) {
		response = gtk_dialog_run(GTK_DIALOG(attributes_dialog));

		if (response == GTK_RESPONSE_CANCEL)
			break;

		if (response == GTK_RESPONSE_OK) {
			int i;
			/* Copy the values back */
			pcb_attribute_copyback_begin(attributes_list);
			for (i = 0; i < attr_num_rows; i++)
				pcb_attribute_copyback(attributes_list, gtk_entry_get_text(GTK_ENTRY(attr_row[i].w_name)), gtk_entry_get_text(GTK_ENTRY(attr_row[i].w_value)));
			pcb_attribute_copyback_end(attributes_list);
			break;
		}

		if (response == GA_RESPONSE_REVERT) {
			/* Revert */
			ghid_attributes_revert();
		}

		if (response == GA_RESPONSE_NEW) {
			ghid_attributes_need_rows(attr_num_rows + 1);	/* also bumps attr_num_rows */

			gtk_entry_set_text(GTK_ENTRY(attr_row[attr_num_rows - 1].w_name), "");
			gtk_entry_set_text(GTK_ENTRY(attr_row[attr_num_rows - 1].w_value), "");

			ghid_attr_set_table_size();
		}
	}

	gtk_widget_destroy(attributes_dialog);
	free(attr_row);
	attr_row = NULL;
}
