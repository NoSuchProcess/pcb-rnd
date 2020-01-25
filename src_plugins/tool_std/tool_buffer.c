/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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
#include "buffer.h"
#include <librnd/core/compat_misc.h>
#include "data.h"
#include <librnd/core/actions.h>
#include <librnd/poly/rtree.h>
#include "search.h"
#include <librnd/core/tool.h>
#include "undo.h"

void pcb_tool_buffer_init(void)
{
}

void pcb_tool_buffer_uninit(void)
{
}

static void pcb_tool_buffer_notify_mode_(pcb_hidlib_t *hl, pcb_bool keep_ids)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (pcb_gui->shift_is_pressed(pcb_gui)) {
		pcb_actionva(hl, "ReplaceFootprint", "object", "@buffer", "dumb", NULL);
		return;
	}

	if (pcb_buffer_copy_to_layout(pcb, pcb_crosshair.AttachedObject.tx, pcb_crosshair.AttachedObject.ty, keep_ids)) {
		pcb_board_set_changed_flag(pcb_true);
		pcb_gui->invalidate_all(pcb_gui);
	}
}

void pcb_tool_buffer_notify_mode(pcb_hidlib_t *hl)
{
	pcb_tool_buffer_notify_mode_(hl, pcb_false);
}

void pcb_tool_buffer_release_mode(pcb_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (pcb_crosshair_note.Moving) {
		pcb_undo_restore_serial();
		pcb_tool_buffer_notify_mode_(hl, pcb_true);
		pcb_buffer_clear(pcb, PCB_PASTEBUFFER);
		pcb_buffer_set_number(pcb_crosshair_note.Buffer);
		pcb_crosshair_note.Moving = pcb_false;
		pcb_crosshair_note.Hit = 0;
	}
}

void pcb_tool_buffer_adjust_attached_objects(pcb_hidlib_t *hl)
{
	pcb_crosshair.AttachedObject.tx = pcb_crosshair.X;
	pcb_crosshair.AttachedObject.ty = pcb_crosshair.Y;
}

void pcb_tool_buffer_draw_attached(pcb_hidlib_t *hl)
{
	pcb_xordraw_buffer(PCB_PASTEBUFFER);
}

/* XPM */
static const char *buf_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c #000000",
". c #7A8584",
"o c None",
/* pixels */
"oooooooo  oo  ooooooo",
"oooooo.. o  o ..ooooo",
"oooooooo oooo ooooooo",
"oooooo.. oooo ..ooooo",
"oooooooo oooo ooooooo",
"oooooo.. oooo ..ooooo",
"oooooooo oooo ooooooo",
"oooooo.. oooo ..ooooo",
"oooooooo oooo ooooooo",
"oooooo.. oooo ..ooooo",
"oooooooo      ooooooo",
"ooooooooooooooooooooo",
"oo     oo ooo o     o",
"ooo ooo o ooo o ooooo",
"ooo ooo o ooo o ooooo",
"ooo ooo o ooo o    oo",
"ooo    oo ooo o ooooo",
"ooo ooo o ooo o ooooo",
"ooo ooo o ooo o ooooo",
"oo     ooo   oo ooooo",
"ooooooooooooooooooooo"
};

pcb_tool_t pcb_tool_buffer = {
	"buffer", NULL, NULL, 100, buf_icon, PCB_TOOL_CURSOR_NAMED("hand"), 0,
	pcb_tool_buffer_init,
	pcb_tool_buffer_uninit,
	pcb_tool_buffer_notify_mode,
	pcb_tool_buffer_release_mode,
	pcb_tool_buffer_adjust_attached_objects,
	pcb_tool_buffer_draw_attached,
	NULL,
	NULL,
	NULL, /* escape */
	
	0
};
