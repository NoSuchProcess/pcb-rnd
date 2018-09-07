/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* object property edit dialog */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

#include "dlg_propedit.h"

#include "compat_misc.h"
#include "compat_nls.h"
#include "polygon.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "buffer.h"
#include "draw.h"

#include "bu_box.h"
#include "wt_preview.h"
#include "compat.h"

static char *str_sub(const char *val, char sepi, char sepo)
{
	char *tmp, *sep;
	tmp = pcb_strdup(val);
	sep = strchr(tmp, sepi);
	if (sep != NULL)
		*sep = sepo;
	return tmp;
}

static void val_combo_changed_cb(GtkComboBox * combo, pcb_gtk_dlg_propedit_t * dlg)
{
	char *cval, *tmp;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(dlg->vals), &iter, 0, &cval, -1);
		dlg->stock_val = 1;
		tmp = str_sub(cval, '=', '\0');
		gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), tmp);
		free(tmp);
	}
}

static void val_entry_changed_cb(GtkWidget * entry, pcb_gtk_dlg_propedit_t * dlg)
{
	if (dlg->stock_val == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(dlg->val_box), -1);
	else
		dlg->stock_val = 0;
}

static void val_combo_reset(pcb_gtk_dlg_propedit_t * dlg)
{
	gtk_list_store_clear(dlg->vals);
}

static void val_combo_add(pcb_gtk_dlg_propedit_t * dlg, const char *val)
{
	gtk_list_store_insert_with_values(dlg->vals, NULL, -1, 0, val, -1);
}

#define NO0(s) pcb_strdup((s) == NULL ? "" : (s))
void pcb_gtk_dlg_propedit_prop_add(pcb_gtk_dlg_propedit_t * dlg, const char *name, const char *common,
																	 const char *min, const char *max, const char *avg)
{
	gtk_list_store_insert_with_values(dlg->props, &dlg->last_add_iter, -1, 0,
																		pcb_strdup(name), 1, NO0(common), 2, NO0(min), 3, NO0(max), 4, NO0(avg), -1);
	dlg->last_add_iter_valid = 1;
}

static void hdr_add(pcb_gtk_dlg_propedit_t * dlg, const char *name, int col)
{
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dlg->tree), -1, name, renderer, "text", col, NULL);
}

static void list_cursor_changed_cb(GtkWidget * tree, pcb_gtk_dlg_propedit_t * dlg)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	const char *prop, *comm, *val;
	char *tmp;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->tree));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, 0, &prop, 1, &comm, -1);

	val_combo_reset(dlg);

	val = dlg->propedit_query(dlg->propedit_pe, "v1st", prop, NULL, 0);
	while (val != NULL) {
		tmp = str_sub(val, '\n', '=');
		val_combo_add(dlg, tmp);
		free(tmp);
		val = dlg->propedit_query(dlg->propedit_pe, "vnxt", prop, NULL, 0);
	}

	tmp = str_sub(comm, '\n', '\0');
	gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), tmp);
	free(tmp);
}

static void do_remove_cb(GtkWidget * tree, pcb_gtk_dlg_propedit_t * dlg)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	char *prop;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->tree));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, 0, &prop, -1);

	if (dlg->propedit_query(dlg->propedit_pe, "vdel", prop, NULL, 0) != NULL)
		gtk_list_store_remove(GTK_LIST_STORE(tm), &iter);

	free(prop);
}

static int keyval_input(char **key, char **val, pcb_gtk_dlg_propedit_t * dlg)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *vbox, *label, *kentry, *ventry;
	gboolean response;

	dialog = gtk_dialog_new_with_buttons("New attribute",
																			 GTK_WINDOW(dlg->com->top_window),
																			 GTK_DIALOG_MODAL,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	vbox = gtkc_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Attribute key:");

	kentry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(kentry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), kentry, TRUE, TRUE, 0);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "Attribute value:");

	ventry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(ventry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), ventry, TRUE, TRUE, 0);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_OK) {
		*key = gtk_editable_get_chars(GTK_EDITABLE(kentry), 0, -1);
		*val = gtk_editable_get_chars(GTK_EDITABLE(ventry), 0, -1);
		gtk_widget_destroy(dialog);
		return 1;
	}

	gtk_widget_destroy(dialog);
	return 0;
}

static void do_addattr_cb(GtkWidget * tree, pcb_gtk_dlg_propedit_t * dlg)
{
	char *name, *value;

	if (keyval_input(&name, &value, dlg)) {
		char *path;
		if ((name[0] == 'a') && (name[1] == '/')) {
			path = name;
			name = NULL;
		}
		else {
			int len = strlen(name);
			path = malloc(len + 3);
			path[0] = 'a';
			path[1] = '/';
			strcpy(path + 2, name);
		}
		dlg->propedit_query(dlg->propedit_pe, "vset", path, value, 0);
		free(path);
		free(name);
	}
}

static void do_apply_cb(GtkWidget * tree, pcb_gtk_dlg_propedit_t * dlg)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreeIter iter;
	char *prop, *val;
	const char *typ;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->tree));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, 0, &prop, -1);

	val = pcb_strdup(gtk_entry_get_text(GTK_ENTRY(dlg->entry_val)));

	typ = dlg->propedit_query(dlg->propedit_pe, "type", prop, NULL, 0);
	if (typ != NULL) {
		if (*typ == 'c') {					/* if type of the field if coords, we may need to fix missing units */
			char *end;
			strtod(val, &end);
			while (isspace(*end))
				end++;
			if (*end == '\0') {				/* no unit - automatically append current default */
				int len = strlen(val);
				char *new_val;
				new_val = malloc(len + 32);
				strcpy(new_val, val);
				sprintf(new_val + len, " %s", conf_core.editor.grid_unit->suffix);
				free(val);
				val = new_val;
			}
		}

		if (dlg->propedit_query(dlg->propedit_pe, "vset", prop, val, 0) != NULL) {
			/* could change values update the table - the new row is already added, remove the old */
			gtk_list_store_remove(GTK_LIST_STORE(tm), &iter);
			if (dlg->last_add_iter_valid) {
				gtk_tree_selection_select_iter(tsel, &dlg->last_add_iter);
				dlg->last_add_iter_valid = 0;
			}
			/* get the combo box updated */
			list_cursor_changed_cb(dlg->tree, dlg);
			if ((*val == '+') || (*val == '-') || (*val == '#'))
				gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), val);	/* keep relative and forced absolute values intact for a reapply */
			pcb_gui->invalidate_all();
		}
		else
			pcb_message(PCB_MSG_WARNING, "Failed to change any object - %s is possibly invalid value for %s\n", val, prop);
	}
	else
		pcb_message(PCB_MSG_ERROR, "Internal error: no type for proeprty %s\n", prop);

	free(val);
	g_free(prop);
}

static pcb_board_t preview_pcb;

static void prop_preview_init(void)
{
	pcb_poly_t *v;
	pcb_layer_id_t n;
	pcb_board_t *old_pcb;
	old_pcb = PCB;
	PCB = &preview_pcb;

	memset(&preview_pcb, 0, sizeof(preview_pcb));
	preview_pcb.Data = pcb_buffer_new(&preview_pcb);
	preview_pcb.MaxWidth = preview_pcb.MaxHeight = PCB_MIL_TO_COORD(1000);
	pcb_font_create_default(&preview_pcb);
	preview_pcb.pstk_on = 1;

	for (n = 0; n < 2; n++) {
		preview_pcb.Data->Layer[n].is_bound = 0;
		preview_pcb.Data->Layer[n].meta.real.vis = 1;
		preview_pcb.Data->Layer[n].meta.real.color = pcb_strdup(old_pcb->Data->Layer[n].meta.real.color);
		preview_pcb.Data->Layer[n].name = pcb_strdup("preview dummy");
		
	}

	memcpy(&preview_pcb.LayerGroups, &old_pcb->LayerGroups, sizeof(old_pcb->LayerGroups));
	preview_pcb.Data->LayerN = 1;
	preview_pcb.Data->Layer[0].meta.real.grp = 0;
	preview_pcb.LayerGroups.grp[0].ltype = PCB_LYT_COPPER | PCB_LYT_TOP;
	preview_pcb.LayerGroups.grp[0].lid[0] = 0;
	preview_pcb.LayerGroups.grp[0].len = 1;
	preview_pcb.LayerGroups.grp[0].parent.any = NULL;
	preview_pcb.LayerGroups.grp[0].parent_type = PCB_PARENT_INVALID;
	preview_pcb.LayerGroups.grp[0].type = PCB_OBJ_LAYERGRP;
	preview_pcb.LayerGroups.len = 1;
	PCB_SET_PARENT(preview_pcb.Data, board, &preview_pcb);
	preview_pcb.Data->Layer[0].parent.data = preview_pcb.Data;
	preview_pcb.Data->Layer[0].parent_type = PCB_PARENT_DATA;
	preview_pcb.Data->Layer[0].type = PCB_OBJ_LAYER;

#warning TODO: preview_pcb is never freed

#warning padstack TODO: create a padstack
#if 0
	pcb_via_new(preview_pcb.Data,
							PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
							PCB_MIL_TO_COORD(50), PCB_MIL_TO_COORD(40), 0, PCB_MIL_TO_COORD(20), "", pcb_no_flags());
#endif

	pcb_line_new(preview_pcb.Data->Layer + 0,
							 PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
							 PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1300),
							 PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(40), pcb_flag_make(PCB_FLAG_CLEARLINE));

	pcb_arc_new(preview_pcb.Data->Layer + 0,
							PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
							PCB_MIL_TO_COORD(100), PCB_MIL_TO_COORD(100),
							0.0, 90.0, PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(40), pcb_flag_make(PCB_FLAG_CLEARLINE), pcb_false);

	pcb_text_new(preview_pcb.Data->Layer + 0, pcb_font(PCB, 0, 1),
							 PCB_MIL_TO_COORD(850), PCB_MIL_TO_COORD(1150), 0, 100, "Text", pcb_flag_make(PCB_FLAG_CLEARLINE));

	v = pcb_poly_new_from_rectangle(preview_pcb.Data->Layer + 0,
																									 PCB_MIL_TO_COORD(10), PCB_MIL_TO_COORD(10),
																									 PCB_MIL_TO_COORD(1200), PCB_MIL_TO_COORD(1200),
																									 0, pcb_flag_make(PCB_FLAG_CLEARPOLY));
	pcb_poly_init_clip(preview_pcb.Data, preview_pcb.Data->Layer + 0, v);
	PCB = old_pcb;

/*
	{
		GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(gport->top_window->window));
		GdkColor clr = {0, 0, 0, 0};
		int x, y;
		double zm = 40000 * zoom1;

		gdk_gc_set_rgb_fg_color(gc, &clr);

		x = (PCB_MIL_TO_COORD(1000) - cx) / zm + 0.5 + 200;
		y = (PCB_MIL_TO_COORD(1000) - cy) / zm + 0.5 + 150;
		gdk_draw_line(pm, gc, x, y, 0, 0);
		gdk_draw_line(pm, gc, x+1, y+1, 1, 1);
		gdk_draw_line(pm, gc, x-1, y-1, -1, -1);
	}
*/
}

static void prop_preview_draw(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_board_t *old_pcb;
	old_pcb = PCB;
	PCB = &preview_pcb;
	pcb_draw_layer(&(PCB->Data->Layer[0]), &e->view, NULL);
	pcb_draw_ppv(0, &e->view);
	PCB = old_pcb;
}


/*static void sort_by_name(GtkTreeModel *liststore)
{
	GtkTreeSortable *sortable = GTK_TREE_SORTABLE(liststore);
	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);
}*/

static gint sort_name_cmp(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer userdata)
{
	gint sortcol = GPOINTER_TO_INT(userdata);
	gint ret = 0;
	gchar *name1, *name2;

	if (sortcol != 0)
		return 0;

	gtk_tree_model_get(model, a, 0, &name1, -1);
	gtk_tree_model_get(model, b, 0, &name2, -1);

	if ((name1 == NULL) && (name2 == NULL))
		return 0;

	if (name1 == NULL)
		ret = -1;
	else if (name2 == NULL)
		ret = 1;
	else if ((*name1 == 'a') && (*name2 == 'p'))	/* force attributes to the bottom */
		return 1;
	else if ((*name1 == 'p') && (*name2 == 'a'))
		return -1;
	else
		ret = strcmp(name1, name2);

	g_free(name1);
	g_free(name2);

	return ret;
}

static void make_sortable(GtkTreeModel * liststore)
{
	GtkTreeSortable *sortable;

	sortable = GTK_TREE_SORTABLE(liststore);
	gtk_tree_sortable_set_sort_func(sortable, 0, sort_name_cmp, GINT_TO_POINTER(0), NULL);

	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);
}

GtkWidget *pcb_gtk_dlg_propedit_create(pcb_gtk_dlg_propedit_t *dlg, pcb_gtk_common_t *com)
{
	GtkWidget *window, *vbox_tree, *vbox_edit, *hbox_win, *label, *scrolled_win;
	GtkWidget *hbx, *dummy, *box_val_edit, *prv;
	GtkCellRenderer *renderer;
	GtkWidget *content_area;
	pcb_gtk_preview_t *p;


	dlg->last_add_iter_valid = 0;
	dlg->com = com;

	window = gtk_dialog_new_with_buttons(_("Edit Properties"),
																			 GTK_WINDOW(com->top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));

	hbox_win = gtkc_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), hbox_win);

	vbox_tree = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_win), vbox_tree, TRUE, TRUE, 4);
	vbox_edit = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_win), vbox_edit, FALSE, FALSE, 4);

/***** LEFT *****/

	label = gtk_label_new("Properties");
	gtk_box_pack_start(GTK_BOX(vbox_tree), label, FALSE, FALSE, 4);

	scrolled_win = GTK_WIDGET(g_object_new(GTK_TYPE_SCROLLED_WINDOW,
		"hscrollbar-policy", GTK_POLICY_AUTOMATIC,
		"vscrollbar-policy", GTK_POLICY_AUTOMATIC,
		"shadow-type", GTK_SHADOW_ETCHED_IN,
		NULL));
	gtk_box_pack_start(GTK_BOX(vbox_tree), scrolled_win, TRUE, TRUE, 0);
	gtk_widget_set_size_request(scrolled_win, 500, 150); /* enlarge the window initially */

	dlg->tree = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scrolled_win), dlg->tree);

	GType ty[5];

	int n;
	for (n = 0; n < 5; n++)
		ty[n] = G_TYPE_STRING;
	dlg->props = gtk_list_store_newv(5, ty);
	make_sortable((GtkTreeModel *) dlg->props);
	gtk_tree_view_set_model(GTK_TREE_VIEW(dlg->tree), GTK_TREE_MODEL(dlg->props));

	hdr_add(dlg, "property", 0);
	hdr_add(dlg, "common", 1);
	hdr_add(dlg, "min", 2);
	hdr_add(dlg, "max", 3);
	hdr_add(dlg, "avg", 4);

	/* list manipulation */
	hbx = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_tree), hbx, FALSE, FALSE, 4);

	dlg->remove = gtk_button_new_with_label("Remove attribute");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->remove, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->remove), "clicked", G_CALLBACK(do_remove_cb), dlg);

	dlg->addattr = gtk_button_new_with_label("Add attribute");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->addattr, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->addattr), "clicked", G_CALLBACK(do_addattr_cb), dlg);


/***** RIGHT *****/
/* preview */
	prop_preview_init();
	prv = pcb_gtk_preview_dialog_new(com, com->init_drawing_widget, com->preview_expose, prop_preview_draw);

	p = (pcb_gtk_preview_t *) prv;
	p->view.x0 = PCB_MIL_TO_COORD(800);
	p->view.y0 = PCB_MIL_TO_COORD(800);
	p->view.coord_per_px = PCB_MIL_TO_COORD(2.5);
	pcb_gtk_zoom_post(&p->view);

	gtk_widget_set_size_request(prv, 150, 150);

	gtk_box_pack_start(GTK_BOX(vbox_edit), prv, TRUE, TRUE, 4);

	label = gtk_label_new("Change property\nof all objects");
	gtk_box_pack_start(GTK_BOX(vbox_edit), label, FALSE, TRUE, 4);

	g_signal_connect(G_OBJECT(dlg->tree), "cursor-changed", G_CALLBACK(list_cursor_changed_cb), dlg);

	/* value edit */
	renderer = gtk_cell_renderer_text_new();

	dlg->stock_val = 0;
	dlg->vals = gtk_list_store_new(1, G_TYPE_STRING);
	box_val_edit = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_edit), box_val_edit, FALSE, TRUE, 4);

	/* combo */
	dlg->val_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(dlg->vals));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(dlg->val_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(dlg->val_box), renderer, "text", 0, NULL);
	gtk_box_pack_start(GTK_BOX(box_val_edit), dlg->val_box, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(dlg->val_box), "changed", G_CALLBACK(val_combo_changed_cb), dlg);

	/* entry */
	dlg->entry_val = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box_val_edit), dlg->entry_val, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(dlg->entry_val), "changed", G_CALLBACK(val_entry_changed_cb), dlg);

	/* Apply button */
	hbx = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_edit), hbx, FALSE, TRUE, 4);

	dummy = gtkc_vbox_new(FALSE, 0);	/* dummy box to eat up free space on the left */
	gtk_box_pack_start(GTK_BOX(hbx), dummy, TRUE, TRUE, 4);

	dlg->apply = gtk_button_new_with_label("Apply");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->apply, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->apply), "clicked", G_CALLBACK(do_apply_cb), dlg);

	/* Runs the dialog */
	gtk_widget_show_all(window);

	gtk_widget_set_size_request(scrolled_win, 100, 100); /* allow resizing to smaller */

	return window;
}
