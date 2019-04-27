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

#include "actions.h"
#include "board.h"
#include "rotate.h"
#include "tool.h"


void pcb_tool_rotate_notify_mode(void)
{
	pcb_screen_obj_rotate90(pcb_tool_note.X, pcb_tool_note.Y, pcb_gui->shift_is_pressed()? (conf_core.editor.show_solder_side ? 1 : 3)
									 : (conf_core.editor.show_solder_side ? 3 : 1));
	pcb_subc_as_board_update(PCB);
}

pcb_tool_t pcb_tool_rotate = {
	"rotate", NULL, 100, NULL,
	NULL,
	NULL,
	pcb_tool_rotate_notify_mode,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	pcb_true
};
