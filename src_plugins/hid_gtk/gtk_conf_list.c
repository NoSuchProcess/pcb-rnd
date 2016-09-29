#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "gtk_conf_list.h"
#include "compat_misc.h"

static void fill_misc_cols(gtk_conf_list_t *cl, int row_idx, GtkTreeIter *iter, lht_node_t *nd)
{
	int col;
	if (cl->get_misc_col_data != NULL) {
		for(col = 0; col < cl->num_cols; col++) {
			if ((col == cl->col_data) || (col == cl->col_src))
				continue;
			char *s = cl->get_misc_col_data(row_idx, col, nd);
			if (s != NULL) {
				gtk_list_store_set(cl->l, iter, col, s, -1);
				free(s);
			}
		}
	}
}

/* Rebuild the list from the GUI. There would be a cheaper alternative for most
   operations, but our lists are short (less than 10 items usually) and
   edited rarely - optimize for code size instead of speed. This also solves
   a higher level problem: sometimes the input list is a composite from multiple
   sources whereas the output list must be all in CFR_DESIGN. */
static void rebuild(gtk_conf_list_t *cl)
{
	GtkTreeModel *tm = gtk_tree_view_get_model(GTK_TREE_VIEW(cl->t));
	GtkTreeIter it;
	gboolean valid;
	int n;

	if (cl->inhibit_rebuild)
		return;

	if (cl->pre_rebuild != NULL)
		cl->pre_rebuild(cl);

/*	printf("rebuild\n");*/

	for(valid = gtk_tree_model_get_iter_first(tm, &it), n = 0; valid; valid = gtk_tree_model_iter_next(tm, &it), n++) {
		gchar *s;
		lht_node_t *nd;

		gtk_tree_model_get(tm, &it, cl->col_data, &s, -1);
/*		printf(" -> %s\n", s);*/
		if (cl->col_src > 0)
			gtk_list_store_set(cl->l, &it, cl->col_src, "<not saved yet>", -1);

		nd = lht_dom_node_alloc(LHT_TEXT, "");
		nd->data.text.value = pcb_strdup(s == NULL ? "" : s);
		nd->doc = cl->lst->doc;
		lht_dom_list_append(cl->lst, nd);

		fill_misc_cols(cl, n, &it, nd);
		g_free(s);
	}

	if (cl->post_rebuild != NULL)
		cl->post_rebuild(cl);
}

static int get_sel(gtk_conf_list_t *cl, GtkTreeIter *iter)
{
	GtkTreeSelection *tsel;
	GtkTreeModel *tm;
	GtkTreePath *path;
	int *i;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cl->t));
	if (tsel == NULL)
		return -1;

	gtk_tree_selection_get_selected(tsel, &tm, iter);
	if (iter->stamp == 0)
		return -1;
	path = gtk_tree_model_get_path(tm, iter);
	if (path != NULL) {
		i = gtk_tree_path_get_indices(path);
		if (i != NULL)
			return i[0];
	}
	return -1;
}

static void row_insert_cb(GtkTreeModel *m, GtkTreePath *p, GtkTreeIter *iter, gtk_conf_list_t *cl)
{
	rebuild(cl);
}

static void row_delete_cb(GtkTreeModel *m, GtkTreePath *p, gtk_conf_list_t *cl)
{
	rebuild(cl);
}

static void button_ins_cb(GtkButton * button, gtk_conf_list_t *cl)
{
	GtkTreeIter *sibl, sibl_, iter;
	lht_node_t *nd;
	int idx = get_sel(cl, &sibl_);
	if (idx < 0) {
		idx = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(cl->t)), NULL);
		sibl = NULL;
	}
	else
		sibl = &sibl_;

	gtk_list_store_insert_before(cl->l, &iter, sibl);

	rebuild(cl);
#warning TODO: insert new item at idx
	nd = NULL;

	fill_misc_cols(cl, idx, &iter, nd);
	printf("ins %d!\n", idx);
}

static void button_del_cb(GtkButton * button, gtk_conf_list_t *cl)
{
	GtkTreeIter iter;
	int max, idx = get_sel(cl, &iter);

	if (idx < 0)
		return;

	printf("del %d!\n", idx);
	gtk_list_store_remove(cl->l, &iter);
	rebuild(cl);
#warning TODO: remove list item idx
	
	/* set cursor to where the user may expect it */
	max = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(cl->t)), NULL) - 1;
	if (idx > max)
		idx = max;
	if (idx >= 0) {
		GtkTreePath *path;
		path = gtk_tree_path_new_from_indices(idx, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(cl->t), path, NULL, 0);
	}
}

static void button_sel_cb(GtkButton * button, gtk_conf_list_t *cl)
{
	GtkWidget *fcd;
	GtkTreeIter iter;
	int idx = get_sel(cl, &iter);
	if (idx < 0)
		return;
	fcd = gtk_file_chooser_dialog_new(cl->file_chooser_title, NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_action(GTK_FILE_CHOOSER(fcd), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	if (gtk_dialog_run(GTK_DIALOG(fcd)) == GTK_RESPONSE_ACCEPT) {
		char *fno = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcd));
		const char *fn = fno;
		
		if (cl->file_chooser_postproc != NULL)
			fn = cl->file_chooser_postproc(fno);

		printf("sel %d '%s'\n", idx, fn);
		gtk_list_store_set(cl->l, &iter, cl->col_data, fn, -1);
#warning TODO: replace list item idx
		rebuild(cl);
		g_free(fno);
	}
	gtk_widget_destroy(fcd);
}


static void cell_edited_cb(GtkCellRendererText *cell, gchar *path, gchar *new_text, gtk_conf_list_t *cl)
{
	GtkTreeIter iter;
	int idx = get_sel(cl, &iter);

	cl->editing = 0;

	printf("edit %d to %s!\n", idx, new_text);
	gtk_list_store_set(cl->l, &iter, cl->col_data, new_text, -1);
#warning TODO: replace list item idx
	rebuild(cl);
}

static void cell_edit_started_cb(GtkCellRendererText *cell, GtkCellEditable *e,gchar *path, gtk_conf_list_t *cl)
{
	cl->editing = 1;
}

static void cell_edit_canceled_cb(GtkCellRendererText *cell, gtk_conf_list_t *cl)
{
	cl->editing = 0;
}

/* bind a few intuitive keys so that the list can be used without mouse */
gboolean key_release_cb(GtkWidget *widget, GdkEventKey *event, gtk_conf_list_t *cl)
{
	unsigned short int kv = event->keyval;
	if (cl->editing)
		return FALSE;
/*	printf("REL! %d %d\n", cl->editing, kv);*/
	switch(kv) {
#ifdef GDK_KEY_KP_Insert
		case GDK_KEY_KP_Insert:
#endif
#ifdef GDK_KEY_Insert
		case GDK_KEY_Insert:
#endif
		case 'i':
			button_ins_cb(NULL, cl);
			break;
#ifdef GDK_KEY_KP_Delete
		case GDK_KEY_KP_Delete:
#endif
#ifdef GDK_KEY_Delete
		case GDK_KEY_Delete:
#endif
		case 'd':
			button_del_cb(NULL, cl);
			break;
		case 'c':
		case 's':
			if (cl->file_chooser_title != NULL)
				button_sel_cb(NULL, cl);
			break;
	}
	return 0;
}

int gtk_conf_list_set_list(gtk_conf_list_t *cl, lht_node_t *lst)
{
	GtkTreeIter iter;
	lht_node_t *nd;

	if ((lst == NULL) || (lst->type != LHT_LIST))
		return -1;

	cl->lst = lst;
	cl->inhibit_rebuild = 1;

	gtk_list_store_clear(cl->l);

	/* fill in the list with initial data */
	for(nd = cl->lst->data.list.first; nd != NULL; nd = nd->next) {
		if (nd->type != LHT_TEXT)
			continue;

		gtk_list_store_append(cl->l, &iter);
		gtk_list_store_set(cl->l, &iter, cl->col_data, nd->data.text.value, -1);
		if (nd->file_name != NULL)
			gtk_list_store_set(cl->l, &iter, cl->col_src, nd->file_name, -1);
		fill_misc_cols(cl, cl->num_cols, &iter, nd);
	}

	cl->inhibit_rebuild = 0;

	return 0;
}

GtkWidget *gtk_conf_list_widget(gtk_conf_list_t *cl)
{
	GtkWidget *vbox, *hbox, *bins, *bdel, *bsel;
	int n;
	GType *ty;

	cl->editing = 0;

	vbox = gtk_vbox_new(FALSE, 0);
	cl->t = gtk_tree_view_new();

	/* create the list model */
	ty = malloc(sizeof(GType) * cl->num_cols);
	for(n = 0; n < cl->num_cols; n++)
		ty[n] = G_TYPE_STRING;
	cl->l = gtk_list_store_newv(cl->num_cols, ty);
	free(ty);

	if (cl->lst != NULL) {
		lht_node_t *lst = cl->lst;
		cl->lst = NULL;
		gtk_conf_list_set_list(cl, lst);
	}

	/* add all columns */
	for(n = 0; n < cl->num_cols; n++) {
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cl->t), -1, cl->col_names[n], renderer, "text", n, NULL);
		if (n == cl->col_data) {
			g_object_set(renderer, "editable", TRUE, NULL);
			g_signal_connect(renderer, "edited", G_CALLBACK(cell_edited_cb), cl);
			g_signal_connect(renderer, "editing-started", G_CALLBACK(cell_edit_started_cb), cl);
			g_signal_connect(renderer, "editing-canceled", G_CALLBACK(cell_edit_canceled_cb), cl);
		}
	}

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(cl->t), cl->reorder);

	g_signal_connect(G_OBJECT(cl->l), "row_deleted", G_CALLBACK(row_delete_cb), cl);
	g_signal_connect(G_OBJECT(cl->l), "row_inserted", G_CALLBACK(row_insert_cb), cl);
	g_signal_connect(G_OBJECT(cl->t), "key-release-event", G_CALLBACK(key_release_cb), cl);

	gtk_tree_view_set_model(GTK_TREE_VIEW(cl->t), GTK_TREE_MODEL(cl->l));

	hbox = gtk_hbox_new(FALSE, 0);
	bins = gtk_button_new_with_label("insert new");
	bdel = gtk_button_new_with_label("remove");

	gtk_box_pack_start(GTK_BOX(hbox), bins, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), bdel, FALSE, FALSE, 2);

	g_signal_connect(G_OBJECT(bins), "clicked", G_CALLBACK(button_ins_cb), cl);
	g_signal_connect(G_OBJECT(bdel), "clicked", G_CALLBACK(button_del_cb), cl);

	if (cl->file_chooser_title != NULL) {
		bsel = gtk_button_new_with_label("change path");
		gtk_box_pack_start(GTK_BOX(hbox), bsel, FALSE, FALSE, 2);
		g_signal_connect(G_OBJECT(bsel), "clicked", G_CALLBACK(button_sel_cb), cl);
	}

	gtk_box_pack_start(GTK_BOX(vbox), cl->t, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	return vbox;
}
