#include <gtk/gtk.h>
#include "font.h"
#include "hid.h"

void pcb_gtk_dlg_fontsel(void *gport, GtkWidget *top_window, pcb_font_t *fnt, int modal);

/* temporary back reference to hid_gtk: */
extern void ghid_init_drawing_widget(GtkWidget *widget, void *gport);
extern gboolean ghid_preview_expose(GtkWidget *widget, GdkEventExpose *ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
