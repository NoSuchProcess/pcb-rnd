#ifndef PCB_HID_GTK_CONF_H
#define PCB_HID_GTK_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct hid_gtk {
			CFT_BOOLEAN listen;                          /* @usage Listen for actions on stdin. */
			CFT_STRING  bg_image;                        /* @usage File name of an image to put into the background of the GUI canvas. The image is read via GdkPixbuf library. It can be any size, and will be automatically scaled to fit the canvas. */

			CFT_BOOLEAN compact_horizontal;              /* OBSOLETE: ignored; use central appearance/compact instead */
			CFT_BOOLEAN compact_vertical;                /* OBSOLETE: ignored; use central appearance/compact instead */
			CFT_INTEGER history_size;                    /* OBSOLETE: ignored; use plugins/lib_hid_common/cli_history/slots instead */
			CFT_INTEGER n_mode_button_columns;           /* OBSOLETE: always have horizontal mode buttons */

			const struct local_grid {
				CFT_BOOLEAN enable;                       /* enable local grid to draw grid points only in a small radius around the crosshair - speeds up software rendering on large screens */
				CFT_INTEGER radius;                       /* radius, in number of grid points, around the local grid */
			} local_grid;

			const struct global_grid {
				CFT_INTEGER min_dist_px;                  /* never try to draw a grid so dense that the distance between grid points is smaller than this */
				CFT_BOOLEAN sparse;                       /* enable drawing sparse grid: when zoomed out beyond min_dist_px draw every 2nd, 4th, 8th, etc. grid point; if disabled the grid is turned off when it'd get too dense */
			} global_grid;

			const struct auto_save_window_geometry {
				CFT_BOOLEAN to_design;                    /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_design instead */
				CFT_BOOLEAN to_project;                   /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_project instead */
				CFT_BOOLEAN to_user;                      /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_user instead */
			} auto_save_window_geometry;

			const struct window_geometry {
				CFT_INTEGER top_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER top_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER top_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER top_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER log_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER log_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER log_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER log_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER drc_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER drc_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER drc_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER drc_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER library_x;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER library_y;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER library_width;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER library_height;               /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER keyref_x;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER keyref_y;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER keyref_width;                 /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER keyref_height;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER netlist_x;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER netlist_y;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER netlist_height;               /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER netlist_width;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				CFT_INTEGER pinout_x;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER pinout_y;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER pinout_height;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				CFT_INTEGER pinout_width;                 /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
			} window_geometry;
		} hid_gtk;
	} plugins;
} conf_hid_gtk_t;

extern conf_hid_gtk_t conf_hid_gtk;

#endif
