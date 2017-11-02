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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef PCB_TOOL_H
#define PCB_TOOL_H

#include <genvector/vtp0.h>

#include "pcb_bool.h"

typedef int pcb_toolid_t;
#define PCB_TOOLID_INVALID (-1)

typedef struct pcb_tool_s {
	const char *name;             /* textual name of the tool */
	const char *cookie;           /* plugin cookie _pointer_ of the registrar (comparision is pointer based, not strcmp) */
	unsigned int priority;        /* lower values are higher priorities; escaping mode will try to select the highest prio tool */

	/* tool implementation */
	void     (*notify_mode)(void);
	void     (*adjust_attached_objects)(void);
	pcb_bool (*undo_act)(void);
	pcb_bool (*redo_act)(void);
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
int pcb_tool_select_by_name(const char *name);
int pcb_tool_select_by_id(pcb_toolid_t id);
int pcb_tool_select_highest(void);

/**** Tool function wrappers; calling these will operate on the current tool 
      as defined in conf_core.editor.mode ****/

void pcb_tool_notify_mode(void);
void pcb_tool_adjust_attached_objects(void);
pcb_bool pcb_tool_undo_act(void);
pcb_bool pcb_tool_redo_act(void);



/**** Low level, for internal use ****/

/* Get the tool pointer of a tool by id */
#define pcb_tool_get(id) ((const pcb_tool_t *)vtp0_get(&pcb_tools, id, 0))

#endif
