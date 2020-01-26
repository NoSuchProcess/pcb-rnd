/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/global_typedefs.h>
#include <librnd/core/pcb_bool.h>

typedef int pcb_toolid_t;
#define PCB_TOOLID_INVALID (-1)

typedef struct pcb_tool_cursor_s {
	const char *name;            /* if no custom graphics is provided, use a stock cursor by name */
	const unsigned char *pixel;  /* 32 bytes: 16*16 bitmap */
	const unsigned char *mask;   /* 32 bytes: 16*16 mask (1 means draw pixel) */
} pcb_tool_cursor_t;

#define PCB_TOOL_CURSOR_NAMED(name)       { name, NULL, NULL }
#define PCB_TOOL_CURSOR_XBM(pixel, mask)  { NULL, pixel, mask }

typedef enum pcb_tool_flags_e {
	PCB_TLF_AUTO_TOOLBAR = 1      /* automatically insert in the toolbar if the menu file didn't do it */
} pcb_tool_flags_t;

typedef struct pcb_tool_s {
	const char *name;             /* textual name of the tool */
	const char *help;             /* help/tooltip that explains the feature */
	const char *cookie;           /* plugin cookie _pointer_ of the registrar (comparision is pointer based, not strcmp) */
	unsigned int priority;        /* lower values are higher priorities; escaping mode will try to select the highest prio tool */
	const char **icon;            /* XPM for the tool buttons */
	pcb_tool_cursor_t cursor;     /* name of the mouse cursor to switch to when the tool is activated */
	pcb_tool_flags_t flags;

	/* tool implementation */
	void     (*init)(void);
	void     (*uninit)(void);
	void     (*press)(pcb_hidlib_t *hl);
	void     (*release)(pcb_hidlib_t *hl);
	void     (*adjust_attached)(pcb_hidlib_t *hl);
	void     (*draw_attached)(pcb_hidlib_t *hl);
	pcb_bool (*undo_act)(pcb_hidlib_t *hl);
	pcb_bool (*redo_act)(pcb_hidlib_t *hl);
	pcb_bool (*escape)(pcb_hidlib_t *hl);
	
	unsigned long user_flags;
} pcb_tool_t;

extern vtp0_t pcb_tools;
extern pcb_toolid_t pcb_tool_prev_id;
extern pcb_toolid_t pcb_tool_next_id;

/* (un)initialize the tool subsystem */
void pcb_tool_init(void);
void pcb_tool_uninit(void);

/* call this when the mode (tool) config node changes */
void pcb_tool_chg_mode(pcb_hidlib_t *hl);

/* Insert a new tool in pcb_tools; returns -1 on failure */
pcb_toolid_t pcb_tool_reg(pcb_tool_t *tool, const char *cookie);

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

/* Called after GUI_INIT; registers all mouse cursors in the GUI */
void pcb_tool_gui_init(void);


/**** Tool function wrappers; calling these will operate on the current tool 
      as defined in pcbhl_conf.editor.mode ****/

void pcb_tool_press(pcb_hidlib_t *hidlib);
void pcb_tool_release(pcb_hidlib_t *hidlib);
void pcb_tool_adjust_attached(pcb_hidlib_t *hl);
void pcb_tool_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_undo_act(pcb_hidlib_t *hl);
pcb_bool pcb_tool_redo_act(pcb_hidlib_t *hl);


/**** Low level, for internal use ****/

/* Get the tool pointer of a tool by id */
#define pcb_tool_get(id) ((const pcb_tool_t *)*vtp0_get(&pcb_tools, id, 0))

#endif
