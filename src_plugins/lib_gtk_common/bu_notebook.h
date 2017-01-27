#include <gtk/gtk.h>

/**  Appends a page to notebook */
GtkWidget *ghid_notebook_page(GtkWidget * tabs, const char *name, gint pad, gint border);

/**  */
GtkWidget *ghid_framed_notebook_page(GtkWidget * tabs, const char *name, gint border,
																		 gint frame_border, gint vbox_pad, gint vbox_border);
