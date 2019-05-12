#ifndef PCB_GTK_COMMON_GLUE_H
#define PCB_GTK_COMMON_GLUE_H

#include <gtk/gtk.h>
#include "hid.h"
#include "conf.h"

#include "compat.h"

/* The HID using pcb_gtk_common needs to fill in this struct and pass it
   on to most of the calls. This is the only legal way pcb_gtk_common can
   back reference to the HID. This lets multiple HIDs use gtk_common code
   without linker errors. */
typedef struct pcb_gtk_common_s {
	pcb_hidlib_t *hidlib;
	void *gport;      /* Opaque pointer back to the HID's internal struct - used when common calls a HID function */
	GtkWidget *top_window;

	/* rendering */
	void (*drawing_realize)(GtkWidget *w, void *gport);
	gboolean (*drawing_area_expose)(GtkWidget *w, pcb_gtk_expose_t *p, void *gport);
	void (*drawing_area_configure_hook)(void *);

	GtkWidget *(*new_drawing_widget)(struct pcb_gtk_common_s *common);
	void (*init_drawing_widget)(GtkWidget *widget, void *gport);
	gboolean (*preview_expose)(GtkWidget *widget, pcb_gtk_expose_t *p, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx);
	void (*load_bg_image)(void);
	void (*init_renderer)(int *argc, char ***argv, void *port);
	void (*draw_grid_local)(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy);

	/* UI */
	void (*note_event_location)(GdkEventButton *ev);
	int (*shift_is_pressed)();
	void (*interface_input_signals_disconnect)(void);
	void (*interface_input_signals_connect)(void);
	void (*interface_set_sensitive)(gboolean sensitive);
	void (*port_button_press_main)(void);
	void (*port_button_release_main)(void);
	void (*port_ranges_changed)(void);

	/* screen */
	void (*mode_cursor_main)(void);
	void (*invalidate_all)(pcb_hidlib_t *hidlib);
	void (*pan_common)(void);
	void (*port_ranges_scale)(void);
	void (*screen_update)(void);
	void (*shutdown_renderer)(void *port);

	/* execute events */
	void (*main_destroy)(void *gport);

	pcb_bool (*map_color_string)(const char *color_string, pcb_gtk_color_t * color);
	const gchar *(*get_color_name)(pcb_gtk_color_t * color);

	void (*set_special_colors)(conf_native_t *cfg);

	int (*command_entry_is_active)(void);
} pcb_gtk_common_t;

#endif
