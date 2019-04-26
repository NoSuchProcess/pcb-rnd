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

/* Boxes and group widgets */

static GtkWidget *ghid_category_vbox(GtkWidget * box, const gchar * category_header,
															gint header_pad, gint box_pad, gboolean pack_start, gboolean bottom_pad)
{
	GtkWidget *vbox, *vbox1, *hbox, *label;
	gchar *s;

	vbox = gtkc_vbox_new(FALSE, 0);
	if (pack_start)
		gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);
	else
		gtk_box_pack_end(GTK_BOX(box), vbox, FALSE, FALSE, 0);

	if (category_header) {
		label = gtk_label_new(NULL);
		s = g_strconcat("<span weight=\"bold\">", category_header, "</span>", NULL);
		gtk_label_set_markup(GTK_LABEL(label), s);
		/*TODO: Deprecated in GTK3. Use gtk_widget_set_[h|v]align () functions ? */
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, header_pad);
		g_free(s);
	}

	hbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new("     ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	vbox1 = gtkc_vbox_new(FALSE, box_pad);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 0);

	if (bottom_pad) {
		label = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}
	return vbox1;
}


static int ghid_pane_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val)
{
	GtkWidget *pane = ctx->wl[idx];
	GtkAllocation a;
	double ratio = val->real_value;
	gint p, minp, maxp;

	if (ratio < 0.0) ratio = 0.0;
	else if (ratio > 1.0) ratio = 1.0;

	g_object_get(G_OBJECT(pane), "min-position", &minp, "max-position", &maxp, NULL);
	gtk_widget_get_allocation(pane, &a);
	switch(ctx->attrs[idx].type) {
		case PCB_HATT_BEGIN_HPANE: p = a.width; break;
		case PCB_HATT_BEGIN_VPANE: p = a.height; break;
		default: abort();
	}
	p = (double)p * ratio;
	if (p < minp) p = minp;
	if (p > maxp) p = maxp;

	gtk_paned_set_position(GTK_PANED(pane), p);
	return 0;
}

static GtkWidget *ghid_pane_append(attr_dlg_t *ctx, ghid_attr_tb_t *ts, GtkWidget *parent)
{
	GtkWidget *page = gtkc_vbox_new(FALSE, 4);
	switch(ts->val.pane.next) {
		case 1: gtk_paned_pack1(GTK_PANED(parent), page, TRUE, FALSE); break;
		case 2: gtk_paned_pack2(GTK_PANED(parent), page, TRUE, FALSE); break;
		default:
			pcb_message(PCB_MSG_ERROR, "Wrong number of pages for a paned widget (%d): must be exactly 2\n", ts->val.pane.next);
	}
	ts->val.pane.next++;
	return page;
}

static int ghid_pane_create(attr_dlg_t *ctx, int j, GtkWidget *parent, int ishor)
{
	GtkWidget *bparent, *widget;
	ghid_attr_tb_t ts;

	ts.type = TB_PANE;
	ts.val.pane.next = 1;
	ctx->wl[j] = widget = ishor ? gtkc_hpaned_new() : gtkc_vpaned_new();

	bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags, &ctx->wltop[j]);
	gtk_box_pack_start(GTK_BOX(bparent), widget, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
	j = ghid_attr_dlg_add(ctx, widget, &ts, j+1);
	return j;
}
