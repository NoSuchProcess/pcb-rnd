/* #include this file to get widget print functions */

#include <stdio.h>
#include <glib.h>

static void print_widget(GtkWidget * w)
{
	fprintf(stderr, " %p %d;%d %d;%d %d;%d\n", (void *)w, w->allocation.x, w->allocation.y, w->allocation.width, w->allocation.height,
					w->requisition.width, w->requisition.height);
	fprintf(stderr, "  flags=%x typ=%d realized=%d vis=%d\n", GTK_WIDGET_FLAGS(w), GTK_WIDGET_TYPE(w), GTK_WIDGET_REALIZED(w),
					GTK_WIDGET_VISIBLE(w));
}


static void print_menu_shell_cb(void *obj, void *ud)
{
	GtkWidget *w = obj;
	print_widget(w);
}

static void print_menu_shell(GtkMenuShell * sh)
{
	GList *children;
	children = gtk_container_get_children(GTK_CONTAINER(sh));
	g_list_foreach(children, print_menu_shell_cb, NULL);
}
