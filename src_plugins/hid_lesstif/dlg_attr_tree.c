#include "xm_tree_table_widget.h"
typedef struct {
	gdl_list_t model;
	Widget *w;
} ltf_tree_t;

static void ltf_ttbl_xevent_cb(const tt_table_event_data_t *data)
{
	pcb_trace("ttbl x event\n");
}


static void ltf_tree_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{

}


static void ltf_tt_insert_row(ltf_tree_t *lt, pcb_hid_row_t *new_row)
{
	tt_entry_t *e;
	int n;

	e = tt_entry_alloc(new_row->cols);
	gdl_append(&lt->model, e, gdl_linkfield);

	new_row->user_data = e;
	for(n = 0; n < new_row->cols; n++)
		tt_get_cell(e, n)[0] = new_row->cell[n];
}

static void ltf_tree_insert_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *new_row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	ltf_tt_insert_row(lt, new_row);
}

void ltf_tree_modify_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int col)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

void ltf_tree_remove_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

void ltf_tree_free_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

pcb_hid_row_t *ltf_tree_get_selected_cb(pcb_hid_attribute_t *attrib, void *hid_wdata)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

	return NULL;
}

void ltf_tree_jumpto_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

void ltf_tree_expcoll_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int expanded)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

void ltf_tree_update_hide_cb(pcb_hid_attribute_t *attrib, void *hid_wdata)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

static void ltf_tt_import(ltf_tree_t *lt, gdl_list_t *lst)
{
	pcb_hid_row_t *r;

	for(r = gdl_first(lst); r != NULL; r = gdl_next(lst, r)) {
		ltf_tt_insert_row(lt, r);
		ltf_tt_import(lt, &r->children);
	}
}

static Widget ltf_tree_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attr->enumerations;
	ltf_tree_t *lt = calloc(sizeof(ltf_tree_t), 1);
	Widget table = xm_create_tree_table_widget_cb(parent, &lt->model, ltf_ttbl_xevent_cb, NULL, NULL);

	lt->w = table;
	ht->hid_wdata = lt;

	ht->hid_insert_cb = ltf_tree_insert_cb;
	ht->hid_modify_cb = ltf_tree_modify_cb;
	ht->hid_remove_cb = ltf_tree_remove_cb;
	ht->hid_free_cb = ltf_tree_free_cb;
	ht->hid_get_selected_cb = ltf_tree_get_selected_cb;
	ht->hid_jumpto_cb = ltf_tree_jumpto_cb;
	ht->hid_expcoll_cb = ltf_tree_expcoll_cb;
	ht->hid_update_hide_cb = ltf_tree_update_hide_cb;

	xm_tree_table_tree_mode(lt->w, !!(attr->pcb_hatt_flags & PCB_HATF_TREE_COL));

	if (ht->hdr != NULL) {
		int n;
		const char **s;
		for(n = 0, s = ht->hdr; *s != NULL; s++, n++) ;
		xm_attach_tree_table_header(lt->w, n, ht->hdr);
	}

	ltf_tt_import(lt, &ht->rows);

	XtManageChild(table);
	return table;
}

