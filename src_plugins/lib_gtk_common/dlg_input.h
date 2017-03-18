#include <gtk/gtk.h>

/** Query the user for a string. Fill in initial on start. 
    Returns a string on which g_free() should be ran, or NULL on cancel. */
gchar *pcb_gtk_dlg_input(const char *prompt, const char *initial, GtkWindow *parent);
