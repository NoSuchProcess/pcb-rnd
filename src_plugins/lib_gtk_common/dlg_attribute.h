#include <gtk/gtk.h>
#include "hid.h"

int ghid_attribute_dialog(GtkWidget *top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr);

void pcb_gtk_dlg_attributes(GtkWidget *top_window, const char *owner, pcb_attribute_list_t * attrs);




