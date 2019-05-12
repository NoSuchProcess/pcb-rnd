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

#include "board.h"
#include "data.h"
#include "draw.h"
#include "actions.h"
#include "tool.h"
#include "undo.h"

#include "obj_text_draw.h"


void pcb_tool_text_notify_mode(void)
{
	char *string;

	if ((string = pcb_hid_prompt_for("Enter text:", "", "text")) != NULL) {
		if (strlen(string) > 0) {
			pcb_text_t *text;
			int flag = PCB_FLAG_CLEARLINE;

			if (pcb_layer_flags(PCB, INDEXOFCURRENT) & PCB_LYT_BOTTOM)
				flag |= PCB_FLAG_ONSOLDER;
			if ((text = pcb_text_new(pcb_loose_subc_layer(PCB, CURRENT, pcb_true), pcb_font(PCB, conf_core.design.text_font_id, 1), pcb_tool_note.X,
																pcb_tool_note.Y, 0, conf_core.design.text_scale, conf_core.design.text_thickness, string, pcb_flag_make(flag))) != NULL) {
				pcb_undo_add_obj_to_create(PCB_OBJ_TEXT, CURRENT, text, text);
				pcb_undo_inc_serial();
				pcb_text_invalidate_draw(CURRENT, text);
				pcb_subc_as_board_update(PCB);
				pcb_draw();
			}
		}
		free(string);
	}
}

void pcb_tool_text_draw_attached(void)
{
	pcb_text_t text;
	int flag = PCB_FLAG_CLEARLINE;

	if (pcb_layer_flags(PCB, INDEXOFCURRENT) & PCB_LYT_BOTTOM)
		flag |= PCB_FLAG_ONSOLDER;

	text.X = pcb_crosshair.X;
	text.Y = pcb_crosshair.Y;
	text.rot = 0;
	text.Flags = pcb_flag_make(flag);
	text.Scale = conf_core.design.text_scale;
	text.thickness = conf_core.design.text_thickness;
	text.TextString = "A";
	text.fid = conf_core.design.text_font_id;
	text.ID = 0;
	text.BoundingBox.X1 = text.BoundingBox.Y1 = text.BoundingBox.X2 = text.BoundingBox.Y2 = 0;
	pcb_text_draw_xor(&text,0,0);

}

/* XPM */
static const char *text_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 4 1",
"  c black",
". c #4E85b7",
"X c gray100",
"o c None",
/* pixels */
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"oo.ooo.ooo.ooo.ooo.oo",
"o.o.o.o.o.o.o.o.o.o.o",
"oo.ooo.ooo.ooo.ooo.oo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"     o   o ooo o     ",
"oo ooo ooo ooo ooo oo",
"oo ooo oooo o oooo oo",
"oo ooo ooooo ooooo oo",
"oo ooo  ooo o oooo oo",
"oo ooo ooo ooo ooo oo",
"oo ooo ooo ooo ooo oo",
"oo ooo   o ooo ooo oo",
"ooooooooooooooooooooo"
};

pcb_tool_t pcb_tool_text = {
	"text", NULL, 100, text_icon, PCB_TOOL_CURSOR_NAMED("xterm"), 0,
	NULL,
	NULL,
	pcb_tool_text_notify_mode,
	NULL,
	NULL,
	pcb_tool_text_draw_attached,
	NULL,
	NULL,
	
	pcb_false
};

