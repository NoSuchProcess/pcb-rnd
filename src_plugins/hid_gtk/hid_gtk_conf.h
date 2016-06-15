#ifndef PCB_HID_GTK_CONF_H
#define PCB_HID_GTK_CONF_H

#include "globalconst.h"
#include "conf.h"

typedef struct {
	const struct plugins {
		const struct hid_gtk {
			CFT_BOOLEAN compact_horizontal;
			CFT_BOOLEAN compact_vertical;
			CFT_BOOLEAN use_command_window;
			CFT_INTEGER history_size;
			CFT_INTEGER n_mode_button_columns;

			CFT_BOOLEAN auto_save_window_geometry;       /* true=save window geometry so they are preserved after a restart of pcb */
			CFT_BOOLEAN save_window_geometry_in_design;  /* true=save window geometry per design; false=save it per user */
			const struct window_geometry {
				CFT_INTEGER top_width;
				CFT_INTEGER top_height;
				CFT_INTEGER log_width;
				CFT_INTEGER log_height;
				CFT_INTEGER drc_width;
				CFT_INTEGER drc_height;
				CFT_INTEGER library_width;
				CFT_INTEGER library_height;
				CFT_INTEGER netlist_height;
				CFT_INTEGER keyref_width;
				CFT_INTEGER keyref_height;
			} window_geometry;
		} hid_gtk;
	} plugins;
} conf_hid_gtk_t;

typedef struct window_geometry window_geometry_t;
extern window_geometry_t hid_gtk_wgeo;
extern conf_hid_gtk_t conf_hid_gtk;

#endif
