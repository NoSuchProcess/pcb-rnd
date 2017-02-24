#ifndef PCB_GTK_COMMON_GLUE_H
#define PCB_GTK_COMMON_GLUE_H

/** The HID using pcb_gtk_common needs to fill in this struct and pass it
    on to most of the calls. This is the only legal way pcb_gtk_common can
    back reference to the HID. This lets multiple HIDs use gtk_common code
    without linker errors. */
typedef struct pcb_gtk_common_s {
	void *gport;      /* Opaque pointer back to the HID's interna struct - used when common calls a HID function */

	GdkPixmap *(*render_pixmap)(int cx, int cy, double zoom, int width, int height, int depth);

} pcb_gtk_common_t;

#endif
