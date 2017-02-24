#include <gtk/gtk.h>
#include "global_typedefs.h"
#include "hid.h"
#include "glue.h"

/* type is PCB_TYPE_TEXT or PCB_TYPE_ELEMENT_NAME */
void pcb_gtk_dlg_fontsel(pcb_gtk_common_t *com, pcb_layer_t *txtly, pcb_text_t *txt, int type, int modal);

/* temporary back reference to hid_gtk: */
extern void ghid_init_drawing_widget(GtkWidget *widget, void *gport);
extern gboolean ghid_preview_expose(GtkWidget *widget, GdkEventExpose *ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
