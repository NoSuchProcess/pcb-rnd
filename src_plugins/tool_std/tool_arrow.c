/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include "board.h"
#include "buffer.h"
#include "crosshair.h"
#include "data.h"
#include <librnd/core/actions.h>
#include "remove.h"
#include "move.h"
#include "search.h"
#include "select.h"
#include <librnd/core/tool.h>
#include "undo.h"
#include "extobj.h"
#include "tool_logic.h"


void pcb_tool_arrow_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

/* Called some time after the click if there was a release but no second click
	 a.k.a. finalize single click (some things are already done in pcb_press_mode
	 at the initial click event) */
static void click_timer_cb(rnd_hidval_t hv)
{
	rnd_hidlib_t *hl = hv.ptr;
	pcb_board_t *pcb = hv.ptr;

	if (hl->tool_click) {
		rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
		hl->tool_click = rnd_false;
		if (pcb_crosshair_note.Moving && !rnd_gui->shift_is_pressed(rnd_gui)) {
			hl->tool_grabbed.status = rnd_true;
			pcb_crosshair_note.Buffer = conf_core.editor.buffer_number;
			pcb_buffer_set_number(PCB_MAX_BUFFER - 1);
			pcb_buffer_clear(pcb, PCB_PASTEBUFFER);
			pcb_buffer_add_selected(pcb, PCB_PASTEBUFFER, hl->tool_x, hl->tool_y, rnd_true, rnd_true);
			pcb_undo_save_serial();
			pcb_remove_selected(rnd_false);
			rnd_tool_save(hl);
			rnd_tool_is_saved = rnd_true;
			rnd_tool_select_by_name(hl, "buffer");
		}
		else if (hl->tool_hit && !rnd_gui->shift_is_pressed(rnd_gui)) {
			rnd_rnd_box_t box;

			hl->tool_grabbed.status = rnd_true;
			rnd_tool_save(hl);
			rnd_tool_is_saved = rnd_true;
			rnd_tool_select_by_name(hl, rnd_gui->control_is_pressed(rnd_gui)? "copy" : "move");
			pcb_crosshair.AttachedObject.Ptr1 = pcb_crosshair_note.ptr1;
			pcb_crosshair.AttachedObject.Ptr2 = pcb_crosshair_note.ptr2;
			pcb_crosshair.AttachedObject.Ptr3 = pcb_crosshair_note.ptr3;
			pcb_crosshair.AttachedObject.Type = hl->tool_hit;

			if (pcb_crosshair.drags != NULL) {
				free(pcb_crosshair.drags);
				pcb_crosshair.drags = NULL;
			}
			pcb_crosshair.dragx = hl->tool_x;
			pcb_crosshair.dragy = hl->tool_y;
			box.X1 = hl->tool_x + PCB_SLOP * rnd_pixel_slop;
			box.X2 = hl->tool_x - PCB_SLOP * rnd_pixel_slop;
			box.Y1 = hl->tool_y + PCB_SLOP * rnd_pixel_slop;
			box.Y2 = hl->tool_y - PCB_SLOP * rnd_pixel_slop;
			pcb_crosshair.drags = pcb_list_block(pcb, &box, &pcb_crosshair.drags_len);
			pcb_crosshair.drags_current = 0;
			pcb_tool_attach_for_copy(hl, hl->tool_x, hl->tool_y, rnd_true);
		}
		else {
			rnd_rnd_box_t box;

			hl->tool_hit = 0;
			pcb_crosshair_note.Moving = rnd_false;
			pcb_undo_save_serial();
			box.X1 = -RND_MAX_COORD;
			box.Y1 = -RND_MAX_COORD;
			box.X2 = RND_MAX_COORD;
			box.Y2 = RND_MAX_COORD;
			/* unselect first if shift key not down */
			if (!rnd_gui->shift_is_pressed(rnd_gui) && pcb_select_block(pcb, &box, rnd_false, rnd_false, rnd_false))
				pcb_board_set_changed_flag(rnd_true);
			pcb_tool_notify_block();
			pcb_crosshair.AttachedBox.Point1.X = hl->tool_x;
			pcb_crosshair.AttachedBox.Point1.Y = hl->tool_y;
		}
		rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
	}

	if (pcb_crosshair.extobj_edit != NULL) {
		pcb_extobj_float_geo(pcb_crosshair.extobj_edit);
		rnd_gui->invalidate_all(rnd_gui);
	}
}

void pcb_tool_arrow_notify_mode(rnd_hidlib_t *hl)
{
	void *ptr1, *ptr2, *ptr3;
	int otype, type;
	int test;
	rnd_hidval_t hv;

	hl->tool_click = rnd_true;
	/* do something after click time */
	hv.ptr = hl;
	rnd_gui->add_timer(rnd_gui, click_timer_cb, conf_core.editor.click_time, hv);

	/* see if we clicked on something already selected
	 * (pcb_crosshair_note.Moving) or clicked on a MOVE_TYPE
	 * (hl->tool_hit)
	 */
	for (test = (PCB_SELECT_TYPES | PCB_MOVE_TYPES | PCB_OBJ_FLOATER | PCB_LOOSE_SUBC(PCB)) & ~PCB_OBJ_RAT; test; test &= ~otype) {
		/* grab object/point (e.g. line endpoint) for edit */
		otype = type = pcb_search_screen(hl->tool_x, hl->tool_y, test, &ptr1, &ptr2, &ptr3);
		if (otype == PCB_OBJ_ARC_POINT) { /* ignore arc endpoints if arc radius is 0 (so arc center is grabbed) */
			pcb_arc_t *arc = (pcb_arc_t *)ptr2;
			if ((arc->Width == 0) && (arc->Height == 0))
				continue;
		}
		if (!hl->tool_hit && (type & PCB_MOVE_TYPES) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2)) {
			hl->tool_hit = type;
			pcb_crosshair_note.ptr1 = ptr1;
			pcb_crosshair_note.ptr2 = ptr2;
			pcb_crosshair_note.ptr3 = ptr3;
			pcb_crosshair.AttachedObject.tx = hl->tool_x;
			pcb_crosshair.AttachedObject.ty = hl->tool_y;
		}
		if (!pcb_crosshair_note.Moving && (type & (PCB_SELECT_TYPES | PCB_LOOSE_SUBC(PCB))) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_any_obj_t *) ptr2)) {
			pcb_crosshair_note.Moving = rnd_true;
			/* remember where the user clicked to start this op */
			pcb_crosshair.AttachedObject.tx = pcb_crosshair.AttachedObject.X = hl->tool_x;
			pcb_crosshair.AttachedObject.ty = pcb_crosshair.AttachedObject.Y = hl->tool_y;
		}
		if ((hl->tool_hit && pcb_crosshair_note.Moving) || type == PCB_OBJ_VOID)
			return;
	}
}

void pcb_tool_arrow_release_mode(rnd_hidlib_t *hl)
{
	rnd_rnd_box_t box;
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (hl->tool_click) {
		rnd_rnd_box_t box;

		box.X1 = -RND_MAX_COORD;
		box.Y1 = -RND_MAX_COORD;
		box.X2 = RND_MAX_COORD;
		box.Y2 = RND_MAX_COORD;

		hl->tool_click = rnd_false;					/* inhibit timer action */
		pcb_undo_save_serial();
		/* unselect first if shift key not down */
		if (!rnd_gui->shift_is_pressed(rnd_gui)) {
			if (pcb_select_block(pcb, &box, rnd_false, rnd_false, rnd_false))
				pcb_board_set_changed_flag(rnd_true);
			if (pcb_crosshair_note.Moving) {
				pcb_crosshair_note.Moving = 0;
				hl->tool_hit = 0;
				return;
			}
		}
		/* Restore the SN so that if we select something the deselect/select combo
		   gets the same SN. */
		pcb_undo_restore_serial();
		if (pcb_select_object(PCB))
			pcb_board_set_changed_flag(rnd_true);
		else
			pcb_undo_inc_serial(); /* We didn't select anything new, so, the deselection should get its  own SN. */
		hl->tool_hit = 0;
		pcb_crosshair_note.Moving = 0;
	}
	else if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND) {
		box.X1 = pcb_crosshair.AttachedBox.Point1.X;
		box.Y1 = pcb_crosshair.AttachedBox.Point1.Y;
		box.X2 = pcb_crosshair.AttachedBox.Point2.X;
		box.Y2 = pcb_crosshair.AttachedBox.Point2.Y;

		pcb_undo_restore_serial();
		if (pcb_select_block(pcb, &box, rnd_true, rnd_true, rnd_false)) {
			pcb_board_set_changed_flag(rnd_true);
			pcb_undo_inc_serial();
		}
		else if (pcb_bumped)
			pcb_undo_inc_serial();
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	}
}

void pcb_tool_arrow_adjust_attached_objects(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedBox.State) {
		pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.X;
		pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.Y;
	}
}

/* XPM */
static const char *arrow_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c #000000",
". c #6EA5D7",
"o c None",
/* pixels */
"oo .. ooooooooooooooo",
"oo .... ooooooooooooo",
"ooo ...... oooooooooo",
"ooo ........ oooooooo",
"ooo ....... ooooooooo",
"oooo ..... oooooooooo",
"oooo ...... ooooooooo",
"ooooo .. ... oooooooo",
"ooooo . o ... ooooooo",
"oooooooooo ... oooooo",
"ooooooooooo .. oooooo",
"oooooooooooo  ooooooo",
"ooooooooooooooooooooo",
"ooo ooo    ooo    ooo",
"oo o oo ooo oo ooo oo",
"oo o oo ooo oo ooo oo",
"o ooo o    ooo    ooo",
"o ooo o ooo oo ooo oo",
"o     o ooo oo ooo oo",
"o ooo o ooo oo ooo oo",
"o ooo o ooo oo ooo oo"
};

pcb_tool_t pcb_tool_arrow = {
	"arrow", NULL, NULL, 10, arrow_icon, RND_TOOL_CURSOR_NAMED("left_ptr"), 0,
	NULL,
	pcb_tool_arrow_uninit,
	pcb_tool_arrow_notify_mode,
	pcb_tool_arrow_release_mode,
	pcb_tool_arrow_adjust_attached_objects,
	NULL,
	NULL,
	NULL,
	NULL, /* escape */
	
	PCB_TLF_RAT
};
