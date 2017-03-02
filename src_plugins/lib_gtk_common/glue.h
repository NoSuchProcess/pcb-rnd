#ifndef PCB_GTK_COMMON_GLUE_H
#define PCB_GTK_COMMON_GLUE_H

#include <gtk/gtk.h>
#include "hid.h"
#include "conf.h"

/** The HID using pcb_gtk_common needs to fill in this struct and pass it
    on to most of the calls. This is the only legal way pcb_gtk_common can
    back reference to the HID. This lets multiple HIDs use gtk_common code
    without linker errors. */
typedef struct pcb_gtk_common_s {
	void *gport;      /* Opaque pointer back to the HID's interna struct - used when common calls a HID function */
	GtkWidget *top_window;

	/* rendering */
	GdkPixmap *(*render_pixmap)(int cx, int cy, double zoom, int width, int height, int depth);
	gboolean (*preview_draw)(GtkWidget *widget, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
	void (*drawing_realize)(GtkWidget *w, void *gport);
	gboolean (*drawing_area_expose)(GtkWidget *w, GdkEventExpose *ev, void *gport);

	void (*init_drawing_widget)(GtkWidget *widget, void *gport);
	gboolean (*preview_expose)(GtkWidget *widget, GdkEventExpose *ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);

	/* main window */
	void (*window_set_name_label)(gchar *name);
	void (*set_status_line_label)(void);
	void (*status_line_set_text)(const gchar *text);

	void (*note_event_location)(GdkEventButton *ev);
	gboolean (*idle_cb)(gpointer data);
	int (*shift_is_pressed)();
	void (*interface_input_signals_disconnect)(void);
	void (*interface_input_signals_connect)(void);
	void (*interface_set_sensitive)(gboolean sensitive);
	void (*port_button_press_main)(void);
	void (*port_button_release_main)(void);

	void (*route_styles_edited_cb)(void);

	void (*mode_cursor_main)(int mode);

	void (*invalidate_all)();
	void (*cancel_lead_user)(void);
	void (*lead_user_to_location)(pcb_coord_t x, pcb_coord_t y);
	void (*pan_common)(void);
	void (*port_ranges_scale)(void);
	void (*pack_mode_buttons)(void);

	void (*LayersChanged)(void);

	void (*load_bg_image)(void);
	void (*main_destroy)(void *gport);

	/* only for config: */
	const gchar *(*get_color_name)(GdkColor * color);
	void (*map_color_string)(const char *color_string, GdkColor * color);
	void (*set_special_colors)(conf_native_t *cfg);
	void (*layer_buttons_color_update)(void);

	int (*command_entry_is_active)(void);
	void (*command_use_command_window_sync)(void);

} pcb_gtk_common_t;

#endif
