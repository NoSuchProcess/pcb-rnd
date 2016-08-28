#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gui.h"

static void val_combo_changed_cb(GtkComboBox * combo, ghid_propedit_dialog_t *dlg)
{
	char *cval;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(dlg->vals), &iter, 0, &cval, -1);
		dlg->stock_val = 1;
		gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), cval);
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

void ghid_propedit_prop_add(ghid_propedit_dialog_t *dlg, const char *name, const char *common, const char *min, const char *max, const char *avg)
{
	GtkTreeIter i;
	gtk_list_store_insert_with_values(dlg->props, &i, -1,  0,name,  1,common,  2,min,  3,max,  4,avg,  -1);
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
	GtkTreePath *path;
	GtkTreeIter iter;
	const char *prop, *comm;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->tree));
	if (tsel == NULL)
		return;

	gtk_tree_selection_get_selected(tsel, &tm, &iter);
	if (iter.stamp == 0)
		return;

	gtk_tree_model_get(tm, &iter, 0, &prop, 1, &comm, -1);

	printf("prop: %s!\n", prop);

	val_combo_reset(dlg);
	val_combo_add(dlg, prop);
	val_combo_add(dlg, "val0");
	val_combo_add(dlg, "val1");

	gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), comm);
}

static void do_apply(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
{
	printf("Apply!\n");
}

GtkWidget *ghid_propedit_dialog_create(ghid_propedit_dialog_t *dlg)
{
	GtkWidget *window, *vbox_tree, *vbox_edit, *hbox_win, *label, *hbx, *btn, *dummy, *box_val_edit;
	GtkCellRenderer *renderer ;
	GtkWidget *content_area, *top_window = gport->top_window;

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


	label = gtk_label_new("Properies");
	gtk_box_pack_start(GTK_BOX(vbox_tree), label, FALSE, FALSE, 4);

	dlg->tree = gtk_tree_view_new();
	gtk_box_pack_start(GTK_BOX(vbox_tree), dlg->tree, FALSE, TRUE, 4);

	GType ty[5];

	int n;
	for(n = 0; n < 5; n++)
		ty[n] = G_TYPE_STRING;
	dlg->props = gtk_list_store_newv(5, ty);
	gtk_tree_view_set_model(GTK_TREE_VIEW(dlg->tree), GTK_TREE_MODEL(dlg->props));

	hdr_add(dlg, "property", 0);
	hdr_add(dlg, "common", 1);
	hdr_add(dlg, "min", 2);
	hdr_add(dlg, "max", 3);
	hdr_add(dlg, "avg", 4);

	label = gtk_label_new("(preview)");
	gtk_box_pack_start(GTK_BOX(vbox_edit), label, TRUE, TRUE, 4);

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
	g_signal_connect(G_OBJECT(dlg->apply), "clicked", G_CALLBACK(do_apply), dlg);


	gtk_widget_show_all(window);

	return window;
}
