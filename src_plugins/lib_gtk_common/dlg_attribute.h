#include <gtk/gtk.h>
#include <librnd/core/hid.h>
#include "pcb_gtk.h"

void *ghid_attr_dlg_new(pcb_gtk_t *gctx, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);
int ghid_attr_dlg_run(void *hid_ctx);
void ghid_attr_dlg_raise(void *hid_ctx);
void ghid_attr_dlg_close(void *hid_ctx);
void ghid_attr_dlg_free(void *hid_ctx);
void ghid_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val);

int ghid_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled);
int ghid_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide);

int ghid_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void ghid_attr_dlg_set_help(void *hid_ctx, int idx, const char *val);


/* Create an interacgive DAD subdialog under parent_vbox */
void *ghid_attr_sub_new(pcb_gtk_t *gctx, GtkWidget *parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data);

/* Fix up background color of various widgets - useful if the host dialog's background color is not the default */
void pcb_gtk_dad_fixcolor(void *hid_ctx, const GdkColor *color);

/* Report new window coords to the central window placement code
   emitting an event */
int pcb_gtk_winplace_cfg(rnd_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id);

rnd_hidlib_t *ghid_attr_get_dad_hidlib(void *hid_ctx);

