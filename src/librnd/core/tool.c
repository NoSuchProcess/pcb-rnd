/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>

#define PCB_MAX_MODESTACK_DEPTH         16  /* maximum depth of mode stack */

rnd_bool pcb_tool_is_saved = pcb_false;

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
		rnd_message(PCB_MSG_WARNING, "Unregistered tool: %s of %s; check your plugins, fix them to unregister their tools!\n", tool->name, tool->cookie);
		pcb_tool_unreg_by_cookie(tool->cookie);
	}
	vtp0_uninit(&pcb_tools);
}

void pcb_tool_chg_mode(rnd_hidlib_t *hl)
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

int pcb_tool_select_by_name(rnd_hidlib_t *hidlib, const char *name)
{
	pcb_toolid_t id = pcb_tool_lookup(name);
	if (id == PCB_TOOLID_INVALID)
		return -1;
	return pcb_tool_select_by_id(hidlib, id);
}

int pcb_tool_select_by_id(rnd_hidlib_t *hidlib, pcb_toolid_t id)
{
	char id_s[32];
	static rnd_bool recursing = pcb_false;
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
	rnd_conf_set(RND_CFR_DESIGN, "editor/mode", -1, id_s, RND_POL_OVERWRITE);
	tool_select_lock = 0;
	init_current_tool();

	recursing = pcb_false;

	if (pcb_gui != NULL)
		pcb_gui->set_mouse_cursor(pcb_gui, id);
	return 0;
}

int pcb_tool_select_highest(rnd_hidlib_t *hidlib)
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

int pcb_tool_save(rnd_hidlib_t *hidlib)
{
	save_stack[save_position] = pcbhl_conf.editor.mode;
	if (save_position < PCB_MAX_MODESTACK_DEPTH - 1)
		save_position++;
	else
		return -1;
	return 0;
}

int pcb_tool_restore(rnd_hidlib_t *hidlib)
{
	if (save_position == 0) {
		rnd_message(PCB_MSG_ERROR, "hace: underflow of restore mode\n");
		return -1;
	}
	return pcb_tool_select_by_id(hidlib, save_stack[--save_position]);
}

void rnd_tool_gui_init(void)
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

void pcb_tool_press(rnd_hidlib_t *hidlib)
{
	wrap_void(press, (hidlib));
}

void pcb_tool_release(rnd_hidlib_t *hidlib)
{
	wrap_void(release, (hidlib));
}

void pcb_tool_adjust_attached(rnd_hidlib_t *hl)
{
	wrap_void(adjust_attached, (hl));
}

void pcb_tool_draw_attached(rnd_hidlib_t *hl)
{
	wrap_void(draw_attached, (hl));
}

rnd_bool pcb_tool_undo_act(rnd_hidlib_t *hl)
{
	wrap_retv(undo_act, return pcb_true, (hl));
}

rnd_bool pcb_tool_redo_act(rnd_hidlib_t *hl)
{
	wrap_retv(redo_act, return pcb_true, (hl));
}

static void do_release(rnd_hidlib_t *hidlib)
{
	if (pcbhl_conf.temp.click_cmd_entry_active && (rnd_cli_mouse(hidlib, 0) == 0))
		return;

	hidlib->tool_grabbed.status = pcb_false;

	pcb_tool_release(hidlib);

	if (pcb_tool_is_saved)
		pcb_tool_restore(hidlib);
	pcb_tool_is_saved = pcb_false;
	pcb_event(hidlib, PCB_EVENT_TOOL_RELEASE, NULL);
}

void pcb_tool_do_press(rnd_hidlib_t *hidlib)
{
	if (pcbhl_conf.temp.click_cmd_entry_active && (rnd_cli_mouse(hidlib, 1) == 0))
		return;

	hidlib->tool_grabbed.X = hidlib->tool_x;
	hidlib->tool_grabbed.Y = hidlib->tool_y;

	pcb_tool_press(hidlib);
	pcb_event(hidlib, PCB_EVENT_TOOL_PRESS, NULL);
}


static const char pcb_acts_Tool[] =
	"Tool(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
	"Tool(Poly|Rectangle|Remove|Rotate|Text|Thermal|Via)\n" "Tool(Press|Release|Cancel|Stroke)\n" "Tool(Save|Restore)";

static const char pcb_acth_Tool[] = "Change or use the tool mode.";
/* DOC: tool.html */
static fgw_error_t pcb_act_Tool(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hidlib_t *hidlib = RND_ACT_HIDLIB;
	const char *cmd;
	RND_ACT_IRES(0);
	RND_PCB_ACT_CONVARG(1, FGW_STR, Tool, cmd = argv[1].val.str);

	/* it is okay to use crosshair directly here, the mode command is called from a click when it needs coords */
	hidlib->tool_x = hidlib->ch_x;
	hidlib->tool_y = hidlib->ch_y;
	pcb_hid_notify_crosshair_change(RND_ACT_HIDLIB, pcb_false);
	if (rnd_strcasecmp(cmd, "Cancel") == 0) {
		pcb_tool_select_by_id(RND_ACT_HIDLIB, pcbhl_conf.editor.mode);
	}
	else if (rnd_strcasecmp(cmd, "Escape") == 0) {
		const pcb_tool_t *t;
		escape:;
		t = pcb_tool_get(pcbhl_conf.editor.mode);
		if ((t == NULL) || (t->escape == NULL)) {
			pcb_tool_select_by_name(RND_ACT_HIDLIB, "arrow");
			hidlib->tool_hit = hidlib->tool_click = 0; /* if the mouse button is still pressed, don't start selecting a box */
		}
		else
			t->escape(RND_ACT_HIDLIB);
	}
	else if ((rnd_strcasecmp(cmd, "Press") == 0) || (rnd_strcasecmp(cmd, "Notify") == 0)) {
		pcb_tool_do_press(RND_ACT_HIDLIB);
	}
	else if (rnd_strcasecmp(cmd, "Release") == 0) {
		if (pcbhl_conf.editor.enable_stroke) {
			int handled = 0;
			pcb_event(RND_ACT_HIDLIB, PCB_EVENT_STROKE_FINISH, "p", &handled);
			if (handled) {
			/* Ugly hack: returning 1 here will break execution of the
			   action script, so actions after this one could do things
			   that would be executed only after non-recognized gestures */
				do_release(RND_ACT_HIDLIB);
				pcb_hid_notify_crosshair_change(RND_ACT_HIDLIB, pcb_true);
				return 1;
			}
		}
		do_release(RND_ACT_HIDLIB);
	}
	else if (rnd_strcasecmp(cmd, "Stroke") == 0) {
		if (pcbhl_conf.editor.enable_stroke)
			pcb_event(RND_ACT_HIDLIB, PCB_EVENT_STROKE_START, "cc", hidlib->tool_x, hidlib->tool_y);
		else
			goto escape; /* Right mouse button restarts drawing mode. */
	}
	else if (rnd_strcasecmp(cmd, "Restore") == 0) { /* restore the last saved tool */
		pcb_tool_restore(RND_ACT_HIDLIB);
	}
	else if (rnd_strcasecmp(cmd, "Save") == 0) { /* save currently selected tool */
		pcb_tool_save(RND_ACT_HIDLIB);
	}
	else {
		if (pcb_tool_select_by_name(RND_ACT_HIDLIB, cmd) != 0)
			rnd_message(PCB_MSG_ERROR, "No such tool: '%s'\n", cmd);
	}
	pcb_hid_notify_crosshair_change(RND_ACT_HIDLIB, pcb_true);
	return 0;
}

static rnd_action_t tool_action_list[] = {
	{"Tool", pcb_act_Tool, pcb_acth_Tool, pcb_acts_Tool},
	{"Mode", pcb_act_Tool, pcb_acth_Tool, pcb_acts_Tool}
};

void rnd_tool_act_init2(void)
{
	RND_REGISTER_ACTIONS(tool_action_list, NULL);
}


