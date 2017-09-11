#include <gtk/gtk.h>
#include "hid.h"

void *ghid_attr_dlg_new(GtkWidget *top_window, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data);
int ghid_attr_dlg_run(void *hid_ctx);
void ghid_attr_dlg_free(void *hid_ctx);

int ghid_attribute_dialog(GtkWidget *top_window, pcb_hid_attribute_t * attrs, int n_attrs, pcb_hid_attr_val_t * results, const char *title, const char *descr, void *caller_data);

int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled);

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);


