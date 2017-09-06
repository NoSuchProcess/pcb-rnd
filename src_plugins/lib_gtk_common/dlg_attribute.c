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

#include "compat.h"
#include "wt_coord_entry.h"

GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header, gint header_pad, gint box_pad,
															gboolean pack_start, gboolean bottom_pad);
void ghid_spin_button(GtkWidget * box, GtkWidget ** spin_button, gfloat value, gfloat low, gfloat high, gfloat step0,
											gfloat step1, gint digits, gint width, void (*cb_func) (GtkSpinButton *, pcb_hid_attribute_t *), pcb_hid_attribute_t *data,
											gboolean right_align, const gchar * string);
void pcb_gtk_check_button_connected(GtkWidget * box, GtkWidget ** button, gboolean active,
																		gboolean pack_start, gboolean expand,
																		gboolean fill, gint pad,
																		void (*cb_func) (GtkToggleButton *, pcb_hid_attribute_t *), pcb_hid_attribute_t *data,
																		const gchar * string);


#define change_cb(dst) do { if (dst->change_cb != NULL) dst->change_cb(dst); } while(0)

static void set_flag_cb(GtkToggleButton *button, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_toggle_button_get_active(button);
	dst->changed = 1;
	change_cb(dst);
}

static void intspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
	dst->changed = 1;
	change_cb(dst);
}

static void coordentry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	const gchar *s = gtk_entry_get_text(entry);
	dst->default_val.coord_value = pcb_get_value(s, NULL, NULL, NULL);
	dst->changed = 1;
	change_cb(dst);
}

static void dblspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	dst->default_val.real_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
	dst->changed = 1;
	change_cb(dst);
}

static void entry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	free((char *)dst->default_val.str_value);
	dst->default_val.str_value = pcb_strdup(gtk_entry_get_text(entry));
	dst->changed = 1;
	change_cb(dst);
}

static void enum_changed_cb(GtkWidget *combo_box, pcb_hid_attribute_t *dst)
{
	dst->default_val.int_value = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
	dst->changed = 1;
	change_cb(dst);
}

static GtkWidget *ghid_attr_dlg_frame(GtkWidget *parent)
{
	GtkWidget *box, *fr;

	fr = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(parent), fr, FALSE, FALSE, 0);

	box = gtkc_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(fr), box);
	return box;
}

typedef struct {
	int cols, rows;
	int col, row;
} ghid_attr_tbl_t;

static int ghid_attr_dlg_add(pcb_hid_attribute_t *attrs, pcb_hid_attr_val_t *results, GtkWidget *real_parent, ghid_attr_tbl_t *tbl_st, int n_attrs, int start_from, int add_labels)
{
	int j, i, n;
	GtkWidget *combo, *widget, *entry, *vbox1, *hbox, *bparent, *parent, *tbl;

	/*
	 * Iterate over all the export options and build up a dialog box
	 * that lets us control all of the options.  By doing things this
	 * way, any changes to the exporter HID's automatically are
	 * reflected in this dialog box.
	 */
	for (j = start_from; j < n_attrs; j++) {
		const pcb_unit_t *unit_list;
		if (attrs[j].help_text == ATTR_UNDOCUMENTED)
			continue;
		if (attrs[j].type == PCB_HATT_END)
			break;

		/* if we are willing a table, allocate parent boxes in row-major */
		if (tbl_st != NULL) {
			parent = gtkc_vbox_new(FALSE, 4);
			gtkc_table_attach1(real_parent, parent, tbl_st->row, tbl_st->col);
			tbl_st->col++;
			if (tbl_st->col >= tbl_st->cols) {
				tbl_st->col = 0;
				tbl_st->row++;
			}
		}
		else
			parent = real_parent;

		/* create the actual widget from attrs */
		switch (attrs[j].type) {
			case PCB_HATT_BEGIN_HBOX:
				if (attrs[j].pcb_hatt_flags & PCB_HATF_FRAME)
					bparent = ghid_attr_dlg_frame(parent);
				else
					bparent = parent;
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), hbox, FALSE, FALSE, 0);
				j = ghid_attr_dlg_add(attrs, results, hbox, NULL, n_attrs, j+1, (attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_VBOX:
				if (attrs[j].pcb_hatt_flags & PCB_HATF_FRAME)
					bparent = ghid_attr_dlg_frame(parent);
				else
					bparent = parent;
				vbox1 = gtkc_vbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), vbox1, FALSE, FALSE, 0);
				j = ghid_attr_dlg_add(attrs, results, vbox1, NULL, n_attrs, j+1, (attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_TABLE:
				{
					ghid_attr_tbl_t ts;
					ts.cols = attrs[j].pcb_hatt_table_cols;
					ts.rows = pcb_hid_atrdlg_num_children(attrs, j+1, n_attrs) / ts.cols;
					ts.col = 0;
					ts.row = 0;
					tbl = gtkc_table_static(ts.rows, ts.cols, 1);
					gtk_box_pack_start(GTK_BOX(parent), tbl, FALSE, FALSE, 0);
					j = ghid_attr_dlg_add(attrs, results, tbl, &ts, n_attrs, j+1, (attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				}
				break;

			case PCB_HATT_LABEL:
				widget = gtk_label_new(attrs[j].name);
				gtk_box_pack_start(GTK_BOX(parent), widget, FALSE, FALSE, 0);
				gtk_misc_set_alignment(GTK_MISC(widget), 0., 0.5);
				gtk_widget_set_tooltip_text(widget, attrs[j].help_text);
				break;

			case PCB_HATT_INTEGER:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				/*
				 * FIXME
				 * need to pick the "digits" argument based on min/max
				 * values
				 */
				ghid_spin_button(hbox, &widget, attrs[j].default_val.int_value,
												 attrs[j].min_val, attrs[j].max_val, 1.0, 1.0, 0, 0,
												 intspinner_changed_cb, &(attrs[j]), FALSE, NULL);
				gtk_widget_set_tooltip_text(widget, attrs[j].help_text);

				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_COORD:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = pcb_gtk_coord_entry_new(attrs[j].min_val, attrs[j].max_val,
																				attrs[j].default_val.coord_value, conf_core.editor.grid_unit, CE_SMALL);
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				if (attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(coordentry_changed_cb), &(attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_REAL:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				/*
				 * FIXME
				 * need to pick the "digits" and step size argument more
				 * intelligently
				 */
				ghid_spin_button(hbox, &widget, attrs[j].default_val.real_value,
												 attrs[j].min_val, attrs[j].max_val, 0.01, 0.01, 3,
												 0, dblspinner_changed_cb, &(attrs[j]), FALSE, NULL);

				gtk_widget_set_tooltip_text(widget, attrs[j].help_text);

				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_STRING:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				if (attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_BOOL:
				/* put this in a check button */
				pcb_gtk_check_button_connected(parent, &widget,
                                       attrs[j].default_val.int_value,
                                       TRUE, FALSE, FALSE, 0, set_flag_cb, &(attrs[j]), (add_labels ? attrs[j].name : NULL));
				gtk_widget_set_tooltip_text(widget, attrs[j].help_text);
				break;

			case PCB_HATT_ENUM:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

			do_enum:
				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);


				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				i = 0;
				while (attrs[j].enumerations[i]) {
					gtkc_combo_box_text_append_text(combo, attrs[j].enumerations[i]);
					i++;
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(attrs[j]));
				break;

			case PCB_HATT_MIXED:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

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

			case PCB_HATT_PATH:
				vbox1 = ghid_category_vbox(parent, (add_labels ? attrs[j].name : NULL), 4, 2, TRUE, TRUE);
				entry = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(vbox1), entry, FALSE, FALSE, 0);
				gtk_entry_set_text(GTK_ENTRY(entry), attrs[j].default_val.str_value);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(attrs[j]));

				gtk_widget_set_tooltip_text(entry, attrs[j].help_text);
				break;

			case PCB_HATT_UNIT:
				unit_list = get_unit_list();
				n = pcb_get_n_units();

				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(attrs[j]));

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				for (i = 0; i < n; ++i)
					gtkc_combo_box_text_append_text(combo, unit_list[i].in_suffix);
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;
			default:
				printf("ghid_attribute_dialog: unknown type of HID attribute\n");
				break;
		}
	}
	return j;
}

int ghid_attribute_dialog(GtkWidget * top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results,
													const char *title, const char *descr)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *main_vbox, *vbox;
	int i;
	int rc = 0;

	dialog = gtk_dialog_new_with_buttons(_(title),
																			 GTK_WINDOW(top_window),
																			 (GtkDialogFlags) (GTK_DIALOG_MODAL
																												 | GTK_DIALOG_DESTROY_WITH_PARENT),
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_NONE, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_role(GTK_WINDOW(dialog), "PCB_attribute_editor");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	main_vbox = gtkc_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	gtk_container_add(GTK_CONTAINER(content_area), main_vbox);

	if (!PCB_HATT_IS_COMPOSITE(attrs[0].type)) {
		vbox = ghid_category_vbox(main_vbox, descr != NULL ? descr : "", 4, 2, TRUE, TRUE);
		ghid_attr_dlg_add(attrs, results, vbox, NULL, n_attrs, 0, 1);
	}
	else
		ghid_attr_dlg_add(attrs, results, main_vbox, NULL, n_attrs, 0, (attrs[0].pcb_hatt_flags & PCB_HATF_LABEL));

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

