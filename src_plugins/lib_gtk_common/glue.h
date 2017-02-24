#ifndef PCB_GTK_COMMON_GLUE_H
#define PCB_GTK_COMMON_GLUE_H

#include <gtk/gtk.h>
#include "hid.h"

/** The HID using pcb_gtk_common needs to fill in this struct and pass it
    on to most of the calls. This is the only legal way pcb_gtk_common can
    back reference to the HID. This lets multiple HIDs use gtk_common code
    without linker errors. */
typedef struct pcb_gtk_common_s {
	void *gport;      /* Opaque pointer back to the HID's interna struct - used when common calls a HID function */
	GtkWidget *top_window;

	GdkPixmap *(*render_pixmap)(int cx, int cy, double zoom, int width, int height, int depth);

	void (*init_drawing_widget)(GtkWidget *widget, void *gport);
	gboolean (*preview_expose)(GtkWidget *widget, GdkEventExpose *ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);

	void (*window_set_name_label)(gchar *name);
	void (*set_status_line_label)(void);

} pcb_gtk_common_t;

#endif
