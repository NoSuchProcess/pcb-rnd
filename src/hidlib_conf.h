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

#include "conf.h"
#include "color.h"

/* to @conf_gen.sh: begin hidlib */

typedef struct {

	const struct {                       /* rc */
		CFT_INTEGER verbose;
		CFT_INTEGER quiet;                 /* print only errors on stderr */
		CFT_BOOLEAN dup_log_to_stderr;     /* copy log messages to stderr even if there is a HID that can show them */
		CFT_STRING cli_prompt;             /* plain text prompt to prefix the command entry */
		CFT_STRING cli_backend;            /* command parser action */
		CFT_BOOLEAN export_basename;       /* if an exported file contains the source file name, remove path from it, keeping the basename only */
		CFT_STRING menu_file;              /* where to load the default menu file from. If empty/unset, fall back to the legacy 'per hid ow menu file' setup. If contains slash, take it as a full path, if no slash, do a normal menu search for pcb-menu-NAME.lht */
		const struct {
			CFT_STRING home;                 /* user's home dir, determined run-time */
			CFT_STRING exec_prefix;          /* exec prefix path (extracted from argv[0]) */
		} path;
	} rc;

	const struct {
		CFT_REAL layer_alpha;              /* alpha value for layer drawing */
		CFT_REAL drill_alpha;              /* alpha value for drill drawing */

		const struct {
			CFT_STRING   debug_tag;          /* log style tag of debug messages */
			CFT_BOOLEAN  debug_popup;        /* whether a debug line should pop up the log window */
			CFT_STRING   info_tag;           /* log style tag of info messages */
			CFT_BOOLEAN  info_popup;         /* whether an info line should pop up the log window */
			CFT_STRING   warning_tag;        /* log style tag of warnings */
			CFT_BOOLEAN  warning_popup;      /* whether a warning should pop up the log window */
			CFT_STRING   error_tag;          /* log style tag of errors */
			CFT_BOOLEAN  error_popup;        /* whether an error should pop up the log window */
		} loglevels;
		const struct {
			CFT_COLOR background;            /* background and cursor color ... */
			CFT_COLOR off_limit;             /* on-screen background beyond the configured drawing area */
			CFT_COLOR grid;                  /* on-screen grid */
			CFT_COLOR cross;                 /* crosshair, drc outline color */
		} color;
	} appearance;

	const struct {
		CFT_UNIT grid_unit;                /* select whether you draw in mm or mil */
		CFT_COORD grid;                    /* grid in pcb-units */
		CFT_LIST grids;                    /* grid in grid-string format */
		CFT_INTEGER grids_idx;             /* the index of the currently active grid from grids */
		CFT_BOOLEAN draw_grid;             /* draw grid points */
		CFT_BOOLEAN auto_place;            /* force placement of GUI windows (dialogs), trying to override the window manager */
		CFT_BOOLEAN fullscreen;            /* hide widgets to make more room for the drawing */

		const struct {
			CFT_BOOLEAN flip_x;              /* view: flip the board along the X (horizontal) axis */
			CFT_BOOLEAN flip_y;              /* view: flip the board along the Y (vertical) axis */
		} view;

	} editor;
} pcbhl_conf_t;

/* to @conf_gen.sh: end hidlib */


extern pcbhl_conf_t pcbhl_conf;

int pcb_hidlib_conf_init();

/* sets cursor grid with respect to grid spacing, offset and unit values */
void pcb_hidlib_set_grid(pcb_hidlib_t *hidlib, pcb_coord_t Grid, pcb_bool align, pcb_coord_t ox, pcb_coord_t oy);
void pcb_hidlib_set_unit(pcb_hidlib_t *hidlib, const pcb_unit_t *new_unit);


#endif
