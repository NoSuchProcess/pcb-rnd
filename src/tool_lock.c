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

#include "action_helper.h"
#include "board.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "hid_actions.h"
#include "search.h"
#include "tool.h"
#include "tool_lock.h"

#include "obj_elem_draw.h"


void pcb_tool_lock_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	
	type = pcb_search_screen(Note.X, Note.Y, PCB_TYPEMASK_LOCK, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_ELEMENT) {
		pcb_element_t *element = (pcb_element_t *) ptr2;

		PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, element);
		PCB_PIN_LOOP(element);
		{
			PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, pin);
			PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pin);
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, pad);
			PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pad);
		}
		PCB_END_LOOP;
		PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, element);
		/* always re-draw it since I'm too lazy
		 * to tell if a selected flag changed
		 */
		pcb_elem_invalidate_draw(element);
		pcb_draw();
		pcb_hid_actionl("Report", "Object", NULL);
	}
	else if (type == PCB_TYPE_SUBC) {
		pcb_subc_t *subc = (pcb_subc_t *)ptr2;
		pcb_flag_change(PCB, PCB_CHGFLG_TOGGLE, PCB_FLAG_LOCK, PCB_TYPE_SUBC, ptr1, ptr2, ptr3);

		DrawSubc(subc);
		pcb_draw();
		pcb_hid_actionl("Report", "Subc", NULL);
	}
	else if (type != PCB_TYPE_NONE) {
		pcb_text_t *thing = (pcb_text_t *) ptr3;
		PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, thing);
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, thing)
				&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, thing)) {
			/* this is not un-doable since LOCK isn't */
			PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, thing);
			pcb_draw_obj((pcb_any_obj_t *)ptr2);
			pcb_draw();
		}
		pcb_hid_actionl("Report", "Object", NULL);
	}
}

pcb_tool_t pcb_tool_lock = {
	"lock", NULL, 100,
	pcb_tool_lock_notify_mode,
	NULL
};
