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

void pcb_ltf_preview_callback(Widget da, pcb_ltf_preview_t *pd, XmDrawingAreaCallbackStruct *cbs)
{
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)pd->attr->enumerations;
	int save_vx, save_vy, save_vw, save_vh;
	int save_fx, save_fy;
	double save_vz;
	Pixmap save_px;
	int reason = cbs != NULL ? cbs->reason : 0;
	pcb_hid_expose_ctx_t ex;

	if ((reason == XmCR_RESIZE) || (pd->resized == 0)) {
		Dimension w, h;
		double z;

		pd->resized = 1;
		pd->window = XtWindow(da);
		stdarg_n = 0;
		stdarg(XmNwidth, &w);
		stdarg(XmNheight, &h);
		XtGetValues(da, stdarg_args, stdarg_n);

		pd->v_width = w;
		pd->v_height = h;
		pd->zoom = (pd->right - pd->left + 1) / (double) w;
		z = (pd->bottom - pd->top + 1) / (double) h;
		if (pd->zoom < z)
			pd->zoom = z;

		pd->x = (pd->left + pd->right) / 2 - pd->v_width * pd->zoom / 2;
		pd->y = (pd->top + pd->bottom) / 2 - pd->v_height * pd->zoom / 2;
	}

	save_vx = view_left_x;
	save_vy = view_top_y;
	save_vz = view_zoom;
	save_vw = view_width;
	save_vh = view_height;
	save_fx = conf_core.editor.view.flip_x;
	save_fy = conf_core.editor.view.flip_y;
	save_px = main_pixmap;
	main_pixmap = pd->window;
	view_left_x = pd->x;
	view_top_y = pd->y;
	view_zoom = pd->zoom;
	view_width = pd->v_width;
	view_height = pd->v_height;
	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);

	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height);

/*	prv->user_expose_cb(pd->attr, pd->hid_ctx, my_gc, &ex);*/
	ex.view.X1 = view_left_x;
	ex.view.Y1 = view_top_y;
	ex.view.X2 = view_left_x + pd->v_width;
	ex.view.Y2 = view_top_y + pd->v_height;

	pcb_hid_expose_generic(&lesstif_hid, &ex);

	view_left_x = save_vx;
	view_top_y = save_vy;
	view_zoom = save_vz;
	view_width = save_vw;
	view_height = save_vh;
	main_pixmap = save_px;
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy);
}
