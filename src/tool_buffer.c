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
#include "conf_core.h"

#include "board.h"
#include "buffer.h"
#include "compat_misc.h"
#include "data.h"
#include "actions.h"
#include "rtree.h"
#include "search.h"
#include "tool.h"
#include "undo.h"

void pcb_tool_buffer_init(void)
{
	pcb_crosshair_range_to_buffer();
}

void pcb_tool_buffer_uninit(void)
{
	pcb_crosshair_set_range(0, 0, PCB->MaxWidth, PCB->MaxHeight);
}

void pcb_tool_buffer_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_subc_t *orig_subc = NULL;
	int need_redraw = 0;

	if (pcb_gui->shift_is_pressed()) {
		int type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);
		if (type == PCB_OBJ_SUBC) {
			orig_subc = (pcb_subc_t *) ptr1;
			pcb_subc_remove(orig_subc);
			need_redraw = 1;
		}
	}
	if (pcb_buffer_copy_to_layout(PCB, pcb_crosshair.AttachedObject.tx, pcb_crosshair.AttachedObject.ty)) {
		pcb_board_set_changed_flag(pcb_true);
		need_redraw = 1;
	}

	if (orig_subc != NULL) {
		int type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);
		if (type == PCB_OBJ_SUBC && (ptr1 != NULL)) {
			int n;
			pcb_attribute_list_t *dst = &(((pcb_subc_t *)ptr1)->Attributes), *src = &orig_subc->Attributes;
			for (n = 0; n < src->Number; n++)
				if (strcmp(src->List[n].name, "footprint") != 0)
					pcb_attribute_put(dst, src->List[n].name, src->List[n].value);
		}
	}

	if (need_redraw)
		pcb_gui->invalidate_all();
}

void pcb_tool_buffer_release_mode(void)
{
	if (pcb_tool_note.Moving) {
		pcb_undo_restore_serial();
		pcb_tool_buffer_notify_mode();
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		pcb_buffer_set_number(pcb_tool_note.Buffer);
		pcb_tool_note.Moving = pcb_false;
		pcb_tool_note.Hit = 0;
	}
}

void pcb_tool_buffer_adjust_attached_objects(void)
{
	pcb_crosshair.AttachedObject.tx = pcb_crosshair.X;
	pcb_crosshair.AttachedObject.ty = pcb_crosshair.Y;
}

void pcb_tool_buffer_draw_attached(void)
{
	pcb_xordraw_buffer(PCB_PASTEBUFFER);
}

pcb_tool_t pcb_tool_buffer = {
	"buffer", NULL, 100,
	pcb_tool_buffer_init,
	pcb_tool_buffer_uninit,
	pcb_tool_buffer_notify_mode,
	pcb_tool_buffer_release_mode,
	pcb_tool_buffer_adjust_attached_objects,
	pcb_tool_buffer_draw_attached,
	NULL,
	NULL,
	
	pcb_true
};
