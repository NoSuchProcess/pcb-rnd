#include "hid_cfg_input.h"

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

extern pcb_hid_cfg_mouse_t ghid_mouse;
extern int ghid_wheel_zoom;

pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button);

void ghid_hand_cursor(void);
void ghid_point_cursor(void);
/** Changes the cursor appearance to signifies a wait state */
void ghid_watch_cursor(void);
/** Changes the cursor appearance according to @mode */
void ghid_mode_cursor(gint mode);
void ghid_corner_cursor(void);
void ghid_restore_cursor(void);

void ghid_get_user_xy(const char *msg);

gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *out);

gboolean ghid_port_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);

/* Temporary call to hid_gtk */
extern gboolean ghid_idle_cb(gpointer data);
