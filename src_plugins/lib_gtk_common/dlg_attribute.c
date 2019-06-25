/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port
   then got reworked heavily by Tibor 'Igor2' Palinkas for pcb-rnd DAD */

#include "config.h"
#include "hidlib_conf.h"
#include "dlg_attribute.h"
#include <stdlib.h>

#include "pcb-printf.h"
#include "hid_attrib.h"
#include "hid_dad_tree.h"
#include "hid_init.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "event.h"

#include "compat.h"

#define PCB_OBJ_PROP "pcb-rnd_context"

typedef struct {
	void *caller_data; /* WARNING: for now, this must be the first field (see core spinbox enter_cb) */
	pcb_gtk_common_t *com;
	pcb_hid_attribute_t *attrs;
	pcb_hid_attr_val_t *results;
	GtkWidget **wl;     /* content widget */
	GtkWidget **wltop;  /* the parent widget, which is different from wl if reparenting (extra boxes, e.g. for framing or scrolling) was needed */
	int n_attrs;
	GtkWidget *dialog;
	int rc, close_cb_called;
	pcb_hid_attr_val_t property[PCB_HATP_max];
	void (*close_cb)(void *caller_data, pcb_hid_attr_ev_t ev);
	char *id;
	gulong destroy_handler;
	unsigned inhibit_valchg:1;
	unsigned freeing:1;
} attr_dlg_t;

#define change_cb(ctx, dst) \
	do { \
		if (ctx->property[PCB_HATP_GLOBAL_CALLBACK].func != NULL) \
			ctx->property[PCB_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, dst); \
		if (dst->change_cb != NULL) \
			dst->change_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

#define right_cb(ctx, dst) \
	do { \
		if (dst->right_cb != NULL) \
			dst->right_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

#define enter_cb(ctx, dst) \
	do { \
		if (dst->enter_cb != NULL) \
			dst->enter_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

static void set_flag_cb(GtkToggleButton *button, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);
	pcb_hid_attribute_t *dst = user_data;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.int_value = gtk_toggle_button_get_active(button);
	change_cb(ctx, dst);
}

static void entry_changed_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	free((char *)dst->default_val.str_value);
	dst->default_val.str_value = pcb_strdup(gtk_entry_get_text(entry));
	change_cb(ctx, dst);
}

static void entry_activate_cb(GtkEntry *entry, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), PCB_OBJ_PROP);
	enter_cb(ctx, dst);
}

static void enum_changed_cb(GtkComboBox *combo_box, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(combo_box), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->default_val.int_value = gtk_combo_box_get_active(combo_box);
	change_cb(ctx, dst);
}

static void notebook_changed_cb(GtkNotebook *nb, GtkWidget *page, guint page_num, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(nb), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	/* Gets the index (starting from 0) of the current page in the notebook. If
	   the notebook has no pages, then -1 will be returned and no call-back occur. */
	if (gtk_notebook_get_current_page(nb) >= 0) {
		dst->default_val.int_value = page_num;
		change_cb(ctx, dst);
	}
}

static void button_changed_cb(GtkButton *button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	change_cb(ctx, dst);
}

static void label_click_cb(GtkButton *evbox, GdkEvent *event, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(evbox), PCB_OBJ_PROP);

	switch(event->button.button) {
		case 1:
			dst->changed = 1;
			if (ctx->inhibit_valchg)
				return;
			change_cb(ctx, dst);
			break;
		case 3:
			right_cb(ctx, dst);
			break;
	}
}

static void color_changed_cb(GtkColorButton *button, pcb_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), PCB_OBJ_PROP);
	pcb_gtk_color_t clr;
	const char *str;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	gtkc_color_button_get_color(GTK_WIDGET(button), &clr);
	str = ctx->com->get_color_name(&clr);
	pcb_color_load_str(&dst->default_val.clr_value, str);

	change_cb(ctx, dst);
}

static GtkWidget *chk_btn_new(GtkWidget *box, gboolean active, void (*cb_func)(GtkToggleButton *, gpointer), gpointer data, const gchar *string)
{
	GtkWidget *b;

	if (string != NULL)
		b = gtk_check_button_new_with_label(string);
	else
		b = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), active);
	gtk_box_pack_start(GTK_BOX(box), b, FALSE, FALSE, 0);
	g_signal_connect(b, "clicked", G_CALLBACK(cb_func), data);
	return b;
}

typedef struct {
	void (*cb)(void *ctx, pcb_hid_attr_ev_t ev);
	void *ctx;
	attr_dlg_t *attrdlg;
} resp_ctx_t;

static GtkWidget *frame_scroll(GtkWidget *parent, pcb_hatt_compflags_t flags, GtkWidget **wltop)
{
	GtkWidget *fr;
	int expfill = (flags & PCB_HATF_EXPFILL);
	int topped = 0;

	if (flags & PCB_HATF_FRAME) {
		fr = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(parent), fr, expfill, expfill, 0);

		parent = gtkc_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(fr), parent);
		if (wltop != NULL) {
			*wltop = fr;
			topped = 1; /* remember the outmost parent */
		}
	}
	if (flags & PCB_HATF_SCROLL) {
		fr = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(parent), fr, TRUE, TRUE, 0);
		parent = gtkc_hbox_new(FALSE, 0);
		gtkc_scrolled_window_add_with_viewport(fr, parent);
		if ((wltop != NULL) && (!topped))
			*wltop = fr;
	}
	return parent;
}


typedef struct {
	enum {
		TB_TABLE,
		TB_TABBED,
		TB_PANE
	} type;
	union {
		struct {
			int cols, rows;
			int col, row;
		} table;
		struct {
			const char **tablab;
		} tabbed;
		struct {
			int next;
		} pane;
	} val;
} ghid_attr_tb_t;

/* Wrap w so that clicks on it are triggering a callback */
static GtkWidget *wrap_bind_click(GtkWidget *w, GCallback cb, void *cb_data)
{
	GtkWidget *event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), w);
	g_signal_connect(event_box, "button-press-event", G_CALLBACK(cb), cb_data);

	return event_box;
}

static int ghid_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, ghid_attr_tb_t *tb_st, int start_from);

#include "dlg_attr_tree.c"
#include "dlg_attr_misc.c"
#include "dlg_attr_txt.c"
#include "dlg_attr_box.c"

static int ghid_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, ghid_attr_tb_t *tb_st, int start_from)
{
	int j, i, expfill;
	GtkWidget *combo, *widget, *entry, *vbox1, *hbox, *bparent, *parent, *tbl;

	for (j = start_from; j < ctx->n_attrs; j++) {
		if (ctx->attrs[j].help_text == ATTR_UNDOCUMENTED)
			continue;
		if (ctx->attrs[j].type == PCB_HATT_END)
			break;

		/* if we are filling a table, allocate parent boxes in row-major */
		if (tb_st != NULL) {
			switch(tb_st->type) {
				case TB_TABLE:
					parent = gtkc_vbox_new(FALSE, 0);
					gtkc_table_attach1(real_parent, parent, tb_st->val.table.row, tb_st->val.table.col);
					tb_st->val.table.col++;
					if (tb_st->val.table.col >= tb_st->val.table.cols) {
						tb_st->val.table.col = 0;
						tb_st->val.table.row++;
					}
					break;
				case TB_TABBED:
					/* Add a new notebook page with an empty vbox, using tab label present in enumerations. */
					parent = gtkc_vbox_new(FALSE, 4);
					if (*tb_st->val.tabbed.tablab) {
						widget = gtk_label_new(*tb_st->val.tabbed.tablab);
						tb_st->val.tabbed.tablab++;
					}
					else
						widget = NULL;
					gtk_notebook_append_page(GTK_NOTEBOOK(real_parent), parent, widget);
					break;
				case TB_PANE:
					parent = ghid_pane_append(ctx, tb_st, real_parent);
					break;
			}
		}
		else
			parent = real_parent;

		expfill = (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_EXPFILL);

		/* create the actual widget from attrs */
		switch (ctx->attrs[j].type) {
			case PCB_HATT_BEGIN_HBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags, &ctx->wltop[j]);
				hbox = gtkc_hbox_new(FALSE, ((ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TIGHT) ? 0 : 4));
				gtk_box_pack_start(GTK_BOX(bparent), hbox, expfill, expfill, 0);
				ctx->wl[j] = hbox;
				j = ghid_attr_dlg_add(ctx, hbox, NULL, j+1);
				break;

			case PCB_HATT_BEGIN_VBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags, &ctx->wltop[j]);
				vbox1 = gtkc_vbox_new(FALSE, ((ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TIGHT) ? 0 : 4));
				expfill = (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_EXPFILL);
				gtk_box_pack_start(GTK_BOX(bparent), vbox1, expfill, expfill, 0);
				ctx->wl[j] = vbox1;
				j = ghid_attr_dlg_add(ctx, vbox1, NULL, j+1);
				break;

			case PCB_HATT_BEGIN_HPANE:
			case PCB_HATT_BEGIN_VPANE:
				j = ghid_pane_create(ctx, j, parent, (ctx->attrs[j].type == PCB_HATT_BEGIN_HPANE));
				break;

			case PCB_HATT_BEGIN_TABLE:
				{
					ghid_attr_tb_t ts;
					bparent = frame_scroll(parent, ctx->attrs[j].pcb_hatt_flags, &ctx->wltop[j]);
					ts.type = TB_TABLE;
					ts.val.table.cols = ctx->attrs[j].pcb_hatt_table_cols;
					ts.val.table.rows = pcb_hid_attrdlg_num_children(ctx->attrs, j+1, ctx->n_attrs) / ts.val.table.cols;
					ts.val.table.col = 0;
					ts.val.table.row = 0;
					tbl = gtkc_table_static(ts.val.table.rows, ts.val.table.cols, 1);
					gtk_box_pack_start(GTK_BOX(bparent), tbl, expfill, expfill, ((ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TIGHT) ? 0 : 4));
					ctx->wl[j] = tbl;
					j = ghid_attr_dlg_add(ctx, tbl, &ts, j+1);
				}
				break;

			case PCB_HATT_BEGIN_TABBED:
				{
					ghid_attr_tb_t ts;
					ts.type = TB_TABBED;
					ts.val.tabbed.tablab = ctx->attrs[j].enumerations;
					ctx->wl[j] = widget = gtk_notebook_new();
					gtk_notebook_set_show_tabs(GTK_NOTEBOOK(widget), !(ctx->attrs[j].pcb_hatt_flags & PCB_HATF_HIDE_TABLAB));
					if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_LEFT_TAB)
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_LEFT);
					else
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_TOP);

					gtk_box_pack_start(GTK_BOX(parent), widget, expfill, expfill, 0);
					g_signal_connect(G_OBJECT(widget), "switch-page", G_CALLBACK(notebook_changed_cb), &(ctx->attrs[j]));
					g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
					j = ghid_attr_dlg_add(ctx, widget, &ts, j+1);
				}
				break;

			case PCB_HATT_BEGIN_COMPOUND:
				j = ghid_attr_dlg_add(ctx, parent, NULL, j+1);
				break;

			case PCB_HATT_LABEL:
				if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TEXT_TRUNCATED)
					widget = gtkc_trunctext_new(ctx->attrs[j].name);
				else
					widget = gtk_label_new(ctx->attrs[j].name);
				if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TEXT_VERTICAL)
					gtk_label_set_angle(GTK_LABEL(widget), 90);

				if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TEXT_TRUNCATED) {
					if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TEXT_VERTICAL) {
						gtk_misc_set_alignment(GTK_MISC(widget), 0, 1);
						gtk_widget_set_size_request(widget, 16, 1);
					}
					else {
						gtk_misc_set_alignment(GTK_MISC(widget), 1, 0);
						gtk_widget_set_size_request(widget, 1, 16);
					}
				}

				ctx->wl[j] = widget;
				ctx->wltop[j] = wrap_bind_click(widget, G_CALLBACK(label_click_cb), &(ctx->attrs[j]));

				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				g_object_set_data(G_OBJECT(ctx->wltop[j]), PCB_OBJ_PROP, ctx);

				gtk_box_pack_start(GTK_BOX(parent), ctx->wltop[j], FALSE, FALSE, 0);
				if (!(ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TEXT_TRUNCATED))
					gtk_misc_set_alignment(GTK_MISC(widget), 0., 0.5);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				break;

			case PCB_HATT_INTEGER:
				ctx->wl[j] = gtk_label_new("ERROR: INTEGER entry");
				break;

			case PCB_HATT_COORD:
				ctx->wl[j] = gtk_label_new("ERROR: COORD entry");
				break;

			case PCB_HATT_STRING:
				ctx->wltop[j] = hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, expfill, expfill, 0);

				entry = gtk_entry_new();
				gtk_box_pack_start(GTK_BOX(hbox), entry, expfill, expfill, 0);
				g_object_set_data(G_OBJECT(entry), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].default_val.str_value != NULL)
					gtk_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].default_val.str_value);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));
				g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(entry_activate_cb), &(ctx->attrs[j]));
				break;

			case PCB_HATT_BOOL:
				/* put this in a check button */
				widget = chk_btn_new(parent, ctx->attrs[j].default_val.int_value, set_flag_cb, &(ctx->attrs[j]), NULL);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = widget;
				break;

			case PCB_HATT_ENUM:
				ctx->wltop[j] = hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, expfill, expfill, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtk_box_pack_start(GTK_BOX(hbox), combo, expfill, expfill, 0);
				g_object_set_data(G_OBJECT(combo), PCB_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				i = 0;
				while (ctx->attrs[j].enumerations[i]) {
					gtkc_combo_box_text_append_text(combo, ctx->attrs[j].enumerations[i]);
					i++;
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].default_val.int_value);
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				break;

			case PCB_HATT_TREE:
				ctx->wl[j] = ghid_tree_table_create(ctx, &ctx->attrs[j], parent, j);
				break;

			case PCB_HATT_PREVIEW:
				ctx->wl[j] = ghid_preview_create(ctx, &ctx->attrs[j], parent, j);
				break;

			case PCB_HATT_TEXT:
				ctx->wl[j] = ghid_text_create(ctx, &ctx->attrs[j], parent);
				break;

			case PCB_HATT_PICTURE:
				ctx->wl[j] = ghid_picture_create(ctx, &ctx->attrs[j], parent, j, G_CALLBACK(label_click_cb), ctx);
				break;

			case PCB_HATT_PICBUTTON:
				ctx->wl[j] = ghid_picbutton_create(ctx, &ctx->attrs[j], parent, j, (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TOGGLE), expfill);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "clicked", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), PCB_OBJ_PROP, ctx);
				break;

			case PCB_HATT_COLOR:
				ctx->wl[j] = ghid_color_create(ctx, &ctx->attrs[j], parent, j);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "color_set", G_CALLBACK(color_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), PCB_OBJ_PROP, ctx);
				break;

			case PCB_HATT_PROGRESS:
				ctx->wl[j] = ghid_progress_create(ctx, &ctx->attrs[j], parent, j);
				break;

			case PCB_HATT_UNIT:
				ctx->wl[j] = gtk_label_new("ERROR: UNIT entry");
				break;

			case PCB_HATT_BUTTON:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtk_box_pack_start(GTK_BOX(parent), hbox, expfill, expfill, 0);

				if (ctx->attrs[j].pcb_hatt_flags & PCB_HATF_TOGGLE)
					ctx->wl[j] = gtk_toggle_button_new_with_label(ctx->attrs[j].default_val.str_value);
				else
					ctx->wl[j] = gtk_button_new_with_label(ctx->attrs[j].default_val.str_value);
				gtk_box_pack_start(GTK_BOX(hbox), ctx->wl[j], expfill, expfill, 0);

				gtk_widget_set_tooltip_text(ctx->wl[j], ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "clicked", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), PCB_OBJ_PROP, ctx);
				break;

			default:
				printf("ghid_attribute_dialog: unknown type of HID attribute\n");
				break;
		}
		if (ctx->wltop[j] == NULL)
			ctx->wltop[j] = ctx->wl[j];
	}
	return j;
}

static int ghid_attr_dlg_set(attr_dlg_t *ctx, int idx, const pcb_hid_attr_val_t *val, int *copied)
{
	int ret, save;
	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;
	*copied = 0;

	/* create the actual widget from attrs */
	switch (ctx->attrs[idx].type) {
		case PCB_HATT_BEGIN_HBOX:
		case PCB_HATT_BEGIN_VBOX:
		case PCB_HATT_BEGIN_TABLE:
		case PCB_HATT_PICBUTTON:
		case PCB_HATT_PICTURE:
		case PCB_HATT_COORD:
		case PCB_HATT_REAL:
		case PCB_HATT_INTEGER:
		case PCB_HATT_BEGIN_COMPOUND:
			goto error;

		case PCB_HATT_END:
			{
				pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)ctx->attrs[idx].enumerations;
				if ((cmp != NULL) && (cmp->set_value != NULL))
					cmp->set_value(&ctx->attrs[idx], ctx, idx, val);
				else
					goto error;
			}
			break;

		case PCB_HATT_TREE:
			ret = ghid_tree_table_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_PROGRESS:
			ret = ghid_progress_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_PREVIEW:
			ret = ghid_preview_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_TEXT:
			ret = ghid_text_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_COLOR:
			ret = ghid_color_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_BEGIN_HPANE:
		case PCB_HATT_BEGIN_VPANE:
			ret = ghid_pane_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case PCB_HATT_LABEL:
			{
				const char *txt = gtk_label_get_text(GTK_LABEL(ctx->wl[idx]));
				if (strcmp(txt, val->str_value) == 0)
					goto nochg;
				gtk_label_set_text(GTK_LABEL(ctx->wl[idx]), val->str_value);
			}
			break;



		case PCB_HATT_STRING:
			{
				const char *nv, *s = gtk_entry_get_text(GTK_ENTRY(ctx->wl[idx]));
				nv = val->str_value;
				if (nv == NULL)
					nv = "";
				if (strcmp(s, nv) == 0)
					goto nochg;
				gtk_entry_set_text(GTK_ENTRY(ctx->wl[idx]), val->str_value);
				ctx->attrs[idx].default_val.str_value = pcb_strdup(val->str_value);
				*copied = 1;
			}
			break;

		case PCB_HATT_BOOL:
			{
				int chk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]));
				if (chk == val->int_value)
					goto nochg;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), val->int_value);
			}
			break;

		case PCB_HATT_ENUM:
			{
				int en = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->wl[idx]));
				if (en == val->int_value)
					goto nochg;
				gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->wl[idx]), val->int_value);
			}
			break;

		case PCB_HATT_UNIT:
			abort();
			break;

		case PCB_HATT_BUTTON:
			{
				const char *s = gtk_button_get_label(GTK_BUTTON(ctx->wl[idx]));
				if (strcmp(s, val->str_value) == 0)
					goto nochg;
				gtk_button_set_label(GTK_BUTTON(ctx->wl[idx]), val->str_value);
			}
			break;

		case PCB_HATT_BEGIN_TABBED:
			gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->wl[idx]), val->int_value);
			break;
	}

	ctx->inhibit_valchg = save;
	return 0;

	error:;
	ctx->inhibit_valchg = save;
	return -1;

	nochg:;
	ctx->inhibit_valchg = save;
	return 1;
}

static gint ghid_attr_dlg_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, gpointer data)
{
	attr_dlg_t *ctx = (attr_dlg_t *)data;
	return pcb_gtk_winplace_cfg(ctx->com->hidlib, widget, ctx, ctx->id);
}

static gint ghid_attr_dlg_destroy_event_cb(GtkWidget *widget, gpointer data)
{
	ghid_attr_dlg_free(data);
	return 0;
}

/* Hide (or show) a widget by idx - no check performed on idx range */
static int ghid_attr_dlg_widget_hide_(attr_dlg_t *ctx, int idx, pcb_bool hide)
{
	GtkWidget *w;

	if (ctx->attrs[idx].type == PCB_HATT_BEGIN_COMPOUND)
		return -1;

	if (ctx->attrs[idx].type == PCB_HATT_END) {
		pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)ctx->attrs[idx].enumerations;
		if ((cmp != NULL) && (cmp->widget_hide != NULL))
			return cmp->widget_hide(&ctx->attrs[idx], ctx, idx, hide);
		else
			return -1;
	}

	w = (ctx->wltop[idx] != NULL) ? ctx->wltop[idx] : ctx->wl[idx];
	if (w == NULL)
		return -1;

	if (hide)
		gtk_widget_hide(w);
	else
		gtk_widget_show(w);

	return 0;
}

static void ghid_initial_wstates(attr_dlg_t *ctx)
{
	int n;
	for(n = 0; n < ctx->n_attrs; n++)
		if (ctx->attrs[n].pcb_hatt_flags & PCB_HATF_HIDE)
			ghid_attr_dlg_widget_hide_(ctx, n, 1);
}

void *ghid_attr_dlg_new(pcb_gtk_common_t *com, const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	GtkWidget *content_area;
	GtkWidget *main_vbox;
	attr_dlg_t *ctx;
	int plc[4] = {-1, -1, -1, -1};


	plc[2] = defx;
	plc[3] = defy;

	ctx = calloc(sizeof(attr_dlg_t), 1);

	ctx->com = com;
	ctx->attrs = attrs;
	ctx->results = results;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->wltop = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;
	ctx->rc = 1; /* just in case the window is destroyed in an unknown way: take it as cancel */
	ctx->close_cb_called = 0;
	ctx->close_cb = button_cb;
	ctx->id = pcb_strdup(id);

	pcb_event(com->hidlib, PCB_EVENT_DAD_NEW_DIALOG, "psp", ctx, ctx->id, plc);

	ctx->dialog = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(ctx->dialog), GTK_WINDOW(com->top_window));

	gtk_window_set_title(GTK_WINDOW(ctx->dialog), title);
	gtk_window_set_role(GTK_WINDOW(ctx->dialog), id);
	gtk_window_set_modal(GTK_WINDOW(ctx->dialog), modal);

	if (pcbhl_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0))
			gtk_window_resize(GTK_WINDOW(ctx->dialog), plc[2], plc[3]);
		if ((plc[0] >= 0) && (plc[1] >= 0))
			gtk_window_move(GTK_WINDOW(ctx->dialog), plc[0], plc[1]);
	}
	else if ((defx > 0) && (defy > 0))
		gtk_window_resize(GTK_WINDOW(ctx->dialog), defx, defy);

	g_signal_connect(ctx->dialog, "configure_event", G_CALLBACK(ghid_attr_dlg_configure_event_cb), ctx);
	ctx->destroy_handler = g_signal_connect(ctx->dialog, "destroy", G_CALLBACK(ghid_attr_dlg_destroy_event_cb), ctx);

	main_vbox = gtkc_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 6);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
	gtk_container_add_with_properties(GTK_CONTAINER(content_area), main_vbox, "expand", TRUE, "fill", TRUE, NULL);

	ghid_attr_dlg_add(ctx, main_vbox, NULL, 0);

	gtk_widget_show_all(ctx->dialog);

	ghid_initial_wstates(ctx);
	return ctx;
}

void *ghid_attr_sub_new(pcb_gtk_common_t *com, GtkWidget *parent_box, pcb_hid_attribute_t *attrs, int n_attrs, void *caller_data)
{
	attr_dlg_t *ctx;

	ctx = calloc(sizeof(attr_dlg_t), 1);

	ctx->com = com;
	ctx->attrs = attrs;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->wltop = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;
	ctx->rc = 1; /* just in case the window is destroyed in an unknown way: take it as cancel */

	ghid_attr_dlg_add(ctx, parent_box, NULL, 0);

	gtk_widget_show_all(parent_box);

	ghid_initial_wstates(ctx);

	return ctx;
}

int ghid_attr_dlg_run(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	GtkResponseType res = gtk_dialog_run(GTK_DIALOG(ctx->dialog));
	if (res == GTK_RESPONSE_NONE) {
		/* the close cb destroyed the window; rc is already set */
	}
	else if (res == GTK_RESPONSE_OK)
		ctx->rc = 0;
	else
		ctx->rc = 1;
	return ctx->rc;
}

void ghid_attr_dlg_raise(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	gtk_window_present(GTK_WINDOW(ctx->dialog));
}

void ghid_attr_dlg_free(void *hid_ctx)
{
	int i;
	attr_dlg_t *ctx = hid_ctx;

	/* make sure there are no nested ghid_attr_dlg_free() calls */
	if (ctx->freeing)
		return;
	ctx->freeing = 1;

	/* make sure we are not called again from the destroy signal */
	g_signal_handler_disconnect(ctx->dialog, ctx->destroy_handler);

	if (!ctx->close_cb_called) {
		ctx->close_cb_called = 1;
		if (ctx->close_cb != NULL)
			ctx->close_cb(ctx->caller_data, PCB_HID_ATTR_EV_CODECLOSE);
	}

	for(i = 0; i < ctx->n_attrs; i++) {
		switch(ctx->attrs[i].type) {
			case PCB_HATT_TREE: ghid_tree_pre_free(ctx, &ctx->attrs[i], i); break;
			case PCB_HATT_BUTTON: g_signal_handlers_block_by_func(G_OBJECT(ctx->wl[i]), G_CALLBACK(button_changed_cb), &(ctx->attrs[i])); break;
			default: break;
		}
	}

	if (ctx->rc == 0) { /* copy over the results */
		for (i = 0; i < ctx->n_attrs; i++) {
			ctx->results[i] = ctx->attrs[i].default_val;
			if (PCB_HAT_IS_STR(ctx->attrs[i].type) && (ctx->results[i].str_value))
				ctx->results[i].str_value = pcb_strdup(ctx->results[i].str_value);
			else
				ctx->results[i].str_value = NULL;
		}
	}

	if (ctx->dialog != NULL)
		gtk_widget_destroy(ctx->dialog);
	free(ctx->id);
	free(ctx->wl);
	free(ctx->wltop);
	ctx->id = NULL;
	ctx->wl = NULL;
	ctx->wltop = NULL;
	ctx->dialog = NULL;
	TODO("#51: free(ctx);");
}

void ghid_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((prop >= 0) && (prop < PCB_HATP_max))
		ctx->property[prop] = *val;
}

int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (ctx->attrs[idx].type == PCB_HATT_BEGIN_COMPOUND)
		return -1;

	if (ctx->attrs[idx].type == PCB_HATT_END) {
		pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)ctx->attrs[idx].enumerations;
		if ((cmp != NULL) && (cmp->widget_state != NULL))
			cmp->widget_state(&ctx->attrs[idx], ctx, idx, enabled);
		else
			return -1;
	}

	gtk_widget_set_sensitive(ctx->wl[idx], enabled);

	switch(ctx->attrs[idx].type) {
		case PCB_HATT_LABEL:
			pcb_gtk_set_selected(ctx->wltop[idx], (enabled == 2));
			break;
		case PCB_HATT_PICBUTTON:
		case PCB_HATT_BUTTON:
			if (ctx->attrs[idx].pcb_hatt_flags & PCB_HATF_TOGGLE)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), (enabled == 2));
			break;
		default:;
	}

	return 0;
}


int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	return ghid_attr_dlg_widget_hide_(ctx, idx, hide);
}

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;
	int res, copied;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	res = ghid_attr_dlg_set(ctx, idx, val, &copied);

	if (res == 0) {
		if (!copied)
			ctx->attrs[idx].default_val = *val;
		return 0;
	}
	else if (res == 1)
		return 0; /* no change */

	return -1;
}

void ghid_attr_dlg_set_help(void *hid_ctx, int idx, const char *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return;

	gtk_widget_set_tooltip_text(ctx->wl[idx], val);
}

void pcb_gtk_dad_fixcolor(void *hid_ctx, const GdkColor *color)
{
	attr_dlg_t *ctx = hid_ctx;

	int n;
	for(n = 0; n < ctx->n_attrs; n++) {
		switch(ctx->attrs[n].type) {
			case PCB_HATT_BUTTON:
			case PCB_HATT_LABEL:
			case PCB_HATT_PICTURE:
				gtk_widget_modify_bg(ctx->wltop[n], GTK_STATE_NORMAL, color);
			default:;
		}
	}
}

int pcb_gtk_winplace_cfg(pcb_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);

	/* For whatever reason, get_allocation doesn't set these. Gtk. */
	gtk_window_get_position(GTK_WINDOW(widget), &allocation.x, &allocation.y);

	pcb_event(hidlib, PCB_EVENT_DAD_NEW_GEO, "psiiii", ctx, id,
		(int)allocation.x, (int)allocation.y, (int)allocation.width, (int)allocation.height);

	return 0;
}
