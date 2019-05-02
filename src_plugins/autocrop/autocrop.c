/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "undo.h"
#include "actions.h"
#include "plugins.h"

static const char pcb_acth_autocrop[] = "Autocrops the board dimensions to (extants + a margin of 1 grid), keeping the move and board size grid aligned";
static const char pcb_acts_autocrop[] = "autocrop()";
static fgw_error_t pcb_act_autocrop(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_box_t box;
	pcb_coord_t dx, dy, w, h;

	if (pcb_data_is_empty(PCB->Data))
		return 0;

	pcb_data_bbox(&box, PCB->Data, 0);
	dx = -((box.X1 / PCB->hidlib.grid - 1) * PCB->hidlib.grid);
	dy = -((box.Y1 / PCB->hidlib.grid - 1) * PCB->hidlib.grid);
	w = ((box.X2 + dx) / PCB->hidlib.grid + 2) * PCB->hidlib.grid;
	h = ((box.Y2 + dy) / PCB->hidlib.grid + 2) * PCB->hidlib.grid;

	if ((dx == 0) && (dy == 0) && (w == PCB->hidlib.size_x) && (h == PCB->hidlib.size_y))
		return 0;

	pcb_draw_inhibit_inc();
	pcb_data_clip_inhibit_inc(PCB->Data);
	pcb_data_move(PCB->Data, dx, dy);
	pcb_board_resize(w, h);
	pcb_data_clip_inhibit_dec(PCB->Data, 1);
	pcb_draw_inhibit_dec();

	pcb_undo_inc_serial();
	pcb_hid_redraw();
	pcb_board_set_changed_flag(1);

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t autocrop_action_list[] = {
	{"autocrop", pcb_act_autocrop, pcb_acth_autocrop, pcb_acts_autocrop}
};

char *autocrop_cookie = "autocrop plugin";

PCB_REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie)

int pplg_check_ver_autocrop(int ver_needed) { return 0; }

void pplg_uninit_autocrop(void)
{
	pcb_remove_actions_by_cookie(autocrop_cookie);
}

#include "dolists.h"
int pplg_init_autocrop(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie);
	return 0;
}
