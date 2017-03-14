#include <gtk/gtk.h>

/** Returns in \p button , a new gtk_check_button inside a \p box,
    labeled \p string if not \c NULL.
 */
void pcb_gtk_check_button_connected(GtkWidget * box,
																		GtkWidget ** button,
																		gboolean active,
																		gboolean pack_start,
																		gboolean expand,
																		gboolean fill,
																		gint pad,
																		void (*cb_func) (GtkToggleButton *, gpointer), gpointer data, const gchar * string);
