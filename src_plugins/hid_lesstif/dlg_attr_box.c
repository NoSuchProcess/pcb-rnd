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

#include <Xm/PanedW.h>

static int ltf_pane_create(lesstif_attr_dlg_t *ctx, int j, Widget parent, int ishor, int add_labels)
{
	Widget pane;

	stdarg_n = 0;
	stdarg(XmNorientation, (ishor ? XmHORIZONTAL : XmVERTICAL));
	ctx->wl[j] = pane = XmCreatePanedWindow(parent, "pane", stdarg_args, stdarg_n);
	XtManageChild(pane);

	return attribute_dialog_add(ctx, pane, NULL, j+1, add_labels);
}
