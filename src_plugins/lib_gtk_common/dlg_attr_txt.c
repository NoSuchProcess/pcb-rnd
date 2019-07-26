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

typedef struct {
	attr_dlg_t *hid_ctx;
	int attr_idx;
	unsigned markup_inited:1;
} dad_txt_t;

static void txt_free_cb(pcb_hid_attribute_t *attr, void *hid_ctx)
{
	dad_txt_t *tctx = hid_ctx;
	free(tctx);
}

static void txt_changed_cb(GtkTextView *wtxt, gpointer user_data)
{
	dad_txt_t *tctx = user_data;
	pcb_hid_attribute_t *attr = &tctx->hid_ctx->attrs[tctx->attr_idx];
	attr->changed = 1;
	if (tctx->hid_ctx->inhibit_valchg)
		return;
	change_cb(tctx->hid_ctx, attr);
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

static void txt_set_text(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_text_set_t how, const char *str)
{
	attr_dlg_t *ctx = hid_ctx;
	pcb_hid_text_t *txt = attrib->wdata;
	dad_txt_t *tctx = txt->hid_wdata;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));

	if (how & PCB_HID_TEXT_MARKUP) {
		pcb_markup_state_t st = 0;
		const char *seg;
		long seglen;

		if (!tctx->markup_inited) {
			gtk_text_buffer_create_tag(b, "italic", "style", PANGO_STYLE_ITALIC, NULL);
			gtk_text_buffer_create_tag(b, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
			gtk_text_buffer_create_tag(b, "red", "foreground", "#aa0000", NULL);
			gtk_text_buffer_create_tag(b, "green", "foreground", "#00aa00", NULL);
			gtk_text_buffer_create_tag(b, "blue", "foreground", "#0000aa", NULL);
			tctx->markup_inited = 1;
		}
		while((seg = pcb_markup_next(&st, &str, &seglen)) != NULL) {
			GtkTextIter it1, it2;
			GtkTextMark *m;
			const char *tag;
			long o1;

			m = gtk_text_buffer_get_insert(b);
			gtk_text_buffer_get_iter_at_mark(b, &it1, m);
			o1 = gtk_text_iter_get_offset(&it1);
			txt_set_text_(b, how, seg, seglen);
			if (st != 0) {
				if (st & PCB_MKS_RED) tag = "red";
				if (st & PCB_MKS_GREEN) tag = "green";
				if (st & PCB_MKS_BLUE) tag = "blue";
				if (st & PCB_MKS_BOLD) tag = "bold";
				if (st & PCB_MKS_ITALIC) tag = "italic";
				m = gtk_text_buffer_get_insert(b);
				gtk_text_buffer_get_iter_at_mark(b, &it2, m);
				gtk_text_buffer_get_iter_at_mark(b, &it1, m);
				gtk_text_iter_set_offset(&it1, o1);
				gtk_text_buffer_apply_tag_by_name(b, tag, &it1, &it2);
			}
		}
	}
	else
		txt_set_text_(b, how, str, strlen(str));
}

static int ghid_text_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	txt_set_text(&ctx->attrs[idx], ctx, PCB_HID_TEXT_REPLACE, val->str);
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
	gtk_text_view_set_editable(GTK_TEXT_VIEW(wtxt), !readonly);
}

static void txt_scroll_to_bottom(pcb_hid_attribute_t *attrib, void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *wtxt = ctx->wl[idx];
	GtkTextIter iter;
	GtkTextBuffer *buffer;
	GtkTextMark *mark;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));
	gtk_text_buffer_get_end_iter(buffer, &iter);

	mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(wtxt), mark, 0, TRUE, 0.0, 1.0);
	gtk_text_buffer_delete_mark(buffer, mark);
}

static GtkWidget *ghid_text_create(attr_dlg_t *ctx, pcb_hid_attribute_t *attr, GtkWidget *parent)
{
	GtkWidget *wtxt;
	GtkTextBuffer *buffer;
	pcb_hid_text_t *txt = attr->wdata;
	dad_txt_t *tctx;


	if (attr->pcb_hatt_flags & PCB_HATF_SCROLL) {
		GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(parent), scrolled, TRUE, TRUE, 0);
		wtxt = gtk_text_view_new();
		gtk_container_add(GTK_CONTAINER(scrolled), wtxt);
	}
	else {
		wtxt = gtk_text_view_new();
		gtk_box_pack_start(GTK_BOX(parent), wtxt, TRUE, TRUE, 0);
	}

	gtk_widget_set_tooltip_text(wtxt, attr->help_text);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wtxt));

	tctx = malloc(sizeof(dad_txt_t));
	tctx->hid_ctx = ctx;
	tctx->attr_idx = attr - ctx->attrs;
	tctx->markup_inited = 0;
	txt->hid_wdata = tctx;
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(txt_changed_cb), tctx);

	txt->hid_get_xy = txt_get_xy;
	txt->hid_get_offs = txt_get_offs;
	txt->hid_set_xy = txt_set_xy;
	txt->hid_set_offs = txt_set_offs;
	txt->hid_scroll_to_bottom = txt_scroll_to_bottom;
	txt->hid_set_text = txt_set_text;
	txt->hid_get_text = txt_get_text;
	txt->hid_set_readonly = txt_set_readonly;
	txt->hid_free_cb = txt_free_cb;
	return wtxt;
}
