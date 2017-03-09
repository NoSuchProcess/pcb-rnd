#include <gtk/gtk.h>
#include <glib.h>
#include "conf.h"

void ghid_cancel_lead_user(void);
gboolean ghid_preview_draw(GtkWidget *widget, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
void ghid_set_special_colors(conf_native_t *cfg);

gboolean ghid_drawing_area_expose_cb(GtkWidget *widget, GdkEventExpose *ev, void *port);
void ghid_port_drawing_realize_cb(GtkWidget *widget, gpointer data);

int ghid_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);

