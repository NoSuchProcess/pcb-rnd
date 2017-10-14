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
#include "compat_nls.h"
#include "event.h"
#include "hid_actions.h"
#include "undo.h"
#include "remove.h"
#include "search.h"


void pcb_tool_remove_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	
	if ((type = pcb_search_screen(Note.X, Note.Y, PCB_REMOVE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_line_t *) ptr2)) {
			pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
			return;
		}
		if (type == PCB_TYPE_ELEMENT)
			pcb_event(PCB_EVENT_RUBBER_REMOVE_ELEMENT, "ppp", ptr1, ptr2, ptr3);
		pcb_remove_object(type, ptr1, ptr2, ptr3);
		pcb_undo_inc_serial();
		pcb_board_set_changed_flag(pcb_true);
	}
}
