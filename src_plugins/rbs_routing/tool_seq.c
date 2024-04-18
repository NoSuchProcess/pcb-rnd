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

static rbsr_seq_t seq;

void pcb_tool_seq_init(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	/* TODO: tool activated */
	pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST;
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

void pcb_tool_seq_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	/* TODO: do we need this? */
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

/* click: creates next point of the route */
void pcb_tool_seq_notify_mode(rnd_design_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;
	pcb_layer_t *ly;
	rnd_layer_id_t lid;


	switch(pcb_crosshair.AttachedLine.State) {
		case PCB_CH_STATE_FIRST:
			if (pcb->RatDraw)
				return; /* do not route rats */

			ly = PCB_CURRLAYER(pcb);
			lid = pcb_layer_id(pcb->Data, ly);

			if (rbsr_seq_begin_at(&seq, pcb, lid, pcb_crosshair.X, pcb_crosshair.Y, conf_core.design.line_thickness, conf_core.design.clearance) == 0)
				pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
			break;

		case PCB_CH_STATE_SECOND:
			/* TODO: if (pcb_crosshair.X == pcb_crosshair.AttachedLine.Point1.X && pcb_crosshair.Y == pcb_crosshair.AttachedLine.Point1.Y) 		rnd_tool_select_by_name(hl, "line"); */
			rbsr_seq_accept(&seq);
			break;
	}
}

void pcb_tool_seq_adjust_attached_objects(rnd_design_t *hl)
{
	int rdrw;

	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_SECOND)
		return;

	rbsr_seq_consider(&seq, pcb_crosshair.X, pcb_crosshair.Y, &rdrw);
	if (rdrw)
		rnd_gui->invalidate_all(rnd_gui);
}

void pcb_tool_seq_draw_attached(rnd_design_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (pcb_crosshair.AttachedLine.State != PCB_CH_STATE_SECOND)
		return;

	rnd_render->draw_line(pcb_crosshair.GC, seq.last_x, seq.last_y, pcb_crosshair.X, pcb_crosshair.Y);
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
"OOOOOOOOOOOOOOOXOOOOO",
"OOOOOOOOOOOOOOOXXOOOO",
"OOOOOOOXXXXXXXXXXXOOO",
"OOOOOOXOOOOOOOOXXOOOO",
"OOOOOXOOO.OOOOOXOOOOO",
"OOOOOXOO...OOOOOOOOOO",
"OOOOOXOOO.OOOOOOOOOOO",
"OOOOOXOOOOOOOOOOOOOOO",
"OOOO.X.OOOOOOOOOOOOOO",
"OOOOO.OOOOOOOOOOOOOOO",
"OOOOOOOOOOOOOOOOOOOOO",
"   OOO   O    OO   OO",
" OO O OOOO OOOO OOO O",
" OO O OOOO OOOO OOO O",
" OO O OOOO OOOO OOO O",
"   OOO  OO   OO OOO O",
" OO OOOO O OOOO OOO O",
" OO OOOO O OOOO OO  O",
" OO O   OO    O     O",
"OOOOOOOOOOOOOOOOOOOO "
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
