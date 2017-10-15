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
#include "hid_actions.h"
#include "search.h"
#include "tool.h"


void pcb_tool_thermal_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	if (((type = pcb_search_screen(Note.X, Note.Y, PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
			&& !PCB_FLAG_TEST(PCB_FLAG_HOLE, (pcb_pin_t *) ptr3)) {
		if (pcb_gui->shift_is_pressed()) {
			int tstyle = PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3);
			tstyle++;
			if (tstyle > 5)
				tstyle = 1;
			pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, tstyle);
		}
		else if (PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3))
			pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, 0);
		else
			pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, PCB->ThermStyle);
	}
}
