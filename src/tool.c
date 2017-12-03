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
 */

#include "config.h"

#include "tool.h"

#include "board.h"
#include "conf_core.h"
#include "error.h"
#include "event.h"

#warning tool TODO: remove this when pcb_crosshair_set_mode() got moved here
#include "crosshair.h"

static void default_tool_reg(void);
static void default_tool_unreg(void);

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
	if (pcb_tool_lookup(tool->name) != PCB_TOOLID_INVALID) /* don't register two tools with the same name */
		return -1;
	tool->cookie = cookie;
	vtp0_append(&pcb_tools, (void *)tool);
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

int pcb_tool_select_by_name(const char *name)
{
	pcb_toolid_t id = pcb_tool_lookup(name);
	if (id == PCB_TOOLID_INVALID)
		return -1;
	pcb_crosshair_set_mode(id);
	return 0;
}

int pcb_tool_select_by_id(pcb_toolid_t id)
{
	if ((id < 0) || (id > vtp0_len(&pcb_tools)))
		return -1;
	pcb_crosshair_set_mode(id);
	return 0;
}

int pcb_tool_select_highest(void)
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
	pcb_crosshair_set_mode(bestn);
	return 0;
}

/**** current tool function wrappers ****/
#define wrap(func, err_ret, prefix, args) \
	do { \
		const pcb_tool_t *tool; \
		if ((conf_core.editor.mode < 0) || (conf_core.editor.mode >= vtp0_len(&pcb_tools))) \
			{ err_ret; } \
		tool = (const pcb_tool_t *)pcb_tools.array[conf_core.editor.mode]; \
		if (tool->func == NULL) \
			{ err_ret; } \
		prefix tool->func args; \
	} while(0)

#define wrap_void(func, args)           wrap(func, return,  ;,      args)
#define wrap_retv(func, err_ret, args)  wrap(func, err_ret, return, args)

void pcb_tool_notify_mode(void)
{
	wrap_void(notify_mode, ());
}

void pcb_tool_release_mode(void)
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
	case PCB_TYPE_VIA:
		*x = ((pcb_pin_t *) ptr2)->X;
		*y = ((pcb_pin_t *) ptr2)->Y;
		break;
	case PCB_TYPE_LINE:
		*x = ((pcb_line_t *) ptr2)->Point1.X;
		*y = ((pcb_line_t *) ptr2)->Point1.Y;
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		*x = ((pcb_text_t *) ptr2)->X;
		*y = ((pcb_text_t *) ptr2)->Y;
		break;
	case PCB_TYPE_ELEMENT:
		*x = ((pcb_element_t *) ptr2)->MarkX;
		*y = ((pcb_element_t *) ptr2)->MarkY;
		break;
	case PCB_TYPE_POLY:
		*x = ((pcb_poly_t *) ptr2)->Points[0].X;
		*y = ((pcb_poly_t *) ptr2)->Points[0].Y;
		break;

	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLY_POINT:
		*x = ((pcb_point_t *) ptr3)->X;
		*y = ((pcb_point_t *) ptr3)->Y;
		break;
	case PCB_TYPE_ARC:
		pcb_arc_get_end((pcb_arc_t *) ptr2, 0, x, y);
		break;
	case PCB_TYPE_ARC_POINT:
		if (ptr3 != NULL) /* need to check because: if snap off, there's no known endpoint (leave x;y as is, then) */
			pcb_arc_get_end((pcb_arc_t *) ptr2, ((*(int **)ptr3) != pcb_arc_start_ptr), x, y);
		break;
	}
}

void pcb_tool_attach_for_copy(pcb_coord_t PlaceX, pcb_coord_t PlaceY, pcb_bool do_rubberband)
{
	pcb_box_t box;
	pcb_coord_t mx = 0, my = 0;

	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		get_grid_lock_coordinates(pcb_crosshair.AttachedObject.Type,
															pcb_crosshair.AttachedObject.Ptr1,
															pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = pcb_grid_fit(mx, PCB->Grid, PCB->GridOffsetX) - mx;
		my = pcb_grid_fit(my, PCB->Grid, PCB->GridOffsetY) - my;
	}
	pcb_crosshair.AttachedObject.X = PlaceX - mx;
	pcb_crosshair.AttachedObject.Y = PlaceY - my;
	if (!pcb_marked.status || conf_core.editor.local_ref)
		pcb_crosshair_set_local_ref(PlaceX - mx, PlaceY - my, pcb_true);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;

	/* get boundingbox of object and set cursor range */
	GetObjectBoundingBox(pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &box);
	pcb_crosshair_set_range(pcb_crosshair.AttachedObject.X - box.X1,
										pcb_crosshair.AttachedObject.Y - box.Y1,
										PCB->MaxWidth - (box.X2 - pcb_crosshair.AttachedObject.X),
										PCB->MaxHeight - (box.Y2 - pcb_crosshair.AttachedObject.Y));

	/* get all attached objects if necessary */
	if (do_rubberband && conf_core.editor.rubber_band_mode)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	if (do_rubberband &&
			(pcb_crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT ||
			 pcb_crosshair.AttachedObject.Type == PCB_TYPE_VIA ||
			 pcb_crosshair.AttachedObject.Type == PCB_TYPE_LINE || pcb_crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT))
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
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


#warning tool TODO: move this out to a tool plugin

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

