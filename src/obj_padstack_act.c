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
#include "data.h"
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

	pcb_message(PCB_MSG_INFO, "Pad stack registered with ID %d\n", pid);
	
	return 0;
}

static const char pcb_acts_padstackplace[] = "PadstackConvert([proto_id|default], [x, y])";
static const char pcb_acth_padstackplace[] = "Place a pad stack (either proto_id, or if not specified, the default for style)";

int pcb_act_padstackplace(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_cardinal_t pid;
	pcb_padstack_t *ps;

	if (argc > 1) {
		pcb_bool s1, s2;
		x = pcb_get_value(argv[1], "mil", NULL, &s1);
		y = pcb_get_value(argv[2], "mil", NULL, &s2);
		if (!s1 || !s2) {
			pcb_message(PCB_MSG_ERROR, "Error in coordinate format\n");
			return -1;
		}
	}

	if ((argc <= 0) || (strcmp(argv[0], "default") == 0)) {
#warning padstack TODO: style default proto
		pid = 0;
	}
	else {
		char *end;
		pid = strtol(argv[0], &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "Error in proto ID format: need an integer\n");
			return -1;
		}
	}

	if ((pid >= PCB->Data->ps_protos.used) || (PCB->Data->ps_protos.array[pid].in_use == 0)) {
		pcb_message(PCB_MSG_ERROR, "Invalid padstack proto %ld\n", (long)pid);
		return -1;
	}

	ps = pcb_padstack_new(PCB->Data, pid, x, y, pcb_no_flags());
	if (ps == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to place padstack\n");
		return -1;
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_hid_action_t padstack_action_list[] = {
	{"PadstackConvert", 0, pcb_act_padstackconvert,
	 pcb_acth_padstackconvert, pcb_acts_padstackconvert},
	{"PadstackPlace", 0, pcb_act_padstackplace,
	 pcb_acth_padstackplace, pcb_acts_padstackplace}
};

PCB_REGISTER_ACTIONS(padstack_action_list, NULL)
