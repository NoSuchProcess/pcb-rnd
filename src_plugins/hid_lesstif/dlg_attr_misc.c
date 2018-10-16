/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Boxes and group widgets */

#define PB_SCALE_UP 10000.0

#include <Xm/PanedW.h>

static void ltf_progress_set(lesstif_attr_dlg_t *ctx, int idx, double val)
{
	Widget bar = ctx->wl[idx];

	if (val < 0.0) val = 0.0;
	else if (val > 1.0) val = 1.0;
	
	stdarg_n = 0;
	stdarg(XmNsliderSize, (val * PB_SCALE_UP)+1);
	XtSetValues(bar, stdarg_args, stdarg_n);
}

static Widget ltf_progress_create(lesstif_attr_dlg_t *ctx, Widget parent)
{
	Widget bar;

	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNmaximum, (int)PB_SCALE_UP+1);
	stdarg(XmNsliderSize, 1);
	stdarg(XmNorientation, XmHORIZONTAL);
	stdarg(XmNshowArrows, pcb_false);
	stdarg_do_color("#000099", XmNforeground);
	stdarg(XmNsliderVisual, XmFOREGROUND_COLOR);
	
	bar = XmCreateScrollBar(parent, XmStrCast("scale"), stdarg_args, stdarg_n);
	XtManageChild(bar);
	return bar;
}


#include "wt_preview.h"

/* called back from core (which is called from wt_preview) to get the user
   expose function called */
static void ltf_preview_expose(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_ltf_preview_t *pd = e->content.draw_data;
	pcb_hid_attribute_t *attr = pd->attr;
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;
printf("user exp!\n");
	prv->user_expose_cb(attr, prv, gc, e);
}

static Widget ltf_preview_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget pw;
	pcb_ltf_preview_t *pd;

	pd = calloc(1, sizeof(pcb_ltf_preview_t));
	pd->attr = attr;
	pd->hid_ctx = ctx;
	memset(&pd->exp_ctx, 0, sizeof(pd->exp_ctx));
	pd->exp_ctx.content.draw_data = pd;
	pd->exp_ctx.dialog_draw = ltf_preview_expose;
	pd->exp_ctx.force = 1;

	pd->resized = 0;
#warning TODO make these configurable:
	pd->left = 0;
	pd->top = 0;
	pd->right = PCB_MM_TO_COORD(100);
	pd->bottom = PCB_MM_TO_COORD(100);

	stdarg_n = 0;
	stdarg(XmNwidth, 200);
	stdarg(XmNheight, 200);
	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	pw = XmCreateDrawingArea(parent, XmStrCast("dad_preview"), stdarg_args, stdarg_n);
	XtManageChild(pw);

	XtAddCallback(pw, XmNexposeCallback, (XtCallbackProc)pcb_ltf_preview_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNresizeCallback, (XtCallbackProc)pcb_ltf_preview_callback, (XtPointer)pd);

	XtManageChild(pw);
	return pw;
}

