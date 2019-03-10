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

typedef struct ltf_tab_s ltf_tab_t;

typedef struct {
	Widget w;
	ltf_tab_t *tctx;
} ltf_tabbtn_t;

struct ltf_tab_s {
	Widget wpages;
	int len, at;
	ltf_tabbtn_t btn[1];
};

static int ltf_tabbed_set_(ltf_tab_t *tctx, int page)
{
	if ((page < 0) || (page >= tctx->len))
		return -1;

	XtVaSetValues(tctx->btn[tctx->at].w, XmNshadowThickness, 1, NULL);
	tctx->at = page;
	XtVaSetValues(tctx->btn[tctx->at].w, XmNshadowThickness, 3, NULL);
	XtVaSetValues(tctx->wpages, PxmNpagesAt, page, NULL);

	return 0;
}

static int ltf_tabbed_set(Widget tabbed, int page)
{
	if (pcb_brave & PCB_BRAVE_LESSTIF_NEWTABBED) {
		ltf_tab_t *tctx;
		XtVaGetValues(tabbed, XmNuserData, &tctx, NULL);
		return ltf_tabbed_set_(tctx, page);
	}
	else {
		XtVaSetValues(tabbed, XmNcurrentPageNumber, page+1, NULL);
	}
	return 0;
}

static void tabsw_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ltf_tabbtn_t *bctx = client_data;
	ltf_tabbed_set_(bctx->tctx, bctx - bctx->tctx->btn);
}

static void tabbed_destroy_cb(Widget tabbed, void *v, void *cbs)
{
	ltf_tab_t *tctx;
	XtVaGetValues(tabbed, XmNuserData, &tctx, NULL);
	free(tctx);
	XtVaSetValues(tabbed, XmNuserData, NULL, NULL);
}

static int ltf_tabbed_create_new(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int i)
{
	Widget wtop, wtab, wframe, t;
	int res, add_top = 0, numtabs;
	ltf_tab_t *tctx;
	const char **l;


	for(l = ctx->attrs[i].enumerations, numtabs = 0; *l != NULL; l++, numtabs++) ;
	tctx = calloc(1, sizeof(ltf_tab_t) + sizeof(ltf_tabbtn_t) * numtabs-1);
	tctx->len = numtabs;

	if (!(ctx->attrs[i].pcb_hatt_flags & PCB_HATF_HIDE_TABLAB)) {
		int n;

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
		for(n = 0, l = ctx->attrs[i].enumerations; *l != NULL; l++,n++) {
			stdarg_n = 0;
			stdarg(XmNshadowThickness, 1);
			t = XmCreatePushButton(wtab, (char *)*l, stdarg_args, stdarg_n);
			tctx->btn[n].w = t;
			tctx->btn[n].tctx = tctx;
			XtAddCallback(t, XmNactivateCallback, tabsw_cb, (XtPointer)&tctx->btn[n]);
			XtManageChild(t);
		}

		XtVaSetValues(wtop, PxmNfillBoxFill, 1, XmNuserData, tctx, NULL);
		XtVaSetValues(wtab, XmNmarginWidth, 0, XmNmarginHeight, 0, NULL);
		XtManageChild(wtop);
		XtManageChild(wtab);
		add_top = 1;
	}
	else
		wtop = parent;

	stdarg_n = 0;

	/* create and insert frame around the content table */
	stdarg_n = 0;
	stdarg(PxmNfillBoxFill, 1);
	wframe = XmCreateFrame(wtop, XmStrCast("pages-frame"), stdarg_args, stdarg_n);
	XtManageChild(wframe);

	stdarg_n = 0;
	stdarg(XmNuserData, tctx);
	tctx->wpages = PxmCreatePages(wframe, "pages", stdarg_args, stdarg_n);
	XtAddCallback(tctx->wpages, XmNunmapCallback, tabbed_destroy_cb, ctx);
	XtManageChild(tctx->wpages);

	if (add_top)
		ctx->wl[i] = wtop;
	else
		ctx->wl[i] = wframe;

	res = attribute_dialog_add(ctx, tctx->wpages, NULL, i+1, (ctx->attrs[i].pcb_hatt_flags & PCB_HATF_LABEL));

	ltf_tabbed_set_(tctx, ctx->attrs[i].default_val.int_value);
	return res;
}

static int ltf_tabbed_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int i)
{
	if (pcb_brave & PCB_BRAVE_LESSTIF_NEWTABBED)
		return ltf_tabbed_create_new(ctx, parent, attr, i);
	else
		return ltf_tabbed_create_old(ctx, parent, attr, i);
}


