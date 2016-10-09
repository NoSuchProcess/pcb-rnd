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
#include "gui.h"
#include "create.h"
#include "compat_misc.h"
#include "ghid-search.h"

typedef struct {
	GtkWidget *window;
	GtkWidget *expr;      /* manual expression entry */
} ghid_search_dialog_t;

static ghid_search_dialog_t sdlg;

static void ghid_search_window_create()
{
	GtkWidget *vbox_win;
	GtkWidget *content_area, *top_window = gport->top_window;

	sdlg.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sdlg.window = gtk_dialog_new_with_buttons(_("Advanced search"),
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(sdlg.window));

	vbox_win = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), vbox_win);

/* */

/* */

	gtk_widget_realize(sdlg.window);
}

void ghid_search_window_show(gboolean raise)
{
	ghid_search_window_create();
	gtk_widget_show_all(sdlg.window);
	if (raise)
		gtk_window_present(GTK_WINDOW(sdlg.window));
}
