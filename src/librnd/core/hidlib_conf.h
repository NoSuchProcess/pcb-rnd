/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_HIDLIB_CONF_H
#define PCB_HIDLIB_CONF_H

#include <librnd/core/conf.h>
#include <librnd/core/color.h>

enum pcb_crosshair_shape_e {
	pcb_ch_shape_basic       = 0,    /*  4-ray */
	pcb_ch_shape_union_jack  = 1,    /*  8-ray */
	pcb_ch_shape_dozen       = 2,    /* 12-ray */
	pcb_ch_shape_NUM
};

/* to @conf_gen.sh: begin hidlib */

typedef struct {

	struct {
		RND_CFT_BOOLEAN click_cmd_entry_active;/* true if the command line is active when the user click - this gives the command interpreter a chance to capture the click and use the coords */
	} temp;

	const struct {                       /* rc */
		RND_CFT_INTEGER verbose;
		RND_CFT_INTEGER quiet;                 /* print only errors on stderr */
		RND_CFT_BOOLEAN dup_log_to_stderr;     /* copy log messages to stderr even if there is a HID that can show them */
		RND_CFT_STRING cli_prompt;             /* plain text prompt to prefix the command entry */
		RND_CFT_STRING cli_backend;            /* command parser action */
		RND_CFT_BOOLEAN export_basename;       /* if an exported file contains the source file name, remove path from it, keeping the basename only */
		RND_CFT_STRING menu_file;              /* where to load the default menu file from. If empty/unset, fall back to the legacy 'per hid ow menu file' setup. If contains slash, take it as a full path, if no slash, do a normal menu search for pcb-menu-NAME.lht */
		RND_CFT_LIST preferred_gui;            /* if set, try GUI HIDs in this order when no GUI is explicitly selected */
		RND_CFT_BOOLEAN hid_fallback;          /* if there is no explicitly specified HID (--gui) and the preferred GUI fails, automatically fall back on other HIDs, eventually running in batch mode */

		const struct {
			RND_CFT_STRING home;                 /* user's home dir, determined run-time */
			RND_CFT_STRING exec_prefix;          /* exec prefix path (extracted from argv[0]) */
		} path;
	} rc;

	const struct {
		RND_CFT_REAL layer_alpha;              /* alpha value for layer drawing */
		RND_CFT_REAL drill_alpha;              /* alpha value for drill drawing */

		const struct {
			RND_CFT_STRING   debug_tag;          /* log style tag of debug messages */
			RND_CFT_BOOLEAN  debug_popup;        /* whether a debug line should pop up the log window */
			RND_CFT_STRING   info_tag;           /* log style tag of info messages */
			RND_CFT_BOOLEAN  info_popup;         /* whether an info line should pop up the log window */
			RND_CFT_STRING   warning_tag;        /* log style tag of warnings */
			RND_CFT_BOOLEAN  warning_popup;      /* whether a warning should pop up the log window */
			RND_CFT_STRING   error_tag;          /* log style tag of errors */
			RND_CFT_BOOLEAN  error_popup;        /* whether an error should pop up the log window */
		} loglevels;
		const struct {
			RND_CFT_COLOR background;            /* background and cursor color ... */
			RND_CFT_COLOR off_limit;             /* on-screen background beyond the configured drawing area */
			RND_CFT_COLOR grid;                  /* on-screen grid */
			RND_CFT_COLOR cross;                 /* on-screen crosshair color (inverted) */
		} color;
	} appearance;

	const struct {
		RND_CFT_INTEGER mode;                  /* currently active tool */
		RND_CFT_UNIT grid_unit;                /* select whether you draw in mm or mil */
		RND_CFT_COORD grid;                    /* grid in pcb-units */
		RND_CFT_LIST grids;                    /* grid in grid-string format */
		RND_CFT_INTEGER grids_idx;             /* the index of the currently active grid from grids */
		RND_CFT_BOOLEAN draw_grid;             /* draw grid points */
		RND_CFT_BOOLEAN auto_place;            /* force placement of GUI windows (dialogs), trying to override the window manager */
		RND_CFT_BOOLEAN fullscreen;            /* hide widgets to make more room for the drawing */
		RND_CFT_INTEGER crosshair_shape_idx;   /* crosshair shape as defined in pcb_crosshair_shape_e */
		RND_CFT_BOOLEAN enable_stroke;         /* Enable libstroke gestures on middle mouse button when non-zero */

		const struct {
			RND_CFT_BOOLEAN flip_x;              /* view: flip the board along the X (horizontal) axis */
			RND_CFT_BOOLEAN flip_y;              /* view: flip the board along the Y (vertical) axis */
		} view;

	} editor;
} pcbhl_conf_t;

/* to @conf_gen.sh: end hidlib */


extern pcbhl_conf_t pcbhl_conf;

int pcb_hidlib_conf_init();

/* sets cursor grid with respect to grid spacing, offset and unit values */
void pcb_hidlib_set_grid(rnd_hidlib_t *hidlib, rnd_coord_t Grid, rnd_bool align, rnd_coord_t ox, rnd_coord_t oy);
void pcb_hidlib_set_unit(rnd_hidlib_t *hidlib, const pcb_unit_t *new_unit);


#endif
