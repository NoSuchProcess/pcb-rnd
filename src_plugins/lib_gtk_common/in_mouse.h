#ifndef PCB_GTK_IN_MOUSE_H
#define PCB_GTK_IN_MOUSE_H

#include "hid_cfg_input.h"
#include "glue.h"

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

typedef struct pcb_gtk_mouse_s {
	GtkWidget *drawing_area, *top_window;
	GdkCursor *X_cursor;          /* used X cursor */
	GdkCursorType X_cursor_shape; /* and its shape */
	pcb_gtk_common_t *com;
} pcb_gtk_mouse_t;

extern pcb_hid_cfg_mouse_t ghid_mouse;
extern int ghid_wheel_zoom;

pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button);

void ghid_hand_cursor(pcb_gtk_mouse_t *ctx);
void ghid_point_cursor(pcb_gtk_mouse_t *ctx, pcb_bool grabbed);

/* Changes the cursor appearance to signifies a wait state */
void ghid_watch_cursor(pcb_gtk_mouse_t *ctx);

/* Changes the cursor appearance according to mode */
void ghid_mode_cursor(pcb_gtk_mouse_t *ctx, gint mode);

void ghid_corner_cursor(pcb_gtk_mouse_t *ctx);
void ghid_restore_cursor(pcb_gtk_mouse_t *ctx);

gboolean ghid_get_user_xy(pcb_gtk_mouse_t *ctx, const char *msg);

gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *out);

gboolean ghid_port_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);

#endif
