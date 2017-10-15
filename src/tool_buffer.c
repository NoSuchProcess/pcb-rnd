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
#include "compat_misc.h"
#include "data.h"
#include "hid_actions.h"
#include "rtree.h"
#include "search.h"
#include "tool.h"

#include "obj_elem_draw.h"


void pcb_tool_buffer_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_text_t estr[PCB_MAX_ELEMENTNAMES];
	pcb_element_t *e = 0;

	if (pcb_gui->shift_is_pressed()) {
		int type = pcb_search_screen(Note.X, Note.Y, PCB_TYPE_ELEMENT | PCB_TYPE_SUBC, &ptr1, &ptr2,
														&ptr3);
		if (type == PCB_TYPE_ELEMENT) {
			e = (pcb_element_t *) ptr1;
			if (e) {
				int i;

				memcpy(estr, e->Name, PCB_MAX_ELEMENTNAMES * sizeof(pcb_text_t));
				for (i = 0; i < PCB_MAX_ELEMENTNAMES; ++i)
					estr[i].TextString = estr[i].TextString ? pcb_strdup(estr[i].TextString) : NULL;
				pcb_element_remove(e);
			}
		}
	}
	if (pcb_buffer_copy_to_layout(PCB, Note.X, Note.Y))
		pcb_board_set_changed_flag(pcb_true);
	if (e) {
		int type = pcb_search_screen(Note.X, Note.Y, PCB_TYPE_ELEMENT | PCB_TYPE_SUBC, &ptr1, &ptr2,
														&ptr3);
		if (type == PCB_TYPE_ELEMENT && ptr1) {
			int i, save_n;
			e = (pcb_element_t *) ptr1;

			save_n = PCB_ELEMNAME_IDX_VISIBLE();

			for (i = 0; i < PCB_MAX_ELEMENTNAMES; i++) {
				if (i == save_n)
					pcb_elem_name_invalidate_erase(e);
				pcb_r_delete_entry(PCB->Data->name_tree[i], (pcb_box_t *) & (e->Name[i]));
				memcpy(&(e->Name[i]), &(estr[i]), sizeof(pcb_text_t));
				e->Name[i].Element = e;
				pcb_text_bbox(pcb_font(PCB, 0, 1), &(e->Name[i]));
				pcb_r_insert_entry(PCB->Data->name_tree[i], (pcb_box_t *) & (e->Name[i]), 0);
				if (i == save_n)
					pcb_elem_name_invalidate_draw(e);
			}
		}
	}
}
