#ifndef PCB_GTK_IN_MOUSE_H
#define PCB_GTK_IN_MOUSE_H

#include <librnd/core/hid_cfg_input.h>
#include "pcb_gtk.h"

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>


extern rnd_hid_cfg_mouse_t ghid_mouse;
extern int ghid_wheel_zoom;

rnd_hid_cfg_mod_t ghid_mouse_button(int ev_button);

int ghid_get_user_xy(pcb_gtk_t *ctx, const char *msg);

gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *out);

gboolean ghid_port_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);

void ghid_port_reg_mouse_cursor(pcb_gtk_t *ctx, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask);
void ghid_port_set_mouse_cursor(pcb_gtk_t *ctx, int idx);

void ghid_watch_cursor(pcb_gtk_t *ctx); /* Override the cursor appearance to signifies a wait state */
void ghid_point_cursor(pcb_gtk_t *ctx, rnd_bool grabbed); /* Override the cursor appearance to signifies a point is found */
void ghid_mode_cursor(pcb_gtk_t *ctx); /* Changes the normal cursor appearance according to last set mode, but respect override */
void ghid_restore_cursor(pcb_gtk_t *ctx); /* Remove override and restore the mode cursor */


#endif
