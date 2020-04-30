#ifndef PCB_GTK_TOPWIN_H
#define PCB_GTK_TOPWIN_H

/* OPTIONAL top window implementation */

#include <gtk/gtk.h>

#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>

#include "pcb_gtk.h"
#include "bu_menu.h"
#include "bu_command.h"

void ghid_update_toggle_flags(rnd_hidlib_t *hidlib, pcb_gtk_topwin_t *tw, const char *cookie);
void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_create_pcb_widgets(pcb_gtk_t *ctx, pcb_gtk_topwin_t *tw, GtkWidget *in_top_window);
void ghid_fullscreen_apply(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_layer_vis_update(pcb_gtk_topwin_t *tw);

void pcb_gtk_tw_interface_set_sensitive(pcb_gtk_topwin_t *tw, gboolean sensitive);

int pcb_gtk_tw_dock_enter(pcb_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id);
void pcb_gtk_tw_dock_leave(pcb_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub);

void pcb_gtk_tw_set_title(pcb_gtk_topwin_t *tw, const char *title);

gboolean ghid_idle_cb(void *topwin);
gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, pcb_gtk_topwin_t *tw);

#endif

