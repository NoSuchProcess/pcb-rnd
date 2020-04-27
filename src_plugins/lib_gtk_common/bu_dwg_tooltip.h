#include <gtk/gtk.h>
gboolean pcb_gtk_dwg_tooltip_check_object(rnd_hidlib_t *hl, GtkWidget *drawing_area, pcb_coord_t crosshairx, pcb_coord_t crosshairy);
void pcb_gtk_dwg_tooltip_cancel_update(void);
void pcb_gtk_dwg_tooltip_queue(GtkWidget *drawing_area, GSourceFunc cb, void *ctx);

