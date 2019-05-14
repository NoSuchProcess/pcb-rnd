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

/* XPM */
static const char *rot_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 4 1",
"  c black",
". c #6EA5D7",
"X c gray100",
"o c None",
/* pixels */
"ooooooooooo.ooooooooo",
"oooooooooo..ooooooooo",
"ooooooooo....oooooooo",
"oooooooooo..o.ooooooo",
"ooooooooooo.oo.oooooo",
"oooooooooooooo.oooooo",
"oooooooooooooo.oooooo",
"oooooooooooooo.oooooo",
"oooooooooooooo.oooooo",
"ooooooooooooo.ooooooo",
"oooooooooooo.oooooooo",
"oooooooooo..ooooooooo",
"ooooooooooooooooooooo",
"ooo    ooo   oo     o",
"ooo ooo o ooo ooo ooo",
"ooo ooo o ooo ooo ooo",
"ooo    oo ooo ooo ooo",
"ooo   ooo ooo ooo ooo",
"ooo o  oo ooo ooo ooo",
"ooo oo  o ooo ooo ooo",
"ooo ooo oo   oooo ooo"
};

#define rotateIcon_width 16
#define rotateIcon_height 16
static unsigned char rotateIcon_bits[] = {
   0xf0, 0x03, 0xf8, 0x87, 0x0c, 0xcc, 0x06, 0xf8, 0x03, 0xb0, 0x01, 0x98,
   0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x19, 0x80, 0x0d, 0xc0,
   0x1f, 0x60, 0x3b, 0x30, 0xe1, 0x3f, 0xc0, 0x0f};

#define rotateMask_width 16
#define rotateMask_height 16
static unsigned char rotateMask_bits[] = {
   0xf0, 0x03, 0xf8, 0x87, 0x0c, 0xcc, 0x06, 0xf8, 0x03, 0xf0, 0x01, 0xf8,
   0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x1f, 0x80, 0x0f, 0xc0,
   0x1f, 0x60, 0x3b, 0x30, 0xe1, 0x3f, 0xc0, 0x0f};

pcb_tool_t pcb_tool_rotate = {
	"rotate", NULL, 100, rot_icon, PCB_TOOL_CURSOR_XBM(rotateIcon_bits, rotateMask_bits), 0,
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
