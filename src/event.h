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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_EVENT_H
#define PCB_EVENT_H
#include "config.h"
#include "unit.h"
#include "global_typedefs.h"

typedef enum {
	PCB_EVENT_GUI_INIT,               /* finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	PCB_EVENT_CLI_ENTER,              /* the user pressed enter on a CLI command - called before parsing the line for actions; args: (str commandline) */
	PCB_EVENT_SAVE_PRE,               /* called before saving the design */
	PCB_EVENT_SAVE_POST,              /* called after saving the design */
	PCB_EVENT_LOAD_PRE,               /* called before loading a new design */
	PCB_EVENT_LOAD_POST,              /* called after loading a new design, whether it was successful or not */

	PCB_EVENT_BOARD_CHANGED,          /* called after the board being edited got _replaced_ (used to be the PCBChanged action) */
	PCB_EVENT_BOARD_META_CHANGED,     /* called if the metadata of the board has changed */
	PCB_EVENT_ROUTE_STYLES_CHANGED,   /* called after any route style change (used to be the RouteStylesChanged action) */
	PCB_EVENT_NETLIST_CHANGED,        /* called after any netlist change (used to be the NetlistChanged action) */
	PCB_EVENT_LAYERS_CHANGED,         /* called after layers or layer groups change (used to be the LayersChanged action) */
	PCB_EVENT_LAYER_CHANGED_GRP,      /* called after a layer changed its group; argument: layer pointer */
	PCB_EVENT_LAYERVIS_CHANGED,       /* called after the visibility of layers has changed */
	PCB_EVENT_LIBRARY_CHANGED,        /* called after a change in the footprint lib (used to be the LibraryChanged action) */
	PCB_EVENT_FONT_CHANGED,           /* called when a font has changed; argument is the font ID */

	PCB_EVENT_UNDO_POST,              /* called after an undo/redo operation; argument is an integer pcb_undo_ev_t */

	PCB_EVENT_NEW_PSTK,               /* called when a new padstack is created */

	PCB_EVENT_BUSY,                   /* called before CPU-intensive task begins */
	PCB_EVENT_LOG_APPEND,             /* called when a new log line is appended */

	PCB_EVENT_RUBBER_RESET,           /* rubber band: reset attached */
	PCB_EVENT_RUBBER_MOVE,            /* rubber band: object moved */
	PCB_EVENT_RUBBER_MOVE_DRAW,       /* rubber band: draw crosshair-attached rubber band objects after a move or copy */
	PCB_EVENT_RUBBER_ROTATE90,        /* rubber band: crosshair object rotated by 90 degrees */
	PCB_EVENT_RUBBER_ROTATE,          /* rubber band: crosshair object rotated by arbitrary angle */
	PCB_EVENT_RUBBER_LOOKUP_LINES,    /* rubber band: attach rubber banded line objects to crosshair */
	PCB_EVENT_RUBBER_LOOKUP_RATS,     /* rubber band: attach rubber banded rat lines objects to crosshair */
	PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, /* rubber band: adapt main line to keep rubberband lines direction */

	PCB_EVENT_GUI_SYNC,               /* sync full GUI state (e.g. after a menu clicked) */
	PCB_EVENT_GUI_SYNC_STATUS,        /* sync partial GUI state (status line update - do not update menus and do not redraw) */
	PCB_EVENT_GUI_LEAD_USER,          /* GUI aid to lead the user attention to a specific location on the board in the main drawing area; args: (coord x, coord y, int enabled) */
	PCB_EVENT_GUI_DRAW_OVERLAY_XOR,   /* called in board draw after finished drawing the xor marks, still in xor draw mode */
	PCB_EVENT_USER_INPUT_POST,        /* generated any time any user input reaches core, after processing it */

	PCB_EVENT_DRAW_CROSSHAIR_CHATT,   /* called from crosshair code upon attached object recalculation; event handlers can use this hook to enforce various geometric restrictions */

	PCB_EVENT_DRC_RUN,                /* called from core to run all configured DRCs (implemented in plugins) */
	PCB_EVENT_DAD_NEW_DIALOG,         /* called by the GUI after a new DAD dialog is open; args are pointer hid_ctx,  string dialog id and a pointer to int[4] for getting back preferre {x,y,w,h} (-1 means unknown) */
	PCB_EVENT_DAD_NEW_GEO,            /* called by the GUI after the window geometry got reconfigured; args are: void *hid_ctx, const char *dialog id, int x1, int y1, int width, int height */

	PCB_EVENT_NET_INDICATE_SHORT,     /* called by core to get a shortcircuit indicated (e.g. by mincut). Args: (pcb_net_t *net, pcb_any_obj_t *offending_term, pcb_net_t *offending_net, int *handled) - if *handled is non-zero, the short is already indicated */

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
struct pcb_event_arg_s {
	pcb_event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
		void *p;
		pcb_coord_t c;
		pcb_angle_t a;
	} d;
};

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
