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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#define PCB_OBJ_PROP "pcb-rnd_context"

typedef struct {
	pcb_hid_attribute_t *attrs;
	pcb_hid_attr_val_t *results;
	GtkWidget **wl;
	int n_attrs;
	void *caller_data;
	GtkWidget *dialog;
	int rc;
	pcb_hid_attr_val_t property[PCB_HATP_max];
	unsigned inhibit_valchg:1;
} attr_dlg_t;

#define change_cb(ctx, dst) \
	do { \
		if (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func != NULL) \
			ctx->property[PCB_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, dst); \
		if (dst->change_cb != NULL) \
			dst->change_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

static void set_flag_cb(GtkToggleButton *button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);
	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;
	dst->default_val.int_value = gtk_toggle_button_get_active(button);
	change_cb(ctx, dst);
}

static void intspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(spin_button), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;
	dst->default_val.int_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
	change_cb(ctx, dst);
}

static void coordentry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);
	const gchar *s;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	s = gtk_entry_get_text(entry);
	dst->default_val.coord_value = pcb_get_value(s, NULL, NULL, NULL);
	change_cb(ctx, dst);
}

static void dblspinner_changed_cb(GtkSpinButton *spin_button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(spin_button), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.real_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON((GtkWidget *)spin_button));
	change_cb(ctx, dst);
}

static void entry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	free((char *)dst->default_val.str_value);
	dst->default_val.str_value = pcb_strdup(gtk_entry_get_text(entry));
	change_cb(ctx, dst);
}

static void enum_changed_cb(GtkWidget *combo_box, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(combo_box), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.int_value = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
	change_cb(ctx, dst);
}

static void button_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	change_cb(ctx, dst);
}

typedef struct {
	void (*cb)(void *ctx, pcb_hid_attr_ev_t ev);
	void *ctx;
} resp_ctx_t;

static void ghid_attr_dlg_response_cb(GtkDialog *dialog, gint arg1, gpointer user_data)
{
	resp_ctx_t *ctx = (resp_ctx_t *)user_data;
	if ((ctx != NULL) && (ctx->cb != NULL)) {
		switch (arg1) {
		case GTK_RESPONSE_OK:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_OK);
			break;
		case GTK_RESPONSE_CANCEL:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_CANCEL);
			break;
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_DELETE_EVENT:
			ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_WINCLOSE);
			break;
		}
	}
	free(ctx);
}


static GtkWidget *frame_scroll(GtkWidget *parent, pcb_hatt_compflags_t flags)
{
	GtkWidget *fr;

	if (flags & PCB_HATF_FRAME) {
		fr = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(parent), fr, FALSE, FALSE, 0);

		parent = gtkc_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(fr), parent);
	}
	if (flags & PCB_HATF_SCROLL) {
		fr = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(parent), fr, TRUE, TRUE, 0);
		parent = gtkc_hbox_new(FALSE, 0);
		gtkc_scrolled_window_add_with_viewport(fr, parent);
	}
	return parent;
}

typedef struct {
	int cols, rows;
	int col, row;
} ghid_attr_tbl_t;

static int ghid_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, ghid_attr_tbl_t *tbl_st, int start_from, int add_labels)
{
	int j, i, n;
	GtkWidget *combo, *widget, *entry, *vbox1, *hbox, *bparent, *parent, *tbl;

	/*
	 * Iterate over all the export options and build up a dialog box
	 * that lets us control all of the options.  By doing things this
	 * way, any changes to the exporter HID's automatically are
	 * reflected in this dialog box.
	 */
	for (j = start_from; j < ctx->n_attrs; j++) {
		const pcb_unit_t *unit_list;
		if (ctx->attrs[j].help_text == ATTR_UNDOCUMENTED)
			continue;
		if (ctx->attrs[j].type == PCB_HATT_END)
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
		switch (ctx->attrs[j].type) {
			case PCB_HATT_BEGIN_HBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), hbox, FALSE, FALSE, 0);
				ctx->wl[j] = hbox;
				j = ghid_attr_dlg_add(ctx, hbox, NULL, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_VBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
				vbox1 = gtkc_vbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(bparent), vbox1, FALSE, FALSE, 0);
				ctx->wl[j] = vbox1;
				j = ghid_attr_dlg_add(ctx, vbox1, NULL, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				break;

			case PCB_HATT_BEGIN_TABLE:
				{
					ghid_attr_tbl_t ts;
					bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags);
					ts.cols = ctx->attrs[j].pcb_hatt_table_cols;
					ts.rows = pcb_hid_atrdlg_num_children(ctx->attrs, j+1, ctx->n_attrs) / ts.cols;
					ts.col = 0;
					ts.row = 0;
					tbl = gtkc_table_static(ts.rows, ts.cols, 1);
					gtk_box_pack_start(GTK_BOX(bparent), tbl, FALSE, FALSE, 0);
					ctx->wl[j] = tbl;
					j = ghid_attr_dlg_add(ctx, tbl, &ts, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));
				}
				break;

			case PCB_HATT_BEGIN_TABBED:
				ctx->wl[j] = widget = gtk_notebook_new();
				gtk_box_pack_start(GTK_BOX(parent), widget, FALSE, FALSE, 0);
				vbox1 = gtkc_vbox_new(FALSE, 0);
				j = ghid_attr_dlg_add(ctx, vbox1, NULL, j+1, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LABEL));;
				gtk_widget_show_all(vbox1);
				gtk_notebook_append_page(GTK_NOTEBOOK(widget), vbox1, NULL);
				gtk_widget_show_all(widget);
				;
				break;

			case PCB_HATT_LABEL:
				ctx->wl[j] = widget = gtk_label_new(ctx->attrs[j].name);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);

				gtk_box_pack_start(GTK_BOX(parent), widget, FALSE, FALSE, 0);
				gtk_misc_set_alignment(GTK_MISC(widget), 0., 0.5);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				break;

			case PCB_HATT_INTEGER:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				/*
				 * FIXME
				 * need to pick the "digits" argument based on min/max
				 * values
				 */
				ghid_spin_button(hbox, &widget, ctx->attrs[j].default_val.int_value,
												 ctx->attrs[j].min_val, ctx->attrs[j].max_val, 1.0, 1.0, 0, 0,
												 intspinner_changed_cb, &(ctx->attrs[j]), FALSE, NULL);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_COORD:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = pcb_gtk_coord_entry_new(ctx->attrs[j].min_val, ctx->attrs[j].max_val,
																				ctx->attrs[j].default_val.coord_value, conf_core.editor.grid_unit, CE_SMALL);
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(coordentry_changed_cb), &(ctx->attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
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
				ghid_spin_button(hbox, &widget, ctx->attrs[j].default_val.real_value,
												 ctx->attrs[j].min_val, ctx->attrs[j].max_val, 0.01, 0.01, 3,
												 0, dblspinner_changed_cb, &(ctx->attrs[j]), FALSE, NULL);

				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_STRING:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				entry = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;

			case PCB_HATT_BOOL:
				/* put this in a check button */
				pcb_gtk_check_button_connected(parent, &widget,
                                       ctx->attrs[j].default_val.int_value,
                                       TRUE, FALSE, FALSE, 0, set_flag_cb, &(ctx->attrs[j]), (add_labels ? ctx->attrs[j].name : NULL));
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;
				break;

			case PCB_HATT_ENUM:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
				g_object_set_data(G_OBJECT(combo), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				i = 0;
				while (ctx->attrs[j].enumerations[i]) {
					gtkc_combo_box_text_append_text(combo, ctx->attrs[j].enumerations[i]);
					i++;
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				break;

			case PCB_HATT_PATH:
				vbox1 = ghid_category_vbox(parent, (add_labels ? ctx->attrs[j].name : NULL), 4, 2, TRUE, TRUE);
				entry = gtk_entry_new();
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				gtk_box_pack_start(GTK_BOX(vbox1), entry, FALSE, FALSE, 0);
				gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));

				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				break;

			case PCB_HATT_UNIT:
				unit_list = get_unit_list();
				n = pcb_get_n_units();

				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(combo), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				for (i = 0; i < n; ++i)
					gtkc_combo_box_text_append_text(combo, unit_list[i].in_suffix);
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].default_val.int_value);
				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
					gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
				}
				break;
			case PCB_HATT_BUTTON:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

				ctx->wl[j] = gtk_button_new_with_label(ctx->attrs[j].default_val.str_value);
				gtk_box_pack_start(GTK_BOX(hbox), ctx->wl[j], FALSE, FALSE, 0);

				gtk_widget_set_tooltip_text(ctx->wl[j], ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "pressed", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), PCB_OBJ_PROP, ctx);

				if (add_labels) {
					widget = gtk_label_new(ctx->attrs[j].name);
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

static int ghid_attr_dlg_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	int save;
	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;

	/* create the actual widget from attrs */
	switch (ctx->attrs[idx].type) {
		case PCB_HATT_BEGIN_HBOX:
		case PCB_HATT_BEGIN_VBOX:
		case PCB_HATT_BEGIN_TABLE:
		case PCB_HATT_END:
			goto error;

		case PCB_HATT_LABEL:
			gtk_label_set_text(GTK_LABEL(ctx->wl[idx]), val->str_value);
			break;

		case PCB_HATT_INTEGER:
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->wl[idx]), val->int_value);
			break;

		case PCB_HATT_COORD:
			pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(ctx->wl[idx]), val->coord_value);
			break;

		case PCB_HATT_REAL:
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctx->wl[idx]), val->real_value);
			break;

		case PCB_HATT_STRING:
		case PCB_HATT_PATH:
			gtk_entry_set_text(GTK_ENTRY(ctx->wl[idx]), val->str_value);
			break;

		case PCB_HATT_BOOL:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), val->int_value);
			break;

		case PCB_HATT_ENUM:
			gtkc_combo_box_set_active(ctx->wl[idx], val->int_value);
			break;

		case PCB_HATT_UNIT:
			abort();
			break;

		case PCB_HATT_BUTTON:
			gtk_button_set_label(GTK_BUTTON(ctx->wl[idx]), val->str_value);
			break;
	}

	ctx->inhibit_valchg = save;
	return 0;

	error:;
	ctx->inhibit_valchg = save;
	return -1;
}

void *ghid_attr_dlg_new(GtkWidget *top_window, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
{
	GtkWidget *content_area;
	GtkWidget *main_vbox, *vbox;
	attr_dlg_t *ctx;
	resp_ctx_t *resp_ctx = NULL;

	if (button_cb != NULL) {
		resp_ctx = malloc(sizeof(resp_ctx_t));
		resp_ctx->cb = button_cb;
		resp_ctx->ctx = caller_data;
	}

	ctx = calloc(sizeof(attr_dlg_t), 1);
	ctx->attrs = attrs;
	ctx->results = results;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;

	ctx->dialog = gtk_dialog_new_with_buttons(_(title),
																			 GTK_WINDOW(top_window),
																			 (GtkDialogFlags) ((modal?GTK_DIALOG_MODAL:0)
																												 | GTK_DIALOG_DESTROY_WITH_PARENT),
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_role(GTK_WINDOW(ctx->dialog), "PCB_attribute_editor");

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
	g_signal_connect(ctx->dialog, "response", G_CALLBACK(ghid_attr_dlg_response_cb), resp_ctx);

	main_vbox = gtkc_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	gtk_container_add(GTK_CONTAINER(content_area), main_vbox);

	if (!PCB_HATT_IS_COMPOSITE(attrs[0].type)) {
		vbox = ghid_category_vbox(main_vbox, descr != NULL ? descr : "", 4, 2, TRUE, TRUE);
		ghid_attr_dlg_add(ctx, vbox, NULL, 0, 1);
	}
	else
		ghid_attr_dlg_add(ctx, main_vbox, NULL, 0, (attrs[0].pcb_hatt_flags & PCB_HATF_LABEL));

	gtk_widget_show_all(ctx->dialog);

	return ctx;
}

int ghid_attr_dlg_run(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	if (gtk_dialog_run(GTK_DIALOG(ctx->dialog)) == GTK_RESPONSE_OK)
		ctx->rc = 0;
	else
		ctx->rc = 1;
	return ctx->rc;
}

void ghid_attr_dlg_free(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;

	if (ctx->rc == 0) { /* copy over the results */
		int i;
		for (i = 0; i < ctx->n_attrs; i++) {
			ctx->results[i] = ctx->attrs[i].default_val;
			if (PCB_HAT_IS_STR(ctx->attrs[i].type) && (ctx->results[i].str_value))
				ctx->results[i].str_value = pcb_strdup(ctx->results[i].str_value);
			else
				ctx->results[i].str_value = NULL;
		}
	}

	gtk_widget_destroy(ctx->dialog);
	free(ctx->wl);
}

void ghid_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((prop >= 0) && (prop < PCB_HATP_max))
		ctx->property[prop] = *val;
}


int ghid_attribute_dialog(GtkWidget * top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data)
{
	void *hid_ctx;
	int rc;

	hid_ctx = ghid_attr_dlg_new(top_window, attrs, n_attrs, results, title, descr, caller_data, pcb_true, NULL);
	rc = ghid_attr_dlg_run(hid_ctx);
	ghid_attr_dlg_free(hid_ctx);

	return rc;
}


int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	gtk_widget_set_sensitive(ctx->wl[idx], enabled);

	return 0;
}

int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (hide)
		gtk_widget_hide(ctx->wl[idx]);
	else
		gtk_widget_show(ctx->wl[idx]);

	return 0;
}

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	if (ghid_attr_dlg_set(ctx, idx, val) == 0) {
		ctx->attrs[idx].default_val = *val;
		return 0;
	}

	return -1;
}
