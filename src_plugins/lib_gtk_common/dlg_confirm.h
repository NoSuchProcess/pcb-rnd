#include <stdarg.h>
#include <gtk/gtk.h>

int pcb_gtk_dlg_confirm_open(GtkWidget *top_window, const char *msg, va_list ap);
int pcb_gtk_dlg_confirm_close(GtkWidget *top_window);

