/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "obj_padstack.h"

#include "action_helper.h"
#include "board.h"
#include "hid_actions.h"

static const char pcb_acts_padstackconvert[] = "PadstackConvert(buffer|selected)";
static const char pcb_acth_padstackconvert[] = "Convert selection or current buffer to padstack";

int pcb_act_padstackconvert(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_cardinal_t pid;

	if (argv[0] == NULL)
		PCB_ACT_FAIL(padstackconvert);
	if (strcmp(argv[0], "selected") == 0)
		pid = pcb_padstack_conv_selection(PCB, 0);
	else if (strcmp(argv[0], "buffer") == 0)
		pid = pcb_padstack_conv_buffer(0);
	else
		PCB_ACT_FAIL(padstackconvert);
	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_hid_action_t padstack_action_list[] = {
	{"PadstackConvert", 0, pcb_act_padstackconvert,
	 pcb_acth_padstackconvert, pcb_acts_padstackconvert}
};

PCB_REGISTER_ACTIONS(padstack_action_list, NULL)
