/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Alain Vigne
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
 */

/* route style edit dialog */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

#include "compat_misc.h"
#include "compat_nls.h"
#include "polygon.h"

#include "bu_box.h"
#include "compat.h"
#include "board.h"
#include "conf_core.h"
#include "error.h"
#include "event.h"

#include "dlg_route_style.h"
#include "wt_coord_entry.h"

/* SIGNAL HANDLERS */

/* Rebuild the gtk table for attribute list from style */
static void update_attrib(pcb_gtk_dlg_route_style_t *dialog, pcb_gtk_obj_route_style_t *style)
{
	GtkTreeIter iter;
	int i;

	gtk_list_store_clear(dialog->attr_model);

	for (i = 0; i < style->rst->attr.Number; i++) {
		gtk_list_store_append(dialog->attr_model, &iter);
		gtk_list_store_set(dialog->attr_model, &iter, 0, style->rst->attr.List[i].name, -1);
		gtk_list_store_set(dialog->attr_model, &iter, 1, style->rst->attr.List[i].value, -1);
	}

	gtk_list_store_append(dialog->attr_model, &iter);
	gtk_list_store_set(dialog->attr_model, &iter, 0, "<new>", -1);
	gtk_list_store_set(dialog->attr_model, &iter, 1, "<new>", -1);
}

static int get_sel(pcb_gtk_dlg_route_style_t *dlg)
{
	GtkTreeIter iter;
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreePath *path;
	int *i;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->attr_table));
	if (tsel == NULL)
		return -1;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return -1;

	path = gtk_tree_model_get_path(tm, &iter);
	if (path != NULL) {
		i = gtk_tree_path_get_indices(path);
		if (i != NULL) {
/*			gtk_tree_model_get(GTK_TREE_MODEL(dlg->attr_model), &iter, 0, nkey, 1, nval, -1);*/
			return i[0];
		}
	}
	return -1;
}

/* When a different style is selected, load that style's data into the dialog. */
static void dialog_style_changed_cb(GtkComboBox *combo, pcb_gtk_dlg_route_style_t *dialog)
{
	pcb_gtk_route_style_t *rss;
	pcb_gtk_obj_route_style_t *style;
	GtkTreeIter iter;
	GtkTreeModel *tree_model;

	if (dialog->inhibit_style_change)
		return;

	rss = dialog->rss;
	tree_model = GTK_TREE_MODEL(rss->model);
	gtk_combo_box_get_active_iter(combo, &iter);
	gtk_tree_model_get(tree_model, &iter, STYLE_DATA_COL, &style, -1);

	if (style == NULL) {
		gtk_entry_set_text(GTK_ENTRY(dialog->name_entry), _("New Style"));
		rss->selected = -1;
		return;
	}

	gtk_entry_set_text(GTK_ENTRY(dialog->name_entry), style->rst->name);
	pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(dialog->line_entry), style->rst->Thick);
	pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(dialog->textt_entry), style->rst->textt);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->texts_entry), style->rst->texts);
	pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(dialog->via_hole_entry), style->rst->Hole);
	pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(dialog->via_size_entry), style->rst->Diameter);
	pcb_gtk_coord_entry_set_value(GHID_COORD_ENTRY(dialog->clearance_entry), style->rst->Clearance);

	if (style->hidden)
		rss->selected = -1;
	else
		rss->selected = style->rst - PCB->RouteStyle.array;

	update_attrib(dialog, style);
}

static void attr_edited(int col, GtkCellRendererText *cell, gchar *path, gchar *new_text, pcb_gtk_dlg_route_style_t *dlg)
{
	GtkTreeIter iter;
	pcb_gtk_obj_route_style_t *style;
	int idx = get_sel(dlg);

	dlg->attr_editing = 0;

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlg->select_box), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(dlg->rss->model), &iter, STYLE_DATA_COL, &style, -1);

	if (style == NULL)
		return;

	if (idx >= style->rst->attr.Number) { /* add new */
		if (col == 0)
			pcb_attribute_put(&style->rst->attr, new_text, "n/a");
		else
			pcb_attribute_put(&style->rst->attr, "n/a", new_text);
	}
	else {  /* overwrite existing */
		char **dest;
		if (col == 0)
			dest = &style->rst->attr.List[idx].name;
		else
			dest = &style->rst->attr.List[idx].value;
		if (*dest != NULL)
			free(*dest);
		*dest = pcb_strdup(new_text);
	}

	/* rebuild the table to keep it in sync - expensive, but it happens rarely */
	update_attrib(dlg, style);
}

static void attr_edited_key_cb(GtkCellRendererText *cell, gchar *path, gchar *new_text, pcb_gtk_dlg_route_style_t *dlg)
{
	attr_edited(0, cell, path, new_text, dlg);
}

static void attr_edited_val_cb(GtkCellRendererText *cell, gchar *path, gchar *new_text, pcb_gtk_dlg_route_style_t *dlg)
{
	attr_edited(1, cell, path, new_text, dlg);
}

static void attr_edit_started_cb(GtkCellRendererText *cell, GtkCellEditable *e, gchar *path, pcb_gtk_dlg_route_style_t *dlg)
{
	dlg->attr_editing = 1;
}

static void attr_edit_canceled_cb(GtkCellRendererText *cell, pcb_gtk_dlg_route_style_t *dlg)
{
	dlg->attr_editing = 0;
}

static gboolean attr_key_release_cb(GtkWidget *widget, GdkEventKey *event, pcb_gtk_dlg_route_style_t *dlg)
{
	unsigned short int kv = event->keyval;

	if (dlg->attr_editing)
		return FALSE;

	switch (kv) {
#ifdef GDK_KEY_KP_Delete
	case GDK_KEY_KP_Delete:
#endif
#ifdef GDK_KEY_Delete
	case GDK_KEY_Delete:
#endif
	case 'd':
		{
			int idx = get_sel(dlg);
			GtkTreeIter iter;
			pcb_gtk_obj_route_style_t *style;
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlg->select_box), &iter);
			gtk_tree_model_get(GTK_TREE_MODEL(dlg->rss->model), &iter, STYLE_DATA_COL, &style, -1);
			if (style == NULL)
				return FALSE;
			if ((idx >= 0) && (idx < style->rst->attr.Number)) {
				pcb_attribute_remove_idx(&style->rst->attr, idx);
				update_attrib(dlg, style);
			}
		}
		break;
	}
	return FALSE;
}

static void add_new_iter(pcb_gtk_route_style_t *rss)
{
	/* Add "new style" option to list */
	gtk_list_store_append(rss->model, &rss->new_iter);
	gtk_list_store_set(rss->model, &rss->new_iter, STYLE_TEXT_COL, _("<New>"), STYLE_DATA_COL, NULL, -1);
}

/*  Callback for Delete route style button */
static void delete_button_cb(GtkButton *button, pcb_gtk_dlg_route_style_t *dialog)
{
	if (dialog->rss->selected < 0)
		return;

	dialog->inhibit_style_change = 1;
	pcb_gtk_route_style_empty(dialog->rss);

TODO(": some of these should be in core")
	pcb_gtk_route_style_copy(dialog->rss->selected);

	vtroutestyle_remove(&PCB->RouteStyle, dialog->rss->selected, 1);
	dialog->rss->active_style = NULL;
	make_route_style_buttons(GHID_ROUTE_STYLE(dialog->rss));
	pcb_trace("Style: %d deleted\n", dialog->rss->selected);
	pcb_board_set_changed_flag(pcb_true);
	dialog->rss->com->window_set_name_label(PCB->Name);
	add_new_iter(dialog->rss);
	dialog->inhibit_style_change = 0;
	pcb_gtk_route_style_select_style(dialog->rss, &pcb_custom_route_style);
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->select_box), 0);
	pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
}

static void _table_attach_(GtkWidget *table, gint row, const gchar *label, GtkWidget *entry)
{
	GtkWidget *label_w = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(label_w), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label_w, 0, 1, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
}

static void _table_attach(GtkWidget *table, gint row, const gchar *label, GtkWidget **entry, pcb_coord_t min, pcb_coord_t max)
{
	*entry = pcb_gtk_coord_entry_new(min, max, 0, conf_core.editor.grid_unit, CE_SMALL);
	_table_attach_(table, row, label, *entry);
}

static void _table_attach_spin(GtkWidget *table, gint row, const gchar *label, GtkWidget **entry, pcb_coord_t min, pcb_coord_t max)
{
	*entry = gtk_spin_button_new_with_range(min, max, 10);
	_table_attach_(table, row, label, *entry);
}

void pcb_gtk_route_style_edit_dialog(pcb_gtk_common_t *com, pcb_gtk_route_style_t *rss)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	pcb_gtk_dlg_route_style_t dialog_data;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(rss));
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *vbox, *hbox, *sub_vbox, *table;
	GtkWidget *label, *select_box;
	GtkWidget *button;
	const char *new_name;

	memset(&dialog_data, 0, sizeof(dialog_data));  /* make sure all flags are cleared */

	/* Build dialog */
	dialog = gtk_dialog_new_with_buttons(_("Edit Route Styles"),
																			 GTK_WINDOW(window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_NONE, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	label = gtk_label_new(_("Edit Style:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

	select_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(rss->model));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(select_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(select_box), renderer, "text", STYLE_TEXT_COL, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	hbox = gtkc_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 4);
	vbox = gtkc_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

	hbox = gtkc_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), select_box, TRUE, TRUE, 0);

	sub_vbox = ghid_category_vbox(vbox, _("Route Style Data"), 4, 2, TRUE, TRUE);
	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(sub_vbox), table, TRUE, TRUE, 4);
	label = gtk_label_new(_("Name:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	dialog_data.name_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
	gtk_table_attach(GTK_TABLE(table), dialog_data.name_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);

	_table_attach(table, 1, _("Line width:"), &dialog_data.line_entry, PCB_MIN_LINESIZE, PCB_MAX_LINESIZE);
	_table_attach_spin(table, 2, _("Text scale:"), &dialog_data.texts_entry, 0, 10000);
	_table_attach(table, 3, _("Text thickness:"), &dialog_data.textt_entry, 0, PCB_MAX_LINESIZE);
	_table_attach(table, 4, _("Via hole size:"),
								&dialog_data.via_hole_entry, PCB_MIN_PINORVIAHOLE, PCB_MAX_PINORVIASIZE - PCB_MIN_PINORVIACOPPER);
	_table_attach(table, 5, _("Via ring size:"),
								&dialog_data.via_size_entry, PCB_MIN_PINORVIAHOLE + PCB_MIN_PINORVIACOPPER, PCB_MAX_PINORVIASIZE);
	_table_attach(table, 6, _("Clearance:"), &dialog_data.clearance_entry, 0, PCB_MAX_LINESIZE);

	_table_attach_(table, 7, "", gtk_label_new(""));

	/* create attrib table */
	{
		GType *ty;
		GtkCellRenderer *renderer;

		dialog_data.attr_table = gtk_tree_view_new();

		ty = malloc(sizeof(GType) * 2);
		ty[0] = G_TYPE_STRING;
		ty[1] = G_TYPE_STRING;
		dialog_data.attr_model = gtk_list_store_newv(2, ty);
		free(ty);

		renderer = gtk_cell_renderer_text_new();
		g_object_set(renderer, "editable", TRUE, NULL);
		g_signal_connect(renderer, "edited", G_CALLBACK(attr_edited_key_cb), &dialog_data);
		g_signal_connect(renderer, "editing-started", G_CALLBACK(attr_edit_started_cb), &dialog_data);
		g_signal_connect(renderer, "editing-canceled", G_CALLBACK(attr_edit_canceled_cb), &dialog_data);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dialog_data.attr_table), -1, "key", renderer, "text", 0, NULL);


		renderer = gtk_cell_renderer_text_new();
		g_object_set(renderer, "editable", TRUE, NULL);
		g_signal_connect(renderer, "edited", G_CALLBACK(attr_edited_val_cb), &dialog_data);
		g_signal_connect(renderer, "editing-started", G_CALLBACK(attr_edit_started_cb), &dialog_data);
		g_signal_connect(renderer, "editing-canceled", G_CALLBACK(attr_edit_canceled_cb), &dialog_data);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dialog_data.attr_table), -1, "value", renderer, "text", 1, NULL);

		gtk_tree_view_set_model(GTK_TREE_VIEW(dialog_data.attr_table), GTK_TREE_MODEL(dialog_data.attr_model));
		g_signal_connect(G_OBJECT(dialog_data.attr_table), "key-release-event", G_CALLBACK(attr_key_release_cb), &dialog_data);

	}
	_table_attach_(table, 8, _("Attributes:"), dialog_data.attr_table);


	/* create delete button */
	button = gtk_button_new_with_label(_("Delete Style"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(delete_button_cb), &dialog_data);
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 0);

	add_new_iter(rss);

	/* Display dialog */
	dialog_data.rss = rss;
	dialog_data.select_box = select_box;
	if (rss->active_style != NULL) {
		path = gtk_tree_row_reference_get_path(rss->active_style->rref);
		gtk_tree_model_get_iter(GTK_TREE_MODEL(rss->model), &iter, path);
		g_signal_connect(G_OBJECT(select_box), "changed", G_CALLBACK(dialog_style_changed_cb), &dialog_data);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(select_box), &iter);
	}
	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		int changed = 0, need_rebuild = 0;
		pcb_route_style_t *rst;
		pcb_gtk_obj_route_style_t *style;

		if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(select_box), &iter))
			goto cancel;
		gtk_tree_model_get(GTK_TREE_MODEL(rss->model), &iter, STYLE_DATA_COL, &style, -1);
		if (style == NULL) {
			int n = vtroutestyle_len(&PCB->RouteStyle);
			rst = vtroutestyle_get(&PCB->RouteStyle, n, 1);
			need_rebuild = 1;
			changed = 1;
		}
		else {
			rst = style->rst;
			*rst->name = '\0';
		}

		new_name = gtk_entry_get_text(GTK_ENTRY(dialog_data.name_entry));

		while (isspace(*new_name))
			new_name++;
		if (strcmp(rst->name, new_name) != 0) {
			strncpy(rst->name, new_name, sizeof(rst->name) - 1);
			rst->name[sizeof(rst->name) - 1] = '0';
			changed = 1;
		}

/* Modify the route style only if there's significant difference (beyond rouding errors) */
#define rst_modify(changed, dst, src) \
	do { \
		pcb_coord_t __tmp__ = src; \
		if (abs(dst - __tmp__) > 10) { \
			changed = 1; \
			dst = __tmp__; \
		} \
	} while(0)
		rst_modify(changed, rst->Thick, pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(dialog_data.line_entry)));
		rst_modify(changed, rst->textt, pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(dialog_data.textt_entry)));
		rst_modify(changed, rst->texts, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog_data.texts_entry)));
		rst_modify(changed, rst->Hole, pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(dialog_data.via_hole_entry)));
		rst_modify(changed, rst->Diameter, pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(dialog_data.via_size_entry)));
		rst_modify(changed, rst->Clearance, pcb_gtk_coord_entry_get_value(GHID_COORD_ENTRY(dialog_data.clearance_entry)));
#undef rst_modify
		if (style == NULL) {
			style = pcb_gtk_route_style_add_route_style(rss, rst, 0);
		}
		else {
			gtk_action_set_label(GTK_ACTION(style->action), rst->name);
			gtk_list_store_set(rss->model, &iter, STYLE_TEXT_COL, rst->name, -1);
		}

		/* Cleanup */
		gtk_widget_destroy(dialog);
		gtk_list_store_remove(rss->model, &rss->new_iter);

		/* if the style array in core might have been reallocated, we need to update
		   all our pointers in the rss "cache" */
		if (need_rebuild) {
			pcb_gtk_route_style_empty(rss);
			make_route_style_buttons(GHID_ROUTE_STYLE(rss));
		}

		/* Emit change signals */
		pcb_gtk_route_style_select_style(rss, rst);

		if (changed) {
			pcb_board_set_changed_flag(pcb_true);
			com->window_set_name_label(PCB->Name);
		}

		if (changed || need_rebuild)
			pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	}
	else {
	cancel:;
		gtk_widget_destroy(dialog);
		gtk_list_store_remove(rss->model, &rss->new_iter);
	}
}
