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
#include "dlg_attribute.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "wt_coord_entry.h"

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header, gint header_pad, gint box_pad,
															gboolean pack_start, gboolean bottom_pad);
void ghid_spin_button(GtkWidget * box, GtkWidget ** spin_button, gfloat value, gfloat low, gfloat high, gfloat step0,
											gfloat step1, gint digits, gint width, void (*cb_func) (GtkSpinButton *, pcb_hid_attribute_t *), pcb_hid_attribute_t *data,
											gboolean right_align, const gchar * string);
void ghid_check_button_connected(GtkWidget * box, GtkWidget ** button, gboolean active, gboolean pack_start, gboolean expand,
																 gboolean fill, gint pad, void (*cb_func) (GtkToggleButton *, pcb_hid_attribute_t *), pcb_hid_attribute_t *data,
																 const gchar * string);



static void set_flag_cb(GtkToggleButton *button, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_toggle_button_get_active(button);
}

static void intspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
}

static void coordentry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	const gchar *s = gtk_entry_get_text(entry);
	dst->default_val.coord_value = pcb_get_value(s, NULL, NULL, NULL);
}

static void dblspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	dst->default_val.real_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
}

static void entry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	free((char *)dst->default_val.str_value);
	dst->default_val.str_value = pcb_strdup(gtk_entry_get_text(entry));
}

static void enum_changed_cb(GtkWidget *combo_box, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
}

int ghid_attribute_dialog(GtkWidget * top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results,
													const char *title, const char *descr)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *main_vbox, *vbox, *vbox1, *hbox, *entry;
	GtkWidget *combo;
	GtkWidget *widget;
	int i, j, n;
	int rc = 0;

	dialog = gtk_dialog_new_with_buttons(_(title),
																			 GTK_WINDOW(top_window),
																			 (GtkDialogFlags) (GTK_DIALOG_MODAL
																												 | GTK_DIALOG_DESTROY_WITH_PARENT),
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_NONE, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_wmclass(GTK_WINDOW(dialog), "PCB_attribute_editor", "PCB");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	main_vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	gtk_container_add(GTK_CONTAINER(content_area), main_vbox);

	vbox = ghid_category_vbox(main_vbox, descr != NULL ? descr : "", 4, 2, TRUE, TRUE);

	/*
	 * Iterate over all the export options and build up a dialog box
	 * that lets us control all of the options.  By doing things this
	 * way, any changes to the exporter HID's automatically are
	 * reflected in this dialog box.
	 */
	for (j = 0; j < n_attrs; j++) {
		const pcb_unit_t *unit_list;
		if (attrs[j].help_text == ATTR_UNDOCUMENTED)
			continue;
		switch (attrs[j].type) {
		case HID_Label:
			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
			gtk_widget_set_tooltip_text(widget, attrs[j].help_text);
			break;

		case HID_Integer:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			/*
			 * FIXME
			 * need to pick the "digits" argument based on min/max
			 * values
			 */
			ghid_spin_button(hbox, &widget, attrs[j].default_val.int_value,
											 attrs[j].min_val, attrs[j].max_val, 1.0, 1.0, 0, 0,
											 intspinner_changed_cb, &(attrs[j]), FALSE, NULL);
			gtk_widget_set_tooltip_text(widget, attrs[j].help_text);

			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;

		case HID_Coord:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			entry = pcb_gtk_coord_entry_new(attrs[j].min_val, attrs[j].max_val,
																			attrs[j].default_val.coord_value, conf_core.editor.grid_unit, CE_SMALL);
			gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
			if (attrs[j].default_val.str_value != NULL)
				gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
			gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
			g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(coordentry_changed_cb), &(attrs[j]));

			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;

		case HID_Real:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			/*
			 * FIXME
			 * need to pick the "digits" and step size argument more
			 * intelligently
			 */
			ghid_spin_button(hbox, &widget, attrs[j].default_val.real_value,
											 attrs[j].min_val, attrs[j].max_val, 0.01, 0.01, 3,
											 0, dblspinner_changed_cb, &(attrs[j]), FALSE, NULL);

			gtk_widget_set_tooltip_text(widget, attrs[j].help_text);

			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;

		case HID_String:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			entry = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
			if (attrs[j].default_val.str_value != NULL)
				gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
			gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
			g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(attrs[j]));

			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;

		case HID_Boolean:
			/* put this in a check button */
			ghid_check_button_connected(vbox, &widget,
																	attrs[j].default_val.int_value,
																	TRUE, FALSE, FALSE, 0, set_flag_cb, &(attrs[j]), attrs[j].name);
			gtk_widget_set_tooltip_text(widget, attrs[j].help_text);
			break;

		case HID_Enum:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

		do_enum:
			combo = gtk_combo_box_new_text();
			gtk_widget_set_tooltip_text(combo, attrs[j].help_text);
			gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
			g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(attrs[j]));


			/*
			 * Iterate through each value and add them to the
			 * combo box
			 */
			i = 0;
			while (attrs[j].enumerations[i]) {
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo), attrs[j].enumerations[i]);
				i++;
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), attrs[j].default_val.int_value);
			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;

		case HID_Mixed:
			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			/*
			 * FIXME
			 * need to pick the "digits" and step size argument more
			 * intelligently
			 */
			ghid_spin_button(hbox, &widget, attrs[j].default_val.real_value,
											 attrs[j].min_val, attrs[j].max_val, 0.01, 0.01, 3,
											 0, dblspinner_changed_cb, &(attrs[j]), FALSE, NULL);
			gtk_widget_set_tooltip_text(widget, attrs[j].help_text);

			goto do_enum;
			break;

		case HID_Path:
			vbox1 = ghid_category_vbox(vbox, attrs[j].name, 4, 2, TRUE, TRUE);
			entry = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(vbox1), entry, FALSE, FALSE, 0);
			gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
			g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(attrs[j]));

			gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
			break;

		case HID_Unit:
			unit_list = get_unit_list();
			n = pcb_get_n_units();

			hbox = gtk_hbox_new(FALSE, 4);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

			combo = gtk_combo_box_new_text();
			gtk_widget_set_tooltip_text(combo, attrs[j].help_text);
			gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
			g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(attrs[j]));

			/*
			 * Iterate through each value and add them to the
			 * combo box
			 */
			for (i = 0; i < n; ++i)
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo), unit_list[i].in_suffix);
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), attrs[j].default_val.int_value);
			widget = gtk_label_new(attrs[j].name);
			gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
			break;
		default:
			printf("ghid_attribute_dialog: unknown type of HID attribute\n");
			break;
		}
	}


	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		/* copy over the results */
		for (i = 0; i < n_attrs; i++) {
			results[i] = attrs[i].default_val;
			if (results[i].str_value)
				results[i].str_value = pcb_strdup(results[i].str_value);
		}
		rc = 0;
	}
	else
		rc = 1;

	gtk_widget_destroy(dialog);

	return rc;
}

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
			for (i = 0; i < attributes_list->Number; i++) {
				if (attributes_list->List[i].name)
					free(attributes_list->List[i].name);
				if (attributes_list->List[i].value)
					free(attributes_list->List[i].value);
			}
			if (attributes_list->Max < attr_num_rows) {
				int sz = attr_num_rows * sizeof(pcb_attribute_t);
				if (attributes_list->List == NULL)
					attributes_list->List = (pcb_attribute_t *) malloc(sz);
				else
					attributes_list->List = (pcb_attribute_t *) realloc(attributes_list->List, sz);
				attributes_list->Max = attr_num_rows;
			}
			for (i = 0; i < attr_num_rows; i++) {
				attributes_list->List[i].name = pcb_strdup(gtk_entry_get_text(GTK_ENTRY(attr_row[i].w_name)));
				attributes_list->List[i].value = pcb_strdup(gtk_entry_get_text(GTK_ENTRY(attr_row[i].w_value)));
				attributes_list->Number = attr_num_rows;
			}

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
