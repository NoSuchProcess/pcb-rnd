/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

#ifndef PCB_EVENT_H
#define PCB_EVENT_H
#include "config.h"
#include "unit.h"

typedef enum {
	PCB_EVENT_GUI_INIT,               /* finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	PCB_EVENT_CLI_ENTER,              /* the user pressed enter on a CLI command - called before parsing the line for actions; args: (str commandline) */
	PCB_EVENT_SAVE_PRE,               /* called before saving the design */
	PCB_EVENT_SAVE_POST,              /* called after saving the design */
	PCB_EVENT_LOAD_PRE,               /* called before loading a new design */
	PCB_EVENT_LOAD_POST,              /* called after loading a new design, whether it was successful or not */

	PCB_EVENT_RUBBER_RESET,           /* rubber band: reset attached */
	PCB_EVENT_RUBBER_REMOVE_ELEMENT,  /* rubber band: removed an element with rubber bands attached */

	PCB_EVENT_last                    /* not a real event */
} pcb_event_id_t;

/* Maximum number of arguments for an event handler, auto-set argv[0] included */
#define EVENT_MAX_ARG 16

/* Argument types in event's argv[] */
typedef enum {
	PCB_EVARG_INT,											/* format char: i */
	PCB_EVARG_DOUBLE,										/* format char: d */
	PCB_EVARG_STR,											/* format char: s */
	PCB_EVARG_PTR,											/* format char: p */
	PCB_EVARG_COORD,										/* format char: c */
	PCB_EVARG_ANGLE											/* format char: a */
} pcb_event_argtype_t;


/* An argument is its type and value */
typedef struct {
	pcb_event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
		void *p;
		pcb_coord_t c;
		pcb_angle_t a;
	} d;
} pcb_event_arg_t;

/* Initialize the event system */
void pcb_events_init(void);

/* Uninitialize the event system and remove all events */
void pcb_events_uninit(void);


/* Event callback prototype; user_data is the same as in pcb_event_bind().
   argv[0] is always an PCB_EVARG_INT with the event id that triggered the event. */
typedef void (pcb_event_handler_t) (void *user_data, int argc, pcb_event_arg_t argv[]);

/* Bind: add a handler to the call-list of an event; the cookie is also remembered
   so that mass-unbind is easier later. user_data is passed to the handler. */
void pcb_event_bind(pcb_event_id_t ev, pcb_event_handler_t * handler, void *user_data, const char *cookie);

/* Unbind: remove a handler from an event */
void pcb_event_unbind(pcb_event_id_t ev, pcb_event_handler_t * handler);

/* Unbind by cookie: remove all handlers from an event matching the cookie */
void pcb_event_unbind_cookie(pcb_event_id_t ev, const char *cookie);

/* Unbind all by cookie: remove all handlers from all events matching the cookie */
void pcb_event_unbind_allcookie(const char *cookie);

/* Event trigger: call all handlers for an event. Fmt is a list of
   format characters (e.g. i for PCB_EVARG_INT). */
void pcb_event(pcb_event_id_t ev, const char *fmt, ...);
#endif
