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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
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
	prv->user_expose_cb(prv->attrib, prv, gc, e);
}

static pcb_bool ghid_preview_mouse(void *widget, void *draw_data, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	pcb_hid_preview_t *prv = draw_data;
	if (prv->user_mouse_cb != NULL)
		return prv->user_mouse_cb(prv->attrib, prv, kind, x, y);
	return pcb_false;
}

void ghid_preview_zoomto(pcb_hid_attribute_t *attrib, void *hid_ctx, const pcb_box_t *view)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *prv = ctx->wl[idx];
	pcb_gtk_preview_zoomto(PCB_GTK_PREVIEW(prv), view);
	gtk_widget_queue_draw(prv);
}

static int ghid_preview_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	GtkWidget *prv = ctx->wl[idx];

	gtk_widget_queue_draw(prv);
	return 0;
}


void ghid_preview_config(pcb_gtk_preview_t *gp, GtkWidget *widget)
{
	pcb_hid_preview_t *prv = gp->expose_data.content.draw_data;
	if (prv->initial_view_valid) {
		pcb_gtk_preview_zoomto(PCB_GTK_PREVIEW(widget), &prv->initial_view);
		gtk_widget_queue_draw(widget);
		prv->initial_view_valid = 0;
	}
}

static GtkWidget *ghid_preview_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *prv;
	pcb_gtk_preview_t *p;
	pcb_hid_preview_t *hp = (pcb_hid_preview_t *)attr->enumerations;

	hp->hid_ctx = ctx;
	hp->hid_zoomto_cb = ghid_preview_zoomto;

	bparent = frame_scroll(parent, attr->pcb_hatt_flags);
	prv = pcb_gtk_preview_generic_new(ctx->com, ctx->com->init_drawing_widget, ctx->com->preview_expose, ghid_preview_expose, ghid_preview_config, attr->enumerations);
	gtk_box_pack_start(GTK_BOX(bparent), prv, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(prv, attr->help_text);
	p = (pcb_gtk_preview_t *) prv;
	p->mouse_cb = ghid_preview_mouse;

/*	p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
#warning TODO make these configurable:
	p->x_min = 0;
	p->y_min = 0;
	p->x_max = PCB_MM_TO_COORD(100);
	p->y_max = PCB_MM_TO_COORD(100);
	p->w_pixels = PCB_MM_TO_COORD(10);
	p->h_pixels = PCB_MM_TO_COORD(10);
	gtk_widget_set_size_request(prv, hp->min_sizex_px, hp->min_sizey_px);
	return prv;
}
