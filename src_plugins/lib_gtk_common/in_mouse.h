#ifndef PCB_GTK_IN_MOUSE_H
#define PCB_GTK_IN_MOUSE_H

#include "hid_cfg_input.h"

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

typedef struct pcb_gtk_mouse_s {
	GtkWidget *drawing_area, *top_window;
	GdkCursor *X_cursor;          /* used X cursor */
	GdkCursorType X_cursor_shape; /* and its shape */
} pcb_gtk_mouse_t;

extern pcb_hid_cfg_mouse_t ghid_mouse;
extern int ghid_wheel_zoom;

pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button);

void ghid_hand_cursor(pcb_gtk_mouse_t *ctx);
void ghid_point_cursor(pcb_gtk_mouse_t *ctx);
/** Changes the cursor appearance to signifies a wait state */
void ghid_watch_cursor(pcb_gtk_mouse_t *ctx);
/** Changes the cursor appearance according to @mode */
void ghid_mode_cursor(pcb_gtk_mouse_t *ctx, gint mode);
void ghid_corner_cursor(pcb_gtk_mouse_t *ctx);
void ghid_restore_cursor(pcb_gtk_mouse_t *ctx);

void ghid_get_user_xy(pcb_gtk_mouse_t *ctx, const char *msg);

gint ghid_port_window_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *out);

gboolean ghid_port_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);

extern GdkPixmap *XC_hand_source, *XC_hand_mask;
extern GdkPixmap *XC_lock_source, *XC_lock_mask;
extern GdkPixmap *XC_clock_source, *XC_clock_mask;

/* Temporary calls to hid_gtk */
extern gboolean ghid_idle_cb(gpointer data);
extern int ghid_shift_is_pressed();
extern void ghid_interface_input_signals_disconnect(void);
extern void ghid_interface_input_signals_connect(void);
extern void ghid_interface_set_sensitive(gboolean sensitive);
extern void ghid_port_button_press_main(void);
extern void ghid_port_button_release_main(void);
extern void ghid_status_line_set_text(const gchar *text);
extern void ghid_set_status_line_label(void);

#endif
