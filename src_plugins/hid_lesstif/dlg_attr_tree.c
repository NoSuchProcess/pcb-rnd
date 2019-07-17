#include "brave.h"
#include "xm_tree_table_widget.h"
#include "hid_dad_tree.h"

typedef struct {
	lesstif_attr_dlg_t *ctx;
	pcb_hid_attribute_t *attr;
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

static void ltf_tt_insert_row(ltf_tree_t *lt, pcb_hid_row_t *new_row)
{
	tt_entry_t *e, *at;
	pcb_hid_row_t *prev, *next, *parent;
	gdl_list_t *parlist;
	int n;

	e = tt_entry_alloc(new_row->cols);

	parent = pcb_dad_tree_parent_row(lt->ht, new_row);
	if (parent != NULL) {
		parlist = &parent->children;
		at = parent->hid_data;
		e->level = at->level+1;
	}
	else {
		parlist = &lt->ht->rows;
		e->level = 1; /* the tree-table widget doesn't like level 0 */
	}

	prev = gdl_prev(parlist, new_row);
	next = gdl_next(parlist, new_row);

	/* insert in the model at the right place (in the flat sense) */
	if (next != NULL) {
		/* there is a known node after the new one, insert before that */
		gdl_insert_before(&lt->model, next->hid_data, e, gdl_linkfield);
	}
	else if (prev != NULL) {
		tt_entry_t *pe, *ne;

		/* there is a known node before the new one; seek the last children
		   in its subtree */
		pe = prev->hid_data;
		for(ne = gdl_next(&lt->model, pe); (ne != NULL) && (ne->level > pe->level); ne = gdl_next(&lt->model, ne)) ;

		if (ne != NULL)
			gdl_insert_before(&lt->model, ne, e, gdl_linkfield);
		else
			gdl_append(&lt->model, e, gdl_linkfield);
	}
	else if (parent != NULL) {
		/* no siblings, first node under the parent, insert right after the parent */
		gdl_insert_after(&lt->model, parent->hid_data, e, gdl_linkfield);
	}
	else {
		/* no siblings, no parent: first node in the tree */
		gdl_append(&lt->model, e, gdl_linkfield);
	}


	new_row->hid_data = e;
	e->user_data = new_row;
	e->flags.is_branch = gdl_length(&new_row->children) > 0;
	for(n = 0; n < new_row->cols; n++)
		tt_get_cell(e, n)[0] = new_row->cell[n];

	/* update parent's branch flag */
	if (parent != NULL) {
		e = parent->hid_data;
		e->flags.is_branch = 1;
	}
}

extern void xm_extent_prediction(XmTreeTableWidget w);

static void ltf_tree_insert_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *new_row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	ltf_tt_insert_row(lt, new_row);
	xm_extent_prediction((XmTreeTableWidget)lt->w);
	REDRAW();
}

static void ltf_tree_modify_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int col)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;

	/* the caller modifies data strings directly, no need to do anything just flush */
	REDRAW();
	
}

static void cursor_changed(ltf_tree_t *lt)
{
	pcb_hid_tree_t *ht = lt->ht;
	pcb_hid_row_t *c_row = NULL;

	if (lt->cursor != NULL)
		c_row = lt->cursor->user_data;

	valchg(lt->w, lt->w, lt->w);
	if (ht->user_selected_cb != NULL)
		ht->user_selected_cb(lt->attr, lt->ctx, c_row);
}

static void ltf_tree_remove_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	tt_entry_t *e = row->hid_data;
	int changed = 0;

	if (lt->cursor == e) {
		lt->cursor = NULL;
		changed = 1;
	}

/*	gdl_remove(&lt->model, e, gdl_linkfield);*/
	delete_tt_entry(&lt->model, e);
	xm_extent_prediction((XmTreeTableWidget)lt->w);
	REDRAW();

	if (changed)
		cursor_changed(lt);
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

static void ltf_tt_jumpto(ltf_tree_t *lt, tt_entry_t *e, int inhibit_cb)
{
	int changed = 0;

	if (lt->cursor != NULL)
		lt->cursor->flags.is_selected = 0;
	changed = (lt->cursor != e);
	lt->cursor = e;
	lt->cursor->flags.is_selected = 1;
	if (e != NULL)
		xm_tree_table_focus_row(lt->w, e->row_index);
	REDRAW();

	if ((changed) && (!inhibit_cb))
		cursor_changed(lt);
}

static void ltf_tt_jumprel(ltf_tree_t *lt, int dir)
{
	tt_entry_t *e;
	int changed = 0;

	if (lt->cursor != NULL)
		lt->cursor->flags.is_selected = 0;

	e = lt->cursor;
	if (e == NULL) { /* pick first if there is no cursor */
		changed = 1;
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

	if (e != NULL) {
		if (lt->cursor != e)
			changed = 1;
		lt->cursor = e;
	}
	if (lt->cursor != NULL) {
		lt->cursor->flags.is_selected = 1;
		xm_tree_table_focus_row(lt->w, lt->cursor->row_index);
	}
	REDRAW();
	if (changed)
		cursor_changed(lt);
}

static void ltf_tree_jumpto_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	if (row != NULL) {
		tt_entry_t *e = row->hid_data;
		ltf_tt_jumpto(lt, e, 1);
	}
	else {
		TODO("remove cursor");
	}
}

static void ltf_hide_rows(ltf_tree_t *lt, tt_entry_t *root, int val, int user, int parent, int children)
{
	if (parent) {
		if (user)
			root->flags.is_uhidden = val;
		else
			root->flags.is_thidden = val;
	}
	if (children) {
		tt_entry_t *e;
		for(e = gdl_next(&lt->model, root); e != NULL; e = gdl_next(&lt->model, e)) {
			if (e->level <= root->level)
				break;
			if (user)
				e->flags.is_uhidden = val;
			else
				e->flags.is_thidden = val;
		}
	}
}

static void ltf_tree_expcoll(ltf_tree_t *lt, tt_entry_t *e, int expanded)
{
	ltf_hide_rows(lt, e, !expanded, 0, 0, 1);
	e->flags.is_unfolded = expanded;
}

static void ltf_tree_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{
	pcb_hid_attribute_t *attr = &ctx->attrs[idx];
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attr->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	pcb_hid_row_t *r, *row;
	tt_entry_t *e;

	if (val == NULL) {
		if (lt != NULL) {
			/* remove the cursor */
			if (lt->cursor != NULL)
				lt->cursor->flags.is_selected = 0;
			REDRAW();
		}
		return;
	}

	row = htsp_get(&lt->ht->paths, val);
	if (row == NULL)
		return;

	e = row->hid_data;
	e->flags.is_uhidden = 0;
	e->flags.is_thidden = 0;

	/* make sure the path is visible: expand all parent, but not all
	   children of those parents */
	for(r = pcb_dad_tree_parent_row(lt->ht, row); r != NULL; r = pcb_dad_tree_parent_row(lt->ht, r)) {
		e = r->hid_data;
		e->flags.is_thidden = 0;
		e->flags.is_uhidden = 0;
		e->flags.is_unfolded = 1;
	}

	ltf_tt_jumpto(lt, row->hid_data, 1); /* implies a REDRAW() */
}

static void ltf_tree_expcoll_cb(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int expanded)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	ltf_tree_expcoll(lt, row->hid_data, expanded);
	REDRAW();
}

static void ltf_tree_update_hide_cb(pcb_hid_attribute_t *attrib, void *hid_wdata)
{
	pcb_hid_tree_t *ht = (pcb_hid_tree_t *)attrib->enumerations;
	ltf_tree_t *lt = ht->hid_wdata;
	tt_entry_t *e;
	for(e = gdl_first(&lt->model); e != NULL; e = gdl_next(&lt->model, e)) {
		pcb_hid_row_t *row = e->user_data;
		e->flags.is_uhidden = row->hide;
	}
	REDRAW();
}

static void ltf_tt_append_row(ltf_tree_t *lt, pcb_hid_row_t *new_row, int level)
{
	tt_entry_t *e;
	int n;

	e = tt_entry_alloc(new_row->cols);
	new_row->hid_data = e;
	e->user_data = new_row;
	e->flags.is_branch = gdl_length(&new_row->children) > 0;
	e->level = level;
	for(n = 0; n < new_row->cols; n++)
		tt_get_cell(e, n)[0] = new_row->cell[n];

	gdl_append(&lt->model, e, gdl_linkfield);
}


static void ltf_tt_import(ltf_tree_t *lt, gdl_list_t *lst, int level)
{
	pcb_hid_row_t *r;

	for(r = gdl_first(lst); r != NULL; r = gdl_next(lst, r)) {
		ltf_tt_append_row(lt, r, level);
		ltf_tt_import(lt, &r->children, level+1);
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
			if (e == lt->cursor) { /* second click */
				ltf_tree_expcoll(lt, lt->cursor, !lt->cursor->flags.is_unfolded);
				REDRAW();
			}
			else
				ltf_tt_jumpto(lt, e, 0);
			break;
		case ett_key:
			XLookupString(&(data->event->xkey), text, sizeof(text), &key, 0);
			switch(key) {
				case XK_Up: ltf_tt_jumprel(lt, -1); break;
				case XK_Down: ltf_tt_jumprel(lt, +1); break;
				case XK_Return:
				case XK_KP_Enter:
					if (lt->cursor != NULL) {
						ltf_tree_expcoll(lt, lt->cursor, !lt->cursor->flags.is_unfolded);
						REDRAW();
					}
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
	lt->attr = attr;

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

	ltf_tt_import(lt, &ht->rows, 1);

	XtManageChild(table);
	return table;
}

static Widget ltf_tree_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget w;

	if (pcb_brave & PCB_BRAVE_LESSTIF_TREETABLE)
		return ltf_tree_create_(ctx, parent, attr);

	stdarg_n = 0;
	stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
	stdarg(XmNlabelString, XmStringCreatePCB("TODO: tree table"));
	w = XmCreateLabel(parent, XmStrCast("TODO"), stdarg_args, stdarg_n);

	return w;
}
