#ifndef PCB_HID_GTK_CONF_H
#define PCB_HID_GTK_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct hid_gtk {
			CFT_BOOLEAN listen;                          /* @usage Listen for actions on stdin. */
			CFT_STRING  bg_image;                        /* @usage File name of an image to put into the background of the GUI canvas. The image must be a color PPM image, in binary (not ASCII) format. It can be any size, and will be automatically scaled to fit the canvas. */

			CFT_BOOLEAN compact_horizontal;
			CFT_BOOLEAN compact_vertical;
			CFT_BOOLEAN use_command_window;
			CFT_INTEGER history_size;
			CFT_INTEGER n_mode_button_columns;

			const struct auto_save_window_geometry {
				CFT_BOOLEAN to_design;
				CFT_BOOLEAN to_project;
				CFT_BOOLEAN to_user;
			} auto_save_window_geometry;

			const struct window_geometry {
				CFT_INTEGER top_x;
				CFT_INTEGER top_y;
				CFT_INTEGER top_width;
				CFT_INTEGER top_height;

				CFT_INTEGER log_x;
				CFT_INTEGER log_y;
				CFT_INTEGER log_width;
				CFT_INTEGER log_height;

				CFT_INTEGER drc_x;
				CFT_INTEGER drc_y;
				CFT_INTEGER drc_width;
				CFT_INTEGER drc_height;

				CFT_INTEGER library_x;
				CFT_INTEGER library_y;
				CFT_INTEGER library_width;
				CFT_INTEGER library_height;

				CFT_INTEGER keyref_x;
				CFT_INTEGER keyref_y;
				CFT_INTEGER keyref_width;
				CFT_INTEGER keyref_height;

				CFT_INTEGER netlist_x;
				CFT_INTEGER netlist_y;
				CFT_INTEGER netlist_height;
				CFT_INTEGER netlist_width;

				CFT_INTEGER pinout_x;
				CFT_INTEGER pinout_y;
				CFT_INTEGER pinout_height;
				CFT_INTEGER pinout_width;
			} window_geometry;
		} hid_gtk;
	} plugins;
} conf_hid_gtk_t;

#define GHID_WGEO1(win, op, ...) \
	op(win ## _x, __VA_ARGS__); \
	op(win ## _y, __VA_ARGS__); \
	op(win ## _width, __VA_ARGS__); \
	op(win ## _height, __VA_ARGS__);

/* Call macro op(field_name, ...) for each window geometry field */
#define GHID_WGEO_ALL(op, ...) \
do { \
	GHID_WGEO1(top, op, __VA_ARGS__); \
	GHID_WGEO1(log, op, __VA_ARGS__); \
	GHID_WGEO1(drc, op, __VA_ARGS__); \
	GHID_WGEO1(library, op, __VA_ARGS__); \
	GHID_WGEO1(keyref, op, __VA_ARGS__); \
	GHID_WGEO1(netlist, op, __VA_ARGS__); \
	GHID_WGEO1(pinout, op, __VA_ARGS__); \
} while(0)

typedef struct window_geometry window_geometry_t;
extern window_geometry_t hid_gtk_wgeo;
extern conf_hid_gtk_t conf_hid_gtk;

#endif
