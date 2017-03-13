#include <gtk/gtk.h>

/** Returns a new empty label able to use markup language. */
GtkWidget *pcb_gtk_status_line_label_new(void);
void ghid_status_line_set_text_(GtkWidget *status_line_label, const gchar * text);

void ghid_set_status_line_label_(GtkWidget *status_line_label, int compat_horiz);

