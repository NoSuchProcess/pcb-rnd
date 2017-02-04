#include <gtk/gtk.h>
#include "obj_elem.h"
#include "hid.h"

void ghid_pinout_window_show(void *gport, pcb_element_t *element);

/* glue from hid_gtk: */
extern void ghid_init_drawing_widget(GtkWidget *widget, void *gport);
extern gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);

