/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

#ifndef RND_EVENT_H
#define RND_EVENT_H
#include <librnd/config.h>
#include <librnd/core/unit.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/hidlib.h>

typedef enum {
	PCB_EVENT_GUI_INIT,               /* finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	PCB_EVENT_CLI_ENTER,              /* the user pressed enter on a CLI command - called before parsing the line for actions; args: (str commandline) */

	PCB_EVENT_TOOL_REG,               /* called after a new tool has been registered; arg is (pcb_tool_t *) of the new tool */
	PCB_EVENT_TOOL_SELECT_PRE,        /* called before a tool is selected; arg is (int *ok, int toolid); if ok is non-zero if the selection is accepted */
	PCB_EVENT_TOOL_RELEASE,           /* no arg */
	PCB_EVENT_TOOL_PRESS,             /* no arg */

	PCB_EVENT_BUSY,                   /* called before/after CPU-intensive task begins; argument is an integer: 1 is before, 0 is after */
	PCB_EVENT_LOG_APPEND,             /* called after a new log line is appended; arg is a pointer to the log line */
	PCB_EVENT_LOG_CLEAR,              /* called after a clear; args: two pointers; unsigned long "from" and "to" */

	PCB_EVENT_GUI_LEAD_USER,          /* GUI aid to lead the user attention to a specific location on the board in the main drawing area; args: (coord x, coord y, int enabled) */
	PCB_EVENT_GUI_DRAW_OVERLAY_XOR,   /* called in board draw after finished drawing the xor marks, still in xor draw mode; argument is a pointer to the GC to use for drawing */
	PCB_EVENT_USER_INPUT_POST,        /* generated any time any user input reaches core, after processing it */
	PCB_EVENT_USER_INPUT_KEY,         /* generated any time a keypress is registered by the hid_cfg_key code (may be one key stroke of a multi-stroke sequence) */

	PCB_EVENT_CROSSHAIR_MOVE,         /* called when the crosshair changed coord; arg is an integer which is zero if more to come */

	PCB_EVENT_DAD_NEW_DIALOG,         /* called by the GUI after a new DAD dialog is open; args are pointer hid_ctx,  string dialog id and a pointer to int[4] for getting back preferre {x,y,w,h} (-1 means unknown) */
	PCB_EVENT_DAD_NEW_GEO,            /* called by the GUI after the window geometry got reconfigured; args are: void *hid_ctx, const char *dialog id, int x1, int y1, int width, int height */

	PCB_EVENT_EXPORT_SESSION_BEGIN,   /* called before an export session (e.g. CAM script execution) starts; should not be nested; there's no guarantee that options are parsed before or after this event */
	PCB_EVENT_EXPORT_SESSION_END,     /* called after an export session (e.g. CAM script execution) ends */

	PCB_EVENT_STROKE_START,           /* parameters: rnd_coord_t x, rnd_coord_t y */
	PCB_EVENT_STROKE_RECORD,          /* parameters: rnd_coord_t x, rnd_coord_t y */
	PCB_EVENT_STROKE_FINISH,          /* parameters: int *handled; if it is non-zero, stroke has handled the request and Tool() should return 1, breaking action script execution */

	PCB_EVENT_BOARD_CHANGED,          /* called after the board being edited got _replaced_ (used to be the PCBChanged action) */
	PCB_EVENT_BOARD_META_CHANGED,     /* called if the metadata of the board has changed */
	PCB_EVENT_BOARD_FN_CHANGED,       /* called after the file name of the board has changed */

	PCB_EVENT_SAVE_PRE,               /* called before saving the design (required for window placement) */
	PCB_EVENT_SAVE_POST,              /* called after saving the design (required for window placement) */
	PCB_EVENT_LOAD_PRE,               /* called before loading a new design (required for window placement) */
	PCB_EVENT_LOAD_POST,              /* called after loading a new design, whether it was successful or not (required for window placement) */

	RND_EVENT_last                    /* not a real event */
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
struct pcb_event_arg_s {
	pcb_event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
		void *p;
		rnd_coord_t c;
		pcb_angle_t a;
	} d;
};

/* Initialize the event system */
void pcb_events_init(void);

/* Uninitialize the event system and remove all events */
void pcb_events_uninit(void);


/* Event callback prototype; user_data is the same as in pcb_event_bind().
   argv[0] is always an PCB_EVARG_INT with the event id that triggered the event. */
typedef void (pcb_event_handler_t)(rnd_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);

/* Bind: add a handler to the call-list of an event; the cookie is also remembered
   so that mass-unbind is easier later. user_data is passed to the handler. */
void pcb_event_bind(pcb_event_id_t ev, pcb_event_handler_t *handler, void *user_data, const char *cookie);

/* Unbind: remove a handler from an event */
void pcb_event_unbind(pcb_event_id_t ev, pcb_event_handler_t *handler);

/* Unbind by cookie: remove all handlers from an event matching the cookie */
void pcb_event_unbind_cookie(pcb_event_id_t ev, const char *cookie);

/* Unbind all by cookie: remove all handlers from all events matching the cookie */
void pcb_event_unbind_allcookie(const char *cookie);

/* Event trigger: call all handlers for an event. Fmt is a list of
   format characters (e.g. i for PCB_EVARG_INT). */
void pcb_event(rnd_hidlib_t *hidlib, pcb_event_id_t ev, const char *fmt, ...);

/* Return the name of an event as seen on regisrtation */
const char *pcb_event_name(pcb_event_id_t ev);

/* The application may register its events by defining an enum
   starting from RND_EVENT_app and calling this function after hidlib_init1() */
void pcb_event_app_reg(long last_event_id, const char **event_names, long sizeof_event_names);
#define RND_EVENT_app 100

#endif
