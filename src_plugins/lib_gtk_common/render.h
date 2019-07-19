#ifndef PCB_GTK_RENDER_H
#define PCB_GTK_RENDER_H

#include <gtk/gtk.h>
#include <glib.h>
#include "gui.h"
#include "conf.h"
#include "pcb_bool.h"

/* Utility functions for renderers to call ***/
void ghid_draw_area_update(GHidPort *out, GdkRectangle *rect);

#endif
