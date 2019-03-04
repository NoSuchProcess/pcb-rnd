#include <gtk/gtk.h>
#include "hid.h"
#include "glue.h"

void *ghid_attr_dlg_new(pcb_gtk_common_t *com, const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy);
int ghid_attr_dlg_run(void *hid_ctx);
void ghid_attr_dlg_free(void *hid_ctx);
void ghid_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);

int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled);
int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide);

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);


/* Create an interacgive DAD subdialog under parent_vbox */
void *ghid_attr_sub_new(pcb_gtk_common_t *com, GtkWidget *parent_box, pcb_hid_attribute_t *attrs, int n_attrs, void *caller_data);
