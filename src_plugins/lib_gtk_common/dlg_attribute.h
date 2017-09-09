#include <gtk/gtk.h>
#include "hid.h"

int ghid_attribute_dialog(GtkWidget *top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr);

int ghid_att_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled);

int ghid_att_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);


