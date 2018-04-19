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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
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

#define PCB_OBJ_CLASS_LOCK (PCB_OBJ_PSTK | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_SUBC | PCB_OBJ_TEXT | PCB_OBJ_LOCKED)

void pcb_tool_lock_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	
	type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_CLASS_LOCK, &ptr1, &ptr2, &ptr3);

	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *subc = (pcb_subc_t *)ptr2;
		pcb_flag_change(PCB, PCB_CHGFLG_TOGGLE, PCB_FLAG_LOCK, PCB_OBJ_SUBC, ptr1, ptr2, ptr3);

		DrawSubc(subc);
		pcb_draw();
		pcb_hid_actionl("Report", "Subc", NULL);
	}
	else if (type != PCB_OBJ_VOID) {
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
	NULL,
	NULL,
	pcb_tool_lock_notify_mode,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	pcb_true
};
