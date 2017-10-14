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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "conf_core.h"

#include "action_helper.h"
#include "board.h"
#include "buffer.h"
#include "crosshair.h"
#include "data.h"
#include "hid_actions.h"
#include "remove.h"
#include "search.h"
#include "select.h"
#include "undo.h"


/* Called some time after the click if there was a release but no second click
   a.k.a. finalize single click (some things are already done in pcb_notify_mode
   at the initial click event) */
static void click_timer_cb(pcb_hidval_t hv)
{
	if (Note.Click) {
		pcb_notify_crosshair_change(pcb_false);
		Note.Click = pcb_false;
		if (Note.Moving && !pcb_gui->shift_is_pressed()) {
			Note.Buffer = conf_core.editor.buffer_number;
			pcb_buffer_set_number(PCB_MAX_BUFFER - 1);
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, Note.X, Note.Y, pcb_true);
			pcb_undo_save_serial();
			pcb_remove_selected();
			pcb_crosshair_save_mode();
			saved_mode = pcb_true;
			pcb_crosshair_set_mode(PCB_MODE_PASTE_BUFFER);
		}
		else if (Note.Hit && !pcb_gui->shift_is_pressed()) {
			pcb_box_t box;

			pcb_crosshair_save_mode();
			saved_mode = pcb_true;
			pcb_crosshair_set_mode(pcb_gui->control_is_pressed()? PCB_MODE_COPY : PCB_MODE_MOVE);
			pcb_crosshair.AttachedObject.Ptr1 = Note.ptr1;
			pcb_crosshair.AttachedObject.Ptr2 = Note.ptr2;
			pcb_crosshair.AttachedObject.Ptr3 = Note.ptr3;
			pcb_crosshair.AttachedObject.Type = Note.Hit;

			if (pcb_crosshair.drags != NULL) {
				free(pcb_crosshair.drags);
				pcb_crosshair.drags = NULL;
			}
			pcb_crosshair.dragx = Note.X;
			pcb_crosshair.dragy = Note.Y;
			box.X1 = Note.X + PCB_SLOP * pcb_pixel_slop;
			box.X2 = Note.X - PCB_SLOP * pcb_pixel_slop;
			box.Y1 = Note.Y + PCB_SLOP * pcb_pixel_slop;
			box.Y2 = Note.Y - PCB_SLOP * pcb_pixel_slop;
			pcb_crosshair.drags = pcb_list_block(PCB, &box, &pcb_crosshair.drags_len);
			pcb_crosshair.drags_current = 0;
			pcb_attach_for_copy(Note.X, Note.Y);
		}
		else {
			pcb_box_t box;

			Note.Hit = 0;
			Note.Moving = pcb_false;
			pcb_undo_save_serial();
			box.X1 = -PCB_MAX_COORD;
			box.Y1 = -PCB_MAX_COORD;
			box.X2 = PCB_MAX_COORD;
			box.Y2 = PCB_MAX_COORD;
			/* unselect first if shift key not down */
			if (!pcb_gui->shift_is_pressed() && pcb_select_block(PCB, &box, pcb_false))
				pcb_board_set_changed_flag(pcb_true);
			pcb_notify_block();
			pcb_crosshair.AttachedBox.Point1.X = Note.X;
			pcb_crosshair.AttachedBox.Point1.Y = Note.Y;
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

  Note.Click = pcb_true;
  /* do something after click time */
  pcb_gui->add_timer(click_timer_cb, conf_core.editor.click_time, hv);

  /* see if we clicked on something already selected
   * (Note.Moving) or clicked on a MOVE_TYPE
   * (Note.Hit)
   */
  for (test = (PCB_SELECT_TYPES | PCB_MOVE_TYPES) & ~PCB_TYPE_RATLINE; test; test &= ~type) {
    type = pcb_search_screen(Note.X, Note.Y, test, &ptr1, &ptr2, &ptr3);
    if (!Note.Hit && (type & PCB_MOVE_TYPES) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2)) {
      Note.Hit = type;
      Note.ptr1 = ptr1;
      Note.ptr2 = ptr2;
      Note.ptr3 = ptr3;
    }
    if (!Note.Moving && (type & PCB_SELECT_TYPES) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_pin_t *) ptr2))
      Note.Moving = pcb_true;
    if ((Note.Hit && Note.Moving) || type == PCB_TYPE_NONE)
      return;
  }
}
