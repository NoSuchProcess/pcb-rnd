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
#include "obj_padstack_inlines.h"

#include "action_helper.h"
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "hid_actions.h"

static const char pcb_acts_padstackconvert[] = "PadstackConvert(buffer|selected, [originx, originy])";
static const char pcb_acth_padstackconvert[] = "Convert selection or current buffer to padstack";

int pcb_act_padstackconvert(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_cardinal_t pid;
	pcb_padstack_proto_t tmp, *p;

	if (argv[0] == NULL)
		PCB_ACT_FAIL(padstackconvert);
	if (strcmp(argv[0], "selected") == 0) {
		if (argc > 2) {
			pcb_bool s1, s2;
			x = pcb_get_value(argv[1], "mil", NULL, &s1);
			y = pcb_get_value(argv[2], "mil", NULL, &s2);
			if (!s1 || !s2) {
				pcb_message(PCB_MSG_ERROR, "Error in coordinate format\n");
				return -1;
			}
		}
		else
			pcb_gui->get_coords("Click at padstack origin", &x, &y);
		pid = pcb_padstack_conv_selection(PCB, 0, x, y);

		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
		pcb_padstack_proto_copy(p, &PCB->Data->ps_protos.array[pid]);
		p->parent = PCB_PASTEBUFFER->Data;
		pid = pcb_padstack_get_proto_id(p); /* should be 0 because of the clear, but just in case... */
	}
	else if (strcmp(argv[0], "buffer") == 0) {

		pid = pcb_padstack_conv_buffer(0);

		/* have to save and restore the prototype around the buffer clear */
		tmp = PCB_PASTEBUFFER->Data->ps_protos.array[pid];
		memset(&PCB_PASTEBUFFER->Data->ps_protos.array[pid], 0, sizeof(PCB_PASTEBUFFER->Data->ps_protos.array[0]));
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		p = pcb_vtpadstack_proto_alloc_append(&PCB_PASTEBUFFER->Data->ps_protos, 1);
		*p = tmp;
		p->parent = PCB_PASTEBUFFER->Data;
		pid = pcb_padstack_get_proto_id(p); /* should be 0 because of the clear, but just in case... */

	}
	else
		PCB_ACT_FAIL(padstackconvert);

	pcb_message(PCB_MSG_INFO, "Pad stack registered with ID %d\n", pid);
	pcb_padstack_new(PCB_PASTEBUFFER->Data, pid, 0, 0, conf_core.design.clearance, pcb_no_flags());
	pcb_set_buffer_bbox(PCB_PASTEBUFFER);
	PCB_PASTEBUFFER->X = PCB_PASTEBUFFER->Y = 0;

	return 0;
}

static const char pcb_acts_padstackplace[] = "PadstackPlace([proto_id|default], [x, y])";
static const char pcb_acth_padstackplace[] = "Place a pad stack (either proto_id, or if not specified, the default for style)";

int pcb_act_padstackplace(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_cardinal_t pid;
	pcb_padstack_t *ps;

	if (argc > 2) {
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

	ps = pcb_padstack_new(PCB->Data, pid, x, y, conf_core.design.clearance, pcb_no_flags());
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
