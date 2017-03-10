#include <gtk/gtk.h>
#include <glib.h>
#include "conf.h"

void ghid_cancel_lead_user(void);

/* the only entry points we should see */
void ghid_gdk_install(pcb_gtk_common_t *common, pcb_hid_t *hid);
void ghid_gl_install(pcb_gtk_common_t *common, pcb_hid_t *hid);
