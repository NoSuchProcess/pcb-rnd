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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"

#include "board.h"
#include "move.h"
#include "crosshair.h"
#include "search.h"
#include "tool_logic.h"
#include <librnd/hid/tool.h>

void pcb_tool_copy_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	pcb_crosshair_attached_clean(&PCB->hidlib);
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
	pcb_crosshair.extobj_edit = NULL;
}

void pcb_tool_copy_notify_mode(rnd_design_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	switch (pcb_crosshair.AttachedObject.State) {
		/* first notify, lookup object */
	case PCB_CH_STATE_FIRST:
		{
			int types = PCB_COPY_TYPES;

			pcb_crosshair_attached_clean(hl);
			pcb_crosshair.AttachedObject.Type =
				pcb_search_screen(hl->tool_x, hl->tool_y, types,
										 &pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);
			if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
				pcb_tool_attach_for_copy(hl, hl->tool_x, hl->tool_y, rnd_false);
			}
			break;
		}

		/* second notify, move or copy object */
	case PCB_CH_STATE_SECOND:

		if ((pcb->is_footprint) && (pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC)) {
			rnd_message(RND_MSG_WARNING, "Can not copy subcircuit in the footprint edit mode\n");
		}
		else {
			pcb_copy_obj(pcb_crosshair.AttachedObject.Type,
							 pcb_crosshair.AttachedObject.Ptr1,
							 pcb_crosshair.AttachedObject.Ptr2,
							 pcb_crosshair.AttachedObject.Ptr3, pcb_crosshair.AttachedObject.tx - pcb_crosshair.AttachedObject.X, pcb_crosshair.AttachedObject.ty - pcb_crosshair.AttachedObject.Y);
			pcb_subc_as_board_update(PCB);
			pcb_board_set_changed_flag(pcb, rnd_true);
		}

		/* reset identifiers */
		pcb_crosshair_attached_clean(hl);
		pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		pcb_crosshair.extobj_edit = NULL;
		break;
	}
}

void pcb_tool_copy_release_mode(rnd_design_t *hl)
{
	if (hl->tool_hit) {
		pcb_tool_copy_notify_mode(hl);
		hl->tool_hit = 0;
	}
}

void pcb_tool_copy_adjust_attached_objects(rnd_design_t *hl)
{
	pcb_crosshair.AttachedObject.tx = pcb_crosshair.X;
	pcb_crosshair.AttachedObject.ty = pcb_crosshair.Y;
}

void pcb_tool_copy_draw_attached(rnd_design_t *hl)
{
	pcb_xordraw_movecopy(rnd_gui->control_is_pressed(rnd_gui));
}

rnd_bool pcb_tool_copy_undo_act(rnd_design_t *hl)
{
	/* don't allow undo in the middle of an operation */
	if (pcb_crosshair.AttachedObject.State != PCB_CH_STATE_FIRST)
		return rnd_false;
	return rnd_true;
}

rnd_tool_t pcb_tool_copy = {
	"copy", NULL, NULL, 100, NULL, RND_TOOL_CURSOR_NAMED("crosshair"), 0,
	NULL,
	pcb_tool_copy_uninit,
	pcb_tool_copy_notify_mode,
	pcb_tool_copy_release_mode,
	pcb_tool_copy_adjust_attached_objects,
	pcb_tool_copy_draw_attached,
	pcb_tool_copy_undo_act,
	NULL,
	NULL, /* escape */
	
	0
};
