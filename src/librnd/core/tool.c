/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019,2020 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <librnd/core/tool.h>

#include <librnd/core/hidlib_conf.h>
#include <librnd/core/event.h>
#include <librnd/core/hid.h>

#define PCB_MAX_MODESTACK_DEPTH         16  /* maximum depth of mode stack */

vtp0_t pcb_tools;

pcb_toolid_t pcb_tool_prev_id;
pcb_toolid_t pcb_tool_next_id;
static int save_position = 0;
static int save_stack[PCB_MAX_MODESTACK_DEPTH];

static void init_current_tool(void);
static void uninit_current_tool(void);

static int tool_select_lock = 0;

void pcb_tool_init(void)
{
	vtp0_init(&pcb_tools);
}

void pcb_tool_uninit(void)
{
	while(vtp0_len(&pcb_tools) != 0) {
		const pcb_tool_t *tool = pcb_tool_get(0);
		pcb_message(PCB_MSG_WARNING, "Unregistered tool: %s of %s; check your plugins, fix them to unregister their tools!\n", tool->name, tool->cookie);
		pcb_tool_unreg_by_cookie(tool->cookie);
	}
	vtp0_uninit(&pcb_tools);
}

void pcb_tool_chg_mode(pcb_hidlib_t *hl)
{
	if ((hl != NULL) && (!tool_select_lock))
		pcb_tool_select_by_id(hl, pcbhl_conf.editor.mode);
}

pcb_toolid_t pcb_tool_reg(pcb_tool_t *tool, const char *cookie)
{
	pcb_toolid_t id;
	if (pcb_tool_lookup(tool->name) != PCB_TOOLID_INVALID) /* don't register two tools with the same name */
		return -1;
	tool->cookie = cookie;
	id = pcb_tools.used;
	vtp0_append(&pcb_tools, (void *)tool);
	if (pcb_gui != NULL)
		pcb_gui->reg_mouse_cursor(pcb_gui, id, tool->cursor.name, tool->cursor.pixel, tool->cursor.mask);
	pcb_event(NULL, PCB_EVENT_TOOL_REG, "p", tool);
	return id;
}

void pcb_tool_unreg_by_cookie(const char *cookie)
{
	pcb_toolid_t n;
	for(n = 0; n < vtp0_len(&pcb_tools); n++) {
		const pcb_tool_t *tool = (const pcb_tool_t *)pcb_tools.array[n];
		if (tool->cookie == cookie) {
			vtp0_remove(&pcb_tools, n, 1);
			n--;
		}
	}
}

pcb_toolid_t pcb_tool_lookup(const char *name)
{
	pcb_toolid_t n;
	for(n = 0; n < vtp0_len(&pcb_tools); n++) {
		const pcb_tool_t *tool = (const pcb_tool_t *)pcb_tools.array[n];
		if (strcmp(tool->name, name) == 0)
			return n;
	}
	return PCB_TOOLID_INVALID;
}

int pcb_tool_select_by_name(pcb_hidlib_t *hidlib, const char *name)
{
	pcb_toolid_t id = pcb_tool_lookup(name);
	if (id == PCB_TOOLID_INVALID)
		return -1;
	return pcb_tool_select_by_id(hidlib, id);
}

int pcb_tool_select_by_id(pcb_hidlib_t *hidlib, pcb_toolid_t id)
{
	char id_s[32];
	static pcb_bool recursing = pcb_false;
	int ok = 1;
	
	if ((id < 0) || (id > vtp0_len(&pcb_tools)))
		return -1;
	
	/* protect the cursor while changing the mode
	 * perform some additional stuff depending on the new mode
	 * reset 'state' of attached objects
	 */
	if (recursing)
		return -1;

	/* check if the UI logic allows picking that tool */
	pcb_event(hidlib, PCB_EVENT_TOOL_SELECT_PRE, "pi", &ok, id);
	if (ok == 0)
		id = pcbhl_conf.editor.mode;

	recursing = pcb_true;

	pcb_tool_prev_id = pcbhl_conf.editor.mode;
	pcb_tool_next_id = id;
	uninit_current_tool();
	sprintf(id_s, "%d", id);
	tool_select_lock = 1;
	pcb_conf_set(CFR_DESIGN, "editor/mode", -1, id_s, POL_OVERWRITE);
	tool_select_lock = 0;
	init_current_tool();

	recursing = pcb_false;

	if (pcb_gui != NULL)
		pcb_gui->set_mouse_cursor(pcb_gui, id);
	return 0;
}

int pcb_tool_select_highest(pcb_hidlib_t *hidlib)
{
	pcb_toolid_t n, bestn = PCB_TOOLID_INVALID;
	unsigned int bestp = -1;
	for(n = 0; n < vtp0_len(&pcb_tools) && (bestp > 0); n++) {
		const pcb_tool_t *tool = (const pcb_tool_t *)pcb_tools.array[n];
		if (tool->priority < bestp) {
			bestp = tool->priority;
			bestn = n;
		}
	}
	if (bestn == PCB_TOOLID_INVALID)
		return -1;
	return pcb_tool_select_by_id(hidlib, bestn);
}

int pcb_tool_save(pcb_hidlib_t *hidlib)
{
	save_stack[save_position] = pcbhl_conf.editor.mode;
	if (save_position < PCB_MAX_MODESTACK_DEPTH - 1)
		save_position++;
	else
		return -1;
	return 0;
}

int pcb_tool_restore(pcb_hidlib_t *hidlib)
{
	if (save_position == 0) {
		pcb_message(PCB_MSG_ERROR, "hace: underflow of restore mode\n");
		return -1;
	}
	return pcb_tool_select_by_id(hidlib, save_stack[--save_position]);
}

void pcb_tool_gui_init(void)
{
	pcb_toolid_t n;
	pcb_tool_t **tool;

	if (pcb_gui == NULL)
		return;

	for(n = 0, tool = (pcb_tool_t **)pcb_tools.array; n < pcb_tools.used; n++,tool++)
		if (*tool != NULL)
			pcb_gui->reg_mouse_cursor(pcb_gui, n, (*tool)->cursor.name, (*tool)->cursor.pixel, (*tool)->cursor.mask);
}

/**** current tool function wrappers ****/
#define wrap(func, err_ret, prefix, args) \
	do { \
		const pcb_tool_t *tool; \
		if ((pcbhl_conf.editor.mode < 0) || (pcbhl_conf.editor.mode >= vtp0_len(&pcb_tools))) \
			{ err_ret; } \
		tool = (const pcb_tool_t *)pcb_tools.array[pcbhl_conf.editor.mode]; \
		if (tool->func == NULL) \
			{ err_ret; } \
		prefix tool->func args; \
	} while(0)

#define wrap_void(func, args)           wrap(func, return,  ;,      args)
#define wrap_retv(func, err_ret, args)  wrap(func, err_ret, return, args)

static void init_current_tool(void)
{
	wrap_void(init, ());
}

static void uninit_current_tool(void)
{
	wrap_void(uninit, ());
}

void pcb_tool_notify_mode(pcb_hidlib_t *hidlib)
{
	wrap_void(notify_mode, (hidlib));
}

void pcb_tool_release_mode(pcb_hidlib_t *hidlib)
{
	wrap_void(release_mode, (hidlib));
}

void pcb_tool_adjust_attached_objects(pcb_hidlib_t *hl)
{
	wrap_void(adjust_attached_objects, (hl));
}

void pcb_tool_draw_attached(pcb_hidlib_t *hl)
{
	wrap_void(draw_attached, (hl));
}

pcb_bool pcb_tool_undo_act(pcb_hidlib_t *hl)
{
	wrap_retv(undo_act, return pcb_true, (hl));
}

pcb_bool pcb_tool_redo_act(pcb_hidlib_t *hl)
{
	wrap_retv(redo_act, return pcb_true, (hl));
}


