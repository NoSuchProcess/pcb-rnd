#include <gtk/gtk.h>

/** Initializes icon pixbufs (cursor bits, pcb-rnd logo). */
void pcb_gtk_icons_init(GtkWindow *top_window);

extern GdkPixbuf *XC_clock_source, *XC_hand_source, *XC_lock_source;
