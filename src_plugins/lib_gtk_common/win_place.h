#ifndef GHID_WIN_PLACE
#define GHID_WIN_PLACE

#include <gtk/gtk.h>

/* Place a dialog box if auto-placement is enabled and there is saved
   preference for dialog box coord or shape. Call this right after the
   dialog box is created but before the configure event is bound. */
void pcb_gtk_winplace(GtkWidget *dialog, const char *id);

/* Report new window coords to the central window placement code
   emitting an event */
gint pcb_gtk_winplace_cfg(GtkWidget *widget, void *ctx, const char *id);

#endif
