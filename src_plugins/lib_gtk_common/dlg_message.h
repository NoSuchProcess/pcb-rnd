#include <gtk/gtk.h>

/*FIXME: Remove this dependency : remove such: str _("text"); */
#include "compat_nls.h"

gint pcb_gtk_dlg_message(const char *message, GtkWindow * parent);
