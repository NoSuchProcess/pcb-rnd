#include <gtk/gtk.h>
#include <glib.h>
#include "conf.h"

void ghid_cancel_lead_user(void);

void ghid_gdk_set_special_colors(conf_native_t *cfg);

gboolean ghid_gdk_drawing_area_expose_cb(GtkWidget *widget, GdkEventExpose *ev, void *port);
void ghid_gdk_port_drawing_realize_cb(GtkWidget *widget, gpointer data);

int ghid_gdk_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);

void ghid_gl_set_special_colors(conf_native_t *cfg);

gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, GdkEventExpose *ev, void *port);
void ghid_gl_port_drawing_realize_cb(GtkWidget *widget, gpointer data);

int ghid_gl_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);

/* the only entry points we should see */
void ghid_gdk_install(pcb_gtk_common_t *common);
void ghid_gl_install(pcb_gtk_common_t *common);
