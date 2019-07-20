#include <gtk/gtk.h>
#include "hid.h"
#include "pcb_gtk.h"

void *ghid_attr_dlg_new(pcb_gtk_t *gctx, const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);
int ghid_attr_dlg_run(void *hid_ctx);
void ghid_attr_dlg_raise(void *hid_ctx);
void ghid_attr_dlg_free(void *hid_ctx);
void ghid_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);

int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled);
int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide);

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);
void ghid_attr_dlg_set_help(void *hid_ctx, int idx, const char *val);


/* Create an interacgive DAD subdialog under parent_vbox */
void *ghid_attr_sub_new(pcb_gtk_t *gctx, GtkWidget *parent_box, pcb_hid_attribute_t *attrs, int n_attrs, void *caller_data);

/* Fix up background color of various widgets - useful if the host dialog's background color is not the default */
void pcb_gtk_dad_fixcolor(void *hid_ctx, const GdkColor *color);

/* Report new window coords to the central window placement code
   emitting an event */
int pcb_gtk_winplace_cfg(pcb_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id);
