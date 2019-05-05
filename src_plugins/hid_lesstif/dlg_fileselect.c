/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "xincludes.h"
#include "lesstif.h"
#include "ltf_stdarg.h"

#include "hid.h"
#include "hid_dad.h"
#include "event.h"

#include "dlg_fileselect.h"

typedef struct {
	Widget dialog;
	int active;
	void *hid_ctx; /* DAD subdialog context */
} pcb_ltf_fsd_t;

static void fsb_ok_value(Widget w, void *v, void *cbs)
{
	pcb_ltf_ok = (int)(size_t)v;
}

static char *pcb_ltf_get_path(pcb_ltf_fsd_t *pctx)
{
	char *res, *name;
	XmString xmname;

	stdarg_n = 0;
	stdarg(XmNdirSpec, &xmname);
	XtGetValues(pctx->dialog, stdarg_args, stdarg_n);

	XmStringGetLtoR(xmname, XmFONTLIST_DEFAULT_TAG, &name);
	res = pcb_strdup(name);
	XtFree(name);
	return res;
}

static void pcb_ltf_set_fn(pcb_ltf_fsd_t *pctx, int append, const char *fn)
{
	XmString xms_path;

	if ((*fn != PCB_DIR_SEPARATOR_C) && (append)) {
		char *end, *path, *dir = pcb_ltf_get_path(pctx);

		end = strrchr(dir, PCB_DIR_SEPARATOR_C);
		if (end == NULL) {
			path = pcb_concat(dir, "/", fn, NULL);
		}
		else if (end[1] == '\0') {
			/* dir is a directory, ending in /, append fn */
			path = pcb_concat(dir, fn, NULL);
		}
		else {
			/* dir is a full path, with file name included, replace that */
			end[1] = '\0';
			path = pcb_concat(dir, fn, NULL);
		}

		xms_path = XmStringCreatePCB(path);
		stdarg_n = 0;
		stdarg(XmNdirSpec, xms_path);
		XtSetValues(pctx->dialog, stdarg_args, stdarg_n);

		XmStringFree(xms_path);
		free(path);
		free(dir);
		return;
	}

	xms_path = XmStringCreatePCB(fn);
	stdarg_n = 0;
	stdarg(XmNdirSpec, xms_path);
	XtSetValues(pctx->dialog, stdarg_args, stdarg_n);
	XmStringFree(xms_path);
}

static int pcb_ltf_fsd_poke(pcb_hid_dad_subdialog_t *sub, const char *cmd, pcb_event_arg_t *res, int argc, pcb_event_arg_t *argv)
{
	pcb_ltf_fsd_t *pctx = sub->parent_ctx;

	if (strcmp(cmd, "close") == 0) {
		if (pctx->active) {
			pctx->active = 0;
			XtDestroyWidget(pctx->dialog);
		}
		return 0;
	}

	if (strcmp(cmd, "get_path") == 0) {
		res->type = PCB_EVARG_STR;
		res->d.s = pcb_ltf_get_path(pctx);
		return 0;
	}

	if ((strcmp(cmd, "set_file_name") == 0) && (argc == 1) && (argv[0].type == PCB_EVARG_STR)) {
		pcb_ltf_set_fn(pctx, 1, argv[0].d.s);
		return 0;
	}

	return -1;
}

char *pcb_ltf_fileselect(const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub)
{
	XmString xms_ext = NULL, xms_load = NULL;
	pcb_ltf_fsd_t pctx;
	char *res;

	stdarg_n = 0;
	pctx.dialog = XmCreateFileSelectionDialog(mainwind, XmStrCast("file"), stdarg_args, stdarg_n);

	XtAddCallback(pctx.dialog, XmNokCallback, (XtCallbackProc)fsb_ok_value, (XtPointer)1);
	XtAddCallback(pctx.dialog, XmNcancelCallback, (XtCallbackProc)fsb_ok_value, (XtPointer)0);

	if (sub != NULL) {
		Widget subbox;
		stdarg_n = 0;
		stdarg(XmNorientation, XmVERTICAL);
		stdarg(XmNpacking, XmPACK_COLUMN);
		subbox = XmCreateRowColumn(pctx.dialog, "extra", stdarg_args, stdarg_n);

		sub->parent_ctx = &pctx;
		sub->parent_poke = pcb_ltf_fsd_poke;

		pctx.hid_ctx = lesstif_attr_sub_new(subbox, sub->dlg, sub->dlg_len, sub);
		XtManageChild(subbox);
	}


	stdarg_n = 0;
	stdarg(XmNtitle, title);
	XtSetValues(XtParent(pctx.dialog), stdarg_args, stdarg_n);

	if (flags & PCB_HID_FSD_READ) {
		xms_load = XmStringCreatePCB("Load From");
		stdarg_n = 0;
		stdarg(XmNselectionLabelString, xms_load);
		XtSetValues(pctx.dialog, stdarg_args, stdarg_n);
	}

	if (default_ext != NULL) {
		xms_ext = XmStringCreatePCB(default_ext);
		stdarg_n = 0;
		stdarg(XmNpattern, xms_ext);
		stdarg(XmNmustMatch, True);
		XtSetValues(pctx.dialog, stdarg_args, stdarg_n);
	}

	if (default_file != 0)
		pcb_ltf_set_fn(&pctx, 1, default_file);

	if (pcb_ltf_wait_for_dialog(pctx.dialog))
		res = pcb_ltf_get_path(&pctx);
	else
		res = NULL;

	if (xms_load != NULL) XmStringFree(xms_load);
	if (xms_ext != NULL) XmStringFree(xms_ext);
	return res;
}

