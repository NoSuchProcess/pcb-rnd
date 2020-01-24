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

#include "tool.h"

#include "board.h"
#include <librnd/core/hidlib_conf.h>
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include "find.h"
#include <librnd/core/grid.h>
#include "undo.h"
#include <librnd/core/actions.h>
#include <librnd/core/conf_hid.h>


pcb_toolid_t pcb_tool_prev_id;
pcb_toolid_t pcb_tool_next_id;
static int save_position = 0;
static int save_stack[PCB_MAX_MODESTACK_DEPTH];

static void default_tool_reg(void);
static void default_tool_unreg(void);
static void init_current_tool(void);
static void uninit_current_tool(void);

static conf_hid_id_t tool_conf_id;
static const char *pcb_tool_cookie = "default tools";
static int tool_select_lock = 0;

void tool_chg_mode(conf_native_t *cfg, int arr_idx)
{
	if ((PCB != NULL) && (!tool_select_lock))
		pcb_tool_select_by_id(&PCB->hidlib, pcbhl_conf.editor.mode);
}

void pcb_tool_init()
{
	static conf_hid_callbacks_t cbs_mode;
	conf_native_t *n_mode = pcb_conf_get_field("editor/mode");

	vtp0_init(&pcb_tools);

	tool_conf_id = pcb_conf_hid_reg(pcb_tool_cookie, NULL);

	if (n_mode != NULL) {
		memset(&cbs_mode, 0, sizeof(conf_hid_callbacks_t));
		cbs_mode.val_change_post = tool_chg_mode;
		pcb_conf_hid_set_cb(n_mode, tool_conf_id, &cbs_mode);
	}
}

void pcb_tool_uninit()
{
	while(vtp0_len(&pcb_tools) != 0) {
		const pcb_tool_t *tool = pcb_tool_get(0);
		pcb_message(PCB_MSG_WARNING, "Unregistered tool: %s of %s; check your plugins, fix them to unregister their tools!\n", tool->name, tool->cookie);
		pcb_tool_unreg_by_cookie(tool->cookie);
	}
	vtp0_uninit(&pcb_tools);
}

void pcb_tool_uninit_conf(void)
{
	pcb_conf_hid_unreg(pcb_tool_cookie);
}


int pcb_tool_reg(pcb_tool_t *tool, const char *cookie)
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
	return 0;
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
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	char id_s[32];
	static pcb_bool recursing = pcb_false;
	
	if ((id < 0) || (id > vtp0_len(&pcb_tools)))
		return -1;
	
	/* protect the cursor while changing the mode
	 * perform some additional stuff depending on the new mode
	 * reset 'state' of attached objects
	 */
	if (recursing)
		return -1;
	recursing = pcb_true;
	
	if (pcb->RatDraw && !pcb_tool_get(id)->allow_when_drawing_ratlines) {
		pcb_message(PCB_MSG_WARNING, "That mode is NOT allowed when drawing ratlines!\n");
		id = PCB_MODE_ARROW;
	}
	
	pcb_tool_prev_id = pcbhl_conf.editor.mode;
	pcb_tool_next_id = id;
	uninit_current_tool();
	sprintf(id_s, "%d", id);
	tool_select_lock = 1;
	pcb_conf_set(CFR_DESIGN, "editor/mode", -1, id_s, POL_OVERWRITE);
	tool_select_lock = 0;
	init_current_tool();

	recursing = pcb_false;

	/* force a crosshair grid update because the valid range may have changed */
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair_move_relative(0, 0);
	pcb_notify_crosshair_change(pcb_true);
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


/**** tool helper functions ****/

pcb_tool_note_t pcb_tool_note;
pcb_bool pcb_tool_is_saved = pcb_false;

static void get_grid_lock_coordinates(int type, void *ptr1, void *ptr2, void *ptr3, pcb_coord_t * x, pcb_coord_t * y)
{
	switch (type) {
	case PCB_OBJ_LINE:
		*x = ((pcb_line_t *) ptr2)->Point1.X;
		*y = ((pcb_line_t *) ptr2)->Point1.Y;
		break;
	case PCB_OBJ_TEXT:
		*x = ((pcb_text_t *) ptr2)->X;
		*y = ((pcb_text_t *) ptr2)->Y;
		break;
	case PCB_OBJ_POLY:
		*x = ((pcb_poly_t *) ptr2)->Points[0].X;
		*y = ((pcb_poly_t *) ptr2)->Points[0].Y;
		break;

	case PCB_OBJ_LINE_POINT:
	case PCB_OBJ_POLY_POINT:
		*x = ((pcb_point_t *) ptr3)->X;
		*y = ((pcb_point_t *) ptr3)->Y;
		break;
	case PCB_OBJ_ARC:
		pcb_arc_get_end((pcb_arc_t *) ptr2, 0, x, y);
		break;
	case PCB_OBJ_ARC_POINT:
		if (ptr3 != NULL) /* need to check because: if snap off, there's no known endpoint (leave x;y as is, then) */
			pcb_arc_get_end((pcb_arc_t *) ptr2, ((*(int **)ptr3) != pcb_arc_start_ptr), x, y);
		break;
	}
}

void pcb_tool_attach_for_copy(pcb_hidlib_t *hl, pcb_coord_t PlaceX, pcb_coord_t PlaceY, pcb_bool do_rubberband)
{
	pcb_coord_t mx = 0, my = 0;

	pcb_event(hl, PCB_EVENT_RUBBER_RESET, NULL);
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		get_grid_lock_coordinates(pcb_crosshair.AttachedObject.Type,
															pcb_crosshair.AttachedObject.Ptr1,
															pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = pcb_grid_fit(mx, hl->grid, hl->grid_ox) - mx;
		my = pcb_grid_fit(my, hl->grid, hl->grid_oy) - my;
	}
	pcb_crosshair.AttachedObject.X = PlaceX - mx;
	pcb_crosshair.AttachedObject.Y = PlaceY - my;
	if ((!pcb_marked.status || conf_core.editor.local_ref) && !pcb_marked.user_placed)
		pcb_crosshair_set_local_ref(PlaceX - mx, PlaceY - my, pcb_true);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;

	/* get all attached objects if necessary */
	if (do_rubberband && conf_core.editor.rubber_band_mode)
		pcb_event(hl, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	if (do_rubberband &&
			(pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_PSTK ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE || pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT))
		pcb_event(hl, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
}

void pcb_tool_notify_block(void)
{
	pcb_notify_crosshair_change(pcb_false);
	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_FIRST:						/* setup first point */
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.Y;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:						/* setup second point */
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_THIRD;
		break;
	}
	pcb_notify_crosshair_change(pcb_true);
}

/*** old helpers ***/
void pcb_release_mode(pcb_hidlib_t *hidlib)
{
	if (pcbhl_conf.temp.click_cmd_entry_active && (pcb_cli_mouse(hidlib, 0) == 0))
		return;

	pcb_grabbed.status = pcb_false;
	pcb_tool_release_mode(hidlib);

	if (pcb_tool_is_saved)
		pcb_tool_restore(hidlib);
	pcb_tool_is_saved = pcb_false;
	pcb_draw();
}

void pcb_notify_mode(pcb_hidlib_t *hidlib)
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;

	if (pcbhl_conf.temp.click_cmd_entry_active && (pcb_cli_mouse(hidlib, 1) == 0))
		return;

	pcb_grabbed.X = pcb_crosshair.X;
	pcb_grabbed.Y = pcb_crosshair.Y;

	if (conf_core.temp.rat_warn) {
		if (pcb_data_clear_flag(pcb->Data, PCB_FLAG_WARN, 1, 0) > 0)
			pcb_board_set_changed_flag(pcb_true);
	}
	pcb_tool_notify_mode(hidlib);
	pcb_draw();
}
