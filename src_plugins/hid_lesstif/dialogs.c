#include "xincludes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <Xm/Notebook.h>

#include "compat_misc.h"
#include "data.h"
#include "build_run.h"
#include "crosshair.h"
#include "layer.h"
#include "pcb-printf.h"

#include "hid.h"
#include "lesstif.h"
#include "hid_attrib.h"
#include "actions.h"
#include "hid_init.h"
#include "stdarg.h"
#include "misc_util.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "search.h"
#include "change.h"

static int ok;

#define COMPONENT_SIDE_NAME "(top)"
#define SOLDER_SIDE_NAME "(bottom)"

/* ------------------------------------------------------------ */

static void dialog_callback_ok_value(Widget w, void *v, void *cbs)
{
	ok = (int) (size_t) v;
}

static int wait_for_dialog(Widget w)
{
	ok = -1;
	XtManageChild(w);
	while (ok == -1 && XtIsManaged(w)) {
		XEvent e;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}
	XtUnmanageChild(w);
	return ok;
}

/* ------------------------------------------------------------ */

static Widget fsb = 0;
static XmString xms_pcb, xms_net, xms_vend, xms_all, xms_load, xms_loadv, xms_save, xms_fp;

static void setup_fsb_dialog()
{
	if (fsb)
		return;

	xms_pcb = XmStringCreatePCB("*.pcb");
	xms_fp = XmStringCreatePCB("*.fp");
	xms_net = XmStringCreatePCB("*.net");
	xms_vend = XmStringCreatePCB("*.vend");
	xms_all = XmStringCreatePCB("*");
	xms_load = XmStringCreatePCB("Load From");
	xms_loadv = XmStringCreatePCB("Load Vendor");
	xms_save = XmStringCreatePCB("Save As");

	stdarg_n = 0;
	fsb = XmCreateFileSelectionDialog(mainwind, XmStrCast("file"), stdarg_args, stdarg_n);

	XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 1);
	XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 0);
}

static const char pcb_acts_Load[] = "Load()\n" "Load(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert)";
static const char pcb_acth_Load[] = "Load layout data from a user-selected file.";
/* DOC: load.html */
extern fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv);
static fgw_error_t pcb_act_Load(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function = "Layout";
	char *name;
	XmString xmname, pattern;

	if (argc > 2)
		return PCB_ACT_CALL_C(pcb_act_LoadFrom, res, argc, argv);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Load, function = argv[1].val.str);

	setup_fsb_dialog();

	if (pcb_strcasecmp(function, "Netlist") == 0)
		pattern = xms_net;
	else if (pcb_strcasecmp(function, "ElementToBuffer") == 0)
		pattern = xms_fp;
	else
		pattern = xms_pcb;

	stdarg_n = 0;
	stdarg(XmNtitle, "Load From");
	XtSetValues(XtParent(fsb), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNpattern, pattern);
	stdarg(XmNmustMatch, True);
	stdarg(XmNselectionLabelString, xms_load);
	XtSetValues(fsb, stdarg_args, stdarg_n);

	if (!wait_for_dialog(fsb)) {
		PCB_ACT_IRES(1);
		return 0;
	}

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	pcb_actionl("LoadFrom", function, name, NULL);

	XtFree(name);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Save[] = "Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";
static const char pcb_acth_Save[] = "Save layout data to a user-selected file.";
/* DOC: save.html */
extern fgw_error_t pcb_act_SaveTo(fgw_arg_t *res, int argc, fgw_arg_t *argv);
static fgw_error_t pcb_act_Save(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function = "Layout";
	char *name;
	XmString xmname, pattern;

	if (argc > 2)
		return PCB_ACT_CALL_C(pcb_act_SaveTo, res, argc, argv);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Save, function = argv[1].val.str);

	if (pcb_strcasecmp(function, "Layout") == 0)
		if (PCB->Filename)
			return pcb_actionl("SaveTo", "Layout", NULL);

	setup_fsb_dialog();

	pattern = xms_pcb;

	XtManageChild(fsb);

	stdarg_n = 0;
	stdarg(XmNtitle, "Save As");
	XtSetValues(XtParent(fsb), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNpattern, pattern);
	stdarg(XmNmustMatch, False);
	stdarg(XmNselectionLabelString, xms_save);
	XtSetValues(fsb, stdarg_args, stdarg_n);

	if (!wait_for_dialog(fsb)) {
		PCB_ACT_IRES(1);
		return 0;
	}

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0)
		pcb_actionl("PasteBuffer", "Save", name, NULL);
	else {
		/*
		 * if we got this far and the function is Layout, then
		 * we really needed it to be a LayoutAs.  Otherwise
		 * ActionSaveTo() will ignore the new file name we
		 * just obtained.
		 */
		if (pcb_strcasecmp(function, "Layout") == 0)
			pcb_actionl("SaveTo", "LayoutAs", name, NULL);
		else
			pcb_actionl("SaveTo", function, name, NULL);
	}
	XtFree(name);

	PCB_ACT_IRES(0);
	return 0;
}

/* ------------------------------------------------------------ */

static Widget log_form, log_text;
static int log_size = 0;
static int pending_newline = 0;

static void log_clear(Widget w, void *up, void *cbp)
{
	XmTextSetString(log_text, XmStrCast(""));
	log_size = 0;
	pending_newline = 0;
}

static void log_dismiss(Widget w, void *up, void *cbp)
{
	XtUnmanageChild(log_form);
}

void lesstif_logv(enum pcb_message_level level, const char *fmt, va_list ap)
{
	char *buf, *scan;
	if (!mainwind) {
		vprintf(fmt, ap);
		return;
	}
	if (!log_form) {
		Widget clear_button, dismiss_button;

		stdarg_n = 0;
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNwidth, 600);
		stdarg(XmNheight, 200);
		stdarg(XmNtitle, "pcb-rnd Log");
		log_form = XmCreateFormDialog(mainwind, XmStrCast("log"), stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		clear_button = XmCreatePushButton(log_form, XmStrCast("clear"), stdarg_args, stdarg_n);
		XtManageChild(clear_button);
		XtAddCallback(clear_button, XmNactivateCallback, (XtCallbackProc) log_clear, 0);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_WIDGET);
		stdarg(XmNrightWidget, clear_button);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		dismiss_button = XmCreatePushButton(log_form, XmStrCast("dismiss"), stdarg_args, stdarg_n);
		XtManageChild(dismiss_button);
		XtAddCallback(dismiss_button, XmNactivateCallback, (XtCallbackProc) log_dismiss, 0);

		stdarg_n = 0;
		stdarg(XmNeditable, False);
		stdarg(XmNeditMode, XmMULTI_LINE_EDIT);
		stdarg(XmNcursorPositionVisible, True);
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, clear_button);
		log_text = XmCreateScrolledText(log_form, XmStrCast("text"), stdarg_args, stdarg_n);
		XtManageChild(log_text);

		XtManageChild(log_form);
	}
	if (pending_newline) {
		XmTextInsert(log_text, log_size++, XmStrCast("\n"));
		pending_newline = 0;
	}
	buf = pcb_strdup_vprintf(fmt, ap);
	scan = &buf[strlen(buf) - 1];
	while (scan >= buf && *scan == '\n') {
		pending_newline++;
		*scan-- = 0;
	}
	switch(level) {
		case PCB_MSG_ERROR:   XmTextInsert(log_text, log_size, "Err: "); break;
		case PCB_MSG_WARNING: XmTextInsert(log_text, log_size, "Wrn: "); break;
		case PCB_MSG_INFO:    XmTextInsert(log_text, log_size, "Inf: "); break;
		case PCB_MSG_DEBUG:   XmTextInsert(log_text, log_size, "Dbg: "); break;
	}
	log_size += 5;
	XmTextInsert(log_text, log_size, buf);
	log_size += strlen(buf);

	scan = strrchr(buf, '\n');
	if (scan)
		scan++;
	else
		scan = buf;
	XmTextSetCursorPosition(log_text, log_size - strlen(scan));
	free(buf);
}

void lesstif_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	lesstif_logv(PCB_MSG_INFO, fmt, ap);
	va_end(ap);
}

/* ------------------------------------------------------------ */
/* FIXME -- make this a proper file select dialog box */
char *lesstif_fileselect(const char *title, const char *descr,
												 const char *default_file, const char *default_ext, const char *history_tag, int flags)
{

	return pcb_hid_prompt_for(title, default_file, "fileselect");
}

/* ------------------------------------------------------------ */

static Widget pcb_motif_box(Widget parent, char *name, char type, int num_table_rows, int want_frame, int want_scroll)
{
	Widget cnt;

	if (want_frame) {
		/* create and insert frame around the content table */
		stdarg(XmNalignment, XmALIGNMENT_END);
		parent = XmCreateFrame(parent, XmStrCast("box-frame"), stdarg_args, stdarg_n);
		XtManageChild(parent);
		stdarg_n = 0;
	}

	if (want_scroll) {
		stdarg(XmNscrollingPolicy, XmAUTOMATIC);
		stdarg(XmNvisualPolicy, XmVARIABLE);

		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);

		parent = XmCreateScrolledWindow(parent, "scrolled_box", stdarg_args, stdarg_n);
		XtManageChild(parent);
		stdarg_n = 0;
	}

	switch(type) {
		case 'h': /* "hbox" */
			stdarg(XmNorientation, XmHORIZONTAL);
			stdarg(XmNpacking, XmPACK_TIGHT);
			break;
		case 'v': /* "vbox" */
			stdarg(XmNorientation, XmVERTICAL);
			stdarg(XmNpacking, XmPACK_TIGHT);
			break;
		case 't': /* "table" */
			stdarg(XmNorientation, XmHORIZONTAL);
			stdarg(XmNpacking, XmPACK_COLUMN);
			stdarg(XmNnumColumns, num_table_rows);
			stdarg(XmNisAligned, True);
			stdarg(XmNentryAlignment, XmALIGNMENT_END);
			break;
		default:
			abort();
	}
	cnt = XmCreateRowColumn(parent, name, stdarg_args, stdarg_n);
	return cnt;
}

typedef struct {
	pcb_hid_attribute_t *attrs;
	int n_attrs, actual_nattrs;
	Widget *wl;
	Widget **btn; /* enum value buttons */
	pcb_hid_attr_val_t *results;
	void *caller_data;
	Widget dialog;
	pcb_hid_attr_val_t property[PCB_HATP_max];
	Dimension minw, minh;
	void (*close_cb)(void *caller_data, pcb_hid_attr_ev_t ev);
	unsigned close_cb_called:1;
	unsigned already_closing:1;
	unsigned inhibit_valchg:1;
} lesstif_attr_dlg_t;

static void attribute_dialog_readres(lesstif_attr_dlg_t *ctx, int widx)
{
	char *cp;

	if (ctx->attrs[widx].help_text == ATTR_UNDOCUMENTED)
		return;

	switch(ctx->attrs[widx].type) {
		case PCB_HATT_BOOL:
			ctx->attrs[widx].default_val.int_value = ctx->results[widx].int_value = XmToggleButtonGetState(ctx->wl[widx]);
			break;
		case PCB_HATT_STRING:
			ctx->results[widx].str_value = pcb_strdup(XmTextGetString(ctx->wl[widx]));
			break;
		case PCB_HATT_INTEGER:
			cp = XmTextGetString(ctx->wl[widx]);
			sscanf(cp, "%d", &ctx->results[widx].int_value);
			break;
		case PCB_HATT_COORD:
			cp = XmTextGetString(ctx->wl[widx]);
			ctx->results[widx].coord_value = pcb_get_value(cp, NULL, NULL, NULL);
			break;
		case PCB_HATT_REAL:
			cp = XmTextGetString(ctx->wl[widx]);
			sscanf(cp, "%lg", &ctx->results[widx].real_value);
			break;
		case PCB_HATT_ENUM:
			{
				const char **uptr;
				Widget btn;

				stdarg_n = 0;
				stdarg(XmNmenuHistory, &btn);
				XtGetValues(ctx->wl[widx], stdarg_args, stdarg_n);
				stdarg_n = 0;
				stdarg(XmNuserData, &uptr);
				XtGetValues(btn, stdarg_args, stdarg_n);
				ctx->results[widx].int_value = uptr - ctx->attrs[widx].enumerations;
			}
			break;
		default:
			break;
	}
	ctx->attrs[widx].default_val = ctx->results[widx];
}

static void valchg(Widget w, XtPointer dlg_widget_, XtPointer call_data)
{
	lesstif_attr_dlg_t *ctx;
	Widget dlg_widget = (Widget)dlg_widget_; /* ctx->wl[i] */
	int widx;

	if (dlg_widget == NULL)
		return;

	XtVaGetValues(dlg_widget, XmNuserData, &ctx, NULL);

	if (ctx == NULL)
		return;

	if (ctx->inhibit_valchg)
		return;

	for(widx = 0; widx < ctx->n_attrs; widx++)
		if (ctx->wl[widx] == dlg_widget)
			break;

	if (widx >= ctx->n_attrs)
		return;

	ctx->attrs[widx].changed = 1;

	if ((ctx->attrs[widx].change_cb == NULL) && (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func == NULL))
		return;

	attribute_dialog_readres(ctx, widx);
	if (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func != NULL)
		ctx->property[PCB_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, &ctx->attrs[widx]);
	if (ctx->attrs[widx].change_cb != NULL)
		ctx->attrs[widx].change_cb(ctx, ctx->caller_data, &ctx->attrs[widx]);
}

static void pagechg(Widget w, XtPointer client_data, XtPointer call_data)
{
	lesstif_attr_dlg_t *ctx;
	pcb_hid_attribute_t *attr = (pcb_hid_attribute_t *)client_data;
	XmNotebookCallbackStruct *nptr = (XmNotebookCallbackStruct *)call_data;
	int widx;

	XtVaGetValues(w, XmNuserData, &ctx, NULL);

	if (ctx == NULL)
		return;

	if (ctx->inhibit_valchg)
		return;

	widx = attr - ctx->attrs;
	if ((widx < 0) || (widx >= ctx->n_attrs))
		return;

	ctx->results[widx].int_value = nptr->page_number - 1;

	attribute_dialog_readres(ctx, widx);
	if (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func != NULL)
		ctx->property[PCB_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, &ctx->attrs[widx]);
	if (attr->change_cb != NULL)
		attr->change_cb(ctx, ctx->caller_data, attr);
}

/* parent info for tabbed */
typedef struct {
	Widget notebook;
	const char **tablab;
	Dimension minw;
	int tabs;
} attr_dlg_tb_t;

static int attribute_dialog_add(lesstif_attr_dlg_t *ctx, Widget real_parent, attr_dlg_tb_t *tb, int start_from, int add_labels);

#include "dlg_attr_misc.c"
#include "dlg_attr_box.c"

/* returns the index of HATT_END where the loop had to stop */
static int attribute_dialog_add(lesstif_attr_dlg_t *ctx, Widget real_parent, attr_dlg_tb_t *tb, int start_from, int add_labels)
{
	int len, i, numch, numcol;
	Widget parent, labbox;
	static XmString empty = 0;

	if (!empty)
		empty = XmStringCreatePCB(" ");

	for (i = start_from; i < ctx->n_attrs; i++) {
		char buf[30];
		Widget w;

		if (ctx->attrs[i].type == PCB_HATT_END)
			break;

		if (ctx->attrs[i].help_text == ATTR_UNDOCUMENTED)
			continue;

		/* if we are in a notebook, a page needs to be created first */
		if (tb != NULL) {
			const char *lab;
			Widget tab;
			Dimension wi;

			parent = XmCreateRowColumn(tb->notebook, "page", NULL, 0);

			stdarg_n = 0;
			stdarg(XmNnotebookChildType, XmMAJOR_TAB);
			if (*tb->tablab != NULL) {
				lab = *tb->tablab;
				tb->tablab++;
			}
			else
				lab = "<anon>";

			tab = XmCreatePushButton(tb->notebook, (char *)lab, stdarg_args, stdarg_n);
			XtManageChild(tab);

			/* update minimum width for tabs */
			stdarg_n = 0;
			stdarg(XmNwidth, &wi);
			XtGetValues(tab, stdarg_args, stdarg_n);
			if (wi > tb->minw)
				tb->minw = wi;
			tb->tabs++;

			XtManageChild(parent);
		}
		else
			parent = real_parent;

		/* Add label */
		if ((add_labels) && (ctx->attrs[i].type != PCB_HATT_LABEL)) {
			stdarg_n = 0;
			stdarg(XmNalignment, XmALIGNMENT_END);

			labbox = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 'h', 0, 0, 0);
			XtManageChild(labbox);
			parent = labbox;

			w = XmCreateLabel(labbox, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtManageChild(w);
		}

		/* Add content */
		stdarg_n = 0;
		stdarg(XmNalignment, XmALIGNMENT_END);
		stdarg(XmNuserData, ctx);

		switch(ctx->attrs[i].type) {
		case PCB_HATT_BEGIN_HBOX:
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 'h', 0, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_FRAME), (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_SCROLL));
			XtManageChild(w);
			ctx->wl[i] = w;
			i = attribute_dialog_add(ctx, w, NULL, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));
			break;

		case PCB_HATT_BEGIN_VBOX:
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 'v', 0, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_FRAME), (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_SCROLL));
			XtManageChild(w);
			ctx->wl[i] = w;
			i = attribute_dialog_add(ctx, w, NULL, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));
			break;

		case PCB_HATT_BEGIN_HPANE:
		case PCB_HATT_BEGIN_VPANE:
			i = ltf_pane_create(ctx, i, parent, (ctx->attrs[i].type == PCB_HATT_BEGIN_HPANE), add_labels);
			break;

		case PCB_HATT_BEGIN_TABLE:
			/* create content table */
			numcol = ctx->attrs[i].pcb_hatt_table_cols;
			len = pcb_hid_attrdlg_num_children(ctx->attrs, i+1, ctx->n_attrs);
			numch = len  / numcol + !!(len % numcol);
			w = pcb_motif_box(parent, XmStrCast(ctx->attrs[i].name), 't', numch, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_FRAME), (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_SCROLL));

			ctx->wl[i] = w;

			i = attribute_dialog_add(ctx, w, NULL, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));
			while((len % numcol) != 0) {
				Widget pad;

				stdarg_n = 0;
				stdarg(XmNalignment, XmALIGNMENT_END);
				pad = XmCreateLabel(w, XmStrCast("."), stdarg_args, stdarg_n);
				XtManageChild(pad);
				len++;
			}
			XtManageChild(w);
			break;

		case PCB_HATT_BEGIN_TABBED:
		{
			Widget scroller;
			attr_dlg_tb_t tb;

			stdarg_n = 0;
			if (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LEFT_TAB) {
				stdarg(XmNbackPagePlacement, XmBOTTOM_LEFT);
				stdarg(XmNorientation, XmHORIZONTAL);
			}
			else {
				stdarg(XmNbackPagePlacement, XmTOP_RIGHT);
				stdarg(XmNorientation, XmVERTICAL);
			}
			stdarg(XmNbackPageNumber, 1);
			stdarg(XmNbackPageSize, 1);
			stdarg(XmNbindingType, XmNONE);


			stdarg(XmNleftAttachment, XmATTACH_FORM);
			stdarg(XmNtopAttachment, XmATTACH_FORM);
			stdarg(XmNrightAttachment, XmATTACH_FORM);
			stdarg(XmNbottomAttachment, XmATTACH_FORM);
			stdarg(XmNuserData, ctx);
			ctx->wl[i] = w = XmCreateNotebook(parent, "notebook", stdarg_args, stdarg_n);

			/* remove the page scroller widget that got automatically created by XmCreateNotebook() */
			scroller = XtNameToWidget(w, "PageScroller");
			XtUnmanageChild (scroller);

			XtAddCallback(w, XmNpageChangedCallback, (XtCallbackProc)pagechg, (XtPointer)&ctx->attrs[i]);

			tb.notebook = w;
			tb.tablab = ctx->attrs[i].enumerations;
			tb.minw = 0;
			tb.tabs = 0;

			i = attribute_dialog_add(ctx, w, &tb, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));

			XtManageChild(w);
			break;
		}

		case PCB_HATT_PREVIEW:
			ctx->wl[i] = ltf_preview_create(ctx, parent, &ctx->attrs[i]);
			break;

		case PCB_HATT_PICTURE:
			ctx->wl[i] = ltf_picture_create(ctx, parent, &ctx->attrs[i]);
			break;

		case PCB_HATT_PICBUTTON:
			ctx->wl[i] = ltf_picbutton_create(ctx, parent, &ctx->attrs[i]);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
			break;
	
		case PCB_HATT_LABEL:
			stdarg_n = 0;
			stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
			ctx->wl[i] = XmCreateLabel(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			break;
		case PCB_HATT_BOOL:
			stdarg(XmNlabelString, empty);
			stdarg(XmNset, ctx->results[i].int_value);
			ctx->wl[i] = XmCreateToggleButton(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case PCB_HATT_STRING:
			stdarg(XmNcolumns, 40);
			stdarg(XmNresizeWidth, True);
			stdarg(XmNvalue, ctx->results[i].str_value);
			ctx->wl[i] = XmCreateTextField(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case PCB_HATT_INTEGER:
			stdarg(XmNcolumns, 13);
			stdarg(XmNresizeWidth, True);
			sprintf(buf, "%d", ctx->results[i].int_value);
			stdarg(XmNvalue, buf);
			ctx->wl[i] = XmCreateTextField(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case PCB_HATT_COORD:
			stdarg(XmNcolumns, 13);
			stdarg(XmNresizeWidth, True);
			pcb_snprintf(buf, sizeof(buf), "%$mS", ctx->results[i].coord_value);
			stdarg(XmNvalue, buf);
			ctx->wl[i] = XmCreateTextField(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case PCB_HATT_REAL:
			stdarg(XmNcolumns, 16);
			stdarg(XmNresizeWidth, True);
			pcb_snprintf(buf, sizeof(buf), "%g", ctx->results[i].real_value);
			stdarg(XmNvalue, buf);
			ctx->wl[i] = XmCreateTextField(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNvalueChangedCallback, valchg, ctx->wl[i]);
			break;
		case PCB_HATT_PROGRESS:
			ctx->wl[i] = ltf_progress_create(ctx, parent);
			break;
		case PCB_HATT_ENUM:
			{
				static XmString empty = 0;
				Widget submenu, default_button = 0;
				int sn = stdarg_n;

				if (empty == 0)
					empty = XmStringCreatePCB("");

				submenu = XmCreatePulldownMenu(parent, XmStrCast(ctx->attrs[i].name == NULL ? "anon" : ctx->attrs[i].name), stdarg_args + sn, stdarg_n - sn);

				stdarg_n = sn;
				stdarg(XmNlabelString, empty);
				stdarg(XmNsubMenuId, submenu);
				ctx->wl[i] = XmCreateOptionMenu(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
				for (sn = 0; ctx->attrs[i].enumerations[sn]; sn++);
				ctx->btn[i] = calloc(sizeof(Widget), sn);
				for (sn = 0; ctx->attrs[i].enumerations[sn]; sn++) {
					Widget btn;
					XmString label;
					stdarg_n = 0;
					label = XmStringCreatePCB(ctx->attrs[i].enumerations[sn]);
					stdarg(XmNuserData, &ctx->attrs[i].enumerations[sn]);
					stdarg(XmNlabelString, label);
					btn = XmCreatePushButton(submenu, XmStrCast("menubutton"), stdarg_args, stdarg_n);
					XtManageChild(btn);
					XmStringFree(label);
					if (sn == ctx->attrs[i].default_val.int_value)
						default_button = btn;
					XtAddCallback(btn, XmNactivateCallback, valchg, ctx->wl[i]);
					(ctx->btn[i])[sn] = btn;
				}
				if (default_button) {
					stdarg_n = 0;
					stdarg(XmNmenuHistory, default_button);
					XtSetValues(ctx->wl[i], stdarg_args, stdarg_n);
				}
			}
			break;
		case PCB_HATT_BUTTON:
			stdarg(XmNlabelString, XmStringCreatePCB(ctx->attrs[i].default_val.str_value));
			ctx->wl[i] = XmCreatePushButton(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
			XtAddCallback(ctx->wl[i], XmNactivateCallback, valchg, ctx->wl[i]);
			break;
		default:
			ctx->wl[i] = XmCreateLabel(parent, XmStrCast("UNIMPLEMENTED"), stdarg_args, stdarg_n);
			break;
		}
		if (ctx->wl[i] != NULL)
			XtManageChild(ctx->wl[i]);
		if (tb != NULL) {
			tb->minw = tb->tabs * (tb->minw+10) + 10;
			if (tb->minw > ctx->minw)
				ctx->minw = tb->minw;
		}
	}
	return i;
}


static int attribute_dialog_set(lesstif_attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	char buf[30];
	int save, n;

	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;
	switch(ctx->attrs[idx].type) {
		case PCB_HATT_BEGIN_HBOX:
		case PCB_HATT_BEGIN_VBOX:
		case PCB_HATT_BEGIN_TABLE:
		case PCB_HATT_END:
			goto err;
		case PCB_HATT_BEGIN_TABBED:
			XtVaSetValues(ctx->wl[idx], XmNcurrentPageNumber, val->int_value+1, NULL);
			break;
		case PCB_HATT_BEGIN_HPANE:
		case PCB_HATT_BEGIN_VPANE:
			/* not possible to change the pane with the default motif widget */
			break;
		case PCB_HATT_BUTTON:
			XtVaSetValues(ctx->wl[idx], XmNlabelString, XmStringCreatePCB(val->str_value), NULL);
			break;
		case PCB_HATT_LABEL:
			XtVaSetValues(ctx->wl[idx], XmNlabelString, XmStringCreatePCB(val->str_value), NULL);
			break;
		case PCB_HATT_BOOL:
			XtVaSetValues(ctx->wl[idx], XmNset, val->int_value, NULL);
			break;
		case PCB_HATT_STRING:
			XtVaSetValues(ctx->wl[idx], XmNvalue, XmStrCast(val->str_value), NULL);
			break;
		case PCB_HATT_INTEGER:
			sprintf(buf, "%d", val->int_value);
			XtVaSetValues(ctx->wl[idx], XmNvalue, XmStrCast(buf), NULL);
			break;
		case PCB_HATT_COORD:
			pcb_snprintf(buf, sizeof(buf), "%$mS", val->coord_value);
			XtVaSetValues(ctx->wl[idx], XmNvalue, XmStrCast(buf), NULL);
			break;
		case PCB_HATT_REAL:
			pcb_snprintf(buf, sizeof(buf), "%g", val->real_value);
			XtVaSetValues(ctx->wl[idx], XmNvalue, XmStrCast(buf), NULL);
			break;
		case PCB_HATT_PROGRESS:
			ltf_progress_set(ctx, idx, val->real_value);
			break;
		case PCB_HATT_PREVIEW:
			ltf_preview_set(ctx, idx, val->real_value);
			break;
		case PCB_HATT_ENUM:
			for (n = 0; ctx->attrs[idx].enumerations[n]; n++) {
				if (n == val->int_value) {
					stdarg_n = 0;
					stdarg(XmNmenuHistory, (ctx->btn[idx])[n]);
					XtSetValues(ctx->wl[idx], stdarg_args, stdarg_n);
					goto ok;
				}
			}
			goto err;
		default:
			goto err;
	}

	ok:;
	ctx->attrs[idx].default_val = *val;
	ctx->inhibit_valchg = save;
	return 0;

	err:;
	ctx->inhibit_valchg = save;
	return -1;
}

static void ltf_attr_destroy_cb(Widget w, void *v, void *cbs)
{
	lesstif_attr_dlg_t *ctx = v;
	if ((!ctx->close_cb_called) && (ctx->close_cb != NULL)) {
		ctx->close_cb(ctx->caller_data, PCB_HID_ATTR_EV_WINCLOSE);
		ctx->close_cb_called = 1;
	}
	XtUnmanageChild(w);
	XtDestroyWidget(w);
}

void *lesstif_attr_dlg_new(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
{
	Widget topform, main_tbl;
	int i;
	lesstif_attr_dlg_t *ctx;

	ctx = calloc(sizeof(lesstif_attr_dlg_t), 1);
	ctx->attrs = attrs;
	ctx->results = results;
	ctx->n_attrs = n_attrs;
	ctx->caller_data = caller_data;
	ctx->minw = ctx->minh = 32;
	ctx->close_cb = button_cb;
	ctx->close_cb_called = 0;

	for (i = 0; i < n_attrs; i++) {
		if (attrs[i].help_text != ATTR_UNDOCUMENTED)
			ctx->actual_nattrs++;
		results[i] = attrs[i].default_val;
		if (PCB_HAT_IS_STR(attrs[i].type) && (results[i].str_value))
			results[i].str_value = pcb_strdup(results[i].str_value);
		else
			results[i].str_value = NULL;
	}

	ctx->wl = (Widget *) calloc(n_attrs, sizeof(Widget));
	ctx->btn = (Widget **) calloc(n_attrs, sizeof(Widget *));

	stdarg_n = 0;
	topform = XmCreateFormDialog(mainwind, XmStrCast(title), stdarg_args, stdarg_n);
	XtManageChild(topform);

	ctx->dialog = XtParent(topform);
	XtAddCallback(topform, XmNunmapCallback, ltf_attr_destroy_cb, ctx);


	stdarg_n = 0;
	stdarg(XmNfractionBase, ctx->n_attrs);
	XtSetValues(topform, stdarg_args, stdarg_n);


	if (!PCB_HATT_IS_COMPOSITE(attrs[0].type)) {
		stdarg_n = 0;
		main_tbl = pcb_motif_box(topform, XmStrCast("layout"), 't', pcb_hid_attrdlg_num_children(ctx->attrs, 0, ctx->n_attrs), 0, 0);
		XtManageChild(main_tbl);
		attribute_dialog_add(ctx, main_tbl, NULL, 0, 1);
	}
	else
		attribute_dialog_add(ctx, topform, NULL, 0, (ctx->attrs[0].pcb_hatt_flags & PCB_HATF_LABEL));

	/* don't expect screens larger than 800x600 */
	if (ctx->minw > 750)
		ctx->minw = 750;
	if (ctx->minw > 550)
		ctx->minw = 550;

	/* set top form's minimum width/height to content request */
	stdarg_n = 0;
	stdarg(XmNminWidth, ctx->minw);
	stdarg(XmNminHeight, ctx->minh);
	XtSetValues(XtParent(ctx->dialog), stdarg_args, stdarg_n);

	if (!modal)
		XtManageChild(ctx->dialog);

	return ctx;
}

int lesstif_attr_dlg_run(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	return wait_for_dialog(ctx->dialog);
}

void lesstif_attr_dlg_free(void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int i;

	if (ctx->already_closing)
		return;

	ctx->already_closing = 1;
	for (i = 0; i < ctx->n_attrs; i++) {
		attribute_dialog_readres(ctx, i);
		free(ctx->btn[i]);
	}

	if ((!ctx->close_cb_called) && (ctx->close_cb != NULL)) {
		ctx->close_cb_called = 1;
		ctx->close_cb(ctx->caller_data, PCB_HID_ATTR_EV_CODECLOSE);
	}

	free(ctx->wl);
	XtDestroyWidget(ctx->dialog);
	free(ctx);
}

void lesstif_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	if ((prop >= 0) && (prop < PCB_HATP_max))
		ctx->property[prop] = *val;
}

int lesstif_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	XtSetSensitive(ctx->wl[idx], enabled);
	return 0;
}

int lesstif_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (hide)
		XtUnmanageChild(ctx->wl[idx]);
	else
		XtManageChild(ctx->wl[idx]);

	return 0;
}

int lesstif_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	if (attribute_dialog_set(ctx, idx, val) == 0) {
		ctx->results[idx] = *val;
		return 0;
	}

	return -1;
}


static const char pcb_acts_DoWindows[] = "DoWindows(1|2|3|4)\n" "DoWindows(Layout|Library|Log|Netlist)";
static const char pcb_acth_DoWindows[] = "Open various GUI windows.";
/* DOC: dowindows.html */
static fgw_error_t pcb_act_DoWindows(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a = "";
	PCB_ACT_MAY_CONVARG(1, FGW_STR, DoWindows, a = argv[1].val.str);
	if (strcmp(a, "1") == 0 || pcb_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || pcb_strcasecmp(a, "Library") == 0) {
		lesstif_show_library();
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		if (log_form == 0)
			lesstif_log("");
		XtManageChild(log_form);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		lesstif_show_netlist();
	}
	else {
		PCB_ACT_FAIL(DoWindows);
		PCB_ACT_IRES(1);
		return 1;
	}
	PCB_ACT_IRES(0);
	return 0;
}

/* ------------------------------------------------------------ */

static Widget sizes_dialog = 0;
static Widget sz_pcb_w, sz_pcb_h, sz_bloat, sz_shrink, sz_drc_wid, sz_drc_slk, sz_drc_drill, sz_drc_ring;
static Widget sz_text;
static Widget sz_set, sz_reset, sz_units;

static int sz_str2val(Widget w, pcb_bool pcbu)
{
	char *buf = XmTextGetString(w);
	if (!pcbu)
		return strtol(buf, NULL, 0);
	return pcb_get_value_ex(buf, NULL, NULL, NULL, conf_core.editor.grid_unit->suffix, NULL);
}

static void sz_val2str(Widget w, pcb_coord_t u, int pcbu)
{
	static char buf[40];
	if (pcbu)
		pcb_sprintf(buf, "%m+%.2mS", conf_core.editor.grid_unit->allow, u);
	else
		pcb_snprintf(buf, sizeof(buf), "%#mS %%", u);
	XmTextSetString(w, buf);
}

static void sizes_set()
{
	PCB->MaxWidth = sz_str2val(sz_pcb_w, 1);
	PCB->MaxHeight = sz_str2val(sz_pcb_h, 1);

	conf_set_design("design/text_scale", "%s", sz_text);
	conf_set_design("design/bloat", "%s", sz_bloat);
	conf_set_design("design/shrink", "%s", sz_shrink);
	conf_set_design("design/min_wid", "%s", sz_drc_wid);
	conf_set_design("design/min_slk", "%s", sz_drc_slk);
	conf_set_design("design/min_drill", "%s", sz_drc_drill);
	conf_set_design("design/min_ring", "%s", sz_drc_ring);

	pcb_crosshair_set_range(0, 0, PCB->MaxWidth, PCB->MaxHeight);
	lesstif_pan_fixup();
}

void lesstif_sizes_reset()
{
	char *ls;
	if (!sizes_dialog)
		return;
	sz_val2str(sz_pcb_w, PCB->MaxWidth, 1);
	sz_val2str(sz_pcb_h, PCB->MaxHeight, 1);
	sz_val2str(sz_text, conf_core.design.text_scale, 0);

	ls = pcb_strdup_printf(_("Units are %s."), conf_core.editor.grid_unit->in_suffix);
	stdarg_n = 0;
	stdarg(XmNlabelString, XmStringCreatePCB(ls));
	XtSetValues(sz_units, stdarg_args, stdarg_n);
	free(ls);
}

static Widget size_field(Widget parent, const char *label, int posn)
{
	Widget w, l;
	stdarg_n = 0;
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_POSITION);
	stdarg(XmNtopPosition, posn);
	stdarg(XmNbottomAttachment, XmATTACH_POSITION);
	stdarg(XmNbottomPosition, posn + 1);
	stdarg(XmNcolumns, 10);
	w = XmCreateTextField(parent, XmStrCast("field"), stdarg_args, stdarg_n);
	XtManageChild(w);

	stdarg_n = 0;
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_WIDGET);
	stdarg(XmNrightWidget, w);
	stdarg(XmNtopAttachment, XmATTACH_POSITION);
	stdarg(XmNtopPosition, posn);
	stdarg(XmNbottomAttachment, XmATTACH_POSITION);
	stdarg(XmNbottomPosition, posn + 1);
	stdarg(XmNlabelString, XmStringCreatePCB(label));
	stdarg(XmNalignment, XmALIGNMENT_END);
	l = XmCreateLabel(parent, XmStrCast("label"), stdarg_args, stdarg_n);
	XtManageChild(l);

	return w;
}

static const char pcb_acts_AdjustSizes[] = "AdjustSizes()";
static const char pcb_acth_AdjustSizes[] = "Let the user change the board size, DRC parameters, etc";
/* DOC: adjustsizes.html */
static fgw_error_t pcb_act_AdjustSizes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (!sizes_dialog) {
		Widget inf, sep;

		stdarg_n = 0;
		stdarg(XmNmarginWidth, 3);
		stdarg(XmNmarginHeight, 3);
		stdarg(XmNhorizontalSpacing, 3);
		stdarg(XmNverticalSpacing, 3);
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNtitle, "Board Sizes");
		sizes_dialog = XmCreateFormDialog(mainwind, XmStrCast("sizes"), stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		sz_reset = XmCreatePushButton(sizes_dialog, XmStrCast("Reset"), stdarg_args, stdarg_n);
		XtManageChild(sz_reset);
		XtAddCallback(sz_reset, XmNactivateCallback, (XtCallbackProc) lesstif_sizes_reset, 0);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_WIDGET);
		stdarg(XmNrightWidget, sz_reset);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		sz_set = XmCreatePushButton(sizes_dialog, XmStrCast("Set"), stdarg_args, stdarg_n);
		XtManageChild(sz_set);
		XtAddCallback(sz_set, XmNactivateCallback, (XtCallbackProc) sizes_set, 0);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, sz_reset);
		sep = XmCreateSeparator(sizes_dialog, XmStrCast("sep"), stdarg_args, stdarg_n);
		XtManageChild(sep);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, sep);
		sz_units = XmCreateLabel(sizes_dialog, XmStrCast("units"), stdarg_args, stdarg_n);
		XtManageChild(sz_units);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, sz_units);
		stdarg(XmNfractionBase, 9);
		inf = XmCreateForm(sizes_dialog, XmStrCast("sizes"), stdarg_args, stdarg_n);
		XtManageChild(inf);

		sz_pcb_w = size_field(inf, "PCB Width", 0);
		sz_pcb_h = size_field(inf, "PCB Height", 1);
		sz_bloat = size_field(inf, "Bloat", 2);
		sz_shrink = size_field(inf, "Shrink", 3);
		sz_drc_wid = size_field(inf, "DRC Min Wid", 4);
		sz_drc_slk = size_field(inf, "DRC Min Silk", 5);
		sz_drc_drill = size_field(inf, "DRC Min Drill", 6);
		sz_drc_ring = size_field(inf, "DRC Min Annular Ring", 7);
		sz_text = size_field(inf, "Text Scale", 8);
	}
	lesstif_sizes_reset();
	XtManageChild(sizes_dialog);
	PCB_ACT_IRES(0);
	return 0;
}

/* ------------------------------------------------------------ */

void lesstif_update_layer_groups()
{
TODO("layer: call a redraw on the edit group")
}

/* ------------------------------------------------------------ */

typedef struct {
	Widget del;
	Widget w_name;
	Widget w_value;
} AttrRow;

static AttrRow *attr_row = 0;
static int attr_num_rows = 0;
static int attr_max_rows = 0;
static Widget attr_dialog = NULL, f_top;
static pcb_attribute_list_t *attributes_list;

static void attributes_delete_callback(Widget w, void *v, void *cbs);

static void fiddle_with_bb_layout()
{
	int i;
	int max_height = 0;
	int max_del_width = 0;
	int max_name_width = 0;
	int max_value_width = 0;
	short ncolumns = 20;
	short vcolumns = 20;

	for (i = 0; i < attr_num_rows; i++) {
		String v;

		stdarg_n = 0;
		stdarg(XmNvalue, &v);
		XtGetValues(attr_row[i].w_name, stdarg_args, stdarg_n);
		if (ncolumns < strlen(v))
			ncolumns = strlen(v);

		stdarg_n = 0;
		stdarg(XmNvalue, &v);
		XtGetValues(attr_row[i].w_value, stdarg_args, stdarg_n);
		if (vcolumns < strlen(v))
			vcolumns = strlen(v);
	}

	for (i = 0; i < attr_num_rows; i++) {
		stdarg_n = 0;
		stdarg(XmNcolumns, ncolumns);
		XtSetValues(attr_row[i].w_name, stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNcolumns, vcolumns);
		XtSetValues(attr_row[i].w_value, stdarg_args, stdarg_n);
	}

	for (i = 0; i < attr_num_rows; i++) {
		Dimension w, h;
		stdarg_n = 0;
		stdarg(XmNwidth, &w);
		stdarg(XmNheight, &h);

		XtGetValues(attr_row[i].del, stdarg_args, stdarg_n);
		if (max_height < h)
			max_height = h;
		if (max_del_width < w)
			max_del_width = w;

		XtGetValues(attr_row[i].w_name, stdarg_args, stdarg_n);
		if (max_height < h)
			max_height = h;
		if (max_name_width < w)
			max_name_width = w;

		XtGetValues(attr_row[i].w_value, stdarg_args, stdarg_n);
		if (max_height < h)
			max_height = h;
		if (max_value_width < w)
			max_value_width = w;
	}

	for (i = 0; i < attr_num_rows; i++) {
		stdarg_n = 0;
		stdarg(XmNx, 0);
		stdarg(XmNy, i * max_height);
		stdarg(XmNwidth, max_del_width);
		stdarg(XmNheight, max_height);
		XtSetValues(attr_row[i].del, stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNx, max_del_width);
		stdarg(XmNy, i * max_height);
		stdarg(XmNwidth, max_name_width);
		stdarg(XmNheight, max_height);
		XtSetValues(attr_row[i].w_name, stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNx, max_del_width + max_name_width);
		stdarg(XmNy, i * max_height);
		stdarg(XmNwidth, max_value_width);
		stdarg(XmNheight, max_height);
		XtSetValues(attr_row[i].w_value, stdarg_args, stdarg_n);
	}

	stdarg_n = 0;
	stdarg(XmNwidth, max_del_width + max_name_width + max_value_width + 1);
	stdarg(XmNheight, max_height * attr_num_rows + 1);
	XtSetValues(f_top, stdarg_args, stdarg_n);
}

static void lesstif_attributes_need_rows(int new_max)
{
	if (attr_max_rows < new_max) {
		if (attr_row)
			attr_row = (AttrRow *) realloc(attr_row, new_max * sizeof(AttrRow));
		else
			attr_row = (AttrRow *) malloc(new_max * sizeof(AttrRow));
	}

	while (attr_max_rows < new_max) {
		stdarg_n = 0;
		attr_row[attr_max_rows].del = XmCreatePushButton(f_top, XmStrCast("del"), stdarg_args, stdarg_n);
		XtManageChild(attr_row[attr_max_rows].del);
		XtAddCallback(attr_row[attr_max_rows].del, XmNactivateCallback,
									(XtCallbackProc) attributes_delete_callback, (XtPointer) (size_t) attr_max_rows);

		stdarg_n = 0;
		stdarg(XmNresizeWidth, True);
		attr_row[attr_max_rows].w_name = XmCreateTextField(f_top, XmStrCast("name"), stdarg_args, stdarg_n);
		XtManageChild(attr_row[attr_max_rows].w_name);
		XtAddCallback(attr_row[attr_max_rows].w_name, XmNvalueChangedCallback, (XtCallbackProc) fiddle_with_bb_layout, NULL);

		stdarg_n = 0;
		stdarg(XmNresizeWidth, True);
		attr_row[attr_max_rows].w_value = XmCreateTextField(f_top, XmStrCast("value"), stdarg_args, stdarg_n);
		XtManageChild(attr_row[attr_max_rows].w_value);
		XtAddCallback(attr_row[attr_max_rows].w_value, XmNvalueChangedCallback, (XtCallbackProc) fiddle_with_bb_layout, NULL);

		attr_max_rows++;
	}

	/* Manage any previously unused rows we now need to show.  */
	while (attr_num_rows < new_max) {
		XtManageChild(attr_row[attr_num_rows].del);
		XtManageChild(attr_row[attr_num_rows].w_name);
		XtManageChild(attr_row[attr_num_rows].w_value);
		attr_num_rows++;
	}
}

static void lesstif_attributes_revert()
{
	int i;

	lesstif_attributes_need_rows(attributes_list->Number);

	/* Unmanage any previously used rows we don't need.  */
	while (attr_num_rows > attributes_list->Number) {
		attr_num_rows--;
		XtUnmanageChild(attr_row[attr_num_rows].del);
		XtUnmanageChild(attr_row[attr_num_rows].w_name);
		XtUnmanageChild(attr_row[attr_num_rows].w_value);
	}

	/* Fill in values */
	for (i = 0; i < attributes_list->Number; i++) {
		XmTextFieldSetString(attr_row[i].w_name, attributes_list->List[i].name);
		XmTextFieldSetString(attr_row[i].w_value, attributes_list->List[i].value);
	}

	fiddle_with_bb_layout();
}

static void attributes_new_callback(Widget w, void *v, void *cbs)
{
	lesstif_attributes_need_rows(attr_num_rows + 1);	/* also bumps attr_num_rows */
	XmTextFieldSetString(attr_row[attr_num_rows - 1].w_name, XmStrCast(""));
	XmTextFieldSetString(attr_row[attr_num_rows - 1].w_value, XmStrCast(""));

	fiddle_with_bb_layout();
}

static void attributes_delete_callback(Widget w, void *v, void *cbs)
{
	int i, n;
	Widget wn, wv;

	n = (int) (size_t) v;

	wn = attr_row[n].w_name;
	wv = attr_row[n].w_value;

	for (i = n; i < attr_num_rows - 1; i++) {
		attr_row[i].w_name = attr_row[i + 1].w_name;
		attr_row[i].w_value = attr_row[i + 1].w_value;
	}
	attr_row[attr_num_rows - 1].w_name = wn;
	attr_row[attr_num_rows - 1].w_value = wv;
	attr_num_rows--;

	XtUnmanageChild(wn);
	XtUnmanageChild(wv);

	fiddle_with_bb_layout();
}

static void attributes_revert_callback(Widget w, void *v, void *cbs)
{
	lesstif_attributes_revert();
}

void lesstif_attributes_dialog(const char *owner, pcb_attribute_list_t * attrs_list)
{
	Widget bform, sw, b_ok, b_cancel, b_revert, b_new;
	Widget sep;

	if (attr_dialog == NULL) {
		stdarg_n = 0;
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNtitle, owner);
		stdarg(XmNwidth, 400);
		stdarg(XmNheight, 300);
		attr_dialog = XmCreateFormDialog(mainwind, XmStrCast("attributes"), stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNorientation, XmHORIZONTAL);
		stdarg(XmNentryAlignment, XmALIGNMENT_CENTER);
		stdarg(XmNpacking, XmPACK_COLUMN);
		bform = XmCreateRowColumn(attr_dialog, XmStrCast("attributes"), stdarg_args, stdarg_n);
		XtManageChild(bform);

		stdarg_n = 0;
		b_ok = XmCreatePushButton(bform, XmStrCast("OK"), stdarg_args, stdarg_n);
		XtManageChild(b_ok);
		XtAddCallback(b_ok, XmNactivateCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 0);

		stdarg_n = 0;
		b_new = XmCreatePushButton(bform, XmStrCast("New"), stdarg_args, stdarg_n);
		XtManageChild(b_new);
		XtAddCallback(b_new, XmNactivateCallback, (XtCallbackProc) attributes_new_callback, NULL);

		stdarg_n = 0;
		b_revert = XmCreatePushButton(bform, XmStrCast("Revert"), stdarg_args, stdarg_n);
		XtManageChild(b_revert);
		XtAddCallback(b_revert, XmNactivateCallback, (XtCallbackProc) attributes_revert_callback, NULL);

		stdarg_n = 0;
		b_cancel = XmCreatePushButton(bform, XmStrCast("Cancel"), stdarg_args, stdarg_n);
		XtManageChild(b_cancel);
		XtAddCallback(b_cancel, XmNactivateCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 1);

		stdarg_n = 0;
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, bform);
		sep = XmCreateSeparator(attr_dialog, XmStrCast("attributes"), stdarg_args, stdarg_n);
		XtManageChild(sep);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNbottomWidget, sep);
		stdarg(XmNscrollingPolicy, XmAUTOMATIC);
		sw = XmCreateScrolledWindow(attr_dialog, XmStrCast("attributes"), stdarg_args, stdarg_n);
		XtManageChild(sw);

		stdarg_n = 0;
		stdarg(XmNmarginHeight, 0);
		stdarg(XmNmarginWidth, 0);
		f_top = XmCreateBulletinBoard(sw, XmStrCast("f_top"), stdarg_args, stdarg_n);
		XtManageChild(f_top);
	}
	else {
		stdarg_n = 0;
		stdarg(XmNtitle, owner);
		XtSetValues(XtParent(attr_dialog), stdarg_args, stdarg_n);
	}

	attributes_list = attrs_list;
	lesstif_attributes_revert();

	fiddle_with_bb_layout();

	if (wait_for_dialog(attr_dialog) == 0) {
		int i;
		/* Copy the values back */
		pcb_attribute_copyback_begin(attributes_list);
		for (i = 0; i < attr_num_rows; i++)
			pcb_attribute_copyback(attributes_list, XmTextFieldGetString(attr_row[i].w_name), XmTextFieldGetString(attr_row[i].w_value));
		pcb_attribute_copyback_end(attributes_list);
	}

	return;
}

static const char pcb_acts_ImportGUI[] = "ImportGUI()";
static const char pcb_acth_ImportGUI[] = "Lets the user choose the schematics to import from";
/* DOC: importgui.html */
static fgw_error_t pcb_act_ImportGUI(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static int I_am_recursing = 0;
	static XmString xms_sch = 0, xms_import = 0;
	int rv;
	XmString xmname;
	char *name, *bname;
	char *original_dir, *target_dir, *last_slash;

	if (I_am_recursing)
		return 1;

	if (xms_sch == 0)
		xms_sch = XmStringCreatePCB("*.sch");
	if (xms_import == 0)
		xms_import = XmStringCreatePCB("Import from");

	setup_fsb_dialog();

	stdarg_n = 0;
	stdarg(XmNtitle, "Import From");
	XtSetValues(XtParent(fsb), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNpattern, xms_sch);
	stdarg(XmNmustMatch, True);
	stdarg(XmNselectionLabelString, xms_import);
	XtSetValues(fsb, stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNdirectory, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);
	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &original_dir);

	if (!wait_for_dialog(fsb))
		return 1;

	stdarg_n = 0;
	stdarg(XmNdirectory, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);
	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &target_dir);

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	/* If the user didn't change directories, use just the base name.
	   This is the common case and means we don't have to get clever
	   about converting absolute paths into relative paths.  */
	bname = name;
	if (strcmp(original_dir, target_dir) == 0) {
		last_slash = strrchr(name, '/');
		if (last_slash)
			bname = last_slash + 1;
	}

	pcb_attrib_put(PCB, "import::src0", bname);

	XtFree(name);


	I_am_recursing = 1;
	rv = pcb_action("Import");
	I_am_recursing = 0;

	PCB_ACT_IRES(rv);
	return 0;
}

/* ------------------------------------------------------------ */

pcb_action_t lesstif_dialog_action_list[] = {
	{"Load", pcb_act_Load, pcb_acth_Load, pcb_acts_Load},
	{"Save", pcb_act_Save, pcb_acth_Save, pcb_acts_Save},
	{"DoWindows", pcb_act_DoWindows, pcb_acth_DoWindows, pcb_acts_DoWindows},
	{"AdjustSizes", pcb_act_AdjustSizes, pcb_acth_AdjustSizes, pcb_acts_AdjustSizes},
	{"ImportGUI", pcb_act_ImportGUI, pcb_acth_ImportGUI, pcb_acts_ImportGUI}
};

PCB_REGISTER_ACTIONS(lesstif_dialog_action_list, lesstif_cookie)
