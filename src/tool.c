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

#include "config.h"

#include "tool.h"

#include "board.h"
#include "hidlib_conf.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "event.h"
#include "find.h"
#include "grid.h"
#include "undo.h"
#include "actions.h"


pcb_toolid_t pcb_tool_prev_id;
pcb_toolid_t pcb_tool_next_id;
static int save_position = 0;
static int save_stack[PCB_MAX_MODESTACK_DEPTH];

static void default_tool_reg(void);
static void default_tool_unreg(void);
static void init_current_tool(void);
static void uninit_current_tool(void);

void pcb_tool_init(void)
{
	vtp0_init(&pcb_tools);
	default_tool_reg(); /* temporary */
}

void pcb_tool_uninit(void)
{
	default_tool_unreg(); /* temporary */
	while(vtp0_len(&pcb_tools) != 0) {
		const pcb_tool_t *tool = pcb_tool_get(0);
		pcb_message(PCB_MSG_WARNING, "Unregistered tool: %s of %s; check your plugins, fix them to unregister their tools!\n", tool->name, tool->cookie);
		pcb_tool_unreg_by_cookie(tool->cookie);
	}
	vtp0_uninit(&pcb_tools);
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
		pcb_gui->reg_mouse_cursor(NULL, id, tool->cursor.name, tool->cursor.pixel, tool->cursor.mask);
	pcb_event(&PCB->hidlib, PCB_EVENT_TOOL_REG, "p", tool);
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
	
	if (PCB->RatDraw && !pcb_tool_get(id)->allow_when_drawing_ratlines) {
		pcb_message(PCB_MSG_WARNING, "That mode is NOT allowed when drawing ratlines!\n");
		id = PCB_MODE_ARROW;
	}
	
	pcb_tool_prev_id = pcbhl_conf.editor.mode;
	pcb_tool_next_id = id;
	uninit_current_tool();
	sprintf(id_s, "%d", id);
	conf_set(CFR_DESIGN, "editor/mode", -1, id_s, POL_OVERWRITE);
	init_current_tool();

	recursing = pcb_false;

	/* force a crosshair grid update because the valid range
	 * may have changed
	 */
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair_move_relative(0, 0);
	pcb_notify_crosshair_change(pcb_true);
	if (pcb_gui != NULL)
		pcb_gui->set_mouse_cursor(hidlib, id);
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
			pcb_gui->reg_mouse_cursor(NULL, n, (*tool)->cursor.name, (*tool)->cursor.pixel, (*tool)->cursor.mask);
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

void pcb_tool_notify_mode(void)
{
	wrap_void(notify_mode, ());
}

void pcb_tool_release_mode(pcb_hidlib_t *hidlib)
{
	wrap_void(release_mode, ());
}

void pcb_tool_adjust_attached_objects(void)
{
	wrap_void(adjust_attached_objects, ());
}

void pcb_tool_draw_attached(void)
{
	wrap_void(draw_attached, ());
}

pcb_bool pcb_tool_undo_act(void)
{
	wrap_retv(undo_act, return pcb_true, ());
}

pcb_bool pcb_tool_redo_act(void)
{
	wrap_retv(redo_act, return pcb_true, ());
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

void pcb_tool_attach_for_copy(pcb_coord_t PlaceX, pcb_coord_t PlaceY, pcb_bool do_rubberband)
{
	pcb_box_t box;
	pcb_coord_t mx = 0, my = 0;

	pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_RESET, NULL);
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		get_grid_lock_coordinates(pcb_crosshair.AttachedObject.Type,
															pcb_crosshair.AttachedObject.Ptr1,
															pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = pcb_grid_fit(mx, PCB->hidlib.grid, PCB->hidlib.grid_ox) - mx;
		my = pcb_grid_fit(my, PCB->hidlib.grid, PCB->hidlib.grid_oy) - my;
	}
	pcb_crosshair.AttachedObject.X = PlaceX - mx;
	pcb_crosshair.AttachedObject.Y = PlaceY - my;
	if (!pcb_marked.status || conf_core.editor.local_ref)
		pcb_crosshair_set_local_ref(PlaceX - mx, PlaceY - my, pcb_true);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;

	/* get boundingbox of object and set cursor range */
	pcb_obj_get_bbox_naked(pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &box);
	pcb_crosshair_set_range(pcb_crosshair.AttachedObject.X - box.X1,
										pcb_crosshair.AttachedObject.Y - box.Y1,
										PCB->hidlib.size_x - (box.X2 - pcb_crosshair.AttachedObject.X),
										PCB->hidlib.size_y - (box.Y2 - pcb_crosshair.AttachedObject.Y));

	/* get all attached objects if necessary */
	if (do_rubberband && conf_core.editor.rubber_band_mode)
		pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	if (do_rubberband &&
			(pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_PSTK ||
			 pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE || pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT))
		pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
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

pcb_bool pcb_tool_should_snap_offgrid_line(pcb_layer_t *layer, pcb_line_t *line)
{
	/* Allow snapping to off-grid lines when drawing new lines (on
	 * the same layer), and when moving a line end-point
	 * (but don't snap to the same line)
	 */
	if ((pcbhl_conf.editor.mode == PCB_MODE_LINE && CURRENT == layer) ||
			(pcbhl_conf.editor.mode == PCB_MODE_MOVE
			 && pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT
			 && pcb_crosshair.AttachedObject.Ptr1 == layer
			 && pcb_crosshair.AttachedObject.Ptr2 != line))
		return pcb_true;
	else
		return pcb_false;
}

TODO("tool: move this out to a tool plugin")

#include "tool_arc.h"
#include "tool_arrow.h"
#include "tool_buffer.h"
#include "tool_copy.h"
#include "tool_insert.h"
#include "tool_line.h"
#include "tool_lock.h"
#include "tool_move.h"
#include "tool_poly.h"
#include "tool_polyhole.h"
#include "tool_rectangle.h"
#include "tool_remove.h"
#include "tool_rotate.h"
#include "tool_text.h"
#include "tool_thermal.h"
#include "tool_via.h"

static const char *pcb_tool_cookie = "default tools";

static void default_tool_reg(void)
{
	pcb_tool_reg(&pcb_tool_arc, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_arrow, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_buffer, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_copy, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_insert, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_line, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_lock, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_move, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_poly, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_polyhole, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_rectangle, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_remove, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_rotate, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_text, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_thermal, pcb_tool_cookie);
	pcb_tool_reg(&pcb_tool_via, pcb_tool_cookie);
}

static void default_tool_unreg(void)
{
	pcb_tool_unreg_by_cookie(pcb_tool_cookie);
}


/*** old helpers ***/
void pcb_release_mode(pcb_hidlib_t *hidlib)
{
	if (pcbhl_conf.temp.click_cmd_entry_active && (pcb_cli_mouse(0) == 0))
		return;

	pcb_tool_release_mode(hidlib);

	if (pcb_tool_is_saved)
		pcb_tool_restore(hidlib);
	pcb_tool_is_saved = pcb_false;
	pcb_draw();
}

void pcb_notify_mode(pcb_hidlib_t *hidlib)
{
	if (pcbhl_conf.temp.click_cmd_entry_active && (pcb_cli_mouse(1) == 0))
		return;

	if (conf_core.temp.rat_warn)
		pcb_data_clear_flag(PCB->Data, PCB_FLAG_WARN, 1, 0);
	pcb_tool_notify_mode();
	pcb_draw();
}
