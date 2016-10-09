#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "create.h"
#include "compat_misc.h"
#include "ghid-search.h"

GtkWidget *ghid_search_dialog_create(ghid_search_dialog_t *dlg)
{
	GtkWidget *window, *vbox_win;
	GtkWidget *content_area, *top_window = gport->top_window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	window = gtk_dialog_new_with_buttons(_("Advanced search"),
																			 GTK_WINDOW(top_window),
																			 GTK_DIALOG_DESTROY_WITH_PARENT,
																			 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));

	vbox_win = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), vbox_win);

/* */

/* */

	gtk_widget_show_all(window);

	return window;
}
