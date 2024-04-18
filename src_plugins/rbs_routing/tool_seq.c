/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - Rubber Band Stretch Router
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "conf_core.h"
#include <librnd/core/rnd_conf.h>

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "tool_logic.h"

#include "map.h"
#include "seq.h"
#include "seq.c"

#include "tool_seq.h"

static pcb_layer_t *last_layer;

void pcb_tool_seq_init(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	/* TODO: tool activated */
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

void pcb_tool_seq_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	/* TODO: do we need this? */
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

/* creates next point of the route */
static void notify_line(rnd_design_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	switch (pcb_crosshair.AttachedLine.State) {
		case PCB_CH_STATE_FIRST:
			if (pcb->RatDraw)
				return; /* do not route rats */

		/* TODO: start routing */
			last_layer = PCB_CURRLAYER(pcb);
/*			pcb_crosshair.X, pcb_crosshair.Y
			pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;*/
			break;

		case PCB_CH_STATE_SECOND:
			/* TODO: route to next point */
			break;
	}
}

void pcb_tool_seq_notify_mode(rnd_design_t *hl)
{
	/* TODO: consider next */
/*	if (pcb_crosshair.X == pcb_crosshair.AttachedLine.Point1.X && pcb_crosshair.Y == pcb_crosshair.AttachedLine.Point1.Y) {
		rnd_tool_select_by_name(hl, "line");
		return;
	}*/
}

void pcb_tool_seq_adjust_attached_objects(rnd_design_t *hl)
{
	/* TODO: figure if we need this: */
/*
	if (rnd_gui->control_is_pressed(rnd_gui)) {
		pcb_crosshair.AttachedLine.draw = rnd_false;
	}
	else {
		pcb_crosshair.AttachedLine.draw = rnd_true;
		pcb_line_adjust_attached();
	}
*/
}

void pcb_tool_seq_draw_attached(rnd_design_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	/* TODO: draw attached to pcb_crosshair.X, pcb_crosshair.Y, consider new route point */
}

void pcb_tool_seq_escape(rnd_design_t *hl)
{
	rnd_tool_select_by_name(hl, "arrow");
}

/* XPM */
static const char *seq_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 4 1",
"  c #000000",
". c #7A8584",
"X c #6EA5D7",
"O c None",
/* pixels */
"OOOOOOOOOOOOOOOOOOOOO",
"OO...OOOOOOOOOOOOOOOO",
"O.OOO.OOOOOOOOOOOOOOO",
"O.OXXXOOOOOOOOOOOOOOO",
"O.OOO.XXXOOOOOOOOOOOO",
"OO...OOOOXXXOOOO...OO",
"OOOOOOOOOOOOXXX.OOO.O",
"OOOOOXOOOOOOOOOXXXO.O",
"OXXXXXXXXOOOOOO.OOO.O",
"OOOOOXOOOOOOOOOO...OO",
"OOOOOXOOOOOOOOOOOOOOO",
"OOOOOOOOOOOOOOOOOOOOO",
" OOOO   O OOOO O    O",
" OOOOO OO  OOO O OOOO",
" OOOOO OO O OO O OOOO",
" OOOOO OO O OO O OOOO",
" OOOOO OO OO O O   OO",
" OOOOO OO OO O O OOOO",
" OOOOO OO OOO  O OOOO",
"    O   O OOOO O    O",
"OOOOOOOOOOOOOOOOOOOOO"
};


rnd_tool_t pcb_tool_seq = {
	"rbsr_seq", "rubber band stretch route: sequential",
	NULL, 1100, seq_icon, RND_TOOL_CURSOR_NAMED("pencil"), 1,
	pcb_tool_seq_init,
	pcb_tool_seq_uninit,
	pcb_tool_seq_notify_mode,
	NULL,
	pcb_tool_seq_adjust_attached_objects,
	pcb_tool_seq_draw_attached,
	NULL, /* pcb_tool_seq_undo_act, */
	NULL, /* pcb_tool_seq_redo_act, */
	pcb_tool_seq_escape,

	PCB_TLF_RAT
};
