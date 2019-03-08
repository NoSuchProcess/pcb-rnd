#include "brave.h"
#include "xm_tree_table_widget.h"
#include "hid_dad_tree.h"

typedef struct {
	lesstif_attr_dlg_t *ctx;
	gdl_list_t model;
	Widget w;
	pcb_hid_tree_t *ht;
	tt_entry_t *cursor;
} ltf_tree_t;

#define REDRAW() xm_draw_tree_table_widget(lt->w);

static int is_hidden(tt_entry_t *et)
{
	return et->flags.is_thidden || et->flags.is_uhidden;
}

static tt_entry_t *ltf_tt_lookup_row(const tt_table_event_data_t *data, unsigned row_index)
{
	tt_entry_t *e;
	for(e = gdl_first(data->root_entry); e != NULL; e = gdl_next(data->root_entry, e)) {
		if (e->row_index == row_index)
			return e;
	}
	return NULL;
}



static void ltf_tree_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{

}


static void ltf_tt_insert_row(ltf_tree_t *lt, pcb_hid_row_t *new_row)
{
	tt_entry_t *e, *before, *after = NULL;
	pcb_hid_row_t *rafter, *lvl = NULL;
	int n, base;

	e = tt_entry_alloc(new_row->cols);
	e->level = 0;

	/* insert after an existing sibling or if this is the first node under a
	   new level, directly under the parent */
	rafter = pcb_dad_tree_prev_row(lt->ht, new_row);
	if (rafter == NULL) {
		rafter = pcb_dad_tree_parent_row(lt->ht, new_row);
		e->level = 1;
	}

	if (rafter != NULL) {
		for(lvl = rafter; lvl != NULL; lvl = pcb_dad_tree_parent_row(lt->ht, lvl))
			e->level++;

		/* find the first node that is at least on the same level as rafter;
		   this effectively skips all children of rafter */
		before = rafter->hid_data;
		base = before->level;
		for(;(before != NULL) && (before->level > base); before = gdl_next(&lt->model, before)) ;
		if (before != NULL)
			gdl_insert_after(&lt->model, after, e, gdl_linkfield);
		else
			gdl_append(&lt->model, e, gdl_linkfield);
	}
	else
		gdl_append(&lt->model, e, gdl_linkfield);

	new_row->hid_data = e;
	e->user_data = new_row;
	for(n = 0; n < new_row->cols; n++)
		tt_get_cell(e, n)[0] = new_row->cell[n];
}

static void ltf_tree_insert_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *new_row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	ltf_tt_insert_row(lt, new_row);
}

static void ltf_tree_modify_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int col)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	/* the caller modifies data strings directly, no need to do anything just flush */
	REDRAW();
	
}

static void ltf_tree_remove_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	tt_entry_t *e = row->hid_data;

	if (lt->cursor == e)
		lt->cursor = NULL;

/*	gdl_remove(&lt->model, e, gdl_linkfield);*/
	delete_tt_entry(&lt->model, e);
}

static void ltf_tree_free_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	tt_entry_t *i;

	if (lt == NULL)
		return;

	for(i = gdl_first(&lt->model); i != NULL; i = gdl_first(&lt->model))
		delete_tt_entry(&lt->model, i);

	free(lt);
	ht->hid_wdata = NULL;
}

static pcb_hid_row_t *ltf_tree_get_selected_cb(pcb_hid_attribute_t *attrib, void *hid_wdata)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	if (lt->cursor == NULL)
		return NULL;
	return lt->cursor->user_data;
}

static void ltf_tt_jumpto(ltf_tree_t *lt, tt_entry_t *e)
{
	if (lt->cursor != NULL)
		lt->cursor->flags.is_selected = 0;
	lt->cursor = e;
	lt->cursor->flags.is_selected = 1;
	xm_tree_table_focus_row(lt->w, e->row_index);
	REDRAW();
}

static void ltf_tt_jumprel(ltf_tree_t *lt, int dir)
{
	tt_entry_t *e;
	if (lt->cursor != NULL)
		lt->cursor->flags.is_selected = 0;

	e = lt->cursor;
	if (e == NULL) { /* pick first if there is no cursor */
		lt->cursor = e = gdl_first(&lt->model);
		if (is_hidden(e)) /* if first is hidden, step downward until the first visible */
			dir = 1;
	}

	for(;;) {
		if (e == NULL)
			break;
		e = (dir > 0 ? gdl_next(&lt->model, e) : gdl_prev(&lt->model, e));
		if ((e == NULL) || (!is_hidden(e)))
			break;
	}

	if (e != NULL)
		lt->cursor = e;
	if (lt->cursor != NULL) {
		lt->cursor->flags.is_selected = 1;
		xm_tree_table_focus_row(lt->w, lt->cursor->row_index);
	}
	REDRAW();
}

static void ltf_tree_jumpto_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	tt_entry_t *e = row->hid_data;
	ltf_tt_jumpto(lt, e);
}

static void ltf_tree_expcoll_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int expanded)
{
/*	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;*/

}

static void ltf_tree_update_hide_cb(pcb_hid_attribute_t *attrib, void *hid_wdata)
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

static void ltf_tt_xevent_cb(const tt_table_event_data_t *data)
{
	tt_entry_t *e;
	ltf_tree_t *lt = data->user_data;
	char text[64];
	KeySym key;

	switch(data->type) {
		case ett_none:
		case ett_mouse_btn_up:
		case ett_mouse_btn_drag:
			break;
		case ett_mouse_btn_down:
			XtSetKeyboardFocus(lt->ctx->dialog, lt->w);
			e = ltf_tt_lookup_row(data, data->current_row);
			if (e == NULL)
				return;
			ltf_tt_jumpto(lt, e);
			break;
		case ett_key:
			XLookupString(&(data->event->xkey), text, sizeof(text), &key, 0);
			switch(key) {
				case XK_Up: ltf_tt_jumprel(lt, -1); break;
				case XK_Down: ltf_tt_jumprel(lt, +1); break;
				case XK_Return:
				case XK_KP_Enter:
					pcb_trace("tree key {enter}\n");
					break;
				default: pcb_trace("tree key %s\n", text);
			}
			break;
	}
}

static Widget ltf_tree_create_(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attr->enumerations;
	ltf_tree_t *lt = calloc(sizeof(ltf_tree_t), 1);
	Widget table = xm_create_tree_table_widget_cb(parent, &lt->model, lt, ltf_tt_xevent_cb, NULL, NULL);

	lt->w = table;
	lt->ht = ht;
	ht->hid_wdata = lt;
	lt->ctx = ctx;

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

static Widget ltf_tree_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget *w;

	if (pcb_brave & PCB_BRAVE_LESSTIF_TREETABLE)
		return ltf_tree_create_(ctx, parent, attr);

	stdarg_n = 0;
	stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
	stdarg(XmNlabelString, XmStringCreatePCB("TODO: tree table"));
	w = XmCreateLabel(parent, XmStrCast("TODO"), stdarg_args, stdarg_n);

	return w;
}
