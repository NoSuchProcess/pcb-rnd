/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

/* Boxes and group widgets */

#include <Xm/PanedW.h>
#include "Pages.h"
#include "brave.h"

static int ltf_pane_create(lesstif_attr_dlg_t *ctx, int j, Widget parent, int ishor, int add_labels)
{
	Widget pane;

	stdarg_n = 0;
	stdarg(XmNorientation, (ishor ? XmHORIZONTAL : XmVERTICAL));
	ctx->wl[j] = pane = XmCreatePanedWindow(parent, "pane", stdarg_args, stdarg_n);
	XtManageChild(pane);

	return attribute_dialog_add(ctx, pane, NULL, j+1, add_labels);
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
			stdarg(PxmNfillBoxVertical, 0);
			cnt = PxmCreateFillBox(parent, name, stdarg_args, stdarg_n);
			break;
		case 'v': /* "vbox" */
			stdarg(PxmNfillBoxVertical, 1);
			cnt = PxmCreateFillBox(parent, name, stdarg_args, stdarg_n);
			break;
		case 't': /* "table" */
			stdarg(XmNorientation, XmHORIZONTAL);
			stdarg(XmNpacking, XmPACK_COLUMN);
			stdarg(XmNnumColumns, num_table_rows);
			stdarg(XmNisAligned, True);
			stdarg(XmNentryAlignment, XmALIGNMENT_END);
			cnt = XmCreateRowColumn(parent, name, stdarg_args, stdarg_n);
			break;
		default:
			abort();
	}
	return cnt;
}

static int ltf_tabbed_create_old(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int i)
{
	Widget w, scroller;
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

	stdarg(XmNuserData, ctx);
	stdarg(PxmNfillBoxFill, 1);
	ctx->wl[i] = w = XmCreateNotebook(parent, "notebook", stdarg_args, stdarg_n);

	/* remove the page scroller widget that got automatically created by XmCreateNotebook() */
	scroller = XtNameToWidget(w, "PageScroller");
	XtUnmanageChild (scroller);

	XtAddCallback(w, XmNpageChangedCallback, (XtCallbackProc)pagechg, (XtPointer)&ctx->attrs[i]);

	tb.notebook = w;
	tb.tablab = ctx->attrs[i].enumerations;
	tb.minw = 0;
	tb.tabs = 0;

	ctx->wl[i] = w;
	i = attribute_dialog_add(ctx, w, &tb, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));

	XtManageChild(w);

	return i;
}

static int ltf_tabbed_create_new(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int i)
{
	Widget wpages, wtop, wtab, wframe, t;
	int add_top = 0;

	if (!(ctx->attrs[i].pcb_hatt_flags & PCB_HATF_HIDE_TABLAB)) {
		const char **l;

		/* create the boxing for the labels */
		if (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LEFT_TAB) {
			wtop = pcb_motif_box(parent, "tabbed_top", 'h', 0, 0, 0);
			wtab = pcb_motif_box(wtop, "tabbed_tabs", 'v', 0, 0, 0);
		}
		else {
			wtop = pcb_motif_box(parent, "tabbed_top", 'v', 0, 0, 0);
			wtab = pcb_motif_box(wtop, "tabbed_tabs", 'h', 0, 0, 0);
		}

		/* create the label buttons */
		for(l = ctx->attrs[i].enumerations; *l != NULL; l++) {
			stdarg_n = 0;
			t = XmCreatePushButton(wtab, (char *)*l, stdarg_args, stdarg_n);
			XtManageChild(t);
		}
		XtManageChild(wtop);
		XtManageChild(wtab);
		add_top = 1;
	}
	else
		wtop = parent;

	stdarg_n = 0;

	/* create and insert frame around the content table */
	stdarg_n = 0;
	wframe = XmCreateFrame(wtop, XmStrCast("pages-frame"), stdarg_args, stdarg_n);
	XtManageChild(wframe);

	stdarg_n = 0;
	wpages = PxmCreatePages(wframe, "pages", stdarg_args, stdarg_n);
	XtManageChild(wpages);

	if (add_top)
		ctx->wl[i] = wtop;
	else
		ctx->wl[i] = wframe;

	return attribute_dialog_add(ctx, wpages, NULL, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));
}

static int ltf_tabbed_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int i)
{
	if (pcb_brave & PCB_BRAVE_LESSTIF_NEWTABBED)
		return ltf_tabbed_create_new(ctx, parent, attr, i);
	else
		return ltf_tabbed_create_old(ctx, parent, attr, i);
}

