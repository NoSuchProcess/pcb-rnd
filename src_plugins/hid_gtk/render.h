#include <gtk/gtk.h>
#include <glib.h>
#include "conf.h"

void ghid_cancel_lead_user(void);
gboolean ghid_preview_draw(GtkWidget *widget, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
void ghid_set_special_colors(conf_native_t *cfg);

