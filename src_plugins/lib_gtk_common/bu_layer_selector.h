#ifndef PCB_GTK_BU_LAYER_SELECTOR_H
#define PCB_GTK_BU_LAYER_SELECTOR_H

#include <glib.h>
#include <gtk/gtk.h>
#include "wt_layer_selector.h"
#include "bu_menu.h"

void layer_process(const gchar **color_string, const char **text, int *set, int i);
void make_virtual_layer_buttons(GtkWidget *layer_selector);
void make_layer_buttons(GtkWidget *layersel);
void layer_selector_select_callback(pcb_gtk_layer_selector_t *ls, int layer, gpointer d);
void layer_selector_toggle_callback(pcb_gtk_layer_selector_t *ls, int layer, gpointer d);
void pcb_gtk_layer_buttons_update(GtkWidget *layer_selector, GHidMainMenu *menu);



extern const char selectlayer_syntax[];
extern const char selectlayer_help[];
int pcb_gtk_SelectLayer(GtkWidget *layer_selector, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);


extern const char toggleview_syntax[];
extern const char toggleview_help[];
int pcb_gtk_ToggleView(GtkWidget *layer_selector, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

#endif
