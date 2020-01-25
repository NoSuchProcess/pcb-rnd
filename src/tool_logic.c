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

#include <librnd/core/hidlib_conf.h>
#include <librnd/core/event.h>
#include <librnd/core/grid.h>
#include <librnd/core/actions.h>
#include <librnd/core/tool.h>

#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"

#include "tool_logic.h"

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
	pcb_hid_notify_crosshair_change(&PCB->hidlib, pcb_false);
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
	pcb_hid_notify_crosshair_change(&PCB->hidlib, pcb_true);
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
