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

/* Smallish misc DAD widgets */

#include "wt_preview.h"

static int ghid_progress_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	GtkWidget *prg = ctx->wl[idx];
	double pos = val->real_value;

	if (pos < 0.0) pos = 0.0;
	else if (pos > 1.0) pos = 1.0;

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prg), pos);
	return 0;
}


static GtkWidget *ghid_progress_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *prg;

	prg = gtk_progress_bar_new();
	gtk_widget_set_size_request(prg, -1, 16);

	gtk_widget_set_tooltip_text(prg, attr->help_text);
	bparent = frame_scroll(parent, attr->pcb_hatt_flags);
	gtk_box_pack_start(GTK_BOX(bparent), prg, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(prg), PCB_OBJ_PROP, ctx);
	return prg;
}

static void ghid_preview_expose(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_hid_preview_t *prv = e->content.draw_data;
	pcb_trace("preview expose in dlg_attr_misc!\n");
	prv->user_expose_cb(prv->attrib, prv, gc, e);
}

static GtkWidget *ghid_preview_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *prv;
	pcb_gtk_preview_t *p;

	bparent = frame_scroll(parent, attr->pcb_hatt_flags);
	prv = pcb_gtk_preview_generic_new(ctx->com, ctx->com->init_drawing_widget, ctx->com->preview_expose, ghid_preview_expose, attr->enumerations);
	gtk_box_pack_start(GTK_BOX(bparent), prv, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(prv, attr->help_text);
	p = (pcb_gtk_preview_t *) prv;
/*	p->mouse_cb = pcb_stub_draw_fontsel_mouse_ev;*/
/*	p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
#warning TODO make this configurable:
	gtk_widget_set_size_request(prv, 200, 200);
	return prv;
}
