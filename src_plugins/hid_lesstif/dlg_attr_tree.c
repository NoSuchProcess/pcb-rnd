#include "xm_tree_table_widget.h"
typedef struct {
	gdl_list_t model;
} ltf_tree_t;

static void ltf_ttbl_xevent_cb(const tt_table_event_data_t *data)
{
	pcb_trace("ttbl x event\n");
}


static void ltf_tree_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{

}

static Widget ltf_tree_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
#if 0
	ltf_tree_t *lt = calloc(sizeof(ltf_tree_t), 1);
	Widget table = xm_create_tree_table_widget_cb(parent, &lt->model, ltf_ttbl_xevent_cb, NULL, NULL);

	XtManageChild(table);
	return table;
#else
	Widget w;
	stdarg_n = 0;
	stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
	stdarg(XmNlabelString, XmStringCreatePCB("todo: tree table"));
	w = XmCreateLabel(parent, XmStrCast("todo"), stdarg_args, stdarg_n);

	return w;
#endif
}

