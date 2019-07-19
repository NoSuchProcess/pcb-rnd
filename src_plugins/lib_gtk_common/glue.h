#ifndef PCB_GTK_COMMON_GLUE_H
#define PCB_GTK_COMMON_GLUE_H

#include <gtk/gtk.h>
#include "hid.h"
#include "conf.h"
#include <genlist/gendlist.h>

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
	gboolean (*preview_expose)(GtkWidget *widget, pcb_gtk_expose_t *p, pcb_hid_expose_t expcall, pcb_hid_expose_ctx_t *ctx); /* p == NULL when called from the code, not from a GUI expose event */
	void (*load_bg_image)(void);
	void (*init_renderer)(int *argc, char ***argv, void *port);
	void (*draw_grid_local)(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy);

	/* UI */
	void (*note_event_location)(GdkEventButton *ev); /* remove */
	void (*interface_input_signals_disconnect)(void); /* remove */
	void (*interface_input_signals_connect)(void); /* remove */
	void (*interface_set_sensitive)(gboolean sensitive); /* remove */
	void (*port_button_press_main)(void); /* remove */
	void (*port_button_release_main)(void);/* remove */
	void (*port_ranges_changed)(void); /* remove */

	/* screen */
	void (*mode_cursor_main)(void); /* remove */
	void (*invalidate_all)(pcb_hidlib_t *hidlib); /* remove? */
	void (*pan_common)(void);  /* remove */
	void (*port_ranges_scale)(void); /* remove */
	void (*screen_update)(void);
	void (*shutdown_renderer)(void *port);

	/* execute events */
	void (*main_destroy)(void *gport); /* remove */

	pcb_bool (*map_color_string)(const char *color_string, pcb_gtk_color_t * color);
	const gchar *(*get_color_name)(pcb_gtk_color_t * color);

	void (*set_special_colors)(conf_native_t *cfg);

	/* all widget lists */
	gdl_list_t previews;
} pcb_gtk_common_t;

#endif
