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

#include "board.h"
#include "event.h"
#include "actions.h"
#include "undo.h"
#include "remove.h"
#include "search.h"
#include "obj_rat.h"
#include "tool.h"


void pcb_tool_remove_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *obj;
	int type;
	
	if ((type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_REMOVE_TYPES | PCB_LOOSE_SUBC | PCB_OBJ_FLOATER, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
		obj = (pcb_any_obj_t *)ptr2;
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj)) {
			pcb_message(PCB_MSG_WARNING, "Sorry, %s object is locked\n", pcb_obj_type_name(obj->type));
			return;
		}

		if (type == PCB_OBJ_SUBC) {
			if(PCB->is_footprint) {
				pcb_message(PCB_MSG_WARNING, "Can not remove the subcircuit being edited in the footprint edit mode\n");
				return;
			}
		}

		obj = ptr2;
		pcb_rat_update_obj_removed(PCB, obj);

		/* preserve original parent over the board layer pcb_search_screen operated on -
		   this is essential for undo: it needs to put back the object to the original
		   layer (e.g. inside a subc) instead of on the board layer */
		if (obj->parent_type == PCB_PARENT_LAYER)
			ptr1 = obj->parent.layer;
		else if (obj->parent_type == PCB_PARENT_DATA)
			ptr1 = obj->parent.data;

		pcb_remove_object(type, ptr1, ptr2, ptr3);
		pcb_undo_inc_serial();
		pcb_subc_as_board_update(PCB);
		pcb_board_set_changed_flag(pcb_true);
	}
}

/* XPM */
static const char *del_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c black",
". c gray100",
"X c None",
/* pixels */
"XXX XXXXXXXXXXXX XXXX",
"XXXX XXX    XXX XXXXX",
"XXXXX X      X XXXXXX",
"XXXXXX XX  XX XXXXXXX",
"XXXXX  XX  XX  XXXXXX",
"XXXXX          XXXXXX",
"XXXXXX        XXXXXXX",
"XXXXXXX  XX  XXXXXXXX",
"XXXXXXX  XX XXXXXXXXX",
"XXXXXX X XX  XXXXXXXX",
"XXXX  XX    X  XXXXXX",
"XX  XXXXX  XXXX  XXXX",
"XXXXXXXXXXXXXXXXXXXXX",
"XX     XX     X XXXXX",
"XXX XXX X XXXXX XXXXX",
"XXX XXX X XXXXX XXXXX",
"XXX XXX X XXXXX XXXXX",
"XXX XXX X    XX XXXXX",
"XXX XXX X XXXXX XXXXX",
"XXX XXX X XXXXX XXXXX",
"XX     XX     X     X"
};

pcb_tool_t pcb_tool_remove = {
	"remove", NULL, 100, del_icon,
	NULL,
	NULL,
	pcb_tool_remove_notify_mode,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	pcb_true
};
