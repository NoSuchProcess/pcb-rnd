#include <gtk/gtk.h>

/** Returns a new empty label able to use markup language. */
GtkWidget *pcb_gtk_status_line_label_new(void);

void pcb_gtk_status_line_set_text(GtkWidget *status_line_label, const gchar * text);

void pcb_gtk_status_line_update(GtkWidget *status_line_label, int compat_horiz);

