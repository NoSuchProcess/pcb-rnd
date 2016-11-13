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
typedef enum {
	EVENT_GUI_INIT,               /* finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	EVENT_CLI_ENTER,              /* the user pressed enter on a CLI command - called before parsing the line for actions; args: (str commandline) */
	EVENT_SAVE_PRE,               /* called before saving the design */
	EVENT_SAVE_POST,              /* called after saving the design */
	EVENT_LOAD_PRE,               /* called before loading a new design */
	EVENT_LOAD_POST,              /* called after loading a new design, whether it was successful or not */
	EVENT_last                    /* not a real event */
} pcb_event_id_t;

/* Maximum number of arguments for an event handler, auto-set argv[0] included */
#define EVENT_MAX_ARG 16

/* Argument types in event's argv[] */
typedef enum {
	ARG_INT,											/* format char: i */
	ARG_DOUBLE,										/* format char: d */
	ARG_STR												/* format char: s */
} pcb_event_argtype_t;


/* An argument is its type and value */
typedef struct {
	pcb_event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
	} d;
} pcb_event_arg_t;

/* Initialize the event system */
void pcb_events_init(void);

/* Uninitialize the event system and remove all events */
void pcb_events_uninit(void);


/* Event callback prototype; user_data is the same as in pcb_event_bind().
   argv[0] is always an ARG_INT with the event id that triggered the event. */
typedef void (pcb_event_handler_t) (void *user_data, int argc, pcb_event_arg_t * argv[]);

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
   format characters (e.g. i for ARG_INT). */
void pcb_event(pcb_event_id_t ev, const char *fmt, ...);
#endif
