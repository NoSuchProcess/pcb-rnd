#include <gtk/gtk.h>

/** Opens a dialog window displaying the text to be reported */
void pcb_gtk_dlg_report(GtkWidget * top_window, const gchar * title, const gchar * message, gboolean modal);
