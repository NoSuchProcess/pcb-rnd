/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* advanced search dialog */


#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <genlist/gendlist.h>
#include "gui.h"
#include "create.h"
#include "compat_misc.h"
#include "ghid-search.h"
#include "win_place.h"

typedef struct expr1_s {
	GtkWidget *and;        /* only if not the first row */

	GtkWidget *hbox;       /* only if first in row*/
	GtkWidget *remove_row; /* only if first in row*/


	GtkWidget *remove;
	GtkWidget *left, *operator, *right;
	GtkWidget *or;         /* only if not the first in the row */

	gdl_elem_t next_or;   /* --> */
	gdl_elem_t next_and;  /* v */
	gdl_list_t ors;       /* only in the first; the row-first is not on this list, so it collects the subseqent siblings only */
} expr1_t;

typedef struct {
	GtkWidget *window;
	GtkWidget *expr;          /* manual expression entry */
	GtkWidget *action;        /* what-to-do combo box */
	GtkWidget *wizard_enable; /* checkbox */
	GtkWidget *wizard_vbox;
	GtkWidget *new_row;

	gdl_list_t wizard;       /* of expr1_t */
} ghid_search_dialog_t;

static ghid_search_dialog_t sdlg;

static void build_expr1(expr1_t *e, GtkWidget *parent_box)
{
	const char **s, *ops[] = {"==", "!=", ">=", "<=", ">", "<", NULL};

	e->left = gtk_button_new_with_label("<pick>");
	gtk_box_pack_start(GTK_BOX(parent_box), e->left, FALSE, FALSE, 0);

	e->operator = gtk_combo_box_new_text();
	for(s = ops; *s != NULL; s++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(e->operator), *s);
	gtk_box_pack_start(GTK_BOX(parent_box), e->operator, FALSE, FALSE, 0);

	e->right = gtk_button_new_with_label("<pick>");
	gtk_box_pack_start(GTK_BOX(parent_box), e->right, FALSE, FALSE, 0);
}

static expr1_t *append_row()
{
	expr1_t *e = calloc(sizeof(expr1_t), 1);

	if (gdl_first(&sdlg.wizard) != NULL) {
		e->and = gtk_label_new("AND");
		gtk_misc_set_alignment(GTK_MISC(e->and), -1, 0.);
		gtk_box_pack_start(GTK_BOX(sdlg.wizard_vbox), e->and, FALSE, FALSE, 0);
	}

	e->hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sdlg.wizard_vbox), e->hbox, FALSE, FALSE, 0);

	build_expr1(e, e->hbox);

	gdl_append(&sdlg.wizard, e, next_and);
	return e;
}

static expr1_t *append_col(expr1_t *row)
{
	expr1_t *e = calloc(sizeof(expr1_t), 1);

	e->or = gtk_label_new(" OR ");
	gtk_misc_set_alignment(GTK_MISC(e->or), -1, 1);
	gtk_box_pack_start(GTK_BOX(row->hbox), e->or, FALSE, FALSE, 0);

	build_expr1(e, row->hbox);

	gdl_append(&row->ors, e, next_or);
	return e;
}

static void ghid_search_window_create()
{
	GtkWidget *vbox_win, *hbox, *lab, *vbox;
	GtkWidget *content_area, *top_window = gport->top_window;
	const char *actions[] = { "select", "unselect", NULL };
	const char **s;

	/* make sure the list is empty */
	memset(&sdlg.wizard, 0, sizeof(sdlg.wizard));

	sdlg.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sdlg.window = gtk_dialog_new_with_buttons(_("Advanced search"),
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(sdlg.window));

	vbox_win = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), vbox_win);

	lab = gtk_label_new("Query expression:");
	gtk_box_pack_start(GTK_BOX(vbox_win), lab, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(lab), -1, 0.);

/* expr entry */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_win), hbox, TRUE, TRUE, 0);


	sdlg.expr = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), sdlg.expr, TRUE, TRUE, 0);

	sdlg.action = gtk_combo_box_new_text();
	gtk_widget_set_tooltip_text(sdlg.action, "Do this with any object matching the query expression");
/*	g_signal_connect(G_OBJECT(sdlg.action), "changed", G_CALLBACK(action_changed_cb), NULL);*/
	for(s = actions; *s != NULL; s++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(sdlg.action), *s);
	gtk_box_pack_start(GTK_BOX(hbox), sdlg.action, TRUE, TRUE, 0);

/* */
	vbox = ghid_framed_vbox(vbox_win, "wizard", 1, TRUE, 4, 1);
	sdlg.wizard_enable = gtk_check_button_new_with_label("Enable wizard");
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.wizard_enable, TRUE, TRUE, 0);

	sdlg.wizard_vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.wizard_vbox, TRUE, TRUE, 4);

	sdlg.new_row = gtk_button_new_with_label("Add new row");
	gtk_box_pack_start(GTK_BOX(vbox), sdlg.new_row, TRUE, TRUE, 0);

/* */
	{
		expr1_t *r1;
		r1 = append_row();
		append_col(r1);
		append_row();
	}

	gtk_widget_realize(sdlg.window);
}

void ghid_search_window_show(gboolean raise)
{
	ghid_search_window_create();
	gtk_widget_show_all(sdlg.window);
	wplc_place(WPLC_SEARCH, sdlg.window);
	if (raise)
		gtk_window_present(GTK_WINDOW(sdlg.window));
}
