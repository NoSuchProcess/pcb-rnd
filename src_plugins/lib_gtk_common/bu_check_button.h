#include <gtk/gtk.h>

void pcb_gtk_check_button_connected(GtkWidget * box,
																		GtkWidget ** button,
																		gboolean active,
																		gboolean pack_start,
																		gboolean expand,
																		gboolean fill,
																		gint pad,
																		void (*cb_func) (GtkToggleButton *, gpointer), gpointer data, const gchar * string);
