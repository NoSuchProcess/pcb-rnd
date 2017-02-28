#include <gtk/gtk.h>

/* initializes icon pixmap (cursor bits, pcb-rnd logo) */
void pcb_gtk_icons_init(GdkWindow *top_window);

extern GdkPixmap *XC_hand_source, *XC_hand_mask;
extern GdkPixmap *XC_lock_source, *XC_lock_mask;
extern GdkPixmap *XC_clock_source, *XC_clock_mask;
