/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for action routines */

#ifndef	PCB_ACTION_HELPER_H
#define	PCB_ACTION_HELPER_H

#include "global_typedefs.h"

/* Event handler to set the cursor according to the X pointer position
   called from inside main.c */
void pcb_event_move_crosshair(int ev_x, int ev_y);

/* adjusts the objects which are to be created like attached lines... */
void pcb_adjust_attached_objects(void);

#define PCB_ACTION_ARG(n) (argc > (n) ? argv[n] : NULL)

/* helper: get route style size for a function and selected object type.
   size_id: 0=main size; 1=2nd size (drill); 2=clearance */
int pcb_get_style_size(int funcid, pcb_coord_t * out, int type, int size_id);


extern int defer_updates;
extern int defer_needs_update;
extern pcb_layer_t *lastLayer;


void pcb_notify_line(void);       /* creates points of a line (when clicked) */
void pcb_notify_block(void);           /* create first or second corner of a marked block (when clicked) */

/* does what's appropriate for the current mode setting (when clicked). This
   normally means creation of an object at the current crosshair location.
   new created objects are added to the create undo list of course */
void pcb_notify_mode(void);

/* Clear warning color from pins/pads */
void pcb_clear_warnings(void);

typedef struct {
	pcb_coord_t X, Y;
	pcb_cardinal_t Buffer;
	pcb_bool Click;
	pcb_bool Moving;									/* selected type clicked on */
	int Hit;											/* move type clicked on */
	void *ptr1;
	void *ptr2;
	void *ptr3;
} pcb_action_note_t;

extern pcb_action_note_t Note;
extern pcb_bool saved_mode;

void pcb_release_mode(void);

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define PCB_AFAIL(x) { pcb_message(PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

#endif
