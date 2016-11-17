/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* object property edit dialog */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "config.h"
#include "gui.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "polygon.h"
#include "obj_all.h"

static char *str_sub(const char *val, char sepi, char sepo)
{
	char *tmp, *sep;
	tmp = pcb_strdup(val);
	sep = strchr(tmp, sepi);
	if (sep != NULL)
		*sep = sepo;
	return tmp;
}

static void val_combo_changed_cb(GtkComboBox * combo, ghid_propedit_dialog_t *dlg)
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

static void val_entry_changed_cb(GtkWidget *entry, ghid_propedit_dialog_t *dlg)
{
	if (dlg->stock_val == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(dlg->val_box), -1);
	else
		dlg->stock_val = 0;
}

static void val_combo_reset(ghid_propedit_dialog_t *dlg)
{
	gtk_list_store_clear(dlg->vals);
}

static void val_combo_add(ghid_propedit_dialog_t *dlg, const char *val)
{
	gtk_list_store_insert_with_values(dlg->vals, NULL, -1, 0, val, -1);
}

#define NO0(s) pcb_strdup((s) == NULL ? "" : (s))
void ghid_propedit_prop_add(ghid_propedit_dialog_t *dlg, const char *name, const char *common, const char *min, const char *max, const char *avg)
{
	gtk_list_store_insert_with_values(dlg->props, &dlg->last_add_iter, -1,  0,pcb_strdup(name),  1,NO0(common),  2,NO0(min),  3,NO0(max),  4,NO0(avg),  -1);
	dlg->last_add_iter_valid = 1;
}

static void hdr_add(ghid_propedit_dialog_t *dlg, const char *name, int col)
{
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dlg->tree), -1, name, renderer, "text", col, NULL);
}


static void list_cursor_changed_cb(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
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

	printf("prop: %s!\n", prop);

	val_combo_reset(dlg);

	val = ghidgui->propedit_query(ghidgui->propedit_pe, "v1st", prop, NULL, 0);
	while(val != NULL) {
		tmp = str_sub(val, '\n', '=');
		val_combo_add(dlg, tmp);
		free(tmp);
		val = ghidgui->propedit_query(ghidgui->propedit_pe, "vnxt", prop, NULL, 0);
	}

	tmp = str_sub(comm, '\n', '\0');
	gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), tmp);
	free(tmp);
}

static void do_remove_cb(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
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

	if (ghidgui->propedit_query(ghidgui->propedit_pe, "vdel", prop, NULL, 0) != NULL)
		gtk_list_store_remove(GTK_LIST_STORE(tm), &iter);

	free(prop);
}

static int keyval_input(char **key, char **val)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *vbox, *label, *kentry, *ventry;
	gboolean response;
	GHidPort *out = &ghid_port;

	dialog = gtk_dialog_new_with_buttons("New attribute",
																			 GTK_WINDOW(out->top_window),
																			 GTK_DIALOG_MODAL,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	vbox = gtk_vbox_new(FALSE, 4);
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


static void do_addattr_cb(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
{
	char *name, *value;

	if (keyval_input(&name, &value)) {
		char *path;
		if ((name[0] == 'a') && (name[1] == '/')) {
			path = name;
			name = NULL;
		}
		else {
			int len = strlen(name);
			path = malloc(len+3);
			path[0] = 'a';
			path[1] = '/';
			strcpy(path+2, name);
		}
		ghidgui->propedit_query(ghidgui->propedit_pe, "vset", path, value, 0);
		free(path);
		free(name);
	}
}

static void do_apply_cb(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
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

	typ = ghidgui->propedit_query(ghidgui->propedit_pe, "type", prop, NULL, 0);
	if (typ != NULL) {
		if (*typ == 'c') { /* if type of the field if coords, we may need to fix missing units */
			char *end;
			strtod(val, &end);
			while(isspace(*end)) end++;
			if (*end == '\0') { /* no unit - automatically append current default */
				int len = strlen(val);
				char *new_val;
				new_val = malloc(len+32);
				strcpy(new_val, val);
				sprintf(new_val+len, " %s", conf_core.editor.grid_unit->suffix);
				free(val);
				val = new_val;
			}
		}

		if (ghidgui->propedit_query(ghidgui->propedit_pe, "vset", prop, val, 0) != NULL) {
			/* could change values update the table - the new row is already added, remove the old */
			gtk_list_store_remove(GTK_LIST_STORE(tm), &iter);
			if (dlg->last_add_iter_valid) {
				gtk_tree_selection_select_iter(tsel, &dlg->last_add_iter);
				dlg->last_add_iter_valid = 0;
			}
			/* get the combo box updated */
			list_cursor_changed_cb(dlg->tree, dlg);
			if ((*val == '+') || (*val == '-'))
				gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), val); /* keep relative values intact for a reapply */
		}
		else
			pcb_message(PCB_MSG_WARNING, "Failed to change any object - %s is possibly invalid value for %s\n", val, prop);
	}
	else
		pcb_message(PCB_MSG_ERROR, "Internal error: no type for proeprty %s\n", prop);

	free(val);
	g_free(prop);
}

static GdkPixmap *pm;
static gboolean preview_expose_event(GtkWidget *w, GdkEventExpose *event)
{
	gdk_draw_drawable(w->window, w->style->fg_gc[gtk_widget_get_state(w)],
		pm,
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);
	return FALSE;
}

static pcb_board_t preview_pcb;

static GtkWidget *preview_init(ghid_propedit_dialog_t *dlg)
{
	GtkWidget *area = gtk_drawing_area_new();
	pcb_board_t *old_pcb;
	int n, zoom1, fx, fy;
	pcb_coord_t cx, cy;

/*
	void *v;
	v = pcb_poly_new_from_rectangle(PCB->Data->Layer+1,
		PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(0),
		PCB_MIL_TO_COORD(1500), PCB_MIL_TO_COORD(1500),
		pcb_flag_make(PCB_FLAG_CLEARPOLY | PCB_FLAG_FULLPOLY));
	printf("poly2=%p -----------\n", (void *)v);
	DrawPolygon(PCB->Data->Layer+1, v);
	pcb_draw();
	gtk_drawing_area_size(GTK_DRAWING_AREA(area), 300, 400);
	return;
*/

	memset(&preview_pcb, 0, sizeof(preview_pcb));
	preview_pcb.Data = pcb_buffer_new();
	preview_pcb.MaxWidth = preview_pcb.MaxHeight = PCB_MIL_TO_COORD(2000);
	pcb_colors_from_settings(&preview_pcb);
	pcb_font_create_default(&preview_pcb);
	preview_pcb.ViaOn = 1;

	for(n = 0; n < pcb_max_copper_layer+2; n++) {
		preview_pcb.Data->Layer[n].On = 1;
		preview_pcb.Data->Layer[n].Color = pcb_strdup(PCB->Data->Layer[n].Color);
		preview_pcb.Data->Layer[n].Name = pcb_strdup("preview dummy");
	}

	memcpy(&preview_pcb.LayerGroups, &PCB->LayerGroups, sizeof(PCB->LayerGroups));
	preview_pcb.Data->LayerN = pcb_max_copper_layer;
	preview_pcb.Data->pcb = &preview_pcb;

#warning TODO: preview_pcb is never freed

	pcb_via_new(preview_pcb.Data,
							PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
							PCB_MIL_TO_COORD(50), PCB_MIL_TO_COORD(10), 0, PCB_MIL_TO_COORD(20), "", pcb_no_flags());

	pcb_line_new(preview_pcb.Data->Layer+0,
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1300),
		PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(20), pcb_flag_make(PCB_FLAG_CLEARLINE));

	pcb_arc_new(preview_pcb.Data->Layer+0,
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
		PCB_MIL_TO_COORD(100), PCB_MIL_TO_COORD(100),
		0.0, 90.0,
		PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(20), pcb_flag_make(PCB_FLAG_CLEARLINE));

		pcb_text_new(preview_pcb.Data->Layer+0, &PCB->Font,
							PCB_MIL_TO_COORD(850), PCB_MIL_TO_COORD(1150), 0, 100, "Text", pcb_flag_make(PCB_FLAG_CLEARLINE));

	{
		pcb_polygon_t *v = pcb_poly_new_from_rectangle(preview_pcb.Data->Layer,
			PCB_MIL_TO_COORD(10), PCB_MIL_TO_COORD(10),
			PCB_MIL_TO_COORD(1200), PCB_MIL_TO_COORD(1200),
			pcb_flag_make(PCB_FLAG_CLEARPOLY));
		pcb_poly_init_clip(preview_pcb.Data, preview_pcb.Data->Layer, v);
	}

	old_pcb = PCB;
	PCB = &preview_pcb;

	zoom1 = 1;
	cx = PCB_MIL_TO_COORD(1000+300/2*zoom1);
	cy = PCB_MIL_TO_COORD(1000+400/2*zoom1);
	fx = conf_core.editor.view.flip_x;
	fy = conf_core.editor.view.flip_y;
	conf_set(CFR_DESIGN, "editor/view/flip_x", -1, "0", POL_OVERWRITE);
	conf_set(CFR_DESIGN, "editor/view/flip_y", -1, "0", POL_OVERWRITE);
	pm = ghid_render_pixmap(cx, cy,
	40000 * zoom1, 300, 400, gdk_drawable_get_depth(GDK_DRAWABLE(gport->top_window->window)));
	conf_setf(CFR_DESIGN, "editor/view/flip_x", -1, "%d", fx, POL_OVERWRITE);
	conf_setf(CFR_DESIGN, "editor/view/flip_y", -1, "%d", fy, POL_OVERWRITE);

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


	PCB = old_pcb;


	g_signal_connect(G_OBJECT(area), "expose-event", G_CALLBACK(preview_expose_event), pm);
	return area;
}

/*static void sort_by_name(GtkTreeModel *liststore)
{
	GtkTreeSortable *sortable = GTK_TREE_SORTABLE(liststore);
	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);
}*/

static gint sort_name_cmp(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
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
	else if ((*name1 == 'a') && (*name2 == 'p')) /* force attributes to the bottom */
		return 1;
	else if ((*name1 == 'p') && (*name2 == 'a'))
		return -1;
	else
		ret = strcmp(name1, name2);


	g_free(name1);
	g_free(name2);

	return ret;
}

static void make_sortable(GtkTreeModel *liststore)
{
	GtkTreeSortable *sortable;

	sortable = GTK_TREE_SORTABLE(liststore);
	gtk_tree_sortable_set_sort_func(sortable, 0, sort_name_cmp, GINT_TO_POINTER(0), NULL);

	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);
}

GtkWidget *ghid_propedit_dialog_create(ghid_propedit_dialog_t *dlg)
{
	GtkWidget *window, *vbox_tree, *vbox_edit, *hbox_win, *label, *hbx, *dummy, *box_val_edit, *preview;
	GtkCellRenderer *renderer ;
	GtkWidget *content_area, *top_window = gport->top_window;

	dlg->last_add_iter_valid = 0;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	window = gtk_dialog_new_with_buttons(_("Edit Properties"),
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));

	hbox_win = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), hbox_win);


	vbox_tree = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_win), vbox_tree, TRUE, TRUE, 4);
	vbox_edit = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_win), vbox_edit, TRUE, TRUE, 4);

/***** LEFT *****/

	label = gtk_label_new("Properties");
	gtk_box_pack_start(GTK_BOX(vbox_tree), label, FALSE, FALSE, 4);

	dlg->tree = gtk_tree_view_new();
	gtk_box_pack_start(GTK_BOX(vbox_tree), dlg->tree, FALSE, TRUE, 4);

	GType ty[5];

	int n;
	for(n = 0; n < 5; n++)
		ty[n] = G_TYPE_STRING;
	dlg->props = gtk_list_store_newv(5, ty);
	make_sortable((GtkTreeModel *)dlg->props);
	gtk_tree_view_set_model(GTK_TREE_VIEW(dlg->tree), GTK_TREE_MODEL(dlg->props));

	hdr_add(dlg, "property", 0);
	hdr_add(dlg, "common", 1);
	hdr_add(dlg, "min", 2);
	hdr_add(dlg, "max", 3);
	hdr_add(dlg, "avg", 4);

	/* dummy box to eat up vertical space */
	hbx = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_tree), hbx, TRUE, TRUE, 4);

	/* list manipulation */
	hbx = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_tree), hbx, FALSE, TRUE, 4);

	dlg->remove = gtk_button_new_with_label("Remove attribute");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->remove, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->remove), "clicked", G_CALLBACK(do_remove_cb), dlg);

	dlg->addattr = gtk_button_new_with_label("Add attribute");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->addattr, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->addattr), "clicked", G_CALLBACK(do_addattr_cb), dlg);


/***** RIGHT *****/
/* preview */
	preview = preview_init(dlg);
	gtk_box_pack_start(GTK_BOX(vbox_edit), preview, TRUE, TRUE, 4);

	label = gtk_label_new("Change property of all objects");
	gtk_box_pack_start(GTK_BOX(vbox_edit), label, FALSE, TRUE, 4);

	g_signal_connect(G_OBJECT(dlg->tree), "cursor-changed", G_CALLBACK(list_cursor_changed_cb), dlg);

	/* value edit */
	renderer = gtk_cell_renderer_text_new();

	dlg->stock_val = 0;
	dlg->vals = gtk_list_store_new(1, G_TYPE_STRING);
	box_val_edit = gtk_vbox_new(FALSE, 0);
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
	hbx = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_edit), hbx, FALSE, TRUE, 4);

	dummy = gtk_vbox_new(FALSE, 0); /* dummy box to eat up free space on the left */
	gtk_box_pack_start(GTK_BOX(hbx), dummy, TRUE, TRUE, 4);

	dlg->apply = gtk_button_new_with_label("Apply");
	gtk_box_pack_start(GTK_BOX(hbx), dlg->apply, FALSE, TRUE, 4);
	g_signal_connect(G_OBJECT(dlg->apply), "clicked", G_CALLBACK(do_apply_cb), dlg);


	gtk_widget_show_all(window);

	return window;
}
