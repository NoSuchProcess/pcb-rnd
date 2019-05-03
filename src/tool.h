/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#ifndef PCB_TOOL_H
#define PCB_TOOL_H

#include <genvector/vtp0.h>

#include "global_typedefs.h"
#include "pcb_bool.h"

typedef enum {
	PCB_MODE_VIA             = 15,  /* draw vias */
	PCB_MODE_LINE            = 5,   /* draw lines */
	PCB_MODE_RECTANGLE       = 10,  /* create rectangles */
	PCB_MODE_POLYGON         = 8,   /* draw filled polygons */
	PCB_MODE_PASTE_BUFFER    = 2,   /* paste objects from buffer */
	PCB_MODE_TEXT            = 13,  /* create text objects */
	PCB_MODE_ROTATE          = 12,  /* rotate objects */
	PCB_MODE_REMOVE          = 11,  /* remove objects */
	PCB_MODE_MOVE            = 7,   /* move objects */
	PCB_MODE_COPY            = 3,   /* copy objects */
	PCB_MODE_INSERT_POINT    = 4,   /* insert point into line/polygon */
	PCB_MODE_RUBBERBAND_MOVE = 16,  /* move objects and attached lines */
	PCB_MODE_THERMAL         = 14,  /* toggle thermal layer flag */
	PCB_MODE_ARC             = 0,   /* draw arcs */
	PCB_MODE_ARROW           = 1,   /* selection with arrow mode */
	PCB_MODE_LOCK            = 6,   /* lock/unlock objects */
	PCB_MODE_POLYGON_HOLE    = 9    /* cut holes in filled polygons */
} pcb_mode_t;

typedef int pcb_toolid_t;
#define PCB_TOOLID_INVALID (-1)

typedef struct pcb_tool_s {
	const char *name;             /* textual name of the tool */
	const char *cookie;           /* plugin cookie _pointer_ of the registrar (comparision is pointer based, not strcmp) */
	unsigned int priority;        /* lower values are higher priorities; escaping mode will try to select the highest prio tool */
	const char **icon;            /* XPM for the tool buttons */

	/* tool implementation */
	void     (*init)(void);
	void     (*uninit)(void);
	void     (*notify_mode)(void);
	void     (*release_mode)(void);
	void     (*adjust_attached_objects)(void);
	void     (*draw_attached)(void);
	pcb_bool (*undo_act)(void);
	pcb_bool (*redo_act)(void);
	
	pcb_bool allow_when_drawing_ratlines;
} pcb_tool_t;

vtp0_t pcb_tools;

/* (un)initialize the tool subsystem */
void pcb_tool_init(void);
void pcb_tool_uninit(void);

/* Insert a new tool in pcb_tools; returns 0 on success */
int pcb_tool_reg(pcb_tool_t *tool, const char *cookie);

/* Unregister all tools that has matching cookie */
void pcb_tool_unreg_by_cookie(const char *cookie);

/* Return the ID of a tool by name; returns -1 on error */
pcb_toolid_t pcb_tool_lookup(const char *name);


/* Select a tool by name, id or pick the highest prio tool; return 0 on success */
int pcb_tool_select_by_name(pcb_hidlib_t *hidlib, const char *name);
int pcb_tool_select_by_id(pcb_hidlib_t *hidlib, pcb_toolid_t id);
int pcb_tool_select_highest(pcb_hidlib_t *hidlib);

int pcb_tool_save(pcb_hidlib_t *hidlib);
int pcb_tool_restore(pcb_hidlib_t *hidlib);

/**** Tool function wrappers; calling these will operate on the current tool 
      as defined in conf_core.editor.mode ****/

void pcb_tool_notify_mode(void);
void pcb_tool_adjust_attached_objects(void);
void pcb_tool_draw_attached(void);
pcb_bool pcb_tool_undo_act(void);
pcb_bool pcb_tool_redo_act(void);


/**** tool helper functions ****/

typedef struct {
	pcb_coord_t X, Y;
	pcb_cardinal_t Buffer;	/* buffer number */
	pcb_bool Click;		/* true if clicked somewhere with the arrow tool */
	pcb_bool Moving;	/* true if clicked on an object of PCB_SELECT_TYPES */
	int Hit;					/* type of a hit object of PCB_MOVE_TYPES; 0 if there was no PCB_MOVE_TYPES object under the crosshair */
	void *ptr1;
	void *ptr2;
	void *ptr3;
} pcb_tool_note_t;

extern pcb_tool_note_t pcb_tool_note;
extern pcb_bool pcb_tool_is_saved;
extern pcb_toolid_t pcb_tool_prev_id;
extern pcb_toolid_t pcb_tool_next_id;

void pcb_tool_attach_for_copy(pcb_coord_t PlaceX, pcb_coord_t PlaceY, pcb_bool do_rubberband);
void pcb_tool_notify_block(void);	/* create first or second corner of a marked block (when clicked) */
pcb_bool pcb_tool_should_snap_offgrid_line(pcb_layer_t *layer, pcb_line_t *line);


/**** old helpers ****/

/* does what's appropriate for the current mode setting (when clicked). This
   normally means creation of an object at the current crosshair location.
   new created objects are added to the create undo list of course */
void pcb_notify_mode(pcb_hidlib_t *hidlib);

void pcb_release_mode(pcb_hidlib_t *hidlib);


/**** Low level, for internal use ****/

/* Get the tool pointer of a tool by id */
#define pcb_tool_get(id) ((const pcb_tool_t *)*vtp0_get(&pcb_tools, id, 0))

#endif
