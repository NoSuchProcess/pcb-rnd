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

/* Smallish misc DAD widgets */

#include "wt_preview.h"

#include "../src_plugins/lib_hid_common/dad_markup.h"

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
	pcb_hid_preview_t *prv = e->draw_data;
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
	pcb_hid_preview_t *prv = gp->expose_data.draw_data;
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
	prv = pcb_gtk_preview_new(ctx->com, ctx->com->init_drawing_widget, ctx->com->preview_expose, ghid_preview_expose, ghid_preview_config, attr->enumerations);
	gtk_box_pack_start(GTK_BOX(bparent), prv, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(prv, attr->help_text);
	p = (pcb_gtk_preview_t *) prv;
	p->mouse_cb = ghid_preview_mouse;

/*	p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
TODO("TODO make these configurable:")
	p->x_min = 0;
	p->y_min = 0;
	p->x_max = PCB_MM_TO_COORD(100);
	p->y_max = PCB_MM_TO_COORD(100);
	p->w_pixels = PCB_MM_TO_COORD(10);
	p->h_pixels = PCB_MM_TO_COORD(10);
	gtk_widget_set_size_request(prv, hp->min_sizex_px, hp->min_sizey_px);
	return prv;
}

static GtkWidget *ghid_picture_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *pic;
	GdkPixbuf *pixbuf;
	bparent = frame_scroll(parent, attr->pcb_hatt_flags);

	pixbuf = gdk_pixbuf_new_from_xpm_data(attr->enumerations);
	pic = gtk_image_new_from_pixbuf(pixbuf);
	gtk_box_pack_start(GTK_BOX(bparent), pic, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(pic, attr->help_text);

	return pic;
}


static GtkWidget *ghid_picbutton_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *button, *img;
	GdkPixbuf *pixbuf;
	bparent = frame_scroll(parent, attr->pcb_hatt_flags);

	pixbuf = gdk_pixbuf_new_from_xpm_data(attr->enumerations);
	img = gtk_image_new_from_pixbuf(pixbuf);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), img);

	gtk_box_pack_start(GTK_BOX(bparent), button, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}

static GtkWidget *ghid_color_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *button;
	pcb_gtk_color_t gclr;
	bparent = frame_scroll(parent, attr->pcb_hatt_flags);

	ctx->com->map_color_string("#000000", &gclr);

	button = gtkc_color_button_new_with_color(&gclr);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), NULL);

	gtk_box_pack_start(GTK_BOX(bparent), button, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}


static int ghid_color_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	pcb_gtk_color_t gclr;
	GtkWidget *btn = ctx->wl[idx];

	ctx->com->map_color_string(val->clr_value.str, &gclr);
	gtkc_color_button_set_color(btn, &gclr);

	return 0;
}


static void txt_changed_cb(GtkTextView *wtxt, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(wtxt), PCB_OBJ_PROP);
	pcb_hid_attribute_t *dst = user_data;
	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;
	change_cb(ctx, dst);
}

static void txt_get_xyo(pcb_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y, long *o)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextIter it;
	GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));
	GtkTextMark *m = gtk_text_buffer_get_insert(b);
	gtk_text_buffer_get_iter_at_mark(b, &it, m);
	if (y != NULL)
		*y = gtk_text_iter_get_line(&it);
	if (x != NULL)
		*x = gtk_text_iter_get_line_offset(&it);
	if (o != NULL)
		*o = gtk_text_iter_get_offset(&it);
}

static void txt_get_xy(pcb_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y)
{
	txt_get_xyo(attrib, hid_ctx, x, y, NULL);
}

static long txt_get_offs(pcb_hid_attribute_t *attrib, void *hid_ctx)
{
	long o;
	txt_get_xyo(attrib, hid_ctx, NULL, NULL, &o);
	return o;
}

static void txt_set_xyo(pcb_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y, long *o)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextIter it;
	GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));
	GtkTextMark *m = gtk_text_buffer_get_insert(b);

	gtk_text_buffer_get_iter_at_mark(b, &it, m);
	if (y != NULL)
		gtk_text_iter_set_line(&it, *y);
	if (x != NULL)
		gtk_text_iter_set_line_offset(&it, *x);
	if (o != NULL)
		gtk_text_iter_set_offset(&it, *o);
	gtk_text_buffer_place_cursor(b, &it);
}

static void txt_set_xy(pcb_hid_attribute_t *attrib, void *hid_ctx, long x, long y)
{
	txt_set_xyo(attrib, hid_ctx, &x, &y, NULL);
}

static void txt_set_offs(pcb_hid_attribute_t *attrib, void *hid_ctx, long offs)
{
	txt_set_xyo(attrib, hid_ctx, NULL, NULL, &offs);
}

static void txt_set_text_(GtkTextBuffer *b, unsigned how, const char *txt, long len)
{
	GtkTextIter it, it2;

	switch(how & 0x0F) {
		case PCB_HID_TEXT_INSERT:
			gtk_text_buffer_insert_at_cursor(b, txt, len);
			break;
		case PCB_HID_TEXT_REPLACE:
			gtk_text_buffer_get_start_iter(b, &it);
			gtk_text_buffer_get_end_iter(b, &it2);
			gtk_text_buffer_delete(b, &it, &it2);
			gtk_text_buffer_get_start_iter(b, &it);
			gtk_text_buffer_insert(b, &it, txt, len);
			break;
		case PCB_HID_TEXT_APPEND:
			gtk_text_buffer_get_end_iter(b, &it);
			gtk_text_buffer_insert(b, &it, txt, len);
			break;
	}
}

static void txt_set_text(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_text_set_t how, const char *txt)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));

	if (how & PCB_HID_TEXT_MARKUP) {
		pcb_markup_state_t st = 0;
		const char *seg;
		long seglen;
		while((seg = pcb_markup_next(&st, &txt, &seglen)) != NULL)
			txt_set_text_(b, how, seg, seglen);
	}
	else
		txt_set_text_(b, how, txt, strlen(txt));
}

static int ghid_text_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	txt_set_text(ctx->wl[idx], ctx, PCB_HID_TEXT_REPLACE, val->str_value);
	return 0;
}

static char *txt_get_text(pcb_hid_attribute_t *attrib, void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextIter it, it2;
	GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));

	gtk_text_buffer_get_start_iter(b, &it);
	gtk_text_buffer_get_end_iter(b, &it2);
	return gtk_text_buffer_get_text(b, &it, &it2, 0);
}

static void txt_set_readonly(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_bool readonly)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	gtk_text_view_set_editable(wtxt, !readonly);
}


static GtkWidget *ghid_text_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *bparent, *wtxt;
	GtkTextBuffer *buffer;
	pcb_hid_text_t *txt = (pcb_hid_text_t *)attr->enumerations;

	txt->hid_ctx = ctx;

	bparent = frame_scroll(parent, attr->pcb_hatt_flags);
	wtxt = gtk_text_view_new();
	gtk_box_pack_start(GTK_BOX(bparent), wtxt, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(wtxt, attr->help_text);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(txt_changed_cb), attr);
	g_object_set_data(G_OBJECT(buffer), PCB_OBJ_PROP, ctx);

	txt->hid_get_xy = txt_get_xy;
	txt->hid_get_offs = txt_get_offs;
	txt->hid_set_xy = txt_set_xy;
	txt->hid_set_offs = txt_set_offs;
	txt->hid_set_text = txt_set_text;
	txt->hid_get_text = txt_get_text;
	txt->hid_set_readonly = txt_set_readonly;
	return wtxt;
}
