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

#ifndef PCB_HIDLIB_H
#define PCB_HIDLIB_H

struct pcb_hidlib_s {
	pcb_coord_t grid;                  /* grid resolution */
	pcb_coord_t grid_ox, grid_oy;      /* grid offset */
	pcb_coord_t size_x, size_y;        /* drawing area extents (or board dimensions) */
	char *name;                        /* name of the design */
	char *filename;                    /* name of the file (from load) */
};

void pcb_hidlib_event_uninit(void);
void pcb_hidlib_event_init(void);

/* print pending log messages to stderr after gui uninit */
void pcbhl_log_print_uninit_errs(const char *title);


/*** The following API is implemented by the host application ***/

/* update crosshair-attached object because crosshair coords likely changed */
void pcb_hidlib_adjust_attached_objects(void);

/* Suspend the crosshair: save all crosshair states in a newly allocated
   and returned temp buffer, then reset the crosshair to initial state;
   the returned buffer is used to restore the crosshair states later on.
   Used in the get location loop. */
void *pcb_hidlib_crosshair_suspend(void);
void pcb_hidlib_crosshair_restore(void *susp_data);

/* Move the crosshair to an absolute x;y coord on the board and update the GUI;
   if mouse_mot is non-zero, the request is a direct result of a mouse motion
   event */
void pcb_hidlib_crosshair_move_to(pcb_coord_t abs_x, pcb_coord_t abs_y, int mouse_mot);

/* The whole default menu file embedded in the executable; NULL if not present */
extern const char *pcb_hidlib_default_embedded_menu;

/* Draw any fixed mark on XOR overlay; if inhibit_drawing_mode is true, do not call ->set_drawing_mode */
void pcbhl_draw_marks(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode);

/* Draw any mark following the crosshair on XOR overlay; if inhibit_drawing_mode is true, do not call ->set_drawing_mode */
void pcbhl_draw_attached(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode);

/*** One of these two functions will be called whenever (parts of) the screen
     needs redrawing (on screen, print or export, board or preview). The expose
     function does the following:
      - allocate any GCs needed
      - set drawing mode
      - cycle through the layers, calling set_layer for each layer to be
        drawn, and only drawing objects (all or specified) of desired
        layers. ***/

/* Main expose: draw the design in the top window
   (pcb-rnd: all layers with all flags (no .content is used) */
void pcbhl_expose_main(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *region, pcb_xform_t *xform_caller);

/* Preview expose: generic, dialog based, used in preview widgets */
void pcbhl_expose_preview(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *e);



/* NULL terminated list of paths where the menu file should be looked at for */
extern const char *pcbhl_menu_file_paths[];

/* printf format string for the menu file name; may contain one %s that
   will be substituted with "default" or the HID's short name. */
extern const char *pcbhl_menu_name_fmt;

/* path to the user's config directory and main config file (CFR_USER) */
extern const char *pcbhl_conf_userdir_path;
extern const char *pcphl_conf_user_path;

/* path to the system (installed) config directory and main config file (CFR_SYSTEM) */
extern const char *pcbhl_conf_sysdir_path;
extern const char *pcbhl_conf_sys_path;

/* application information (to be displayed on the UI) */
extern const char *pcbhl_app_package;
extern const char *pcbhl_app_version;
extern const char *pcbhl_app_url;

#endif
