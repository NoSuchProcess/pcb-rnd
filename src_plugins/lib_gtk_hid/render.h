#ifndef PCB_GTK_RENDER_H
#define PCB_GTK_RENDER_H

#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"
#include "conf.h"
#include "pcb_bool.h"

/* the only entry points we should see from the actual renderers */
void ghid_gdk_install(pcb_gtk_common_t *common, pcb_hid_t *hid);
void ghid_gl_install(pcb_gtk_common_t *common, pcb_hid_t *hid);

/* Utility functions for renderers to call ***/
void ghid_draw_area_update(GHidPort *out, GdkRectangle *rect);

static inline void ghid_invalidate_all(void) { pcb_gui->invalidate_all(); }
void ghid_drawing_area_configure_hook(void *out);

#endif
