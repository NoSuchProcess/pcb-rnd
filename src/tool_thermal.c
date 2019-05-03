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
#include "change.h"
#include "data.h"
#include "actions.h"
#include "obj_pstk.h"
#include "search.h"
#include "thermal.h"
#include "tool.h"

void pcb_tool_thermal_on_pstk(pcb_pstk_t *ps, unsigned long lid)
{
	unsigned char *th, newth = 0;
	unsigned char cycle[] = {
		PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL, /* default start shape */
		PCB_THERMAL_ON | PCB_THERMAL_ROUND,
		PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL,
		PCB_THERMAL_ON | PCB_THERMAL_SHARP,
		PCB_THERMAL_ON | PCB_THERMAL_SOLID,
		PCB_THERMAL_ON | PCB_THERMAL_NOSHAPE
	};
	int cycles = sizeof(cycle) / sizeof(cycle[0]);

	th = pcb_pstk_get_thermal(ps, lid, 1);
	if (pcb_gui->shift_is_pressed()) {
		int n, curr = -1;
		/* cycle through the variants to find the current one */
		for(n = 0; n < cycles; n++) {
			if (*th == cycle[n]) {
				curr = n;
				break;
			}
		}
		
		/* bump current pattern to the next or set it up */
		if (curr >= 0) {
			curr++;
			if (curr >= cycles)
				curr = 0;
		}
		else
			curr = 0;

		newth = cycle[curr];
	}
	else {
		if ((th != NULL) && (*th != 0))
			newth = *th ^ PCB_THERMAL_ON; /* existing thermal, toggle */
		else
			newth = cycle[0]; /* new thermal, use default */
	}

	pcb_chg_obj_thermal(PCB_OBJ_PSTK, ps, ps, ps, newth, lid);
}


void pcb_tool_thermal_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	if (((type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_CLASS_PIN, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID)
			&& !PCB_FLAG_TEST(PCB_FLAG_HOLE, (pcb_any_obj_t *) ptr3)) {
		if (type == PCB_OBJ_PSTK)
			pcb_tool_thermal_on_pstk((pcb_pstk_t *)ptr2, INDEXOFCURRENT);
	}
}

/* XPM */
static const char *thrm_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 4 1",
"  c black",
". c #69E1B0",
"X c gray100",
"o c None",
/* pixels */
"ooooooooooooooooooooo",
"oooo ooooooooo oooooo",
"ooooo ooooooo ooooooo",
"oooooo o...o oooooooo",
"ooooooo ooo ooooooooo",
"oooooo.ooooo.oooooooo",
"oooooo.ooooo.oooooooo",
"oooooo.ooooo.oooooooo",
"ooooooo ooo ooooooooo",
"oooooo o...o oooooooo",
"ooooo ooooooo ooooooo",
"oooo ooooooooo oooooo",
"ooooooooooooooooooooo",
"     o oo o   oo ooo ",
"oo ooo oo o oo o  o  ",
"oo ooo oo o oo o o o ",
"oo ooo oo o   oo o o ",
"oo ooo    o o oo ooo ",
"oo ooo oo o oo o ooo ",
"oo ooo oo o oo o ooo ",
"oo ooo oo o oo o ooo "
};

pcb_tool_t pcb_tool_thermal = {
	"thermal", NULL, 100, thrm_icon, PCB_TOOL_CURSOR_NAMED("iron_cross"),
	NULL,
	NULL,
	pcb_tool_thermal_notify_mode,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	pcb_false
};
