#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "create.h"

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
	const char *prop, *comm, *val;

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
		val_combo_add(dlg, val);
		val = ghidgui->propedit_query(ghidgui->propedit_pe, "vnxt", prop, NULL, 0);
	}

	gtk_entry_set_text(GTK_ENTRY(dlg->entry_val), comm);
}

static void do_apply(GtkWidget *tree, ghid_propedit_dialog_t *dlg)
{
	printf("Apply!\n");
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

static PCBType preview_pcb;

static GtkWidget *preview_init(ghid_propedit_dialog_t *dlg)
{
	GtkWidget *area = gtk_drawing_area_new();
	PCBType *old_pcb;
	int n;
	void *v;

/*
	v = CreateNewPolygonFromRectangle(PCB->Data->Layer+1,
		PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(0),
		PCB_MIL_TO_COORD(1500), PCB_MIL_TO_COORD(1500),
		MakeFlags(PCB_FLAG_CLEARPOLY | PCB_FLAG_FULLPOLY));
	printf("poly2=%p -----------\n", v);
	DrawPolygon(PCB->Data->Layer+1, v);
	Draw();
	gtk_drawing_area_size(GTK_DRAWING_AREA(area), 300, 400);
	return;
*/

	memset(&preview_pcb, 0, sizeof(preview_pcb));
	preview_pcb.Data = CreateNewBuffer();
	preview_pcb.MaxWidth = preview_pcb.MaxHeight = PCB_MIL_TO_COORD(2000);
	pcb_colors_from_settings(&preview_pcb);
	CreateDefaultFont(&preview_pcb);
	preview_pcb.ViaOn = 1;

	for(n = 0; n < max_copper_layer+2; n++) {
		preview_pcb.Data->Layer[n].On = 1;
		preview_pcb.Data->Layer[n].Color = pcb_strdup(PCB->Data->Layer[n].Color);
		preview_pcb.Data->Layer[n].Name = pcb_strdup("preview dummy");
	}

	memcpy(&preview_pcb.LayerGroups, &PCB->LayerGroups, sizeof(PCB->LayerGroups));
	preview_pcb.Data->LayerN = max_copper_layer;
	preview_pcb.Data->pcb = &preview_pcb;

#warning TODO: preview_pcb is never freed

	CreateNewVia(preview_pcb.Data,
							PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
							PCB_MIL_TO_COORD(50), PCB_MIL_TO_COORD(10), 0, PCB_MIL_TO_COORD(30), "", NoFlags());

	CreateNewLineOnLayer(preview_pcb.Data->Layer+0,
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1300),
		PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(20), NoFlags());

	CreateNewArcOnLayer(preview_pcb.Data->Layer+0,
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
		PCB_MIL_TO_COORD(100), PCB_MIL_TO_COORD(100),
		0.0, 90.0, 
		PCB_MIL_TO_COORD(20), PCB_MIL_TO_COORD(20), NoFlags());

/*	v = CreateNewPolygonFromRectangle(preview_pcb.Data->Layer+0,
		PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(0),
		PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
		NoFlags());
printf("poly=%p\n", v);
			DrawPolygon(preview_pcb.Data->Layer+0, v);*/


	old_pcb = PCB;
	PCB = &preview_pcb;

	pm = ghid_render_pixmap(PCB_MIL_TO_COORD(1000), PCB_MIL_TO_COORD(1000),
	20000, 300, 400, gdk_drawable_get_depth(GDK_DRAWABLE(gport->top_window->window)));
	PCB = old_pcb;

	g_signal_connect(G_OBJECT(area), "expose-event", G_CALLBACK(preview_expose_event), pm);
	return area;
}

GtkWidget *ghid_propedit_dialog_create(ghid_propedit_dialog_t *dlg)
{
	GtkWidget *window, *vbox_tree, *vbox_edit, *hbox_win, *label, *hbx, *btn, *dummy, *box_val_edit, *preview;
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

/*	label = gtk_label_new("(preview)");
	gtk_box_pack_start(GTK_BOX(vbox_edit), label, TRUE, TRUE, 4);*/
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
	g_signal_connect(G_OBJECT(dlg->apply), "clicked", G_CALLBACK(do_apply), dlg);


	gtk_widget_show_all(window);

	return window;
}
