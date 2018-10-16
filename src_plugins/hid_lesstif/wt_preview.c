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

#include "wt_preview.h"
#include "hid_attrib.h"

static int widget_depth(Widget w);

void pcb_ltf_preview_zoom_update(pcb_ltf_preview_t *pd)
{
	double z;
	Dimension w, h;

	pd->resized = 1;
	stdarg_n = 0;
	stdarg(XmNwidth, &w);
	stdarg(XmNheight, &h);
	XtGetValues(pd->pw, stdarg_args, stdarg_n);
	pd->v_width = w;
	pd->v_height = h;

	pd->zoom = (pd->x2 - pd->x1 + 1) / (double)pd->v_width;
pcb_printf("zoomx: %ld %f %f\n", (pd->x2 - pd->x1 + 1), (double)pd->v_width, pd->zoom);
	z = (pd->y2 - pd->y1 + 1) / (double)pd->v_height;
	if (pd->zoom < z)
		pd->zoom = z;

pcb_printf("zoomy: %ld %f %f\n", (pd->y2 - pd->y1 + 1), (double)pd->v_height, pd->zoom);

	pd->x = (pd->x1 + pd->x2) / 2 - pd->v_width * pd->zoom / 2;
	pd->y = (pd->y1 + pd->y2) / 2 - pd->v_height * pd->zoom / 2;
}

void pcb_ltf_preview_callback(Widget da, pcb_ltf_preview_t *pd, XmDrawingAreaCallbackStruct *cbs)
{
	int save_vx, save_vy, save_vw, save_vh;
	int save_fx, save_fy;
	double save_vz;
	Pixmap save_px, save_main_px, save_mask_px, save_mask_bm;
	int reason = cbs != NULL ? cbs->reason : 0;
	XGCValues gcv;
	GC gc;

	memset(&gcv, 0, sizeof(gcv));
	gcv.graphics_exposures = 0;
	gc = XtGetGC(da, GCGraphicsExposures, &gcv);

	if ((reason == XmCR_RESIZE) || (pd->resized == 0)) {

		pcb_ltf_preview_zoom_update(pd);
	}

	pinout = 0;
	save_vx = view_left_x;
	save_vy = view_top_y;
	save_vz = view_zoom;
	save_vw = view_width;
	save_vh = view_height;
	save_fx = conf_core.editor.view.flip_x;
	save_fy = conf_core.editor.view.flip_y;
	save_px = main_pixmap;
	save_main_px = main_pixmap;
	save_mask_px = mask_pixmap;
	save_mask_bm = mask_bitmap;
	main_pixmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, widget_depth(da));
	mask_pixmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, widget_depth(da));
	mask_bitmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, 1);
	pixmap = main_pixmap;
	view_left_x = pd->x1;
	view_top_y = pd->y1;
	view_zoom = pd->zoom;
	view_width = pd->x2;
	view_height = pd->y2;
	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);

	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height);

	pd->exp_ctx.view.X1 = pd->x1;
	pd->exp_ctx.view.Y1 = pd->y1;
	pd->exp_ctx.view.X2 = pd->x2;
	pd->exp_ctx.view.Y2 = pd->y2;

pcb_trace("exp: %mm;%mm %mm;%mm\n", pd->exp_ctx.view.X1, pd->exp_ctx.view.Y1, pd->exp_ctx.view.X2, pd->exp_ctx.view.Y2);

	pcb_hid_expose_generic(&lesstif_hid, &pd->exp_ctx);

	XCopyArea(lesstif_display, pixmap, XtWindow(da), gc, 0, 0, pd->v_width, pd->v_height, 0, 0);
	XtReleaseGC(da, gc);

	view_left_x = save_vx;
	view_top_y = save_vy;
	view_zoom = save_vz;
	view_width = save_vw;
	view_height = save_vh;
	XFreePixmap(lesstif_display, main_pixmap);
	XFreePixmap(lesstif_display, mask_pixmap);
	XFreePixmap(lesstif_display, mask_bitmap);
	main_pixmap = save_main_px;
	mask_pixmap = save_mask_px;
	mask_bitmap = save_mask_bm;
	pixmap = save_px;
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy);
}
