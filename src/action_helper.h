/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef	PCB_ACTION_HELPER_H
#define	PCB_ACTION_HELPER_H

#include "global_typedefs.h"


/* Event handler to set the cursor according to the X pointer position
   called from inside main.c */
void pcb_event_move_crosshair(pcb_coord_t ev_x, pcb_coord_t ev_y);

/* adjusts the objects which are to be created like attached lines... */
void pcb_adjust_attached_objects(void);

#define PCB_ACTION_ARG(n) (argc > (n) ? argv[n] : NULL)


extern int defer_updates;
extern int defer_needs_update;



/* does what's appropriate for the current mode setting (when clicked). This
   normally means creation of an object at the current crosshair location.
   new created objects are added to the create undo list of course */
void pcb_notify_mode(void);

/* Clear warning color from pins/pads */
void pcb_clear_warnings(void);

void pcb_release_mode(void);

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define PCB_AFAIL(x) { pcb_message(PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

#define PCB_ACT_FAIL(x) { pcb_message(PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (pcb_acts_ ## x)); return 1; }

#endif
