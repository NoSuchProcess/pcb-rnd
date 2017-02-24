#include <gtk/gtk.h>

GtkWidget *pcb_gtk_build_status_line_label(void);
void ghid_status_line_set_text_(GtkWidget *status_line_label, const gchar * text);

void ghid_set_status_line_label_(GtkWidget *status_line_label, int compat_horiz);

