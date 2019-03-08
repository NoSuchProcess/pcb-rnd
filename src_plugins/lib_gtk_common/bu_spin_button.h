#include <gtk/gtk.h>

void
ghid_spin_button(GtkWidget * box, GtkWidget ** spin_button, gfloat value,
								 gfloat low, gfloat high, gfloat step0, gfloat step1,
								 gint digits, gint width,
								 void (*cb_func) (GtkSpinButton *, gpointer), gpointer data, gboolean right_align, const gchar * string);
