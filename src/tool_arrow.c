/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
#include "actions.h"
#include "remove.h"
#include "move.h"
#include "search.h"
#include "select.h"
#include "tool.h"
#include "undo.h"


void pcb_tool_arrow_uninit(void)
{
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	pcb_notify_crosshair_change(pcb_true);
}

/* Called some time after the click if there was a release but no second click
	 a.k.a. finalize single click (some things are already done in pcb_notify_mode
	 at the initial click event) */
static void click_timer_cb(pcb_hidval_t hv)
{
	if (pcb_tool_note.Click) {
		pcb_notify_crosshair_change(pcb_false);
		pcb_tool_note.Click = pcb_false;
		if (pcb_tool_note.Moving && !pcb_gui->shift_is_pressed()) {
			pcb_tool_note.Buffer = conf_core.editor.buffer_number;
			pcb_buffer_set_number(PCB_MAX_BUFFER - 1);
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, pcb_tool_note.X, pcb_tool_note.Y, pcb_true);
			pcb_undo_save_serial();
			pcb_remove_selected(pcb_false);
			pcb_tool_save(&PCB->hidlib);
			pcb_tool_is_saved = pcb_true;
			pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
		}
		else if (pcb_tool_note.Hit && !pcb_gui->shift_is_pressed()) {
			pcb_box_t box;

			pcb_tool_save(&PCB->hidlib);
			pcb_tool_is_saved = pcb_true;
			pcb_tool_select_by_id(&PCB->hidlib, pcb_gui->control_is_pressed()? PCB_MODE_COPY : PCB_MODE_MOVE);
			pcb_crosshair.AttachedObject.Ptr1 = pcb_tool_note.ptr1;
			pcb_crosshair.AttachedObject.Ptr2 = pcb_tool_note.ptr2;
			pcb_crosshair.AttachedObject.Ptr3 = pcb_tool_note.ptr3;
			pcb_crosshair.AttachedObject.Type = pcb_tool_note.Hit;

			if (pcb_crosshair.drags != NULL) {
				free(pcb_crosshair.drags);
				pcb_crosshair.drags = NULL;
			}
			pcb_crosshair.dragx = pcb_tool_note.X;
			pcb_crosshair.dragy = pcb_tool_note.Y;
			box.X1 = pcb_tool_note.X + PCB_SLOP * pcb_pixel_slop;
			box.X2 = pcb_tool_note.X - PCB_SLOP * pcb_pixel_slop;
			box.Y1 = pcb_tool_note.Y + PCB_SLOP * pcb_pixel_slop;
			box.Y2 = pcb_tool_note.Y - PCB_SLOP * pcb_pixel_slop;
			pcb_crosshair.drags = pcb_list_block(PCB, &box, &pcb_crosshair.drags_len);
			pcb_crosshair.drags_current = 0;
			pcb_tool_attach_for_copy(pcb_tool_note.X, pcb_tool_note.Y, pcb_true);
		}
		else {
			pcb_box_t box;

			pcb_tool_note.Hit = 0;
			pcb_tool_note.Moving = pcb_false;
			pcb_undo_save_serial();
			box.X1 = -PCB_MAX_COORD;
			box.Y1 = -PCB_MAX_COORD;
			box.X2 = PCB_MAX_COORD;
			box.Y2 = PCB_MAX_COORD;
			/* unselect first if shift key not down */
			if (!pcb_gui->shift_is_pressed() && pcb_select_block(PCB, &box, pcb_false, pcb_false, pcb_false))
				pcb_board_set_changed_flag(pcb_true);
			pcb_tool_notify_block();
			pcb_crosshair.AttachedBox.Point1.X = pcb_tool_note.X;
			pcb_crosshair.AttachedBox.Point1.Y = pcb_tool_note.Y;
		}
		pcb_notify_crosshair_change(pcb_true);
	}
}

void pcb_tool_arrow_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	int test;
	pcb_hidval_t hv;

	pcb_tool_note.Click = pcb_true;
	/* do something after click time */
	pcb_gui->add_timer(click_timer_cb, conf_core.editor.click_time, hv);

	/* see if we clicked on something already selected
	 * (pcb_tool_note.Moving) or clicked on a MOVE_TYPE
	 * (pcb_tool_note.Hit)
	 */
	for (test = (PCB_SELECT_TYPES | PCB_MOVE_TYPES | PCB_OBJ_FLOATER | PCB_LOOSE_SUBC) & ~PCB_OBJ_RAT; test; test &= ~type) {
		type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, test, &ptr1, &ptr2, &ptr3);
		if (!pcb_tool_note.Hit && (type & PCB_MOVE_TYPES) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2)) {
			pcb_tool_note.Hit = type;
			pcb_tool_note.ptr1 = ptr1;
			pcb_tool_note.ptr2 = ptr2;
			pcb_tool_note.ptr3 = ptr3;
			pcb_crosshair.AttachedObject.tx = pcb_tool_note.X;
			pcb_crosshair.AttachedObject.ty = pcb_tool_note.Y;
		}
		if (!pcb_tool_note.Moving && (type & (PCB_SELECT_TYPES | PCB_LOOSE_SUBC)) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_any_obj_t *) ptr2)) {
			pcb_tool_note.Moving = pcb_true;
			/* remember where the user clicked to start this op */
			pcb_crosshair.AttachedObject.tx = pcb_crosshair.AttachedObject.X = pcb_tool_note.X;
			pcb_crosshair.AttachedObject.ty = pcb_crosshair.AttachedObject.Y = pcb_tool_note.Y;
		}
		if ((pcb_tool_note.Hit && pcb_tool_note.Moving) || type == PCB_OBJ_VOID)
			return;
	}
}

void pcb_tool_arrow_release_mode(void)
{
	pcb_box_t box;

	if (pcb_tool_note.Click) {
		pcb_box_t box;

		box.X1 = -PCB_MAX_COORD;
		box.Y1 = -PCB_MAX_COORD;
		box.X2 = PCB_MAX_COORD;
		box.Y2 = PCB_MAX_COORD;

		pcb_tool_note.Click = pcb_false;					/* inhibit timer action */
		pcb_undo_save_serial();
		/* unselect first if shift key not down */
		if (!pcb_gui->shift_is_pressed()) {
			if (pcb_select_block(PCB, &box, pcb_false, pcb_false, pcb_false))
				pcb_board_set_changed_flag(pcb_true);
			if (pcb_tool_note.Moving) {
				pcb_tool_note.Moving = 0;
				pcb_tool_note.Hit = 0;
				return;
			}
		}
		/* Restore the SN so that if we select something the deselect/select combo
		   gets the same SN. */
		pcb_undo_restore_serial();
		if (pcb_select_object(PCB))
			pcb_board_set_changed_flag(pcb_true);
		else
			pcb_undo_inc_serial(); /* We didn't select anything new, so, the deselection should get its  own SN. */
		pcb_tool_note.Hit = 0;
		pcb_tool_note.Moving = 0;
	}
	else if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND) {
		box.X1 = pcb_crosshair.AttachedBox.Point1.X;
		box.Y1 = pcb_crosshair.AttachedBox.Point1.Y;
		box.X2 = pcb_crosshair.AttachedBox.Point2.X;
		box.Y2 = pcb_crosshair.AttachedBox.Point2.Y;

		pcb_undo_restore_serial();
		if (pcb_select_block(PCB, &box, pcb_true, pcb_true, pcb_false))
			pcb_board_set_changed_flag(pcb_true);
		else if (pcb_bumped)
			pcb_undo_inc_serial();
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	}
}

void pcb_tool_arrow_adjust_attached_objects(void)
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
	"arrow", NULL, 10, arrow_icon, PCB_TOOL_CURSOR_NAMED("left_ptr"), 0,
	NULL,
	pcb_tool_arrow_uninit,
	pcb_tool_arrow_notify_mode,
	pcb_tool_arrow_release_mode,
	pcb_tool_arrow_adjust_attached_objects,
	NULL,
	NULL,
	NULL,
	
	pcb_true
};
