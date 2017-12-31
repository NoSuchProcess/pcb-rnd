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
#include "hid_actions.h"
#include "hid_init.h"
#include "stdarg.h"
#include "misc_util.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "search.h"
#include "action_helper.h"
#include "change.h"

static int ok;

#define COMPONENT_SIDE_NAME "(top)"
#define SOLDER_SIDE_NAME "(bottom)"

/* ------------------------------------------------------------ */

static void dialog_callback_ok_value(Widget w, void *v, void *cbs)
{
	ok = (int) (size_t) v;
}

typedef struct {
	void (*cb)(void *ctx, pcb_hid_attr_ev_t ev);
	void *ctx;
} dialog_cb_ctx_t;

static void dialog_callback_cancel(Widget w, void *v, void *cbs)
{
	dialog_cb_ctx_t *ctx = (dialog_cb_ctx_t *)v;
	if (ctx != NULL) {
		ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_CANCEL);
		free(ctx);
	}
	ok = 0;
}

static void dialog_callback_ok(Widget w, void *v, void *cbs)
{
	dialog_cb_ctx_t *ctx = (dialog_cb_ctx_t *)v;
	if (ctx != NULL) {
		ctx->cb(ctx->ctx, PCB_HID_ATTR_EV_OK);
		free(ctx);
	}
	ok = 1;
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

static const char load_syntax[] = "Load()\n" "Load(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert)";

static const char load_help[] = "Load layout data from a user-selected file.";

/* %start-doc actions Load

This action is a GUI front-end to the core's @code{LoadFrom} action
(@pxref{LoadFrom Action}).  If you happen to pass a filename, like
@code{LoadFrom}, then @code{LoadFrom} is called directly.  Else, the
user is prompted for a filename to load, and then @code{LoadFrom} is
called with that filename.

%end-doc */

static int Load(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function;
	char *name;
	XmString xmname, pattern;

	if (argc > 1)
		return pcb_hid_actionv("LoadFrom", argc, argv);

	function = argc ? argv[0] : "Layout";

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

	if (!wait_for_dialog(fsb))
		return 1;

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	pcb_hid_actionl("LoadFrom", function, name, NULL);

	XtFree(name);

	return 0;
}

static const char loadvendor_syntax[] = "LoadVendor()";

static const char loadvendor_help[] = "Loads a user-selected vendor resource file.";

/* %start-doc actions LoadVendor

The user is prompted for a file to load, and then
@code{LoadVendorFrom} is called (@pxref{LoadVendorFrom Action}) to
load that vendor file.

%end-doc */

static int LoadVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	char *name;
	XmString xmname, pattern;

	if (argc > 0)
		return pcb_hid_actionv("LoadVendorFrom", argc, argv);

	setup_fsb_dialog();

	pattern = xms_vend;

	stdarg_n = 0;
	stdarg(XmNtitle, "Load Vendor");
	XtSetValues(XtParent(fsb), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNpattern, pattern);
	stdarg(XmNmustMatch, True);
	stdarg(XmNselectionLabelString, xms_loadv);
	XtSetValues(fsb, stdarg_args, stdarg_n);

	if (!wait_for_dialog(fsb))
		return 1;

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	pcb_hid_actionl("LoadVendorFrom", name, NULL);

	XtFree(name);

	return 0;
}

static const char save_syntax[] =
	"Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";

static const char save_help[] = "Save layout data to a user-selected file.";

/* %start-doc actions Save

This action is a GUI front-end to the core's @code{SaveTo} action
(@pxref{SaveTo Action}).  If you happen to pass a filename, like
@code{SaveTo}, then @code{SaveTo} is called directly.  Else, the
user is prompted for a filename to save, and then @code{SaveTo} is
called with that filename.

%end-doc */

static int Save(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function;
	char *name;
	XmString xmname, pattern;

	if (argc > 1)
		pcb_hid_actionv("SaveTo", argc, argv);

	function = argc ? argv[0] : "Layout";

	if (pcb_strcasecmp(function, "Layout") == 0)
		if (PCB->Filename)
			return pcb_hid_actionl("SaveTo", "Layout", PCB->Filename, NULL);

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

	if (!wait_for_dialog(fsb))
		return 1;

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(fsb, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0)
		pcb_hid_actionl("PasteBuffer", "Save", name, NULL);
	else {
		/*
		 * if we got this far and the function is Layout, then
		 * we really needed it to be a LayoutAs.  Otherwise
		 * ActionSaveTo() will ignore the new file name we
		 * just obtained.
		 */
		if (pcb_strcasecmp(function, "Layout") == 0)
			pcb_hid_actionl("SaveTo", "LayoutAs", name, NULL);
		else
			pcb_hid_actionl("SaveTo", function, name, NULL);
	}
	XtFree(name);

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

static Widget confirm_dialog = 0;
static Widget confirm_cancel, confirm_ok, confirm_label;

int lesstif_confirm_dialog(const char *msg, ...)
{
	const char *cancelmsg, *okmsg;
	va_list ap;
	XmString xs;

	if (mainwind == 0)
		return 1;

	if (confirm_dialog == 0) {
		stdarg_n = 0;
		stdarg(XmNdefaultButtonType, XmDIALOG_OK_BUTTON);
		stdarg(XmNtitle, "Confirm");
		confirm_dialog = XmCreateQuestionDialog(mainwind, XmStrCast("confirm"), stdarg_args, stdarg_n);
		XtAddCallback(confirm_dialog, XmNcancelCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 0);
		XtAddCallback(confirm_dialog, XmNokCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 1);

		confirm_cancel = XmMessageBoxGetChild(confirm_dialog, XmDIALOG_CANCEL_BUTTON);
		confirm_ok = XmMessageBoxGetChild(confirm_dialog, XmDIALOG_OK_BUTTON);
		confirm_label = XmMessageBoxGetChild(confirm_dialog, XmDIALOG_MESSAGE_LABEL);
		XtUnmanageChild(XmMessageBoxGetChild(confirm_dialog, XmDIALOG_HELP_BUTTON));
	}

	va_start(ap, msg);
	cancelmsg = va_arg(ap, const char *);
	okmsg = va_arg(ap, const char *);
	va_end(ap);

	if (!cancelmsg) {
		cancelmsg = "Cancel";
		okmsg = "Ok";
	}

	stdarg_n = 0;
	xs = XmStringCreatePCB(cancelmsg);

	if (okmsg) {
		stdarg(XmNcancelLabelString, xs);
		xs = XmStringCreatePCB(okmsg);
		XtManageChild(confirm_cancel);
	}
	else
		XtUnmanageChild(confirm_cancel);

	stdarg(XmNokLabelString, xs);

	xs = XmStringCreatePCB(msg);
	stdarg(XmNmessageString, xs);
	XtSetValues(confirm_dialog, stdarg_args, stdarg_n);

	wait_for_dialog(confirm_dialog);

	stdarg_n = 0;
	stdarg(XmNdefaultPosition, False);
	XtSetValues(confirm_dialog, stdarg_args, stdarg_n);

	return ok;
}

static int ConfirmAction(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int rv = lesstif_confirm_dialog(argc > 0 ? argv[0] : 0,
																	argc > 1 ? argv[1] : 0,
																	argc > 2 ? argv[2] : 0,
																	0);
	return rv;
}

/* ------------------------------------------------------------ */

int lesstif_close_confirm_dialog()
{
	return lesstif_confirm_dialog("OK to lose data ?", NULL);
}

/* ------------------------------------------------------------ */

static Widget report = 0, report_form;

void lesstif_report_dialog(const char *title, const char *msg)
{
	if (!report) {
		if (mainwind == 0)
			return;

		stdarg_n = 0;
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNwidth, 600);
		stdarg(XmNheight, 200);
		stdarg(XmNtitle, title);
		report_form = XmCreateFormDialog(mainwind, XmStrCast("report"), stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNeditable, False);
		stdarg(XmNeditMode, XmMULTI_LINE_EDIT);
		stdarg(XmNcursorPositionVisible, False);
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		report = XmCreateScrolledText(report_form, XmStrCast("text"), stdarg_args, stdarg_n);
		XtManageChild(report);
	}
	stdarg_n = 0;
	stdarg(XmNtitle, title);
	XtSetValues(report_form, stdarg_args, stdarg_n);
	XmTextSetString(report, (char *) msg);

	XtManageChild(report_form);
}

/* ------------------------------------------------------------ */
/* FIXME -- make this a proper file select dialog box */
char *lesstif_fileselect(const char *title, const char *descr,
												 const char *default_file, const char *default_ext, const char *history_tag, int flags)
{

	return lesstif_prompt_for(title, default_file);
}

/* ------------------------------------------------------------ */

static Widget prompt_dialog = 0;
static Widget prompt_label, prompt_text;

char *lesstif_prompt_for(const char *msg, const char *default_string)
{
	char *rv;
	XmString xs;
	if (prompt_dialog == 0) {
		stdarg_n = 0;
		stdarg(XmNautoUnmanage, False);
		stdarg(XmNtitle, "pcb-rnd Prompt");
		prompt_dialog = XmCreateFormDialog(mainwind, XmStrCast("prompt"), stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
		prompt_label = XmCreateLabel(prompt_dialog, XmStrCast("label"), stdarg_args, stdarg_n);
		XtManageChild(prompt_label);

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_WIDGET);
		stdarg(XmNtopWidget, prompt_label);
		stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		stdarg(XmNeditable, True);
		prompt_text = XmCreateText(prompt_dialog, XmStrCast("text"), stdarg_args, stdarg_n);
		XtManageChild(prompt_text);
		XtAddCallback(prompt_text, XmNactivateCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) 1);
	}
	if (!default_string)
		default_string = "";
	if (!msg)
		msg = "Enter text:";
	stdarg_n = 0;
	xs = XmStringCreatePCB(msg);
	stdarg(XmNlabelString, xs);
	XtSetValues(prompt_label, stdarg_args, stdarg_n);
	XmTextSetString(prompt_text, (char *) default_string);
	XmTextSetCursorPosition(prompt_text, strlen(default_string));
	wait_for_dialog(prompt_dialog);
	rv = XmTextGetString(prompt_text);
	return rv;
}

static const char promptfor_syntax[] = "PromptFor([message[,default]])";

static const char promptfor_help[] = "Prompt for a response.";

/* %start-doc actions PromptFor

This is mostly for testing the lesstif HID interface.  The parameters
are passed to the @code{prompt_for()} HID function, causing the user
to be prompted for a response.  The respose is simply printed to the
user's stdout.

%end-doc */

static int PromptFor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	char *rv = lesstif_prompt_for(argc > 0 ? argv[0] : 0,
																argc > 1 ? argv[1] : 0);
	printf("rv = `%s'\n", rv);
	return 0;
}

/* ------------------------------------------------------------ */

static Widget create_form_ok_dialog(const char *name, int ok, void (*button_cb)(void *ctx, pcb_hid_attr_ev_t ev), void *ctx)
{
	Widget dialog, topform;
	dialog_cb_ctx_t *cb_ctx = NULL;

	stdarg_n = 0;
	dialog = XmCreateQuestionDialog(mainwind, XmStrCast(name), stdarg_args, stdarg_n);

	if (button_cb != NULL) {
		cb_ctx = malloc(sizeof(dialog_cb_ctx_t));
		cb_ctx->cb = button_cb;
		cb_ctx->ctx = ctx;
	}

	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_SYMBOL_LABEL));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_MESSAGE_LABEL));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc) dialog_callback_cancel, cb_ctx);
	if (ok)
		XtAddCallback(dialog, XmNokCallback, (XtCallbackProc) dialog_callback_ok, cb_ctx);
	else
		XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON));

	stdarg_n = 0;
	topform = XmCreateForm(dialog, XmStrCast("attributes"), stdarg_args, stdarg_n);
	XtManageChild(topform);
	return topform;
}

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
			stdarg(XmNpacking, XmPACK_COLUMN);
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
	unsigned inhibit_valchg:1;
	Dimension minw, minh;
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

/* parent info for tabbed */
typedef struct {
	Widget notebook;
	const char **tablab;
	Dimension minw;
	int tabs;
} attr_dlg_tb_t;

/* returns the index of HATT_END where the loop had to stop */
static int attribute_dialog_add(lesstif_attr_dlg_t *ctx, Widget real_parent, attr_dlg_tb_t *tb, int start_from, int add_labels)
{
	int len, i, numch, numcol;
	Widget parent;
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
			w = XmCreateLabel(parent, XmStrCast(ctx->attrs[i].name), stdarg_args, stdarg_n);
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

		case PCB_HATT_BEGIN_TABLE:
			/* create content table */
			numcol = ctx->attrs[i].pcb_hatt_table_cols;
			len = pcb_hid_atrdlg_num_children(ctx->attrs, i+1, ctx->n_attrs);
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
			stdarg(XmNbackPagePlacement, XmTOP_RIGHT);
			stdarg(XmNbackPageNumber, 1);
			stdarg(XmNbackPageSize, 1);
			stdarg(XmNbindingType, XmNONE);
			stdarg(XmNorientation, XmVERTICAL);

			stdarg(XmNleftAttachment, XmATTACH_FORM);
			stdarg(XmNtopAttachment, XmATTACH_FORM);
			stdarg(XmNrightAttachment, XmATTACH_FORM);
			stdarg(XmNbottomAttachment, XmATTACH_FORM);
			ctx->wl[i] = w = XmCreateNotebook(parent, "notebook", stdarg_args, stdarg_n);

			/* remove the page scroller widget that got automatically created by XmCreateNotebook() */
			scroller = XtNameToWidget(w, "PageScroller");
			XtUnmanageChild (scroller);

			tb.notebook = w;
			tb.tablab = ctx->attrs[i].enumerations;
			tb.minw = 0;
			tb.tabs = 0;

			i = attribute_dialog_add(ctx, w, &tb, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));

			XtManageChild(w);
			break;
		}

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

void *lesstif_attr_dlg_new(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev))
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

	topform = create_form_ok_dialog(title, 1, button_cb, caller_data);
	ctx->dialog = XtParent(topform);

	stdarg_n = 0;
	stdarg(XmNfractionBase, ctx->n_attrs);
	XtSetValues(topform, stdarg_args, stdarg_n);


	if (!PCB_HATT_IS_COMPOSITE(attrs[0].type)) {
		stdarg_n = 0;
		main_tbl = pcb_motif_box(topform, XmStrCast("layout"), 't', pcb_hid_atrdlg_num_children(ctx->attrs, 0, ctx->n_attrs), 0, 0);
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

	for (i = 0; i < ctx->n_attrs; i++) {
		attribute_dialog_readres(ctx, i);
		free(ctx->btn[i]);
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

int lesstif_attribute_dialog(pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data)
{
	int rv;
	void *hid_ctx;
	
	hid_ctx = lesstif_attr_dlg_new(attrs, n_attrs, results, title, descr, caller_data, pcb_true, NULL);
	rv = lesstif_attr_dlg_run(hid_ctx);
	lesstif_attr_dlg_free(hid_ctx);

	return rv ? 0 : 1;
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


/* ------------------------------------------------------------ */

static const char dowindows_syntax[] = "DoWindows(1|2|3|4)\n" "DoWindows(Layout|Library|Log|Netlist)";

static const char dowindows_help[] = "Open various GUI windows.";

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@end table

%end-doc */

static int DoWindows(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *a = argc == 1 ? argv[0] : "";
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
		lesstif_log("Usage: DoWindows(1|2|3|4|Layout|Library|Log|Netlist)");
		return 1;
	}
	return 0;
}

/* ------------------------------------------------------------ */
static const char about_syntax[] = "About()";

static const char about_help[] = "Tell the user about this version of PCB.";

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int About(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	static Widget about = 0;
	if (!about) {
		XmString xs;
		stdarg_n = 0;
		xs = XmStringCreatePCB(pcb_get_infostr());
		stdarg(XmNmessageString, xs);
		stdarg(XmNtitle, "About pcb-rnd");
		about = XmCreateInformationDialog(mainwind, XmStrCast("about"), stdarg_args, stdarg_n);
		XtUnmanageChild(XmMessageBoxGetChild(about, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(about, XmDIALOG_HELP_BUTTON));
	}
	wait_for_dialog(about);
	return 0;
}

/* ------------------------------------------------------------ */

static const char print_syntax[] = "Print()";

static const char print_help[] = "Print the layout.";

/* %start-doc actions Print

This will find the default printing HID, prompt the user for its
options, and print the layout.

%end-doc */

static int Print(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_hid_attribute_t *opts;
	pcb_hid_t *printer;
	pcb_hid_attr_val_t *vals;
	int n;

	printer = pcb_hid_find_printer();
	if (!printer) {
		lesstif_confirm_dialog("No printer?", "Oh well", 0);
		return 1;
	}
	opts = printer->get_export_options(&n);
	vals = (pcb_hid_attr_val_t *) calloc(n, sizeof(pcb_hid_attr_val_t));
	if (lesstif_attribute_dialog(opts, n, vals, "Print", "", NULL)) {
		free(vals);
		return 1;
	}
	printer->do_export(vals);
	free(vals);
	return 0;
}

static pcb_hid_attribute_t printer_calibrate_attrs[] = {
	{"Enter Values here:", "",
	 PCB_HATT_LABEL, 0, 0, {0, 0, 0}, 0, 0},
	{"x-calibration", "X scale for calibrating your printer",
	 PCB_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0},
	{"y-calibration", "Y scale for calibrating your printer",
	 PCB_HATT_REAL, 0.5, 25, {0, 0, 1.00}, 0, 0}
};

static pcb_hid_attr_val_t printer_calibrate_values[3];

static const char printcalibrate_syntax[] = "PrintCalibrate()";

static const char printcalibrate_help[] = "Calibrate the printer.";

/* %start-doc actions PrintCalibrate

This will print a calibration page, which you would measure and type
the measurements in, so that future printouts will be more precise.

%end-doc */

static int PrintCalibrate(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_hid_t *printer = pcb_hid_find_printer();
	printer->calibrate(0.0, 0.0);
	if (pcb_gui->attribute_dialog(printer_calibrate_attrs, 3,
														printer_calibrate_values,
														"Printer Calibration Values", "Enter calibration values for your printer", NULL))
		return 1;
	printer->calibrate(printer_calibrate_values[1].real_value, printer_calibrate_values[2].real_value);
	return 0;
}

static const char exportgui_syntax[] = "ExportGUI()";

static const char exportgui_help[] = "Export the layout. Export is configured using dialog a box.";

/* %start-doc actions Export

Prompts the user for an exporter to use.  Then, prompts the user for
that exporter's options, and exports the layout.

%end-doc */

static int ExportGUI(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	static Widget selector = 0;
	pcb_hid_attribute_t *opts;
	pcb_hid_t *printer, **hids;
	pcb_hid_attr_val_t *vals;
	int n, i, count;
	Widget prev = 0;
	Widget w;

	hids = pcb_hid_enumerate();

	if (!selector) {
		stdarg_n = 0;
		stdarg(XmNtitle, "Export HIDs");
		selector = create_form_ok_dialog("export", 0, NULL, NULL);
		count = 0;
		for (i = 0; hids[i]; i++) {
			if (hids[i]->exporter) {
				stdarg_n = 0;
				if (prev) {
					stdarg(XmNtopAttachment, XmATTACH_WIDGET);
					stdarg(XmNtopWidget, prev);
				}
				else {
					stdarg(XmNtopAttachment, XmATTACH_FORM);
				}
				stdarg(XmNrightAttachment, XmATTACH_FORM);
				stdarg(XmNleftAttachment, XmATTACH_FORM);
				w = XmCreatePushButton(selector, (char *) hids[i]->name, stdarg_args, stdarg_n);
				XtManageChild(w);
				XtAddCallback(w, XmNactivateCallback, (XtCallbackProc) dialog_callback_ok_value, (XtPointer) ((size_t) i + 1));
				prev = w;
				count++;
			}
		}
		if (count == 0) {
			Widget label;
			stdarg_n = 0;
			stdarg(XmNlabelString, XmStringCreatePCB("No exporter found. Check your plugins!"));
			stdarg(XmNtopAttachment, XmATTACH_FORM);
			stdarg(XmNrightAttachment, XmATTACH_FORM);
			stdarg(XmNleftAttachment, XmATTACH_FORM);
			label = XmCreateLabel(selector, XmStrCast("label"), stdarg_args, stdarg_n);
			XtManageChild(label);
		}
		selector = XtParent(selector);
	}


	i = wait_for_dialog(selector);

	if (i <= 0)
		return 1;
	printer = hids[i - 1];

	pcb_exporter = printer;

	opts = printer->get_export_options(&n);
	vals = (pcb_hid_attr_val_t *) calloc(n, sizeof(pcb_hid_attr_val_t));
	if (lesstif_attribute_dialog(opts, n, vals, "Export", NULL, NULL)) {
		free(vals);
		return 1;
	}
	printer->do_export(vals);
	free(vals);
	pcb_exporter = NULL;
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
	PCB->Bloat = sz_str2val(sz_bloat, 1);
	PCB->Shrink = sz_str2val(sz_shrink, 1);
	PCB->minWid = sz_str2val(sz_drc_wid, 1);
	PCB->minSlk = sz_str2val(sz_drc_slk, 1);
	PCB->minDrill = sz_str2val(sz_drc_drill, 1);
	PCB->minRing = sz_str2val(sz_drc_ring, 1);
#warning think these over - are these only for new designs amd we keep real values in PCB-> ?
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
	sz_val2str(sz_bloat, PCB->Bloat, 1);
	sz_val2str(sz_shrink, PCB->Shrink, 1);
	sz_val2str(sz_drc_wid, PCB->minWid, 1);
	sz_val2str(sz_drc_slk, PCB->minSlk, 1);
	sz_val2str(sz_drc_drill, PCB->minDrill, 1);
	sz_val2str(sz_drc_ring, PCB->minRing, 1);
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

static const char adjustsizes_syntax[] = "AdjustSizes()";

static const char adjustsizes_help[] = "Let the user change the board size, DRC parameters, etc";

/* %start-doc actions AdjustSizes

Displays a dialog box that lets the user change the board
size, DRC parameters, and text scale.

The units are determined by the default display units.

%end-doc */

static int AdjustSizes(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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
	return 0;
}

/* ------------------------------------------------------------ */

void lesstif_update_layer_groups()
{
#warning layer TODO: call a redraw on the edit group
}

static const char editlayergroups_syntax[] = "EditLayerGroups()";

static const char editlayergroups_help[] = "Let the user change the layer groupings";

/* %start-doc actions EditLayerGroups

Displays a dialog that lets the user view and change the layer
groupings.  Each layer (row) can be a member of any one layer group
(column).  Note the special layers @code{solder} and @code{component}
allow you to specify which groups represent the top and bottom of the
board.

See @ref{ChangeName Action}.

%end-doc */

extern void lesstif_show_layergrp_edit(void);
static int EditLayerGroups(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	lesstif_show_layergrp_edit();
	return 1;
}


static const char pcb_acts_fontsel[] = "EditLayerGroups()";
static const char pcb_acth_fontsel[] = "Let the user change fonts";
extern void lesstif_show_fontsel_edit(pcb_layer_t *txtly, pcb_text_t *txt, int type);
static int pcb_act_fontsel(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc > 1)
		PCB_ACT_FAIL(fontsel);

	if (argc > 0) {
		if (pcb_strcasecmp(argv[0], "Object") == 0) {
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_gui->get_coords(_("Select an Object"), &x, &y);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
/*				pcb_undo_save_serial();*/
				lesstif_show_fontsel_edit(ptr1, ptr2, type);
			}
		}
		else
			PCB_ACT_FAIL(fontsel);
	}
	else
		lesstif_show_fontsel_edit(NULL, NULL, 0);
	return 0;
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

/* ------------------------------------------------------------ */

static const char importgui_syntax[] = "ImportGUI()";

static const char importgui_help[] = "Lets the user choose the schematics to import from";

/* %start-doc actions ImportGUI

Displays a dialog that lets the user select the schematic(s) to import
from, then saves that information in the layout's attributes for
future imports.

%end-doc */

static int ImportGUI(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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
	rv = pcb_hid_action("Import");
	I_am_recursing = 0;

	return rv;
}

/* ------------------------------------------------------------ */

pcb_hid_action_t lesstif_dialog_action_list[] = {
	{"Load", 0, Load,
	 load_help, load_syntax}
	,
	{"LoadVendor", 0, LoadVendor,
	 loadvendor_help, loadvendor_syntax}
	,
	{"Save", 0, Save,
	 save_help, save_syntax}
	,
	{"DoWindows", 0, DoWindows,
	 dowindows_help, dowindows_syntax}
	,
	{"PromptFor", 0, PromptFor,
	 promptfor_help, promptfor_syntax}
	,
	{"Confirm", 0, ConfirmAction}
	,
	{"About", 0, About,
	 about_help, about_syntax}
	,
	{"Print", 0, Print,
	 print_help, print_syntax}
	,
	{"PrintCalibrate", 0, PrintCalibrate,
	 printcalibrate_help, printcalibrate_syntax}
	,
	{"ExportGUI", 0, ExportGUI,
	 exportgui_help, exportgui_syntax}
	,
	{"AdjustSizes", 0, AdjustSizes,
	 adjustsizes_help, adjustsizes_syntax}
	,
	{"EditLayerGroups", 0, EditLayerGroups,
	 editlayergroups_help, editlayergroups_syntax}
	,
	{"FontSel", 0, pcb_act_fontsel,
	 pcb_acth_fontsel, pcb_acts_fontsel}
	,
	{"ImportGUI", 0, ImportGUI,
	 importgui_help, importgui_syntax}
	,
};

PCB_REGISTER_ACTIONS(lesstif_dialog_action_list, lesstif_cookie)
