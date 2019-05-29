#ifndef GHID_WIN_PLACE
#define GHID_WIN_PLACE

#include <gtk/gtk.h>
#include "global_typedefs.h"

/* Report new window coords to the central window placement code
   emitting an event */
gint pcb_gtk_winplace_cfg(pcb_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id);

#endif
